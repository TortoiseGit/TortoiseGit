/*
 * Construct an X11 greeting packet, including making up the right
 * authorisation data.
 */

#include "putty.h"
#include "ssh.h"

void *x11_make_greeting(int endian, int protomajor, int protominor,
                        int auth_proto, const void *auth_data, int auth_len,
                        const char *peer_addr, int peer_port,
                        int *outlen)
{
    unsigned char *greeting;
    unsigned char realauthdata[64];
    const char *authname;
    const unsigned char *authdata;
    int authnamelen, authnamelen_pad;
    int authdatalen, authdatalen_pad;
    int greeting_len;

    authname = x11_authnames[auth_proto];
    authnamelen = strlen(authname);
    authnamelen_pad = (authnamelen + 3) & ~3;

    if (auth_proto == X11_MIT) {
        authdata = auth_data;
        authdatalen = auth_len;
    } else if (auth_proto == X11_XDM && auth_len == 16) {
        time_t t;
        unsigned long peer_ip = 0;

        x11_parse_ip(peer_addr, &peer_ip);

        authdata = realauthdata;
        authdatalen = 24;
        memset(realauthdata, 0, authdatalen);
        memcpy(realauthdata, auth_data, 8);
        PUT_32BIT_MSB_FIRST(realauthdata+8, peer_ip);
        PUT_16BIT_MSB_FIRST(realauthdata+12, peer_port);
        t = time(NULL);
        PUT_32BIT_MSB_FIRST(realauthdata+14, t);

        des_encrypt_xdmauth((char *)auth_data + 9, realauthdata, authdatalen);
    } else {
        authdata = realauthdata;
        authdatalen = 0;
    }

    authdatalen_pad = (authdatalen + 3) & ~3;
    greeting_len = 12 + authnamelen_pad + authdatalen_pad;

    greeting = snewn(greeting_len, unsigned char);
    memset(greeting, 0, greeting_len);
    greeting[0] = endian;
    PUT_16BIT_X11(endian, greeting+2, protomajor);
    PUT_16BIT_X11(endian, greeting+4, protominor);
    PUT_16BIT_X11(endian, greeting+6, authnamelen);
    PUT_16BIT_X11(endian, greeting+8, authdatalen);
    memcpy(greeting+12, authname, authnamelen);
    memcpy(greeting+12+authnamelen_pad, authdata, authdatalen);

    smemclr(realauthdata, sizeof(realauthdata));

    *outlen = greeting_len;
    return greeting;
}
