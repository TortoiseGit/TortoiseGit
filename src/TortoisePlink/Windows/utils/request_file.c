/*
 * GetOpenFileName/GetSaveFileName tend to muck around with the process'
 * working directory on at least some versions of Windows.
 * Here's a wrapper that gives more control over this, and hides a little
 * bit of other grottiness.
 */

#include "putty.h"

struct filereq_tag {
    TCHAR cwd[MAX_PATH];
    WCHAR wcwd[MAX_PATH];
};

/*
 * `of' is expected to be initialised with most interesting fields, but
 * this function does some administrivia. (assume `of' was memset to 0)
 * save==1 -> GetSaveFileName; save==0 -> GetOpenFileName
 * `state' is optional.
 */
bool request_file(filereq *state, OPENFILENAME *of, bool preserve, bool save)
{
    TCHAR cwd[MAX_PATH]; /* process CWD */
    bool ret;

    /* Get process CWD */
    if (preserve) {
        DWORD r = GetCurrentDirectory(lenof(cwd), cwd);
        if (r == 0 || r >= lenof(cwd))
            /* Didn't work, oh well. Stop trying to be clever. */
            preserve = false;
    }

    /* Open the file requester, maybe setting lpstrInitialDir */
    {
#ifdef OPENFILENAME_SIZE_VERSION_400
        of->lStructSize = OPENFILENAME_SIZE_VERSION_400;
#else
        of->lStructSize = sizeof(*of);
#endif
        of->lpstrInitialDir = (state && state->cwd[0]) ? state->cwd : NULL;
        /* Actually put up the requester. */
        ret = save ? GetSaveFileName(of) : GetOpenFileName(of);
    }

    /* Get CWD left by requester */
    if (state) {
        DWORD r = GetCurrentDirectory(lenof(state->cwd), state->cwd);
        if (r == 0 || r >= lenof(state->cwd))
            /* Didn't work, oh well. */
            state->cwd[0] = '\0';
    }

    /* Restore process CWD */
    if (preserve)
        /* If it fails, there's not much we can do. */
        (void) SetCurrentDirectory(cwd);

    return ret;
}

/*
 * Here's the same one again, the wide-string version
 */
bool request_file_w(filereq *state, OPENFILENAMEW *of,
                    bool preserve, bool save)
{
    WCHAR cwd[MAX_PATH]; /* process CWD */
    bool ret;

    /* Get process CWD */
    if (preserve) {
        DWORD r = GetCurrentDirectoryW(lenof(cwd), cwd);
        if (r == 0 || r >= lenof(cwd))
            /* Didn't work, oh well. Stop trying to be clever. */
            preserve = false;
    }

    /* Open the file requester, maybe setting lpstrInitialDir */
    {
        of->lStructSize = sizeof(*of);
        of->lpstrInitialDir = (state && state->wcwd[0]) ? state->wcwd : NULL;
        /* Actually put up the requester. */
        ret = save ? GetSaveFileNameW(of) : GetOpenFileNameW(of);
    }

    /* Get CWD left by requester */
    if (state) {
        DWORD r = GetCurrentDirectoryW(lenof(state->wcwd), state->wcwd);
        if (r == 0 || r >= lenof(state->wcwd))
            /* Didn't work, oh well. */
            state->wcwd[0] = L'\0';
    }

    /* Restore process CWD */
    if (preserve)
        /* If it fails, there's not much we can do. */
        (void) SetCurrentDirectoryW(cwd);

    return ret;
}

filereq *filereq_new(void)
{
    filereq *state = snew(filereq);
    state->cwd[0] = '\0';
    state->wcwd[0] = L'\0';
    return state;
}

void filereq_free(filereq *state)
{
    sfree(state);
}
