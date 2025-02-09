/*
 * Implementation function behind dupwcscat() in misc.h.
 *
 * This function is called with an arbitrary number of 'const wchar_t
 * *' parameters, of which the last one is a null pointer. The wrapper
 * macro puts on the null pointer itself, so normally callers don't
 * have to.
 */

#include <stdarg.h>
#include <wchar.h>

#include "defs.h"
#include "misc.h"

wchar_t *dupwcscat_fn(const wchar_t *s1, ...)
{
    int len;
    wchar_t *p, *q, *sn;
    va_list ap;

    len = wcslen(s1);
    va_start(ap, s1);
    while (1) {
        sn = va_arg(ap, wchar_t *);
        if (!sn)
            break;
        len += wcslen(sn);
    }
    va_end(ap);

    p = snewn(len + 1, wchar_t);
    wcscpy(p, s1);
    q = p + wcslen(p);

    va_start(ap, s1);
    while (1) {
        sn = va_arg(ap, wchar_t *);
        if (!sn)
            break;
        wcscpy(q, sn);
        q += wcslen(q);
    }
    va_end(ap);

    return p;
}

