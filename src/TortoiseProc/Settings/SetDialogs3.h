// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013-2017, 2023 - TortoiseGit

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

/**
 * \ingroup TortoiseProc
 * Settings page responsible for dialog settings.
 */
class CSetDialogs3 : public ISettingsPropPage, public CGitSettings
{
	DECLARE_DYNAMIC(CSetDialogs3)

public:
	CSetDialogs3();
	virtual ~CSetDialogs3();

	UINT GetIconID() override { return IDI_DIALOGS; }

// Dialog Data
	enum { IDD = IDD_SETTINGSDIALOGS3 };

protected:
	void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

	BOOL OnInitDialog() override;
	BOOL OnApply() override;

	void LoadDataImpl(CAutoConfig& config) override;
	BOOL SafeDataImpl(CAutoConfig& config) override;
	void EnDisableControls() override;
	HWND GetDialogHwnd() const override { return GetSafeHwnd(); }

	afx_msg void OnChange();
	afx_msg void OnBnClickedIconfileBrowse();
	GITSETTINGS_RADIO_EVENT_HANDLE;

	static BOOL CALLBACK EnumLocalesProc(LPWSTR lpLocaleString);
	void AddLangToCombo(DWORD langID);

private:
	bool				m_bNeedSave = false;
	CComboBox			m_langCombo;
	CString				m_LogMinSize;
	BOOL				m_bInheritLogMinSize;
	CString				m_Border;
	BOOL				m_bInheritBorder;
	CComboBox			m_cWarnNoSignedOffBy;
	CString				m_iconFile;
	BOOL				m_bInheritIconFile;
};
