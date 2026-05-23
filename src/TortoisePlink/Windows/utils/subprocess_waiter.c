/*
 * Windows implementation of SubprocessWaiter, delegating to
 * add_handle_wait to wait for the process handle.
 */

#include "putty.h"

struct SubprocessWaiter {
    HANDLE hprocess;
    HandleWait *hw;

    SubprocessWaiterCallback callback;
    void *cbctx;
};

static void subproc_waiter_callback(void *vctx)
{
    SubprocessWaiter *waiter = (SubprocessWaiter *)vctx;

    DWORD exitstatus;
    if (!GetExitCodeProcess(waiter->hprocess, &exitstatus))
        return;                        /* process hasn't exited yet */

    /* Stop waiting for this handle, and close it. */
    if (waiter->hw) {
        delete_handle_wait(waiter->hw);
        waiter->hw = NULL;
    }

    if (waiter->hprocess != INVALID_HANDLE_VALUE) {
        CloseHandle(waiter->hprocess);
        waiter->hprocess = INVALID_HANDLE_VALUE;
    }

    /*
     * As mentioned in putty.h, for Windows we count everything as
     * EXITTYPE_NORMAL.
     *
     * If we wanted nicer error messages, we could identify a list of
     * interesting exit statuses like STATUS_CONTROL_C_EXIT and
     * STATUS_ACCESS_VIOLATION and give those dedicated text
     * translations, but that would involve some more complicated API
     * for making the text translations and at the moment I don't
     * think it's especially important.
     */
    waiter->callback(waiter->cbctx, EXITTYPE_NORMAL, exitstatus);
}

SubprocessWaiter *subproc_waiter_from_hprocess(HANDLE hprocess)
{
    SubprocessWaiter *waiter = snew(SubprocessWaiter);
    waiter->callback = NULL;
    waiter->cbctx = NULL;
    waiter->hprocess = hprocess;
    waiter->hw = add_handle_wait(hprocess, subproc_waiter_callback, waiter);
    return waiter;
}

void subproc_waiter_set_callback(
    SubprocessWaiter *waiter, SubprocessWaiterCallback cb, void *cbctx)
{
    waiter->callback = cb;
    waiter->cbctx = cbctx;
}

void subproc_waiter_free(SubprocessWaiter *waiter)
{
    if (waiter->hw)
        delete_handle_wait(waiter->hw);
    if (waiter->hprocess != INVALID_HANDLE_VALUE)
        CloseHandle(waiter->hprocess);
    sfree(waiter);
}
