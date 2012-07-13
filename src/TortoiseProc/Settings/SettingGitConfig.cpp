// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2012 - TortoiseGit

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
#include "MessageBox.h"
#include "ProjectProperties.h"
#include "AppUtils.h"
#include "PathUtils.h"
// CSettingGitConfig dialog

IMPLEMENT_DYNAMIC(CSettingGitConfig, ISettingsPropPage)

CSettingGitConfig::CSettingGitConfig()
	: ISettingsPropPage(CSettingGitConfig::IDD)
	, m_UserName(_T(""))
	, m_UserEmail(_T(""))
	, m_UserSigningKey(_T(""))
	, m_bGlobal(FALSE)
	, m_bAutoCrlf(FALSE)
	, m_bWarnNoSignedOffBy(FALSE)
{
	m_ChangeMask=0;
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
	DDX_Check(pDX, IDC_CHECK_WARN_NO_SIGNED_OFF_BY, m_bWarnNoSignedOffBy);
}

BEGIN_MESSAGE_MAP(CSettingGitConfig, CPropertyPage)
	ON_BN_CLICKED(IDC_CHECK_GLOBAL, &CSettingGitConfig::OnBnClickedCheckGlobal)
	ON_EN_CHANGE(IDC_GIT_USERNAME, &CSettingGitConfig::OnEnChangeGitUsername)
	ON_EN_CHANGE(IDC_GIT_USEREMAIL, &CSettingGitConfig::OnEnChangeGitUseremail)
	ON_EN_CHANGE(IDC_GIT_USERESINGNINGKEY, &CSettingGitConfig::OnEnChangeGitUserSigningKey)
	ON_BN_CLICKED(IDC_CHECK_AUTOCRLF, &CSettingGitConfig::OnBnClickedCheckAutocrlf)
	ON_CBN_SELCHANGE(IDC_COMBO_SAFECRLF, &CSettingGitConfig::OnCbnSelchangeSafeCrLf)
	ON_BN_CLICKED(IDC_EDITGLOBALGITCONFIG, &CSettingGitConfig::OnBnClickedEditglobalgitconfig)
	ON_BN_CLICKED(IDC_EDITLOCALGITCONFIG, &CSettingGitConfig::OnBnClickedEditlocalgitconfig)
	ON_BN_CLICKED(IDC_CHECK_WARN_NO_SIGNED_OFF_BY, &CSettingGitConfig::OnBnClickedCheckWarnNoSignedOffBy)
	ON_BN_CLICKED(IDC_EDITSYSTEMGITCONFIG, &CSettingGitConfig::OnBnClickedEditsystemgitconfig)
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

	ProjectProperties::GetBOOLProps(this->m_bAutoCrlf, _T("core.autocrlf"));
	BOOL bSafeCrLf = FALSE;
	ProjectProperties::GetBOOLProps(bSafeCrLf, _T("core.safecrlf"));
	if (bSafeCrLf)
		m_cSafeCrLf.SetCurSel(1);
	else
	{
		CString sSafeCrLf;
		ProjectProperties::GetStringProps(sSafeCrLf, _T("core.safecrlf"));
		sSafeCrLf = sSafeCrLf.MakeLower().Trim();
		if (sSafeCrLf == _T("warn"))
			m_cSafeCrLf.SetCurSel(2);
		else
			m_cSafeCrLf.SetCurSel(0);
	}
	ProjectProperties::GetBOOLProps(this->m_bWarnNoSignedOffBy, _T("tgit.warnnosignedoffby"));

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
		this->GetDlgItem(IDC_EDITLOCALGITCONFIG)->SetWindowText(CString(MAKEINTRESOURCE(IDS_PROC_GITCONFIG_EDITLOCALGONCFIG)));

	this->UpdateData(FALSE);
	return TRUE;
}
// CSettingGitConfig message handlers

void CSettingGitConfig::OnBnClickedCheckGlobal()
{
	SetModified();
}

void CSettingGitConfig::OnEnChangeGitUsername()
{
	m_ChangeMask |= GIT_NAME;
	SetModified();
}

void CSettingGitConfig::OnEnChangeGitUseremail()
{
	m_ChangeMask |= GIT_EMAIL;
	SetModified();
}

void CSettingGitConfig::OnEnChangeGitUserSigningKey()
{
	m_ChangeMask |= GIT_SIGNINGKEY;
	SetModified();
}

BOOL CSettingGitConfig::OnApply()
{
	CString cmd, out;
	CONFIG_TYPE type=CONFIG_LOCAL;
	this->UpdateData(FALSE);

	if(this->m_bGlobal)
		type = CONFIG_GLOBAL;

	if(m_ChangeMask&GIT_NAME)
		if (!Save(_T("user.name"), this->m_UserName, type))
			return FALSE;

	if(m_ChangeMask&GIT_EMAIL)
		if (!Save(_T("user.email"), this->m_UserEmail,type))
			return FALSE;

	if(m_ChangeMask&GIT_SIGNINGKEY)
		if (!Save(_T("user.signingkey"), this->m_UserSigningKey, type))
			return FALSE;

	if(m_ChangeMask&GIT_WARNNOSIGNEDOFFBY)
		if (!Save(_T("tgit.warnnosignedoffby"), this->m_bWarnNoSignedOffBy ? _T("true") : _T("false"), type))
			return FALSE;

	if(m_ChangeMask&GIT_CRLF)
		if (!Save(_T("core.autocrlf"), this->m_bAutoCrlf ? _T("true") : _T("false"), type))
			return FALSE;

	if(m_ChangeMask&GIT_SAFECRLF)
	{
		CString safecrlf;
		this->m_cSafeCrLf.GetWindowText(safecrlf);
		if (!Save(_T("core.safecrlf"), safecrlf, type))
			return FALSE;
	}

	m_ChangeMask = 0;
	SetModified(FALSE);
	return ISettingsPropPage::OnApply();
}
bool CSettingGitConfig::Save(CString key, CString value, CONFIG_TYPE type)
{
	CString out;
	if (g_Git.SetConfigValue(key, value, type, g_Git.GetGitEncode(L"i18n.commitencoding"), &g_Git.m_CurrentDir))
	{
		CString msg;
		msg.Format(IDS_PROC_SAVECONFIGFAILED, key, value);
		CMessageBox::Show(NULL, msg, _T("TortoiseGit"), MB_OK | MB_ICONERROR);
		return false;
	}
	return true;
}
void CSettingGitConfig::OnBnClickedCheckWarnNoSignedOffBy()
{
	m_ChangeMask |= GIT_WARNNOSIGNEDOFFBY;
	SetModified();
}
void CSettingGitConfig::OnBnClickedCheckAutocrlf()
{
	m_ChangeMask |= GIT_CRLF;
	SetModified();
}

void CSettingGitConfig::OnCbnSelchangeSafeCrLf()
{
	m_ChangeMask |= GIT_SAFECRLF;
	SetModified();
}

void CSettingGitConfig::OnBnClickedEditglobalgitconfig()
{
	CString filename = g_Git.GetHomeDirectory() + _T("\\.gitconfig");
	// use alternative editor because of LineEndings
	CAppUtils::LaunchAlternativeEditor(filename);
}

void CSettingGitConfig::OnBnClickedEditlocalgitconfig()
{
	CString path;
	g_GitAdminDir.GetAdminDirPath(g_Git.m_CurrentDir, path);
	path += _T("config");
	// use alternative editor because of LineEndings
	CAppUtils::LaunchAlternativeEditor(path);
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
	CAppUtils::LaunchAlternativeEditor(filename);
}
