// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2013 - TortoiseGit

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
// SettingGitConfig.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "SettingGitConfig.h"
#include "Git.h"
#include "Settings.h"
#include "GitAdminDir.h"
#include "AppUtils.h"
// CSettingGitConfig dialog

IMPLEMENT_DYNAMIC(CSettingGitConfig, ISettingsPropPage)

CSettingGitConfig::CSettingGitConfig()
	: ISettingsPropPage(CSettingGitConfig::IDD)
	, m_UserName(_T(""))
	, m_UserEmail(_T(""))
	, m_UserSigningKey(_T(""))
	, m_bGlobal(FALSE)
	, m_bAutoCrlf(FALSE)
	, m_bNeedSave(false)
{
}

CSettingGitConfig::~CSettingGitConfig()
{
}

void CSettingGitConfig::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_GIT_USERNAME, m_UserName);
	DDX_Text(pDX, IDC_GIT_USEREMAIL, m_UserEmail);
	DDX_Text(pDX, IDC_GIT_USERESINGNINGKEY, m_UserSigningKey);
	DDX_Check(pDX, IDC_CHECK_GLOBAL, m_bGlobal);
	DDX_Check(pDX, IDC_CHECK_AUTOCRLF, m_bAutoCrlf);
	DDX_Control(pDX, IDC_COMBO_SAFECRLF, m_cSafeCrLf);
}

BEGIN_MESSAGE_MAP(CSettingGitConfig, CPropertyPage)
	ON_BN_CLICKED(IDC_CHECK_GLOBAL, &CSettingGitConfig::OnChange)
	ON_EN_CHANGE(IDC_GIT_USERNAME, &CSettingGitConfig::OnChange)
	ON_EN_CHANGE(IDC_GIT_USEREMAIL, &CSettingGitConfig::OnChange)
	ON_EN_CHANGE(IDC_GIT_USERESINGNINGKEY, &CSettingGitConfig::OnChange)
	ON_BN_CLICKED(IDC_CHECK_AUTOCRLF, &CSettingGitConfig::OnChange)
	ON_CBN_SELCHANGE(IDC_COMBO_SAFECRLF, &CSettingGitConfig::OnChange)
	ON_BN_CLICKED(IDC_EDITGLOBALGITCONFIG, &CSettingGitConfig::OnBnClickedEditglobalgitconfig)
	ON_BN_CLICKED(IDC_EDITGLOBALXDGGITCONFIG, &CSettingGitConfig::OnBnClickedEditglobalxdggitconfig)
	ON_BN_CLICKED(IDC_EDITLOCALGITCONFIG, &CSettingGitConfig::OnBnClickedEditlocalgitconfig)
	ON_BN_CLICKED(IDC_EDITTGITCONFIG, &CSettingGitConfig::OnBnClickedEdittgitconfig)
	ON_BN_CLICKED(IDC_EDITSYSTEMGITCONFIG, &CSettingGitConfig::OnBnClickedEditsystemgitconfig)
	ON_BN_CLICKED(IDC_VIEWSYSTEMGITCONFIG, &CSettingGitConfig::OnBnClickedViewsystemgitconfig)
END_MESSAGE_MAP()

BOOL CSettingGitConfig::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

	m_cSafeCrLf.AddString(_T("false"));
	m_cSafeCrLf.AddString(_T("true"));
	m_cSafeCrLf.AddString(_T("warn"));

	m_UserName = g_Git.GetUserName();
	m_UserEmail = g_Git.GetUserEmail();
	m_UserSigningKey = g_Git.GetConfigValue(_T("user.signingkey"));

	m_bAutoCrlf = g_Git.GetConfigValueBool(_T("core.autocrlf"));
	bool bSafeCrLf = g_Git.GetConfigValueBool(_T("core.safecrlf"));
	if (bSafeCrLf)
		m_cSafeCrLf.SetCurSel(1);
	else
	{
		CString sSafeCrLf = g_Git.GetConfigValue(_T("core.safecrlf"));
		sSafeCrLf = sSafeCrLf.MakeLower().Trim();
		if (sSafeCrLf == _T("warn"))
			m_cSafeCrLf.SetCurSel(2);
		else
			m_cSafeCrLf.SetCurSel(0);
	}

	CString str = ((CSettings*)GetParent())->m_CmdPath.GetWinPath();
	bool isBareRepo = g_GitAdminDir.IsBareRepo(str);
	CString proj;
	if (g_GitAdminDir.HasAdminDir(str, &proj) || isBareRepo)
	{
		CString title;
		this->GetWindowText(title);
		this->SetWindowText(title + _T(" - ") + proj);
		this->GetDlgItem(IDC_CHECK_GLOBAL)->EnableWindow(TRUE);
		this->GetDlgItem(IDC_EDITLOCALGITCONFIG)->EnableWindow(TRUE);
	}
	else
	{
		m_bGlobal = TRUE;
		this->GetDlgItem(IDC_CHECK_GLOBAL)->EnableWindow(FALSE);
		this->GetDlgItem(IDC_EDITLOCALGITCONFIG)->EnableWindow(FALSE);
	}

	if (isBareRepo)
	{
		this->GetDlgItem(IDC_EDITLOCALGITCONFIG)->SetWindowText(CString(MAKEINTRESOURCE(IDS_PROC_GITCONFIG_EDITLOCALGONCFIG)));
		this->GetDlgItem(IDC_EDITTGITCONFIG)->SetWindowText(CString(MAKEINTRESOURCE(IDS_PROC_GITCONFIG_VIEWTGITCONFIG)));
	}

	if (!CAppUtils::IsAdminLogin())
	{
		((CButton *)this->GetDlgItem(IDC_EDITSYSTEMGITCONFIG))->SetShield(TRUE);
		this->GetDlgItem(IDC_VIEWSYSTEMGITCONFIG)->ShowWindow(SW_SHOW);
	}

	if (PathIsDirectory(g_Git.GetGitGlobalXDGConfigPath()))
		this->GetDlgItem(IDC_EDITGLOBALXDGGITCONFIG)->ShowWindow(SW_SHOW);

	this->UpdateData(FALSE);
	return TRUE;
}
// CSettingGitConfig message handlers

void CSettingGitConfig::OnChange()
{
	m_bNeedSave = true;
	SetModified();
}

BOOL CSettingGitConfig::OnApply()
{
	if (!m_bNeedSave)
		return TRUE;
	CONFIG_TYPE type=CONFIG_LOCAL;
	this->UpdateData(FALSE);

	if(this->m_bGlobal)
		type = CONFIG_GLOBAL;

	if (!Save(_T("user.name"), m_UserName, type))
			return FALSE;

	if (!Save(_T("user.email"), m_UserEmail, type))
		return FALSE;

	if (!Save(_T("user.signingkey"), m_UserSigningKey, type))
		return FALSE;

	if (!Save(_T("core.autocrlf"), m_bAutoCrlf ? _T("true") : _T("false"), type))
		return FALSE;

	{
		CString safecrlf;
		this->m_cSafeCrLf.GetWindowText(safecrlf);
		if (!Save(_T("core.safecrlf"), safecrlf, type))
			return FALSE;
	}

	m_bNeedSave = false;
	SetModified(FALSE);
	return ISettingsPropPage::OnApply();
}
bool CSettingGitConfig::Save(CString key, CString value, CONFIG_TYPE type)
{
	CString out;
	if (g_Git.SetConfigValue(key, value, type, g_Git.GetGitEncode(L"i18n.commitencoding")))
	{
		CString msg;
		msg.Format(IDS_PROC_SAVECONFIGFAILED, key, value);
		CMessageBox::Show(NULL, msg, _T("TortoiseGit"), MB_OK | MB_ICONERROR);
		return false;
	}
	return true;
}

void CSettingGitConfig::OnBnClickedEditglobalgitconfig()
{
	// use alternative editor because of LineEndings
	CAppUtils::LaunchAlternativeEditor(g_Git.GetGitGlobalConfig());
}

void CSettingGitConfig::OnBnClickedEditglobalxdggitconfig()
{
	// use alternative editor because of LineEndings
	CAppUtils::LaunchAlternativeEditor(g_Git.GetGitGlobalXDGConfig());
}

void CSettingGitConfig::OnBnClickedEditlocalgitconfig()
{
	// use alternative editor because of LineEndings
	CAppUtils::LaunchAlternativeEditor(g_Git.GetGitLocalConfig());
}

void CSettingGitConfig::OnBnClickedEdittgitconfig()
{
	// use alternative editor because of LineEndings
	if (g_GitAdminDir.IsBareRepo(g_Git.m_CurrentDir))
	{
		CString tmpFile = GetTempFile();
		CTGitPath path(_T(".tgitconfig"));
		if (g_Git.GetOneFile(_T("HEAD"), path, tmpFile) == 0)
		{
			CAppUtils::LaunchAlternativeEditor(tmpFile);
		}
	}
	else
	{
		CAppUtils::LaunchAlternativeEditor(g_Git.m_CurrentDir + _T("\\.tgitconfig"));
	}
}

void CSettingGitConfig::OnBnClickedEditsystemgitconfig()
{
	CString filename = g_Git.GetGitSystemConfig();
	if (filename.IsEmpty())
	{
		CMessageBox::Show(NULL, IDS_PROC_GITCONFIG_NOMSYSGIT, IDS_APPNAME, MB_ICONERROR);
		return;
	}
	// use alternative editor because of LineEndings
	CAppUtils::LaunchAlternativeEditor(filename, true);
}

void CSettingGitConfig::OnBnClickedViewsystemgitconfig()
{
	CString filename = g_Git.GetGitSystemConfig();
	if (filename.IsEmpty())
	{
		CMessageBox::Show(NULL, IDS_PROC_GITCONFIG_NOMSYSGIT, IDS_APPNAME, MB_ICONERROR);
		return;
	}
	// use alternative editor because of LineEndings
	CAppUtils::LaunchAlternativeEditor(filename);
}
