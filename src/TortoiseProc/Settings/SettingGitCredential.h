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
#include "afxwin.h"
#include "SettingsPropPage.h"
#include "Tooltip.h"
#include "registry.h"

// CSettingGitCredential dialog
class CSettingGitCredential : public ISettingsPropPage
{
	DECLARE_DYNAMIC(CSettingGitCredential)

public:
	enum
	{
		CREDENTIAL_URL			= 0x1,
		CREDENTIAL_HELPER		= 0x2,
		CREDENTIAL_USERNAME		= 0x4,
		CREDENTIAL_USEHTTPPATH	= 0x8,
	};
	CSettingGitCredential(CString cmdPath);
	virtual ~CSettingGitCredential();
	UINT GetIconID() { return IDI_GITCREDENTIAL; }
// Dialog Data
	enum { IDD = IDD_SETTINGSCREDENTIAL };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

	afx_msg void OnBnClickedButtonAdd();
	afx_msg void OnLbnSelchangeListUrl();
	afx_msg void OnCbnSelchangeComboConfigType();
	afx_msg void OnEnChangeEditUrl();
	afx_msg void OnEnChangeEditHelper();
	afx_msg void OnEnChangeEditUsername();
	afx_msg void OnBnClickedCheckUsehttppath();
	afx_msg void OnBnClickedButtonRemove();

	BOOL OnInitDialog();
	BOOL OnApply();

	BOOL IsUrlExist(CString &text);

	void AddConfigType(int &index, CString text, bool add = true);
	void LoadList();
	CString Load(CString key);
	void Save(CString key, CString value);

	int			m_ChangedMask;

	CString		m_cmdPath;

	CListBox	m_ctrlUrlList;
	CComboBox	m_ctrlConfigType;
	CString		m_strUrl;
	CString		m_strHelper;
	CString		m_strUsername;
	BOOL		m_bUseHttpPath;
};
