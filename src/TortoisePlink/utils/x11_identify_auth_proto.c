/*
 * Utility function to convert a textual representation of an X11
 * auth protocol name into our integer protocol ids.
 */

#include "putty.h"

int x11_identify_auth_proto(ptrlen protoname)
{
    int protocol;

    for (protocol = 1; protocol < lenof(x11_authnames); protocol++)
        if (ptrlen_eq_string(protoname, x11_authnames[protocol]))
            return protocol;
    return -1;
}
