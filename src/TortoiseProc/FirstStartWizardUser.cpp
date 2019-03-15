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
#include "FirstStartWizardUser.h"
#include "Git.h"

IMPLEMENT_DYNAMIC(CFirstStartWizardUser, CFirstStartWizardBasePage)

CFirstStartWizardUser::CFirstStartWizardUser() : CFirstStartWizardBasePage(CFirstStartWizardUser::IDD)
, m_bNoSave(FALSE)
{
	m_psp.dwFlags |= PSP_DEFAULT | PSP_USEHEADERTITLE;
	m_psp.dwFlags &= ~PSP_HASHELP;
	m_psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_FIRSTSTART_USERTITLE);
}

CFirstStartWizardUser::~CFirstStartWizardUser()
{
}

void CFirstStartWizardUser::DoDataExchange(CDataExchange* pDX)
{
	CFirstStartWizardBasePage::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_DONTSAVE, m_bNoSave);
	DDX_Text(pDX, IDC_GIT_USERNAME, m_sUsername);
	DDX_Text(pDX, IDC_GIT_USEREMAIL, m_sUseremail);
}

BEGIN_MESSAGE_MAP(CFirstStartWizardUser, CFirstStartWizardBasePage)
	ON_BN_CLICKED(IDC_DONTSAVE, OnClickedNoSave)
END_MESSAGE_MAP()

void CFirstStartWizardUser::OnClickedNoSave()
{
	UpdateData();
	GetDlgItem(IDC_GIT_USERNAME)->EnableWindow(!m_bNoSave);
	GetDlgItem(IDC_GIT_USEREMAIL)->EnableWindow(!m_bNoSave);
}

LRESULT CFirstStartWizardUser::OnWizardNext()
{
	UpdateData();

	if (!m_bNoSave)
	{
		if (m_sUsername.Trim().IsEmpty() || m_sUseremail.Trim().IsEmpty())
		{
			MessageBox(L"Username and email must not be empty.", L"TortoiseGit", MB_ICONERROR);
			return -1;
		}

		CAutoConfig config(true);
		int err = git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitGlobalXDGConfig()), GIT_CONFIG_LEVEL_XDG, nullptr, FALSE);
		if (!err && (PathFileExists(g_Git.GetGitGlobalConfig()) || !PathFileExists(g_Git.GetGitGlobalXDGConfig())))
			err = git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitGlobalConfig()), GIT_CONFIG_LEVEL_GLOBAL, nullptr, FALSE);
		if (err)
		{
			MessageBox(g_Git.GetLibGit2LastErr(), L"TortoiseGit", MB_ICONEXCLAMATION);
			return -1;
		}
		if (git_config_set_string(config, "user.name", CUnicodeUtils::GetUTF8(m_sUsername)))
		{
			MessageBox(g_Git.GetLibGit2LastErr(), L"TortoiseGit", MB_ICONEXCLAMATION);
			return -1;
		}
		if (git_config_set_string(config, "user.email", CUnicodeUtils::GetUTF8(m_sUseremail)))
		{
			MessageBox(g_Git.GetLibGit2LastErr(), L"TortoiseGit", MB_ICONEXCLAMATION);
			return -1;
		}
	}

	return __super::OnWizardNext();
}

BOOL CFirstStartWizardUser::OnInitDialog()
{
	CFirstStartWizardBasePage::OnInitDialog();

	AdjustControlSize(IDC_DONTSAVE);

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

	config.GetString(L"user.name", m_sUsername);
	config.GetString(L"user.email", m_sUseremail);

	UpdateData(FALSE);

	return TRUE;
}

BOOL CFirstStartWizardUser::OnSetActive()
{
	auto wiz = static_cast<CFirstStartWizard*>(GetParent());

	wiz->SetWizardButtons(PSWIZB_NEXT | PSWIZB_BACK);

	return CFirstStartWizardBasePage::OnSetActive();
}
