/*
 * Debugging routines used by the debug() macros, at least if you
 * compiled with -DDEBUG (aka the PUTTY_DEBUG cmake option) so that
 * those macros don't optimise down to nothing.
 */

#include "defs.h"
#include "misc.h"
#include "utils/utils.h"

void debug_printf(const char *fmt, ...)
{
    char *buf;
    va_list ap;

    va_start(ap, fmt);
    buf = dupvprintf(fmt, ap);
    dputs(buf);
    sfree(buf);
    va_end(ap);
}

void debug_memdump(const void *buf, int len, bool L)
{
    int i;
    const unsigned char *p = buf;
    char foo[17];
    if (L) {
        int delta;
        debug_printf("\t%d (0x%x) bytes:\n", len, len);
        delta = 15 & (uintptr_t)p;
        p -= delta;
        len += delta;
    }
    for (; 0 < len; p += 16, len -= 16) {
        dputs("  ");
        if (L)
            debug_printf("%p: ", p);
        strcpy(foo, "................");        /* sixteen dots */
        for (i = 0; i < 16 && i < len; ++i) {
            if (&p[i] < (unsigned char *) buf) {
                dputs("   ");          /* 3 spaces */
                foo[i] = ' ';
            } else {
                debug_printf("%c%2.2x",
                        &p[i] != (unsigned char *) buf
                        && i % 4 ? '.' : ' ', p[i]
                    );
                if (p[i] >= ' ' && p[i] <= '~')
                    foo[i] = (char) p[i];
            }
        }
        foo[i] = '\0';
        debug_printf("%*s%s\n", (16 - i) * 3 + 2, "", foo);
    }
}
