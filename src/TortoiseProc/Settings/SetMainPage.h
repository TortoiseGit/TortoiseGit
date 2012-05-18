// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2011 - TortoiseGit
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
#include "Tooltip.h"
#include "Registry.h"


/**
 * \ingroup TortoiseProc
 * This is the main page of the settings. It contains all the most important
 * settings.
 */
class CSetMainPage : public ISettingsPropPage
{
	DECLARE_DYNAMIC(CSetMainPage)

public:
	CSetMainPage();
	virtual ~CSetMainPage();

	UINT GetIconID() {return IDI_GENERAL;}

	enum { IDD = IDD_SETTINGSMAIN };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnApply();
	afx_msg void OnModified();
	afx_msg void OnMsysGitPathModify();
	afx_msg void OnBnClickedEditconfig();
	afx_msg void OnBnClickedChecknewerbutton();
	afx_msg void OnBnClickedSounds();
	afx_msg void OnBrowseDir();
	afx_msg void OnCheck();
	afx_msg void OnBnClickedButtonShowEnv();
	afx_msg void OnBnClickedCreatelib();

	CString GetVersionFromFile(const CString & p_strDateiname);

private:
	CRegString		m_regMsysGitPath;
	CRegString		m_regMsysGitExtranPath;
	CString			m_sMsysGitPath;
	CString			m_sMsysGitExtranPath;
	CToolTips		m_tooltips;
	CComboBox		m_LanguageCombo;
	CRegDWORD		m_regLanguage;
	DWORD			m_dwLanguage;
	CRegDWORD		m_regCheckNewer;
	BOOL			m_bCheckNewer;
};
