// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008 - TortoiseSVN

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
#include "resource.h"
#include "SettingsPropPage.h"
#include "BugTraqAssociations.h"

/**
 * \ingroup TortoiseProc
 * Setting page to configure the client side hook scripts
 */
class CSetBugTraq : public ISettingsPropPage
{
	DECLARE_DYNAMIC(CSetBugTraq)

public:
	CSetBugTraq();   // standard constructor
	virtual ~CSetBugTraq();

	UINT GetIconID() override { return IDI_BUGTRAQ; }

// Dialog Data
	enum { IDD = IDD_SETTINGSBUGTRAQ };

protected:
	virtual BOOL OnInitDialog() override;
	virtual BOOL OnApply() override;
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	afx_msg void OnBnClickedRemovebutton();
	afx_msg void OnBnClickedEditbutton();
	afx_msg void OnBnClickedAddbutton();
	afx_msg void OnLvnItemchangedBugTraqlist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkBugTraqlist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedBugTraqcopybutton();

	DECLARE_MESSAGE_MAP()

	void			RebuildBugTraqList();

	CBugTraqAssociations m_associations;
	CListCtrl m_cBugTraqList;
};
