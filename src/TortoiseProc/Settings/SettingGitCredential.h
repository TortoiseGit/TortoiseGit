// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013-2016 - TortoiseGit

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
#include "Git.h"

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
	CSettingGitCredential();
	virtual ~CSettingGitCredential();
	UINT GetIconID() { return IDI_GITCREDENTIAL; }
// Dialog Data
	enum { IDD = IDD_SETTINGSCREDENTIAL };

	static int GetCredentialDefaultUrlCallback(const git_config_entry *entry, void *payload)
	{
		((STRING_VECTOR*)payload)->push_back(ConfigLevelToKey(entry->level));
		return 0;
	}

	static int GetCredentialUrlCallback(const git_config_entry *entry, void *payload)
	{
		CString name = CUnicodeUtils::GetUnicode(entry->name);
		int pos1 = name.Find(L'.');
		int pos2 = name.ReverseFind(L'.');
		CString url = name.Mid(pos1 + 1, pos2 - pos1 - 1);
		CString display;
		display.Format(L"%s:%s", (LPCTSTR)ConfigLevelToKey(entry->level), url);
		((STRING_VECTOR*)payload)->push_back(display);
		return 0;
	}

	static int GetCredentialEntryCallback(const git_config_entry *entry, void *payload)
	{
		CString name = CUnicodeUtils::GetUnicode(entry->name);
		((STRING_VECTOR*)payload)->push_back(name);
		return 0;
	}

	static int GetCredentialAnyEntryCallback(const git_config_entry *entry, void *payload)
	{
		CString name = CUnicodeUtils::GetUnicode(entry->name);
		CString value = CUnicodeUtils::GetUnicode(entry->value);
		CString text;
		text.Format(L"%s\n%s\n%s", (LPCTSTR)ConfigLevelToKey(entry->level), (LPCTSTR)name, (LPCTSTR)value);
		((STRING_VECTOR*)payload)->push_back(text);
		return 0;
	}

	static CString GetWinstorePath()
	{
		TCHAR winstorebuf[MAX_PATH] = { 0 };
		ExpandEnvironmentStrings(L"%AppData%\\GitCredStore\\git-credential-winstore.exe", winstorebuf, _countof(winstorebuf));
		CString winstore;
		winstore.Format(L"!'%s'", winstorebuf);
		return winstore;
	}

	static bool WincredExists()
	{
		CString path = CGit::ms_MsysGitRootDir;
		path.Append(L"libexec\\git-core\\git-credential-wincred.exe");
		return !!PathFileExists(path);
	}

	static bool WinstoreExists()
	{
		return !!PathFileExists(GetWinstorePath());
	}

	static bool GCMExists()
	{
		CString path = CGit::ms_MsysGitRootDir;
		path.Append(L"libexec\\git-core\\git-credential-manager.exe");
		if (!PathFileExists(path))
			return false;
		// CCM requires .NET 4.5.1 or later
		// try to detect it (only works for >=4.5): https://msdn.microsoft.com/en-us/library/hh925568
		return CRegDWORD(L"SOFTWARE\\Microsoft\\NET Framework Setup\\NDP\\v4\\Full\\Release", 0, false, HKEY_LOCAL_MACHINE) >= 378675;
	}

protected:
	static CString ConfigLevelToKey(git_config_level_t level)
	{
		switch (level)
		{
		case GIT_CONFIG_LEVEL_PROGRAMDATA:
			return L"P";
		case GIT_CONFIG_LEVEL_SYSTEM:
			return L"S";
		case GIT_CONFIG_LEVEL_XDG:
			return L"X";
		case GIT_CONFIG_LEVEL_GLOBAL:
			return L"G";
		default:
			return L"L";
		}
	}

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

	afx_msg void OnCbnSelchangeComboSimplecredential();
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

	void EnableAdvancedOptions();
	BOOL IsUrlExist(CString &text);

	void AddConfigType(int &index, CString text, bool add = true);
	void AddSimpleCredential(int &index, CString text, bool add = true);
	void LoadList();
	CString Load(CString key);
	void Save(CString key, CString value);
	int DeleteOtherKeys(int type);
	bool SaveSimpleCredential(int type);

	int			m_ChangedMask;

	CComboBox	m_ctrlSimpleCredential;
	CListBox	m_ctrlUrlList;
	CComboBox	m_ctrlConfigType;
	CString		m_strUrl;
	CString		m_strHelper;
	CString		m_strUsername;
	BOOL		m_bUseHttpPath;
};
