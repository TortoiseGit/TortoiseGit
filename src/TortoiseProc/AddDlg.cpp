// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2019 - TortoiseGit
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
#include "MessageBox.h"
#include "AddDlg.h"
#include "PathUtils.h"
#include "Git.h"
#include "AppUtils.h"

#define REFRESHTIMER   100

IMPLEMENT_DYNAMIC(CAddDlg, CResizableStandAloneDialog)
CAddDlg::CAddDlg(CWnd* pParent /*=nullptr*/)
	: CResizableStandAloneDialog(CAddDlg::IDD, pParent)
	, m_bThreadRunning(FALSE)
	, m_bCancelled(false)
	, m_bIncludeIgnored(FALSE)
{
}

CAddDlg::~CAddDlg()
{
}

void CAddDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ADDLIST, m_addListCtrl);
	DDX_Control(pDX, IDC_SELECTALL, m_SelectAll);
	DDX_Check(pDX, IDC_INCLUDE_IGNORED, m_bIncludeIgnored);
}

BEGIN_MESSAGE_MAP(CAddDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_SELECTALL, OnBnClickedSelectall)
	ON_REGISTERED_MESSAGE(CGitStatusListCtrl::GITSLNM_NEEDSREFRESH, OnSVNStatusListCtrlNeedsRefresh)
	ON_REGISTERED_MESSAGE(CGitStatusListCtrl::GITSLNM_ADDFILE, OnFileDropped)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_INCLUDE_IGNORED, &CAddDlg::OnBnClickedIncludeIgnored)
END_MESSAGE_MAP()


BOOL CAddDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	// initialize the svn status list control
	m_addListCtrl.Init(GITSLC_COLEXT, L"AddDlg", GITSLC_POPALL ^ (GITSLC_POPADD | GITSLC_POPCOMMIT | GITSLC_POPCHANGELISTS | GITSLC_POPPREPAREDIFF | GITSLC_POPCHANGELISTS), true, true, GITSLC_COLEXT | GITSLC_COLMODIFICATIONDATE | GITSLC_COLSIZE); // adding and committing is useless in the add dialog
	m_addListCtrl.SetIgnoreRemoveOnly();	// when ignoring, don't add the parent folder since we're in the add dialog
	m_addListCtrl.SetSelectButton(&m_SelectAll);
	m_addListCtrl.SetConfirmButton(static_cast<CButton*>(GetDlgItem(IDOK)));
	m_addListCtrl.SetEmptyString(IDS_ERR_NOTHINGTOADD);
	m_addListCtrl.SetCancelBool(&m_bCancelled);
	m_addListCtrl.SetBackgroundImage(IDI_ADD_BKG);
	m_addListCtrl.EnableFileDrop();

	CString sWindowTitle;
	GetWindowText(sWindowTitle);
	CAppUtils::SetWindowTitle(m_hWnd, g_Git.CombinePath(m_pathList.GetCommonRoot().GetUIPathString()), sWindowTitle);

	AdjustControlSize(IDC_SELECTALL);
	AdjustControlSize(IDC_INCLUDE_IGNORED);

	AddAnchor(IDC_ADDLIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SELECTALL, BOTTOM_LEFT);
	AddAnchor(IDC_INCLUDE_IGNORED, BOTTOM_LEFT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);

	if (GetExplorerHWND())
		CenterWindow(CWnd::FromHandle(GetExplorerHWND()));
	EnableSaveRestore(L"AddDlg");

	//first start a thread to obtain the file list with the status without
	//blocking the dialog
	InterlockedExchange(&m_bThreadRunning, TRUE);
	if (!AfxBeginThread(AddThreadEntry, this))
	{
		InterlockedExchange(&m_bThreadRunning, FALSE);
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	return TRUE;
}

void CAddDlg::OnOK()
{
	if (m_bThreadRunning)
		return;

	// save only the files the user has selected into the path list
	m_addListCtrl.WriteCheckedNamesToPathList(m_pathList);

	CResizableStandAloneDialog::OnOK();
}

void CAddDlg::OnCancel()
{
	m_bCancelled = true;
	if (m_bThreadRunning)
		return;

	CResizableStandAloneDialog::OnCancel();
}

void CAddDlg::OnBnClickedSelectall()
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
	m_addListCtrl.SelectAll(state == BST_CHECKED);
	theApp.DoWaitCursor(-1);
}

UINT CAddDlg::AddThreadEntry(LPVOID pVoid)
{
	return static_cast<CAddDlg*>(pVoid)->AddThread();
}

UINT CAddDlg::AddThread()
{
	// get the status of all selected file/folders recursively
	// and show the ones which the user can add (i.e. the unversioned ones)
	DialogEnableWindow(IDOK, false);
	m_bCancelled = false;
	m_addListCtrl.StoreScrollPos();
	m_addListCtrl.Clear();
	if (!m_addListCtrl.GetStatus(&m_pathList, false, m_bIncludeIgnored != FALSE, true))
		m_addListCtrl.SetEmptyString(m_addListCtrl.GetLastErrorMessage());
	unsigned int dwShow = GITSLC_SHOWUNVERSIONED | GITSLC_SHOWDIRECTFILES | GITSLC_SHOWREMOVEDANDPRESENT;
	if (m_bIncludeIgnored)
		dwShow |= GITSLC_SHOWIGNORED;
	m_addListCtrl.Show(dwShow, GITSLC_SHOWUNVERSIONED);

	InterlockedExchange(&m_bThreadRunning, FALSE);
	return 0;
}

BOOL CAddDlg::PreTranslateMessage(MSG* pMsg)
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
						PostMessage(WM_COMMAND, IDOK);
					return TRUE;
				}
			}
			break;
		case VK_F5:
			{
				Refresh();
			}
			break;
		}
	}

	return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}

LRESULT CAddDlg::OnSVNStatusListCtrlNeedsRefresh(WPARAM, LPARAM)
{
	if (InterlockedExchange(&m_bThreadRunning, TRUE))
		return 0;
	if (!AfxBeginThread(AddThreadEntry, this))
	{
		InterlockedExchange(&m_bThreadRunning, FALSE);
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	return 0;
}

LRESULT CAddDlg::OnFileDropped(WPARAM, LPARAM lParam)
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
	path.SetFromWin(reinterpret_cast<LPCTSTR>(lParam));

	// check whether the dropped file belongs to the very same repository
	CString projectDir;
	if (!path.HasAdminDir(&projectDir) || !CPathUtils::ArePathStringsEqual(g_Git.m_CurrentDir, projectDir))
		return 0;

	if (!m_addListCtrl.HasPath(path))
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
	m_addListCtrl.ResetChecked(path);

	// Always start the timer, since the status of an existing item might have changed
	SetTimer(REFRESHTIMER, 200, nullptr);
	CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Item %s dropped, timer started\n", path.GetWinPath());
	return 0;
}

void CAddDlg::Refresh()
{
	if (InterlockedExchange(&m_bThreadRunning, TRUE))
		return;
	if (!AfxBeginThread(AddThreadEntry, this))
	{
		InterlockedExchange(&m_bThreadRunning, FALSE);
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
}

void CAddDlg::OnTimer(UINT_PTR nIDEvent)
{
	switch (nIDEvent)
	{
	case REFRESHTIMER:
		if (m_bThreadRunning)
		{
			SetTimer(REFRESHTIMER, 200, nullptr);
			CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Wait some more before refreshing\n");
		}
		else
		{
			KillTimer(REFRESHTIMER);
			CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Refreshing after items dropped\n");
			OnSVNStatusListCtrlNeedsRefresh(0, 0);
		}
		break;
	}
	__super::OnTimer(nIDEvent);
}

void CAddDlg::OnBnClickedIncludeIgnored()
{
	UpdateData();
	Refresh();
}
