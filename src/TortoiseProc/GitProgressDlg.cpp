// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2013 - TortoiseGit
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
#include "GITProgressDlg.h"
#include "LogDlg.h"
#include "TGitPath.h"
#include "registry.h"
#include "GitStatus.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "StringUtils.h"
#include "TempFile.h"
#include "UnicodeUtils.h"
#include "SoundUtils.h"
#include "GitDiff.h"
#include "Hooks.h"
#include "DropFiles.h"
//#include "GitLogHelper.h"
#include "RegHistory.h"
//#include "ConflictResolveDlg.h"
#include "LogFile.h"
#include "ShellUpdater.h"
#include "IconMenu.h"
#include "BugTraqAssociations.h"
#include "patch.h"
#include "MassiveGitTask.h"
#include "SmartHandle.h"


IMPLEMENT_DYNAMIC(CGitProgressDlg, CResizableStandAloneDialog)
CGitProgressDlg::CGitProgressDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CGitProgressDlg::IDD, pParent)

#if 0
	, m_Revision(_T("HEAD"))
	//, m_RevisionEnd(0)
	, m_bLockWarning(false)
	, m_bLockExists(false)
	, m_bThreadRunning(FALSE)
	, m_nConflicts(0)
	, m_bMergesAddsDeletesOccurred(FALSE)
	, m_dwCloseOnEnd((DWORD)-1)
	, m_bFinishedItemAdded(false)
	, m_bLastVisible(false)
//	, m_depth(svn_depth_unknown)
	, m_itemCount(-1)
	, m_itemCountTotal(-1)
	, m_AlwaysConflicted(false)
	, m_BugTraqProvider(NULL)
	, sDryRun(MAKEINTRESOURCE(IDS_PROGRS_DRYRUN))
	, sRecordOnly(MAKEINTRESOURCE(IDS_MERGE_RECORDONLY))
#endif
{
}

CGitProgressDlg::~CGitProgressDlg()
{
}

void CGitProgressDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SVNPROGRESS, m_ProgList);
	DDX_Control(pDX, IDC_TITLE_ANIMATE, m_Animate);
	DDX_Control(pDX, IDC_PROGRESSBAR, m_ProgCtrl);
	DDX_Control(pDX, IDC_INFOTEXT, m_InfoCtrl);
	DDX_Control(pDX, IDC_PROGRESSLABEL, m_ProgLableCtrl);
}

BEGIN_MESSAGE_MAP(CGitProgressDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_LOGBUTTON, OnBnClickedLogbutton)
	ON_WM_CLOSE()
	ON_WM_SETCURSOR()
	ON_EN_SETFOCUS(IDC_INFOTEXT, &CGitProgressDlg::OnEnSetfocusInfotext)
	ON_BN_CLICKED(IDC_NONINTERACTIVE, &CGitProgressDlg::OnBnClickedNoninteractive)
	ON_WM_CTLCOLOR()
	ON_MESSAGE(WM_PROG_CMD_FINISH, OnCmdEnd)
	ON_MESSAGE(WM_PROG_CMD_START, OnCmdStart)
END_MESSAGE_MAP()



BOOL CGitProgressDlg::OnInitDialog()
{
	__super::OnInitDialog();


	UpdateData(FALSE);

	AddAnchor(IDC_SVNPROGRESS, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_TITLE_ANIMATE, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_PROGRESSLABEL, BOTTOM_LEFT, BOTTOM_CENTER);
	AddAnchor(IDC_PROGRESSBAR, BOTTOM_CENTER, BOTTOM_RIGHT);
	AddAnchor(IDC_INFOTEXT, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_NONINTERACTIVE, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDC_LOGBUTTON, BOTTOM_RIGHT);
	//SetPromptParentWindow(this->m_hWnd);

	m_Animate.Open(IDR_DOWNLOAD);
	m_ProgList.m_pAnimate = &m_Animate;
	m_ProgList.m_pProgControl = &m_ProgCtrl;
	m_ProgList.m_pProgressLabelCtrl = &m_ProgLableCtrl;
	m_ProgList.m_pInfoCtrl = &m_InfoCtrl;
	m_ProgList.m_pPostWnd = this;
	m_ProgList.m_bSetTitle = true;

	if (hWndExplorer)
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	EnableSaveRestore(_T("GITProgressDlg"));

	m_background_brush.CreateSolidBrush(GetSysColor(COLOR_WINDOW));
	m_ProgList.Init();

	return TRUE;
}



void CGitProgressDlg::OnBnClickedLogbutton()
{
	switch(m_ProgList.m_Command)
	{
	case CGitProgressList::GitProgress_Add:
	case CGitProgressList::GitProgress_Resolve:
		{
			CString cmd = _T(" /command:commit");
			cmd += _T(" /path:\"")+g_Git.m_CurrentDir+_T("\"");

			CAppUtils::RunTortoiseGitProc(cmd);
			this->EndDialog(IDOK);
			break;
		}
	}
#if 0
	if (m_targetPathList.GetCount() != 1)
		return;
	StringRevMap::iterator it = m_UpdateStartRevMap.begin();
	svn_revnum_t rev = -1;
	if (it != m_UpdateStartRevMap.end())
	{
		rev = it->second;
	}
	CLogDlg dlg;
	dlg.SetParams(m_targetPathList[0], m_RevisionEnd, m_RevisionEnd, rev, 0, TRUE);
	dlg.DoModal();
#endif
}


void CGitProgressDlg::OnClose()
{
	DialogEnableWindow(IDCANCEL, TRUE);
	__super::OnClose();
}

void CGitProgressDlg::OnOK()
{
	if ((m_ProgList.IsCancelled())&&(!m_ProgList.IsRunning()))
	{
		// I have made this wait a sensible amount of time (10 seconds) for the thread to finish
		// You must be careful in the thread that after posting the WM_COMMAND/IDOK message, you
		// don't do any more operations on the window which might require message passing
		// If you try to send windows messages once we're waiting here, then the thread can't finished
		// because the Window's message loop is blocked at this wait
		WaitForSingleObject(m_ProgList.m_pThread->m_hThread, 10000);
		__super::OnOK();
	}
	m_ProgList.Cancel();
}

void CGitProgressDlg::OnCancel()
{
	if ((m_ProgList.IsCancelled())&&(!m_ProgList.IsRunning()))
		__super::OnCancel();
	m_ProgList.Cancel();
}


BOOL CGitProgressDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (!GetDlgItem(IDOK)->IsWindowEnabled())
	{
		// only show the wait cursor over the list control
		if ((pWnd)&&(pWnd == GetDlgItem(IDC_SVNPROGRESS)))
		{
			HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_WAIT));
			SetCursor(hCur);
			return TRUE;
		}
	}
	HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
	SetCursor(hCur);
	return CResizableStandAloneDialog::OnSetCursor(pWnd, nHitTest, message);
}

BOOL CGitProgressDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		if (pMsg->wParam == VK_ESCAPE)
		{
			// pressing the ESC key should close the dialog. But since we disabled the escape
			// key (so the user doesn't get the idea that he could simply undo an e.g. update)
			// this won't work.
			// So if the user presses the ESC key, change it to VK_RETURN so the dialog gets
			// the impression that the OK button was pressed.
			if ((!m_ProgList.IsRunning())&&(!GetDlgItem(IDCANCEL)->IsWindowEnabled())
				&&(GetDlgItem(IDOK)->IsWindowEnabled())&&(GetDlgItem(IDOK)->IsWindowVisible()))
			{
				// since we convert ESC to RETURN, make sure the OK button has the focus.
				GetDlgItem(IDOK)->SetFocus();
				pMsg->wParam = VK_RETURN;
			}
		}
	}
	return __super::PreTranslateMessage(pMsg);
}



void CGitProgressDlg::OnEnSetfocusInfotext()
{
	CString sTemp;
	GetDlgItemText(IDC_INFOTEXT, sTemp);
	if (sTemp.IsEmpty())
		GetDlgItem(IDC_INFOTEXT)->HideCaret();
}


void CGitProgressDlg::OnBnClickedNoninteractive()
{
	LRESULT res = ::SendMessage(GetDlgItem(IDC_NONINTERACTIVE)->GetSafeHwnd(), BM_GETCHECK, 0, 0);
	m_ProgList.m_AlwaysConflicted = (res == BST_CHECKED);
	CRegDWORD nonint = CRegDWORD(_T("Software\\TortoiseGit\\MergeNonInteractive"), FALSE);
	nonint = m_ProgList.m_AlwaysConflicted;
}

LRESULT CGitProgressDlg::OnCtlColorStatic(WPARAM wParam, LPARAM lParam)
{
	HDC hDC = (HDC)wParam;
	HWND hwndCtl = (HWND)lParam;

	if (::GetDlgCtrlID(hwndCtl) == IDC_TITLE_ANIMATE)
	{
		CDC *pDC = CDC::FromHandle(hDC);
		pDC->SetBkColor(GetSysColor(COLOR_WINDOW));
		pDC->SetBkMode(TRANSPARENT);
		return (LRESULT)(HBRUSH)m_background_brush.GetSafeHandle();
	}
	return FALSE;
}

HBRUSH CGitProgressDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr;
	if (pWnd->GetDlgCtrlID() == IDC_TITLE_ANIMATE)
	{
		pDC->SetBkColor(GetSysColor(COLOR_WINDOW)); // add this
		pDC->SetBkMode(TRANSPARENT);
		return (HBRUSH)m_background_brush.GetSafeHandle();
	}
	else
		hbr = CResizableStandAloneDialog::OnCtlColor(pDC, pWnd, nCtlColor);

	// TODO:  Return a different brush if the default is not desired
	return hbr;
}

LRESULT	CGitProgressDlg::OnCmdEnd(WPARAM wParam, LPARAM /*lParam*/)
{
	DialogEnableWindow(IDCANCEL, FALSE);
	DialogEnableWindow(IDOK, TRUE);

	switch(wParam)
	{
	case CGitProgressList::GitProgress_Add:
	case CGitProgressList::GitProgress_Revert:
		this->GetDlgItem(IDC_LOGBUTTON)->SetWindowText(CString(MAKEINTRESOURCE(IDS_COMMITBUTTON)));
		this->GetDlgItem(IDC_LOGBUTTON)->ShowWindow(SW_SHOW);
		break;
	}

	CWnd * pWndOk = GetDlgItem(IDOK);
	if (pWndOk && ::IsWindow(pWndOk->GetSafeHwnd()))
	{
		SendMessage(DM_SETDEFID, IDOK);
		GetDlgItem(IDOK)->SetFocus();
	}

	return 0;
}
LRESULT	CGitProgressDlg::OnCmdStart(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	DialogEnableWindow(IDOK, FALSE);
	DialogEnableWindow(IDCANCEL, TRUE);

	return 0;
}
