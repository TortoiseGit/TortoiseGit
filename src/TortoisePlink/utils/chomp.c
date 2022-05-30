/*
 * Perl-style 'chomp', for a line we just read with fgetline.
 *
 * Unlike Perl chomp, however, we're deliberately forgiving of strange
 * line-ending conventions.
 *
 * Also we forgive NULL on input, so you can just write 'line =
 * chomp(fgetline(fp));' and not bother checking for NULL until
 * afterwards.
 */

#include <string.h>

#include "defs.h"
#include "misc.h"

char *chomp(char *str)
{
    if (str) {
        int len = strlen(str);
        while (len > 0 && (str[len-1] == '\r' || str[len-1] == '\n'))
            len--;
        str[len] = '\0';
    }
    return str;
}
