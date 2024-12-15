/*
 * x11.c: fetch local auth data for X forwarding.
 */

#include <ctype.h>
#include <assert.h>
#include <stdlib.h>

#include "putty.h"
#include "ssh.h"

void platform_get_x11_auth(struct X11Display *disp, Conf *conf)
{
    Filename *xauthfn = conf_get_filename(conf, CONF_xauthfile);
    if (!filename_is_null(xauthfn))
        x11_get_auth_from_authfile(disp, xauthfn);
}

const bool platform_uses_x11_unix_by_default = false;
