// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2007 - TortoiseSVN

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
#include "SettingsPropPage.h"

/**
 * \ingroup TortoiseProc
 * Setting page to configure the client side hook scripts
 */
class CSetHooks : public ISettingsPropPage
{
	DECLARE_DYNAMIC(CSetHooks)

public:
	CSetHooks();   // standard constructor
	virtual ~CSetHooks();

	UINT GetIconID() override { return IDI_HOOK; }

// Dialog Data
	enum { IDD = IDD_SETTINGSHOOKS };

protected:
	virtual BOOL OnInitDialog() override;
	virtual BOOL OnApply() override;
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	afx_msg void OnBnClickedRemovebutton();
	afx_msg void OnBnClickedEditbutton();
	afx_msg void OnBnClickedAddbutton();
	afx_msg void OnLvnItemchangedHooklist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkHooklist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedHookcopybutton();

	DECLARE_MESSAGE_MAP()

	void RebuildHookList();
	CListCtrl m_cHookList;
};
