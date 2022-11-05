/*
 * Implementation of platform_get_x_display for Windows, common to all
 * tools.
 */

#include "putty.h"
#include "ssh.h"

char *platform_get_x_display(void)
{
    /* We may as well check for DISPLAY in case it's useful. */
    return dupstr(getenv("DISPLAY"));
}
