/*
 * Read an entire line of text from a file. Return a buffer
 * malloced to be as big as necessary (caller must free).
 */

#include "defs.h"
#include "misc.h"

char *fgetline(FILE *fp)
{
    char *ret = snewn(512, char);
    size_t size = 512, len = 0;
    while (fgets(ret + len, size - len, fp)) {
        len += strlen(ret + len);
        if (len > 0 && ret[len-1] == '\n')
            break;                     /* got a newline, we're done */
        sgrowarrayn_nm(ret, size, len, 512);
    }
    if (len == 0) {                    /* first fgets returned NULL */
        sfree(ret);
        return NULL;
    }
    ret[len] = '\0';
    return ret;
}
