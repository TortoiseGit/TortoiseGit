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
#include "UnlockDlg.h"
#include "messagebox.h"


#define REFRESHTIMER   100

IMPLEMENT_DYNAMIC(CUnlockDlg, CResizableStandAloneDialog)

CUnlockDlg::CUnlockDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CUnlockDlg::IDD, pParent)
	, m_bThreadRunning(FALSE)
	, m_bCancelled(false)
{

}

CUnlockDlg::~CUnlockDlg()
{
}

void CUnlockDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_UNLOCKLIST, m_unlockListCtrl);
	DDX_Control(pDX, IDC_SELECTALL, m_SelectAll);
}


BEGIN_MESSAGE_MAP(CUnlockDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_SELECTALL, OnBnClickedSelectall)
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
	ON_REGISTERED_MESSAGE(CSVNStatusListCtrl::SVNSLNM_NEEDSREFRESH, OnSVNStatusListCtrlNeedsRefresh)
	ON_REGISTERED_MESSAGE(CSVNStatusListCtrl::SVNSLNM_ADDFILE, OnFileDropped)
	ON_WM_TIMER()
END_MESSAGE_MAP()


BOOL CUnlockDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	// initialize the svn status list control
	m_unlockListCtrl.Init(0, _T("UnlockDlg"), SVNSLC_POPALL);
	m_unlockListCtrl.SetIgnoreRemoveOnly();	// when ignoring, don't add the parent folder since we're in the unlock dialog
	m_unlockListCtrl.SetSelectButton(&m_SelectAll);
	m_unlockListCtrl.SetConfirmButton((CButton*)GetDlgItem(IDOK));
	m_unlockListCtrl.SetEmptyString(IDS_ERR_NOTHINGTOUNLOCK);
	m_unlockListCtrl.SetCancelBool(&m_bCancelled);
	m_unlockListCtrl.SetBackgroundImage(IDI_UNLOCK_BKG);
	m_unlockListCtrl.EnableFileDrop();

	AdjustControlSize(IDC_SELECTALL);

	AddAnchor(IDC_UNLOCKLIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SELECTALL, BOTTOM_LEFT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	if (hWndExplorer)
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	EnableSaveRestore(_T("UnlockDlg"));

	//first start a thread to obtain the file list with the status without
	//blocking the dialog
	if(AfxBeginThread(UnlockThreadEntry, this) == NULL)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	InterlockedExchange(&m_bThreadRunning, TRUE);

	return TRUE;
}

void CUnlockDlg::OnOK()
{
	if (m_bThreadRunning)
		return;

	// save only the files the user has selected into the path list
	m_unlockListCtrl.WriteCheckedNamesToPathList(m_pathList);

	CResizableStandAloneDialog::OnOK();
}

void CUnlockDlg::OnCancel()
{
	m_bCancelled = true;
	if (m_bThreadRunning)
		return;

	CResizableStandAloneDialog::OnCancel();
}

void CUnlockDlg::OnBnClickedSelectall()
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
	m_unlockListCtrl.SelectAll(state == BST_CHECKED);
	theApp.DoWaitCursor(-1);
}

UINT CUnlockDlg::UnlockThreadEntry(LPVOID pVoid)
{
	return ((CUnlockDlg*)pVoid)->UnlockThread();
}

UINT CUnlockDlg::UnlockThread()
{
	// get the status of all selected file/folders recursively
	// and show the ones which the user can add (i.e. the unversioned ones)
	DialogEnableWindow(IDOK, false);
	m_bCancelled = false;
	if (!m_unlockListCtrl.GetStatus(m_pathList))
	{
		m_unlockListCtrl.SetEmptyString(m_unlockListCtrl.GetLastErrorMessage());
	}
	m_unlockListCtrl.Show(SVNSLC_SHOWLOCKS | SVNSLC_SHOWDIRECTFILES, 
		SVNSLC_SHOWLOCKS | SVNSLC_SHOWDIRECTFILES);

	InterlockedExchange(&m_bThreadRunning, FALSE);
	return 0;
}

void CUnlockDlg::OnBnClickedHelp()
{
	OnHelp();
}

BOOL CUnlockDlg::PreTranslateMessage(MSG* pMsg)
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
					if(AfxBeginThread(UnlockThreadEntry, this) == NULL)
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

LRESULT CUnlockDlg::OnSVNStatusListCtrlNeedsRefresh(WPARAM, LPARAM)
{
	if(AfxBeginThread(UnlockThreadEntry, this) == NULL)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	return 0;
}

LRESULT CUnlockDlg::OnFileDropped(WPARAM, LPARAM lParam)
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
	CTSVNPath path;
	path.SetFromWin((LPCTSTR)lParam);

	if (!m_unlockListCtrl.HasPath(path))
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

void CUnlockDlg::OnTimer(UINT_PTR nIDEvent)
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
