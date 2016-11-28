// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2016 - TortoiseGit

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
#include "FirstStartWizardGit.h"
#include "Git.h"
#include "MessageBox.h"
#include "GitForWindows.h"
#include "..\..\TGitCache\CacheInterface.h"

IMPLEMENT_DYNAMIC(CFirstStartWizardGit, CFirstStartWizardBasePage)

CFirstStartWizardGit::CFirstStartWizardGit() : CFirstStartWizardBasePage(CFirstStartWizardGit::IDD)
{
	m_psp.dwFlags |= PSP_DEFAULT | PSP_USEHEADERTITLE;
	m_psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_FIRSTSTART_GITTITLE);

	g_Git.CheckMsysGitDir(TRUE);
	m_regMsysGitPath = CRegString(REG_MSYSGIT_PATH);

	m_regMsysGitExtranPath = CRegString(REG_MSYSGIT_EXTRA_PATH);

	m_sMsysGitPath = m_regMsysGitPath;
	m_sMsysGitExtranPath = m_regMsysGitExtranPath;
}

CFirstStartWizardGit::~CFirstStartWizardGit()
{
}

void CFirstStartWizardGit::DoDataExchange(CDataExchange* pDX)
{
	CFirstStartWizardBasePage::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_MSYSGIT_PATH, m_sMsysGitPath);
	DDX_Text(pDX, IDC_MSYSGIT_EXTERN_PATH, m_sMsysGitExtranPath);
	DDX_Control(pDX, IDC_LINK, m_link);
}

BEGIN_MESSAGE_MAP(CFirstStartWizardGit, CFirstStartWizardBasePage)
	ON_BN_CLICKED(IDC_MSYSGIT_BROWSE, OnBrowseDir)
	ON_BN_CLICKED(IDC_MSYSGIT_CHECK, OnCheck)
END_MESSAGE_MAP()

LRESULT CFirstStartWizardGit::OnWizardNext()
{
	UpdateData();

	PerformCommonGitPathCleanup(m_sMsysGitPath);
	UpdateData(FALSE);

	if (m_sMsysGitPath.Compare(CString(m_regMsysGitPath)) || m_sMsysGitExtranPath.Compare(CString(m_regMsysGitExtranPath)))
	{
		StoreSetting(GetSafeHwnd(), m_sMsysGitPath, m_regMsysGitPath);
		StoreSetting(GetSafeHwnd(), m_sMsysGitExtranPath, m_regMsysGitExtranPath);
		SendCacheCommand(TGITCACHECOMMAND_END);
	}

	// only complete if the msysgit directory is ok
	if (!CheckGitExe(GetSafeHwnd(), m_sMsysGitPath, m_sMsysGitExtranPath, IDC_MSYSGIT_VER, [&](UINT helpid) { HtmlHelp(0x20000 + helpid); }))
		return -1;

	return __super::OnWizardNext();
}

BOOL CFirstStartWizardGit::OnInitDialog()
{
	CFirstStartWizardBasePage::OnInitDialog();

	SHAutoComplete(GetDlgItem(IDC_MSYSGIT_PATH)->m_hWnd, SHACF_FILESYSTEM);

	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_MSYSGIT_EXTERN_PATH, IDS_EXTRAPATH_TT);

	GetDlgItem(IDC_LINK)->SetWindowText(GIT_FOR_WINDOWS_URL);
	m_link.SetURL(GIT_FOR_WINDOWS_URL);

	AdjustControlSize(IDC_LINK, false);

	return TRUE;
}

BOOL CFirstStartWizardGit::OnSetActive()
{
	CFirstStartWizard* wiz = (CFirstStartWizard*)GetParent();

	wiz->SetWizardButtons(PSWIZB_NEXT | PSWIZB_BACK);

	return CFirstStartWizardBasePage::OnSetActive();
}

void CFirstStartWizardGit::OnMsysGitPathModify()
{
	UpdateData();
	if (GuessExtraPath(m_sMsysGitPath, m_sMsysGitExtranPath))
		UpdateData(FALSE);
}

void CFirstStartWizardGit::OnBrowseDir()
{
	UpdateData(TRUE);

	if (!SelectFolder(GetSafeHwnd(), m_sMsysGitPath, m_sMsysGitExtranPath))
		return;

	UpdateData(FALSE);
}

void CFirstStartWizardGit::OnCheck()
{
	UpdateData();

	CheckGitExe(GetSafeHwnd(), m_sMsysGitPath, m_sMsysGitExtranPath, IDC_MSYSGIT_VER, [&](UINT helpid) { HtmlHelp(0x20000 + helpid); });
}

BOOL CFirstStartWizardGit::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return __super::PreTranslateMessage(pMsg);
}
