/*
 * Functions to handle .Xauthority files.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "putty.h"
#include "ssh.h"

ptrlen BinarySource_get_string_xauth(BinarySource *src)
{
    size_t len = get_uint16(src);
    return get_data(src, len);
}
#define get_string_xauth(src) \
    BinarySource_get_string_xauth(BinarySource_UPCAST(src))

void BinarySink_put_stringpl_xauth(BinarySink *bs, ptrlen pl)
{
    assert((pl.len >> 16) == 0);
    put_uint16(bs, pl.len);
    put_datapl(bs, pl);
}
#define put_stringpl_xauth(bs, ptrlen) \
    BinarySink_put_stringpl_xauth(BinarySink_UPCAST(bs),ptrlen)

void x11_get_auth_from_authfile(struct X11Display *disp,
                                const char *authfilename)
{
    FILE *authfp;
    char *buf;
    int size;
    BinarySource src[1];
    int family, protocol;
    ptrlen addr, protoname, data;
    char *displaynum_string;
    int displaynum;
    bool ideal_match = false;
    char *ourhostname;

    /* A maximally sized (wildly implausible) .Xauthority record
     * consists of a 16-bit integer to start with, then four strings,
     * each of which has a 16-bit length field followed by that many
     * bytes of data (i.e. up to 0xFFFF bytes). */
    const size_t MAX_RECORD_SIZE = 2 + 4 * (2+0xFFFF);

    /* We'll want a buffer of twice that size (see below). */
    const size_t BUF_SIZE = 2 * MAX_RECORD_SIZE;

    /*
     * Normally we should look for precisely the details specified in
     * `disp'. However, there's an oddity when the display is local:
     * displays like "localhost:0" usually have their details stored
     * in a Unix-domain-socket record (even if there isn't actually a
     * real Unix-domain socket available, as with OpenSSH's proxy X11
     * server).
     *
     * This is apparently a fudge to get round the meaninglessness of
     * "localhost" in a shared-home-directory context -- xauth entries
     * for Unix-domain sockets already disambiguate this by storing
     * the *local* hostname in the conveniently-blank hostname field,
     * but IP "localhost" records couldn't do this. So, typically, an
     * IP "localhost" entry in the auth database isn't present and if
     * it were it would be ignored.
     *
     * However, we don't entirely trust that (say) Windows X servers
     * won't rely on a straight "localhost" entry, bad idea though
     * that is; so if we can't find a Unix-domain-socket entry we'll
     * fall back to an IP-based entry if we can find one.
     */
    bool localhost = !disp->unixdomain && sk_address_is_local(disp->addr);

    authfp = fopen(authfilename, "rb");
    if (!authfp)
        return;

    ourhostname = get_hostname();

    /*
     * Allocate enough space to hold two maximally sized records, so
     * that a full record can start anywhere in the first half. That
     * way we avoid the accidentally-quadratic algorithm that would
     * arise if we moved everything to the front of the buffer after
     * consuming each record; instead, we only move everything to the
     * front after our current position gets past the half-way mark.
     * Before then, there's no need to move anyway; so this guarantees
     * linear time, in that every byte written into this buffer moves
     * at most once (because every move is from the second half of the
     * buffer to the first half).
     */
    buf = snewn(BUF_SIZE, char);
    size = fread(buf, 1, BUF_SIZE, authfp);
    BinarySource_BARE_INIT(src, buf, size);

    while (!ideal_match) {
        bool match = false;

        if (src->pos >= MAX_RECORD_SIZE) {
            size -= src->pos;
            memcpy(buf, buf + src->pos, size);
            size += fread(buf + size, 1, BUF_SIZE - size, authfp);
            BinarySource_BARE_INIT(src, buf, size);
        }

        family = get_uint16(src);
        addr = get_string_xauth(src);
        displaynum_string = mkstr(get_string_xauth(src));
        displaynum = displaynum_string[0] ? atoi(displaynum_string) : -1;
        sfree(displaynum_string);
        protoname = get_string_xauth(src);
        data = get_string_xauth(src);
        if (get_err(src))
            break;

        /*
         * Now we have a full X authority record in memory. See
         * whether it matches the display we're trying to
         * authenticate to.
         *
         * The details we've just read should be interpreted as
         * follows:
         *
         *  - 'family' is the network address family used to
         *    connect to the display. 0 means IPv4; 6 means IPv6;
         *    256 means Unix-domain sockets.
         *
         *  - 'addr' is the network address itself. For IPv4 and
         *    IPv6, this is a string of binary data of the
         *    appropriate length (respectively 4 and 16 bytes)
         *    representing the address in big-endian format, e.g.
         *    7F 00 00 01 means IPv4 localhost. For Unix-domain
         *    sockets, this is the host name of the machine on
         *    which the Unix-domain display resides (so that an
         *    .Xauthority file on a shared file system can contain
         *    authority entries for Unix-domain displays on
         *    several machines without them clashing).
         *
         *  - 'displaynum' is the display number. An empty display
         *    number is a wildcard for any display number.
         *
         *  - 'protoname' is the authorisation protocol, encoded as
         *    its canonical string name (i.e. "MIT-MAGIC-COOKIE-1",
         *    "XDM-AUTHORIZATION-1" or something we don't recognise).
         *
         *  - 'data' is the actual authorisation data, stored in
         *    binary form.
         */

        if (disp->displaynum < 0 ||
            (displaynum >= 0 && disp->displaynum != displaynum))
            continue;                  /* not the one */

        for (protocol = 1; protocol < lenof(x11_authnames); protocol++)
            if (ptrlen_eq_string(protoname, x11_authnames[protocol]))
                break;
        if (protocol == lenof(x11_authnames))
            continue;  /* don't recognise this protocol, look for another */

        switch (family) {
          case 0:   /* IPv4 */
            if (!disp->unixdomain &&
                sk_addrtype(disp->addr) == ADDRTYPE_IPV4) {
                char buf[4];
                sk_addrcopy(disp->addr, buf);
                if (addr.len == 4 && !memcmp(addr.ptr, buf, 4)) {
                    match = true;
                    /* If this is a "localhost" entry, note it down
                     * but carry on looking for a Unix-domain entry. */
                    ideal_match = !localhost;
                }
            }
            break;
          case 6:   /* IPv6 */
            if (!disp->unixdomain &&
                sk_addrtype(disp->addr) == ADDRTYPE_IPV6) {
                char buf[16];
                sk_addrcopy(disp->addr, buf);
                if (addr.len == 16 && !memcmp(addr.ptr, buf, 16)) {
                    match = true;
                    ideal_match = !localhost;
                }
            }
            break;
          case 256: /* Unix-domain / localhost */
            if ((disp->unixdomain || localhost)
                && ourhostname && ptrlen_eq_string(addr, ourhostname)) {
                /* A matching Unix-domain socket is always the best
                 * match. */
                match = true;
                ideal_match = true;
            }
            break;
        }

        if (match) {
            /* Current best guess -- may be overridden if !ideal_match */
            disp->localauthproto = protocol;
            sfree(disp->localauthdata); /* free previous guess, if any */
            disp->localauthdata = snewn(data.len, unsigned char);
            memcpy(disp->localauthdata, data.ptr, data.len);
            disp->localauthdatalen = data.len;
        }
    }

    fclose(authfp);
    smemclr(buf, 2 * MAX_RECORD_SIZE);
    sfree(buf);
    sfree(ourhostname);
}

void x11_format_auth_for_authfile(
    BinarySink *bs, SockAddr *addr, int display_no,
    ptrlen authproto, ptrlen authdata)
{
    if (sk_address_is_special_local(addr)) {
        char *ourhostname = get_hostname();
        put_uint16(bs, 256); /* indicates Unix-domain socket */
        put_stringpl_xauth(bs, ptrlen_from_asciz(ourhostname));
        sfree(ourhostname);
    } else if (sk_addrtype(addr) == ADDRTYPE_IPV4) {
        char ipv4buf[4];
        sk_addrcopy(addr, ipv4buf);
        put_uint16(bs, 0); /* indicates IPv4 */
        put_stringpl_xauth(bs, make_ptrlen(ipv4buf, 4));
    } else if (sk_addrtype(addr) == ADDRTYPE_IPV6) {
        char ipv6buf[16];
        sk_addrcopy(addr, ipv6buf);
        put_uint16(bs, 6); /* indicates IPv6 */
        put_stringpl_xauth(bs, make_ptrlen(ipv6buf, 16));
    } else {
        unreachable("Bad address type in x11_format_auth_for_authfile");
    }

    {
        char *numberbuf = dupprintf("%d", display_no);
        put_stringpl_xauth(bs, ptrlen_from_asciz(numberbuf));
        sfree(numberbuf);
    }

    put_stringpl_xauth(bs, authproto);
    put_stringpl_xauth(bs, authdata);
}
