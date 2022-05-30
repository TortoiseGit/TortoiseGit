/*
 * Validate a manual host key specification (either entered in the
 * GUI, or via -hostkey). If valid, we return true, and update 'key'
 * to contain a canonicalised version of the key string in 'key'
 * (which is guaranteed to take up at most as much space as the
 * original version), suitable for putting into the Conf. If not
 * valid, we return false.
 */

#include <string.h>
#include <ctype.h>

#include "putty.h"
#include "misc.h"

#define BASE64_CHARS_NOEQ \
    "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ+/"
#define BASE64_CHARS_ALL BASE64_CHARS_NOEQ "="

bool validate_manual_hostkey(char *key)
{
    char *p, *q, *r, *s;

    /*
     * Step through the string word by word, looking for a word that's
     * in one of the formats we like.
     */
    p = key;
    while ((p += strspn(p, " \t"))[0]) {
        q = p;
        p += strcspn(p, " \t");
        if (*p) *p++ = '\0';

        /*
         * Now q is our word.
         */

        if (strstartswith(q, "SHA256:")) {
            /* Test for a valid SHA256 key fingerprint. */
            r = q + 7;
            if (strspn(r, BASE64_CHARS_NOEQ) == 43) {
                memmove(key, q, 50); /* 7-char prefix + 43-char base64 */
                key[50] = '\0';
                return true;
            }
        }

        r = q;
        if (strstartswith(r, "MD5:"))
            r += 4;
        if (strlen(r) == 16*3 - 1 &&
            r[strspn(r, "0123456789abcdefABCDEF:")] == 0) {
            /*
             * Test for a valid MD5 key fingerprint. Check the colons
             * are in the right places, and if so, return the same
             * fingerprint canonicalised into lowercase.
             */
            int i;
            for (i = 0; i < 16; i++)
                if (r[3*i] == ':' || r[3*i+1] == ':')
                    goto not_fingerprint; /* sorry */
            for (i = 0; i < 15; i++)
                if (r[3*i+2] != ':')
                    goto not_fingerprint; /* sorry */
            for (i = 0; i < 16*3 - 1; i++)
                key[i] = tolower(r[i]);
            key[16*3 - 1] = '\0';
            return true;
        }
      not_fingerprint:;

        /*
         * Before we check for a public-key blob, trim newlines out of
         * the middle of the word, in case someone's managed to paste
         * in a public-key blob _with_ them.
         */
        for (r = s = q; *r; r++)
            if (*r != '\n' && *r != '\r')
                *s++ = *r;
        *s = '\0';

        if (strlen(q) % 4 == 0 && strlen(q) > 2*4 &&
            q[strspn(q, BASE64_CHARS_ALL)] == 0) {
            /*
             * Might be a base64-encoded SSH-2 public key blob. Check
             * that it starts with a sensible algorithm string. No
             * canonicalisation is necessary for this string type.
             *
             * The algorithm string must be at most 64 characters long
             * (RFC 4251 section 6).
             */
            unsigned char decoded[6];
            unsigned alglen;
            int minlen;
            int len = 0;

            len += base64_decode_atom(q, decoded+len);
            if (len < 3)
                goto not_ssh2_blob;    /* sorry */
            len += base64_decode_atom(q+4, decoded+len);
            if (len < 4)
                goto not_ssh2_blob;    /* sorry */

            alglen = GET_32BIT_MSB_FIRST(decoded);
            if (alglen > 64)
                goto not_ssh2_blob;    /* sorry */

            minlen = ((alglen + 4) + 2) / 3;
            if (strlen(q) < minlen)
                goto not_ssh2_blob;    /* sorry */

            size_t base64_len = strspn(q, BASE64_CHARS_ALL);
            memmove(key, q, base64_len);
            key[base64_len] = '\0';
            return true;
        }
      not_ssh2_blob:;
    }

    return false;
}
