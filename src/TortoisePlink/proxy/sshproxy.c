/*
 * sshproxy.c: implement a Socket type that talks to an entire
 * subsidiary SSH connection (sometimes called a 'jump host').
 */

#include <stdio.h>
#include <assert.h>

#include "putty.h"
#include "ssh.h"
#include "network.h"
#include "storage.h"

const bool ssh_proxy_supported = true;

typedef struct SshProxy {
    char *errmsg;
    Conf *conf;
    LogContext *logctx;
    Backend *backend;
    LogPolicy *clientlp;
    Seat *clientseat;
    Interactor *clientitr;

    bool got_proxy_password, tried_proxy_password;
    char *proxy_password;

    ProxyStderrBuf psb;
    Plug *plug;

    bool frozen;
    bufchain ssh_to_socket;
    bool rcvd_eof_ssh_to_socket, sent_eof_ssh_to_socket;
    bool conn_established;

    SockAddr *addr;
    int port;

    /* Traits implemented: we're a Socket from the point of view of
     * the client connection, and a Seat from the POV of the SSH
     * backend we instantiate. */
    Socket sock;
    LogPolicy logpolicy;
    Seat seat;
} SshProxy;

static Plug *sshproxy_plug(Socket *s, Plug *p)
{
    SshProxy *sp = container_of(s, SshProxy, sock);
    Plug *oldplug = sp->plug;
    if (p)
        sp->plug = p;
    return oldplug;
}

static void sshproxy_close(Socket *s)
{
    SshProxy *sp = container_of(s, SshProxy, sock);

    sk_addr_free(sp->addr);
    sfree(sp->errmsg);
    conf_free(sp->conf);
    if (sp->backend)
        backend_free(sp->backend);
    if (sp->logctx)
        log_free(sp->logctx);
    if (sp->proxy_password)
        burnstr(sp->proxy_password);
    bufchain_clear(&sp->ssh_to_socket);

    delete_callbacks_for_context(sp);
    sfree(sp);
}

static size_t sshproxy_write(Socket *s, const void *data, size_t len)
{
    SshProxy *sp = container_of(s, SshProxy, sock);
    if (!sp->backend)
        return 0;
    backend_send(sp->backend, data, len);
    return backend_sendbuffer(sp->backend);
}

static size_t sshproxy_write_oob(Socket *s, const void *data, size_t len)
{
    /*
     * oob data is treated as inband; nasty, but nothing really
     * better we can do
     */
    return sshproxy_write(s, data, len);
}

static void sshproxy_write_eof(Socket *s)
{
    SshProxy *sp = container_of(s, SshProxy, sock);
    if (!sp->backend)
        return;
    backend_special(sp->backend, SS_EOF, 0);
}

static void try_send_ssh_to_socket(void *ctx);

static void try_send_ssh_to_socket_cb(void *ctx)
{
    SshProxy *sp = (SshProxy *)ctx;
    try_send_ssh_to_socket(sp);
    if (sp->backend)
        backend_unthrottle(sp->backend, bufchain_size(&sp->ssh_to_socket));
}

static void sshproxy_set_frozen(Socket *s, bool is_frozen)
{
    SshProxy *sp = container_of(s, SshProxy, sock);
    sp->frozen = is_frozen;
    if (!sp->frozen)
        queue_toplevel_callback(try_send_ssh_to_socket_cb, sp);
}

static const char *sshproxy_socket_error(Socket *s)
{
    SshProxy *sp = container_of(s, SshProxy, sock);
    return sp->errmsg;
}

static SocketPeerInfo *sshproxy_peer_info(Socket *s)
{
    return NULL;
}

static const SocketVtable SshProxy_sock_vt = {
    .plug = sshproxy_plug,
    .close = sshproxy_close,
    .write = sshproxy_write,
    .write_oob = sshproxy_write_oob,
    .write_eof = sshproxy_write_eof,
    .set_frozen = sshproxy_set_frozen,
    .socket_error = sshproxy_socket_error,
    .peer_info = sshproxy_peer_info,
};

static void sshproxy_eventlog(LogPolicy *lp, const char *event)
{
    SshProxy *sp = container_of(lp, SshProxy, logpolicy);
    log_proxy_stderr(sp->plug, &sp->psb, event, strlen(event));
    log_proxy_stderr(sp->plug, &sp->psb, "\n", 1);
}

static int sshproxy_askappend(LogPolicy *lp, Filename *filename,
                              void (*callback)(void *ctx, int result),
                              void *ctx)
{
    SshProxy *sp = container_of(lp, SshProxy, logpolicy);

    /*
     * If we have access to the outer LogPolicy, pass on this request
     * to the end user.
     */
    if (sp->clientlp)
        return lp_askappend(sp->clientlp, filename, callback, ctx);

    /*
     * Otherwise, fall back to the safe noninteractive assumption.
     */
    char *msg = dupprintf("Log file \"%s\" already exists; logging cancelled",
                          filename_to_str(filename));
    sshproxy_eventlog(lp, msg);
    sfree(msg);
    return 0;
}

static void sshproxy_logging_error(LogPolicy *lp, const char *event)
{
    SshProxy *sp = container_of(lp, SshProxy, logpolicy);

    /*
     * If we have access to the outer LogPolicy, pass on this request
     * to it.
     */
    if (sp->clientlp) {
        lp_logging_error(sp->clientlp, event);
        return;
    }

    /*
     * Otherwise, the best we can do is to put it in the outer SSH
     * connection's Event Log.
     */
    char *msg = dupprintf("Logging error: %s", event);
    sshproxy_eventlog(lp, msg);
    sfree(msg);
}

static const LogPolicyVtable SshProxy_logpolicy_vt = {
    .eventlog = sshproxy_eventlog,
    .askappend = sshproxy_askappend,
    .logging_error = sshproxy_logging_error,
    .verbose = null_lp_verbose_no,
};

/*
 * Function called when we encounter an error during connection setup that's
 * likely to be the cause of terminating the proxy SSH connection. Putting it
 * in the Event Log is useful on general principles; also putting it in
 * sp->errmsg meaks that it will be passed back through plug_closing when the
 * proxy SSH connection actually terminates, so that the end user will see
 * what went wrong in the proxy connection.
 */
static void sshproxy_error(SshProxy *sp, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    char *msg = dupvprintf(fmt, ap);
    va_end(ap);

    if (!sp->errmsg)
        sp->errmsg = dupstr(msg);

    sshproxy_eventlog(&sp->logpolicy, msg);
    sfree(msg);
}

static void try_send_ssh_to_socket(void *ctx)
{
    SshProxy *sp = (SshProxy *)ctx;

    if (sp->frozen)
        return;

    while (bufchain_size(&sp->ssh_to_socket)) {
        ptrlen pl = bufchain_prefix(&sp->ssh_to_socket);
        plug_receive(sp->plug, 0, pl.ptr, pl.len);
        bufchain_consume(&sp->ssh_to_socket, pl.len);
    }

    if (sp->rcvd_eof_ssh_to_socket &&
        !sp->sent_eof_ssh_to_socket) {
        sp->sent_eof_ssh_to_socket = true;
        plug_closing_normal(sp->plug);
    }
}

static void sshproxy_notify_session_started(Seat *seat)
{
    SshProxy *sp = container_of(seat, SshProxy, seat);

    if (sp->clientseat)
        interactor_return_seat(sp->clientitr);
    sp->conn_established = true;

    plug_log(sp->plug, PLUGLOG_CONNECT_SUCCESS, sp->addr, sp->port, NULL, 0);
}

static size_t sshproxy_output(Seat *seat, SeatOutputType type,
                              const void *data, size_t len)
{
    SshProxy *sp = container_of(seat, SshProxy, seat);
    bufchain_add(&sp->ssh_to_socket, data, len);
    try_send_ssh_to_socket(sp);
    return bufchain_size(&sp->ssh_to_socket);
}

static inline InteractionReadySeat wrap(Seat *seat)
{
    /*
     * When we receive interaction requests from the proxy and want to
     * pass them on to our client Seat, we have to present them to the
     * latter in the form of an InteractionReadySeat. This forwarding
     * scenario is the one case where we _mustn't_ get an
     * InteractionReadySeat by calling interactor_announce(), because
     * the point is that we're _not_ the originating Interactor, we're
     * just forwarding the request from the real one, which has
     * already announced itself.
     *
     * So, just here in the code, it really is the right thing to make
     * an InteractionReadySeat out of a plain Seat * without an
     * announcement.
     */
    InteractionReadySeat iseat;
    iseat.seat = seat;
    return iseat;
}

static size_t sshproxy_banner(Seat *seat, const void *data, size_t len)
{
    SshProxy *sp = container_of(seat, SshProxy, seat);
    if (sp->clientseat) {
        /*
         * If we have access to the outer Seat, pass the SSH login
         * banner on to it.
         */
        return seat_banner(wrap(sp->clientseat), data, len);
    } else {
        return 0;
    }
}

static bool sshproxy_eof(Seat *seat)
{
    SshProxy *sp = container_of(seat, SshProxy, seat);
    sp->rcvd_eof_ssh_to_socket = true;
    try_send_ssh_to_socket(sp);
    return false;
}

static void sshproxy_sent(Seat *seat, size_t new_bufsize)
{
    SshProxy *sp = container_of(seat, SshProxy, seat);
    plug_sent(sp->plug, new_bufsize);
}

static void sshproxy_send_close(SshProxy *sp)
{
    if (sp->clientseat)
        interactor_return_seat(sp->clientitr);

    if (!sp->conn_established)
        plug_log(sp->plug, PLUGLOG_CONNECT_FAILED, sp->addr, sp->port,
                 sp->errmsg, 0);

    if (sp->errmsg)
        plug_closing_error(sp->plug, sp->errmsg);
    else if (!sp->conn_established && backend_exitcode(sp->backend) == 0)
        plug_closing_user_abort(sp->plug);
    else
        plug_closing_normal(sp->plug);
}

static void sshproxy_notify_remote_disconnect_callback(void *vctx)
{
    SshProxy *sp = (SshProxy *)vctx;

    /* notify_remote_disconnect can be called redundantly, so first
     * check if the backend really has become disconnected */
    if (backend_connected(sp->backend))
        return;

    sshproxy_send_close(sp);
}

static void sshproxy_notify_remote_disconnect(Seat *seat)
{
    SshProxy *sp = container_of(seat, SshProxy, seat);
    queue_toplevel_callback(sshproxy_notify_remote_disconnect_callback, sp);
}

static SeatPromptResult sshproxy_get_userpass_input(Seat *seat, prompts_t *p)
{
    SshProxy *sp = container_of(seat, SshProxy, seat);

    /*
     * If we have a stored proxy_password, use that, via logic similar
     * to cmdline_get_passwd_input: we only try it if we're given a
     * prompts_t containing exactly one prompt, and that prompt is set
     * to non-echoing.
     */
    if (sp->got_proxy_password && !sp->tried_proxy_password &&
        p->n_prompts == 1 && !p->prompts[0]->echo) {
        prompt_set_result(p->prompts[0], sp->proxy_password);
        burnstr(sp->proxy_password);
        sp->proxy_password = NULL;
        sp->tried_proxy_password = true;
        return SPR_OK;
    }

    if (sp->clientseat) {
        /*
         * If we have access to the outer Seat, pass this prompt
         * request on to it.
         */
        return seat_get_userpass_input(wrap(sp->clientseat), p);
    }

    /*
     * Otherwise, behave as if noninteractive (like plink -batch):
     * reject all attempts to present a prompt to the user, and log in
     * the Event Log to say why not.
     */
    sshproxy_error(sp, "Unable to provide interactive authentication "
                   "requested by proxy SSH connection");
    return SPR_SW_ABORT("Noninteractive SSH proxy cannot perform "
                        "interactive authentication");
}

static void sshproxy_connection_fatal_callback(void *vctx)
{
    SshProxy *sp = (SshProxy *)vctx;
    sshproxy_send_close(sp);
}

static void sshproxy_connection_fatal(Seat *seat, const char *message)
{
    SshProxy *sp = container_of(seat, SshProxy, seat);
    if (!sp->errmsg) {
        sp->errmsg = dupprintf(
            "fatal error in proxy SSH connection: %s", message);
        queue_toplevel_callback(sshproxy_connection_fatal_callback, sp);
    }
}

static SeatPromptResult sshproxy_confirm_ssh_host_key(
    Seat *seat, const char *host, int port, const char *keytype,
    char *keystr, const char *keydisp, char **key_fingerprints, bool mismatch,
    void (*callback)(void *ctx, SeatPromptResult result), void *ctx)
{
    SshProxy *sp = container_of(seat, SshProxy, seat);

    if (sp->clientseat) {
        /*
         * If we have access to the outer Seat, pass this prompt
         * request on to it.
         */
        return seat_confirm_ssh_host_key(
            wrap(sp->clientseat), host, port, keytype, keystr, keydisp,
            key_fingerprints, mismatch, callback, ctx);
    }

    /*
     * Otherwise, behave as if we're in batch mode, i.e. take the safe
     * option in the absence of interactive confirmation, i.e. abort
     * the connection.
     */
    return SPR_SW_ABORT("Noninteractive SSH proxy cannot confirm host key");
}

static SeatPromptResult sshproxy_confirm_weak_crypto_primitive(
        Seat *seat, const char *algtype, const char *algname,
        void (*callback)(void *ctx, SeatPromptResult result), void *ctx)
{
    SshProxy *sp = container_of(seat, SshProxy, seat);

    if (sp->clientseat) {
        /*
         * If we have access to the outer Seat, pass this prompt
         * request on to it.
         */
        return seat_confirm_weak_crypto_primitive(
            wrap(sp->clientseat), algtype, algname, callback, ctx);
    }

    /*
     * Otherwise, behave as if we're in batch mode: take the safest
     * option.
     */
    sshproxy_error(sp, "First %s supported by server is %s, below warning "
                   "threshold. Abandoning proxy SSH connection.",
                   algtype, algname);
    return SPR_SW_ABORT("Noninteractive SSH proxy cannot confirm "
                        "weak crypto primitive");
}

static SeatPromptResult sshproxy_confirm_weak_cached_hostkey(
        Seat *seat, const char *algname, const char *betteralgs,
        void (*callback)(void *ctx, SeatPromptResult result), void *ctx)
{
    SshProxy *sp = container_of(seat, SshProxy, seat);

    if (sp->clientseat) {
        /*
         * If we have access to the outer Seat, pass this prompt
         * request on to it.
         */
        return seat_confirm_weak_cached_hostkey(
            wrap(sp->clientseat), algname, betteralgs, callback, ctx);
    }

    /*
     * Otherwise, behave as if we're in batch mode: take the safest
     * option.
     */
    sshproxy_error(sp, "First host key type stored for server is %s, below "
                   "warning threshold. Abandoning proxy SSH connection.",
                   algname);
    return SPR_SW_ABORT("Noninteractive SSH proxy cannot confirm "
                        "weak cached host key");
}

static StripCtrlChars *sshproxy_stripctrl_new(
    Seat *seat, BinarySink *bs_out, SeatInteractionContext sic)
{
    SshProxy *sp = container_of(seat, SshProxy, seat);
    if (sp->clientseat)
        return seat_stripctrl_new(sp->clientseat, bs_out, sic);
    else
        return NULL;
}

static void sshproxy_set_trust_status(Seat *seat, bool trusted)
{
    SshProxy *sp = container_of(seat, SshProxy, seat);
    if (sp->clientseat)
        seat_set_trust_status(sp->clientseat, trusted);
}

static bool sshproxy_can_set_trust_status(Seat *seat)
{
    SshProxy *sp = container_of(seat, SshProxy, seat);
    return sp->clientseat && seat_can_set_trust_status(sp->clientseat);
}

static bool sshproxy_verbose(Seat *seat)
{
    SshProxy *sp = container_of(seat, SshProxy, seat);
    return sp->clientseat && seat_verbose(sp->clientseat);
}

static bool sshproxy_interactive(Seat *seat)
{
    SshProxy *sp = container_of(seat, SshProxy, seat);
    return sp->clientseat && seat_interactive(sp->clientseat);
}

static const SeatVtable SshProxy_seat_vt = {
    .output = sshproxy_output,
    .eof = sshproxy_eof,
    .sent = sshproxy_sent,
    .banner = sshproxy_banner,
    .get_userpass_input = sshproxy_get_userpass_input,
    .notify_session_started = sshproxy_notify_session_started,
    .notify_remote_exit = nullseat_notify_remote_exit,
    .notify_remote_disconnect = sshproxy_notify_remote_disconnect,
    .connection_fatal = sshproxy_connection_fatal,
    .update_specials_menu = nullseat_update_specials_menu,
    .get_ttymode = nullseat_get_ttymode,
    .set_busy_status = nullseat_set_busy_status,
    .confirm_ssh_host_key = sshproxy_confirm_ssh_host_key,
    .confirm_weak_crypto_primitive = sshproxy_confirm_weak_crypto_primitive,
    .confirm_weak_cached_hostkey = sshproxy_confirm_weak_cached_hostkey,
    .is_utf8 = nullseat_is_never_utf8,
    .echoedit_update = nullseat_echoedit_update,
    .get_x_display = nullseat_get_x_display,
    .get_windowid = nullseat_get_windowid,
    .get_window_pixel_size = nullseat_get_window_pixel_size,
    .stripctrl_new = sshproxy_stripctrl_new,
    .set_trust_status = sshproxy_set_trust_status,
    .can_set_trust_status = sshproxy_can_set_trust_status,
    .has_mixed_input_stream = nullseat_has_mixed_input_stream_no,
    .verbose = sshproxy_verbose,
    .interactive = sshproxy_interactive,
    .get_cursor_position = nullseat_get_cursor_position,
};

Socket *sshproxy_new_connection(SockAddr *addr, const char *hostname,
                                int port, bool privport,
                                bool oobinline, bool nodelay, bool keepalive,
                                Plug *plug, Conf *clientconf,
                                Interactor *clientitr)
{
    SshProxy *sp = snew(SshProxy);
    memset(sp, 0, sizeof(*sp));

    sp->sock.vt = &SshProxy_sock_vt;
    sp->logpolicy.vt = &SshProxy_logpolicy_vt;
    sp->seat.vt = &SshProxy_seat_vt;
    sp->plug = plug;
    psb_init(&sp->psb);
    bufchain_init(&sp->ssh_to_socket);

    sp->addr = addr;
    sp->port = port;

    sp->conf = conf_new();
    /* Try to treat proxy_hostname as the title of a saved session. If
     * that fails, set up a default Conf of our own treating it as a
     * hostname. */
    const char *proxy_hostname = conf_get_str(clientconf, CONF_proxy_host);
    if (do_defaults(proxy_hostname, sp->conf)) {
        if (!conf_launchable(sp->conf)) {
            sp->errmsg = dupprintf("saved session '%s' is not launchable",
                                   proxy_hostname);
            return &sp->sock;
        }
    } else {
        do_defaults(NULL, sp->conf);
        /* In hostname mode, we default to PROT_SSH. This is more useful than
         * the obvious approach of defaulting to the protocol defined in
         * Default Settings, because only SSH (ok, and bare ssh-connection)
         * can be used for this kind of proxy. */
        conf_set_int(sp->conf, CONF_protocol, PROT_SSH);
        conf_set_str(sp->conf, CONF_host, proxy_hostname);
        conf_set_int(sp->conf, CONF_port,
                     conf_get_int(clientconf, CONF_proxy_port));
    }
    const char *proxy_username = conf_get_str(clientconf, CONF_proxy_username);
    if (*proxy_username)
        conf_set_str(sp->conf, CONF_username, proxy_username);

    const char *proxy_password = conf_get_str(clientconf, CONF_proxy_password);
    if (*proxy_password) {
        sp->proxy_password = dupstr(proxy_password);
        sp->got_proxy_password = true;
    }

    const struct BackendVtable *backvt = backend_vt_from_proto(
        conf_get_int(sp->conf, CONF_protocol));

    /*
     * We don't actually need an _SSH_ session specifically: it's also
     * OK to use PROT_SSHCONN, because really, the criterion is
     * whether setting CONF_ssh_nc_host will do anything useful. So
     * our check is for whether the backend sets the flag promising
     * that it does.
     */
    if (!backvt || !(backvt->flags & BACKEND_SUPPORTS_NC_HOST)) {
        sp->errmsg = dupprintf("saved session '%s' is not an SSH session",
                               proxy_hostname);
        return &sp->sock;
    }

    /*
     * We also expect that the backend will announce a willingness to
     * notify us that the session has started. Any backend providing
     * NC_HOST should also provide this.
     */
    assert(backvt->flags & BACKEND_NOTIFIES_SESSION_START &&
           "Backend provides NC_HOST without SESSION_START!");

    /*
     * Turn off SSH features we definitely don't want. It would be
     * awkward and counterintuitive to have the proxy SSH connection
     * become a connection-sharing upstream (but it's fine to have it
     * be a downstream, if that's configured). And we don't want to
     * open X forwardings, agent forwardings or (other) port
     * forwardings as a side effect of this one operation.
     */
    conf_set_bool(sp->conf, CONF_ssh_connection_sharing_upstream, false);
    conf_set_bool(sp->conf, CONF_x11_forward, false);
    conf_set_bool(sp->conf, CONF_agentfwd, false);
    for (const char *subkey;
         (subkey = conf_get_str_nthstrkey(sp->conf, CONF_portfwd, 0)) != NULL;)
        conf_del_str_str(sp->conf, CONF_portfwd, subkey);

    /*
     * We'll only be running one channel through this connection
     * (since we've just turned off all the other things we might have
     * done with it), so we can configure it as simple.
     */
    conf_set_bool(sp->conf, CONF_ssh_simple, true);

    /*
     * Configure the main channel of this SSH session to be a
     * direct-tcpip connection to the destination host/port.
     */
    conf_set_str(sp->conf, CONF_ssh_nc_host, hostname);
    conf_set_int(sp->conf, CONF_ssh_nc_port, port);

    /*
     * Do the usual normalisation of things in the Conf like a "user@"
     * prefix on the hostname field.
     */
    prepare_session(sp->conf);

    sp->logctx = log_init(&sp->logpolicy, sp->conf);

    char *error, *realhost;
    error = backend_init(backvt, &sp->seat, &sp->backend, sp->logctx, sp->conf,
                         conf_get_str(sp->conf, CONF_host),
                         conf_get_int(sp->conf, CONF_port),
                         &realhost, nodelay,
                         conf_get_bool(sp->conf, CONF_tcp_keepalives));
    if (error) {
        sp->errmsg = dupprintf("unable to open SSH proxy connection: %s",
                               error);
        return &sp->sock;
    }

    sfree(realhost);

    /*
     * If we've been given an Interactor by the caller, set ourselves
     * up to work with it.
     */
    if (clientitr) {
        sp->clientitr = clientitr;
        interactor_set_child(sp->clientitr, sp->backend->interactor);

        sp->clientlp = interactor_logpolicy(clientitr);

        /*
         * We can only borrow the client's Seat if our own backend
         * will tell us when to give it back. (SSH-based backends
         * _should_ do that, but we check the flag here anyway.)
         */
        if (backvt->flags & BACKEND_NOTIFIES_SESSION_START)
            sp->clientseat = interactor_borrow_seat(clientitr);
    }

    return &sp->sock;
}
