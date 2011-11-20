// TortoiseGit - a Windows shell extension for easy version control

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
class CSettingsColors : public ISettingsPropPage
{
	DECLARE_DYNAMIC(CSettingsColors)

public:
	CSettingsColors();
	virtual ~CSettingsColors();

	UINT GetIconID() {return IDI_LOOKANDFEEL;}

	enum { IDD = IDD_SETTINGSCOLORS_1 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedColor();
	afx_msg void OnBnClickedRestore();
	virtual BOOL OnApply();

	DECLARE_MESSAGE_MAP()

private:
	CMFCColorButton	m_cConflict;
	CMFCColorButton	m_cAdded;
	CMFCColorButton	m_cDeleted;
	CMFCColorButton	m_cMerged;
	CMFCColorButton	m_cModified;
	CMFCColorButton	m_cAddedNode;
	CMFCColorButton	m_cDeletedNode;
	CMFCColorButton	m_cRenamedNode;
	CMFCColorButton	m_cReplacedNode;
	CColors			m_Colors;
};
