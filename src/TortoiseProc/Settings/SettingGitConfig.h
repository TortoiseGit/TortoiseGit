// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2012 - TortoiseGit

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
#include "registry.h"
#include "afxwin.h"
#include "Git.h"

// CSettingGitConfig dialog

class CSettingGitConfig : public ISettingsPropPage
{
	DECLARE_DYNAMIC(CSettingGitConfig)

public:
	CSettingGitConfig();
	virtual ~CSettingGitConfig();
	UINT GetIconID() {return IDI_GITCONFIG;}
// Dialog Data
	enum { IDD = IDD_SETTINGIT_CONFIG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	BOOL OnInitDialog();
	BOOL OnApply();

	bool Save(CString key, CString value, CONFIG_TYPE type);

	int m_ChangeMask;
	enum
	{
		GIT_NAME=0x1,
		GIT_EMAIL=0x2,
		GIT_CRLF=0x4,
		GIT_SAFECRLF=0x8,
		GIT_SIGNINGKEY=0x10,
		GIT_WARNNOSIGNEDOFFBY=0x20,
	};
	DECLARE_MESSAGE_MAP()

	afx_msg void OnBnClickedCheckGlobal();
	afx_msg void OnEnChangeGitUsername();
	afx_msg void OnEnChangeGitUseremail();
	afx_msg void OnEnChangeGitUserSigningKey();
	afx_msg void OnBnClickedCheckAutocrlf();
	afx_msg void OnCbnSelchangeSafeCrLf();
	afx_msg void OnBnClickedCheckWarnNoSignedOffBy();
	afx_msg void OnBnClickedEditsystemgitconfig();
	afx_msg void OnBnClickedEditglobalgitconfig();
	afx_msg void OnBnClickedEditlocalgitconfig();

	CString	m_UserName;
	CString	m_UserEmail;
	CString	m_UserSigningKey;
	BOOL	m_bWarnNoSignedOffBy;
	BOOL	m_bGlobal;
	BOOL	m_bAutoCrlf;
	CComboBox m_cSafeCrLf;
};
