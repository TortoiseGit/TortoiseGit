/*
 * Split a complete command line into argc/argv, attempting to do it
 * exactly the same way the Visual Studio C library would do it (so
 * that our console utilities, which receive argc and argv already
 * broken apart by the C library, will have their command lines
 * processed in the same way as the GUI utilities which get a whole
 * command line and must call this function).
 *
 * Does not modify the input command line.
 *
 * The final parameter (argstart) is used to return a second array
 * of char * pointers, the same length as argv, each one pointing
 * at the start of the corresponding element of argv in the
 * original command line. So if you get half way through processing
 * your command line in argc/argv form and then decide you want to
 * treat the rest as a raw string, you can. If you don't want to,
 * `argstart' can be safely left NULL.
 */

#include "putty.h"

/*
 * The precise argument-breaking rules vary with compiler version, or
 * rather, with the crt0-type startup code that comes with each
 * compiler's C library. We do our best to match the compiler version,
 * so that we faithfully imitate in our GUI utilities what the
 * corresponding set of CLI utilities can't be prevented from doing.
 *
 * The basic rules are:
 *
 *  - Single quotes are not special characters.
 *
 *  - Double quotes are removed, but within them spaces cease to be
 *    special.
 *
 *  - Backslashes are _only_ special when a sequence of them appear
 *    just before a double quote. In this situation, they are treated
 *    like C backslashes: so \" just gives a literal quote, \\" gives
 *    a literal backslash and then opens or closes a double-quoted
 *    segment, \\\" gives a literal backslash and then a literal
 *    quote, \\\\" gives two literal backslashes and then opens/closes
 *    a double-quoted segment, and so forth. Note that this behaviour
 *    is identical inside and outside double quotes.
 *
 *  - Two successive double quotes become one literal double quote,
 *    but only _inside_ a double-quoted segment. Outside, they just
 *    form an empty double-quoted segment (which may cause an empty
 *    argument word).
 *
 * That only leaves the interesting question of what happens when one
 * or more backslashes precedes two or more double quotes, starting
 * inside a double-quoted string.
 *
 * Modern Visual Studio (as of 2021)
 * ---------------------------------
 *
 * I investigated this in an ordinary CLI program, using the
 * toolchain's crt0 to split a command line of the form
 *
 *    "a\\\"""b c" d
 *
 * Here I tabulate number of backslashes (across the top) against
 * number of quotes (down the left), and indicate how many backslashes
 * are output, how many quotes are output, and whether a quoted
 * segment is open at the end of the sequence:
 *
 *                      backslashes
 *
 *               0         1      2      3      4
 *
 *         0   0,0,y  |  1,0,y  2,0,y  3,0,y  4,0,y
 *            --------+-----------------------------
 *         1   0,0,n  |  0,1,y  1,0,n  1,1,y  2,0,n
 *    q    2   0,1,y  |  0,1,n  1,1,y  1,1,n  2,1,y
 *    u    3   0,1,n  |  0,2,y  1,1,n  1,2,y  2,1,n
 *    o    4   0,2,y  |  0,2,n  1,2,y  1,2,n  2,2,y
 *    t    5   0,2,n  |  0,3,y  1,2,n  1,3,y  2,2,n
 *    e    6   0,3,y  |  0,3,n  1,3,y  1,3,n  2,3,y
 *    s    7   0,3,n  |  0,4,y  1,3,n  1,4,y  2,3,n
 *         8   0,4,y  |  0,4,n  1,4,y  1,4,n  2,4,y
 *
 * The row at the top of this table, with quotes=0, demonstrates what
 * I claimed above, that when a sequence of backslashes are not
 * followed by a double quote, they don't act specially at all. The
 * rest of the table shows that the backslashes escape each other in
 * pairs (so that with 2n or 2n+1 input backslashes you get n output
 * ones); if there's an odd number of input backslashes then the last
 * one escapes the first double quote (so you get a literal quote and
 * enter a quoted string); thereafter, each input quote character
 * either opens or closes a quoted string, and if it closes one, it
 * generates a literal " as a side effect.
 *
 * Older Visual Studio
 * -------------------
 *
 * But here's the corresponding table from the older Visual Studio 7:
 *
 *                      backslashes
 *
 *               0         1      2      3      4
 *
 *         0   0,0,y  |  1,0,y  2,0,y  3,0,y  4,0,y
 *            --------+-----------------------------
 *         1   0,0,n  |  0,1,y  1,0,n  1,1,y  2,0,n
 *    q    2   0,1,n  |  0,1,n  1,1,n  1,1,n  2,1,n
 *    u    3   0,1,y  |  0,2,n  1,1,y  1,2,n  2,1,y
 *    o    4   0,1,n  |  0,2,y  1,1,n  1,2,y  2,1,n
 *    t    5   0,2,n  |  0,2,n  1,2,n  1,2,n  2,2,n
 *    e    6   0,2,y  |  0,3,n  1,2,y  1,3,n  2,2,y
 *    s    7   0,2,n  |  0,3,y  1,2,n  1,3,y  2,2,n
 *         8   0,3,n  |  0,3,n  1,3,n  1,3,n  2,3,n
 *         9   0,3,y  |  0,4,n  1,3,y  1,4,n  2,3,y
 *        10   0,3,n  |  0,4,y  1,3,n  1,4,y  2,3,n
 *        11   0,4,n  |  0,4,n  1,4,n  1,4,n  2,4,n
 *
 * There is very weird mod-3 behaviour going on here in the number of
 * quotes, and it even applies when there aren't any backslashes! How
 * ghastly.
 *
 * With a bit of thought, this extremely odd diagram suddenly
 * coalesced itself into a coherent, if still ghastly, model of how
 * things work:
 *
 *  - As before, backslashes are only special when one or more of them
 *    appear contiguously before at least one double quote. In this
 *    situation the backslashes do exactly what you'd expect: each one
 *    quotes the next thing in front of it, so you end up with n/2
 *    literal backslashes (if n is even) or (n-1)/2 literal
 *    backslashes and a literal quote (if n is odd). In the latter
 *    case the double quote character right after the backslashes is
 *    used up.
 *
 *  - After that, any remaining double quotes are processed. A string
 *    of contiguous unescaped double quotes has a mod-3 behaviour:
 *
 *     * inside a quoted segment, a quote ends the segment.
 *     * _immediately_ after ending a quoted segment, a quote simply
 *       produces a literal quote.
 *     * otherwise, outside a quoted segment, a quote begins a quoted
 *       segment.
 *
 *    So, for example, if we started inside a quoted segment then two
 *    contiguous quotes would close the segment and produce a literal
 *    quote; three would close the segment, produce a literal quote,
 *    and open a new segment. If we started outside a quoted segment,
 *    then two contiguous quotes would open and then close a segment,
 *    producing no output (but potentially creating a zero-length
 *    argument); but three quotes would open and close a segment and
 *    then produce a literal quote.
 *
 * I don't know exactly when the bug fix happened, but I know that VS7
 * had the odd mod-3 behaviour. So the #if below will ensure that
 * modern (2015 onwards) versions of VS use the new more sensible
 * behaviour, and VS7 uses the old one. Things in between may be
 * wrong; if anyone cares, patches to change the cutoff version in
 * this #if are welcome.
 */
#if _MSC_VER < 1400
#define MOD3 1
#else
#define MOD3 0
#endif

#ifdef SPLIT_INTO_ARGV_W
#define FUNCTION split_into_argv_w
#define CHAR wchar_t
#define STRLEN wcslen
#else
#define FUNCTION split_into_argv
#define CHAR char
#define STRLEN strlen
#endif

static inline bool is_word_sep(CHAR c)
{
    return c == ' ' || c == '\t';
}

void FUNCTION(const CHAR *cmdline, bool includes_program_name,
              int *argc, CHAR ***argv, CHAR ***argstart)
{
    const CHAR *p;
    CHAR *outputline, *q;
    CHAR **outputargv, **outputargstart;
    int outputargc;

    /*
     * First deal with the simplest of all special cases: if there
     * aren't any arguments, return 0,NULL,NULL.
     */
    while (*cmdline && is_word_sep(*cmdline)) cmdline++;
    if (!*cmdline) {
        if (argc) *argc = 0;
        if (argv) *argv = NULL;
        if (argstart) *argstart = NULL;
        return;
    }

    /*
     * This will guaranteeably be big enough; we can realloc it
     * down later.
     */
    {
        size_t len = STRLEN(cmdline);
        outputline = snewn(1+len, CHAR);
        outputargv = snewn((len+1) / 2, CHAR *);
        outputargstart = snewn((len+1) / 2, CHAR *);
    }

    p = cmdline; q = outputline; outputargc = 0;

    while (*p) {
        bool quote;

        /* Skip whitespace searching for start of argument. */
        while (*p && is_word_sep(*p)) p++;
        if (!*p) break;

        /*
         * Check if this argument is the program name. If so,
         * different rules apply.
         *
         * In most arguments, the special characters are the double
         * quote and the backslash. An exception is the program name
         * at the start of the command line, in which backslashes are
         * _not_ special - if one appears before a quote, it does not
         * make the quote literal.
         *
         * The C library must implement this special rule, and we must
         * follow suit here, in order to match the way CreateProcess
         * scans the command line to determine the program name. It
         * will consider that all these commands refer to the same
         * file equally validly:
         *
         *   "C:\Program Files\Foo"\bar.exe
         *   "C:\Program Files\Foo\"bar.exe
         *   "C:\Program Files\Foo\bar.exe"
         *
         * Each one contains a quoted section that protects the space
         * in "Program Files", and the closing quote takes effect the
         * same in all cases - even though, in the middle case, it's
         * immediately preceded by one of the path separators in the
         * name. For CreateProcess, backslashes aren't special.
         *
         * So, if our caller told us that the input command line
         * includes the program name (which it does if it came from
         * GetCommandLine, but not if it was passed in to WinMain),
         * then we must treat the 0th output argument specially, by
         * not considering backslashes to affect the quoting.
         */
        bool backslash_special = !(outputargc == 0 && includes_program_name);

        /* We have an argument; start it. */
        outputargv[outputargc] = q;
        outputargstart[outputargc] = (CHAR *)p;
        outputargc++;
        quote = false;

        /* Copy data into the argument until it's finished. */
        while (*p) {
            if (!quote && is_word_sep(*p))
                break;                 /* argument is finished */

            if (*p == '"' || (*p == '\\' && backslash_special)) {
                /*
                 * We have a sequence of zero or more backslashes
                 * followed by a sequence of zero or more quotes.
                 * Count up how many of each, and then deal with
                 * them as appropriate.
                 */
                int i, slashes = 0, quotes = 0;
                while (*p == '\\') slashes++, p++;
                while (*p == '"') quotes++, p++;

                if (!quotes) {
                    /*
                     * Special case: if there are no quotes,
                     * slashes are not special at all, so just copy
                     * n slashes to the output string.
                     */
                    while (slashes--) *q++ = '\\';
                } else {
                    /* Slashes annihilate in pairs. */
                    while (slashes >= 2) slashes -= 2, *q++ = '\\';

                    /* One remaining slash takes out the first quote. */
                    if (slashes) quotes--, *q++ = '"';

                    if (quotes > 0) {
                        /* Outside a quote segment, a quote starts one. */
                        if (!quote) quotes--;

#if !MOD3
                        /* New behaviour: produce n/2 literal quotes... */
                        for (i = 2; i <= quotes; i += 2) *q++ = '"';
                        /* ... and end in a quote segment iff 2 divides n. */
                        quote = (quotes % 2 == 0);
#else
                        /* Old behaviour: produce (n+1)/3 literal quotes... */
                        for (i = 3; i <= quotes+1; i += 3) *q++ = '"';
                        /* ... and end in a quote segment iff 3 divides n. */
                        quote = (quotes % 3 == 0);
#endif
                    }
                }
            } else {
                *q++ = *p++;
            }
        }

        /* At the end of an argument, just append a trailing NUL. */
        *q++ = '\0';
    }

    outputargv = sresize(outputargv, outputargc, CHAR *);
    outputargstart = sresize(outputargstart, outputargc, CHAR *);

    if (argc) *argc = outputargc;
    if (argv) *argv = outputargv; else sfree(outputargv);
    if (argstart) *argstart = outputargstart; else sfree(outputargstart);
}
