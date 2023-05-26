// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2017, 2023 - TortoiseGit

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
// GitLogList.cpp : implementation file
//
#pragma once

#include "HintCtrl.h"
#include "GitHash.h"

class CGitRefCompareList : public CHintCtrl<CListCtrl>
{
	DECLARE_DYNAMIC(CGitRefCompareList);

	enum class ChangeType
	{
		Unknown,
		New,
		Deleted,
		FastForward,
		NewerTime,
		Rewind,
		OlderTime,
		SameTime,
		Same
	};

	struct RefEntry
	{
		CString fullName;
		CGit::REF_TYPE refType = CGit::REF_TYPE::UNKNOWN;
		CString shortName;
		CString change;
		ChangeType changeType = ChangeType::Unknown;
		CString oldHash;
		CString oldMessage;
		CString newHash;
		CString newMessage;
	};

public:
	CGitRefCompareList();

	virtual ~CGitRefCompareList()
	{
	}

	void Init();

	int AddEntry(git_repository* repo, const CString& ref, const CGitHash* oldHash, const CGitHash* newHash);
	void Show();
	void Clear();
	static CString GetCommitMessage(git_commit *commit);

protected:
	afx_msg void OnHdnItemclick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnContextMenu(CWnd *pWnd, CPoint point);
	void OnContextMenuList(CWnd *pWnd, CPoint point);
	void OnContextMenuHeader(CWnd *pWnd, CPoint point);
	ULONG GetGestureStatus(CPoint ptTouch) override;

	DECLARE_MESSAGE_MAP()

private:
	static bool SortPredicate(const RefEntry &e1, const RefEntry &e2);

	std::vector<RefEntry>	m_RefList;
	BOOL					m_bHideUnchanged;
	static BOOL 			m_bSortLogical;

	int colRef = 0;
	int colRefType = 0;
	int colChange = 0;
	int colOldHash = 0;
	int colOldMessage = 0;
	int colNewHash = 0;
	int colNewMessage = 0;

	bool	m_bAscending = false;		///< sort direction
	int		m_nSortedColumn = -1;	///< which column to sort
};
