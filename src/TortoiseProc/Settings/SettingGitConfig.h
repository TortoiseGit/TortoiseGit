// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2017, 2019-2020, 2023, 2025 - TortoiseGit

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
#include "GitSettings.h"

// CSettingGitConfig dialog

class CSettingGitConfig : public ISettingsPropPage, public CGitSettings
{
	DECLARE_DYNAMIC(CSettingGitConfig)

public:
	CSettingGitConfig();
	virtual ~CSettingGitConfig();
	UINT GetIconID() override { return IDI_GITCONFIG; }
// Dialog Data
	enum { IDD = IDD_SETTINGIT_CONFIG };

protected:
	void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	BOOL OnInitDialog() override;
	BOOL OnApply() override;

	void LoadDataImpl(CAutoConfig& config) override;
	BOOL SafeDataImpl(CAutoConfig& config) override;
	void EnDisableControls() override;
	HWND GetDialogHwnd() const override { return GetSafeHwnd(); }

	DECLARE_MESSAGE_MAP()

	afx_msg void OnChange();
	afx_msg void OnBnClickedEditsystemgitconfig();
	afx_msg void OnBnClickedViewsystemgitconfig();
	afx_msg void OnBnClickedEditglobalgitconfig();
	afx_msg void OnBnClickedEditglobalxdggitconfig();
	afx_msg void OnBnClickedEditlocalgitconfig();
	afx_msg void OnBnClickedEdittgitconfig();
	afx_msg void OnBnClickedVieweffectivegitconfig();
	GITSETTINGS_RADIO_EVENT_HANDLE;

	bool	m_bNeedSave = false;
	CString	m_UserName;
	BOOL	m_bInheritUserName;
	CString	m_UserEmail;
	BOOL	m_bInheritEmail;
	CString	m_UserSigningKey;
	BOOL	m_bInheritSigningKey;
	CComboBox m_cAutoCrLf;
	BOOL	m_bQuotePath;
	BOOL	m_bPrune;
	CComboBox m_cSafeCrLf;
};
