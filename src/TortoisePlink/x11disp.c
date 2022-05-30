/*
 * Functions to manage an X11Display structure, by creating one from
 * an ordinary display name string, and freeing one.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#include "putty.h"
#include "ssh.h"
#include "ssh/channel.h"
#include "tree234.h"

struct X11Display *x11_setup_display(const char *display, Conf *conf,
                                     char **error_msg)
{
    struct X11Display *disp = snew(struct X11Display);
    char *localcopy;

    *error_msg = NULL;

    if (!display || !*display) {
        localcopy = platform_get_x_display();
        if (!localcopy || !*localcopy) {
            sfree(localcopy);
            localcopy = dupstr(":0");  /* plausible default for any platform */
        }
    } else
        localcopy = dupstr(display);

    /*
     * Parse the display name.
     *
     * We expect this to have one of the following forms:
     *
     *  - the standard X format which looks like
     *    [ [ protocol '/' ] host ] ':' displaynumber [ '.' screennumber ]
     *    (X11 also permits a double colon to indicate DECnet, but
     *    that's not our problem, thankfully!)
     *
     *  - only seen in the wild on MacOS (so far): a pathname to a
     *    Unix-domain socket, which will typically and confusingly
     *    end in ":0", and which I'm currently distinguishing from
     *    the standard scheme by noting that it starts with '/'.
     */
    if (localcopy[0] == '/') {
        disp->unixsocketpath = localcopy;
        disp->unixdomain = true;
        disp->hostname = NULL;
        disp->displaynum = -1;
        disp->screennum = 0;
        disp->addr = NULL;
    } else {
        char *colon, *dot, *slash;
        char *protocol, *hostname;

        colon = host_strrchr(localcopy, ':');
        if (!colon) {
            *error_msg = dupprintf("display name '%s' has no ':number'"
                                   " suffix", localcopy);

            sfree(disp);
            sfree(localcopy);
            return NULL;
        }

        *colon++ = '\0';
        dot = strchr(colon, '.');
        if (dot)
            *dot++ = '\0';

        disp->displaynum = atoi(colon);
        if (dot)
            disp->screennum = atoi(dot);
        else
            disp->screennum = 0;

        protocol = NULL;
        hostname = localcopy;
        if (colon > localcopy) {
            slash = strchr(localcopy, '/');
            if (slash) {
                *slash++ = '\0';
                protocol = localcopy;
                hostname = slash;
            }
        }

        disp->hostname = *hostname ? dupstr(hostname) : NULL;

        if (protocol)
            disp->unixdomain = (!strcmp(protocol, "local") ||
                                !strcmp(protocol, "unix"));
        else if (!*hostname || !strcmp(hostname, "unix"))
            disp->unixdomain = platform_uses_x11_unix_by_default;
        else
            disp->unixdomain = false;

        if (!disp->hostname && !disp->unixdomain)
            disp->hostname = dupstr("localhost");

        disp->unixsocketpath = NULL;
        disp->addr = NULL;

        sfree(localcopy);
    }

    /*
     * Look up the display hostname, if we need to.
     */
    if (!disp->unixdomain) {
        const char *err;

        disp->port = 6000 + disp->displaynum;
        disp->addr = name_lookup(disp->hostname, disp->port,
                                 &disp->realhost, conf, ADDRTYPE_UNSPEC,
                                 NULL, NULL);

        if ((err = sk_addr_error(disp->addr)) != NULL) {
            *error_msg = dupprintf("unable to resolve host name '%s' in "
                                   "display name", disp->hostname);

            sk_addr_free(disp->addr);
            sfree(disp->hostname);
            sfree(disp->unixsocketpath);
            sfree(disp);
            return NULL;
        }
    }

    /*
     * Try upgrading an IP-style localhost display to a Unix-socket
     * display (as the standard X connection libraries do).
     */
    if (!disp->unixdomain && sk_address_is_local(disp->addr)) {
        SockAddr *ux = platform_get_x11_unix_address(NULL, disp->displaynum);
        const char *err = sk_addr_error(ux);
        if (!err) {
            /* Create trial connection to see if there is a useful Unix-domain
             * socket */
            Socket *s = sk_new(sk_addr_dup(ux), 0, false, false,
                               false, false, nullplug);
            err = sk_socket_error(s);
            sk_close(s);
        }
        if (err) {
            sk_addr_free(ux);
        } else {
            sk_addr_free(disp->addr);
            disp->unixdomain = true;
            disp->addr = ux;
            /* Fill in the rest in a moment */
        }
    }

    if (disp->unixdomain) {
        if (!disp->addr)
            disp->addr = platform_get_x11_unix_address(disp->unixsocketpath,
                                                       disp->displaynum);
        if (disp->unixsocketpath)
            disp->realhost = dupstr(disp->unixsocketpath);
        else
            disp->realhost = dupprintf("unix:%d", disp->displaynum);
        disp->port = 0;
    }

    /*
     * Fetch the local authorisation details.
     */
    disp->localauthproto = X11_NO_AUTH;
    disp->localauthdata = NULL;
    disp->localauthdatalen = 0;
    platform_get_x11_auth(disp, conf);

    return disp;
}

void x11_free_display(struct X11Display *disp)
{
    sfree(disp->hostname);
    sfree(disp->unixsocketpath);
    if (disp->localauthdata)
        smemclr(disp->localauthdata, disp->localauthdatalen);
    sfree(disp->localauthdata);
    sk_addr_free(disp->addr);
    sfree(disp);
}
