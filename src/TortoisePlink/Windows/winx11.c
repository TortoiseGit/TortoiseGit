/*
 * winx11.c: fetch local auth data for X forwarding.
 */

#include <ctype.h>
#include <assert.h>
#include <stdlib.h>

#include "putty.h"
#include "ssh.h"

void platform_get_x11_auth(struct X11Display *disp, const Config *cfg)
{
    if (cfg->xauthfile.path[0])
	x11_get_auth_from_authfile(disp, cfg->xauthfile.path);
}

const int platform_uses_x11_unix_by_default = FALSE;
