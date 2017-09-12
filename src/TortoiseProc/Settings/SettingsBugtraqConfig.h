// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009, 2011-2017 - TortoiseGit

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
#include "RegexEdit.h"
#include "GitSettings.h"
// CSettingsBugtraqConfig dialog

class CSettingsBugtraqConfig : public ISettingsPropPage, public CGitSettings
{
	DECLARE_DYNAMIC(CSettingsBugtraqConfig)

public:
	CSettingsBugtraqConfig();
	virtual ~CSettingsBugtraqConfig();

// Dialog Data
	enum { IDD = IDD_SETTINGSBUGTRAQ_CONFIG };
	UINT GetIconID() override { return IDI_BUGTRAQ; }

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support

	virtual BOOL OnInitDialog() override;
	virtual BOOL OnApply() override;

	afx_msg void OnBnClickedTestbugtraqregexbutton();

	virtual void LoadDataImpl(CAutoConfig& config) override;
	virtual BOOL SafeDataImpl(CAutoConfig& config) override;
	virtual void EnDisableControls() override;
	virtual HWND GetDialogHwnd() const override { return GetSafeHwnd(); }

	DECLARE_MESSAGE_MAP()

	afx_msg void OnChange();
	GITSETTINGS_RADIO_EVENT_HANDLE

	CRegexEdit	m_BugtraqRegex1;

	bool	m_bNeedSave;
	CString	m_URL;
	BOOL	m_bInheritURL;
	CComboBox	m_cWarningifnoissue;
	CString	m_Message;
	BOOL	m_bInheritMessage;
	CComboBox	m_cAppend;
	CString	m_Label;
	BOOL	m_bInheritLabel;
	CComboBox	m_cNumber;
	CString	m_Logregex;
	BOOL	m_bInheritLogregex;
	CString m_UUID32;
	BOOL	m_bInheritUUID32;
	CString m_UUID64;
	BOOL	m_bInheritUUID64;
	CString m_Params;
	BOOL	m_bInheritParams;
};
