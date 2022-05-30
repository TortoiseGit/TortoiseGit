/*
 * Platform-independent bits of X11 forwarding.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#include "putty.h"
#include "ssh.h"
#include "channel.h"
#include "tree234.h"

struct XDMSeen {
    unsigned int time;
    unsigned char clientid[6];
};

typedef struct X11Connection {
    unsigned char firstpkt[12];        /* first X data packet */
    tree234 *authtree;
    struct X11Display *disp;
    char *auth_protocol;
    unsigned char *auth_data;
    int data_read, auth_plen, auth_psize, auth_dlen, auth_dsize;
    bool verified;
    bool input_wanted;
    bool no_data_sent_to_x_client;
    char *peer_addr;
    int peer_port;
    SshChannel *c;               /* channel structure held by SSH backend */
    Socket *s;

    Plug plug;
    Channel chan;
} X11Connection;

static int xdmseen_cmp(void *a, void *b)
{
    struct XDMSeen *sa = a, *sb = b;
    return sa->time > sb->time ? 1 :
           sa->time < sb->time ? -1 :
           memcmp(sa->clientid, sb->clientid, sizeof(sa->clientid));
}

struct X11FakeAuth *x11_invent_fake_auth(tree234 *authtree, int authtype)
{
    struct X11FakeAuth *auth = snew(struct X11FakeAuth);
    int i;

    /*
     * This function has the job of inventing a set of X11 fake auth
     * data, and adding it to 'authtree'. We must preserve the
     * property that for any given actual authorisation attempt, _at
     * most one_ thing in the tree can possibly match it.
     *
     * For MIT-MAGIC-COOKIE-1, that's not too difficult: the match
     * criterion is simply that the entire cookie is correct, so we
     * just have to make sure we don't make up two cookies the same.
     * (Vanishingly unlikely, but we check anyway to be sure, and go
     * round again inventing a new cookie if add234 tells us the one
     * we thought of is already in use.)
     *
     * For XDM-AUTHORIZATION-1, it's a little more fiddly. The setup
     * with XA1 is that half the cookie is used as a DES key with
     * which to CBC-encrypt an assortment of stuff. Happily, the stuff
     * encrypted _begins_ with the other half of the cookie, and the
     * IV is always zero, which means that any valid XA1 authorisation
     * attempt for a given cookie must begin with the same cipher
     * block, consisting of the DES ECB encryption of the first half
     * of the cookie using the second half as a key. So we compute
     * that cipher block here and now, and use it as the sorting key
     * for distinguishing XA1 entries in the tree.
     */

    if (authtype == X11_MIT) {
        auth->proto = X11_MIT;

        /* MIT-MAGIC-COOKIE-1. Cookie size is 128 bits (16 bytes). */
        auth->datalen = 16;
        auth->data = snewn(auth->datalen, unsigned char);
        auth->xa1_firstblock = NULL;

        while (1) {
            random_read(auth->data, auth->datalen);
            if (add234(authtree, auth) == auth)
                break;
        }

        auth->xdmseen = NULL;
    } else {
        assert(authtype == X11_XDM);
        auth->proto = X11_XDM;

        /* XDM-AUTHORIZATION-1. Cookie size is 16 bytes; byte 8 is zero. */
        auth->datalen = 16;
        auth->data = snewn(auth->datalen, unsigned char);
        auth->xa1_firstblock = snewn(8, unsigned char);
        memset(auth->xa1_firstblock, 0, 8);

        while (1) {
            random_read(auth->data, 15);
            auth->data[15] = auth->data[8];
            auth->data[8] = 0;

            memcpy(auth->xa1_firstblock, auth->data, 8);
            des_encrypt_xdmauth(auth->data + 9, auth->xa1_firstblock, 8);
            if (add234(authtree, auth) == auth)
                break;
        }

        auth->xdmseen = newtree234(xdmseen_cmp);
    }
    auth->protoname = dupstr(x11_authnames[auth->proto]);
    auth->datastring = snewn(auth->datalen * 2 + 1, char);
    for (i = 0; i < auth->datalen; i++)
        sprintf(auth->datastring + i*2, "%02x",
                auth->data[i]);

    auth->disp = NULL;
    auth->share_cs = NULL;
    auth->share_chan = NULL;

    return auth;
}

void x11_free_fake_auth(struct X11FakeAuth *auth)
{
    if (auth->data)
        smemclr(auth->data, auth->datalen);
    sfree(auth->data);
    sfree(auth->protoname);
    sfree(auth->datastring);
    sfree(auth->xa1_firstblock);
    if (auth->xdmseen != NULL) {
        struct XDMSeen *seen;
        while ((seen = delpos234(auth->xdmseen, 0)) != NULL)
            sfree(seen);
        freetree234(auth->xdmseen);
    }
    sfree(auth);
}

int x11_authcmp(void *av, void *bv)
{
    struct X11FakeAuth *a = (struct X11FakeAuth *)av;
    struct X11FakeAuth *b = (struct X11FakeAuth *)bv;

    if (a->proto < b->proto)
        return -1;
    else if (a->proto > b->proto)
        return +1;

    if (a->proto == X11_MIT) {
        if (a->datalen < b->datalen)
            return -1;
        else if (a->datalen > b->datalen)
            return +1;

        return memcmp(a->data, b->data, a->datalen);
    } else {
        assert(a->proto == X11_XDM);

        return memcmp(a->xa1_firstblock, b->xa1_firstblock, 8);
    }
}

#define XDM_MAXSKEW 20*60      /* 20 minute clock skew should be OK */

static const char *x11_verify(unsigned long peer_ip, int peer_port,
                              tree234 *authtree, char *proto,
                              unsigned char *data, int dlen,
                              struct X11FakeAuth **auth_ret)
{
    struct X11FakeAuth match_dummy;    /* for passing to find234 */
    struct X11FakeAuth *auth;

    /*
     * First, do a lookup in our tree to find the only authorisation
     * record that _might_ match.
     */
    if (!strcmp(proto, x11_authnames[X11_MIT])) {
        /*
         * Just look up the whole cookie that was presented to us,
         * which x11_authcmp will compare against the cookies we
         * currently believe in.
         */
        match_dummy.proto = X11_MIT;
        match_dummy.datalen = dlen;
        match_dummy.data = data;
    } else if (!strcmp(proto, x11_authnames[X11_XDM])) {
        /*
         * Look up the first cipher block, against the stored first
         * cipher blocks for the XDM-AUTHORIZATION-1 cookies we
         * currently know. (See comment in x11_invent_fake_auth.)
         */
        match_dummy.proto = X11_XDM;
        match_dummy.xa1_firstblock = data;
    } else {
        return "Unsupported authorisation protocol";
    }

    if ((auth = find234(authtree, &match_dummy, 0)) == NULL)
        return "Authorisation not recognised";

    /*
     * If we're using MIT-MAGIC-COOKIE-1, that was all we needed. If
     * we're doing XDM-AUTHORIZATION-1, though, we have to check the
     * rest of the auth data.
     */
    if (auth->proto == X11_XDM) {
        unsigned long t;
        time_t tim;
        int i;
        struct XDMSeen *seen, *ret;

        if (dlen != 24)
            return "XDM-AUTHORIZATION-1 data was wrong length";
        if (peer_port == -1)
            return "cannot do XDM-AUTHORIZATION-1 without remote address data";
        des_decrypt_xdmauth(auth->data+9, data, 24);
        if (memcmp(auth->data, data, 8) != 0)
            return "XDM-AUTHORIZATION-1 data failed check"; /* cookie wrong */
        if (GET_32BIT_MSB_FIRST(data+8) != peer_ip)
            return "XDM-AUTHORIZATION-1 data failed check";   /* IP wrong */
        if ((int)GET_16BIT_MSB_FIRST(data+12) != peer_port)
            return "XDM-AUTHORIZATION-1 data failed check";   /* port wrong */
        t = GET_32BIT_MSB_FIRST(data+14);
        for (i = 18; i < 24; i++)
            if (data[i] != 0)          /* zero padding wrong */
                return "XDM-AUTHORIZATION-1 data failed check";
        tim = time(NULL);
        if (((unsigned long)t - (unsigned long)tim
             + XDM_MAXSKEW) > 2*XDM_MAXSKEW)
            return "XDM-AUTHORIZATION-1 time stamp was too far out";
        seen = snew(struct XDMSeen);
        seen->time = t;
        memcpy(seen->clientid, data+8, 6);
        assert(auth->xdmseen != NULL);
        ret = add234(auth->xdmseen, seen);
        if (ret != seen) {
            sfree(seen);
            return "XDM-AUTHORIZATION-1 data replayed";
        }
        /* While we're here, purge entries too old to be replayed. */
        for (;;) {
            seen = index234(auth->xdmseen, 0);
            assert(seen != NULL);
            if (t - seen->time <= XDM_MAXSKEW)
                break;
            sfree(delpos234(auth->xdmseen, 0));
        }
    }
    /* implement other protocols here if ever required */

    *auth_ret = auth;
    return NULL;
}

static void x11_log(Plug *p, PlugLogType type, SockAddr *addr, int port,
                    const char *error_msg, int error_code)
{
    /* We have no interface to the logging module here, so we drop these. */
}

static void x11_send_init_error(struct X11Connection *conn,
                                const char *err_message);

static void x11_closing(Plug *plug, PlugCloseType type, const char *error_msg)
{
    struct X11Connection *xconn = container_of(
        plug, struct X11Connection, plug);

    if (type != PLUGCLOSE_NORMAL) {
        /*
         * Socket error. If we're still at the connection setup stage,
         * construct an X11 error packet passing on the problem.
         */
        if (xconn->no_data_sent_to_x_client) {
            char *err_message = dupprintf("unable to connect to forwarded "
                                          "X server: %s", error_msg);
            x11_send_init_error(xconn, err_message);
            sfree(err_message);
        }

        /*
         * Whether we did that or not, now we slam the connection
         * shut.
         */
        sshfwd_initiate_close(xconn->c, error_msg);
    } else {
        /*
         * Ordinary EOF received on socket. Send an EOF on the SSH
         * channel.
         */
        if (xconn->c)
            sshfwd_write_eof(xconn->c);
    }
}

static void x11_receive(Plug *plug, int urgent, const char *data, size_t len)
{
    struct X11Connection *xconn = container_of(
        plug, struct X11Connection, plug);

    xconn->no_data_sent_to_x_client = false;
    sshfwd_write(xconn->c, data, len);
}

static void x11_sent(Plug *plug, size_t bufsize)
{
    struct X11Connection *xconn = container_of(
        plug, struct X11Connection, plug);

    sshfwd_unthrottle(xconn->c, bufsize);
}

static const PlugVtable X11Connection_plugvt = {
    .log = x11_log,
    .closing = x11_closing,
    .receive = x11_receive,
    .sent = x11_sent,
};

static void x11_chan_free(Channel *chan);
static size_t x11_send(
    Channel *chan, bool is_stderr, const void *vdata, size_t len);
static void x11_send_eof(Channel *chan);
static void x11_set_input_wanted(Channel *chan, bool wanted);
static char *x11_log_close_msg(Channel *chan);

static const ChannelVtable X11Connection_channelvt = {
    .free = x11_chan_free,
    .open_confirmation = chan_remotely_opened_confirmation,
    .open_failed = chan_remotely_opened_failure,
    .send = x11_send,
    .send_eof = x11_send_eof,
    .set_input_wanted = x11_set_input_wanted,
    .log_close_msg = x11_log_close_msg,
    .want_close = chan_default_want_close,
    .rcvd_exit_status = chan_no_exit_status,
    .rcvd_exit_signal = chan_no_exit_signal,
    .rcvd_exit_signal_numeric = chan_no_exit_signal_numeric,
    .run_shell = chan_no_run_shell,
    .run_command = chan_no_run_command,
    .run_subsystem = chan_no_run_subsystem,
    .enable_x11_forwarding = chan_no_enable_x11_forwarding,
    .enable_agent_forwarding = chan_no_enable_agent_forwarding,
    .allocate_pty = chan_no_allocate_pty,
    .set_env = chan_no_set_env,
    .send_break = chan_no_send_break,
    .send_signal = chan_no_send_signal,
    .change_window_size = chan_no_change_window_size,
    .request_response = chan_no_request_response,
};

/*
 * Called to set up the X11Connection structure, though this does not
 * yet connect to an actual server.
 */
Channel *x11_new_channel(tree234 *authtree, SshChannel *c,
                         const char *peeraddr, int peerport,
                         bool connection_sharing_possible)
{
    struct X11Connection *xconn;

    /*
     * Open socket.
     */
    xconn = snew(struct X11Connection);
    xconn->plug.vt = &X11Connection_plugvt;
    xconn->chan.vt = &X11Connection_channelvt;
    xconn->chan.initial_fixed_window_size =
        (connection_sharing_possible ? 128 : 0);
    xconn->auth_protocol = NULL;
    xconn->authtree = authtree;
    xconn->verified = false;
    xconn->data_read = 0;
    xconn->input_wanted = true;
    xconn->no_data_sent_to_x_client = true;
    xconn->c = c;

    /*
     * We don't actually open a local socket to the X server just yet,
     * because we don't know which one it is. Instead, we'll wait
     * until we see the incoming authentication data, which may tell
     * us what display to connect to, or whether we have to divert
     * this X forwarding channel to a connection-sharing downstream
     * rather than handling it ourself.
     */
    xconn->disp = NULL;
    xconn->s = NULL;

    /*
     * Stash the peer address we were given in its original text form.
     */
    xconn->peer_addr = peeraddr ? dupstr(peeraddr) : NULL;
    xconn->peer_port = peerport;

    return &xconn->chan;
}

static void x11_chan_free(Channel *chan)
{
    assert(chan->vt == &X11Connection_channelvt);
    X11Connection *xconn = container_of(chan, X11Connection, chan);

    if (xconn->auth_protocol) {
        sfree(xconn->auth_protocol);
        sfree(xconn->auth_data);
    }

    if (xconn->s)
        sk_close(xconn->s);

    sfree(xconn->peer_addr);
    sfree(xconn);
}

static void x11_set_input_wanted(Channel *chan, bool wanted)
{
    assert(chan->vt == &X11Connection_channelvt);
    X11Connection *xconn = container_of(chan, X11Connection, chan);

    xconn->input_wanted = wanted;
    if (xconn->s)
        sk_set_frozen(xconn->s, !xconn->input_wanted);
}

static void x11_send_init_error(struct X11Connection *xconn,
                                const char *err_message)
{
    char *full_message;
    int msglen, msgsize;
    unsigned char *reply;

    full_message = dupprintf("%s X11 proxy: %s\n", appname, err_message);

    msglen = strlen(full_message);
    reply = snewn(8 + msglen+1 + 4, unsigned char); /* include zero */
    msgsize = (msglen + 3) & ~3;
    reply[0] = 0;              /* failure */
    reply[1] = msglen;         /* length of reason string */
    memcpy(reply + 2, xconn->firstpkt + 2, 4);  /* major/minor proto vsn */
    PUT_16BIT_X11(xconn->firstpkt[0], reply + 6, msgsize >> 2);/* data len */
    memset(reply + 8, 0, msgsize);
    memcpy(reply + 8, full_message, msglen);
    sshfwd_write(xconn->c, reply, 8 + msgsize);
    sshfwd_write_eof(xconn->c);
    xconn->no_data_sent_to_x_client = false;
    sfree(reply);
    sfree(full_message);
}

/*
 * Called to send data down the raw connection.
 */
static size_t x11_send(
    Channel *chan, bool is_stderr, const void *vdata, size_t len)
{
    assert(chan->vt == &X11Connection_channelvt);
    X11Connection *xconn = container_of(chan, X11Connection, chan);
    const char *data = (const char *)vdata;

    /*
     * Read the first packet.
     */
    while (len > 0 && xconn->data_read < 12)
        xconn->firstpkt[xconn->data_read++] = (unsigned char) (len--, *data++);
    if (xconn->data_read < 12)
        return 0;

    /*
     * If we have not allocated the auth_protocol and auth_data
     * strings, do so now.
     */
    if (!xconn->auth_protocol) {
        char endian = xconn->firstpkt[0];
        xconn->auth_plen = GET_16BIT_X11(endian, xconn->firstpkt + 6);
        xconn->auth_dlen = GET_16BIT_X11(endian, xconn->firstpkt + 8);
        xconn->auth_psize = (xconn->auth_plen + 3) & ~3;
        xconn->auth_dsize = (xconn->auth_dlen + 3) & ~3;
        /* Leave room for a terminating zero, to make our lives easier. */
        xconn->auth_protocol = snewn(xconn->auth_psize + 1, char);
        xconn->auth_data = snewn(xconn->auth_dsize, unsigned char);
    }

    /*
     * Read the auth_protocol and auth_data strings.
     */
    while (len > 0 &&
           xconn->data_read < 12 + xconn->auth_psize)
        xconn->auth_protocol[xconn->data_read++ - 12] = (len--, *data++);
    while (len > 0 &&
           xconn->data_read < 12 + xconn->auth_psize + xconn->auth_dsize)
        xconn->auth_data[xconn->data_read++ - 12 -
                      xconn->auth_psize] = (unsigned char) (len--, *data++);
    if (xconn->data_read < 12 + xconn->auth_psize + xconn->auth_dsize)
        return 0;

    /*
     * If we haven't verified the authorisation, do so now.
     */
    if (!xconn->verified) {
        const char *err;
        struct X11FakeAuth *auth_matched = NULL;
        unsigned long peer_ip;
        int peer_port;
        int protomajor, protominor;
        void *greeting;
        int greeting_len;
        unsigned char *socketdata;
        int socketdatalen;
        char new_peer_addr[32];
        int new_peer_port;
        char endian = xconn->firstpkt[0];

        protomajor = GET_16BIT_X11(endian, xconn->firstpkt + 2);
        protominor = GET_16BIT_X11(endian, xconn->firstpkt + 4);

        assert(!xconn->s);

        xconn->auth_protocol[xconn->auth_plen] = '\0';  /* ASCIZ */

        peer_ip = 0;                   /* placate optimiser */
        if (x11_parse_ip(xconn->peer_addr, &peer_ip))
            peer_port = xconn->peer_port;
        else
            peer_port = -1; /* signal no peer address data available */

        err = x11_verify(peer_ip, peer_port,
                         xconn->authtree, xconn->auth_protocol,
                         xconn->auth_data, xconn->auth_dlen, &auth_matched);
        if (err) {
            x11_send_init_error(xconn, err);
            return 0;
        }
        assert(auth_matched);

        /*
         * If this auth points to a connection-sharing downstream
         * rather than an X display we know how to connect to
         * directly, pass it off to the sharing module now. (This will
         * have the side effect of freeing xconn.)
         */
        if (auth_matched->share_cs) {
            sshfwd_x11_sharing_handover(xconn->c, auth_matched->share_cs,
                                        auth_matched->share_chan,
                                        xconn->peer_addr, xconn->peer_port,
                                        xconn->firstpkt[0],
                                        protomajor, protominor, data, len);
            return 0;
        }

        /*
         * Now we know we're going to accept the connection, and what
         * X display to connect to. Actually connect to it.
         */
        xconn->chan.initial_fixed_window_size = 0;
        sshfwd_window_override_removed(xconn->c);
        xconn->disp = auth_matched->disp;
        xconn->s = new_connection(sk_addr_dup(xconn->disp->addr),
                                  xconn->disp->realhost, xconn->disp->port,
                                  false, true, false, false, &xconn->plug,
                                  sshfwd_get_conf(xconn->c), NULL);
        if ((err = sk_socket_error(xconn->s)) != NULL) {
            char *err_message = dupprintf("unable to connect to"
                                          " forwarded X server: %s", err);
            x11_send_init_error(xconn, err_message);
            sfree(err_message);
            return 0;
        }

        /*
         * Write a new connection header containing our replacement
         * auth data.
         */
        socketdatalen = 0;             /* placate compiler warning */
        socketdata = sk_getxdmdata(xconn->s, &socketdatalen);
        if (socketdata && socketdatalen==6) {
            sprintf(new_peer_addr, "%d.%d.%d.%d", socketdata[0],
                    socketdata[1], socketdata[2], socketdata[3]);
            new_peer_port = GET_16BIT_MSB_FIRST(socketdata + 4);
        } else {
            strcpy(new_peer_addr, "0.0.0.0");
            new_peer_port = 0;
        }

        greeting = x11_make_greeting(xconn->firstpkt[0],
                                     protomajor, protominor,
                                     xconn->disp->localauthproto,
                                     xconn->disp->localauthdata,
                                     xconn->disp->localauthdatalen,
                                     new_peer_addr, new_peer_port,
                                     &greeting_len);

        sk_write(xconn->s, greeting, greeting_len);

        smemclr(greeting, greeting_len);
        sfree(greeting);

        /*
         * Now we're done.
         */
        xconn->verified = true;
    }

    /*
     * After initialisation, just copy data simply.
     */

    return sk_write(xconn->s, data, len);
}

static void x11_send_eof(Channel *chan)
{
    assert(chan->vt == &X11Connection_channelvt);
    X11Connection *xconn = container_of(chan, X11Connection, chan);

    if (xconn->s) {
        sk_write_eof(xconn->s);
    } else {
        /*
         * If EOF is received from the X client before we've got to
         * the point of actually connecting to an X server, then we
         * should send an EOF back to the client so that the
         * forwarded channel will be terminated.
         */
        if (xconn->c)
            sshfwd_write_eof(xconn->c);
    }
}

static char *x11_log_close_msg(Channel *chan)
{
    return dupstr("Forwarded X11 connection terminated");
}
