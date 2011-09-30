// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#include "stdafx.h"
#include "TortoiseProc.h"
#include "messagebox.h"
#include "SVN.h"
#include "Registry.h"
#include "DeleteUnversionedDlg.h"

IMPLEMENT_DYNAMIC(CDeleteUnversionedDlg, CResizableStandAloneDialog)
CDeleteUnversionedDlg::CDeleteUnversionedDlg(CWnd* pParent /*=NULL*/)
: CResizableStandAloneDialog(CDeleteUnversionedDlg::IDD, pParent)
, m_bSelectAll(TRUE)
, m_bThreadRunning(FALSE)
, m_bCancelled(false)
{
}

CDeleteUnversionedDlg::~CDeleteUnversionedDlg()
{
}

void CDeleteUnversionedDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ITEMLIST, m_StatusList);
	DDX_Check(pDX, IDC_SELECTALL, m_bSelectAll);
	DDX_Control(pDX, IDC_SELECTALL, m_SelectAll);
}


BEGIN_MESSAGE_MAP(CDeleteUnversionedDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_SELECTALL, OnBnClickedSelectall)
	ON_REGISTERED_MESSAGE(CSVNStatusListCtrl::SVNSLNM_NEEDSREFRESH, OnSVNStatusListCtrlNeedsRefresh)
END_MESSAGE_MAP()



BOOL CDeleteUnversionedDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	m_StatusList.Init(SVNSLC_COLEXT | SVNSLC_COLSTATUS, _T("DeleteUnversionedDlg"), 0, true);
	m_StatusList.SetUnversionedRecurse(true);
	m_StatusList.PutUnversionedLast(false);
	m_StatusList.CheckChildrenWithParent(true);
	m_StatusList.SetConfirmButton((CButton*)GetDlgItem(IDOK));
	m_StatusList.SetSelectButton(&m_SelectAll);
	m_StatusList.SetCancelBool(&m_bCancelled);
	m_StatusList.SetBackgroundImage(IDI_DELUNVERSIONED_BKG);

	GetWindowText(m_sWindowTitle);

	AdjustControlSize(IDC_SELECTALL);

	AddAnchor(IDC_ITEMLIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SELECTALL, BOTTOM_LEFT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	if (hWndExplorer)
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	EnableSaveRestore(_T("DeleteUnversionedDlg"));

	// first start a thread to obtain the file list with the status without
	// blocking the dialog
	if (AfxBeginThread(StatusThreadEntry, this)==0)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	InterlockedExchange(&m_bThreadRunning, TRUE);

	return TRUE;
}

UINT CDeleteUnversionedDlg::StatusThreadEntry(LPVOID pVoid)
{
	return ((CDeleteUnversionedDlg*)pVoid)->StatusThread();
}

UINT CDeleteUnversionedDlg::StatusThread()
{
	// get the status of all selected file/folders recursively
	// and show the ones which are unversioned/ignored to the user
	// in a list control.
	DialogEnableWindow(IDOK, false);
	m_bCancelled = false;

	if (!m_StatusList.GetStatus(m_pathList, false, true))
	{
		m_StatusList.SetEmptyString(m_StatusList.GetLastErrorMessage());
	}
	m_StatusList.Show(SVNSLC_SHOWUNVERSIONED | SVNSLC_SHOWIGNORED,
		SVNSLC_SHOWUNVERSIONED | SVNSLC_SHOWIGNORED);

	CTSVNPath commonDir = m_StatusList.GetCommonDirectory(false);
	SetWindowText(m_sWindowTitle + _T(" - ") + commonDir.GetWinPathString());

	InterlockedExchange(&m_bThreadRunning, FALSE);
	RefreshCursor();

	return 0;
}

void CDeleteUnversionedDlg::OnOK()
{
	if (m_bThreadRunning)
		return;
	// save only the files the user has selected into the temporary file
	m_StatusList.WriteCheckedNamesToPathList(m_pathList);

	CResizableStandAloneDialog::OnOK();
}

void CDeleteUnversionedDlg::OnCancel()
{
	m_bCancelled = true;
	if (m_bThreadRunning)
		return;

	CResizableStandAloneDialog::OnCancel();
}

void CDeleteUnversionedDlg::OnBnClickedSelectall()
{
	UINT state = (m_SelectAll.GetState() & 0x0003);
	if (state == BST_INDETERMINATE)
	{
		// It is not at all useful to manually place the checkbox into the indeterminate state...
		// We will force this on to the unchecked state
		state = BST_UNCHECKED;
		m_SelectAll.SetCheck(state);
	}
	theApp.DoWaitCursor(1);
	m_StatusList.SelectAll(state == BST_CHECKED);
	theApp.DoWaitCursor(-1);
}

BOOL CDeleteUnversionedDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case VK_RETURN:
			{
				if (GetAsyncKeyState(VK_CONTROL)&0x8000)
				{
					if ( GetDlgItem(IDOK)->IsWindowEnabled() )
					{
						PostMessage(WM_COMMAND, IDOK);
					}
					return TRUE;
				}
			}
			break;
		case VK_F5:
			{
				if (!m_bThreadRunning)
				{
					if (AfxBeginThread(StatusThreadEntry, this)==0)
					{
						CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
					}
					else
						InterlockedExchange(&m_bThreadRunning, TRUE);
				}
			}
			break;
		}
	}

	return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}

LRESULT CDeleteUnversionedDlg::OnSVNStatusListCtrlNeedsRefresh(WPARAM, LPARAM)
{
	if (AfxBeginThread(StatusThreadEntry, this)==0)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	return 0;
}
