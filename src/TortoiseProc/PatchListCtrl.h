// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2011, 2015-2016, 2018 - TortoiseGit

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


// CPatchListCtrl

class CPatchListCtrl : public CListCtrl
{
	DECLARE_DYNAMIC(CPatchListCtrl)

public:
	CPatchListCtrl();
	virtual ~CPatchListCtrl();
	DWORD m_ContextMenuMask;
	enum
	{
		MENU_SENDMAIL=1,
		MENU_VIEWPATCH,
		MENU_VIEWWITHMERGE,
		MENU_APPLY
	};

	enum
	{
		STATUS_NONE,
		STATUS_APPLYING = 0x10000,
		STATUS_APPLY_RETRY = 0x1,
		STATUS_APPLY_FAIL = 0x2,
		STATUS_APPLY_SUCCESS =0x4,
		STATUS_APPLY_SKIP=0x8,
		STATUS_MASK = 0xFFFF,
	};

	DWORD GetMenuMask(int x){return 1<<x;}

	CFont				m_boldFont;

protected:
	DECLARE_MESSAGE_MAP()
	void PreSubclassWindow() override;

public:
	afx_msg void OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
	int LaunchProc(const CString& cmd);
	afx_msg void OnNMCustomdraw(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDropFiles(HDROP hDropInfo);
};
