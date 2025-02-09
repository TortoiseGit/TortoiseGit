/*
 * ldisc.c: PuTTY line discipline. Sits between the input coming
 * from keypresses in the window, and the output channel leading to
 * the back end. Implements echo and/or local line editing,
 * depending on what's currently configured.
 */

#include <stdio.h>
#include <ctype.h>
#include <assert.h>

#include "putty.h"
#include "terminal.h"

typedef enum InputType { NORMAL, DEDICATED, NONINTERACTIVE } InputType;

struct input_chunk {
    struct input_chunk *next;
    InputType type;
    size_t size;
};

struct Ldisc_tag {
    Terminal *term;
    Backend *backend;
    Seat *seat;

    /*
     * When the backend is not reporting true from sendok(), we must
     * buffer the input received by ldisc_send(). It's stored in the
     * bufchain below, together with a linked list of input_chunk
     * blocks storing the extra metadata about special keys and
     * interactivity that ldisc_send() receives.
     *
     * All input is added to this buffer initially, but we then
     * process as much of it as possible immediately and hand it off
     * to the backend or a TermLineEditor. Anything left stays in this
     * buffer until ldisc_check_sendok() is next called, triggering a
     * run of the callback that tries again to process the queue.
     */
    bufchain input_queue;
    struct input_chunk *inchunk_head, *inchunk_tail;

    IdempotentCallback input_queue_callback;

    /*
     * Values cached out of conf.
     */
    bool telnet_keyboard, telnet_newline;
    int protocol, localecho, localedit;

    TermLineEditor *le;
    TermLineEditorCallbackReceiver le_rcv;

    /* We get one of these communicated to us by
     * term_get_userpass_input while it's reading a prompt, so that we
     * can push data straight into it */
    TermLineEditor *userpass_le;
};

#define ECHOING (ldisc->localecho == FORCE_ON || \
                 (ldisc->localecho == AUTO && \
                      (backend_ldisc_option_state(ldisc->backend, LD_ECHO))))
#define EDITING (ldisc->localedit == FORCE_ON || \
                 (ldisc->localedit == AUTO && \
                      (backend_ldisc_option_state(ldisc->backend, LD_EDIT))))

static void ldisc_input_queue_callback(void *ctx);

static const TermLineEditorCallbackReceiverVtable ldisc_lineedit_receiver_vt;

#define CTRL(x) (x^'@')

Ldisc *ldisc_create(Conf *conf, Terminal *term, Backend *backend, Seat *seat)
{
    Ldisc *ldisc = snew(Ldisc);
    memset(ldisc, 0, sizeof(Ldisc));

    ldisc->backend = backend;
    ldisc->term = term;
    ldisc->seat = seat;

    bufchain_init(&ldisc->input_queue);

    ldisc->input_queue_callback.fn = ldisc_input_queue_callback;
    ldisc->input_queue_callback.ctx = ldisc;
    bufchain_set_callback(&ldisc->input_queue, &ldisc->input_queue_callback);

    if (ldisc->term) {
        ldisc->le_rcv.vt = &ldisc_lineedit_receiver_vt;
        ldisc->le = lineedit_new(ldisc->term, 0, &ldisc->le_rcv);
    }

    ldisc_configure(ldisc, conf);

    /* Link ourselves into the backend and the terminal */
    if (term)
        term->ldisc = ldisc;
    if (backend)
        backend_provide_ldisc(backend, ldisc);

    return ldisc;
}

void ldisc_configure(Ldisc *ldisc, Conf *conf)
{
    ldisc->telnet_keyboard = conf_get_bool(conf, CONF_telnet_keyboard);
    ldisc->telnet_newline = conf_get_bool(conf, CONF_telnet_newline);
    ldisc->protocol = conf_get_int(conf, CONF_protocol);
    ldisc->localecho = conf_get_int(conf, CONF_localecho);
    ldisc->localedit = conf_get_int(conf, CONF_localedit);

    unsigned flags = 0;
    if (ldisc->protocol == PROT_RAW)
        flags |= LE_CRLF_NEWLINE;
    if (ldisc->telnet_keyboard)
        flags |= LE_INTERRUPT | LE_SUSPEND | LE_ABORT;
    lineedit_modify_flags(ldisc->le, ~0U, flags);
}

void ldisc_free(Ldisc *ldisc)
{
    bufchain_clear(&ldisc->input_queue);
    while (ldisc->inchunk_head) {
        struct input_chunk *oldhead = ldisc->inchunk_head;
        ldisc->inchunk_head = ldisc->inchunk_head->next;
        sfree(oldhead);
    }
    lineedit_free(ldisc->le);
    if (ldisc->term)
        ldisc->term->ldisc = NULL;
    if (ldisc->backend)
        backend_provide_ldisc(ldisc->backend, NULL);
    delete_callbacks_for_context(ldisc);
    sfree(ldisc);
}

void ldisc_echoedit_update(Ldisc *ldisc)
{
    seat_echoedit_update(ldisc->seat, ECHOING, EDITING);

    /*
     * If we've just turned off local line editing mode, and our
     * TermLineEditor had a partial buffer, then send the contents of
     * the buffer. Rationale: (a) otherwise you lose data; (b) the
     * user quite likely typed the buffer contents _anticipating_ that
     * local editing would be turned off shortly, and the event was
     * slow arriving.
     */
    if (!EDITING)
        lineedit_send_line(ldisc->le);
}

void ldisc_provide_userpass_le(Ldisc *ldisc, TermLineEditor *le)
{
    /*
     * Called by term_get_userpass_input to tell us when it has its
     * own TermLineEditor processing a password prompt, so that we can
     * inject our input into that instead of putting it into our own
     * TermLineEditor or sending it straight to the backend.
     */
    ldisc->userpass_le = le;
}

static inline bool is_dedicated_byte(char c, InputType type)
{
    switch (type) {
      case DEDICATED:
        return true;
      case NORMAL:
        return false;
      case NONINTERACTIVE:
        /*
         * Non-interactive input (e.g. from a paste) doesn't come with
         * the ability to distinguish dedicated keypresses like Return
         * from generic ones like Ctrl+M. So we just have to make up
         * an answer to this question. In particular, we _must_ treat
         * Ctrl+M as the Return key, because that's the only way a
         * newline can be pasted at all.
         */
        return c == '\r';
      default:
        unreachable("those values should be exhaustive");
    }
}

static void ldisc_input_queue_consume(Ldisc *ldisc, size_t size)
{
    bufchain_consume(&ldisc->input_queue, size);
    while (size > 0) {
        size_t thissize = (size < ldisc->inchunk_head->size ?
                           size : ldisc->inchunk_head->size);
        ldisc->inchunk_head->size -= thissize;
        size -= thissize;

        if (!ldisc->inchunk_head->size) {
            struct input_chunk *oldhead = ldisc->inchunk_head;
            ldisc->inchunk_head = ldisc->inchunk_head->next;
            if (!ldisc->inchunk_head)
                ldisc->inchunk_tail = NULL;
            sfree(oldhead);
        }
    }
}

static void ldisc_lineedit_to_terminal(
    TermLineEditorCallbackReceiver *rcv, ptrlen data)
{
    Ldisc *ldisc = container_of(rcv, Ldisc, le_rcv);
    if (ECHOING)
        seat_stdout(ldisc->seat, data.ptr, data.len);
}

static void ldisc_lineedit_to_backend(
    TermLineEditorCallbackReceiver *rcv, ptrlen data)
{
    Ldisc *ldisc = container_of(rcv, Ldisc, le_rcv);
    backend_send(ldisc->backend, data.ptr, data.len);
}

static void ldisc_lineedit_special(
    TermLineEditorCallbackReceiver *rcv, SessionSpecialCode code, int arg)
{
    Ldisc *ldisc = container_of(rcv, Ldisc, le_rcv);
    backend_special(ldisc->backend, code, arg);
}

static void ldisc_lineedit_newline(TermLineEditorCallbackReceiver *rcv)
{
    Ldisc *ldisc = container_of(rcv, Ldisc, le_rcv);
    if (ldisc->protocol == PROT_RAW)
        backend_send(ldisc->backend, "\r\n", 2);
    else if (ldisc->protocol == PROT_TELNET && ldisc->telnet_newline)
        backend_special(ldisc->backend, SS_EOL, 0);
    else
        backend_send(ldisc->backend, "\r", 1);
}

static const TermLineEditorCallbackReceiverVtable
ldisc_lineedit_receiver_vt = {
    .to_terminal = ldisc_lineedit_to_terminal,
    .to_backend = ldisc_lineedit_to_backend,
    .special = ldisc_lineedit_special,
    .newline = ldisc_lineedit_newline,
};

void ldisc_check_sendok(Ldisc *ldisc)
{
    queue_idempotent_callback(&ldisc->input_queue_callback);
}

void ldisc_send(Ldisc *ldisc, const void *vbuf, int len, bool interactive)
{
    assert(ldisc->term);

    if (interactive) {
        /*
         * Interrupt a paste from the clipboard, if one was in
         * progress when the user pressed a key. This is easier than
         * buffering the current piece of data and saving it until the
         * terminal has finished pasting, and has the potential side
         * benefit of permitting a user to cancel an accidental huge
         * paste.
         */
        term_nopaste(ldisc->term);
    }

    InputType type;
    if (len < 0) {
        /*
         * Less than zero means null terminated special string.
         */
        len = strlen(vbuf);
        type = DEDICATED;
    } else if (len > 0) {
        type = interactive ? NORMAL : NONINTERACTIVE;
    } else {
        return; /* nothing to do anyway */
    }

    /*
     * Append our data to input_queue, and ensure it's marked with the
     * right type.
     */
    bufchain_add(&ldisc->input_queue, vbuf, len);
    if (!(ldisc->inchunk_tail && ldisc->inchunk_tail->type == type)) {
        struct input_chunk *new_chunk = snew(struct input_chunk);

        new_chunk->type = type;
        new_chunk->size = 0;

        new_chunk->next = NULL;
        if (ldisc->inchunk_tail)
            ldisc->inchunk_tail->next = new_chunk;
        else
            ldisc->inchunk_head = new_chunk;
        ldisc->inchunk_tail = new_chunk;
    }
    ldisc->inchunk_tail->size += len;

    /*
     * And process as much of the data immediately as we can.
     */
    ldisc_input_queue_callback(ldisc);
}

static void ldisc_input_queue_callback(void *ctx)
{
    Ldisc *ldisc = (Ldisc *)ctx;

    /*
     * Toplevel callback that is triggered whenever the input queue
     * lengthens.
     */
    while (bufchain_size(&ldisc->input_queue)) {
        ptrlen pl = bufchain_prefix(&ldisc->input_queue);
        const char *start = pl.ptr, *buf = pl.ptr;
        size_t len = (pl.len < ldisc->inchunk_head->size ?
                      pl.len : ldisc->inchunk_head->size);
        InputType type = ldisc->inchunk_head->type;

        while (len > 0 && ldisc->userpass_le) {
            char c = *buf++;
            len--;

            bool dedicated = is_dedicated_byte(c, type);
            lineedit_input(ldisc->userpass_le, c, dedicated);
        }

        if (!backend_sendok(ldisc->backend)) {
            ldisc_input_queue_consume(ldisc, buf - start);
            break;
        }

        /*
         * Either perform local editing, or just send characters.
         */
        if (EDITING) {
            while (len > 0) {
                char c = *buf++;
                len--;

                bool dedicated = is_dedicated_byte(c, type);
                lineedit_input(ldisc->le, c, dedicated);
            }
            ldisc_input_queue_consume(ldisc, buf - start);
        } else {
            if (ECHOING)
                seat_stdout(ldisc->seat, buf, len);
            if (type == DEDICATED && ldisc->protocol == PROT_TELNET) {
                while (len > 0) {
                    char c = *buf++;
                    len--;
                    switch (c) {
                      case CTRL('M'):
                        if (ldisc->telnet_newline)
                            backend_special(ldisc->backend, SS_EOL, 0);
                        else
                            backend_send(ldisc->backend, "\r", 1);
                        break;
                      case CTRL('?'):
                      case CTRL('H'):
                        if (ldisc->telnet_keyboard) {
                            backend_special(ldisc->backend, SS_EC, 0);
                            break;
                        }
                      case CTRL('C'):
                        if (ldisc->telnet_keyboard) {
                            backend_special(ldisc->backend, SS_IP, 0);
                            break;
                        }
                      case CTRL('Z'):
                        if (ldisc->telnet_keyboard) {
                            backend_special(ldisc->backend, SS_SUSP, 0);
                            break;
                        }

                      default:
                        backend_send(ldisc->backend, &c, 1);
                        break;
                    }
                }
                ldisc_input_queue_consume(ldisc, buf - start);
            } else {
                backend_send(ldisc->backend, buf, len);
                ldisc_input_queue_consume(ldisc, len);
            }
        }
    }
}
