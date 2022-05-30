/*
 * Encode a string of wchar_t as UTF-8.
 */

#include "putty.h"
#include "misc.h"

char *encode_wide_string_as_utf8(const wchar_t *ws)
{
    strbuf *sb = strbuf_new();
    while (*ws) {
        unsigned long ch = *ws++;
        if (sizeof(wchar_t) == 2 && IS_HIGH_SURROGATE(ch) &&
            IS_LOW_SURROGATE(*ws)) {
            ch = FROM_SURROGATES(ch, *ws);
            ws++;
        } else if (IS_SURROGATE(ch)) {
            ch = 0xfffd; /* illegal UTF-16 -> REPLACEMENT CHARACTER */
        }
        char utf8[6];
        size_t size = encode_utf8(utf8, ch);
        put_data(sb, utf8, size);
    }
    return strbuf_to_str(sb);
}
