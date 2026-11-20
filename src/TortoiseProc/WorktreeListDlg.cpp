// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2022-2026 - TortoiseGit

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

static void SetSortArrow(CListCtrl* control, int nColumn, bool bAscending)
{
	if (!control)
		return;

	// set the sort arrow
	auto pHeader = control->GetHeaderCtrl();
	HDITEM HeaderItem{};
	HeaderItem.mask = HDI_FORMAT;
	for (int i = 0; i < pHeader->GetItemCount(); ++i)
	{
		pHeader->GetItem(i, &HeaderItem);
		HeaderItem.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
		pHeader->SetItem(i, &HeaderItem);
	}
	if (nColumn >= 0)
	{
		pHeader->GetItem(nColumn, &HeaderItem);
		HeaderItem.fmt |= (bAscending ? HDF_SORTUP : HDF_SORTDOWN);
		pHeader->SetItem(nColumn, &HeaderItem);
	}
}

class CWorktreeListCompareFunc
{
public:
	CWorktreeListCompareFunc(CListCtrl* pList, int col, bool desc)
		: m_col(col)
		, m_desc(desc)
		, m_pList(pList)
	{}

	static int CALLBACK StaticCompare(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
	{
		return reinterpret_cast<CWorktreeListCompareFunc*>(lParamSort)->Compare(lParam1, lParam2);
	}

	int Compare(LPARAM lParam1, LPARAM lParam2)
	{
		auto pLeft = reinterpret_cast<const WorktreeDetails*>(m_pList->GetItemData(static_cast<int>(lParam1)));
		auto pRight = reinterpret_cast<const WorktreeDetails*>(m_pList->GetItemData(static_cast<int>(lParam2)));

		int result = 0;
		switch (m_col)
		{
		case CWorktreeListDlg::eCol_Path:
			result = SortStrCmp(pLeft->m_Path, pRight->m_Path);
			if (result != 0)
				break;
			[[fallthrough]];
		case CWorktreeListDlg::eCol_Hash:
		{
			const auto cmp = pLeft->m_Hash <=> pRight->m_Hash;
			result = std::is_gt(cmp) - std::is_lt(cmp);
			if (result != 0)
				break;
			[[fallthrough]];
		}
		case CWorktreeListDlg::eCol_Branch:
			result = SortStrCmp(pLeft->m_Branch, pRight->m_Branch);
			if (result != 0)
				break;
			[[fallthrough]];
		case CWorktreeListDlg::eCol_Locked:
			result = (pLeft->m_IsLocked > pRight->m_IsLocked) - (pLeft->m_IsLocked < pRight->m_IsLocked);
			if (result != 0)
				break;
			[[fallthrough]];
		case CWorktreeListDlg::eCol_Reason:
			result = SortStrCmp(pLeft->m_LockedReason, pRight->m_LockedReason);
			if (result != 0)
				break;

			break;
		default:
			ASSERT(false);
		}

		if (m_desc)
			result = -result;

		return result;
	}

	int SortStrCmp(const CString& left, const CString& right)
	{
		if (CWorktreeListDlg::s_bSortLogical)
			return StrCmpLogicalW(left, right);
		return StrCmpI(left, right);
	}

	int m_col = CWorktreeListDlg::eCol_Path;
	bool m_desc = false;
	CListCtrl* m_pList = nullptr;
};

// CWorktreeListDlg dialog

bool CWorktreeListDlg::s_bSortLogical = true;

IMPLEMENT_DYNAMIC(CWorktreeListDlg, CResizableStandAloneDialog)

CWorktreeListDlg::CWorktreeListDlg(CWnd* pParent /*=nullptr*/)
	: CResizableStandAloneDialog(CWorktreeListDlg::IDD, pParent)
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
	ON_WM_SETCURSOR()
	ON_BN_CLICKED(IDC_BUTTON_ADD, &CWorktreeListDlg::OnBnClickedButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_PRUNE, &CWorktreeListDlg::OnBnClickedButtonPrune)
	ON_NOTIFY(NM_DBLCLK, IDC_WORKTREE_LIST, OnNMDblclkWorktreeList)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_WORKTREE_LIST, &OnLvnColumnclickWorktreeList)
END_MESSAGE_MAP()

// CWorktreeListDlg message handlers

BOOL CWorktreeListDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	CAppUtils::SetWindowTitle(*this, g_Git.m_CurrentDir);

	AddAnchor(IDC_WORKTREE_LIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_BUTTON_ADD, BOTTOM_LEFT);
	AddAnchor(IDC_BUTTON_PRUNE, BOTTOM_LEFT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);

	CWorktreeListDlg::s_bSortLogical = !CRegDWORD(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\NoStrCmpLogical", 0, false, HKEY_CURRENT_USER);
	if (CWorktreeListDlg::s_bSortLogical)
		CWorktreeListDlg::s_bSortLogical = !CRegDWORD(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\NoStrCmpLogical", 0, false, HKEY_LOCAL_MACHINE);

	const int columnWidth = CDPIAware::Instance().ScaleX(GetSafeHwnd(), 100);
	const ColumnDefinition columns[] = {
		{ eCol_Path, IDS_STATUSLIST_COLFILE, true, true, CDPIAware::Instance().ScaleX(GetSafeHwnd(), 150) },
		{ eCol_Hash, IDS_HASH, true, true, columnWidth },
		{ eCol_Branch, IDS_PROC_BRANCH, true, true, columnWidth },
		{ eCol_Locked, IDS_LOCKED, true, true, columnWidth },
		{ eCol_Reason, IDS_REASON, true, true, columnWidth },
	};
	constexpr int columnVersion = 0; // adjust when changing number/names/etc. of columns
	m_WorktreeList.m_ColumnManager.ReadSettings(columns, L"WorktreeList", columnVersion);
	m_WorktreeList.m_ColumnManager.SetRightAlign(eCol_Reason);

	// set up the list control
	// set the extended style of the list control
	m_WorktreeList.Init();
	m_WorktreeList.SetExtendedStyle(m_WorktreeList.GetExtendedStyle() | LVS_EX_DOUBLEBUFFER | LVS_EX_SUBITEMIMAGES);
	m_WorktreeList.SetImageList(&SYS_IMAGE_LIST(), LVSIL_SMALL);
	m_WorktreeList.SetListContextMenuHandler([&](CPoint point) { OnContextMenu_WorktreeList(point); });
	m_nIconFolder = SYS_IMAGE_LIST().GetDirIconIndex();

	CAppUtils::SetListCtrlBackgroundImage(m_WorktreeList.GetSafeHwnd(), IDI_REPOBROWSER_BKG);

	EnableSaveRestore(L"WorktreeList");
	SetTheme(CTheme::Instance().IsDarkTheme());

	Refresh();

	m_WorktreeList.SetFocus();

	return FALSE;
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
	m_currSortCol = -1;
	SetSortArrow(&m_WorktreeList, m_currSortCol, true);
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

	worktrees.AppendTo(list);

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
				worktreeDetails.m_LockedReason = reason.ToString();
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
			worktreeDetails.m_LockedReason = reason.ToString();
		m_Worktrees.push_back(worktreeDetails);
	}

	for (const auto& zfb : m_Worktrees)
	{
		const int indexItem = m_WorktreeList.InsertItem(m_WorktreeList.GetItemCount(), zfb.m_Path, m_nIconFolder);
		m_WorktreeList.SetItemData(indexItem, reinterpret_cast<DWORD_PTR>(&zfb));
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

	auto worktreeItem = reinterpret_cast<const WorktreeDetails*>(m_WorktreeList.GetItemData(selIndex));
	CAppUtils::ExploreTo(GetSafeHwnd(), worktreeItem->m_Path);
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
		auto worktree = reinterpret_cast<const WorktreeDetails*>(m_WorktreeList.GetItemData(indexes[0]));
		showUnlock = worktree->m_IsLocked;
		showLock = !showUnlock;

		// Can't delete or lock the base repo, only worktrees
		if (worktree->m_isBaseRepo)
		{
			showRemove = false;
			showLock = false;
		}
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
	{
		popupMenu.AppendMenuIcon(eCmd_Remove, IDS_REMOVEBUTTON, IDI_DELETE);
		popupMenu.AppendMenuIcon(eCmd_RemoveWithForce, IDS_REMOVEWITHFORCE, IDI_DELETE);
	}

	eCmd cmd = static_cast<eCmd>(popupMenu.TrackPopupMenuEx(TPM_LEFTALIGN | TPM_RETURNCMD, point.x, point.y, this, nullptr));
	switch (cmd)
	{
		case eCmd_Open:
		{
			if (indexes.size() != 1)
				return;
			auto worktree = reinterpret_cast<const WorktreeDetails*>(m_WorktreeList.GetItemData(indexes[0]));
			CAppUtils::ExploreTo(GetSafeHwnd(), worktree->m_Path);
			break;
		}
		case eCmd_Remove:
		case eCmd_RemoveWithForce:
		{
			if (CMessageBox::Show(GetSafeHwnd(), IDS_PROC_DELETE_WORKTREE, IDS_APPNAME, 2, IDI_QUESTION, IDS_MSGBOX_YES, IDS_MSGBOX_NO) == 2)
				return;
			for (int index : indexes)
			{
				auto worktree = reinterpret_cast<const WorktreeDetails*>(m_WorktreeList.GetItemData(index));

				if (worktree->m_isBaseRepo)
				{
					// Silently skip it. It shouldn't be selectable by itself.
					// If included in a multi-selection, just silently skip it.
					continue;
				}

				if (!RemoveWorktree(worktree->m_Path, cmd == eCmd_RemoveWithForce))
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
				auto leaf = reinterpret_cast<const WorktreeDetails*>(m_WorktreeList.GetItemData(index));
				if (leaf->m_isBaseRepo)
					continue;

				CAutoWorktree worktree;
				if (git_worktree_lookup(worktree.GetPointer(), g_Git.GetGitRepository(), CUnicodeUtils::GetUTF8(leaf->m_WorktreeName)) < 0 || git_worktree_lock(worktree, nullptr) < 0)
				{
					if (CMessageBox::Show(GetSafeHwnd(), g_Git.GetLibGit2LastErr(L"Failed to lock worktree \"" + leaf->m_WorktreeName + L"\"."), IDS_APPNAME, 1, IDI_ERROR, IDS_MSGBOX_CONTINUE, IDS_MSGBOX_ABORT) == 2)
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
				auto leaf = reinterpret_cast<const WorktreeDetails*>(m_WorktreeList.GetItemData(index));
				if (leaf->m_isBaseRepo)
					continue;

				CAutoWorktree worktree;
				if (git_worktree_lookup(worktree.GetPointer(), g_Git.GetGitRepository(), CUnicodeUtils::GetUTF8(leaf->m_WorktreeName)) < 0 || git_worktree_unlock(worktree) < 0)
				{
					if (CMessageBox::Show(GetSafeHwnd(), g_Git.GetLibGit2LastErr(L"Failed to unlock worktree \"" + leaf->m_WorktreeName + L"\"."), IDS_APPNAME, 1, IDI_ERROR, IDS_MSGBOX_CONTINUE, IDS_MSGBOX_ABORT) == 2)
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

bool CWorktreeListDlg::RemoveWorktree(const CString& path, bool force)
{
	CString params;
	if (force)
		params += L" --force";

	CString cmd;
	try
	{
		cmd.Format(L"git.exe worktree remove%s -- %s", static_cast<LPCWSTR>(params), static_cast<LPCWSTR>(CGit::QuoteParameter(path)));
	}
	catch (illegal_git_parameter& e)
	{
		MessageBox(e.cause(), L"TortoiseGit", MB_OK | MB_ICONERROR);
		return false;
	}


	CProgressDlg progress;
	progress.m_GitCmd = cmd;
	progress.m_PostCmdCallback = [&](DWORD status, PostCmdList& postCmdList) {
		if (status && !force)
			postCmdList.emplace_back(IDI_DELETE, IDS_REMOVEWITHFORCE, [&] { RemoveWorktree(path, true); });
	};

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

void CWorktreeListDlg::OnLvnColumnclickWorktreeList(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	if (pResult)
		*pResult = 0;

	if (m_currSortCol == pNMLV->iSubItem)
		m_currSortDesc = !m_currSortDesc;
	else
	{
		m_currSortCol = pNMLV->iSubItem;
		m_currSortDesc = false;
	}

	CWorktreeListCompareFunc compareFunc(&m_WorktreeList, m_currSortCol, m_currSortDesc);
	m_WorktreeList.SortItemsEx(&CWorktreeListCompareFunc::StaticCompare, reinterpret_cast<DWORD_PTR>(&compareFunc));

	SetSortArrow(&m_WorktreeList, m_currSortCol, !m_currSortDesc);
}
