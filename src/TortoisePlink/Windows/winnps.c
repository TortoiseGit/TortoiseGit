/*
 * Windows support module which deals with being a named-pipe server.
 */

#include <stdio.h>
#include <assert.h>

#define DEFINE_PLUG_METHOD_MACROS
#include "tree234.h"
#include "putty.h"
#include "network.h"
#include "proxy.h"
#include "ssh.h"

#if !defined NO_SECURITY

#include "winsecur.h"

Socket make_handle_socket(HANDLE send_H, HANDLE recv_H, Plug plug,
                          int overlapped);

typedef struct Socket_named_pipe_server_tag *Named_Pipe_Server_Socket;
struct Socket_named_pipe_server_tag {
    const struct socket_function_table *fn;
    /* the above variable absolutely *must* be the first in this structure */

    /* Parameters for (repeated) creation of named pipe objects */
    PSECURITY_DESCRIPTOR psd;
    PACL acl;
    char *pipename;

    /* The current named pipe object + attempt to connect to it */
    HANDLE pipehandle;
    OVERLAPPED connect_ovl;

    /* PuTTY Socket machinery */
    Plug plug;
    char *error;
};

static Plug sk_namedpipeserver_plug(Socket s, Plug p)
{
    Named_Pipe_Server_Socket ps = (Named_Pipe_Server_Socket) s;
    Plug ret = ps->plug;
    if (p)
	ps->plug = p;
    return ret;
}

static void sk_namedpipeserver_close(Socket s)
{
    Named_Pipe_Server_Socket ps = (Named_Pipe_Server_Socket) s;

    CloseHandle(ps->pipehandle);
    CloseHandle(ps->connect_ovl.hEvent);
    sfree(ps->error);
    sfree(ps->pipename);
    if (ps->acl)
        LocalFree(ps->acl);
    if (ps->psd)
        LocalFree(ps->psd);
    sfree(ps);
}

static const char *sk_namedpipeserver_socket_error(Socket s)
{
    Named_Pipe_Server_Socket ps = (Named_Pipe_Server_Socket) s;
    return ps->error;
}

static int create_named_pipe(Named_Pipe_Server_Socket ps, int first_instance)
{
    SECURITY_ATTRIBUTES sa;

    memset(&sa, 0, sizeof(sa));
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = ps->psd;
    sa.bInheritHandle = FALSE;

    ps->pipehandle = CreateNamedPipe
        (/* lpName */
         ps->pipename,

         /* dwOpenMode */
         PIPE_ACCESS_DUPLEX |
         FILE_FLAG_OVERLAPPED |
         (first_instance ? FILE_FLAG_FIRST_PIPE_INSTANCE : 0),

         /* dwPipeMode */
         PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT
#ifdef PIPE_REJECT_REMOTE_CLIENTS
         | PIPE_REJECT_REMOTE_CLIENTS
#endif
         ,

         /* nMaxInstances */
         PIPE_UNLIMITED_INSTANCES,

         /* nOutBufferSize, nInBufferSize */
         4096, 4096,     /* FIXME: think harder about buffer sizes? */

         /* nDefaultTimeOut */
         0 /* default timeout */,

         /* lpSecurityAttributes */
         &sa);

    return ps->pipehandle != INVALID_HANDLE_VALUE;
}

static Socket named_pipe_accept(accept_ctx_t ctx, Plug plug)
{
    HANDLE conn = (HANDLE)ctx.p;

    return make_handle_socket(conn, conn, plug, TRUE);
}

/*
 * Dummy SockAddr type which just holds a named pipe address. Only
 * used for calling plug_log from named_pipe_accept_loop() here.
 */
SockAddr sk_namedpipe_addr(const char *pipename);

static void named_pipe_accept_loop(Named_Pipe_Server_Socket ps,
                                   int got_one_already)
{
    while (1) {
        int error;
        char *errmsg;

        if (got_one_already) {
            /* If we were called with a connection already waiting,
             * skip this step. */
            got_one_already = FALSE;
            error = 0;
        } else {
            /*
             * Call ConnectNamedPipe, which might succeed or might
             * tell us that an overlapped operation is in progress and
             * we should wait for our event object.
             */
            if (ConnectNamedPipe(ps->pipehandle, &ps->connect_ovl))
                error = 0;
            else
                error = GetLastError();

            if (error == ERROR_IO_PENDING)
                return;
        }

        if (error == 0 || error == ERROR_PIPE_CONNECTED) {
            /*
             * We've successfully retrieved an incoming connection, so
             * ps->pipehandle now refers to that connection. So
             * convert that handle into a separate connection-type
             * Socket, and create a fresh one to be the new listening
             * pipe.
             */
            HANDLE conn = ps->pipehandle;
            accept_ctx_t actx;

            actx.p = (void *)conn;
            if (plug_accepting(ps->plug, named_pipe_accept, actx)) {
                /*
                 * If the plug didn't want the connection, might as
                 * well close this handle.
                 */
                CloseHandle(conn);
            }

            if (!create_named_pipe(ps, FALSE)) {
                error = GetLastError();
            } else {
                /*
                 * Go round again to see if more connections can be
                 * got, or to begin waiting on the event object.
                 */
                continue;
            }
        }

        errmsg = dupprintf("Error while listening to named pipe: %s",
                           win_strerror(error));
        plug_log(ps->plug, 1, sk_namedpipe_addr(ps->pipename), 0,
                 errmsg, error);
        sfree(errmsg);
        break;
    }
}

static void named_pipe_connect_callback(void *vps)
{
    Named_Pipe_Server_Socket ps = (Named_Pipe_Server_Socket)vps;
    named_pipe_accept_loop(ps, TRUE);
}

Socket new_named_pipe_listener(const char *pipename, Plug plug)
{
    /*
     * This socket type is only used for listening, so it should never
     * be asked to write or flush or set_frozen.
     */
    static const struct socket_function_table socket_fn_table = {
	sk_namedpipeserver_plug,
	sk_namedpipeserver_close,
	NULL /* write */,
	NULL /* write_oob */,
        NULL /* write_eof */,
        NULL /* flush */,
        NULL /* set_frozen */,
	sk_namedpipeserver_socket_error
    };

    Named_Pipe_Server_Socket ret;

    ret = snew(struct Socket_named_pipe_server_tag);
    ret->fn = &socket_fn_table;
    ret->plug = plug;
    ret->error = NULL;
    ret->psd = NULL;
    ret->pipename = dupstr(pipename);
    ret->acl = NULL;

    assert(strncmp(pipename, "\\\\.\\pipe\\", 9) == 0);
    assert(strchr(pipename + 9, '\\') == NULL);

    if (!make_private_security_descriptor(GENERIC_READ | GENERIC_WRITE,
                                          &ret->psd, &ret->acl, &ret->error)) {
        goto cleanup;
    }

    if (!create_named_pipe(ret, TRUE)) {
        ret->error = dupprintf("unable to create named pipe '%s': %s",
                               pipename, win_strerror(GetLastError()));
        goto cleanup;
    }

    memset(&ret->connect_ovl, 0, sizeof(ret->connect_ovl));
    ret->connect_ovl.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    handle_add_foreign_event(ret->connect_ovl.hEvent,
                             named_pipe_connect_callback, ret);
    named_pipe_accept_loop(ret, FALSE);

  cleanup:
    return (Socket) ret;
}

#endif /* !defined NO_SECURITY */
