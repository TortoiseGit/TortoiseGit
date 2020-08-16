// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2013, 2016-2020 - TortoiseGit
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
#include "LFSLocksDlg.h"
#include "Git.h"
#include "AppUtils.h"
#include "GitProgressDlg.h"
#include "ProgressCommands/LFSSetLockedProgressCommand.h"

#define REFRESHTIMER    100

IMPLEMENT_DYNAMIC(CLFSLocksDlg, CResizableStandAloneDialog)
CLFSLocksDlg::CLFSLocksDlg(CWnd* pParent /*=nullptr*/)
	: CResizableStandAloneDialog(CLFSLocksDlg::IDD, pParent)
	, m_bThreadRunning(FALSE)
	, m_bCancelled(false)
{
}

CLFSLocksDlg::~CLFSLocksDlg()
{
}

void CLFSLocksDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LOCKSLIST, m_LocksList);
	DDX_Control(pDX, IDC_SELECTALL, m_SelectAll);
}

BEGIN_MESSAGE_MAP(CLFSLocksDlg, CResizableStandAloneDialog)
	ON_REGISTERED_MESSAGE(CGitStatusListCtrl::GITSLNM_NEEDSREFRESH, OnStatusListCtrlNeedsRefresh)
	ON_BN_CLICKED(IDC_LFS_UNLOCK, OnBnClickedUnLock)
	ON_BN_CLICKED(IDC_SELECTALL, OnBnClickedSelectall)
	ON_WM_TIMER()
END_MESSAGE_MAP()

BOOL CLFSLocksDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	m_LocksList.Init(GITSLC_COLEXT | GITSLC_COLLFSLOCK, L"LFSLocksDlg", GITSLC_POPLFSLOCK | GITSLC_POPLFSUNLOCK, true, true, GITSLC_COLEXT | GITSLC_COLFILENAME | GITSLC_COLSIZE | GITSLC_COLMODIFICATIONDATE | GITSLC_COLLFSLOCK);
	m_LocksList.SetSelectButton(&m_SelectAll);
	m_LocksList.SetConfirmButton(static_cast<CButton*>(GetDlgItem(IDC_LFS_UNLOCK)));
	m_LocksList.SetCancelBool(&m_bCancelled);
	m_LocksList.SetBackgroundImage(IDI_LOCK_BKG);
	m_LocksList.m_bEnableDblClick = false;

	CString sWindowTitle;
	GetWindowText(sWindowTitle);
	CAppUtils::SetWindowTitle(m_hWnd, g_Git.m_CurrentDir, sWindowTitle);

	AdjustControlSize(IDC_SELECTALL);

	AddAnchor(IDC_LOCKSLIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SELECTALL, BOTTOM_LEFT);
	AddAnchor(IDC_LFS_UNLOCK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	if (GetExplorerHWND())
		CenterWindow(CWnd::FromHandle(GetExplorerHWND()));
	EnableSaveRestore(L"LFSLocksDlg");

	// first start a thread to obtain the file list with the status without 
	// blocking the dialog
	if (AfxBeginThread(LocksThreadEntry, this) == nullptr)
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	InterlockedExchange(&m_bThreadRunning, TRUE);

	return TRUE;
}

UINT CLFSLocksDlg::LocksThreadEntry(LPVOID pVoid)
{
	return reinterpret_cast<CLFSLocksDlg*>(pVoid)->LocksThread();
}

UINT CLFSLocksDlg::LocksThread()
{
	DialogEnableWindow(IDC_LFS_UNLOCK, false);
	DialogEnableWindow(IDC_SELECTALL, false);

	m_bCancelled = false;

	m_LocksList.StoreScrollPos();
	m_LocksList.Clear();
	if (!m_LocksList.GetStatus(&m_pathList, false, false, false, false, true))
		m_LocksList.SetEmptyString(m_LocksList.GetLastErrorMessage());
	m_LocksList.Show(GITSLC_SHOWLFSLOCKS, GITSLC_SHOWLFSLOCKS);

	InterlockedExchange(&m_bThreadRunning, FALSE);
	RefreshCursor();

	DialogEnableWindow(IDC_SELECTALL, true);

	return 0;
}

void CLFSLocksDlg::OnBnClickedSelectall()
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
	m_LocksList.SelectAll(state == BST_CHECKED);
	theApp.DoWaitCursor(-1);
}

BOOL CLFSLocksDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case VK_F5:
		{
			if (!m_bThreadRunning)
			{
				if (AfxBeginThread(LocksThreadEntry, this) == nullptr)
					CMessageBox::Show(GetSafeHwnd(), IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
				else
					InterlockedExchange(&m_bThreadRunning, TRUE);
			}
		}
		break;
		}
	}

	return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}

LRESULT CLFSLocksDlg::OnStatusListCtrlNeedsRefresh(WPARAM, LPARAM)
{
	Refresh();
	return 0;
}

void CLFSLocksDlg::Refresh()
{
	if (AfxBeginThread(LocksThreadEntry, this) == nullptr)
		CMessageBox::Show(GetSafeHwnd(), IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
}

void CLFSLocksDlg::OnBnClickedUnLock()
{
	if (m_bThreadRunning)
		return;

	auto locker(m_LocksList.AcquireReadLock());

	CTGitPathList selectedPathList;
	for (int i = 0; i<m_LocksList.GetItemCount(); ++i)
	{
		if (m_LocksList.GetCheck(i))
		{
			selectedPathList.AddPath(*m_LocksList.GetListEntry(i));
		}
	}

	CGitProgressDlg progDlg(this);
	LFSSetLockedProgressCommand lfsLockCommand(false, false);
	progDlg.SetCommand(&lfsLockCommand);
	progDlg.SetItemCount(selectedPathList.GetCount());
	lfsLockCommand.SetPathList(selectedPathList);
	progDlg.DoModal();

	Refresh();
}

void CLFSLocksDlg::OnTimer(UINT_PTR nIDEvent)
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
			Refresh();
		}
		break;
	}
	__super::OnTimer(nIDEvent);
}
