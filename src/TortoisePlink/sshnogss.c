#include "putty.h"
#ifndef NO_GSSAPI

/* For platforms not supporting GSSAPI */

void ssh_gss_init(void)
{
}

#endif /* NO_GSSAPI */
