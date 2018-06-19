// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2012, 2016 - TortoiseGit

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

#include "GitBlameLogList.h"
#include "ProjectProperties.h"
#include "GravatarPictureBox.h"
#include <unordered_set>

/////////////////////////////////////////////////////////////////////////////
// COutputList window

class COutputList : public CListBox
{
// Construction
public:
	COutputList();

// Implementation
public:
	virtual ~COutputList();

protected:
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnViewOutput();

	DECLARE_MESSAGE_MAP()
};

class COutputWnd;

class COutputWnd : public CDockablePane
{
	DECLARE_DYNAMIC(COutputWnd)
// Construction
public:
	COutputWnd();

// Attributes
public:
	CGitBlameLogList m_LogList;
	CGravatar m_Gravatar;

// Implementation
public:
	virtual ~COutputWnd();
	afx_msg void OnLvnItemchangedLoglist(NMHDR *pNMHDR, LRESULT *pResult);
	int	LoadHistory(CString filename, CString revision, bool follow);
	int	LoadHistory(std::unordered_set<CGitHash>& hashes);

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);

	DECLARE_MESSAGE_MAP()
};

