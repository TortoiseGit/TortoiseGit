/*
 * Function to wrap text to a fixed number of columns.
 *
 * Currently, assumes the text is in a single-byte character set,
 * because it's only used to display host key prompt messages.
 * Extending to Unicode and using wcwidth() could be an extension.
 */

#include "misc.h"

void wordwrap(BinarySink *bs, ptrlen input, size_t maxwid)
{
    size_t col = 0;
    while (true) {
        ptrlen word = ptrlen_get_word(&input, " ");
        if (!word.len)
            break;

        /* At the start of a line, any word is legal, even if it's
         * overlong, because we have to display it _somehow_ and
         * wrapping to the next line won't make it any better. */
        if (col > 0) {
            size_t newcol = col + 1 + word.len;
            if (newcol <= maxwid) {
                put_byte(bs, ' ');
                col++;
            } else {
                put_byte(bs, '\n');
                col = 0;
            }
        }

        put_datapl(bs, word);
        col += word.len;
    }
}
