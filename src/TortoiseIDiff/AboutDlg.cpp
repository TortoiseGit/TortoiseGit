#include "StdAfx.h"
#include "Resource.h"
#include "AboutDlg.h"
#include "Registry.h"
#include "..\version.h"
#include <string>
#include <Commdlg.h>

using namespace std;

CAboutDlg::CAboutDlg(HWND hParent)
{
    m_hParent = hParent;
}

CAboutDlg::~CAboutDlg(void)
{
}

LRESULT CAboutDlg::DlgFunc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            InitDialog(hwndDlg, IDI_TORTOISEIDIFF);
            // initialize the controls
            TCHAR verbuf[1024] = {0};
            TCHAR maskbuf[1024] = {0};
            if (!::LoadString (hResource, IDS_VERSION, maskbuf, _countof(maskbuf)))
            {
                SecureZeroMemory(maskbuf, sizeof(maskbuf));
            }
            _stprintf_s(verbuf, maskbuf, TGIT_VERMAJOR, TGIT_VERMINOR, TGIT_VERMICRO, TGIT_VERBUILD);
            SetDlgItemText(hwndDlg, IDC_ABOUTVERSION, verbuf);
        }
        return TRUE;
    case WM_COMMAND:
        return DoCommand(LOWORD(wParam));
    default:
        return FALSE;
    }
}

LRESULT CAboutDlg::DoCommand(int id)
{
    switch (id)
    {
    case IDOK:
        // fall through
    case IDCANCEL:
        EndDialog(*this, id);
        break;
    }
    return 1;
}

