// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2012 - TortoiseGit
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
#include "ChangedDlg.h"
#include "messagebox.h"
#include "cursor.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include ".\changeddlg.h"
#include "IconMenu.h"
#include "RefLogDlg.h"

#include "GitStatusListCtrl.h"

IMPLEMENT_DYNAMIC(CChangedDlg, CResizableStandAloneDialog)
CChangedDlg::CChangedDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CChangedDlg::IDD, pParent)
	, m_bShowUnversioned(FALSE)
	, m_iShowUnmodified(0)
	, m_bBlock(FALSE)
	, m_bCanceled(false)
	, m_bShowIgnored(FALSE)
	, m_bShowExternals(TRUE)
	, m_bShowUserProps(FALSE)
{
	m_bRemote = FALSE;
}

CChangedDlg::~CChangedDlg()
{
}

void CChangedDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CHANGEDLIST, m_FileListCtrl);
	DDX_Control(pDX, IDC_BUTTON_STASH, m_ctrlStash);
	DDX_Check(pDX, IDC_SHOWUNVERSIONED, m_bShowUnversioned);
	DDX_Check(pDX, IDC_SHOWUNMODIFIED, m_iShowUnmodified);
	DDX_Check(pDX, IDC_SHOWIGNORED, m_bShowIgnored);
//	DDX_Check(pDX, IDC_SHOWEXTERNALS, m_bShowExternals);
//	DDX_Check(pDX, IDC_SHOWUSERPROPS, m_bShowUserProps);
}


BEGIN_MESSAGE_MAP(CChangedDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_SHOWUNVERSIONED, OnBnClickedShowunversioned)
	ON_BN_CLICKED(IDC_SHOWUNMODIFIED, OnBnClickedShowUnmodified)
//	ON_BN_CLICKED(IDC_SHOWUSERPROPS, OnBnClickedShowUserProps)
	ON_REGISTERED_MESSAGE(CGitStatusListCtrl::GITSLNM_NEEDSREFRESH, OnSVNStatusListCtrlNeedsRefresh)
	ON_REGISTERED_MESSAGE(CGitStatusListCtrl::GITSLNM_ITEMCOUNTCHANGED, OnSVNStatusListCtrlItemCountChanged)
	ON_BN_CLICKED(IDC_SHOWIGNORED, &CChangedDlg::OnBnClickedShowignored)
	ON_BN_CLICKED(IDC_REFRESH, &CChangedDlg::OnBnClickedRefresh)
//	ON_BN_CLICKED(IDC_SHOWEXTERNALS, &CChangedDlg::OnBnClickedShowexternals)
	ON_BN_CLICKED(IDC_COMMIT, &CChangedDlg::OnBnClickedCommit)
	ON_BN_CLICKED(IDC_BUTTON_STASH, &CChangedDlg::OnBnClickedStash)
	ON_BN_CLICKED(IDC_BUTTON_UNIFIEDDIFF, &CChangedDlg::OnBnClickedButtonUnifieddiff)
END_MESSAGE_MAP()

BOOL CChangedDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	GetWindowText(m_sTitle);

	m_tooltips.Create(this);

	m_regAddBeforeCommit = CRegDWORD(_T("Software\\TortoiseGit\\AddBeforeCommit"), TRUE);
	m_bShowUnversioned = m_regAddBeforeCommit;
	UpdateData(FALSE);

	m_FileListCtrl.Init(GITSLC_COLEXT | GITSLC_COLSTATUS, _T("ChangedDlg"),
						(GITSLC_POPALL ^ (GITSLC_POPSAVEAS|GITSLC_POPRESTORE)), false);
	m_FileListCtrl.SetCancelBool(&m_bCanceled);
	m_FileListCtrl.SetBackgroundImage(IDI_CFM_BKG);
	m_FileListCtrl.SetEmptyString(IDS_REPOSTATUS_EMPTYFILELIST);

	AdjustControlSize(IDC_SHOWUNVERSIONED);
	AdjustControlSize(IDC_SHOWUNMODIFIED);
	AdjustControlSize(IDC_SHOWIGNORED);
//	AdjustControlSize(IDC_SHOWEXTERNALS);
//	AdjustControlSize(IDC_SHOWUSERPROPS);

	AddAnchor(IDC_CHANGEDLIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SUMMARYTEXT, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SHOWUNVERSIONED, BOTTOM_LEFT);
	AddAnchor(IDC_SHOWUNMODIFIED, BOTTOM_LEFT);
	AddAnchor(IDC_SHOWIGNORED, BOTTOM_LEFT);
//	AddAnchor(IDC_SHOWEXTERNALS, BOTTOM_LEFT);
//	AddAnchor(IDC_SHOWUSERPROPS, BOTTOM_LEFT);
	AddAnchor(IDC_INFOLABEL, BOTTOM_RIGHT);
	AddAnchor(IDC_BUTTON_STASH, BOTTOM_RIGHT);
	AddAnchor(IDC_BUTTON_UNIFIEDDIFF, BOTTOM_RIGHT);
	AddAnchor(IDC_COMMIT, BOTTOM_RIGHT);
	AddAnchor(IDC_REFRESH, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
//	SetPromptParentWindow(m_hWnd);
	if (hWndExplorer)
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	EnableSaveRestore(_T("ChangedDlg"));

	m_bRemote = !!(DWORD)CRegDWORD(_T("Software\\TortoiseGit\\CheckRepo"), FALSE);

	// first start a thread to obtain the status without
	// blocking the dialog
	if (AfxBeginThread(ChangedStatusThreadEntry, this)==NULL)
	{
		CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}

	return TRUE;
}

UINT CChangedDlg::ChangedStatusThreadEntry(LPVOID pVoid)
{
	return ((CChangedDlg*)pVoid)->ChangedStatusThread();
}

UINT CChangedDlg::ChangedStatusThread()
{
	InterlockedExchange(&m_bBlock, TRUE);

	g_Git.RefreshGitIndex();

	m_bCanceled = false;
	SetDlgItemText(IDOK, CString(MAKEINTRESOURCE(IDS_MSGBOX_CANCEL)));
	DialogEnableWindow(IDC_REFRESH, FALSE);
	DialogEnableWindow(IDC_SHOWUNVERSIONED, FALSE);
	DialogEnableWindow(IDC_SHOWUNMODIFIED, FALSE);
	DialogEnableWindow(IDC_SHOWIGNORED, FALSE);
	DialogEnableWindow(IDC_SHOWUSERPROPS, FALSE);
	CString temp;
	m_FileListCtrl.Clear();
	if (!m_FileListCtrl.GetStatus(&m_pathList, m_bRemote, m_bShowIgnored != FALSE, m_bShowUnversioned != FALSE,m_bShowUserProps != FALSE))
	{
		if (!m_FileListCtrl.GetLastErrorMessage().IsEmpty())
			m_FileListCtrl.SetEmptyString(m_FileListCtrl.GetLastErrorMessage());
	}
	unsigned int dwShow = GITSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALS | GITSLC_SHOWLOCKS | GITSLC_SHOWSWITCHED | GITSLC_SHOWINCHANGELIST;
	dwShow |= m_bShowUnversioned ? GITSLC_SHOWUNVERSIONED : 0;
	dwShow |= m_iShowUnmodified ? GITSLC_SHOWNORMAL : 0;
	dwShow |= m_bShowIgnored ? GITSLC_SHOWIGNORED : 0;
	dwShow |= m_bShowExternals ? GITSLC_SHOWEXTERNAL | GITSLC_SHOWINEXTERNALS | GITSLC_SHOWEXTERNALFROMDIFFERENTREPO : 0;
	m_FileListCtrl.Show(dwShow);
	UpdateStatistics();

	bool bIsDirectory = false;

	CTGitPath commonDir = m_FileListCtrl.GetCommonDirectory(false);
	if (m_pathList.GetCount() == 1)
	{
		if (m_pathList[0].IsEmpty())
			CAppUtils::SetWindowTitle(m_hWnd, g_Git.m_CurrentDir, m_sTitle);
		else
			CAppUtils::SetWindowTitle(m_hWnd, (g_Git.m_CurrentDir + _T("\\") + m_pathList[0].GetWinPathString()).TrimRight('\\'), m_sTitle);
		bIsDirectory = m_pathList[0].IsDirectory() || m_pathList[0].IsEmpty(); // if it is empty it is g_Git.m_CurrentDir which is a directory
	}
	else
		CAppUtils::SetWindowTitle(m_hWnd, commonDir.GetWinPathString(), m_sTitle);

	SetDlgItemText(IDOK, CString(MAKEINTRESOURCE(IDS_MSGBOX_OK)));
	DialogEnableWindow(IDC_REFRESH, TRUE);
	DialogEnableWindow(IDC_SHOWUNVERSIONED, bIsDirectory);
	//DialogEnableWindow(IDC_SHOWUNMODIFIED, bIsDirectory);
	DialogEnableWindow(IDC_SHOWIGNORED, bIsDirectory);
	DialogEnableWindow(IDC_SHOWUSERPROPS, TRUE);
	InterlockedExchange(&m_bBlock, FALSE);
	// revert the remote flag back to the default
	m_bRemote = !!(DWORD)CRegDWORD(_T("Software\\TortoiseGit\\CheckRepo"), FALSE);
	RefreshCursor();
	return 0;
}

void CChangedDlg::OnOK()
{
	if (m_bBlock)
	{
		m_bCanceled = true;
		return;
	}
	__super::OnOK();
}

void CChangedDlg::OnCancel()
{
	if (m_bBlock)
	{
		m_bCanceled = true;
		return;
	}
	__super::OnCancel();
}

DWORD CChangedDlg::UpdateShowFlags()
{
	DWORD dwShow = m_FileListCtrl.GetShowFlags();
	if (m_bShowUnversioned)
		dwShow |= GITSLC_SHOWUNVERSIONED;
	else
		dwShow &= ~GITSLC_SHOWUNVERSIONED;
	if (m_iShowUnmodified)
		dwShow |= GITSLC_SHOWNORMAL;
	else
		dwShow &= ~GITSLC_SHOWNORMAL;
	if (m_bShowIgnored)
		dwShow |= GITSLC_SHOWIGNORED;
	else
		dwShow &= ~GITSLC_SHOWIGNORED;
	if (m_bShowExternals)
		dwShow |= GITSLC_SHOWEXTERNAL | GITSLC_SHOWINEXTERNALS | GITSLC_SHOWEXTERNALFROMDIFFERENTREPO;
	else
		dwShow &= ~(GITSLC_SHOWEXTERNAL | GITSLC_SHOWINEXTERNALS | GITSLC_SHOWEXTERNALFROMDIFFERENTREPO);

	return dwShow;
}

void CChangedDlg::OnBnClickedShowunversioned()
{
	UpdateData();
	if(m_FileListCtrl.m_FileLoaded & CGitStatusListCtrl::FILELIST_UNVER)
	{
		m_FileListCtrl.Show(UpdateShowFlags());
		m_regAddBeforeCommit = m_bShowUnversioned;
	}
	else
	{
		if(m_bShowUnversioned)
		{
			if (AfxBeginThread(ChangedStatusThreadEntry, this)==NULL)
			{
				CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
			}
		}
	}
	UpdateStatistics();
}

void CChangedDlg::OnBnClickedShowUnmodified()
{
	UpdateData();
	m_FileListCtrl.Show(UpdateShowFlags());
	m_regAddBeforeCommit = m_bShowUnversioned;
	UpdateStatistics();
}

void CChangedDlg::OnBnClickedShowignored()
{
	UpdateData();
	if (AfxBeginThread(ChangedStatusThreadEntry, this)==NULL)
	{
		CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
}

void CChangedDlg::OnBnClickedShowexternals()
{
	UpdateData();
	m_FileListCtrl.Show(UpdateShowFlags());
	UpdateStatistics();
}

void CChangedDlg::OnBnClickedShowUserProps()
{
	UpdateData();
	if (AfxBeginThread(ChangedStatusThreadEntry, this)==NULL)
	{
		CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
}

LRESULT CChangedDlg::OnSVNStatusListCtrlNeedsRefresh(WPARAM, LPARAM)
{
	if (AfxBeginThread(ChangedStatusThreadEntry, this)==NULL)
	{
		CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	return 0;
}

LRESULT CChangedDlg::OnSVNStatusListCtrlItemCountChanged(WPARAM, LPARAM)
{
	UpdateStatistics();
	return 0;
}

BOOL CChangedDlg::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case VK_F5:
			{
				if (m_bBlock)
					return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
				if (AfxBeginThread(ChangedStatusThreadEntry, this)==NULL)
				{
					CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
				}
			}
			break;
		}
	}

	return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}

void CChangedDlg::OnBnClickedRefresh()
{
	if (!m_bBlock)
	{
		if (AfxBeginThread(ChangedStatusThreadEntry, this)==NULL)
		{
			CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
		}
	}
}

void CChangedDlg::UpdateStatistics()
{
	CString temp;
#if 0
	LONG lMin, lMax;

	m_FileListCtrl.GetMinMaxRevisions(lMin, lMax, true, false);
	if (LONG(m_FileListCtrl.m_HeadRev) >= 0)
	{
		temp.Format(IDS_REPOSTATUS_HEADREV, lMin, lMax, LONG(m_FileListCtrl.m_HeadRev));
		SetDlgItemText(IDC_SUMMARYTEXT, temp);
	}
	else
	{
		temp.Format(IDS_REPOSTATUS_WCINFO, lMin, lMax);
		SetDlgItemText(IDC_SUMMARYTEXT, temp);
	}
#endif
	temp = m_FileListCtrl.GetStatisticsString();
	temp.Replace(_T(" = "), _T("="));
	temp.Replace(_T("\n"), _T(", "));
	SetDlgItemText(IDC_INFOLABEL, temp);

}

void CChangedDlg::OnBnClickedCommit()
{
	CString cmd = _T("/command:commit /path:\"");
	bool bSingleFile = ((m_pathList.GetCount()==1)&&(!m_pathList[0].IsEmpty())&&(!m_pathList[0].IsDirectory()));
	if (bSingleFile)
		cmd += m_pathList[0].GetWinPathString();
	else
		cmd += g_Git.m_CurrentDir + _T("\\") + m_pathList.GetCommonRoot().GetDirectory().GetWinPath();
	cmd += _T("\"");
	CAppUtils::RunTortoiseProc(cmd);
}

void CChangedDlg::OnBnClickedStash()
{
	CIconMenu popup;

	if (popup.CreatePopupMenu())
	{
		popup.AppendMenuIcon(ID_STASH_SAVE, IDS_MENUSTASHSAVE, IDI_COMMIT);

		CTGitPath root = g_Git.m_CurrentDir;
		if (root.HasStashDir())
		{
			popup.AppendMenuIcon(ID_STASH_POP, IDS_MENUSTASHPOP, IDI_RELOCATE);
			popup.AppendMenuIcon(ID_STASH_APPLY, IDS_MENUSTASHAPPLY, IDI_RELOCATE);
			popup.AppendMenuIcon(ID_STASH_LIST, IDS_MENUSTASHLIST, IDI_LOG);
		}

		POINT cursorPos;
		GetCursorPos(&cursorPos);
		int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, cursorPos.x, cursorPos.y, this, 0);

		switch (cmd & 0xFFFF)
		{
		case ID_STASH_SAVE:
			CAppUtils::StashSave();
			break;
		case ID_STASH_POP:
			CAppUtils::StashPop(false);
			return;
		case ID_STASH_APPLY:
			CAppUtils::StashApply(_T(""), false);
			break;
		case ID_STASH_LIST:
			{
				CRefLogDlg dlg;
				dlg.m_CurrentBranch = _T("refs/stash");
				dlg.DoModal();
			}
			break;
		default:
			return;
		}
		OnBnClickedRefresh();
	}
}

void CChangedDlg::OnBnClickedButtonUnifieddiff()
{
	CTGitPath commonDirectory = m_pathList.GetCommonRoot().GetDirectory();
	bool bSingleFile = ((m_pathList.GetCount()==1)&&(!m_pathList[0].IsEmpty())&&(!m_pathList[0].IsDirectory()));
	if (bSingleFile)
		commonDirectory = m_pathList[0];
	CAppUtils::StartShowUnifiedDiff(m_hWnd, commonDirectory, GitRev::GetHead(), commonDirectory, GitRev::GetWorkingCopy());
}
