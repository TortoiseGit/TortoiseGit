#include "StdAfx.h"

#include "MainDlg.h"
#include "resource.h"

CMainDlg::CMainDlg(void)
{
}

CMainDlg::~CMainDlg(void)
{
}

LRESULT CMainDlg::DlgFunc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			InitDialog(hwndDlg, IDR_MAINFRAME);
			//
			// Set title using app name
			//
			::SetWindowText(*this, CUtility::getAppName().c_str());
			// Hide 'Send' button if required. Position 'Send' and 'Save'.
			//
			HWND okButton = ::GetDlgItem(*this, IDOK);
			HWND saveButton = ::GetDlgItem(*this, IDC_SAVE);
			if (m_sendButton) 
			{
				// Line up Save, Send [OK] and Exit [Cancel] all in a row
				HWND cancelButton = ::GetDlgItem(*this, IDCANCEL);
				WINDOWPLACEMENT okPlace;
				WINDOWPLACEMENT savePlace;
				WINDOWPLACEMENT cancelPlace;

				::GetWindowPlacement(okButton, &okPlace);
				::GetWindowPlacement(saveButton, &savePlace);
				::GetWindowPlacement(cancelButton, &cancelPlace);

				savePlace.rcNormalPosition.left =
					okPlace.rcNormalPosition.left -
					(savePlace.rcNormalPosition.right - savePlace.rcNormalPosition.left) +
					(okPlace.rcNormalPosition.right - cancelPlace.rcNormalPosition.left);
				::SetWindowPlacement(saveButton, &savePlace);

				DWORD style = ::GetWindowLong(okButton, GWL_STYLE);
				::SetWindowLong(okButton, GWL_STYLE, style  | WS_VISIBLE);
			} 
			else 
			{
				WINDOWPLACEMENT okPlace;

				// Put Save on top of the invisible Send [OK]
				::GetWindowPlacement(okButton, &okPlace);

				::SetWindowPlacement(saveButton, &okPlace);

				DWORD style = ::GetWindowLong(okButton, GWL_STYLE);
				::SetWindowLong(okButton, GWL_STYLE, style  & ~ WS_VISIBLE);
			}
		}
		return TRUE;
	case WM_COMMAND:
		return DoCommand(LOWORD(wParam));
	default:
		return FALSE;
	}
	return TRUE;
}

LRESULT CMainDlg::DoCommand(int id)
{
	switch (id)
	{
	case IDOK: // send
		{
			HWND     hWndEmail = ::GetDlgItem(*this, IDC_EMAIL);
			HWND     hWndDesc = ::GetDlgItem(*this, IDC_DESCRIPTION);
			int      nEmailLen = ::GetWindowTextLength(hWndEmail) + 1;
			int      nDescLen = ::GetWindowTextLength(hWndDesc) + 1;

			TCHAR * lpStr = new TCHAR[nEmailLen+1];
			::GetWindowText(hWndEmail, lpStr, nEmailLen);
			m_sEmail = lpStr;
			delete [] lpStr;

			lpStr = new TCHAR[nDescLen+1];
			::GetWindowText(hWndDesc, lpStr, nDescLen);
			m_sDescription = lpStr;
			delete [] lpStr;
		}
		EndDialog(*this, IDOK);
		return 0;
		break;
	case IDC_SAVE:
		EndDialog(*this, IDC_SAVE);
		return 0;
		break;
	case IDCANCEL:
		EndDialog(*this, IDCANCEL);
		PostQuitMessage(IDCANCEL);
		return 0;
		break;
	}
	return 1;
}
