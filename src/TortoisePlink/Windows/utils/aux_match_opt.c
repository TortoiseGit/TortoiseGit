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
AuxMatchOpt aux_match_opt_init(aux_opt_error_fn_t opt_error)
{
    AuxMatchOpt amo[1];

    amo->arglist = cmdline_arg_list_from_GetCommandLineW();
    amo->index = 0;
    amo->doing_opts = true;
    amo->error = opt_error;

    return amo[0];
}

/*
 * Call this with a NULL-terminated list of synonymous option names.
 * Point 'val' at a char * to receive the option argument, if one is
 * expected. Set 'val' to NULL if no argument is expected.
 */
bool aux_match_opt(AuxMatchOpt *amo, CmdlineArg **val,
                   const char *optname, ...)
{
    /* Find the end of the option name */
    CmdlineArg *optarg = amo->arglist->args[amo->index];
    assert(optarg);
    const char *opt = cmdline_arg_to_utf8(optarg);

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
        *val = cmdline_arg_from_utf8(optarg->list, opt + argopt.len + 1);
        amo->index++;
    } else if (!val) {
        amo->index++;
    } else {
        if (!amo->arglist->args[amo->index + 1])
            amo->error("option '%s' expects a value", opt);
        *val = amo->arglist->args[amo->index + 1];
        amo->index += 2;
    }

    return true;
}

/*
 * Call this to return a non-option argument in *val.
 */
bool aux_match_arg(AuxMatchOpt *amo, CmdlineArg **val)
{
    CmdlineArg *optarg = amo->arglist->args[amo->index];
    assert(optarg);
    const char *opt = cmdline_arg_to_utf8(optarg);

    if (!amo->doing_opts || opt[0] != '-' || !strcmp(opt, "-")) {
        *val = optarg;
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
    CmdlineArg *optarg = amo->arglist->args[amo->index];
    const char *opt = cmdline_arg_to_utf8(optarg);
    if (opt && !strcmp(opt, "--")) {
        amo->doing_opts = false;
        amo->index++;
    }

    return amo->arglist->args[amo->index] == NULL;
}
