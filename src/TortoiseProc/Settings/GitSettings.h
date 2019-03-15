// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013-2019 - TortoiseGit

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
#include "TortoiseProc.h"
#include "Git.h"
#include "Tooltip.h"
#include "UnicodeUtils.h"
#include "TempFile.h"

class CSettings;

class CGitSettings
{
public:
	CGitSettings()
	: m_iConfigSource(CFG_SRC_EFFECTIVE)
	, m_bGlobal(false)
	, m_bIsBareRepo(false)
	, m_bHonorProjectConfig(false)
	{
	}

protected:
	CComboBox m_cSaveTo;
	enum
	{
		CFG_SRC_EFFECTIVE = 0,
		CFG_SRC_LOCAL = 1,
		CFG_SRC_PROJECT = 2,
		CFG_SRC_GLOBAL = 3,
		CFG_SRC_SYSTEM = 4,
	};
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
		m_bIsBareRepo = GitAdminDir::IsBareRepo(str);
		CString proj;
		if (GitAdminDir::HasAdminDir(str, &proj) || m_bIsBareRepo)
		{
			CString title;
			page->GetWindowText(title);
			page->SetWindowText(title + L" - " + proj);
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

		CMessageBox::ShowCheck(GetDialogHwnd(), IDS_HIERARCHICALCONFIG, IDS_APPNAME, MB_ICONINFORMATION | MB_OK, L"HintHierarchicalConfig", IDS_MSGBOX_DONOTSHOWAGAIN);

		LoadData();
	}

	bool Save(git_config * config, const CString &key, const CString &value, const bool deleteKey = false)
	{
		CStringA keyA = CUnicodeUtils::GetUTF8(key);
		int err = 0;
		if (deleteKey)
		{
			git_config_entry* entry = nullptr;
			if (git_config_get_entry(&entry, config, keyA) == GIT_ENOTFOUND)
				return true;
			git_config_entry_free(entry);
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
			msg.FormatMessage(IDS_PROC_SAVECONFIGFAILED, static_cast<LPCTSTR>(key), static_cast<LPCTSTR>(value));
			CMessageBox::Show(GetDialogHwnd(), g_Git.GetLibGit2LastErr(msg), L"TortoiseGit", MB_OK | MB_ICONERROR);
			return false;
		}
		return true;
	}

	void LoadData()
	{
		CAutoRepository repo(g_Git.GetGitRepository());
		CAutoConfig config(true);
		if (!m_bGlobal && (m_iConfigSource == CFG_SRC_EFFECTIVE || m_iConfigSource == CFG_SRC_LOCAL))
		{
			if (git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitLocalConfig()), GIT_CONFIG_LEVEL_APP, repo, FALSE)) // this needs to have the highest priority in order to override .tgitconfig settings
				MessageBox(nullptr, g_Git.GetLibGit2LastErr(), L"TortoiseGit", MB_ICONEXCLAMATION);
		}
		if ((m_iConfigSource == CFG_SRC_EFFECTIVE && m_bHonorProjectConfig) || m_iConfigSource == CFG_SRC_PROJECT)
		{
			if (!m_bIsBareRepo)
			{
				if (git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.CombinePath(L".tgitconfig")), GIT_CONFIG_LEVEL_LOCAL, nullptr, FALSE)) // this needs to have the second highest priority
					MessageBox(nullptr, g_Git.GetLibGit2LastErr(), L"TortoiseGit", MB_ICONEXCLAMATION);
			}
			else
			{
				CString tmpFile = CTempFiles::Instance().GetTempFilePath(true).GetWinPathString();
				CTGitPath path(L".tgitconfig");
				if (g_Git.GetOneFile(L"HEAD", path, tmpFile) == 0)
				{
					if (git_config_add_file_ondisk(config, CGit::GetGitPathStringA(tmpFile), GIT_CONFIG_LEVEL_LOCAL, nullptr, FALSE)) // this needs to have the second highest priority
						MessageBox(nullptr, g_Git.GetLibGit2LastErr(), L"TortoiseGit", MB_ICONEXCLAMATION);
				}
			}
		}
		if (m_iConfigSource == CFG_SRC_EFFECTIVE || m_iConfigSource == CFG_SRC_GLOBAL)
		{
			if (git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitGlobalConfig()), GIT_CONFIG_LEVEL_GLOBAL, repo, FALSE))
				MessageBox(nullptr, g_Git.GetLibGit2LastErr(), L"TortoiseGit", MB_ICONEXCLAMATION);
			else
			{
				if (git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitGlobalXDGConfig()), GIT_CONFIG_LEVEL_XDG, repo, FALSE))
					MessageBox(nullptr, g_Git.GetLibGit2LastErr(), L"TortoiseGit", MB_ICONEXCLAMATION);
			}
		}
		if (m_iConfigSource == CFG_SRC_EFFECTIVE || m_iConfigSource == CFG_SRC_SYSTEM)
		{
			if (!g_Git.ms_bCygwinGit && !g_Git.ms_bMsys2Git)
			{
				if (git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitProgramDataConfig()), GIT_CONFIG_LEVEL_PROGRAMDATA, repo, FALSE))
					MessageBox(nullptr, g_Git.GetLibGit2LastErr(), L"TortoiseGit", MB_ICONEXCLAMATION);
			}
			if (git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitSystemConfig()), GIT_CONFIG_LEVEL_SYSTEM, repo, FALSE))
				MessageBox(nullptr, g_Git.GetLibGit2LastErr(), L"TortoiseGit", MB_ICONEXCLAMATION);
		}

		LoadDataImpl(config);

		EnDisableControls();
	}

	bool WarnUserSafeToDifferentDestination(int storeTo)
	{
		if ((storeTo == IDS_CONFIG_GLOBAL && m_iConfigSource != CFG_SRC_GLOBAL) || (storeTo == IDS_CONFIG_PROJECT && m_iConfigSource != CFG_SRC_PROJECT) || (storeTo == IDS_CONFIG_LOCAL && m_iConfigSource != CFG_SRC_LOCAL))
		{
			CString dest;
			dest.LoadString(storeTo);
			CString msg;
			msg.Format(IDS_WARNUSERSAFEDIFFERENT, static_cast<LPCTSTR>(dest));
			if (CMessageBox::Show(GetDialogHwnd(), msg, L"TortoiseGit", 2, IDI_QUESTION, CString(MAKEINTRESOURCE(IDS_SAVEBUTTON)), CString(MAKEINTRESOURCE(IDS_ABORTBUTTON))) == 2)
				return false;
		}
		return true;
	}

	BOOL SafeData()
	{
		CAutoConfig config(true);

		int err = 0;
		if (m_bGlobal || (m_cSaveTo.GetCurSel() == 1 && (!m_bHonorProjectConfig || m_bIsBareRepo)) || m_cSaveTo.GetCurSel() == 2)
		{
			if (!WarnUserSafeToDifferentDestination(IDS_CONFIG_GLOBAL))
				return FALSE;
			err = git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitGlobalXDGConfig()), GIT_CONFIG_LEVEL_XDG, nullptr, FALSE);
			if (!err && (PathFileExists(g_Git.GetGitGlobalConfig()) || !PathFileExists(g_Git.GetGitGlobalXDGConfig())))
				err = git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitGlobalConfig()), GIT_CONFIG_LEVEL_GLOBAL, nullptr, FALSE);
		}
		else if (m_cSaveTo.GetCurSel() == 1 && !m_bIsBareRepo && m_bHonorProjectConfig)
		{
			if (!WarnUserSafeToDifferentDestination(IDS_CONFIG_PROJECT))
				return FALSE;
			err = git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.CombinePath(L".tgitconfig")), GIT_CONFIG_LEVEL_APP, nullptr, FALSE);
		}
		else
		{
			if (!WarnUserSafeToDifferentDestination(IDS_CONFIG_LOCAL))
				return FALSE;
			err = git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitLocalConfig()), GIT_CONFIG_LEVEL_LOCAL, nullptr, FALSE);
		}
		if (err)
		{
			MessageBox(nullptr, g_Git.GetLibGit2LastErr(), L"TortoiseGit", MB_ICONEXCLAMATION);
			return FALSE;
		}

		return SafeDataImpl(config);
	}

	virtual void LoadDataImpl(CAutoConfig& config) = 0;
	virtual BOOL SafeDataImpl(CAutoConfig& config) = 0;
	virtual void EnDisableControls() = 0;
	virtual HWND GetDialogHwnd() const = 0;

	static void AddTrueFalseToComboBox(CComboBox &combobox)
	{
		combobox.AddString(L"");
		combobox.AddString(L"true");
		combobox.AddString(L"false");
	}

	static void GetBoolConfigValueComboBox(CAutoConfig& config, const CString &key, CComboBox &combobox)
	{
		BOOL out = 0;
		if (config.GetBOOL(key, out) == GIT_ENOTFOUND)
			combobox.SetCurSel(0);
		else if (out)
			combobox.SetCurSel(1);
		else
			combobox.SetCurSel(2);
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

#define GITSETTINGS_ADJUSTCONTROLSIZE \
	AdjustControlSize(IDC_RADIO_SETTINGS_EFFECTIVE); \
	AdjustControlSize(IDC_RADIO_SETTINGS_LOCAL); \
	AdjustControlSize(IDC_RADIO_SETTINGS_PROJECT); \
	AdjustControlSize(IDC_RADIO_SETTINGS_GLOBAL); \
	AdjustControlSize(IDC_RADIO_SETTINGS_SYSTEM);

#define GITSETTINGS_RADIO_EVENT_HANDLE \
	afx_msg void OnBnClickedChangedConfigSource() \
	{ \
		m_iConfigSource = GetCheckedRadioButton(IDC_RADIO_SETTINGS_EFFECTIVE, IDC_RADIO_SETTINGS_SYSTEM) - IDC_RADIO_SETTINGS_EFFECTIVE; \
		if (m_iConfigSource == CFG_SRC_LOCAL) \
			m_cSaveTo.SelectString(0, CString(MAKEINTRESOURCE(IDS_CONFIG_LOCAL))); \
		else if (m_iConfigSource == CFG_SRC_PROJECT) \
			m_cSaveTo.SelectString(0, CString(MAKEINTRESOURCE(IDS_CONFIG_PROJECT))); \
		else if (m_iConfigSource == CFG_SRC_GLOBAL) \
			m_cSaveTo.SelectString(0, CString(MAKEINTRESOURCE(IDS_CONFIG_GLOBAL))); \
		LoadData(); \
	}
