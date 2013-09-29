// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013 - TortoiseGit

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

	UINT GetIconID() {return IDI_DIALOGS;}

// Dialog Data
	enum { IDD = IDD_SETTINGSDIALOGS3 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnApply();

	virtual void LoadDataImpl(git_config * config);
	virtual BOOL SafeDataImpl(git_config * config);
	virtual void EnDisableControls();

	afx_msg void OnChange();
	GITSETTINGS_RADIO_EVENT_HANDLE;

	static BOOL CALLBACK EnumLocalesProc(LPTSTR lpLocaleString);

private:
	bool				m_bNeedSave;
	CToolTips			m_tooltips;
	static CComboBox	m_langCombo;
	CString				m_LogMinSize;
	CString				m_Border;
	CComboBox			m_cWarnNoSignedOffBy;
};
