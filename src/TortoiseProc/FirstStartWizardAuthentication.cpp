// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2016-2017, 2019 - TortoiseGit

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
#include "stdafx.h"
#include "TortoiseProc.h"
#include "FirstStartWizard.h"
#include "FirstStartWizardAuthentication.h"
#include "Git.h"
#include "Settings/SettingGitCredential.h"
#include "PathUtils.h"

#define WM_SETPAGEFOCUS WM_APP+2

IMPLEMENT_DYNAMIC(CFirstStartWizardAuthentication, CFirstStartWizardBasePage)

CFirstStartWizardAuthentication::CFirstStartWizardAuthentication() : CFirstStartWizardBasePage(CFirstStartWizardAuthentication::IDD)
, m_bNoSave(FALSE)
{
	m_psp.dwFlags |= PSP_DEFAULT | PSP_USEHEADERTITLE;
	m_psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_FIRSTSTART_AUTHENTICATIONTITLE);
}

CFirstStartWizardAuthentication::~CFirstStartWizardAuthentication()
{
}

void CFirstStartWizardAuthentication::DoDataExchange(CDataExchange* pDX)
{
	CFirstStartWizardBasePage::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_DONTSAVE, m_bNoSave);
	DDX_Control(pDX, IDC_COMBO_SIMPLECREDENTIAL, m_ctrlSimpleCredential);
	DDX_Control(pDX, IDC_COMBO_SSHCLIENT, m_ctrlSSHClient);
}

BEGIN_MESSAGE_MAP(CFirstStartWizardAuthentication, CFirstStartWizardBasePage)
	ON_BN_CLICKED(IDC_GENERATEPUTTYKEY, &CFirstStartWizardAuthentication::OnBnClickedGenerateputtykey)
	ON_BN_CLICKED(IDC_ADVANCEDCONFIGURATION, &CFirstStartWizardAuthentication::OnBnClickedAdvancedconfiguration)
	ON_BN_CLICKED(IDC_DONTSAVE, OnClickedNoSave)
	ON_MESSAGE(WM_SETPAGEFOCUS, OnDialogDisplayed)
	ON_NOTIFY(NM_CLICK, IDC_FIRSTSTART_SSHHINT, OnClickedLink)
END_MESSAGE_MAP()

void CFirstStartWizardAuthentication::OnClickedNoSave()
{
	UpdateData();
	m_ctrlSimpleCredential.EnableWindow(!m_bNoSave);
}

static bool IsToolBasename(const CString& toolname, LPCTSTR setting)
{
	if (toolname.CompareNoCase(setting) == 0)
		return true;

	if ((toolname + L".exe").CompareNoCase(setting) == 0)
		return true;

	return false;
}

static bool IsTool(const CString& toolname, LPCTSTR setting)
{
	if (IsToolBasename(toolname, setting))
		return true;

	if (!IsToolBasename(toolname, PathFindFileName(setting)))
		return false;

	if (PathIsRelative(setting) || !PathFileExists(setting))
		return false;

	return true;
}

BOOL CFirstStartWizardAuthentication::OnWizardFinish()
{
	UpdateData();

	CString sshclient = CRegString(L"Software\\TortoiseGit\\SSH");
	if (sshclient.IsEmpty())
		sshclient = CRegString(L"Software\\TortoiseGit\\SSH", L"", FALSE, HKEY_LOCAL_MACHINE);
	if (m_ctrlSSHClient.GetCurSel() == 0 && !(sshclient.IsEmpty() || IsTool(L"tortoisegitplink", sshclient) || IsTool(L"tortoiseplink", sshclient)))
		CRegString(L"Software\\TortoiseGit\\SSH") = CPathUtils::GetAppDirectory() + L"TortoiseGitPlink.exe";
	else if (m_ctrlSSHClient.GetCurSel() == 1 && !IsTool(L"ssh", sshclient))
		CRegString(L"Software\\TortoiseGit\\SSH") = L"ssh.exe";

	if (m_ctrlSimpleCredential.IsWindowEnabled() && !m_bNoSave && m_ctrlSimpleCredential.GetCurSel() != -1)
	{
		CAutoConfig config(true);
		int err = git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitGlobalXDGConfig()), GIT_CONFIG_LEVEL_XDG, nullptr, FALSE);
		if (!err && (PathFileExists(g_Git.GetGitGlobalConfig()) || !PathFileExists(g_Git.GetGitGlobalXDGConfig())))
			err = git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitGlobalConfig()), GIT_CONFIG_LEVEL_GLOBAL, nullptr, FALSE);
		if (err)
		{
			MessageBox(g_Git.GetLibGit2LastErr(), L"TortoiseGit", MB_ICONEXCLAMATION);
			return FALSE;
		}

		if (m_ctrlSimpleCredential.GetCurSel() == 0 && m_availableHelpers.at(0) == CString(MAKEINTRESOURCE(IDS_NONE)))
		{
			int ret = git_config_delete_entry(config, "credential.helper");
			if (ret != 0 && ret != GIT_ENOTFOUND)
			{
				MessageBox(g_Git.GetLibGit2LastErr(), L"TortoiseGit", MB_ICONEXCLAMATION);
				return FALSE;
			}
		}
		else if (git_config_set_string(config, "credential.helper", CUnicodeUtils::GetUTF8(m_availableHelpers.at(m_ctrlSimpleCredential.GetCurSel()))))
		{
			MessageBox(g_Git.GetLibGit2LastErr(), L"TortoiseGit", MB_ICONEXCLAMATION);
			return FALSE;
		}
	}

	return __super::OnWizardFinish();
}

static void AddHelper(CComboBox& combobox, STRING_VECTOR& availableHelpers, const CString& helper, const CString& selected = L"")
{
	availableHelpers.push_back(helper);
	int idx = combobox.AddString(helper);
	if (selected == helper)
		combobox.SetCurSel(idx);
}

BOOL CFirstStartWizardAuthentication::OnInitDialog()
{
	CFirstStartWizardBasePage::OnInitDialog();

	CString tmp;
	tmp.LoadString(IDS_FIRSTSTART_SSHHINT);
	GetDlgItem(IDC_FIRSTSTART_SSHHINT)->SetWindowText(tmp);

	CAutoConfig config(true);
	if (git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitGlobalConfig()), GIT_CONFIG_LEVEL_GLOBAL, nullptr, FALSE))
		MessageBox(g_Git.GetLibGit2LastErr(), L"TortoiseGit", MB_ICONEXCLAMATION);
	else
	{
		if (git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitGlobalXDGConfig()), GIT_CONFIG_LEVEL_XDG, nullptr, FALSE))
			MessageBox(g_Git.GetLibGit2LastErr(), L"TortoiseGit", MB_ICONEXCLAMATION);
	}
	if (!g_Git.ms_bCygwinGit && !g_Git.ms_bMsys2Git)
	{
		if (git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitProgramDataConfig()), GIT_CONFIG_LEVEL_PROGRAMDATA, nullptr, FALSE))
			MessageBox(g_Git.GetLibGit2LastErr(), L"TortoiseGit", MB_ICONEXCLAMATION);
	}
	if (git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitSystemConfig()), GIT_CONFIG_LEVEL_SYSTEM, nullptr, FALSE))
		MessageBox(g_Git.GetLibGit2LastErr(), L"TortoiseGit", MB_ICONEXCLAMATION);

	STRING_VECTOR defaultList;
	git_config_foreach_match(config, "credential\\.helper", CSettingGitCredential::GetCredentialDefaultUrlCallback, &defaultList);

	if (defaultList.size() <= 1 && (defaultList.empty() || defaultList.at(0)[0] == 'X' || defaultList.at(0)[0] == 'G'))
		AddHelper(m_ctrlSimpleCredential, m_availableHelpers, MAKEINTRESOURCE(IDS_NONE));
	CString selectedHelper;
	if (config.GetString(L"credential.helper", selectedHelper) == GIT_ENOTFOUND)
		m_ctrlSimpleCredential.SetCurSel(0);

	if (CSettingGitCredential::GCMExists())
		AddHelper(m_ctrlSimpleCredential, m_availableHelpers, L"manager", selectedHelper);
	if (CSettingGitCredential::WincredExists())
		AddHelper(m_ctrlSimpleCredential, m_availableHelpers, L"wincred", selectedHelper);
	if (CSettingGitCredential::WinstoreExists())
		AddHelper(m_ctrlSimpleCredential, m_availableHelpers, CSettingGitCredential::GetWinstorePath(), selectedHelper);

	STRING_VECTOR urlList;
	git_config_foreach_match(config, "credential\\..*\\.helper", CSettingGitCredential::GetCredentialUrlCallback, &urlList);
	if (!urlList.empty() || m_ctrlSimpleCredential.GetCurSel() == -1)
	{
		m_ctrlSimpleCredential.EnableWindow(FALSE);
		m_availableHelpers.clear();
		m_ctrlSimpleCredential.ResetContent();
		AddHelper(m_ctrlSimpleCredential, m_availableHelpers, MAKEINTRESOURCE(IDS_ADVANCED));
		m_ctrlSimpleCredential.SetCurSel(0);
		GetDlgItem(IDC_DONTSAVE)->EnableWindow(FALSE);
	}

	AdjustControlSize(IDC_DONTSAVE);

	CString sshclient = CRegString(L"Software\\TortoiseGit\\SSH");
	if (sshclient.IsEmpty())
		sshclient = CRegString(L"Software\\TortoiseGit\\SSH", L"", FALSE, HKEY_LOCAL_MACHINE);

	int idx = m_ctrlSSHClient.AddString(L"TortoiseGitPlink");
	if (sshclient.IsEmpty() || IsTool(L"tortoisegitplink", sshclient) || IsTool(L"tortoiseplink", sshclient))
		m_ctrlSSHClient.SetCurSel(idx);
	idx = m_ctrlSSHClient.AddString(L"OpenSSH");
	if (IsTool(L"ssh", sshclient))
		m_ctrlSSHClient.SetCurSel(idx);
	if (m_ctrlSSHClient.GetCurSel() == -1)
	{
		idx = m_ctrlSSHClient.AddString(sshclient);
		m_ctrlSSHClient.SetCurSel(idx);
	}

	// TODO: Hide the button if PuTTY is not used?
	//GetDlgItem(IDC_GENERATEPUTTYKEY)->ShowWindow(CAppUtils::IsSSHPutty() ? SW_SHOW : SW_HIDE);

	UpdateData(FALSE);

	return TRUE;
}

BOOL CFirstStartWizardAuthentication::OnSetActive()
{
	auto wiz = static_cast<CFirstStartWizard*>(GetParent());

	wiz->SetWizardButtons(PSWIZB_FINISH | PSWIZB_BACK);

	PostMessage(WM_SETPAGEFOCUS, 0, 0);

	return CFirstStartWizardBasePage::OnSetActive();
}

void CFirstStartWizardAuthentication::OnBnClickedGenerateputtykey()
{
	CCommonAppUtils::LaunchApplication(CPathUtils::GetAppDirectory() + L"puttygen.exe", 0, false);
}

void CFirstStartWizardAuthentication::OnBnClickedAdvancedconfiguration()
{
	UpdateData();
	m_bNoSave = TRUE;
	UpdateData(FALSE);
	OnClickedNoSave();
	CAppUtils::RunTortoiseGitProc(L"/command:settings /page:gitcredential", false, false);
}

LRESULT CFirstStartWizardAuthentication::OnDialogDisplayed(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	GetDlgItem(IDC_COMBO_SSHCLIENT)->SetFocus();

	return 0;
}

void CFirstStartWizardAuthentication::OnClickedLink(NMHDR* pNMHDR, LRESULT* pResult)
{
	ATLASSERT(pNMHDR && pResult);
	auto pNMLink = reinterpret_cast<PNMLINK>(pNMHDR);
	if (wcscmp(pNMLink->item.szID, L"manual") == 0)
		HtmlHelp(0x20000 + IDD_FIRSTSTARTWIZARD_AUTHENTICATION);
	else if (wcscmp(pNMLink->item.szID, L"sshfaq") == 0)
	{
		CString helppath(theApp.m_pszHelpFilePath);
		helppath += L"::/tgit-ssh-howto.html";
		::HtmlHelp(GetSafeHwnd(), helppath, HH_DISPLAY_TOPIC, 0);
	}
	*pResult = 0;
}
