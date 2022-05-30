/*
 * Helper function for matching command-line options in the Windows
 * auxiliary tools (PuTTYgen and Pageant).
 *
 * Supports basically the usual kinds of option, including GNUish
 * --foo long options and simple -f short options. But historically
 * those tools have also supported long options with a single dash, so
 * we don't go full GNU and report those as syntax errors.
 */

#include "putty.h"

/*
 * Call this to initialise the state structure.
 */
AuxMatchOpt aux_match_opt_init(int argc, char **argv, int start_index,
                               aux_opt_error_fn_t opt_error)
{
    AuxMatchOpt amo[1];

    amo->argc = argc;
    amo->argv = argv;
    amo->index = start_index;
    amo->doing_opts = true;
    amo->error = opt_error;

    return amo[0];
}

/*
 * Call this with a NULL-terminated list of synonymous option names.
 * Point 'val' at a char * to receive the option argument, if one is
 * expected. Set 'val' to NULL if no argument is expected.
 */
bool aux_match_opt(AuxMatchOpt *amo, char **val, const char *optname, ...)
{
    assert(amo->index < amo->argc);

    /* Find the end of the option name */
    char *opt = amo->argv[amo->index];
    ptrlen argopt;
    argopt.ptr = opt;
    argopt.len = strcspn(opt, "=");

    /* Normalise GNU-style --foo long options to the -foo that this
     * tool has historically used */
    ptrlen argopt2 = make_ptrlen(NULL, 0);
    if (ptrlen_startswith(argopt, PTRLEN_LITERAL("--"), NULL))
        ptrlen_startswith(argopt, PTRLEN_LITERAL("-"), &argopt2);

    /* See if this option matches any of our synonyms */
    va_list ap;
    va_start(ap, optname);
    bool matched = false;
    while (optname) {
        if (ptrlen_eq_string(argopt, optname)) {
            matched = true;
            break;
        }
        if (argopt2.ptr && strlen(optname) > 2 &&
            ptrlen_eq_string(argopt2, optname)) {
            matched = true;
            break;
        }
        optname = va_arg(ap, const char *);
    }
    va_end(ap);
    if (!matched)
        return false;

    /* Check for a value */
    if (opt[argopt.len]) {
        if (!val)
            amo->error("option '%s' does not expect a value", opt);
        *val = opt + argopt.len + 1;
        amo->index++;
    } else if (!val) {
        amo->index++;
    } else {
        if (amo->index + 1 >= amo->argc)
            amo->error("option '%s' expects a value", opt);
        *val = amo->argv[amo->index + 1];
        amo->index += 2;
    }

    return true;
}

/*
 * Call this to return a non-option argument in *val.
 */
bool aux_match_arg(AuxMatchOpt *amo, char **val)
{
    assert(amo->index < amo->argc);
    char *opt = amo->argv[amo->index];

    if (!amo->doing_opts || opt[0] != '-' || !strcmp(opt, "-")) {
        *val = opt;
        amo->index++;
        return true;
    }

    return false;
}

/*
 * And call this to test whether we're done. Also handles '--'.
 */
bool aux_match_done(AuxMatchOpt *amo)
{
    if (amo->index < amo->argc && !strcmp(amo->argv[amo->index], "--")) {
        amo->doing_opts = false;
        amo->index++;
    }

    return amo->index >= amo->argc;
}
