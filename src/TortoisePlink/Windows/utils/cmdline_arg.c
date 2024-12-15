/*
 * Implementation of the CmdlineArg abstraction for Windows
 */

#include <wchar.h>
#include "putty.h"

typedef struct CmdlineArgWin CmdlineArgWin;
struct CmdlineArgWin {
    /*
     * The original wide-character argument.
     */
    wchar_t *wide;

    /*
     * Two translations of the wide-character argument into UTF-8
     * (maximally faithful to the original) and CP_ACP (the normal
     * system code page).
     */
    char *utf8, *acp;

    /*
     * Our index in the CmdlineArgList, or (size_t)-1 if we don't have
     * one and are an argument invented later.
     */
    size_t index;

    /*
     * Public part of the structure.
     */
    CmdlineArg argp;
};

typedef struct CmdlineArgListWin CmdlineArgListWin;
struct CmdlineArgListWin {
    /*
     * Wide string pointer returned from GetCommandLineW. This points
     * to the 'official' version of the command line, in the sense
     * that overwriting it causes different text to show up in the
     * Task Manager display of the process's command line. (So this is
     * what we'll need to overwrite _on purpose_ for cmdline_arg_wipe.)
     */
    wchar_t *cmdline;

    /*
     * Data returned from split_into_argv_w.
     */
    size_t argc;
    wchar_t **argv, **argstart;

    /*
     * Public part of the structure.
     */
    CmdlineArgList listp;
};

static CmdlineArgWin *cmdline_arg_new_in_list(CmdlineArgList *listp)
{
    CmdlineArgWin *arg = snew(CmdlineArgWin);
    arg->wide = NULL;
    arg->utf8 = arg->acp = NULL;
    arg->index = (size_t)-1;
    arg->argp.list = listp;
    sgrowarray(listp->args, listp->argssize, listp->nargs);
    listp->args[listp->nargs++] = &arg->argp;
    return arg;
}

static CmdlineArg *cmdline_arg_from_wide_argv_word(
    CmdlineArgList *list, wchar_t *word)
{
    CmdlineArgWin *arg = cmdline_arg_new_in_list(list);
    arg->wide = dupwcs(word);
    arg->utf8 = dup_wc_to_mb(CP_UTF8, word, "");
    arg->acp = dup_wc_to_mb(CP_ACP, word, "");
    return &arg->argp;
}

CmdlineArgList *cmdline_arg_list_from_GetCommandLineW(void)
{
    CmdlineArgListWin *list = snew(CmdlineArgListWin);
    CmdlineArgList *listp = &list->listp;

    list->cmdline = GetCommandLineW();

    int argc;
    split_into_argv_w(list->cmdline, true,
                      &argc, &list->argv, &list->argstart);
    list->argc = (size_t)argc;

    listp->args = NULL;
    listp->nargs = listp->argssize = 0;
    for (size_t i = 1; i < list->argc; i++) {
        CmdlineArg *argp = cmdline_arg_from_wide_argv_word(
            listp, list->argv[i]);
        CmdlineArgWin *arg = container_of(argp, CmdlineArgWin, argp);
        arg->index = i - 1; /* index in list->args[], not in argv[] */
    }
    sgrowarray(listp->args, listp->argssize, listp->nargs);
    listp->args[listp->nargs++] = NULL;
    return listp;
}

void cmdline_arg_free(CmdlineArg *argp)
{
    if (!argp)
        return;

    CmdlineArgWin *arg = container_of(argp, CmdlineArgWin, argp);
    burnwcs(arg->wide);
    burnstr(arg->utf8);
    burnstr(arg->acp);
    sfree(arg);
}

void cmdline_arg_list_free(CmdlineArgList *listp)
{
    CmdlineArgListWin *list = container_of(listp, CmdlineArgListWin, listp);
    for (size_t i = 0; i < listp->nargs; i++)
        cmdline_arg_free(listp->args[i]);
    /* list->argv[0] points at the start of the string allocated by
     * split_into_argv_w */
    sfree(list->argv[0]);
    sfree(list->argv);
    sfree(list->argstart);
    sfree(list);
}

CmdlineArg *cmdline_arg_from_str(CmdlineArgList *listp, const char *string)
{
    CmdlineArgWin *arg = cmdline_arg_new_in_list(listp);
    arg->acp = dupstr(string);
    arg->wide = dup_mb_to_wc(CP_ACP, string);
    arg->utf8 = dup_wc_to_mb(CP_UTF8, arg->wide, "");
    return &arg->argp;
}

CmdlineArg *cmdline_arg_from_utf8(CmdlineArgList *listp, const char *string)
{
    CmdlineArgWin *arg = cmdline_arg_new_in_list(listp);
    arg->acp = dupstr(string);
    arg->wide = dup_mb_to_wc(CP_UTF8, string);
    arg->utf8 = dup_wc_to_mb(CP_ACP, arg->wide, "");
    return &arg->argp;
}

const char *cmdline_arg_to_str(CmdlineArg *argp)
{
    if (!argp)
        return NULL;

    CmdlineArgWin *arg = container_of(argp, CmdlineArgWin, argp);
    return arg->acp;
}

const char *cmdline_arg_to_utf8(CmdlineArg *argp)
{
    if (!argp)
        return NULL;

    CmdlineArgWin *arg = container_of(argp, CmdlineArgWin, argp);
    return arg->utf8;
}

Filename *cmdline_arg_to_filename(CmdlineArg *argp)
{
    if (!argp)
        return NULL;

    CmdlineArgWin *arg = container_of(argp, CmdlineArgWin, argp);
    return filename_from_wstr(arg->wide);
}

void cmdline_arg_wipe(CmdlineArg *argp)
{
    if (!argp)
        return;

    CmdlineArgWin *arg = container_of(argp, CmdlineArgWin, argp);
    if (arg->index != (size_t)-1) {
        CmdlineArgList *listp = argp->list;
        CmdlineArgListWin *list = container_of(
            listp, CmdlineArgListWin, listp);

        /* arg->index starts from the first argument _after_ program
         * name, whereas argstart is indexed from argv[0] */
        wchar_t *p = list->argstart[arg->index + 1];
        wchar_t *end = (arg->index + 2 < list->argc ?
                        list->argstart[arg->index + 2] :
                        p + wcslen(p));
        while (p < end)
            *p++ = L' ';
    }
}

const wchar_t *cmdline_arg_remainder_wide(CmdlineArg *argp)
{
    CmdlineArgWin *arg = container_of(argp, CmdlineArgWin, argp);
    CmdlineArgList *listp = argp->list;
    CmdlineArgListWin *list = container_of(listp, CmdlineArgListWin, listp);

    size_t index = arg->index;
    assert(index != (size_t)-1);

    /* arg->index starts from the first argument _after_ program
     * name, whereas argstart is indexed from argv[0] */
    return list->argstart[index + 1];
}

char *cmdline_arg_remainder_acp(CmdlineArg *argp)
{
    return dup_wc_to_mb(CP_ACP, cmdline_arg_remainder_wide(argp), "");
}

char *cmdline_arg_remainder_utf8(CmdlineArg *argp)
{
    return dup_wc_to_mb(CP_UTF8, cmdline_arg_remainder_wide(argp), "");
}
