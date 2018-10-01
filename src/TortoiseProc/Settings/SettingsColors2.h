// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009, 2011, 2017-2018 - TortoiseGit
// Copyright (C) 2003-2008 - TortoiseSVN

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
#include "Colors.h"

/**
 * \ingroup TortoiseProc
 * Settings property page to set custom colors used in TortoiseSVN
 */
class CSettingsColors2 : public ISettingsPropPage
{
	DECLARE_DYNAMIC(CSettingsColors2)

public:
	CSettingsColors2();
	virtual ~CSettingsColors2();

	UINT GetIconID() override { return IDI_LOOKANDFEEL; }

	enum { IDD = IDD_SETTINGSCOLORS_2 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	virtual BOOL OnInitDialog() override;
	afx_msg void OnBnClickedColor();
	afx_msg void OnBnClickedRestore();
	virtual BOOL OnApply() override;

	DECLARE_MESSAGE_MAP()

private:
	CMFCColorButton	m_cCurrentBranch;
	CMFCColorButton	m_cRemoteBranch;
	CMFCColorButton	m_cLocalBranch;
	CMFCColorButton	m_cTags;
	CMFCColorButton	m_cFilterMatch;

	CColors			m_Colors;
};
