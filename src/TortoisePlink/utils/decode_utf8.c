/*
 * Decode a single UTF-8 character.
 */

#include "putty.h"
#include "misc.h"

unsigned long decode_utf8(const char **utf8)
{
    unsigned char c = (unsigned char)*(*utf8)++;

    /* One-byte cases. */
    if (c < 0x80) {
        return c;
    } else if (c < 0xC0) {
        return 0xFFFD; /* spurious continuation byte */
    }

    unsigned long wc, min;
    size_t ncont;
    if (c < 0xE0) {
        wc = c & 0x1F; ncont = 1; min = 0x80;
    } else if (c < 0xF0) {
        wc = c & 0x0F; ncont = 2; min = 0x800;
    } else if (c < 0xF8) {
        wc = c & 0x07; ncont = 3; min = 0x10000;
    } else if (c < 0xFC) {
        wc = c & 0x03; ncont = 4; min = 0x200000;
    } else if (c < 0xFE) {
        wc = c & 0x01; ncont = 5; min = 0x4000000;
    } else {
        return 0xFFFD; /* FE or FF illegal bytes */
    }

    while (ncont-- > 0) {
        unsigned char cont = (unsigned char)**utf8;
        if (!(0x80 <= cont && cont < 0xC0))
            return 0xFFFD;             /* short sequence */
        (*utf8)++;

        wc = (wc << 6) | (cont & 0x3F);
    }

    if (wc < min)
        return 0xFFFD;                 /* overlong encoding */
    if (0xD800 <= wc && wc < 0xE000)
        return 0xFFFD;                 /* UTF-8 encoding of surrogate */
    if (wc > 0x10FFFF)
        return 0xFFFD;                 /* outside Unicode range */
    return wc;
}

#ifdef TEST

#include <stdio.h>

bool dotest(const char *file, int line, const char *input,
            const unsigned long *chars, size_t nchars)
{
    const char *start = input;
    const char *end = input + strlen(input) + 1;
    size_t noutput = 0;

    printf("%s:%d: test start\n", file, line);

    while (input < end) {
        const char *before = input;
        unsigned long wc = decode_utf8(&input);

        printf("%s:%d in+%"SIZEu" out+%"SIZEu":",
               file, line, (size_t)(before-start), noutput);
        while (before < input)
            printf(" %02x", (unsigned)(unsigned char)(*before++));
        printf(" -> U-%08lx\n", wc);

        if (noutput >= nchars) {
            printf("%s:%d: FAIL: expected no further output\n", file, line);
            return false;
        }

        if (chars[noutput] != wc) {
            printf("%s:%d: FAIL: expected U-%08lx\n",
                   file, line, chars[noutput]);
            return false;
        }

        noutput++;
    }

    if (noutput < nchars) {
        printf("%s:%d: FAIL: expected further output\n", file, line);
        return false;
    }

    printf("%s:%d: pass\n", file, line);
    return true;
}

#define DOTEST(input, ...) do {                                         \
        static const unsigned long chars[] = { __VA_ARGS__, 0 };        \
        ntest++;                                                        \
        if (dotest(__FILE__, __LINE__, input, chars, lenof(chars)))     \
            npass++;                                                    \
    } while (0)

int main(void)
{
    int ntest = 0, npass = 0;

    DOTEST("\xCE\xBA\xE1\xBD\xB9\xCF\x83\xCE\xBC\xCE\xB5",
           0x03BA, 0x1F79, 0x03C3, 0x03BC, 0x03B5);

    /* First sequence of each length (not counting NUL, which is
     * tested anyway by the string-termination handling in every test) */
    DOTEST("\xC2\x80", 0x0080);
    DOTEST("\xE0\xA0\x80", 0x0800);
    DOTEST("\xF0\x90\x80\x80", 0x00010000);
    DOTEST("\xF8\x88\x80\x80\x80", 0xFFFD); /* would be 0x00200000 */
    DOTEST("\xFC\x84\x80\x80\x80\x80", 0xFFFD); /* would be 0x04000000 */

    /* Last sequence of each length */
    DOTEST("\x7F", 0x007F);
    DOTEST("\xDF\xBF", 0x07FF);
    DOTEST("\xEF\xBF\xBF", 0xFFFF);
    DOTEST("\xF7\xBF\xBF\xBF", 0xFFFD); /* would be 0x001FFFFF */
    DOTEST("\xFB\xBF\xBF\xBF\xBF", 0xFFFD); /* would be 0x03FFFFFF */
    DOTEST("\xFD\xBF\xBF\xBF\xBF\xBF", 0xFFFD); /* would be 0x7FFFFFFF */

    /* Endpoints of the surrogate range */
    DOTEST("\xED\x9F\xBF", 0xD7FF);
    DOTEST("\xED\xA0\x00", 0xFFFD);    /* would be 0xD800 */
    DOTEST("\xED\xBF\xBF", 0xFFFD);    /* would be 0xDFFF */
    DOTEST("\xEE\x80\x80", 0xE000);

    /* REPLACEMENT CHARACTER itself */
    DOTEST("\xEF\xBF\xBD", 0xFFFD);

    /* Endpoints of the legal Unicode range */
    DOTEST("\xF4\x8F\xBF\xBF", 0x0010FFFF);
    DOTEST("\xF4\x90\x80\x80", 0xFFFD); /* would be 0x00110000 */

    /* Spurious continuation bytes, each shown as a separate failure */
    DOTEST("\x80 \x81\x82 \xBD\xBE\xBF",
           0xFFFD, 0x0020, 0xFFFD, 0xFFFD, 0x0020, 0xFFFD, 0xFFFD, 0xFFFD);

    /* Truncated sequences, each shown as just one failure */
    DOTEST("\xC2\xE0\xA0\xF0\x90\x80\xF8\x88\x80\x80\xFC\x84\x80\x80\x80",
           0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD);
    DOTEST("\xC2 \xE0\xA0 \xF0\x90\x80 \xF8\x88\x80\x80 \xFC\x84\x80\x80\x80",
           0xFFFD, 0x0020, 0xFFFD, 0x0020, 0xFFFD, 0x0020, 0xFFFD, 0x0020,
           0xFFFD);

    /* Illegal bytes */
    DOTEST("\xFE\xFF", 0xFFFD, 0xFFFD);

    /* Overlong sequences */
    DOTEST("\xC1\xBF", 0xFFFD);
    DOTEST("\xE0\x9F\xBF", 0xFFFD);
    DOTEST("\xF0\x8F\xBF\xBF", 0xFFFD);
    DOTEST("\xF8\x87\xBF\xBF\xBF", 0xFFFD);
    DOTEST("\xFC\x83\xBF\xBF\xBF\xBF", 0xFFFD);

    DOTEST("\xC0\x80", 0xFFFD);
    DOTEST("\xE0\x80\x80", 0xFFFD);
    DOTEST("\xF0\x80\x80\x80", 0xFFFD);
    DOTEST("\xF8\x80\x80\x80\x80", 0xFFFD);
    DOTEST("\xFC\x80\x80\x80\x80\x80", 0xFFFD);

    printf("%d tests %d passed", ntest, npass);
    if (npass < ntest) {
        printf(" %d FAILED\n", ntest-npass);
        return 1;
    } else {
        printf("\n");
        return 0;
    }
}
#endif
