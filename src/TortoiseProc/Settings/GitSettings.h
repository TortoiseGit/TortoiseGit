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
#include "../TortoiseProc.h"
#include "Git.h"
#include "Tooltip.h"
#include "../../Utils/UnicodeUtils.h"

class CSettings;

class CGitSettings
{
public:
	CGitSettings()
	: m_iConfigSource(0)
	, m_bGlobal(false)
	, m_bIsBareRepo(false)
	, m_bHonorProjectConfig(false)
	{
	}

protected:
	CComboBox m_cSaveTo;
	int		m_iConfigSource;
	bool	m_bGlobal;
	bool	m_bIsBareRepo;
	bool	m_bHonorProjectConfig;

	void InitGitSettings(ISettingsPropPage *page, bool honorProjectConfig, CToolTips * tooltips)
	{
		m_bHonorProjectConfig = honorProjectConfig;

		if (tooltips)
		{
			tooltips->AddTool(IDC_RADIO_SETTINGS_LOCAL, IDS_CONFIG_LOCAL_TT);
			if (page->GetDlgItem(IDC_RADIO_SETTINGS_PROJECT) != nullptr)
				tooltips->AddTool(IDC_RADIO_SETTINGS_PROJECT, IDS_CONFIG_PROJECT_TT);
			tooltips->AddTool(IDC_RADIO_SETTINGS_GLOBAL, IDS_CONFIG_GLOBAL_TT);
		}

		CString str = g_Git.m_CurrentDir;
		m_bIsBareRepo = g_GitAdminDir.IsBareRepo(str);
		CString proj;
		if (g_GitAdminDir.HasAdminDir(str, &proj) || m_bIsBareRepo)
		{
			CString title;
			page->GetWindowText(title);
			page->SetWindowText(title + _T(" - ") + proj);
			page->GetDlgItem(IDC_RADIO_SETTINGS_LOCAL)->EnableWindow(TRUE);

			m_cSaveTo.AddString(CString(MAKEINTRESOURCE(IDS_CONFIG_LOCAL)));

			if (page->GetDlgItem(IDC_RADIO_SETTINGS_PROJECT) != nullptr)
			{
				page->GetDlgItem(IDC_RADIO_SETTINGS_PROJECT)->EnableWindow(honorProjectConfig);
				if (honorProjectConfig && !m_bIsBareRepo)
					m_cSaveTo.AddString(CString(MAKEINTRESOURCE(IDS_CONFIG_PROJECT)));
			}
		}
		else
		{
			m_bGlobal = true;
			page->GetDlgItem(IDC_RADIO_SETTINGS_LOCAL)->EnableWindow(FALSE);
			if (page->GetDlgItem(IDC_RADIO_SETTINGS_PROJECT) != nullptr)
				page->GetDlgItem(IDC_RADIO_SETTINGS_PROJECT)->EnableWindow(FALSE);
		}
		m_cSaveTo.AddString(CString(MAKEINTRESOURCE(IDS_CONFIG_GLOBAL)));
		m_cSaveTo.SetCurSel(0);

		page->CheckRadioButton(IDC_RADIO_SETTINGS_EFFECTIVE, IDC_RADIO_SETTINGS_SYSTEM, IDC_RADIO_SETTINGS_EFFECTIVE + m_iConfigSource);

		CMessageBox::ShowCheck(nullptr, IDS_HIERARCHICALCONFIG, IDS_APPNAME, MB_ICONINFORMATION | MB_OK, _T("HintHierarchicalConfig"), IDS_MSGBOX_DONOTSHOWAGAIN);

		LoadData();
	}

	bool Save(git_config * config, const CString &key, const CString &value, const bool deleteKey = false)
	{
		CStringA keyA = CUnicodeUtils::GetUTF8(key);
		int err = 0;
		if (deleteKey)
		{
			const git_config_entry * entry = nullptr;
			if (git_config_get_entry(&entry, config, keyA) == GIT_ENOTFOUND)
				return true;
			err = git_config_delete_entry(config, keyA);
		}
		else
		{
			CStringA valueA = CUnicodeUtils::GetMulti(value, CP_UTF8);
			err = git_config_set_string(config, keyA, valueA);
		}
		if (err)
		{
			CString msg;
			msg.Format(IDS_PROC_SAVECONFIGFAILED, key, value);
			CMessageBox::Show(nullptr, g_Git.GetLibGit2LastErr(msg), _T("TortoiseGit"), MB_OK | MB_ICONERROR);
			return false;
		}
		return true;
	}

	void LoadData()
	{
		git_config * config;
		git_config_new(&config);
		if (!m_bGlobal && (m_iConfigSource == 0 || m_iConfigSource == 1))
		{
			if (git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitLocalConfig()), GIT_CONFIG_LEVEL_APP, FALSE)) // this needs to have the highest priority in order to override .tgitconfig settings
				MessageBox(nullptr, g_Git.GetLibGit2LastErr(), _T("TortoiseGit"), MB_ICONEXCLAMATION);
		}
		if ((m_iConfigSource == 0 && m_bHonorProjectConfig) || m_iConfigSource == 2)
		{
			if (!m_bIsBareRepo)
			{
				if (git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.m_CurrentDir + L"\\.tgitconfig"), GIT_CONFIG_LEVEL_LOCAL, FALSE)) // this needs to have the second highest priority
					MessageBox(nullptr, g_Git.GetLibGit2LastErr(), _T("TortoiseGit"), MB_ICONEXCLAMATION);
			}
			else
			{
				CString tmpFile = GetTempFile();
				CTGitPath path(_T(".tgitconfig"));
				if (g_Git.GetOneFile(_T("HEAD"), path, tmpFile) == 0)
				{
					if (git_config_add_file_ondisk(config, CGit::GetGitPathStringA(tmpFile), GIT_CONFIG_LEVEL_LOCAL, FALSE)) // this needs to have the second highest priority
						MessageBox(nullptr, g_Git.GetLibGit2LastErr(), _T("TortoiseGit"), MB_ICONEXCLAMATION);
				}
			}
		}
		if (m_iConfigSource == 0 || m_iConfigSource == 3)
		{
			if (git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitGlobalConfig()), GIT_CONFIG_LEVEL_GLOBAL, FALSE))
				MessageBox(nullptr, g_Git.GetLibGit2LastErr(), _T("TortoiseGit"), MB_ICONEXCLAMATION);
			else
			{
				if (git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitGlobalXDGConfig()), GIT_CONFIG_LEVEL_XDG, FALSE))
					MessageBox(nullptr, g_Git.GetLibGit2LastErr(), _T("TortoiseGit"), MB_ICONEXCLAMATION);
			}
		}
		if (m_iConfigSource == 0 || m_iConfigSource == 4)
		{
			if (git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitSystemConfig()), GIT_CONFIG_LEVEL_SYSTEM, FALSE))
				MessageBox(nullptr, g_Git.GetLibGit2LastErr(), _T("TortoiseGit"), MB_ICONEXCLAMATION);
		}

		LoadDataImpl(config);

		git_config_free(config);

		EnDisableControls();
	}

	bool WarnUserSafeToDifferentDestination(int storeTo)
	{
		if ((storeTo == IDS_CONFIG_GLOBAL && m_iConfigSource != 3) || (storeTo == IDS_CONFIG_PROJECT && m_iConfigSource != 2) || (storeTo == IDS_CONFIG_LOCAL && m_iConfigSource != 1))
		{
			CString dest;
			dest.LoadString(storeTo);
			CString msg;
			msg.Format(IDS_WARNUSERSAFEDIFFERENT, dest);
			if (CMessageBox::Show(nullptr, msg, _T("TortoiseGit"), 2, IDI_QUESTION, CString(MAKEINTRESOURCE(IDS_SAVEBUTTON)), CString(MAKEINTRESOURCE(IDS_ABORTBUTTON))) == 2)
				return false;
		}
		return true;
	}

	BOOL SafeData()
	{
		git_config * config;
		git_config_new(&config);

		int err = 0;
		if (m_bGlobal || (m_cSaveTo.GetCurSel() == 1 && (!m_bHonorProjectConfig || m_bIsBareRepo)) || m_cSaveTo.GetCurSel() == 2)
		{
			if (!WarnUserSafeToDifferentDestination(IDS_CONFIG_GLOBAL))
			{
				git_config_free(config);
				return FALSE;
			}
			if (PathIsDirectory(g_Git.GetGitGlobalXDGConfigPath()))
				err = git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitGlobalXDGConfig()), GIT_CONFIG_LEVEL_XDG, FALSE);
			else
				err = git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitGlobalConfig()), GIT_CONFIG_LEVEL_GLOBAL, FALSE);
		}
		else if (m_cSaveTo.GetCurSel() == 1 && !m_bIsBareRepo && m_bHonorProjectConfig)
		{
			if (!WarnUserSafeToDifferentDestination(IDS_CONFIG_PROJECT))
			{
				git_config_free(config);
				return FALSE;
			}
			err = git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.m_CurrentDir + L"\\.tgitconfig"), GIT_CONFIG_LEVEL_APP, FALSE);
		}
		else
		{
			if (!WarnUserSafeToDifferentDestination(IDS_CONFIG_LOCAL))
			{
				git_config_free(config);
				return FALSE;
			}
			err = git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitLocalConfig()), GIT_CONFIG_LEVEL_LOCAL, FALSE);
		}
		if (err)
		{
			MessageBox(nullptr, g_Git.GetLibGit2LastErr(), _T("TortoiseGit"), MB_ICONEXCLAMATION);
			return FALSE;
		}

		int ret = SafeDataImpl(config);

		git_config_free(config);

		return ret;
	}

	virtual void LoadDataImpl(git_config * config) = 0;
	virtual BOOL SafeDataImpl(git_config * config) = 0;
	virtual void EnDisableControls() = 0;

	static void AddTrueFalseToComboBox(CComboBox &combobox)
	{
		combobox.AddString(_T(""));
		combobox.AddString(_T("true"));
		combobox.AddString(_T("false"));
	}

	static void GetBoolConfigValueComboBox(git_config * config, const CString &key, CComboBox &combobox)
	{
		CStringA keyA = CUnicodeUtils::GetUTF8(key);
		BOOL out = 0;
		if (git_config_get_bool(&out, config, keyA) == GIT_ENOTFOUND)
			combobox.SetCurSel(0);
		else if (out)
			combobox.SetCurSel(1);
		else
			combobox.SetCurSel(2);
	}

	static int GetConfigValue(git_config * config, const CString &key, CString &value)
	{
		CStringA keyA = CUnicodeUtils::GetUTF8(key);
		const char *out = nullptr;
		int retval = git_config_get_string(&out, config, keyA);
		value = CUnicodeUtils::GetUnicode((CStringA)out);
		return retval;
	}
};

#define GITSETTINGS_DDX \
	DDX_Control(pDX, IDC_COMBO_SETTINGS_SAFETO, m_cSaveTo);

#define GITSETTINGS_RADIO_EVENT \
	ON_CBN_SELCHANGE(IDC_COMBO_SETTINGS_SAFETO, &OnChange) \
	ON_BN_CLICKED(IDC_RADIO_SETTINGS_EFFECTIVE, &OnBnClickedChangedConfigSource) \
	ON_BN_CLICKED(IDC_RADIO_SETTINGS_LOCAL, &OnBnClickedChangedConfigSource) \
	ON_BN_CLICKED(IDC_RADIO_SETTINGS_PROJECT, &OnBnClickedChangedConfigSource) \
	ON_BN_CLICKED(IDC_RADIO_SETTINGS_GLOBAL, &OnBnClickedChangedConfigSource) \
	ON_BN_CLICKED(IDC_RADIO_SETTINGS_SYSTEM, &OnBnClickedChangedConfigSource)

#define GITSETTINGS_RADIO_EVENT_HANDLE \
	afx_msg void OnBnClickedChangedConfigSource() \
	{ \
		m_iConfigSource = GetCheckedRadioButton(IDC_RADIO_SETTINGS_EFFECTIVE, IDC_RADIO_SETTINGS_SYSTEM) - IDC_RADIO_SETTINGS_EFFECTIVE; \
		if (m_iConfigSource == 1) \
			m_cSaveTo.SelectString(0, CString(MAKEINTRESOURCE(IDS_CONFIG_LOCAL))); \
		else if (m_iConfigSource == 2) \
			m_cSaveTo.SelectString(0, CString(MAKEINTRESOURCE(IDS_CONFIG_PROJECT))); \
		else if (m_iConfigSource == 3) \
			m_cSaveTo.SelectString(0, CString(MAKEINTRESOURCE(IDS_CONFIG_GLOBAL))); \
		LoadData(); \
	}
