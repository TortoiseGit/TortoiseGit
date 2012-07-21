// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2012 - TortoiseGit
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
#include "Revertdlg.h"
#include "Git.h"
#include "Registry.h"
#include ".\revertdlg.h"
#include "AppUtils.h"

#define REFRESHTIMER   100

IMPLEMENT_DYNAMIC(CRevertDlg, CResizableStandAloneDialog)
CRevertDlg::CRevertDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CRevertDlg::IDD, pParent)
	, m_bSelectAll(TRUE)
	, m_bThreadRunning(FALSE)
	, m_bCancelled(false)
{
}

CRevertDlg::~CRevertDlg()
{
}

void CRevertDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_REVERTLIST, m_RevertList);
	DDX_Check(pDX, IDC_SELECTALL, m_bSelectAll);
	DDX_Control(pDX, IDC_SELECTALL, m_SelectAll);
}


BEGIN_MESSAGE_MAP(CRevertDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
	ON_BN_CLICKED(IDC_SELECTALL, OnBnClickedSelectall)
	ON_REGISTERED_MESSAGE(CGitStatusListCtrl::GITSLNM_NEEDSREFRESH, OnSVNStatusListCtrlNeedsRefresh)
	ON_REGISTERED_MESSAGE(CGitStatusListCtrl::GITSLNM_ADDFILE, OnFileDropped)
	ON_WM_TIMER()
END_MESSAGE_MAP()



BOOL CRevertDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	m_RevertList.Init(GITSLC_COLEXT | GITSLC_COLSTATUS | GITSLC_COLADD| GITSLC_COLDEL, _T("RevertDlg"));
	m_RevertList.SetConfirmButton((CButton*)GetDlgItem(IDOK));
	m_RevertList.SetSelectButton(&m_SelectAll);
	m_RevertList.SetCancelBool(&m_bCancelled);
	m_RevertList.SetBackgroundImage(IDI_REVERT_BKG);
	m_RevertList.EnableFileDrop();

	CString sWindowTitle;
	GetWindowText(sWindowTitle);
	if (m_pathList.GetCount() == 1)
		CAppUtils::SetWindowTitle(m_hWnd, (g_Git.m_CurrentDir + _T("\\") + m_pathList[0].GetUIPathString()).TrimRight('\\'), sWindowTitle);
	else
		CAppUtils::SetWindowTitle(m_hWnd, m_pathList.GetCommonRoot().GetUIPathString(), sWindowTitle);

	AdjustControlSize(IDC_SELECTALL);

	AddAnchor(IDC_REVERTLIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SELECTALL, BOTTOM_LEFT);
	AddAnchor(IDC_UNVERSIONEDITEMS, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	if (hWndExplorer)
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	EnableSaveRestore(_T("RevertDlg"));

	// first start a thread to obtain the file list with the status without
	// blocking the dialog
	if (AfxBeginThread(RevertThreadEntry, this)==0)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	InterlockedExchange(&m_bThreadRunning, TRUE);

	return TRUE;
}

UINT CRevertDlg::RevertThreadEntry(LPVOID pVoid)
{
	return ((CRevertDlg*)pVoid)->RevertThread();
}

UINT CRevertDlg::RevertThread()
{
	// get the status of all selected file/folders recursively
	// and show the ones which can be reverted to the user
	// in a list control.
	DialogEnableWindow(IDOK, false);
	m_bCancelled = false;

	m_RevertList.Clear();

	if (!m_RevertList.GetStatus(&m_pathList))
	{
		m_RevertList.SetEmptyString(m_RevertList.GetLastErrorMessage());
	}
	m_RevertList.Show(GITSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALSFROMDIFFERENTREPOS | GITSLC_SHOWDIRECTFILES | GITSLC_SHOWEXTERNALFROMDIFFERENTREPO,
						// do not select all files, only the ones the user has selected directly
						GITSLC_SHOWDIRECTFILES|GITSLC_SHOWADDED);

	if (m_RevertList.HasUnversionedItems())
	{
		if (DWORD(CRegStdDWORD(_T("Software\\TortoiseGit\\UnversionedAsModified"), FALSE)))
		{
			GetDlgItem(IDC_UNVERSIONEDITEMS)->ShowWindow(SW_SHOW);
		}
		else
			GetDlgItem(IDC_UNVERSIONEDITEMS)->ShowWindow(SW_HIDE);
	}
	else
		GetDlgItem(IDC_UNVERSIONEDITEMS)->ShowWindow(SW_HIDE);

	InterlockedExchange(&m_bThreadRunning, FALSE);
	RefreshCursor();

	return 0;
}

void CRevertDlg::OnOK()
{
	if (m_bThreadRunning)
		return;
	// save only the files the user has selected into the temporary file
	m_bRecursive = TRUE;
	for (int i=0; i<m_RevertList.GetItemCount(); ++i)
	{
		if (!m_RevertList.GetCheck(i))
		{
			m_bRecursive = FALSE;
		}
		else
		{
			m_selectedPathList.AddPath(*(CTGitPath*)m_RevertList.GetItemData(i));
#if 0
			CGitStatusListCtrl::FileEntry * entry = m_RevertList.GetListEntry(i);
			// add all selected entries to the list, except the ones with 'added'
			// status: we later *delete* all the entries in the list before
			// the actual revert is done (so the user has the reverted files
			// still in the trash bin to recover from), but it's not good to
			// delete added files because they're not restored by the revert.
			if (entry->status != svn_wc_status_added)
				m_selectedPathList.AddPath(entry->GetPath());
			// if an entry inside an external is selected, we can't revert
			// recursively anymore because the recursive revert stops at the
			// external boundaries.
			if (entry->IsInExternal())
				m_bRecursive = FALSE;
#endif
		}
	}
	if (!m_bRecursive)
	{
		m_RevertList.WriteCheckedNamesToPathList(m_pathList);
	}
	m_selectedPathList.SortByPathname();

	CResizableStandAloneDialog::OnOK();
}

void CRevertDlg::OnCancel()
{
	m_bCancelled = true;
	if (m_bThreadRunning)
		return;

	CResizableStandAloneDialog::OnCancel();
}

void CRevertDlg::OnBnClickedHelp()
{
	OnHelp();
}

void CRevertDlg::OnBnClickedSelectall()
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
	m_RevertList.SelectAll(state == BST_CHECKED);
	theApp.DoWaitCursor(-1);
}

BOOL CRevertDlg::PreTranslateMessage(MSG* pMsg)
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
					if (AfxBeginThread(RevertThreadEntry, this)==0)
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

LRESULT CRevertDlg::OnSVNStatusListCtrlNeedsRefresh(WPARAM, LPARAM)
{
	if (AfxBeginThread(RevertThreadEntry, this)==0)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	return 0;
}

LRESULT CRevertDlg::OnFileDropped(WPARAM, LPARAM lParam)
{
	BringWindowToTop();
	SetForegroundWindow();
	SetActiveWindow();
	// if multiple files/folders are dropped
	// this handler is called for every single item
	// separately.
	// To avoid creating multiple refresh threads and
	// causing crashes, we only add the items to the
	// list control and start a timer.
	// When the timer expires, we start the refresh thread,
	// but only if it isn't already running - otherwise we
	// restart the timer.
	CTGitPath path;
	path.SetFromWin((LPCTSTR)lParam);

	if (!m_RevertList.HasPath(path))
	{
		if (m_pathList.AreAllPathsFiles())
		{
			m_pathList.AddPath(path);
			m_pathList.RemoveDuplicates();
		}
		else
		{
			// if the path list contains folders, we have to check whether
			// our just (maybe) added path is a child of one of those. If it is
			// a child of a folder already in the list, we must not add it. Otherwise
			// that path could show up twice in the list.
			bool bHasParentInList = false;
			for (int i=0; i<m_pathList.GetCount(); ++i)
			{
				if (m_pathList[i].IsAncestorOf(path))
				{
					bHasParentInList = true;
					break;
				}
			}
			if (!bHasParentInList)
			{
				m_pathList.AddPath(path);
				m_pathList.RemoveDuplicates();
			}
		}
	}

	// Always start the timer, since the status of an existing item might have changed
	SetTimer(REFRESHTIMER, 200, NULL);
	ATLTRACE(_T("Item %s dropped, timer started\n"), path.GetWinPath());
	return 0;
}

void CRevertDlg::OnTimer(UINT_PTR nIDEvent)
{
	switch (nIDEvent)
	{
	case REFRESHTIMER:
		if (m_bThreadRunning)
		{
			SetTimer(REFRESHTIMER, 200, NULL);
			ATLTRACE("Wait some more before refreshing\n");
		}
		else
		{
			KillTimer(REFRESHTIMER);
			ATLTRACE("Refreshing after items dropped\n");
			OnSVNStatusListCtrlNeedsRefresh(0, 0);
		}
		break;
	}
	__super::OnTimer(nIDEvent);
}
