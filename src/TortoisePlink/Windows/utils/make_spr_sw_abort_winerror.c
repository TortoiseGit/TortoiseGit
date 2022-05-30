/*
 * Constructor function for a SeatPromptResult of the 'software abort'
 * category, whose error message includes the translation of an OS
 * error code.
 */

#include "putty.h"

static void spr_winerror_errfn(SeatPromptResult spr, BinarySink *bs)
{
    put_fmt(bs, "%s: %s", spr.errdata_lit, win_strerror(spr.errdata_u));
}

SeatPromptResult make_spr_sw_abort_winerror(const char *prefix, DWORD error)
{
    SeatPromptResult spr;
    spr.kind = SPRK_SW_ABORT;
    spr.errfn = spr_winerror_errfn;
    spr.errdata_lit = prefix;
    spr.errdata_u = error;
    return spr;
}
