// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2022-2023 - TortoiseGit

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
// CWorktreeListDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "WorktreeListDlg.h"
#include "ProgressDlg.h"
#include "AppUtils.h"
#include "IconMenu.h"
#include "UnicodeUtils.h"
#include "SysImageList.h"
#include "PathUtils.h"
#include "MessageBox.h"

// CWorktreeListDlg dialog

IMPLEMENT_DYNAMIC(CWorktreeListDlg, CResizableStandAloneDialog)

CWorktreeListDlg::CWorktreeListDlg(CWnd* pParent /*=nullptr*/)
	: CResizableStandAloneDialog(CWorktreeListDlg::IDD, pParent)
	, m_ColumnManager(&m_WorktreeList)
{
}

CWorktreeListDlg::~CWorktreeListDlg()
{
}

void CWorktreeListDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_WORKTREE_LIST, m_WorktreeList);
}

BEGIN_MESSAGE_MAP(CWorktreeListDlg, CResizableStandAloneDialog)
	ON_WM_CONTEXTMENU()
	ON_WM_SETCURSOR()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON_ADD, &CWorktreeListDlg::OnBnClickedButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_PRUNE, &CWorktreeListDlg::OnBnClickedButtonPrune)
	ON_NOTIFY(NM_DBLCLK, IDC_WORKTREE_LIST, OnNMDblclkWorktreeList)
END_MESSAGE_MAP()

// CWorktreeListDlg message handlers

BOOL CWorktreeListDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	CString sWindowTitle;
	GetWindowText(sWindowTitle);
	CAppUtils::SetWindowTitle(m_hWnd, g_Git.m_CurrentDir, sWindowTitle);

	AddAnchor(IDC_WORKTREE_LIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_BUTTON_ADD, BOTTOM_LEFT);
	AddAnchor(IDC_BUTTON_PRUNE, BOTTOM_LEFT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);

	static UINT columnNames[] = { IDS_STATUSLIST_COLFILE, IDS_HASH, IDS_PROC_BRANCH, IDS_LOCKED, IDS_REASON };
	static int columnWidths[] = { CDPIAware::Instance().ScaleX(GetSafeHwnd(), 150), CDPIAware::Instance().ScaleX(GetSafeHwnd(), 100), CDPIAware::Instance().ScaleX(GetSafeHwnd(), 100), CDPIAware::Instance().ScaleX(GetSafeHwnd(), 100), CDPIAware::Instance().ScaleX(GetSafeHwnd(), 100) };
	DWORD dwDefaultColumns = (1 << eCol_Path) | (1 << eCol_Hash) | (1 << eCol_Branch) | (1 << eCol_Locked) | (1 << eCol_Reason);
	m_ColumnManager.SetNames(columnNames, _countof(columnNames));
	constexpr int columnVersion = 0; // adjust when changing number/names/etc. of columns
	m_ColumnManager.ReadSettings(dwDefaultColumns, 0, L"WorktreeList", columnVersion, _countof(columnNames), columnWidths);
	m_ColumnManager.SetRightAlign(m_ColumnManager.GetColumnByName(IDS_REASON));

	// set up the list control
	// set the extended style of the list control
	CRegDWORD regFullRowSelect(L"Software\\TortoiseGit\\FullRowSelect", TRUE);
	DWORD exStyle = LVS_EX_HEADERDRAGDROP | LVS_EX_DOUBLEBUFFER | LVS_EX_SUBITEMIMAGES;
	if (DWORD(regFullRowSelect))
		exStyle |= LVS_EX_FULLROWSELECT;
	m_WorktreeList.SetExtendedStyle(exStyle);
	m_WorktreeList.SetImageList(&SYS_IMAGE_LIST(), LVSIL_SMALL);
	m_nIconFolder = SYS_IMAGE_LIST().GetDirIconIndex();

	CAppUtils::SetListCtrlBackgroundImage(m_WorktreeList.GetSafeHwnd(), IDI_REPOBROWSER_BKG);

	SetWindowTheme(m_WorktreeList.GetSafeHwnd(), L"Explorer", nullptr);

	EnableSaveRestore(L"WorktreeList");

	Refresh();

	m_WorktreeList.SetFocus();

	return FALSE;
}

void CWorktreeListDlg::OnDestroy()
{
	int maxcol = m_ColumnManager.GetColumnCount();
	for (int col = 0; col < maxcol; ++col)
		if (m_ColumnManager.IsVisible(col))
			m_ColumnManager.ColumnResized(col);
	m_ColumnManager.WriteSettings();

	CResizableStandAloneDialog::OnDestroy();
}

void CWorktreeListDlg::OnOK()
{
	CResizableStandAloneDialog::OnOK();
}

void CWorktreeListDlg::Refresh()
{
	BeginWaitCursor();
	m_WorktreeList.DeleteAllItems();
	m_Worktrees.clear();

	if (CString git_err; FillListCtrlWithWorktreeList(git_err) < 0)
		MessageBox(git_err, L"TortoiseGit", MB_OK | MB_ICONERROR);
	EndWaitCursor();
}

int CWorktreeListDlg::GetWorktreeNames(STRING_VECTOR& list, CString& error)
{
	if (!g_Git.m_IsUseLibGit2)
		return -1;

	CAutoRepository repo(g_Git.GetGitRepository());
	if (!repo)
	{
		error = g_Git.GetLibGit2LastErr();
		return -1;
	}

	CAutoStrArray worktrees;
	if (git_worktree_list(worktrees, repo) < 0)
	{
		error = g_Git.GetLibGit2LastErr();
		return -1;
	}

	for (size_t i = 0; i < worktrees->count; ++i)
		list.push_back(CUnicodeUtils::GetUnicode(worktrees->strings[i]));

	return 0;
}

int CWorktreeListDlg::FillListCtrlWithWorktreeList(CString& error)
{
	STRING_VECTOR list;
	if (GetWorktreeNames(list, error) < 0)
		return -1;

	CAutoRepository repo(g_Git.GetGitRepository());
	if (!repo)
	{
		error = g_Git.GetLibGit2LastErr();
		return -1;
	}

	// Base repo (not worktree)
	{
		git_repository* baseRepo = repo;
		CAutoRepository autoBaseRepo;
		if (git_repository_is_worktree(repo))
		{
			autoBaseRepo.Open(git_repository_commondir(repo));
			baseRepo = autoBaseRepo;
		}

		CAutoReference nonWorktreeHead;
		if (git_repository_head(nonWorktreeHead.GetPointer(), baseRepo) < 0)
		{
			error = g_Git.GetLibGit2LastErr();
			return -1;
		}
		CString workDirPath;
		if (git_repository_is_bare(baseRepo))
			workDirPath = CUnicodeUtils::GetUnicode(git_repository_commondir(baseRepo));
		else
			workDirPath = CUnicodeUtils::GetUnicode(git_repository_workdir(baseRepo));
		CPathUtils::ConvertToBackslash(workDirPath);
		CPathUtils::TrimTrailingPathDelimiter(workDirPath);
		WorktreeDetails worktreeDetails(L"", workDirPath, git_reference_target(nonWorktreeHead), CUnicodeUtils::GetUnicode(git_reference_shorthand(nonWorktreeHead)));
		worktreeDetails.m_isBaseRepo = TRUE;
		m_Worktrees.push_back(worktreeDetails);
	}

	for (size_t i = 0; i < list.size(); ++i)
	{
		CAutoWorktree worktree;
		if (git_worktree_lookup(worktree.GetPointer(), repo, CUnicodeUtils::GetUTF8(list[i])) < 0)
		{
			error = g_Git.GetLibGit2LastErr();
			return -1;
		}
		CString worktreePath = CUnicodeUtils::GetUnicode(git_worktree_path(worktree));
		CPathUtils::ConvertToBackslash(worktreePath);

		CAutoBuf reason;
		unsigned int locked = git_worktree_is_locked(reason, worktree);
		if (locked < 0)
		{
			error = g_Git.GetLibGit2LastErr();
			return -1;
		}

		git_worktree_prune_options prune_opts = GIT_WORKTREE_PRUNE_OPTIONS_INIT;
		// Don't need to let it check for locked worktree, we can check that ourself
		prune_opts.flags = GIT_WORKTREE_PRUNE_LOCKED;
		if (int pruneable = git_worktree_is_prunable(worktree, &prune_opts); pruneable < 0)
		{
			error = g_Git.GetLibGit2LastErr();
			return -1;
		}
		// Can't get information about HEAD and so on because worktree checkout doesn't exist
		else if (pruneable == 1)
		{
			WorktreeDetails worktreeDetails(list[i], worktreePath, CGitHash(), L"");
			worktreeDetails.m_isBaseRepo = FALSE;
			worktreeDetails.m_IsLocked = locked;
			if (locked && reason)
				worktreeDetails.m_LockedReason = CUnicodeUtils::GetUnicode(reason->ptr);
			m_Worktrees.push_back(worktreeDetails);
			continue;
		}

		CAutoReference head;
		if (git_repository_head_for_worktree(head.GetPointer(), repo, CUnicodeUtils::GetUTF8(list[i])) < 0)
		{
			error = g_Git.GetLibGit2LastErr();
			return -1;
		}

		CString branch = CUnicodeUtils::GetUnicode(git_reference_shorthand(head));
		if (branch == L"HEAD")
		{
			// `git worktree list` command says "detached HEAD", so we should say the same
			branch = L"detached HEAD";
		}

		WorktreeDetails worktreeDetails(list[i], worktreePath, git_reference_target(head), branch);
		worktreeDetails.m_isBaseRepo = FALSE;
		worktreeDetails.m_IsLocked = locked;
		if (locked && reason)
			worktreeDetails.m_LockedReason = CStringW(reason->ptr, (int)reason->size);
		m_Worktrees.push_back(worktreeDetails);
	}

	for (const auto& zfb : m_Worktrees)
	{
		int indexItem = m_WorktreeList.InsertItem(m_WorktreeList.GetItemCount(), zfb.m_Path, m_nIconFolder);
		if (zfb.m_Hash.IsEmpty())
			m_WorktreeList.SetItemText(indexItem, eCol_Hash, L"");
		else
			m_WorktreeList.SetItemText(indexItem, eCol_Hash, zfb.m_Hash.ToString());
		m_WorktreeList.SetItemText(indexItem, eCol_Branch, zfb.m_Branch);

		if (zfb.m_isBaseRepo)
			continue;

		CString lockStr;
		if (zfb.m_IsLocked)
		{
			lockStr.LoadString(IDS_LOCKED);
			m_WorktreeList.SetItemText(indexItem, eCol_Reason, zfb.m_LockedReason);
		}
		else
			lockStr.LoadString(IDS_UNLOCKED);
		m_WorktreeList.SetItemText(indexItem, eCol_Locked, lockStr);
	}

	return 0;
}

void CWorktreeListDlg::OnContextMenu(CWnd* pWndFrom, CPoint point)
{
	if (pWndFrom == &m_WorktreeList)
	{
		CRect headerPosition;
		m_WorktreeList.GetHeaderCtrl()->GetWindowRect(headerPosition);
		if (!headerPosition.PtInRect(point))
			OnContextMenu_WorktreeList(point);
	}
}

void CWorktreeListDlg::OnContextMenu_WorktreeList(CPoint point)
{
	std::vector<int> indexes;

	POSITION pos = m_WorktreeList.GetFirstSelectedItemPosition();
	while (pos)
	{
		int index = m_WorktreeList.GetNextSelectedItem(pos);
		indexes.push_back(index);
	}

	ShowContextMenu(point, indexes);
}

void CWorktreeListDlg::OnNMDblclkWorktreeList(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	int selIndex = pNMLV->iItem;
	if (selIndex < 0)
		return;
	if (selIndex >= static_cast<int>(m_Worktrees.size()))
		return;

	CAppUtils::ExploreTo(GetSafeHwnd(), m_Worktrees[selIndex].m_Path);
}

void CWorktreeListDlg::ShowContextMenu(CPoint point, std::vector<int>& indexes)
{
	// When selecting multiple worktrees, allow any modification action
	bool showLock = true;
	bool showUnlock =true;
	bool showRemove = true;

	if (indexes.empty())
		return;

	// If there's only one and we can't operate on it, fail.
	// If there are multiple and some of them can't be operated on, just ignore those.
	if (indexes.size() == 1)
	{
		auto& worktree = m_Worktrees[indexes[0]];
		showUnlock = worktree.m_IsLocked;
		showLock = !showUnlock;

		// Can't delete or lock the base repo, only worktrees
		if (worktree.m_isBaseRepo)
			showRemove = false;
	}

	CIconMenu popupMenu;
	popupMenu.CreatePopupMenu();

	if (indexes.size() == 1)
	{
		popupMenu.AppendMenuIcon(eCmd_Open, IDS_STATUSLIST_CONTEXT_EXPLORE, IDI_EXPLORER);
		popupMenu.SetDefaultItem(eCmd_Open, FALSE);
	}
	if (showLock)
		popupMenu.AppendMenuIcon(eCmd_Lock, IDS_MENULFSLOCK, IDI_LFSLOCK);
	if (showUnlock)
		popupMenu.AppendMenuIcon(eCmd_Unlock, IDS_MENULFSUNLOCK, IDI_LFSUNLOCK);
	if (showRemove)
		popupMenu.AppendMenuIcon(eCmd_Remove, IDS_REMOVEBUTTON, IDI_DELETE);

	eCmd cmd = static_cast<eCmd>(popupMenu.TrackPopupMenuEx(TPM_LEFTALIGN | TPM_RETURNCMD, point.x, point.y, this, nullptr));
	switch (cmd)
	{
		case eCmd_Open:
		{
			if (indexes.size() != 1)
				return;
			CAppUtils::ExploreTo(GetSafeHwnd(), m_Worktrees[indexes[0]].m_Path);
			break;
		}
		case eCmd_Remove:
		{
			if (CMessageBox::Show(GetSafeHwnd(), IDS_PROC_DELETE_WORKTREE, IDS_APPNAME, 2, IDI_QUESTION, IDS_MSGBOX_YES, IDS_MSGBOX_NO) == 2)
				return;
			for (int index : indexes)
			{
				auto& zf = m_Worktrees[index];

				if (zf.m_isBaseRepo)
				{
					// Silently skip it. It shouldn't be selectable by itself.
					// If included in a multi-selection, just silently skip it.
					continue;
				}

				if (!RemoveWorktree(zf.m_Path))
					break; // Don't continue with other worktrees
			}

			Refresh();
			break;
		}
		case eCmd_Lock:
		{
			// TODO: Show dialog to enter reason
			int lockedSuccessfully = 0;
			for (int index : indexes)
			{
				auto& leaf = m_Worktrees[index];
				if (leaf.m_isBaseRepo)
					continue;

				CAutoWorktree worktree;
				if (git_worktree_lookup(worktree.GetPointer(), g_Git.GetGitRepository(), CUnicodeUtils::GetUTF8(leaf.m_WorktreeName)) < 0 || git_worktree_lock(worktree, nullptr) < 0)
				{
					if (CMessageBox::Show(GetSafeHwnd(), g_Git.GetLibGit2LastErr(L"Failed to lock worktree \"" + leaf.m_WorktreeName + L"\"."), L"TortoiseGit", 1, IDI_ERROR, CString(MAKEINTRESOURCE(IDS_MSGBOX_CONTINUE)), CString(MAKEINTRESOURCE(IDS_MSGBOX_ABORT))) == 2)
						break;
				}
				else
					++lockedSuccessfully;
			}
			CString msg;
			msg.Format(IDS_PROC_LOCKED_WORKTREES, lockedSuccessfully);
			MessageBox(msg, L"TortoiseGit", MB_OK| MB_ICONINFORMATION);
			Refresh();
			break;
		}
		case eCmd_Unlock:
		{
			int unlockedSuccessfully = 0;
			for (int index : indexes)
			{
				auto& leaf = m_Worktrees[index];
				if (leaf.m_isBaseRepo)
					continue;

				CAutoWorktree worktree;
				if (git_worktree_lookup(worktree.GetPointer(), g_Git.GetGitRepository(), CUnicodeUtils::GetUTF8(leaf.m_WorktreeName)) < 0 || git_worktree_unlock(worktree) < 0)
				{
					if (CMessageBox::Show(GetSafeHwnd(), g_Git.GetLibGit2LastErr(L"Failed to unlock worktree \"" + leaf.m_WorktreeName + L"\"."), L"TortoiseGit", 1, IDI_ERROR, CString(MAKEINTRESOURCE(IDS_MSGBOX_CONTINUE)), CString(MAKEINTRESOURCE(IDS_MSGBOX_ABORT))) == 2)
						break;
				}
				else
					++unlockedSuccessfully;
			}
			CString msg;
			msg.Format(IDS_PROC_UNLOCKED_WORKTREES, unlockedSuccessfully);
			MessageBox(msg, L"TortoiseGit", MB_OK | MB_ICONINFORMATION);
			Refresh();
			break;
		}
	}
}

BOOL CWorktreeListDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
			case VK_F5:
				Refresh();
				break;
		}
	}

	return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}

void CWorktreeListDlg::OnBnClickedButtonAdd()
{
	if (CAppUtils::CreateWorktree(GetSafeHwnd()))
		Refresh();
}

bool CWorktreeListDlg::RemoveWorktree(const CString& path)
{
	CString cmd;
	cmd.Format(L"git.exe worktree remove \"%s\"", static_cast<LPCWSTR>(path));

	CProgressDlg progress;
	progress.m_GitCmd = cmd;

	return progress.DoModal() == IDOK;
}

bool CWorktreeListDlg::PruneWorktrees()
{
	CProgressDlg progress;
	progress.m_GitCmd = L"git.exe worktree prune";
	return progress.DoModal() == IDOK;
}

void CWorktreeListDlg::OnBnClickedButtonPrune()
{
	if (PruneWorktrees())
		Refresh();
}
