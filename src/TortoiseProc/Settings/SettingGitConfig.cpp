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
	, m_bAutoCrlf(FALSE)
	, m_bNeedSave(false)
	, m_bQuotePath(TRUE)
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
	DDX_Check(pDX, IDC_CHECK_AUTOCRLF, m_bAutoCrlf);
	DDX_Check(pDX, IDC_CHECK_QUOTEPATH, m_bQuotePath);
	DDX_Control(pDX, IDC_COMBO_SAFECRLF, m_cSafeCrLf);
	GITSETTINGS_DDX
}

BEGIN_MESSAGE_MAP(CSettingGitConfig, CPropertyPage)
	ON_EN_CHANGE(IDC_GIT_USERNAME, &CSettingGitConfig::OnChange)
	ON_EN_CHANGE(IDC_GIT_USEREMAIL, &CSettingGitConfig::OnChange)
	ON_EN_CHANGE(IDC_GIT_USERESINGNINGKEY, &CSettingGitConfig::OnChange)
	ON_BN_CLICKED(IDC_CHECK_AUTOCRLF, &CSettingGitConfig::OnChange)
	ON_BN_CLICKED(IDC_CHECK_QUOTEPATH, &CSettingGitConfig::OnChange)
	ON_CBN_SELCHANGE(IDC_COMBO_SAFECRLF, &CSettingGitConfig::OnChange)
	ON_BN_CLICKED(IDC_EDITGLOBALGITCONFIG, &CSettingGitConfig::OnBnClickedEditglobalgitconfig)
	ON_BN_CLICKED(IDC_EDITGLOBALXDGGITCONFIG, &CSettingGitConfig::OnBnClickedEditglobalxdggitconfig)
	ON_BN_CLICKED(IDC_EDITLOCALGITCONFIG, &CSettingGitConfig::OnBnClickedEditlocalgitconfig)
	ON_BN_CLICKED(IDC_EDITTGITCONFIG, &CSettingGitConfig::OnBnClickedEdittgitconfig)
	ON_BN_CLICKED(IDC_EDITSYSTEMGITCONFIG, &CSettingGitConfig::OnBnClickedEditsystemgitconfig)
	ON_BN_CLICKED(IDC_VIEWSYSTEMGITCONFIG, &CSettingGitConfig::OnBnClickedViewsystemgitconfig)
	GITSETTINGS_RADIO_EVENT
END_MESSAGE_MAP()

BOOL CSettingGitConfig::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

	m_cSafeCrLf.AddString(_T(""));
	m_cSafeCrLf.AddString(_T("false"));
	m_cSafeCrLf.AddString(_T("true"));
	m_cSafeCrLf.AddString(_T("warn"));

	m_tooltips.Create(this);

	InitGitSettings(this, false, &m_tooltips);

	if (!m_bGlobal || m_bIsBareRepo)
		this->GetDlgItem(IDC_EDITLOCALGITCONFIG)->EnableWindow(TRUE);
	else
		this->GetDlgItem(IDC_EDITLOCALGITCONFIG)->EnableWindow(FALSE);

	if (m_bIsBareRepo)
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

void CSettingGitConfig::LoadDataImpl(git_config * config)
{
	// special handling for UserName and UserEmail, because these can also be defined as environment variables for effective settings
	if (m_iConfigSource == 0)
	{
		m_UserName = g_Git.GetUserName();
		m_UserEmail = g_Git.GetUserEmail();
	}
	else
	{
		GetConfigValue(config, _T("user.name"), m_UserName);
		GetConfigValue(config, _T("user.email"), m_UserEmail);
	}
	GetConfigValue(config, _T("user.signingkey"), m_UserSigningKey);

	if (git_config_get_bool(&m_bAutoCrlf, config, "core.autocrlf") == GIT_ENOTFOUND)
		m_bAutoCrlf = BST_INDETERMINATE;

	if (git_config_get_bool(&m_bQuotePath, config, "core.quotepath") == GIT_ENOTFOUND)
		m_bQuotePath = BST_INDETERMINATE;

	BOOL bSafeCrLf = FALSE;
	if (git_config_get_bool(&bSafeCrLf, config, "core.safecrlf") == GIT_ENOTFOUND)
		m_cSafeCrLf.SetCurSel(0);
	else if (bSafeCrLf)
		m_cSafeCrLf.SetCurSel(2);
	else
	{
		CString sSafeCrLf;
		GetConfigValue(config, _T("core.safecrlf"), sSafeCrLf);
		sSafeCrLf = sSafeCrLf.MakeLower().Trim();
		if (sSafeCrLf == _T("warn"))
			m_cSafeCrLf.SetCurSel(3);
		else
			m_cSafeCrLf.SetCurSel(1);
	}

	m_bNeedSave = false;
	SetModified(FALSE);
	UpdateData(FALSE);
}

void CSettingGitConfig::OnChange()
{
	m_bNeedSave = true;
	SetModified();
}

BOOL CSettingGitConfig::SafeDataImpl(git_config * config)
{
	if (!Save(config, _T("user.name"), this->m_UserName))
		return FALSE;

	if (!Save(config, _T("user.email"), this->m_UserEmail))
		return FALSE;

	if (!Save(config, _T("user.signingkey"), this->m_UserSigningKey, true))
		return FALSE;

	if (!Save(config, _T("core.quotepath"), m_bQuotePath == BST_INDETERMINATE ? _T("") : m_bQuotePath ? _T("true") : _T("false")))
		return FALSE;

	if (!Save(config, _T("core.autocrlf"), m_bAutoCrlf == BST_INDETERMINATE ? _T("") : m_bAutoCrlf ? _T("true") : _T("false")))
		return FALSE;

	{
		CString safecrlf;
		this->m_cSafeCrLf.GetWindowText(safecrlf);
		if (!Save(config, _T("core.safecrlf"), safecrlf))
			return FALSE;
	}

	return TRUE;
}

BOOL CSettingGitConfig::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return ISettingsPropPage::PreTranslateMessage(pMsg);
}

BOOL CSettingGitConfig::OnApply()
{
	if (!m_bNeedSave)
		return TRUE;
	UpdateData();
	if (!SafeData())
		return FALSE;
	m_bNeedSave = false;
	SetModified(FALSE);
	return ISettingsPropPage::OnApply();
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
