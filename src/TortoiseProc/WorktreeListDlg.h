// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2022 - TortoiseGit

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
#pragma once

#include "StandAloneDlg.h"
#include "GitHash.h"
#include "GitStatusListCtrl.h"
#include "GestureEnabledControl.h"

class WorktreeDetails
{
public:
	WorktreeDetails(CString worktreeName, CString path, CGitHash hash, CString branch)
		: m_isBaseRepo(FALSE)
		, m_IsLocked(FALSE)
		, m_WorktreeName(worktreeName)
		, m_Path(path)
		, m_Hash(hash)
		, m_Branch(branch)
	{}

	bool m_isBaseRepo;
	CString m_WorktreeName;
	CString m_Path;
	CGitHash m_Hash;
	CString m_Branch;
	bool m_IsLocked;
	CString m_LockedReason;

	CString GetFullName() const
	{
		return m_WorktreeName;
	}
};

class CWorktreeListDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CWorktreeListDlg)

public:
	CWorktreeListDlg(CWnd* pParent = nullptr); // standard constructor
	virtual ~CWorktreeListDlg();

	// Dialog Data
	enum
	{
		IDD = IDD_WORKTREE_LIST
	};

	enum eCmd
	{
		eCmd_Open = WM_APP,
		eCmd_Remove,
		eCmd_Lock,
		eCmd_Unlock,
	};

	enum eCol
	{
		eCol_Path,
		eCol_Hash,
		eCol_Branch,
		eCol_Locked,
		eCol_Reason,
	};

private:
	virtual void DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support

	DECLARE_MESSAGE_MAP()

	afx_msg void OnOK();
	afx_msg void OnDestroy();
	virtual BOOL OnInitDialog() override;
	virtual BOOL PreTranslateMessage(MSG* pMsg) override;
	afx_msg void OnNMDblclkWorktreeList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBnClickedButtonAdd();
	afx_msg void OnBnClickedButtonPrune();

	CGestureEnabledControlTmpl<CListCtrl> m_WorktreeList;
	ColumnManager m_ColumnManager;

	std::vector<WorktreeDetails> m_Worktrees;

	int m_nIconFolder = -1;

	afx_msg void OnContextMenu(CWnd* pWndFrom, CPoint point);
	void OnContextMenu_WorktreeList(CPoint point);
	void ShowContextMenu(CPoint point, std::vector<int>& indexes);

	void Refresh();
	int FillListCtrlWithWorktreeList(CString& error);
	int GetWorktreeNames(STRING_VECTOR& list, CString& error);

	bool RemoveWorktree(const CString& path);
	bool PruneWorktrees();
};
