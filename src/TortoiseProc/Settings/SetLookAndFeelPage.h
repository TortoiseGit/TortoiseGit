// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2011-2012 - TortoiseGit
// Copyright (C) 2003-2008,2011 - TortoiseSVN

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
#include "Tooltip.h"
#include "Registry.h"

/**
 * \ingroup TortoiseProc
 * Settings page look and feel.
 */
class CSetLookAndFeelPage : public ISettingsPropPage
{
	DECLARE_DYNAMIC(CSetLookAndFeelPage)

public:
	CSetLookAndFeelPage();
	virtual ~CSetLookAndFeelPage();

	UINT GetIconID() {return IDI_MISC;}

// Dialog Data
	enum { IDD = IDD_SETTINGSLOOKANDFEEL };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnApply();
	afx_msg void OnBnClickedSelectall();
	afx_msg void OnBnClickedRestoreDefaults();
	afx_msg void OnLvnItemchangedMenulist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnChange();
	afx_msg void OnEnChangeNocontextpaths();

	DECLARE_MESSAGE_MAP()

	virtual BOOL OnInitDialog();

private:
	CToolTips			m_tooltips;
	CRegStdDWORD			m_regTopmenu;
	CRegStdDWORD			m_regTopmenuhigh;

	CImageList			m_imgList;
	CListCtrl			m_cMenuList;
	BOOL				m_bModified;
	unsigned __int64	m_topmenu;
	bool				m_bBlock;
	CRegDWORD			m_regHideMenus;
	BOOL				m_bHideMenus;

	CString				m_sNoContextPaths;
	CRegString			m_regNoContextPaths;
};
