/*
 * PLink - a Windows command-line (stdin/stdout) variant of PuTTY.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>

#define PUTTY_DO_GLOBALS	       /* actually _define_ globals */
#include "putty.h"
#include "storage.h"
#include "tree234.h"

#define WM_AGENT_CALLBACK (WM_APP + 4)

struct agent_callback {
    void (*callback)(void *, void *, int);
    void *callback_ctx;
    void *data;
    int len;
};

void fatalbox(char *p, ...)
{
	va_list ap;
	char *stuff, morestuff[100];

	va_start(ap, p);
	stuff = dupvprintf(p, ap);
	va_end(ap);
	sprintf(morestuff, "%.70s Fatal Error", appname);
	MessageBox(GetParentHwnd(), stuff, morestuff, MB_ICONERROR | MB_OK);
	sfree(stuff);
    if (logctx) {
        log_free(logctx);
        logctx = NULL;
    }
	cleanup_exit(1);
}
void modalfatalbox(char *p, ...)
{
	va_list ap;
	char *stuff, morestuff[100];

	va_start(ap, p);
	stuff = dupvprintf(p, ap);
	va_end(ap);
	sprintf(morestuff, "%.70s Fatal Error", appname);
	MessageBox(GetParentHwnd(), stuff, morestuff,
		MB_SYSTEMMODAL | MB_ICONERROR | MB_OK);
	sfree(stuff);
    if (logctx) {
        log_free(logctx);
        logctx = NULL;
    }
    cleanup_exit(1);
}
void nonfatal(char *p, ...)
{
    va_list ap;
    char *stuff, morestuff[100];

    va_start(ap, p);
    stuff = dupvprintf(p, ap);
    va_end(ap);
    sprintf(morestuff, "%.70s Fatal Error", appname);
    MessageBox(GetParentHwnd(), stuff, morestuff,
        MB_SYSTEMMODAL | MB_ICONERROR | MB_OK);
    sfree(stuff);
}
void connection_fatal(void *frontend, char *p, ...)
{
    va_list ap;
	char *stuff, morestuff[100];

	va_start(ap, p);
	stuff = dupvprintf(p, ap);
	va_end(ap);
	sprintf(morestuff, "%.70s Fatal Error", appname);
	MessageBox(GetParentHwnd(), stuff, morestuff,
		MB_SYSTEMMODAL | MB_ICONERROR | MB_OK);
	sfree(stuff);
    if (logctx) {
        log_free(logctx);
        logctx = NULL;
    }
	cleanup_exit(1);
}
void cmdline_error(char *p, ...)
{
	va_list ap;
	char *stuff, morestuff[100];

	va_start(ap, p);
	stuff = dupvprintf(p, ap);
	va_end(ap);
	sprintf(morestuff, "%.70s Command Line Error", appname);
	MessageBox(GetParentHwnd(), stuff, morestuff, MB_ICONERROR | MB_OK);
	sfree(stuff);
    exit(1);
}

HANDLE inhandle, outhandle, errhandle;
struct handle *stdin_handle, *stdout_handle, *stderr_handle;
DWORD orig_console_mode;
int connopen;

WSAEVENT netevent;

static Backend *back;
static void *backhandle;
static Conf *conf;

int term_ldisc(Terminal *term, int mode)
{
    return FALSE;
}
void ldisc_update(void *frontend, int echo, int edit)
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

char *get_ttymode(void *frontend, const char *mode) { return NULL; }

int from_backend(void *frontend_handle, int is_stderr,
		 const char *data, int len)
{
    if (is_stderr) {
	handle_write(stderr_handle, data, len);
    } else {
	handle_write(stdout_handle, data, len);
    }

    return handle_backlog(stdout_handle) + handle_backlog(stderr_handle);
}

int from_backend_untrusted(void *frontend_handle, const char *data, int len)
{
    /*
     * No "untrusted" output should get here (the way the code is
     * currently, it's all diverted by FLAG_STDERR).
     */
    assert(!"Unexpected call to from_backend_untrusted()");
    return 0; /* not reached */
}

int from_backend_eof(void *frontend_handle)
{
    handle_write_eof(stdout_handle);
    return FALSE;   /* do not respond to incoming EOF with outgoing */
}

int get_userpass_input(prompts_t *p, unsigned char *in, int inlen)
{
    int ret;
    ret = cmdline_get_passwd_input(p, in, inlen);
    if (ret == -1)
	ret = console_get_userpass_input(p, in, inlen);
    return ret;
}

static DWORD main_thread_id;

void agent_schedule_callback(void (*callback)(void *, void *, int),
			     void *callback_ctx, void *data, int len)
{
    struct agent_callback *c = snew(struct agent_callback);
    c->callback = callback;
    c->callback_ctx = callback_ctx;
    c->data = data;
    c->len = len;
    PostThreadMessage(main_thread_id, WM_AGENT_CALLBACK, 0, (LPARAM)c);
}

/*
 *  Short description of parameters.
 */
static void usage(void)
{
	char buf[10000];
	int j = 0;

	j += sprintf(buf+j, "TortoiseGitPlink: command-line connection utility (based on PuTTY Plink)\n");
    j += sprintf(buf+j, "%s\n", ver);
    j += sprintf(buf+j, "Usage: tortoisegitplink [options] [user@]host [command]\n");
    j += sprintf(buf+j, "       (\"host\" can also be a PuTTY saved session name)\n");
    j += sprintf(buf+j, "Options:\n");
    j += sprintf(buf+j, "  -V        print version information and exit\n");
    j += sprintf(buf+j, "  -pgpfp    print PGP key fingerprints and exit\n");
    j += sprintf(buf+j, "  -v        show verbose messages\n");
    j += sprintf(buf+j, "  -load sessname  Load settings from saved session\n");
    j += sprintf(buf+j, "  -ssh -telnet -rlogin -raw -serial\n");
    j += sprintf(buf+j, "            force use of a particular protocol\n");
    j += sprintf(buf+j, "  -P port   connect to specified port\n");
    j += sprintf(buf+j, "  -l user   connect with specified username\n");
    j += sprintf(buf+j, "  -sercfg configuration-string (e.g. 19200,8,n,1,X)\n");
    j += sprintf(buf+j, "            Specify the serial configuration (serial only)\n");
    j += sprintf(buf+j, "The following options only apply to SSH connections:\n");
    j += sprintf(buf+j, "  -pw passw login with specified password\n");
    j += sprintf(buf+j, "  -D [listen-IP:]listen-port\n");
    j += sprintf(buf+j, "            Dynamic SOCKS-based port forwarding\n");
    j += sprintf(buf+j, "  -L [listen-IP:]listen-port:host:port\n");
    j += sprintf(buf+j, "            Forward local port to remote address\n");
    j += sprintf(buf+j, "  -R [listen-IP:]listen-port:host:port\n");
    j += sprintf(buf+j, "            Forward remote port to local address\n");
    j += sprintf(buf+j, "  -X -x     enable / disable X11 forwarding\n");
    j += sprintf(buf+j, "  -A -a     enable / disable agent forwarding\n");
    j += sprintf(buf+j, "  -t -T     enable / disable pty allocation\n");
    j += sprintf(buf+j, "  -1 -2     force use of particular protocol version\n");
    j += sprintf(buf+j, "  -4 -6     force use of IPv4 or IPv6\n");
    j += sprintf(buf+j, "  -C        enable compression\n");
    j += sprintf(buf+j, "  -i key    private key file for user authentication\n");
    j += sprintf(buf+j, "  -noagent  disable use of Pageant\n");
    j += sprintf(buf+j, "  -agent    enable use of Pageant\n");
    j += sprintf(buf+j, "  -hostkey aa:bb:cc:...\n");
    j += sprintf(buf+j, "            manually specify a host key (may be repeated)\n");
    j += sprintf(buf+j, "  -m file   read remote command(s) from file\n");
    j += sprintf(buf+j, "  -s        remote command is an SSH subsystem (SSH-2 only)\n");
    j += sprintf(buf+j, "  -N        don't start a shell/command (SSH-2 only)\n");
    j += sprintf(buf+j, "  -nc host:port\n");
    j += sprintf(buf+j, "            open tunnel in place of session (SSH-2 only)\n");
	MessageBox(NULL, buf, "TortoiseGitPlink", MB_ICONINFORMATION);
	exit(1);
}

static void version(void)
{
	printf("TortoiseGitPlink: %s\n", ver);
	exit(1);
}

char *do_select(SOCKET skt, int startup)
{
    int events;
    if (startup) {
	events = (FD_CONNECT | FD_READ | FD_WRITE |
		  FD_OOB | FD_CLOSE | FD_ACCEPT);
    } else {
	events = 0;
    }
    if (p_WSAEventSelect(skt, netevent, events) == SOCKET_ERROR) {
	switch (p_WSAGetLastError()) {
	  case WSAENETDOWN:
	    return "Network is down";
	  default:
	    return "WSAEventSelect(): unknown error";
	}
    }
    return NULL;
}

int stdin_gotdata(struct handle *h, void *data, int len)
{
    if (len < 0) {
	/*
	 * Special case: report read error.
	 */
	char buf[4096];
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, -len, 0,
		      buf, lenof(buf), NULL);
	buf[lenof(buf)-1] = '\0';
	if (buf[strlen(buf)-1] == '\n')
	    buf[strlen(buf)-1] = '\0';
	fprintf(stderr, "Unable to read from standard input: %s\n", buf);
	cleanup_exit(0);
    }
    noise_ultralight(len);
    if (connopen && back->connected(backhandle)) {
	if (len > 0) {
	    return back->send(backhandle, data, len);
	} else {
	    back->special(backhandle, TS_EOF);
	    return 0;
	}
    } else
	return 0;
}

void stdouterr_sent(struct handle *h, int new_backlog)
{
    if (new_backlog < 0) {
	/*
	 * Special case: report write error.
	 */
	char buf[4096];
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, -new_backlog, 0,
		      buf, lenof(buf), NULL);
	buf[lenof(buf)-1] = '\0';
	if (buf[strlen(buf)-1] == '\n')
	    buf[strlen(buf)-1] = '\0';
	fprintf(stderr, "Unable to write to standard %s: %s\n",
		(h == stdout_handle ? "output" : "error"), buf);
	cleanup_exit(0);
    }
    if (connopen && back->connected(backhandle)) {
	back->unthrottle(backhandle, (handle_backlog(stdout_handle) +
				      handle_backlog(stderr_handle)));
    }
}

const int share_can_be_downstream = TRUE;
const int share_can_be_upstream = TRUE;

int main(int argc, char **argv)
{
    int sending;
    int portnumber = -1;
    SOCKET *sklist;
    int skcount, sksize;
    int exitcode;
    int errors;
    int got_host = FALSE;
    int use_subsystem = 0;
    unsigned long now, next, then;

    sklist = NULL;
    skcount = sksize = 0;
    /*
     * Initialise port and protocol to sensible defaults. (These
     * will be overridden by more or less anything.)
     */
    default_protocol = PROT_SSH;
    default_port = 22;

    flags = FLAG_STDERR;
    /*
     * Process the command line.
     */
    conf = conf_new();
    do_defaults(NULL, conf);
    loaded_session = FALSE;
    errors = 0;
    conf_set_int(conf, CONF_protocol, default_protocol);
    conf_set_int(conf, CONF_port, default_port);
    conf_set_int(conf, CONF_agentfwd, 0);
    conf_set_int(conf, CONF_x11_forward, 0);
    while (--argc) {
	char *p = *++argv;
	if (*p == '-') {
	    int ret = cmdline_process_param(p, (argc > 1 ? argv[1] : NULL),
					    1, conf);
	    if (ret == -2) {
		fprintf(stderr,
			"tortoisegitplink: option \"%s\" requires an argument\n", p);
		errors = 1;
	    } else if (ret == 2) {
		--argc, ++argv;
	    } else if (ret == 1) {
		continue;
	    } else if (!strcmp(p, "-batch")) {
			// ignore and do not print an error message
	    } else if (!strcmp(p, "-s")) {
		/* Save status to write to conf later. */
		use_subsystem = 1;
	    } else if (!strcmp(p, "-V") || !strcmp(p, "--version")) {
                version();
	    } else if (!strcmp(p, "--help")) {
                usage();
            } else if (!strcmp(p, "-pgpfp")) {
                pgp_fingerprints();
                exit(1);
	    } else {
		fprintf(stderr, "tortoisegitplink: unknown option \"%s\"\n", p);
		errors = 1;
	    }
	} else if (*p) {
	    if (!conf_launchable(conf) || !(got_host || loaded_session)) {
		char *q = p;
		/*
		 * If the hostname starts with "telnet:", set the
		 * protocol to Telnet and process the string as a
		 * Telnet URL.
		 */
		if (!strncmp(q, "telnet:", 7)) {
		    char c;

		    q += 7;
		    if (q[0] == '/' && q[1] == '/')
			q += 2;
		    conf_set_int(conf, CONF_protocol, PROT_TELNET);
		    p = q;
                    p += host_strcspn(p, ":/");
		    c = *p;
		    if (*p)
			*p++ = '\0';
		    if (c == ':')
			conf_set_int(conf, CONF_port, atoi(p));
		    else
			conf_set_int(conf, CONF_port, -1);
		    conf_set_str(conf, CONF_host, q);
		    got_host = TRUE;
		} else {
		    char *r, *user, *host;
		    /*
		     * Before we process the [user@]host string, we
		     * first check for the presence of a protocol
		     * prefix (a protocol name followed by ",").
		     */
		    r = strchr(p, ',');
		    if (r) {
			const Backend *b;
			*r = '\0';
			b = backend_from_name(p);
			if (b) {
			    default_protocol = b->protocol;
			    conf_set_int(conf, CONF_protocol,
					 default_protocol);
			    portnumber = b->default_port;
			}
			p = r + 1;
		    }

		    /*
		     * A nonzero length string followed by an @ is treated
		     * as a username. (We discount an _initial_ @.) The
		     * rest of the string (or the whole string if no @)
		     * is treated as a session name and/or hostname.
		     */
		    r = strrchr(p, '@');
		    if (r == p)
			p++, r = NULL; /* discount initial @ */
		    if (r) {
			*r++ = '\0';
			user = p, host = r;
		    } else {
			user = NULL, host = p;
		    }

		    /*
		     * Now attempt to load a saved session with the
		     * same name as the hostname.
		     */
		    {
			Conf *conf2 = conf_new();
			do_defaults(host, conf2);
			if (loaded_session || !conf_launchable(conf2)) {
			    /* No settings for this host; use defaults */
			    /* (or session was already loaded with -load) */
			    conf_set_str(conf, CONF_host, host);
			    conf_set_int(conf, CONF_port, default_port);
			    got_host = TRUE;
			} else {
			    conf_copy_into(conf, conf2);
			    loaded_session = TRUE;
			}
			conf_free(conf2);
		    }

		    if (user) {
			/* Patch in specified username. */
			conf_set_str(conf, CONF_username, user);
		    }

		}
	    } else {
		char *command;
		int cmdlen, cmdsize;
		cmdlen = cmdsize = 0;
		command = NULL;

		while (argc) {
		    while (*p) {
			if (cmdlen >= cmdsize) {
			    cmdsize = cmdlen + 512;
			    command = sresize(command, cmdsize, char);
			}
			command[cmdlen++]=*p++;
		    }
		    if (cmdlen >= cmdsize) {
			cmdsize = cmdlen + 512;
			command = sresize(command, cmdsize, char);
		    }
		    command[cmdlen++]=' '; /* always add trailing space */
		    if (--argc) p = *++argv;
		}
		if (cmdlen) command[--cmdlen]='\0';
				       /* change trailing blank to NUL */
		conf_set_str(conf, CONF_remote_cmd, command);
		conf_set_str(conf, CONF_remote_cmd2, "");
		conf_set_int(conf, CONF_nopty, TRUE);  /* command => no tty */

		break;		       /* done with cmdline */
	    }
	}
    }

    if (errors)
	return 1;

    if (!conf_launchable(conf) || !(got_host || loaded_session)) {
	usage();
    }

    /*
     * Muck about with the hostname in various ways.
     */
    {
	char *hostbuf = dupstr(conf_get_str(conf, CONF_host));
	char *host = hostbuf;
	char *p, *q;

	/*
	 * Trim leading whitespace.
	 */
	host += strspn(host, " \t");

	/*
	 * See if host is of the form user@host, and separate out
	 * the username if so.
	 */
	if (host[0] != '\0') {
	    char *atsign = strrchr(host, '@');
	    if (atsign) {
		*atsign = '\0';
		conf_set_str(conf, CONF_username, host);
		host = atsign + 1;
	    }
	}

        /*
         * Trim a colon suffix off the hostname if it's there. In
         * order to protect unbracketed IPv6 address literals
         * against this treatment, we do not do this if there's
         * _more_ than one colon.
         */
        {
            char *c = host_strchr(host, ':');
 
            if (c) {
                char *d = host_strchr(c+1, ':');
                if (!d)
                    *c = '\0';
            }
        }

	/*
	 * Remove any remaining whitespace.
	 */
	p = hostbuf;
	q = host;
	while (*q) {
	    if (*q != ' ' && *q != '\t')
		*p++ = *q;
	    q++;
	}
	*p = '\0';

	conf_set_str(conf, CONF_host, hostbuf);
	sfree(hostbuf);
    }

    /*
     * Perform command-line overrides on session configuration.
     */
    cmdline_run_saved(conf);

    /*
     * Apply subsystem status.
     */
    if (use_subsystem)
        conf_set_int(conf, CONF_ssh_subsys, TRUE);

    if (!*conf_get_str(conf, CONF_remote_cmd) &&
	!*conf_get_str(conf, CONF_remote_cmd2) &&
	!*conf_get_str(conf, CONF_ssh_nc_host))
	flags |= FLAG_INTERACTIVE;

    /*
     * Select protocol. This is farmed out into a table in a
     * separate file to enable an ssh-free variant.
     */
    back = backend_from_proto(conf_get_int(conf, CONF_protocol));
    if (back == NULL) {
	fprintf(stderr,
		"Internal fault: Unsupported protocol found\n");
	return 1;
    }

    /*
     * Select port.
     */
    if (portnumber != -1)
	conf_set_int(conf, CONF_port, portnumber);

    sk_init();
    if (p_WSAEventSelect == NULL) {
	fprintf(stderr, "Plink requires WinSock 2\n");
	return 1;
    }

    logctx = log_init(NULL, conf);
    console_provide_logctx(logctx);

    /*
     * Start up the connection.
     */
    netevent = CreateEvent(NULL, FALSE, FALSE, NULL);
    {
	const char *error;
	char *realhost;
	/* nodelay is only useful if stdin is a character device (console) */
	int nodelay = conf_get_int(conf, CONF_tcp_nodelay) &&
	    (GetFileType(GetStdHandle(STD_INPUT_HANDLE)) == FILE_TYPE_CHAR);

	error = back->init(NULL, &backhandle, conf,
			   conf_get_str(conf, CONF_host),
			   conf_get_int(conf, CONF_port),
			   &realhost, nodelay,
			   conf_get_int(conf, CONF_tcp_keepalives));
	if (error) {
	    fprintf(stderr, "Unable to open connection:\n%s", error);
	    return 1;
	}
	back->provide_logctx(backhandle, logctx);
	sfree(realhost);
    }
    connopen = 1;

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

    main_thread_id = GetCurrentThreadId();

    sending = FALSE;

    now = GETTICKCOUNT();

    while (1) {
	int nhandles;
	HANDLE *handles;	
	int n;
	DWORD ticks;

	if (!sending && back->sendok(backhandle)) {
	    stdin_handle = handle_input_new(inhandle, stdin_gotdata, NULL,
					    0);
	    sending = TRUE;
	}

        if (toplevel_callback_pending()) {
            ticks = 0;
            next = now;
        } else if (run_timers(now, &next)) {
	    then = now;
	    now = GETTICKCOUNT();
	    if (now - then > next - then)
		ticks = 0;
	    else
		ticks = next - now;
	} else {
	    ticks = INFINITE;
            /* no need to initialise next here because we can never
             * get WAIT_TIMEOUT */
	}

	handles = handle_get_events(&nhandles);
	handles = sresize(handles, nhandles+1, HANDLE);
	handles[nhandles] = netevent;
	n = MsgWaitForMultipleObjects(nhandles+1, handles, FALSE, ticks,
				      QS_POSTMESSAGE);
	if ((unsigned)(n - WAIT_OBJECT_0) < (unsigned)nhandles) {
	    handle_got_event(handles[n - WAIT_OBJECT_0]);
	} else if (n == WAIT_OBJECT_0 + nhandles) {
	    WSANETWORKEVENTS things;
	    SOCKET socket;
	    extern SOCKET first_socket(int *), next_socket(int *);
	    extern int select_result(WPARAM, LPARAM);
	    int i, socketstate;

	    /*
	     * We must not call select_result() for any socket
	     * until we have finished enumerating within the tree.
	     * This is because select_result() may close the socket
	     * and modify the tree.
	     */
	    /* Count the active sockets. */
	    i = 0;
	    for (socket = first_socket(&socketstate);
		 socket != INVALID_SOCKET;
		 socket = next_socket(&socketstate)) i++;

	    /* Expand the buffer if necessary. */
	    if (i > sksize) {
		sksize = i + 16;
		sklist = sresize(sklist, sksize, SOCKET);
	    }

	    /* Retrieve the sockets into sklist. */
	    skcount = 0;
	    for (socket = first_socket(&socketstate);
		 socket != INVALID_SOCKET;
		 socket = next_socket(&socketstate)) {
		sklist[skcount++] = socket;
	    }

	    /* Now we're done enumerating; go through the list. */
	    for (i = 0; i < skcount; i++) {
		WPARAM wp;
		socket = sklist[i];
		wp = (WPARAM) socket;
		if (!p_WSAEnumNetworkEvents(socket, NULL, &things)) {
                    static const struct { int bit, mask; } eventtypes[] = {
                        {FD_CONNECT_BIT, FD_CONNECT},
                        {FD_READ_BIT, FD_READ},
                        {FD_CLOSE_BIT, FD_CLOSE},
                        {FD_OOB_BIT, FD_OOB},
                        {FD_WRITE_BIT, FD_WRITE},
                        {FD_ACCEPT_BIT, FD_ACCEPT},
                    };
                    int e;

		    noise_ultralight(socket);
		    noise_ultralight(things.lNetworkEvents);

                    for (e = 0; e < lenof(eventtypes); e++)
                        if (things.lNetworkEvents & eventtypes[e].mask) {
                            LPARAM lp;
                            int err = things.iErrorCode[eventtypes[e].bit];
                            lp = WSAMAKESELECTREPLY(eventtypes[e].mask, err);
                            connopen &= select_result(wp, lp);
                        }
		}
	    }
	} else if (n == WAIT_OBJECT_0 + nhandles + 1) {
	    MSG msg;
	    while (PeekMessage(&msg, INVALID_HANDLE_VALUE,
			       WM_AGENT_CALLBACK, WM_AGENT_CALLBACK,
			       PM_REMOVE)) {
		struct agent_callback *c = (struct agent_callback *)msg.lParam;
		c->callback(c->callback_ctx, c->data, c->len);
		sfree(c);
	    }
	}

        run_toplevel_callbacks();

	if (n == WAIT_TIMEOUT) {
	    now = next;
	} else {
	    now = GETTICKCOUNT();
	}

	sfree(handles);

	if (sending)
	    handle_unthrottle(stdin_handle, back->sendbuffer(backhandle));

	if ((!connopen || !back->connected(backhandle)) &&
	    handle_backlog(stdout_handle) + handle_backlog(stderr_handle) == 0)
	    break;		       /* we closed the connection */
    }
    exitcode = back->exitcode(backhandle);
    if (exitcode < 0) {
	fprintf(stderr, "Remote process exit code unavailable\n");
	exitcode = 1;		       /* this is an error condition */
    }
    cleanup_exit(exitcode);
    return 0;			       /* placate compiler warning */
}

int WinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow)
{
	main(__argc,__argv);
}
