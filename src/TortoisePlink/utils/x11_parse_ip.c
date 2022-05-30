/*
 * Try to make sense of a string as an IPv4 address, for
 * XDM-AUTHORIZATION-1 purposes.
 */

#include <stdio.h>

#include "putty.h"

bool x11_parse_ip(const char *addr_string, unsigned long *ip)
{
    int i[4];
    if (addr_string &&
        4 == sscanf(addr_string, "%d.%d.%d.%d", i+0, i+1, i+2, i+3)) {
        *ip = (i[0] << 24) | (i[1] << 16) | (i[2] << 8) | i[3];
        return true;
    } else {
        return false;
    }
}
