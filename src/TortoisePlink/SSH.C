/*
 * SSH backend.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <limits.h>
#include <signal.h>

#include "putty.h"
#include "pageant.h" /* for AGENT_MAX_MSGLEN */
#include "tree234.h"
#include "storage.h"
#include "marshal.h"
#include "ssh.h"
#include "sshcr.h"
#include "sshbpp.h"
#include "sshppl.h"
#include "sshchan.h"
#ifndef NO_GSSAPI
#include "sshgssc.h"
#include "sshgss.h"
#define MIN_CTXT_LIFETIME 5	/* Avoid rekey with short lifetime (seconds) */
#define GSS_KEX_CAPABLE	(1<<0)	/* Can do GSS KEX */
#define GSS_CRED_UPDATED (1<<1) /* Cred updated since previous delegation */
#define GSS_CTXT_EXPIRES (1<<2)	/* Context expires before next timer */
#define GSS_CTXT_MAYFAIL (1<<3)	/* Context may expire during handshake */
#endif

struct Ssh {
    Socket *s;
    Seat *seat;
    Conf *conf;

    struct ssh_version_receiver version_receiver;
    int remote_bugs;

    Plug plug;
    Backend backend;

    Ldisc *ldisc;
    LogContext *logctx;

    /* The last list returned from get_specials. */
    SessionSpecial *specials;

    bool bare_connection;
    ssh_sharing_state *connshare;
    bool attempting_connshare;

    struct ssh_connection_shared_gss_state gss_state;

    char *savedhost;
    int savedport;
    char *fullhostname;

    bool fallback_cmd;
    int exitcode;

    int version;
    int conn_throttle_count;
    size_t overall_bufsize;
    bool throttled_all;

    /*
     * logically_frozen is true if we're not currently _processing_
     * data from the SSH socket (e.g. because a higher layer has asked
     * us not to due to ssh_throttle_conn). socket_frozen is true if
     * we're not even _reading_ data from the socket (i.e. it should
     * always match the value we last passed to sk_set_frozen).
     *
     * The two differ in that socket_frozen can also become
     * temporarily true because of a large backlog in the in_raw
     * bufchain, to force no further plug_receive events until the BPP
     * input function has had a chance to run. (Some front ends, like
     * GTK, can persistently call the network and never get round to
     * the toplevel callbacks.) If we've stopped reading from the
     * socket for that reason, we absolutely _do_ want to carry on
     * processing our input bufchain, because that's the only way
     * it'll ever get cleared!
     *
     * ssh_check_frozen() resets socket_frozen, and should be called
     * whenever either of logically_frozen and the bufchain size
     * changes.
     */
    bool logically_frozen, socket_frozen;

    /* in case we find these out before we have a ConnectionLayer to tell */
    int term_width, term_height;

    bufchain in_raw, out_raw, user_input;
    bool pending_close;
    IdempotentCallback ic_out_raw;

    PacketLogSettings pls;
    struct DataTransferStats stats;

    BinaryPacketProtocol *bpp;

    /*
     * base_layer identifies the bottommost packet protocol layer, the
     * one connected directly to the BPP's packet queues. Any
     * operation that needs to talk to all layers (e.g. free, or
     * get_specials) will do it by talking to this, which will
     * recursively propagate it if necessary.
     */
    PacketProtocolLayer *base_layer;

    /*
     * The ConnectionLayer vtable from our connection layer.
     */
    ConnectionLayer *cl;

    /*
     * A dummy ConnectionLayer that can be used for logging sharing
     * downstreams that connect before the real one is ready.
     */
    ConnectionLayer cl_dummy;

    /*
     * session_started is false until we initialise the main protocol
     * layers. So it distinguishes between base_layer==NULL meaning
     * that the SSH protocol hasn't been set up _yet_, and
     * base_layer==NULL meaning the SSH protocol has run and finished.
     * It's also used to mark the point where we stop counting proxy
     * command diagnostics as pre-session-startup.
     */
    bool session_started;

    Pinger *pinger;

    bool need_random_unref;
};


#define ssh_logevent(params) ( \
        logevent_and_free((ssh)->logctx, dupprintf params))

static void ssh_shutdown(Ssh *ssh);
static void ssh_throttle_all(Ssh *ssh, bool enable, size_t bufsize);
static void ssh_bpp_output_raw_data_callback(void *vctx);

LogContext *ssh_get_logctx(Ssh *ssh)
{
    return ssh->logctx;
}

static void ssh_connect_bpp(Ssh *ssh)
{
    ssh->bpp->ssh = ssh;
    ssh->bpp->in_raw = &ssh->in_raw;
    ssh->bpp->out_raw = &ssh->out_raw;
    bufchain_set_callback(ssh->bpp->out_raw, &ssh->ic_out_raw);
    ssh->bpp->pls = &ssh->pls;
    ssh->bpp->logctx = ssh->logctx;
    ssh->bpp->remote_bugs = ssh->remote_bugs;
}

static void ssh_connect_ppl(Ssh *ssh, PacketProtocolLayer *ppl)
{
    ppl->bpp = ssh->bpp;
    ppl->user_input = &ssh->user_input;
    ppl->seat = ssh->seat;
    ppl->ssh = ssh;
    ppl->logctx = ssh->logctx;
    ppl->remote_bugs = ssh->remote_bugs;
}

static void ssh_got_ssh_version(struct ssh_version_receiver *rcv,
                                int major_version)
{
    Ssh *ssh = container_of(rcv, Ssh, version_receiver);
    BinaryPacketProtocol *old_bpp;
    PacketProtocolLayer *connection_layer;

    ssh->session_started = true;

    /*
     * We don't support choosing a major protocol version dynamically,
     * so this should always be the same value we set up in
     * connect_to_host().
     */
    assert(ssh->version == major_version);

    old_bpp = ssh->bpp;
    ssh->remote_bugs = ssh_verstring_get_bugs(old_bpp);

    if (!ssh->bare_connection) {
        if (ssh->version == 2) {
            PacketProtocolLayer *userauth_layer, *transport_child_layer;

            /*
             * We use the 'simple' variant of the SSH protocol if
             * we're asked to, except not if we're also doing
             * connection-sharing (either tunnelling our packets over
             * an upstream or expecting to be tunnelled over
             * ourselves), since then the assumption that we have only
             * one channel to worry about is not true after all.
             */
            bool is_simple =
                (conf_get_bool(ssh->conf, CONF_ssh_simple) && !ssh->connshare);

            ssh->bpp = ssh2_bpp_new(ssh->logctx, &ssh->stats, false);
            ssh_connect_bpp(ssh);

#ifndef NO_GSSAPI
            /* Load and pick the highest GSS library on the preference
             * list. */
            if (!ssh->gss_state.libs)
                ssh->gss_state.libs = ssh_gss_setup(ssh->conf);
            ssh->gss_state.lib = NULL;
            if (ssh->gss_state.libs->nlibraries > 0) {
                int i, j;
                for (i = 0; i < ngsslibs; i++) {
                    int want_id = conf_get_int_int(ssh->conf,
                                                   CONF_ssh_gsslist, i);
                    for (j = 0; j < ssh->gss_state.libs->nlibraries; j++)
                        if (ssh->gss_state.libs->libraries[j].id == want_id) {
                            ssh->gss_state.lib =
                                &ssh->gss_state.libs->libraries[j];
                            goto got_gsslib;   /* double break */
                        }
                }
              got_gsslib:
                /*
                 * We always expect to have found something in
                 * the above loop: we only came here if there
                 * was at least one viable GSS library, and the
                 * preference list should always mention
                 * everything and only change the order.
                 */
                assert(ssh->gss_state.lib);
            }
#endif

            connection_layer = ssh2_connection_new(
                ssh, ssh->connshare, is_simple, ssh->conf, 
                ssh_verstring_get_remote(old_bpp), &ssh->cl);
            ssh_connect_ppl(ssh, connection_layer);

            if (conf_get_bool(ssh->conf, CONF_ssh_no_userauth)) {
                userauth_layer = NULL;
                transport_child_layer = connection_layer;
            } else {
                char *username = get_remote_username(ssh->conf);

                userauth_layer = ssh2_userauth_new(
                    connection_layer, ssh->savedhost, ssh->fullhostname,
                    conf_get_filename(ssh->conf, CONF_keyfile),
                    conf_get_bool(ssh->conf, CONF_tryagent), username,
                    conf_get_bool(ssh->conf, CONF_change_username),
                    conf_get_bool(ssh->conf, CONF_try_ki_auth),
                    conf_get_bool(ssh->conf, CONF_try_gssapi_auth),
                    conf_get_bool(ssh->conf, CONF_try_gssapi_kex),
                    conf_get_bool(ssh->conf, CONF_gssapifwd),
                    &ssh->gss_state);
                ssh_connect_ppl(ssh, userauth_layer);
                transport_child_layer = userauth_layer;

                sfree(username);
            }

            ssh->base_layer = ssh2_transport_new(
                ssh->conf, ssh->savedhost, ssh->savedport,
                ssh->fullhostname,
                ssh_verstring_get_local(old_bpp),
                ssh_verstring_get_remote(old_bpp),
                &ssh->gss_state,
                &ssh->stats, transport_child_layer, false);
            ssh_connect_ppl(ssh, ssh->base_layer);

            if (userauth_layer)
                ssh2_userauth_set_transport_layer(userauth_layer,
                                                  ssh->base_layer);

        } else {

            ssh->bpp = ssh1_bpp_new(ssh->logctx);
            ssh_connect_bpp(ssh);

            connection_layer = ssh1_connection_new(ssh, ssh->conf, &ssh->cl);
            ssh_connect_ppl(ssh, connection_layer);

            ssh->base_layer = ssh1_login_new(
                ssh->conf, ssh->savedhost, ssh->savedport, connection_layer);
            ssh_connect_ppl(ssh, ssh->base_layer);

        }

    } else {
        ssh->bpp = ssh2_bare_bpp_new(ssh->logctx);
        ssh_connect_bpp(ssh);

        connection_layer = ssh2_connection_new(
            ssh, NULL, false, ssh->conf, ssh_verstring_get_remote(old_bpp),
            &ssh->cl);
        ssh_connect_ppl(ssh, connection_layer);
        ssh->base_layer = connection_layer;
    }

    /* Connect the base layer - whichever it is - to the BPP, and set
     * up its selfptr. */
    ssh->base_layer->selfptr = &ssh->base_layer;
    ssh_ppl_setup_queues(ssh->base_layer, &ssh->bpp->in_pq, &ssh->bpp->out_pq);

    seat_update_specials_menu(ssh->seat);
    ssh->pinger = pinger_new(ssh->conf, &ssh->backend);

    queue_idempotent_callback(&ssh->bpp->ic_in_raw);
    ssh_ppl_process_queue(ssh->base_layer);

    /* Pass in the initial terminal size, if we knew it already. */
    ssh_terminal_size(ssh->cl, ssh->term_width, ssh->term_height);

    ssh_bpp_free(old_bpp);
}

void ssh_check_frozen(Ssh *ssh)
{
    if (!ssh->s)
        return;

    bool prev_frozen = ssh->socket_frozen;
    ssh->socket_frozen = (ssh->logically_frozen ||
                          bufchain_size(&ssh->in_raw) > SSH_MAX_BACKLOG);
    sk_set_frozen(ssh->s, ssh->socket_frozen);
    if (prev_frozen && !ssh->socket_frozen && ssh->bpp) {
        /*
         * If we've just unfrozen, process any SSH connection data
         * that was stashed in our queue while we were frozen.
         */
        queue_idempotent_callback(&ssh->bpp->ic_in_raw);
    }
}

void ssh_conn_processed_data(Ssh *ssh)
{
    ssh_check_frozen(ssh);
}

static void ssh_bpp_output_raw_data_callback(void *vctx)
{
    Ssh *ssh = (Ssh *)vctx;

    if (!ssh->s)
        return;

    while (bufchain_size(&ssh->out_raw) > 0) {
        size_t backlog;

        ptrlen data = bufchain_prefix(&ssh->out_raw);

        if (ssh->logctx)
            log_packet(ssh->logctx, PKT_OUTGOING, -1, NULL, data.ptr, data.len,
                       0, NULL, NULL, 0, NULL);
        backlog = sk_write(ssh->s, data.ptr, data.len);

        bufchain_consume(&ssh->out_raw, data.len);

        if (backlog > SSH_MAX_BACKLOG) {
            ssh_throttle_all(ssh, true, backlog);
            return;
        }
    }

    ssh_check_frozen(ssh);

    if (ssh->pending_close) {
        sk_close(ssh->s);
        ssh->s = NULL;
    }
}

static void ssh_shutdown_internal(Ssh *ssh)
{
    expire_timer_context(ssh);

    if (ssh->connshare) {
        sharestate_free(ssh->connshare);
        ssh->connshare = NULL;
    }

    if (ssh->pinger) {
        pinger_free(ssh->pinger);
        ssh->pinger = NULL;
    }

    /*
     * We only need to free the base PPL, which will free the others
     * (if any) transitively.
     */
    if (ssh->base_layer) {
        ssh_ppl_free(ssh->base_layer);
        ssh->base_layer = NULL;
    }

    ssh->cl = NULL;
}

static void ssh_shutdown(Ssh *ssh)
{
    ssh_shutdown_internal(ssh);

    if (ssh->bpp) {
        ssh_bpp_free(ssh->bpp);
        ssh->bpp = NULL;
    }

    if (ssh->s) {
        sk_close(ssh->s);
        ssh->s = NULL;
    }

    bufchain_clear(&ssh->in_raw);
    bufchain_clear(&ssh->out_raw);
    bufchain_clear(&ssh->user_input);
}

static void ssh_initiate_connection_close(Ssh *ssh)
{
    /* Wind up everything above the BPP. */
    ssh_shutdown_internal(ssh);

    /* Force any remaining queued SSH packets through the BPP, and
     * schedule closing the network socket after they go out. */
    ssh_bpp_handle_output(ssh->bpp);
    ssh->pending_close = true;
    queue_idempotent_callback(&ssh->ic_out_raw);

    /* Now we expect the other end to close the connection too in
     * response, so arrange that we'll receive notification of that
     * via ssh_remote_eof. */
    ssh->bpp->expect_close = true;
}

#define GET_FORMATTED_MSG                       \
    char *msg;                                  \
    va_list ap;                                 \
    va_start(ap, fmt);                          \
    msg = dupvprintf(fmt, ap);                  \
    va_end(ap);

void ssh_remote_error(Ssh *ssh, const char *fmt, ...)
{
    if (ssh->base_layer || !ssh->session_started) {
        GET_FORMATTED_MSG;

        /* Error messages sent by the remote don't count as clean exits */
        ssh->exitcode = 128;

        /* Close the socket immediately, since the server has already
         * closed its end (or is about to). */
        ssh_shutdown(ssh);

        logevent(ssh->logctx, msg);
        seat_connection_fatal(ssh->seat, "%s", msg);
        sfree(msg);
    }
}

void ssh_remote_eof(Ssh *ssh, const char *fmt, ...)
{
    if (ssh->base_layer || !ssh->session_started) {
        GET_FORMATTED_MSG;

        /* EOF from the remote, if we were expecting it, does count as
         * a clean exit */
        ssh->exitcode = 0;

        /* Close the socket immediately, since the server has already
         * closed its end. */
        ssh_shutdown(ssh);

        logevent(ssh->logctx, msg);
        sfree(msg);
        seat_notify_remote_exit(ssh->seat);
    } else {
        /* This is responding to EOF after we've already seen some
         * other reason for terminating the session. */
        ssh_shutdown(ssh);
    }
}

void ssh_proto_error(Ssh *ssh, const char *fmt, ...)
{
    if (ssh->base_layer || !ssh->session_started) {
        GET_FORMATTED_MSG;

        ssh->exitcode = 128;

        ssh_bpp_queue_disconnect(ssh->bpp, msg,
                                 SSH2_DISCONNECT_PROTOCOL_ERROR);
        ssh_initiate_connection_close(ssh);

        logevent(ssh->logctx, msg);
        seat_connection_fatal(ssh->seat, "%s", msg);
        sfree(msg);
    }
}

void ssh_sw_abort(Ssh *ssh, const char *fmt, ...)
{
    if (ssh->base_layer || !ssh->session_started) {
        GET_FORMATTED_MSG;

        ssh->exitcode = 128;

        ssh_initiate_connection_close(ssh);

        logevent(ssh->logctx, msg);
        seat_connection_fatal(ssh->seat, "%s", msg);
        sfree(msg);

        seat_notify_remote_exit(ssh->seat);
    }
}

void ssh_user_close(Ssh *ssh, const char *fmt, ...)
{
    if (ssh->base_layer || !ssh->session_started) {
        GET_FORMATTED_MSG;

        /* Closing the connection due to user action, even if the
         * action is the user aborting during authentication prompts,
         * does count as a clean exit - except that this is also how
         * we signal ordinary session termination, in which case we
         * should use the exit status already sent from the main
         * session (if any). */
        if (ssh->exitcode < 0)
            ssh->exitcode = 0;

        ssh_initiate_connection_close(ssh);

        logevent(ssh->logctx, msg);
        sfree(msg);

        seat_notify_remote_exit(ssh->seat);
    }
}

static void ssh_socket_log(Plug *plug, int type, SockAddr *addr, int port,
                           const char *error_msg, int error_code)
{
    Ssh *ssh = container_of(plug, Ssh, plug);

    /*
     * While we're attempting connection sharing, don't loudly log
     * everything that happens. Real TCP connections need to be logged
     * when we _start_ trying to connect, because it might be ages
     * before they respond if something goes wrong; but connection
     * sharing is local and quick to respond, and it's sufficient to
     * simply wait and see whether it worked afterwards.
     */

    if (!ssh->attempting_connshare)
        backend_socket_log(ssh->seat, ssh->logctx, type, addr, port,
                           error_msg, error_code, ssh->conf,
                           ssh->session_started);
}

static void ssh_closing(Plug *plug, const char *error_msg, int error_code,
			bool calling_back)
{
    Ssh *ssh = container_of(plug, Ssh, plug);
    if (error_msg) {
        ssh_remote_error(ssh, "Network error: %s", error_msg);
    } else if (ssh->bpp) {
        ssh->bpp->input_eof = true;
        queue_idempotent_callback(&ssh->bpp->ic_in_raw);
    }
}

static void ssh_receive(Plug *plug, int urgent, const char *data, size_t len)
{
    Ssh *ssh = container_of(plug, Ssh, plug);

    /* Log raw data, if we're in that mode. */
    if (ssh->logctx)
	log_packet(ssh->logctx, PKT_INCOMING, -1, NULL, data, len,
		   0, NULL, NULL, 0, NULL);

    bufchain_add(&ssh->in_raw, data, len);
    if (!ssh->logically_frozen && ssh->bpp)
        queue_idempotent_callback(&ssh->bpp->ic_in_raw);

    ssh_check_frozen(ssh);
}

static void ssh_sent(Plug *plug, size_t bufsize)
{
    Ssh *ssh = container_of(plug, Ssh, plug);
    /*
     * If the send backlog on the SSH socket itself clears, we should
     * unthrottle the whole world if it was throttled. Also trigger an
     * extra call to the consumer of the BPP's output, to try to send
     * some more data off its bufchain.
     */
    if (bufsize < SSH_MAX_BACKLOG) {
	ssh_throttle_all(ssh, false, bufsize);
        queue_idempotent_callback(&ssh->ic_out_raw);
    }
}

static void ssh_hostport_setup(const char *host, int port, Conf *conf,
                               char **savedhost, int *savedport,
                               char **loghost_ret)
{
    char *loghost = conf_get_str(conf, CONF_loghost);
    if (loghost_ret)
        *loghost_ret = loghost;

    if (*loghost) {
	char *tmphost;
        char *colon;

        tmphost = dupstr(loghost);
	*savedport = 22;	       /* default ssh port */

	/*
	 * A colon suffix on the hostname string also lets us affect
	 * savedport. (Unless there are multiple colons, in which case
	 * we assume this is an unbracketed IPv6 literal.)
	 */
	colon = host_strrchr(tmphost, ':');
	if (colon && colon == host_strchr(tmphost, ':')) {
	    *colon++ = '\0';
	    if (*colon)
		*savedport = atoi(colon);
	}

        *savedhost = host_strduptrim(tmphost);
        sfree(tmphost);
    } else {
	*savedhost = host_strduptrim(host);
	if (port < 0)
	    port = 22;		       /* default ssh port */
	*savedport = port;
    }
}

static bool ssh_test_for_upstream(const char *host, int port, Conf *conf)
{
    char *savedhost;
    int savedport;
    bool ret;

    random_ref(); /* platform may need this to determine share socket name */
    ssh_hostport_setup(host, port, conf, &savedhost, &savedport, NULL);
    ret = ssh_share_test_for_upstream(savedhost, savedport, conf);
    sfree(savedhost);
    random_unref();

    return ret;
}

static const PlugVtable Ssh_plugvt = {
    ssh_socket_log,
    ssh_closing,
    ssh_receive,
    ssh_sent,
    NULL
};

/*
 * Connect to specified host and port.
 * Returns an error message, or NULL on success.
 * Also places the canonical host name into `realhost'. It must be
 * freed by the caller.
 */
static const char *connect_to_host(
    Ssh *ssh, const char *host, int port, char **realhost,
    bool nodelay, bool keepalive)
{
    SockAddr *addr;
    const char *err;
    char *loghost;
    int addressfamily, sshprot;

    ssh_hostport_setup(host, port, ssh->conf,
                       &ssh->savedhost, &ssh->savedport, &loghost);

    ssh->plug.vt = &Ssh_plugvt;

    /*
     * Try connection-sharing, in case that means we don't open a
     * socket after all. ssh_connection_sharing_init will connect to a
     * previously established upstream if it can, and failing that,
     * establish a listening socket for _us_ to be the upstream. In
     * the latter case it will return NULL just as if it had done
     * nothing, because here we only need to care if we're a
     * downstream and need to do our connection setup differently.
     */
    ssh->connshare = NULL;
    ssh->attempting_connshare = true;  /* affects socket logging behaviour */
    ssh->s = ssh_connection_sharing_init(
        ssh->savedhost, ssh->savedport, ssh->conf, ssh->logctx,
        &ssh->plug, &ssh->connshare);
    if (ssh->connshare)
        ssh_connshare_provide_connlayer(ssh->connshare, &ssh->cl_dummy);
    ssh->attempting_connshare = false;
    if (ssh->s != NULL) {
        /*
         * We are a downstream.
         */
        ssh->bare_connection = true;
        ssh->fullhostname = NULL;
        *realhost = dupstr(host);      /* best we can do */

        if ((flags & FLAG_VERBOSE) || (flags & FLAG_INTERACTIVE)) {
            /* In an interactive session, or in verbose mode, announce
             * in the console window that we're a sharing downstream,
             * to avoid confusing users as to why this session doesn't
             * behave in quite the usual way. */
            const char *msg =
                "Reusing a shared connection to this server.\r\n";
            seat_stderr_pl(ssh->seat, ptrlen_from_asciz(msg));
        }
    } else {
        /*
         * We're not a downstream, so open a normal socket.
         */

        /*
         * Try to find host.
         */
        addressfamily = conf_get_int(ssh->conf, CONF_addressfamily);
        addr = name_lookup(host, port, realhost, ssh->conf, addressfamily,
                           ssh->logctx, "SSH connection");
        if ((err = sk_addr_error(addr)) != NULL) {
            sk_addr_free(addr);
            return err;
        }
        ssh->fullhostname = dupstr(*realhost);   /* save in case of GSSAPI */

        ssh->s = new_connection(addr, *realhost, port,
                                false, true, nodelay, keepalive,
                                &ssh->plug, ssh->conf);
        if ((err = sk_socket_error(ssh->s)) != NULL) {
            ssh->s = NULL;
            seat_notify_remote_exit(ssh->seat);
            return err;
        }
    }

    /*
     * The SSH version number is always fixed (since we no longer support
     * fallback between versions), so set it now.
     */
    sshprot = conf_get_int(ssh->conf, CONF_sshprot);
    assert(sshprot == 0 || sshprot == 3);
    if (sshprot == 0)
	/* SSH-1 only */
	ssh->version = 1;
    if (sshprot == 3 || ssh->bare_connection) {
	/* SSH-2 only */
	ssh->version = 2;
    }

    /*
     * Set up the initial BPP that will do the version string
     * exchange, and get it started so that it can send the outgoing
     * version string early if it wants to.
     */
    ssh->version_receiver.got_ssh_version = ssh_got_ssh_version;
    ssh->bpp = ssh_verstring_new(
        ssh->conf, ssh->logctx, ssh->bare_connection,
        ssh->version == 1 ? "1.5" : "2.0", &ssh->version_receiver,
        false, "PuTTY");
    ssh_connect_bpp(ssh);
    queue_idempotent_callback(&ssh->bpp->ic_in_raw);

    /*
     * loghost, if configured, overrides realhost.
     */
    if (*loghost) {
	sfree(*realhost);
	*realhost = dupstr(loghost);
    }

    return NULL;
}

/*
 * Throttle or unthrottle the SSH connection.
 */
void ssh_throttle_conn(Ssh *ssh, int adjust)
{
    int old_count = ssh->conn_throttle_count;
    bool frozen;

    ssh->conn_throttle_count += adjust;
    assert(ssh->conn_throttle_count >= 0);

    if (ssh->conn_throttle_count && !old_count) {
        frozen = true;
    } else if (!ssh->conn_throttle_count && old_count) {
        frozen = false;
    } else {
        return;                /* don't change current frozen state */
    }

    ssh->logically_frozen = frozen;
    ssh_check_frozen(ssh);
}

/*
 * Throttle or unthrottle _all_ local data streams (for when sends
 * on the SSH connection itself back up).
 */
static void ssh_throttle_all(Ssh *ssh, bool enable, size_t bufsize)
{
    if (enable == ssh->throttled_all)
	return;
    ssh->throttled_all = enable;
    ssh->overall_bufsize = bufsize;

    ssh_throttle_all_channels(ssh->cl, enable);
}

static void ssh_cache_conf_values(Ssh *ssh)
{
    ssh->pls.omit_passwords = conf_get_bool(ssh->conf, CONF_logomitpass);
    ssh->pls.omit_data = conf_get_bool(ssh->conf, CONF_logomitdata);
}

/*
 * Called to set up the connection.
 *
 * Returns an error message, or NULL on success.
 */
static const char *ssh_init(Seat *seat, Backend **backend_handle,
                            LogContext *logctx, Conf *conf,
                            const char *host, int port, char **realhost,
			    bool nodelay, bool keepalive)
{
    const char *p;
    Ssh *ssh;

    ssh = snew(Ssh);
    memset(ssh, 0, sizeof(Ssh));

    ssh->conf = conf_copy(conf);
    ssh_cache_conf_values(ssh);
    ssh->exitcode = -1;
    ssh->pls.kctx = SSH2_PKTCTX_NOKEX;
    ssh->pls.actx = SSH2_PKTCTX_NOAUTH;
    bufchain_init(&ssh->in_raw);
    bufchain_init(&ssh->out_raw);
    bufchain_init(&ssh->user_input);
    ssh->ic_out_raw.fn = ssh_bpp_output_raw_data_callback;
    ssh->ic_out_raw.ctx = ssh;

    ssh->backend.vt = &ssh_backend;
    *backend_handle = &ssh->backend;

    ssh->seat = seat;
    ssh->cl_dummy.logctx = ssh->logctx = logctx;

    random_ref(); /* do this now - may be needed by sharing setup code */
    ssh->need_random_unref = true;

    p = connect_to_host(ssh, host, port, realhost, nodelay, keepalive);
    if (p != NULL) {
        /* Call random_unref now instead of waiting until the caller
         * frees this useless Ssh object, in case the caller is
         * impatient and just exits without bothering, in which case
         * the random seed won't be re-saved. */
        ssh->need_random_unref = false;
        random_unref();
	return p;
    }

    return NULL;
}

static void ssh_free(Backend *be)
{
    Ssh *ssh = container_of(be, Ssh, backend);
    bool need_random_unref;

    ssh_shutdown(ssh);

    conf_free(ssh->conf);
    if (ssh->connshare)
        sharestate_free(ssh->connshare);
    sfree(ssh->savedhost);
    sfree(ssh->fullhostname);
    sfree(ssh->specials);

#ifndef NO_GSSAPI
    if (ssh->gss_state.srv_name)
        ssh->gss_state.lib->release_name( 
            ssh->gss_state.lib, &ssh->gss_state.srv_name);
    if (ssh->gss_state.ctx != NULL)
        ssh->gss_state.lib->release_cred(
            ssh->gss_state.lib, &ssh->gss_state.ctx);
    if (ssh->gss_state.libs)
	ssh_gss_cleanup(ssh->gss_state.libs);
#endif

    delete_callbacks_for_context(ssh); /* likely to catch ic_out_raw */

    need_random_unref = ssh->need_random_unref;
    sfree(ssh);

    if (need_random_unref)
        random_unref();
}

/*
 * Reconfigure the SSH backend.
 */
static void ssh_reconfig(Backend *be, Conf *conf)
{
    Ssh *ssh = container_of(be, Ssh, backend);

    if (ssh->pinger)
        pinger_reconfig(ssh->pinger, ssh->conf, conf);

    ssh_ppl_reconfigure(ssh->base_layer, conf);

    conf_free(ssh->conf);
    ssh->conf = conf_copy(conf);
    ssh_cache_conf_values(ssh);
}

/*
 * Called to send data down the SSH connection.
 */
static size_t ssh_send(Backend *be, const char *buf, size_t len)
{
    Ssh *ssh = container_of(be, Ssh, backend);

    if (ssh == NULL || ssh->s == NULL)
	return 0;

    bufchain_add(&ssh->user_input, buf, len);
    if (ssh->base_layer)
        ssh_ppl_got_user_input(ssh->base_layer);

    return backend_sendbuffer(&ssh->backend);
}

/*
 * Called to query the current amount of buffered stdin data.
 */
static size_t ssh_sendbuffer(Backend *be)
{
    Ssh *ssh = container_of(be, Ssh, backend);
    size_t backlog;

    if (!ssh || !ssh->s || !ssh->cl)
	return 0;

    backlog = ssh_stdin_backlog(ssh->cl);

    /* FIXME: also include sizes of pqs */

    /*
     * If the SSH socket itself has backed up, add the total backup
     * size on that to any individual buffer on the stdin channel.
     */
    if (ssh->throttled_all)
	backlog += ssh->overall_bufsize;

    return backlog;
}

/*
 * Called to set the size of the window from SSH's POV.
 */
static void ssh_size(Backend *be, int width, int height)
{
    Ssh *ssh = container_of(be, Ssh, backend);

    ssh->term_width = width;
    ssh->term_height = height;
    if (ssh->cl)
        ssh_terminal_size(ssh->cl, ssh->term_width, ssh->term_height);
}

struct ssh_add_special_ctx {
    SessionSpecial *specials;
    size_t nspecials, specials_size;
};

static void ssh_add_special(void *vctx, const char *text,
                            SessionSpecialCode code, int arg)
{
    struct ssh_add_special_ctx *ctx = (struct ssh_add_special_ctx *)vctx;
    SessionSpecial *spec;

    sgrowarray(ctx->specials, ctx->specials_size, ctx->nspecials);
    spec = &ctx->specials[ctx->nspecials++];
    spec->name = text;
    spec->code = code;
    spec->arg = arg;
}

/*
 * Return a list of the special codes that make sense in this
 * protocol.
 */
static const SessionSpecial *ssh_get_specials(Backend *be)
{
    Ssh *ssh = container_of(be, Ssh, backend);

    /*
     * Ask all our active protocol layers what specials they've got,
     * and amalgamate the list into one combined one.
     */

    struct ssh_add_special_ctx ctx;

    ctx.specials = NULL;
    ctx.nspecials = ctx.specials_size = 0;

    if (ssh->base_layer)
        ssh_ppl_get_specials(ssh->base_layer, ssh_add_special, &ctx);

    if (ctx.specials) {
        /* If the list is non-empty, terminate it with a SS_EXITMENU. */
        ssh_add_special(&ctx, NULL, SS_EXITMENU, 0);
    }

    sfree(ssh->specials);
    ssh->specials = ctx.specials;
    return ssh->specials;
}

/*
 * Send special codes.
 */
static void ssh_special(Backend *be, SessionSpecialCode code, int arg)
{
    Ssh *ssh = container_of(be, Ssh, backend);

    if (ssh->base_layer)
        ssh_ppl_special_cmd(ssh->base_layer, code, arg);
}

/*
 * This is called when the seat's output channel manages to clear some
 * backlog.
 */
static void ssh_unthrottle(Backend *be, size_t bufsize)
{
    Ssh *ssh = container_of(be, Ssh, backend);

    if (ssh->cl)
        ssh_stdout_unthrottle(ssh->cl, bufsize);
}

static bool ssh_connected(Backend *be)
{
    Ssh *ssh = container_of(be, Ssh, backend);
    return ssh->s != NULL;
}

static bool ssh_sendok(Backend *be)
{
    Ssh *ssh = container_of(be, Ssh, backend);
    return ssh->base_layer && ssh_ppl_want_user_input(ssh->base_layer);
}

void ssh_ldisc_update(Ssh *ssh)
{
    /* Called when the connection layer wants to propagate an update
     * to the line discipline options */
    if (ssh->ldisc)
	ldisc_echoedit_update(ssh->ldisc);
}

static bool ssh_ldisc(Backend *be, int option)
{
    Ssh *ssh = container_of(be, Ssh, backend);
    return ssh->cl ? ssh_ldisc_option(ssh->cl, option) : false;
}

static void ssh_provide_ldisc(Backend *be, Ldisc *ldisc)
{
    Ssh *ssh = container_of(be, Ssh, backend);
    ssh->ldisc = ldisc;
}

void ssh_got_exitcode(Ssh *ssh, int exitcode)
{
    ssh->exitcode = exitcode;
}

static int ssh_return_exitcode(Backend *be)
{
    Ssh *ssh = container_of(be, Ssh, backend);
    if (ssh->s && (!ssh->session_started || ssh->base_layer))
        return -1;
    else
        return (ssh->exitcode >= 0 ? ssh->exitcode : INT_MAX);
}

/*
 * cfg_info for SSH is the protocol running in this session.
 * (1 or 2 for the full SSH-1 or SSH-2 protocol; -1 for the bare
 * SSH-2 connection protocol, i.e. a downstream; 0 for not-decided-yet.)
 */
static int ssh_cfg_info(Backend *be)
{
    Ssh *ssh = container_of(be, Ssh, backend);
    if (ssh->version == 0)
	return 0; /* don't know yet */
    else if (ssh->bare_connection)
	return -1;
    else
	return ssh->version;
}

/*
 * Gross hack: pscp will try to start SFTP but fall back to scp1 if
 * that fails. This variable is the means by which scp.c can reach
 * into the SSH code and find out which one it got.
 */
extern bool ssh_fallback_cmd(Backend *be)
{
    Ssh *ssh = container_of(be, Ssh, backend);
    return ssh->fallback_cmd;
}

void ssh_got_fallback_cmd(Ssh *ssh)
{
    ssh->fallback_cmd = true;
}

const struct BackendVtable ssh_backend = {
    ssh_init,
    ssh_free,
    ssh_reconfig,
    ssh_send,
    ssh_sendbuffer,
    ssh_size,
    ssh_special,
    ssh_get_specials,
    ssh_connected,
    ssh_return_exitcode,
    ssh_sendok,
    ssh_ldisc,
    ssh_provide_ldisc,
    ssh_unthrottle,
    ssh_cfg_info,
    ssh_test_for_upstream,
    "ssh",
    PROT_SSH,
    22
};
