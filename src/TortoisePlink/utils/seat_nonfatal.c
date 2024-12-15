/*
 * Wrapper function for the nonfatal() method of a Seat,
 * providing printf-style formatting.
 */

#include "putty.h"

void seat_nonfatal(Seat *seat, const char *fmt, ...)
{
    va_list ap;
    char *msg;

    va_start(ap, fmt);
    msg = dupvprintf(fmt, ap);
    va_end(ap);

    seat->vt->nonfatal(seat, msg);
    sfree(msg);
}
