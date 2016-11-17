#pragma once

#include "resource.h"

const UINT WM_PROCESS_STARTING = WM_APP + 42;
const UINT WM_PROCESS_COMPLETED = WM_APP + 43;

class CWaitDialog :
    public CDialogImpl<CWaitDialog>
{
    HANDLE m_hThread;
    CString m_commandLine;

public:
    CWaitDialog(const CString &commandLine);
    ~CWaitDialog();

    enum { IDD = IDD_WAITDIALOG };

BEGIN_MSG_MAP(CWaitDialog)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    MESSAGE_HANDLER(WM_PROCESS_STARTING, OnProcessStarting)
    MESSAGE_HANDLER(WM_PROCESS_COMPLETED, OnProcessCompleted)
END_MSG_MAP()

// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnProcessStarting(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnProcessCompleted(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

private:
    static unsigned int __stdcall _ThreadRoutine(void *pParam)
    {
        CWaitDialog *pThis = reinterpret_cast<CWaitDialog *>(pParam);
        return pThis->ThreadRoutine();
    }

    unsigned int ThreadRoutine();
};
