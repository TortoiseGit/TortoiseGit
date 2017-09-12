// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2011, 2013, 2016 - TortoiseGit
// Copyright (C) 2003-2008, 2013 - TortoiseSVN

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
#include "registry.h"
#include "ConfigureGitExe.h"

/**
 * \ingroup TortoiseProc
 * This is the main page of the settings. It contains all the most important
 * settings.
 */
class CSetMainPage : public ISettingsPropPage, public CConfigureGitExe
{
	DECLARE_DYNAMIC(CSetMainPage)

public:
	CSetMainPage();
	virtual ~CSetMainPage();

	UINT GetIconID() override { return IDI_GENERAL; }

	enum { IDD = IDD_SETTINGSMAIN };

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

	virtual BOOL OnInitDialog() override;
	virtual BOOL OnApply() override;
	afx_msg void OnModified();
	afx_msg void OnClickVersioncheck();
	afx_msg void OnMsysGitPathModify();
	afx_msg void OnBnClickedChecknewerbutton();
	afx_msg void OnBrowseDir();
	afx_msg void OnCheck();
	afx_msg void OnBnClickedButtonShowEnv();
	afx_msg void OnBnClickedCreatelib();
	afx_msg void OnBnClickedRunfirststartwizard();

private:
	CString			m_sMsysGitPath;
	CString			m_sMsysGitExtranPath;
	CComboBox		m_LanguageCombo;
	CRegDWORD		m_regLanguage;
	DWORD			m_dwLanguage;
	CRegDWORD		m_regCheckNewer;
	BOOL			m_bCheckNewer;
};
