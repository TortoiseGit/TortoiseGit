/*
 * Definition of the array of X11 authorisation method names.
 */

#include "putty.h"

const char *const x11_authnames[X11_NAUTHS] = {
    "", "MIT-MAGIC-COOKIE-1", "XDM-AUTHORIZATION-1"
};
