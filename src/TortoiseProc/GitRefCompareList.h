// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2015 - TortoiseGit

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
#include "Git.h"

class CGitRefCompareList : public CHintCtrl<CListCtrl>
{
	DECLARE_DYNAMIC(CGitRefCompareList);

	enum ChangeType
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
		CGit::REF_TYPE refType;
		CString shortName;
		CString change;
		ChangeType changeType;
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

protected:
	afx_msg void OnContextMenu(CWnd *pWnd, CPoint point);
	void OnContextMenuList(CWnd *pWnd, CPoint point);
	void OnContextMenuHeader(CWnd *pWnd, CPoint point);

	DECLARE_MESSAGE_MAP()

private:
	static CString GetCommitMessage(git_commit *commit);
	static bool SortPredicate(const RefEntry &e1, const RefEntry &e2);

	std::vector<RefEntry>	m_RefList;
	BOOL					m_bHideUnchanged;
	static BOOL 			m_bSortLogical;

	int colRef;
	int colChange;
	int colOldHash;
	int colOldMessage;
	int colNewHash;
	int colNewMessage;
};


