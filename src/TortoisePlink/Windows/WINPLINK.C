/*
 * PLink - a Windows command-line (stdin/stdout) variant of PuTTY.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>

#include "putty.h"
#include "storage.h"
#include "tree234.h"
#include "winsecur.h"

extern HWND GetParentHwnd();

#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

void cmdline_error(const char *fmt, ...)
{
    va_list ap;
    char *stuff, morestuff[100];
    va_start(ap, fmt);
    stuff = dupvprintf(fmt, ap);
    va_end(ap);
    sprintf(morestuff, "%.70s Command Line Error", appname);
    MessageBox(GetParentHwnd(), stuff, morestuff, MB_ICONERROR | MB_OK);
    sfree(stuff);
    exit(1);
}

static HANDLE inhandle, outhandle, errhandle;
static struct handle *stdin_handle, *stdout_handle, *stderr_handle;
static handle_sink stdout_hs, stderr_hs;
static StripCtrlChars *stdout_scc, *stderr_scc;
static BinarySink *stdout_bs, *stderr_bs;
static DWORD orig_console_mode;

static Backend *backend;
static LogContext *logctx;
static Conf *conf;

static void plink_echoedit_update(Seat *seat, bool echo, bool edit)
{
    /* Update stdin read mode to reflect changes in line discipline. */
    DWORD mode;

    mode = ENABLE_PROCESSED_INPUT;
    if (echo)
        mode = mode | ENABLE_ECHO_INPUT;
    else
        mode = mode & ~ENABLE_ECHO_INPUT;
    if (edit)
        mode = mode | ENABLE_LINE_INPUT;
    else
        mode = mode & ~ENABLE_LINE_INPUT;
    SetConsoleMode(inhandle, mode);
}

static size_t plink_output(
    Seat *seat, bool is_stderr, const void *data, size_t len)
{
    BinarySink *bs = is_stderr ? stderr_bs : stdout_bs;
    put_data(bs, data, len);

    return handle_backlog(stdout_handle) + handle_backlog(stderr_handle);
}

static bool plink_eof(Seat *seat)
{
    handle_write_eof(stdout_handle);
    return false;   /* do not respond to incoming EOF with outgoing */
}

static int plink_get_userpass_input(Seat *seat, prompts_t *p, bufchain *input)
{
    int ret;
    ret = cmdline_get_passwd_input(p);
    if (ret == -1)
        ret = console_get_userpass_input(p);
    return ret;
}

static bool plink_seat_interactive(Seat *seat)
{
    return (!*conf_get_str(conf, CONF_remote_cmd) &&
            !*conf_get_str(conf, CONF_remote_cmd2) &&
            !*conf_get_str(conf, CONF_ssh_nc_host));
}

static const SeatVtable plink_seat_vt = {
    .output = plink_output,
    .eof = plink_eof,
    .get_userpass_input = plink_get_userpass_input,
    .notify_remote_exit = nullseat_notify_remote_exit,
    .connection_fatal = console_connection_fatal,
    .update_specials_menu = nullseat_update_specials_menu,
    .get_ttymode = nullseat_get_ttymode,
    .set_busy_status = nullseat_set_busy_status,
    .verify_ssh_host_key = console_verify_ssh_host_key,
    .confirm_weak_crypto_primitive = console_confirm_weak_crypto_primitive,
    .confirm_weak_cached_hostkey = console_confirm_weak_cached_hostkey,
    .is_utf8 = nullseat_is_never_utf8,
    .echoedit_update = plink_echoedit_update,
    .get_x_display = nullseat_get_x_display,
    .get_windowid = nullseat_get_windowid,
    .get_window_pixel_size = nullseat_get_window_pixel_size,
    .stripctrl_new = console_stripctrl_new,
    .set_trust_status = console_set_trust_status,
    .verbose = cmdline_seat_verbose,
    .interactive = plink_seat_interactive,
    .get_cursor_position = nullseat_get_cursor_position,
};
static Seat plink_seat[1] = {{ &plink_seat_vt }};

static DWORD main_thread_id;

/*
 *  Short description of parameters.
 */
static void usage(void)
{
    char buf[10000];
    int j = 0;
    j += sprintf(buf + j, "TortoiseGitPlink: command-line connection utility (based on PuTTY Plink)\n");
    j += sprintf(buf + j, "%s\n", ver);
    j += sprintf(buf + j, "Usage: tortoisegitplink [options] [user@]host [command]\n");
    j += sprintf(buf + j, "       (\"host\" can also be a PuTTY saved session name)\n");
    j += sprintf(buf + j, "Options:\n");
    j += sprintf(buf + j, "  -V        print version information and exit\n");
    j += sprintf(buf + j, "  -pgpfp    print PGP key fingerprints and exit\n");
    j += sprintf(buf + j, "  -v        show verbose messages\n");
    j += sprintf(buf + j, "  -load sessname  Load settings from saved session\n");
    j += sprintf(buf + j, "  -ssh -telnet -rlogin -raw -serial\n");
    j += sprintf(buf + j, "            force use of a particular protocol\n");
    j += sprintf(buf + j, "  -ssh-connection\n");
    j += sprintf(buf + j, "            force use of the bare ssh-connection protocol\n");
    j += sprintf(buf + j, "  -P port   connect to specified port\n");
    j += sprintf(buf + j, "  -l user   connect with specified username\n");
    j += sprintf(buf + j, "  -batch    disable all interactive prompts (ignored)\n");
    j += sprintf(buf + j, "  -proxycmd command\n");
    j += sprintf(buf + j, "            use 'command' as local proxy\n");
    j += sprintf(buf + j, "  -sercfg configuration-string (e.g. 19200,8,n,1,X)\n");
    j += sprintf(buf + j, "            Specify the serial configuration (serial only)\n");
    j += sprintf(buf + j, "The following options only apply to SSH connections:\n");
    j += sprintf(buf + j, "  -pw passw login with specified password\n");
    j += sprintf(buf + j, "  -D [listen-IP:]listen-port\n");
    j += sprintf(buf + j, "            Dynamic SOCKS-based port forwarding\n");
    j += sprintf(buf + j, "  -L [listen-IP:]listen-port:host:port\n");
    j += sprintf(buf + j, "            Forward local port to remote address\n");
    j += sprintf(buf + j, "  -R [listen-IP:]listen-port:host:port\n");
    j += sprintf(buf + j, "            Forward remote port to local address\n");
    j += sprintf(buf + j, "  -X -x     enable / disable X11 forwarding\n");
    j += sprintf(buf + j, "  -A -a     enable / disable agent forwarding\n");
    j += sprintf(buf + j, "  -t -T     enable / disable pty allocation\n");
    j += sprintf(buf + j, "  -1 -2     force use of particular SSH protocol version\n");
    j += sprintf(buf + j, "  -4 -6     force use of IPv4 or IPv6\n");
    j += sprintf(buf + j, "  -C        enable compression\n");
    j += sprintf(buf + j, "  -i key    private key file for user authentication\n");
    j += sprintf(buf + j, "  -noagent  disable use of Pageant\n");
    j += sprintf(buf + j, "  -agent    enable use of Pageant\n");
    j += sprintf(buf + j, "  -no-trivial-auth\n");
    j += sprintf(buf + j, "            disconnect if SSH authentication succeeds trivially\n");
    j += sprintf(buf + j, "  -noshare  disable use of connection sharing\n");
    j += sprintf(buf + j, "  -share    enable use of connection sharing\n");
    j += sprintf(buf + j, "  -hostkey keyid\n");
    j += sprintf(buf + j, "            manually specify a host key (may be repeated)\n");
    j += sprintf(buf + j, "  -sanitise-stderr, -sanitise-stdout,\n");
    j += sprintf(buf + j, "  -no-sanitise-stderr, -no-sanitise-stdout\n");
    j += sprintf(buf + j, "            do/don't strip control chars from standard\n");
    j += sprintf(buf + j, "            output/error\n");
    j += sprintf(buf + j, "  -no-antispoof   omit anti-spoofing prompt after\n");
    j += sprintf(buf + j, "            authentication\n");
    j += sprintf(buf + j, "  -m file   read remote command(s) from file\n");
    j += sprintf(buf + j, "  -s        remote command is an SSH subsystem (SSH-2 only)\n");
    j += sprintf(buf + j, "  -N        don't start a shell/command (SSH-2 only)\n");
    j += sprintf(buf + j, "  -nc host:port\n");
    j += sprintf(buf + j, "            open tunnel in place of session (SSH-2 only)\n");
    j += sprintf(buf + j, "  -sshlog file\n");
    j += sprintf(buf + j, "  -sshrawlog file\n");
    j += sprintf(buf + j, "            log protocol details to a file\n");
    j += sprintf(buf + j, "  -logoverwrite\n");
    j += sprintf(buf + j, "  -logappend\n");
    j += sprintf(buf + j, "            control what happens when a log file already exists\n");
    j += sprintf(buf + j, "  -shareexists\n");
    j += sprintf(buf + j, "            test whether a connection-sharing upstream exists\n");
    MessageBox(NULL, buf, "TortoiseGitPlink", MB_ICONINFORMATION);
    exit(1);
}

static void version(void)
{
    char buf[1000];
    char* buildinfo_text = buildinfo("\n");
    sprintf(buf, "TortoiseGitPlink: %s\n%s\n", ver, buildinfo_text);
    sfree(buildinfo_text);
    MessageBox(NULL, buf, "TortoiseGitPlink", MB_ICONINFORMATION);
    exit(0);
}

size_t stdin_gotdata(struct handle *h, const void *data, size_t len, int err)
{
    if (err) {
        char buf[4096];
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, 0,
                      buf, lenof(buf), NULL);
        buf[lenof(buf)-1] = '\0';
        if (buf[strlen(buf)-1] == '\n')
            buf[strlen(buf)-1] = '\0';
        fprintf(stderr, "Unable to read from standard input: %s\n", buf);
        cleanup_exit(0);
    }

    noise_ultralight(NOISE_SOURCE_IOLEN, len);
    if (backend_connected(backend)) {
        if (len > 0) {
            return backend_send(backend, data, len);
        } else {
            backend_special(backend, SS_EOF, 0);
            return 0;
        }
    } else
        return 0;
}

void stdouterr_sent(struct handle *h, size_t new_backlog, int err)
{
    if (err) {
        char buf[4096];
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, 0,
                      buf, lenof(buf), NULL);
        buf[lenof(buf)-1] = '\0';
        if (buf[strlen(buf)-1] == '\n')
            buf[strlen(buf)-1] = '\0';
        fprintf(stderr, "Unable to write to standard %s: %s\n",
                (h == stdout_handle ? "output" : "error"), buf);
        cleanup_exit(0);
    }

    if (backend_connected(backend)) {
        backend_unthrottle(backend, (handle_backlog(stdout_handle) +
                                     handle_backlog(stderr_handle)));
    }
}

const bool share_can_be_downstream = true;
const bool share_can_be_upstream = true;

const unsigned cmdline_tooltype =
    TOOLTYPE_HOST_ARG |
    TOOLTYPE_HOST_ARG_CAN_BE_SESSION |
    TOOLTYPE_HOST_ARG_PROTOCOL_PREFIX |
    TOOLTYPE_HOST_ARG_FROM_LAUNCHABLE_LOAD;

static bool sending;

static bool plink_mainloop_pre(void *vctx, const HANDLE **extra_handles,
                               size_t *n_extra_handles)
{
    if (!sending && backend_sendok(backend)) {
        stdin_handle = handle_input_new(inhandle, stdin_gotdata, NULL,
                                        0);
        sending = true;
    }

    return true;
}

static bool plink_mainloop_post(void *vctx, size_t extra_handle_index)
{
    if (sending)
        handle_unthrottle(stdin_handle, backend_sendbuffer(backend));

    if (!backend_connected(backend) &&
        handle_backlog(stdout_handle) + handle_backlog(stderr_handle) == 0)
        return false; /* we closed the connection */

    return true;
}

int main(int argc, char **argv)
{
    int exitcode;
    bool errors;
    bool use_subsystem = false;
    bool just_test_share_exists = false;
    enum TriState sanitise_stdout = AUTO, sanitise_stderr = AUTO;
    const struct BackendVtable *vt;

    dll_hijacking_protection();

    InitCommonControls();
    /*
     * Initialise port and protocol to sensible defaults. (These
     * will be overridden by more or less anything.)
     */
    settings_set_default_protocol(PROT_SSH);
    settings_set_default_port(22);

    /*
     * Process the command line.
     */
    conf = conf_new();
    do_defaults(NULL, conf);
    settings_set_default_protocol(PROT_SSH);
    settings_set_default_port(22);
    errors = false;
    conf_set_bool(conf, CONF_agentfwd, FALSE);
    conf_set_bool(conf, CONF_x11_forward, FALSE);
    bool skipFurtherParameters = false;
    while (--argc) {
        char *p = *++argv;
        if (p && !strcmp(p, "--")) {
            skipFurtherParameters = true;
            continue;
        }
        int ret = cmdline_process_param(p, (argc > 1 ? argv[1] : NULL),
                                        1, conf, skipFurtherParameters);
        if (ret == -2) {
            fprintf(stderr,
                    "TortoiseGitPlink: option \"%s\" requires an argument\n", p);
            errors = true;
        } else if (ret == 2) {
            --argc, ++argv;
        } else if (ret == 1) {
            continue;
        } else if (!strcmp(p, "-batch") && !skipFurtherParameters) {
            // ignore and do not print an error message
        } else if (!strcmp(p, "-s") && !skipFurtherParameters) {
            /* Save status to write to conf later. */
            use_subsystem = true;
        } else if ((!strcmp(p, "-V") || !strcmp(p, "--version")) && !skipFurtherParameters) {
            version();
        } else if (!strcmp(p, "--help") && !skipFurtherParameters) {
            usage();
        } else if (!strcmp(p, "-pgpfp") && !skipFurtherParameters) {
            pgp_fingerprints();
            exit(1);
        } else if (!strcmp(p, "-shareexists") && !skipFurtherParameters) {
            just_test_share_exists = true;
        } else if ((!strcmp(p, "-sanitise-stdout") ||
                   !strcmp(p, "-sanitize-stdout")) && !skipFurtherParameters) {
            sanitise_stdout = FORCE_ON;
        } else if ((!strcmp(p, "-no-sanitise-stdout") ||
                   !strcmp(p, "-no-sanitize-stdout"))  && !skipFurtherParameters) {
            sanitise_stdout = FORCE_OFF;
        } else if ((!strcmp(p, "-sanitise-stderr") ||
                   !strcmp(p, "-sanitize-stderr")) && !skipFurtherParameters) {
            sanitise_stderr = FORCE_ON;
        } else if ((!strcmp(p, "-no-sanitise-stderr") ||
                   !strcmp(p, "-no-sanitize-stderr")) && !skipFurtherParameters) {
            sanitise_stderr = FORCE_OFF;
        } else if (!strcmp(p, "-no-antispoof") && !skipFurtherParameters) {
            console_antispoof_prompt = false;
        } else if (*p != '-' || skipFurtherParameters) {
            strbuf *cmdbuf = strbuf_new();

            while (argc > 0) {
                if (cmdbuf->len > 0)
                    put_byte(cmdbuf, ' '); /* add space separator */
                put_datapl(cmdbuf, ptrlen_from_asciz(p));
                if (--argc > 0)
                    p = *++argv;
            }

            conf_set_str(conf, CONF_remote_cmd, cmdbuf->s);
            conf_set_str(conf, CONF_remote_cmd2, "");
            conf_set_bool(conf, CONF_nopty, true);  /* command => no tty */

            strbuf_free(cmdbuf);
            break;                     /* done with cmdline */
        } else {
            fprintf(stderr, "TortoiseGitPlink: unknown option \"%s\"\n", p);
            errors = true;
        }
    }

    if (errors)
        return 1;

    if (!cmdline_host_ok(conf)) {
        usage();
    }

    prepare_session(conf);

    /*
     * Perform command-line overrides on session configuration.
     */
    cmdline_run_saved(conf);

    /*
     * Apply subsystem status.
     */
    if (use_subsystem)
        conf_set_bool(conf, CONF_ssh_subsys, true);

    /*
     * Select protocol. This is farmed out into a table in a
     * separate file to enable an ssh-free variant.
     */
    vt = backend_vt_from_proto(conf_get_int(conf, CONF_protocol));
    if (vt == NULL) {
        fprintf(stderr,
                "Internal fault: Unsupported protocol found\n");
        return 1;
    }

    if (vt->flags & BACKEND_NEEDS_TERMINAL) {
        fprintf(stderr,
                "TortoiseGitPlink doesn't support %s, which needs terminal emulation\n",
                vt->displayname);
        return 1;
    }

    sk_init();
    if (p_WSAEventSelect == NULL) {
        fprintf(stderr, "TortoiseGitPlink requires WinSock 2\n");
        return 1;
    }

    /*
     * Plink doesn't provide any way to add forwardings after the
     * connection is set up, so if there are none now, we can safely set
     * the "simple" flag.
     */
    if (conf_get_int(conf, CONF_protocol) == PROT_SSH &&
        !conf_get_bool(conf, CONF_x11_forward) &&
        !conf_get_bool(conf, CONF_agentfwd) &&
        !conf_get_str_nthstrkey(conf, CONF_portfwd, 0))
        conf_set_bool(conf, CONF_ssh_simple, true);

    logctx = log_init(console_cli_logpolicy, conf);

    if (just_test_share_exists) {
        if (!vt->test_for_upstream) {
            fprintf(stderr, "Connection sharing not supported for this "
                    "connection type (%s)'\n", vt->displayname);
            return 1;
        }
        if (vt->test_for_upstream(conf_get_str(conf, CONF_host),
                                  conf_get_int(conf, CONF_port), conf))
            return 0;
        else
            return 1;
    }

    if (restricted_acl()) {
        lp_eventlog(console_cli_logpolicy,
                    "Running with restricted process ACL");
    }

    inhandle = GetStdHandle(STD_INPUT_HANDLE);
    outhandle = GetStdHandle(STD_OUTPUT_HANDLE);
    errhandle = GetStdHandle(STD_ERROR_HANDLE);

    /*
     * Turn off ECHO and LINE input modes. We don't care if this
     * call fails, because we know we aren't necessarily running in
     * a console.
     */
    GetConsoleMode(inhandle, &orig_console_mode);
    SetConsoleMode(inhandle, ENABLE_PROCESSED_INPUT);

    /*
     * Pass the output handles to the handle-handling subsystem.
     * (The input one we leave until we're through the
     * authentication process.)
     */
    stdout_handle = handle_output_new(outhandle, stdouterr_sent, NULL, 0);
    stderr_handle = handle_output_new(errhandle, stdouterr_sent, NULL, 0);
    handle_sink_init(&stdout_hs, stdout_handle);
    handle_sink_init(&stderr_hs, stderr_handle);
    stdout_bs = BinarySink_UPCAST(&stdout_hs);
    stderr_bs = BinarySink_UPCAST(&stderr_hs);

    /*
     * Decide whether to sanitise control sequences out of standard
     * output and standard error.
     *
     * If we weren't given a command-line override, we do this if (a)
     * the fd in question is pointing at a console, and (b) we aren't
     * trying to allocate a terminal as part of the session.
     *
     * (Rationale: the risk of control sequences is that they cause
     * confusion when sent to a local console, so if there isn't one,
     * no problem. Also, if we allocate a remote terminal, then we
     * sent a terminal type, i.e. we told it what kind of escape
     * sequences we _like_, i.e. we were expecting to receive some.)
     */
    if (sanitise_stdout == FORCE_ON ||
        (sanitise_stdout == AUTO && is_console_handle(outhandle) &&
         conf_get_bool(conf, CONF_nopty))) {
        stdout_scc = stripctrl_new(stdout_bs, true, L'\0');
        stdout_bs = BinarySink_UPCAST(stdout_scc);
    }
    if (sanitise_stderr == FORCE_ON ||
        (sanitise_stderr == AUTO && is_console_handle(errhandle) &&
         conf_get_bool(conf, CONF_nopty))) {
        stderr_scc = stripctrl_new(stderr_bs, true, L'\0');
        stderr_bs = BinarySink_UPCAST(stderr_scc);
    }

    /*
     * Start up the connection.
     */
    winselcli_setup();                 /* ensure event object exists */
    {
        char *error, *realhost;
        /* nodelay is only useful if stdin is a character device (console) */
        bool nodelay = conf_get_bool(conf, CONF_tcp_nodelay) &&
            (GetFileType(GetStdHandle(STD_INPUT_HANDLE)) == FILE_TYPE_CHAR);

        error = backend_init(vt, plink_seat, &backend, logctx, conf,
                             conf_get_str(conf, CONF_host),
                             conf_get_int(conf, CONF_port),
                             &realhost, nodelay,
                             conf_get_bool(conf, CONF_tcp_keepalives));
        if (error) {
            fprintf(stderr, "Unable to open connection:\n%s", error);
            sfree(error);
            return 1;
        }
        ldisc_create(conf, NULL, backend, plink_seat);
        sfree(realhost);
    }

    main_thread_id = GetCurrentThreadId();

    sending = false;

    cli_main_loop(plink_mainloop_pre, plink_mainloop_post, NULL);

    exitcode = backend_exitcode(backend);
    if (exitcode < 0) {
        fprintf(stderr, "Remote process exit code unavailable\n");
        exitcode = 1;                  /* this is an error condition */
    }
    cleanup_exit(exitcode);
    return 0;                          /* placate compiler warning */
}

int WinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow)
{
    main(__argc, __argv);
}
