/*
 * Utility function to convert a textual representation of an X11
 * auth hex cookie into binary auth data.
 */

#include "putty.h"

void *x11_dehexify(ptrlen hexpl, int *outlen)
{
    int len, i;
    unsigned char *ret;

    len = hexpl.len / 2;
    ret = snewn(len, unsigned char);

    for (i = 0; i < len; i++) {
        char bytestr[3];
        unsigned val = 0;
        bytestr[0] = ((const char *)hexpl.ptr)[2*i];
        bytestr[1] = ((const char *)hexpl.ptr)[2*i+1];
        bytestr[2] = '\0';
        sscanf(bytestr, "%x", &val);
        ret[i] = val;
    }

    *outlen = len;
    return ret;
}
