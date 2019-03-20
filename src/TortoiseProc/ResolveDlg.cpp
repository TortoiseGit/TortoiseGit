// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2013, 2015-2019 - TortoiseGit
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
#include "ResolveDlg.h"
#include "PathUtils.h"
#include "AppUtils.h"
#include "Git.h"

#define REFRESHTIMER   100

IMPLEMENT_DYNAMIC(CResolveDlg, CResizableStandAloneDialog)
CResolveDlg::CResolveDlg(CWnd* pParent /*=nullptr*/)
	: CResizableStandAloneDialog(CResolveDlg::IDD, pParent)
	, m_bThreadRunning(FALSE)
	, m_bCancelled(false)
{
}

CResolveDlg::~CResolveDlg()
{
}

void CResolveDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_RESOLVELIST, m_resolveListCtrl);
	DDX_Control(pDX, IDC_SELECTALL, m_SelectAll);
}


BEGIN_MESSAGE_MAP(CResolveDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_SELECTALL, OnBnClickedSelectall)
	ON_REGISTERED_MESSAGE(CGitStatusListCtrl::GITSLNM_NEEDSREFRESH, OnSVNStatusListCtrlNeedsRefresh)
	ON_REGISTERED_MESSAGE(CGitStatusListCtrl::GITSLNM_ADDFILE, OnFileDropped)
	ON_WM_TIMER()
END_MESSAGE_MAP()

BOOL CResolveDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	m_resolveListCtrl.Init(GITSLC_COLEXT, L"ResolveDlg", GITSLC_POPALL ^ (GITSLC_POPIGNORE | GITSLC_POPADD | GITSLC_POPCOMMIT | GITSLC_POPEXPORT | GITSLC_POPRESTORE | GITSLC_POPSAVEAS | GITSLC_POPPREPAREDIFF | GITSLC_POPCHANGELISTS));
	m_resolveListCtrl.SetConfirmButton(static_cast<CButton*>(GetDlgItem(IDOK)));
	m_resolveListCtrl.SetSelectButton(&m_SelectAll);
	m_resolveListCtrl.SetCancelBool(&m_bCancelled);
	m_resolveListCtrl.SetBackgroundImage(IDI_RESOLVE_BKG);
	m_resolveListCtrl.EnableFileDrop();

	CString sWindowTitle;
	GetWindowText(sWindowTitle);
	CAppUtils::SetWindowTitle(m_hWnd, g_Git.m_CurrentDir, sWindowTitle);

	AdjustControlSize(IDC_SELECTALL);

	AddAnchor(IDC_RESOLVELIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SELECTALL, BOTTOM_LEFT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	AddAnchor(IDC_STATIC_REMINDER, BOTTOM_RIGHT);
	if (GetExplorerHWND())
		CenterWindow(CWnd::FromHandle(GetExplorerHWND()));
	EnableSaveRestore(L"ResolveDlg");

	// first start a thread to obtain the file list with the status without
	// blocking the dialog
	InterlockedExchange(&m_bThreadRunning, TRUE);
	if (!AfxBeginThread(ResolveThreadEntry, this))
	{
		InterlockedExchange(&m_bThreadRunning, FALSE);
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}

	return TRUE;
}

void CResolveDlg::OnOK()
{
	if (m_bThreadRunning)
		return;

	//save only the files the user has selected into the path list
	m_resolveListCtrl.WriteCheckedNamesToPathList(m_pathList);

	CResizableStandAloneDialog::OnOK();
}

void CResolveDlg::OnCancel()
{
	m_bCancelled = true;
	if (m_bThreadRunning)
		return;

	CResizableStandAloneDialog::OnCancel();
}

void CResolveDlg::OnBnClickedSelectall()
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
	m_resolveListCtrl.SelectAll(state == BST_CHECKED);
	theApp.DoWaitCursor(-1);
}

UINT CResolveDlg::ResolveThreadEntry(LPVOID pVoid)
{
	return static_cast<CResolveDlg*>(pVoid)->ResolveThread();
}
UINT CResolveDlg::ResolveThread()
{
	// get the status of all selected file/folders recursively
	// and show the ones which are in conflict
	DialogEnableWindow(IDOK, false);

	m_bCancelled = false;

	m_resolveListCtrl.StoreScrollPos();
	m_resolveListCtrl.Clear();
	if (!m_resolveListCtrl.GetStatus(&m_pathList))
		m_resolveListCtrl.SetEmptyString(m_resolveListCtrl.GetLastErrorMessage());
	m_resolveListCtrl.Show(GITSLC_SHOWCONFLICTED|GITSLC_SHOWINEXTERNALS, GITSLC_SHOWCONFLICTED);

	InterlockedExchange(&m_bThreadRunning, FALSE);
	return 0;
}

BOOL CResolveDlg::PreTranslateMessage(MSG* pMsg)
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
				if (!InterlockedExchange(&m_bThreadRunning, TRUE))
				{
					if (!AfxBeginThread(ResolveThreadEntry, this))
					{
						InterlockedExchange(&m_bThreadRunning, FALSE);
						CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
					}
				}
			}
			break;
		}
	}

	return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}

LRESULT CResolveDlg::OnSVNStatusListCtrlNeedsRefresh(WPARAM, LPARAM)
{
	if (InterlockedExchange(&m_bThreadRunning, TRUE))
		return 0;
	if (!AfxBeginThread(ResolveThreadEntry, this))
	{
		InterlockedExchange(&m_bThreadRunning, FALSE);
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	return 0;
}

LRESULT CResolveDlg::OnFileDropped(WPARAM, LPARAM lParam)
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

	if (!m_resolveListCtrl.HasPath(path))
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
	m_resolveListCtrl.ResetChecked(path);

	// Always start the timer, since the status of an existing item might have changed
	SetTimer(REFRESHTIMER, 200, nullptr);
	CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Item %s dropped, timer started\n", path.GetWinPath());
	return 0;
}

void CResolveDlg::OnTimer(UINT_PTR nIDEvent)
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
