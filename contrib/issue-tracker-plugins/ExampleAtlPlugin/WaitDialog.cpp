#include "stdafx.h"
#include "WaitDialog.h"

CWaitDialog::CWaitDialog(const CString &commandLine)
    : m_hThread(NULL)
    , m_commandLine(commandLine)
{
}

CWaitDialog::~CWaitDialog()
{
}

LRESULT CWaitDialog::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // Disable the close button (the 'X').
    HMENU hSystemMenu = GetSystemMenu(FALSE);
    EnableMenuItem(hSystemMenu, SC_CLOSE, MF_BYCOMMAND | MF_GRAYED);

    // Kick off the thread that actually runs the process.
    m_hThread = (HANDLE)_beginthreadex(NULL, 0, _ThreadRoutine, this, 0, NULL);

    bHandled = TRUE;
    return TRUE;
}

LRESULT CWaitDialog::OnProcessStarting(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return 0;
}

LRESULT CWaitDialog::OnProcessCompleted(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    CloseHandle(m_hThread);
    EndDialog(IDOK);
    return 0;
}

unsigned int CWaitDialog::ThreadRoutine()
{
    PostMessage(WM_PROCESS_STARTING);

    STARTUPINFO si;
    SecureZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);

    PROCESS_INFORMATION pi;
    SecureZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

    BOOL ok;

#if defined(UNICODE)
    WCHAR *temp = _wcsdup(m_commandLine);
    ok = CreateProcess(NULL, temp, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    free(temp);
#else
    ok = CreateProcess(NULL, m_commandLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
#endif

    if (ok)
        WaitForSingleObject(pi.hProcess, INFINITE);

    PostMessage(WM_PROCESS_COMPLETED);

    return 0;
}
