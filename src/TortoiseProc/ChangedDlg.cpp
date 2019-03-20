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
#include "ChangedDlg.h"
#include "MessageBox.h"
#include "cursor.h"
#include "AppUtils.h"
#include "ChangedDlg.h"
#include "IconMenu.h"
#include "RefLogDlg.h"

IMPLEMENT_DYNAMIC(CChangedDlg, CResizableStandAloneDialog)
CChangedDlg::CChangedDlg(CWnd* pParent /*=nullptr*/)
	: CResizableStandAloneDialog(CChangedDlg::IDD, pParent)
	, m_bShowUnversioned(FALSE)
	, m_iShowUnmodified(0)
	, m_bBlock(FALSE)
	, m_bCanceled(false)
	, m_bShowIgnored(FALSE)
	, m_bShowLocalChangesIgnored(FALSE)
	, m_bWholeProject(FALSE)
	, m_bRemote(FALSE)
	, m_bShowStaged(TRUE)
{
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
	DDX_Check(pDX, IDC_SHOWLOCALCHANGESIGNORED, m_bShowLocalChangesIgnored);
	DDX_Check(pDX, IDC_WHOLE_PROJECT, m_bWholeProject);
	DDX_Check(pDX, IDC_SHOWSTAGED, m_bShowStaged);
}


BEGIN_MESSAGE_MAP(CChangedDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_SHOWUNVERSIONED, OnBnClickedShowunversioned)
	ON_BN_CLICKED(IDC_SHOWUNMODIFIED, OnBnClickedShowUnmodified)
	ON_REGISTERED_MESSAGE(CGitStatusListCtrl::GITSLNM_NEEDSREFRESH, OnSVNStatusListCtrlNeedsRefresh)
	ON_REGISTERED_MESSAGE(CGitStatusListCtrl::GITSLNM_ITEMCOUNTCHANGED, OnSVNStatusListCtrlItemCountChanged)
	ON_BN_CLICKED(IDC_SHOWIGNORED, &CChangedDlg::OnBnClickedShowignored)
	ON_BN_CLICKED(IDC_REFRESH, &CChangedDlg::OnBnClickedRefresh)
	ON_BN_CLICKED(IDC_COMMIT, &CChangedDlg::OnBnClickedCommit)
	ON_BN_CLICKED(IDC_BUTTON_STASH, &CChangedDlg::OnBnClickedStash)
	ON_BN_CLICKED(IDC_BUTTON_UNIFIEDDIFF, &CChangedDlg::OnBnClickedButtonUnifieddiff)
	ON_BN_CLICKED(IDC_SHOWLOCALCHANGESIGNORED, &CChangedDlg::OnBnClickedShowlocalchangesignored)
	ON_BN_CLICKED(IDC_WHOLE_PROJECT, OnBnClickedWholeProject)
	ON_BN_CLICKED(IDC_SHOWSTAGED, OnBnClickedShowStaged)
END_MESSAGE_MAP()

BOOL CChangedDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	m_regAddBeforeCommit = CRegDWORD(L"Software\\TortoiseGit\\AddBeforeCommit", TRUE);
	m_bShowUnversioned = m_regAddBeforeCommit;

	CString regPath(g_Git.m_CurrentDir);
	regPath.Replace(L':', L'_');
	m_regShowWholeProject = CRegDWORD(L"Software\\TortoiseGit\\TortoiseProc\\ShowWholeProject\\" + regPath, FALSE);
	m_bWholeProject = m_regShowWholeProject;
	m_regShowStaged = CRegDWORD(L"Software\\TortoiseGit\\TortoiseProc\\ChangedFilesIncludeStaged", TRUE);
	m_bShowStaged = m_regShowStaged;
	SetDlgTitle();

	if (m_pathList.GetCount() == 1 && m_pathList[0].GetWinPathString().IsEmpty())
	{
		m_bWholeProject = BST_CHECKED;
		DialogEnableWindow(IDC_WHOLE_PROJECT, FALSE);
	}

	UpdateData(FALSE);

	m_FileListCtrl.Init(GITSLC_COLEXT | GITSLC_COLSTATUS | GITSLC_COLADD | GITSLC_COLDEL | GITSLC_COLMODIFICATIONDATE, L"ChangedDlg", (GITSLC_POPALL ^ (GITSLC_POPSAVEAS | GITSLC_POPRESTORE | GITSLC_POPPREPAREDIFF | GITSLC_POPCHANGELISTS)), false);
	m_FileListCtrl.SetCancelBool(&m_bCanceled);
	m_FileListCtrl.SetBackgroundImage(IDI_CFM_BKG);
	m_FileListCtrl.SetEmptyString(IDS_REPOSTATUS_EMPTYFILELIST);

	AdjustControlSize(IDC_SHOWUNVERSIONED);
	AdjustControlSize(IDC_SHOWUNMODIFIED);
	AdjustControlSize(IDC_SHOWLOCALCHANGESIGNORED);
	AdjustControlSize(IDC_SHOWIGNORED);
	AdjustControlSize(IDC_WHOLE_PROJECT);
	AdjustControlSize(IDC_SHOWSTAGED);

	AddAnchor(IDC_CHANGEDLIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SUMMARYTEXT, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SHOWUNVERSIONED, BOTTOM_LEFT);
	AddAnchor(IDC_SHOWUNMODIFIED, BOTTOM_LEFT);
	AddAnchor(IDC_SHOWLOCALCHANGESIGNORED, BOTTOM_LEFT);
	AddAnchor(IDC_SHOWIGNORED, BOTTOM_LEFT);
	AddAnchor(IDC_WHOLE_PROJECT, BOTTOM_LEFT);
	AddAnchor(IDC_SHOWSTAGED, BOTTOM_LEFT);
	AddAnchor(IDC_INFOLABEL, BOTTOM_RIGHT);
	AddAnchor(IDC_BUTTON_STASH, BOTTOM_RIGHT);
	AddAnchor(IDC_BUTTON_UNIFIEDDIFF, BOTTOM_RIGHT);
	AddAnchor(IDC_COMMIT, BOTTOM_RIGHT);
	AddAnchor(IDC_REFRESH, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
//	SetPromptParentWindow(m_hWnd);
	if (GetExplorerHWND())
		CenterWindow(CWnd::FromHandle(GetExplorerHWND()));
	EnableSaveRestore(L"ChangedDlg");

	m_ctrlStash.m_bAlwaysShowArrow = true;

	m_bRemote = !!static_cast<DWORD>(CRegDWORD(L"Software\\TortoiseGit\\CheckRepo", FALSE));

	// first start a thread to obtain the status without
	// blocking the dialog
	OnBnClickedRefresh();

	return TRUE;
}

UINT CChangedDlg::ChangedStatusThreadEntry(LPVOID pVoid)
{
	return static_cast<CChangedDlg*>(pVoid)->ChangedStatusThread();
}

UINT CChangedDlg::ChangedStatusThread()
{
	m_bCanceled = false;
	SetDlgItemText(IDOK, CString(MAKEINTRESOURCE(IDS_MSGBOX_CANCEL)));
	DialogEnableWindow(IDC_REFRESH, FALSE);
	DialogEnableWindow(IDC_SHOWUNVERSIONED, FALSE);
	DialogEnableWindow(IDC_SHOWUNMODIFIED, FALSE);
	DialogEnableWindow(IDC_SHOWIGNORED, FALSE);
	DialogEnableWindow(IDC_SHOWLOCALCHANGESIGNORED, FALSE);
	DialogEnableWindow(IDC_SHOWSTAGED, FALSE);

	g_Git.RefreshGitIndex();

	m_FileListCtrl.StoreScrollPos();
	m_FileListCtrl.Clear();
	m_FileListCtrl.m_bIncludedStaged = (m_bShowStaged == TRUE);
	if (!m_FileListCtrl.GetStatus(m_bWholeProject ? nullptr : &m_pathList, m_bRemote, m_bShowIgnored != FALSE, m_bShowUnversioned != FALSE, m_bShowLocalChangesIgnored != FALSE))
	{
		if (!m_FileListCtrl.GetLastErrorMessage().IsEmpty())
			m_FileListCtrl.SetEmptyString(m_FileListCtrl.GetLastErrorMessage());
	}
	unsigned int dwShow = GITSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALS | GITSLC_SHOWSWITCHED | GITSLC_SHOWINCHANGELIST;
	dwShow |= m_bShowUnversioned ? GITSLC_SHOWUNVERSIONED : 0;
	dwShow |= m_iShowUnmodified ? GITSLC_SHOWNORMAL : 0;
	dwShow |= m_bShowIgnored ? GITSLC_SHOWIGNORED : 0;
	dwShow |= m_bShowLocalChangesIgnored ? GITSLC_SHOWASSUMEVALID | GITSLC_SHOWSKIPWORKTREE : 0;
	m_FileListCtrl.Show(dwShow);
	UpdateStatistics();

	SetDlgTitle();
	bool bIsDirectory = false;
	if (m_pathList.GetCount() == 1)
		bIsDirectory = m_pathList[0].IsDirectory() || m_pathList[0].IsEmpty(); // if it is empty it is g_Git.m_CurrentDir which is a directory

	SetDlgItemText(IDOK, CString(MAKEINTRESOURCE(IDS_MSGBOX_OK)));
	DialogEnableWindow(IDC_REFRESH, TRUE);
	DialogEnableWindow(IDC_SHOWUNVERSIONED, bIsDirectory);
	//DialogEnableWindow(IDC_SHOWUNMODIFIED, bIsDirectory);
	DialogEnableWindow(IDC_SHOWIGNORED, bIsDirectory);
	DialogEnableWindow(IDC_SHOWLOCALCHANGESIGNORED, TRUE);
	DialogEnableWindow(IDC_SHOWSTAGED, TRUE);
	InterlockedExchange(&m_bBlock, FALSE);
	// revert the remote flag back to the default
	m_bRemote = !!static_cast<DWORD>(CRegDWORD(L"Software\\TortoiseGit\\CheckRepo", FALSE));
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
	if (m_bShowLocalChangesIgnored)
		dwShow |= GITSLC_SHOWASSUMEVALID|GITSLC_SHOWSKIPWORKTREE;
	else
		dwShow &= ~(GITSLC_SHOWASSUMEVALID|GITSLC_SHOWSKIPWORKTREE);

	// old bShowExternals:
	dwShow &= ~(GITSLC_SHOWEXTERNAL | GITSLC_SHOWINEXTERNALS | GITSLC_SHOWEXTERNALFROMDIFFERENTREPO);

	return dwShow;
}

void CChangedDlg::SetDlgTitle()
{
	if (m_sTitle.IsEmpty())
		GetWindowText(m_sTitle);

	if (m_bWholeProject)
		CAppUtils::SetWindowTitle(m_hWnd, g_Git.m_CurrentDir, m_sTitle);
	else
	{
		if (m_pathList.GetCount() == 1)
			CAppUtils::SetWindowTitle(m_hWnd, g_Git.CombinePath(m_pathList[0].GetUIPathString()), m_sTitle);
		else
			CAppUtils::SetWindowTitle(m_hWnd, g_Git.CombinePath(m_FileListCtrl.GetCommonDirectory(false)), m_sTitle);
	}
}

void CChangedDlg::OnBnClickedShowunversioned()
{
	UpdateData();
	m_regAddBeforeCommit = m_bShowUnversioned;
	if(m_FileListCtrl.m_FileLoaded & CGitStatusListCtrl::FILELIST_UNVER)
	{
		m_FileListCtrl.StoreScrollPos();
		m_FileListCtrl.Show(UpdateShowFlags());
	}
	else
	{
		if(m_bShowUnversioned)
			OnBnClickedRefresh();
	}
	UpdateStatistics();
}

void CChangedDlg::OnBnClickedShowUnmodified()
{
	UpdateData();
	m_FileListCtrl.StoreScrollPos();
	m_FileListCtrl.Show(UpdateShowFlags());
	m_regAddBeforeCommit = m_bShowUnversioned;
	UpdateStatistics();
}

void CChangedDlg::OnBnClickedShowignored()
{
	UpdateData();
	m_FileListCtrl.StoreScrollPos();
	if (m_FileListCtrl.m_FileLoaded & CGitStatusListCtrl::FILELIST_IGNORE)
		m_FileListCtrl.Show(UpdateShowFlags());
	else if (m_bShowIgnored)
		OnBnClickedRefresh();
	UpdateStatistics();
}

void CChangedDlg::OnBnClickedShowlocalchangesignored()
{
	UpdateData();
	m_FileListCtrl.StoreScrollPos();
	if (m_FileListCtrl.m_FileLoaded & CGitStatusListCtrl::FILELIST_LOCALCHANGESIGNORED)
		m_FileListCtrl.Show(UpdateShowFlags());
	else if (m_bShowLocalChangesIgnored)
		OnBnClickedRefresh();
	UpdateStatistics();
}

LRESULT CChangedDlg::OnSVNStatusListCtrlNeedsRefresh(WPARAM, LPARAM)
{
	OnBnClickedRefresh();
	return 0;
}

LRESULT CChangedDlg::OnSVNStatusListCtrlItemCountChanged(WPARAM, LPARAM)
{
	UpdateStatistics();
	return 0;
}

BOOL CChangedDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case VK_F5:
			OnBnClickedRefresh();
			break;
		}
	}

	return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}

void CChangedDlg::OnBnClickedRefresh()
{
	if (InterlockedExchange(&m_bBlock, TRUE))
		return;

	if (!AfxBeginThread(ChangedStatusThreadEntry, this))
	{
		InterlockedExchange(&m_bBlock, FALSE);
		CMessageBox::Show(GetSafeHwnd(), IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
}

void CChangedDlg::UpdateStatistics()
{
	CString temp = m_FileListCtrl.GetStatisticsString();
	temp.Replace(L" = ", L"=");
	temp.Replace(L"\n", L", ");
	SetDlgItemText(IDC_INFOLABEL, temp);
}

void CChangedDlg::OnBnClickedCommit()
{
	CTGitPathList pathList;
	bool bSingleFile = (m_pathList.GetCount() >= 1 && !m_pathList[0].IsEmpty() && !m_pathList[0].IsDirectory());
	if (bSingleFile)
		pathList.AddPath(m_pathList[0]);
	else
		pathList.AddPath(m_pathList.GetCommonRoot().GetDirectory());

	bool bSelectFilesForCommit = !!DWORD(CRegStdDWORD(L"Software\\TortoiseGit\\SelectFilesForCommit", TRUE));

	CString logmsg;
	CAppUtils::Commit(GetSafeHwnd(),
		L"",
		m_bWholeProject,
		logmsg,
		pathList,
		bSelectFilesForCommit);

	OnBnClickedRefresh();
}

void CChangedDlg::OnBnClickedStash()
{
	CIconMenu popup;

	if (popup.CreatePopupMenu())
	{
		popup.AppendMenuIcon(ID_STASH_SAVE, IDS_MENUSTASHSAVE, IDI_SHELVE);

		CTGitPath root = g_Git.m_CurrentDir;
		if (root.HasStashDir())
		{
			popup.AppendMenuIcon(ID_STASH_POP, IDS_MENUSTASHPOP, IDI_UNSHELVE);
			popup.AppendMenuIcon(ID_STASH_APPLY, IDS_MENUSTASHAPPLY, IDI_UNSHELVE);
			popup.AppendMenuIcon(ID_STASH_LIST, IDS_MENUSTASHLIST, IDI_LOG);
		}

		RECT rect;
		GetDlgItem(IDC_BUTTON_STASH)->GetWindowRect(&rect);
		TPMPARAMS params;
		params.cbSize = sizeof(TPMPARAMS);
		params.rcExclude = rect;
		int cmd = popup.TrackPopupMenuEx(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY | TPM_VERTICAL, rect.left, rect.top, this, &params);

		switch (cmd & 0xFFFF)
		{
		case ID_STASH_SAVE:
			CAppUtils::StashSave(GetSafeHwnd());
			break;
		case ID_STASH_POP:
			CAppUtils::StashPop(GetSafeHwnd(), 2);
			break;
		case ID_STASH_APPLY:
			CAppUtils::StashApply(GetSafeHwnd(), L"", false);
			break;
		case ID_STASH_LIST:
			{
				CRefLogDlg dlg;
				dlg.m_CurrentBranch = L"refs/stash";
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
	CTGitPath commonDirectory;
	if (!m_bWholeProject)
		commonDirectory = m_pathList.GetCommonRoot().GetDirectory();
	bool bSingleFile = ((m_pathList.GetCount()==1)&&(!m_pathList[0].IsEmpty())&&(!m_pathList[0].IsDirectory()));
	if (bSingleFile)
		commonDirectory = m_pathList[0];
	CString sCmd;
	sCmd.Format(L"/command:showcompare /unified /path:\"%s\" /revision1:HEAD /revision2:%s", static_cast<LPCTSTR>(g_Git.CombinePath(commonDirectory)), static_cast<LPCTSTR>(GitRev::GetWorkingCopy()));
	if (!!(GetAsyncKeyState(VK_SHIFT) & 0x8000))
		sCmd += L" /alternative";
	CAppUtils::RunTortoiseGitProc(sCmd);
}

void CChangedDlg::OnBnClickedWholeProject()
{
	UpdateData();
	m_regShowWholeProject = m_bWholeProject;
	OnBnClickedRefresh();
}

void CChangedDlg::OnBnClickedShowStaged()
{
	UpdateData();
	m_regShowStaged = m_bShowStaged;
	OnBnClickedRefresh();
}
