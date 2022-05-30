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

struct Ldisc_tag {
    Terminal *term;
    Backend *backend;
    Seat *seat;

    /*
     * When the backend is not reporting true from sendok(), terminal
     * input that comes here is stored in this bufchain instead. When
     * the backend later decides it wants session input, we empty the
     * queue in ldisc_check_sendok_callback(), passing its contents on
     * to the backend. Before then, we also provide data from this
     * queue to term_get_userpass_input() via ldisc_get_input_token(),
     * to be interpreted as user responses to username and password
     * prompts during authentication.
     *
     * Unfortunately, the data stored in this queue is not all of the
     * same type: our output to the backend consists of both raw bytes
     * sent to backend_send(), and also session specials such as
     * SS_EOL and SS_EC. So we have to encode our queued data in a way
     * that can represent both.
     *
     * The encoding is private to this source file, so we can change
     * it if necessary and only have to worry about the encode and
     * decode functions here. Currently, it is:
     *
     *  - Bytes other than 0xFF are stored literally.
     *  - The byte 0xFF itself is stored as 0xFF 0xFF.
     *  - A session special (code, arg) is stored as 0xFF, followed by
     *    a big-endian 4-byte integer containing code, followed by
     *    another big-endian 4-byte integer containing arg.
     *
     * (This representation relies on session special codes being at
     * most 0xFEFFFFFF when represented in 32 bits, so that the first
     * byte of the 'code' integer can't be confused with the 0xFF
     * followup byte indicating a literal 0xFF, But since session
     * special codes are defined by an enum counting up from zero, and
     * there are only a couple of dozen of them, that shouldn't be a
     * problem! Even so, just in case, an assertion checks that at
     * encode time.)
     */
    bufchain input_queue;

    IdempotentCallback input_queue_callback;
    prompts_t *prompts;

    /*
     * Values cached out of conf.
     */
    bool telnet_keyboard, telnet_newline;
    int protocol, localecho, localedit;

    char *buf;
    size_t buflen, bufsiz;
    bool quotenext;
};

#define ECHOING (ldisc->localecho == FORCE_ON || \
                 (ldisc->localecho == AUTO && \
                      (backend_ldisc_option_state(ldisc->backend, LD_ECHO))))
#define EDITING (ldisc->localedit == FORCE_ON || \
                 (ldisc->localedit == AUTO && \
                      (backend_ldisc_option_state(ldisc->backend, LD_EDIT))))

static void c_write(Ldisc *ldisc, const void *buf, int len)
{
    seat_stdout(ldisc->seat, buf, len);
}

static int plen(Ldisc *ldisc, unsigned char c)
{
    if ((c >= 32 && c <= 126) || (c >= 160 && !in_utf(ldisc->term)))
        return 1;
    else if (c < 128)
        return 2;                      /* ^x for some x */
    else if (in_utf(ldisc->term) && c >= 0xC0)
        return 1;                      /* UTF-8 introducer character
                                        * (FIXME: combining / wide chars) */
    else if (in_utf(ldisc->term) && c >= 0x80 && c < 0xC0)
        return 0;                      /* UTF-8 followup character */
    else
        return 4;                      /* <XY> hex representation */
}

static void pwrite(Ldisc *ldisc, unsigned char c)
{
    if ((c >= 32 && c <= 126) ||
        (!in_utf(ldisc->term) && c >= 0xA0) ||
        (in_utf(ldisc->term) && c >= 0x80)) {
        c_write(ldisc, &c, 1);
    } else if (c < 128) {
        char cc[2];
        cc[1] = (c == 127 ? '?' : c + 0x40);
        cc[0] = '^';
        c_write(ldisc, cc, 2);
    } else {
        char cc[5];
        sprintf(cc, "<%02X>", c);
        c_write(ldisc, cc, 4);
    }
}

static bool char_start(Ldisc *ldisc, unsigned char c)
{
    if (in_utf(ldisc->term))
        return (c < 0x80 || c >= 0xC0);
    else
        return true;
}

static void bsb(Ldisc *ldisc, int n)
{
    while (n--)
        c_write(ldisc, "\010 \010", 3);
}

static void ldisc_input_queue_callback(void *ctx);

#define CTRL(x) (x^'@')
#define KCTRL(x) ((x^'@') | 0x100)

Ldisc *ldisc_create(Conf *conf, Terminal *term, Backend *backend, Seat *seat)
{
    Ldisc *ldisc = snew(Ldisc);

    ldisc->buf = NULL;
    ldisc->buflen = 0;
    ldisc->bufsiz = 0;
    ldisc->quotenext = false;

    ldisc->backend = backend;
    ldisc->term = term;
    ldisc->seat = seat;

    bufchain_init(&ldisc->input_queue);

    ldisc->prompts = NULL;
    ldisc->input_queue_callback.fn = ldisc_input_queue_callback;
    ldisc->input_queue_callback.ctx = ldisc;
    ldisc->input_queue_callback.queued = false;
    bufchain_set_callback(&ldisc->input_queue, &ldisc->input_queue_callback);

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
}

void ldisc_free(Ldisc *ldisc)
{
    bufchain_clear(&ldisc->input_queue);
    if (ldisc->term)
        ldisc->term->ldisc = NULL;
    if (ldisc->backend)
        backend_provide_ldisc(ldisc->backend, NULL);
    if (ldisc->buf)
        sfree(ldisc->buf);
    if (ldisc->prompts && ldisc->prompts->ldisc_ptr_to_us == &ldisc->prompts)
        ldisc->prompts->ldisc_ptr_to_us = NULL;
    delete_callbacks_for_context(ldisc);
    sfree(ldisc);
}

void ldisc_echoedit_update(Ldisc *ldisc)
{
    seat_echoedit_update(ldisc->seat, ECHOING, EDITING);
}

void ldisc_enable_prompt_callback(Ldisc *ldisc, prompts_t *prompts)
{
    /*
     * Called by the terminal to indicate that there's a prompts_t
     * currently in flight, or to indicate that one has just finished
     * (by passing NULL). When ldisc->prompts is not null, we notify
     * the terminal whenever new data arrives in our input queue, so
     * that it can continue the interactive prompting process.
     */
    ldisc->prompts = prompts;
    if (prompts)
        ldisc->prompts->ldisc_ptr_to_us = &ldisc->prompts;
}

static void ldisc_input_queue_callback(void *ctx)
{
    /*
     * Toplevel callback that is triggered whenever the input queue
     * lengthens. If we're currently processing an interactive prompt,
     * we call back the Terminal to tell it to do some more stuff with
     * that prompt based on the new input.
     */
    Ldisc *ldisc = (Ldisc *)ctx;
    if (ldisc->term && ldisc->prompts) {
        /*
         * The integer return value from this call is discarded,
         * because we have no channel to pass it on to the backend
         * that originally wanted it. But that's OK, because if the
         * return value is >= 0 (that is, the prompts are either
         * completely filled in, or aborted by the user), then the
         * terminal will notify the callback in the prompts_t, and
         * when that calls term_get_userpass_input again, it will
         * return the same answer again.
         */
        term_get_userpass_input(ldisc->term, ldisc->prompts);
    }
}

static void ldisc_to_backend_raw(
    Ldisc *ldisc, const void *vbuf, size_t len)
{
    if (backend_sendok(ldisc->backend)) {
        backend_send(ldisc->backend, vbuf, len);
    } else {
        const char *buf = (const char *)vbuf;
        while (len > 0) {
            /*
             * Encode raw data in input_queue, by storing large chunks
             * as long as they don't include 0xFF, and pausing every
             * time they do to escape it.
             */
            const char *ff = memchr(buf, '\xFF', len);
            size_t this_len = ff ? ff - buf : len;
            if (this_len > 0) {
                bufchain_add(&ldisc->input_queue, buf, len);
            } else {
                bufchain_add(&ldisc->input_queue, "\xFF\xFF", 2);
                this_len = 1;
            }
            buf += this_len;
            len -= this_len;
        }
    }
}

static void ldisc_to_backend_special(
    Ldisc *ldisc, SessionSpecialCode code, int arg)
{
    if (backend_sendok(ldisc->backend)) {
        backend_special(ldisc->backend, code, arg);
    } else {
        /*
         * Encode a session special in input_queue.
         */
        unsigned char data[9];
        data[0] = 0xFF;
        PUT_32BIT_MSB_FIRST(data+1, code);
        PUT_32BIT_MSB_FIRST(data+5, arg);
        assert(data[1] != 0xFF &&
               "SessionSpecialCode encoding collides with FF FF escape");
        bufchain_add(&ldisc->input_queue, data, 9);
    }
}

bool ldisc_has_input_buffered(Ldisc *ldisc)
{
    return bufchain_size(&ldisc->input_queue) > 0;
}

LdiscInputToken ldisc_get_input_token(Ldisc *ldisc)
{
    assert(bufchain_size(&ldisc->input_queue) > 0 &&
           "You're not supposed to call this unless there is buffered input!");

    LdiscInputToken tok;

    char c;
    bufchain_fetch_consume(&ldisc->input_queue, &c, 1);
    if (c != '\xFF') {
        /* A literal non-FF byte */
        tok.is_special = false;
        tok.chr = c;
        return tok;
    } else {
        char data[8];

        /* See if the byte after the FF is also FF, indicating a literal FF */
        bufchain_fetch_consume(&ldisc->input_queue, data, 1);
        if (data[0] == '\xFF') {
            tok.is_special = false;
            tok.chr = '\xFF';
            return tok;
        }

        /* If not, get the rest of an 8-byte chunk and decode a special */
        bufchain_fetch_consume(&ldisc->input_queue, data+1, 7);
        tok.is_special = true;
        tok.code = GET_32BIT_MSB_FIRST(data);
        tok.arg = toint(GET_32BIT_MSB_FIRST(data+4));
        return tok;
    }
}

static void ldisc_check_sendok_callback(void *ctx)
{
    Ldisc *ldisc = (Ldisc *)ctx;

    if (!(ldisc->backend && backend_sendok(ldisc->backend)))
        return;

    /*
     * Flush the ldisc input queue into the backend, which is now
     * willing to receive the data.
     */
    while (bufchain_size(&ldisc->input_queue) > 0) {
        /*
         * Process either a chunk of non-special data, or an FF
         * escape, depending on whether the first thing we see is an
         * FF byte.
         */
        ptrlen data = bufchain_prefix(&ldisc->input_queue);
        const char *ff = memchr(data.ptr, '\xFF', data.len);
        if (ff != data.ptr) {
            /* Send a maximal block of data not containing any
             * difficult bytes. */
            if (ff)
                data.len = ff - (const char *)data.ptr;
            backend_send(ldisc->backend, data.ptr, data.len);
            bufchain_consume(&ldisc->input_queue, data.len);
        } else {
            /* Decode either a special or an escaped FF byte. The
             * easiest way to do this is to reuse the decoding code
             * already in ldisc_get_input_token. */
            LdiscInputToken tok = ldisc_get_input_token(ldisc);
            if (tok.is_special)
                backend_special(ldisc->backend, tok.code, tok.arg);
            else
                backend_send(ldisc->backend, &tok.chr, 1);
        }
    }
}

void ldisc_check_sendok(Ldisc *ldisc)
{
    queue_toplevel_callback(ldisc_check_sendok_callback, ldisc);
}

void ldisc_send(Ldisc *ldisc, const void *vbuf, int len, bool interactive)
{
    const char *buf = (const char *)vbuf;
    int keyflag = 0;

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

    /*
     * Less than zero means null terminated special string.
     */
    if (len < 0) {
        len = strlen(buf);
        keyflag = KCTRL('@');
    }
    /*
     * Either perform local editing, or just send characters.
     */
    if (EDITING) {
        while (len--) {
            int c;
            c = (unsigned char)(*buf++) + keyflag;
            if (!interactive && c == '\r')
                c += KCTRL('@');
            switch (ldisc->quotenext ? ' ' : c) {
                /*
                 * ^h/^?: delete, and output BSBs, to return to
                 * last character boundary (in UTF-8 mode this may
                 * be more than one byte)
                 * ^w: delete, and output BSBs, to return to last
                 * space/nonspace boundary
                 * ^u: delete, and output BSBs, to return to BOL
                 * ^c: Do a ^u then send a telnet IP
                 * ^z: Do a ^u then send a telnet SUSP
                 * ^\: Do a ^u then send a telnet ABORT
                 * ^r: echo "^R\n" and redraw line
                 * ^v: quote next char
                 * ^d: if at BOL, end of file and close connection,
                 * else send line and reset to BOL
                 * ^m: send line-plus-\r\n and reset to BOL
                 */
              case KCTRL('H'):
              case KCTRL('?'):         /* backspace/delete */
                if (ldisc->buflen > 0) {
                    do {
                        if (ECHOING)
                            bsb(ldisc, plen(ldisc, ldisc->buf[ldisc->buflen - 1]));
                        ldisc->buflen--;
                    } while (!char_start(ldisc, ldisc->buf[ldisc->buflen]));
                }
                break;
              case CTRL('W'):          /* delete word */
                while (ldisc->buflen > 0) {
                    if (ECHOING)
                        bsb(ldisc, plen(ldisc, ldisc->buf[ldisc->buflen - 1]));
                    ldisc->buflen--;
                    if (ldisc->buflen > 0 &&
                        isspace((unsigned char)ldisc->buf[ldisc->buflen-1]) &&
                        !isspace((unsigned char)ldisc->buf[ldisc->buflen]))
                        break;
                }
                break;
              case CTRL('U'):          /* delete line */
              case CTRL('C'):          /* Send IP */
              case CTRL('\\'):         /* Quit */
              case CTRL('Z'):          /* Suspend */
                while (ldisc->buflen > 0) {
                    if (ECHOING)
                        bsb(ldisc, plen(ldisc, ldisc->buf[ldisc->buflen - 1]));
                    ldisc->buflen--;
                }
                ldisc_to_backend_special(ldisc, SS_EL, 0);
                /*
                 * We don't send IP, SUSP or ABORT if the user has
                 * configured telnet specials off! This breaks
                 * talkers otherwise.
                 */
                if (!ldisc->telnet_keyboard)
                    goto default_case;
                if (c == CTRL('C'))
                    ldisc_to_backend_special(ldisc, SS_IP, 0);
                if (c == CTRL('Z'))
                    ldisc_to_backend_special(ldisc, SS_SUSP, 0);
                if (c == CTRL('\\'))
                    ldisc_to_backend_special(ldisc, SS_ABORT, 0);
                break;
              case CTRL('R'):          /* redraw line */
                if (ECHOING) {
                    int i;
                    c_write(ldisc, "^R\r\n", 4);
                    for (i = 0; i < ldisc->buflen; i++)
                        pwrite(ldisc, ldisc->buf[i]);
                }
                break;
              case CTRL('V'):          /* quote next char */
                ldisc->quotenext = true;
                break;
              case CTRL('D'):          /* logout or send */
                if (ldisc->buflen == 0) {
                    ldisc_to_backend_special(ldisc, SS_EOF, 0);
                } else {
                    ldisc_to_backend_raw(ldisc, ldisc->buf, ldisc->buflen);
                    ldisc->buflen = 0;
                }
                break;
                /*
                 * This particularly hideous bit of code from RDB
                 * allows ordinary ^M^J to do the same thing as
                 * magic-^M when in Raw protocol. The line `case
                 * KCTRL('M'):' is _inside_ the if block. Thus:
                 *
                 *  - receiving regular ^M goes straight to the
                 *    default clause and inserts as a literal ^M.
                 *  - receiving regular ^J _not_ directly after a
                 *    literal ^M (or not in Raw protocol) fails the
                 *    if condition, leaps to the bottom of the if,
                 *    and falls through into the default clause
                 *    again.
                 *  - receiving regular ^J just after a literal ^M
                 *    in Raw protocol passes the if condition,
                 *    deletes the literal ^M, and falls through
                 *    into the magic-^M code
                 *  - receiving a magic-^M empties the line buffer,
                 *    signals end-of-line in one of the various
                 *    entertaining ways, and _doesn't_ fall out of
                 *    the bottom of the if and through to the
                 *    default clause because of the break.
                 */
              case CTRL('J'):
                if (ldisc->protocol == PROT_RAW &&
                    ldisc->buflen > 0 && ldisc->buf[ldisc->buflen - 1] == '\r') {
                    if (ECHOING)
                        bsb(ldisc, plen(ldisc, ldisc->buf[ldisc->buflen - 1]));
                    ldisc->buflen--;
                    /* FALLTHROUGH */
              case KCTRL('M'):         /* send with newline */
                    if (ldisc->buflen > 0)
                        ldisc_to_backend_raw(ldisc, ldisc->buf, ldisc->buflen);
                    if (ldisc->protocol == PROT_RAW)
                        ldisc_to_backend_raw(ldisc, "\r\n", 2);
                    else if (ldisc->protocol == PROT_TELNET && ldisc->telnet_newline)
                        ldisc_to_backend_special(ldisc, SS_EOL, 0);
                    else
                        ldisc_to_backend_raw(ldisc, "\r", 1);
                    if (ECHOING)
                        c_write(ldisc, "\r\n", 2);
                    ldisc->buflen = 0;
                    break;
                }
                /* FALLTHROUGH */
              default:                 /* get to this label from ^V handler */
                default_case:
                sgrowarray(ldisc->buf, ldisc->bufsiz, ldisc->buflen);
                ldisc->buf[ldisc->buflen++] = c;
                if (ECHOING)
                    pwrite(ldisc, (unsigned char) c);
                ldisc->quotenext = false;
                break;
            }
        }
    } else {
        if (ldisc->buflen != 0) {
            ldisc_to_backend_raw(ldisc, ldisc->buf, ldisc->buflen);
            while (ldisc->buflen > 0) {
                bsb(ldisc, plen(ldisc, ldisc->buf[ldisc->buflen - 1]));
                ldisc->buflen--;
            }
        }
        if (len > 0) {
            if (ECHOING)
                c_write(ldisc, buf, len);
            if (keyflag && ldisc->protocol == PROT_TELNET && len == 1) {
                switch (buf[0]) {
                  case CTRL('M'):
                    if (ldisc->protocol == PROT_TELNET && ldisc->telnet_newline)
                        ldisc_to_backend_special(ldisc, SS_EOL, 0);
                    else
                        ldisc_to_backend_raw(ldisc, "\r", 1);
                    break;
                  case CTRL('?'):
                  case CTRL('H'):
                    if (ldisc->telnet_keyboard) {
                        ldisc_to_backend_special(ldisc, SS_EC, 0);
                        break;
                    }
                  case CTRL('C'):
                    if (ldisc->telnet_keyboard) {
                        ldisc_to_backend_special(ldisc, SS_IP, 0);
                        break;
                    }
                  case CTRL('Z'):
                    if (ldisc->telnet_keyboard) {
                        ldisc_to_backend_special(ldisc, SS_SUSP, 0);
                        break;
                    }

                  default:
                    ldisc_to_backend_raw(ldisc, buf, len);
                    break;
                }
            } else
                ldisc_to_backend_raw(ldisc, buf, len);
        }
    }
}
