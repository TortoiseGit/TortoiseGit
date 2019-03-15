// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2016-2019 - TortoiseGit

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
#include "../../TGitCache/CacheInterface.h"

IMPLEMENT_DYNAMIC(CFirstStartWizardGit, CFirstStartWizardBasePage)

#define CHECK_NEWGIT_TIMER	100

CFirstStartWizardGit::CFirstStartWizardGit() : CFirstStartWizardBasePage(CFirstStartWizardGit::IDD)
{
	m_psp.dwFlags |= PSP_DEFAULT | PSP_USEHEADERTITLE;
	m_psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_FIRSTSTART_GITTITLE);

	g_Git.CheckMsysGitDir(TRUE);
	m_regMsysGitPath = CRegString(REG_MSYSGIT_PATH);

	m_regMsysGitExtranPath = CRegString(REG_MSYSGIT_EXTRA_PATH);

	m_sMsysGitPath = m_regMsysGitPath;
	m_sMsysGitExtranPath = m_regMsysGitExtranPath;

	m_bEnableHacks = (CGit::ms_bCygwinGit || CGit::ms_bMsys2Git);
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
	DDX_Check(pDX, IDC_WORKAROUNDS, m_bEnableHacks);
}

BEGIN_MESSAGE_MAP(CFirstStartWizardGit, CFirstStartWizardBasePage)
	ON_BN_CLICKED(IDC_MSYSGIT_BROWSE, OnBrowseDir)
	ON_BN_CLICKED(IDC_MSYSGIT_CHECK, OnCheck)
	ON_BN_CLICKED(IDC_WORKAROUNDS, OnClickedWorkarounds)
	ON_WM_TIMER()
END_MESSAGE_MAP()

LRESULT CFirstStartWizardGit::OnWizardNext()
{
	UpdateData();

	PerformCommonGitPathCleanup(m_sMsysGitPath);
	UpdateData(FALSE);

	SetGitHacks();

	if (m_sMsysGitPath.Compare(CString(m_regMsysGitPath)) || m_sMsysGitExtranPath.Compare(CString(m_regMsysGitExtranPath)) || CGit::ms_bCygwinGit != (m_regCygwinHack == TRUE) || CGit::ms_bMsys2Git != (m_regMsys2Hack == TRUE))
	{
		StoreSetting(GetSafeHwnd(), m_sMsysGitPath, m_regMsysGitPath);
		StoreSetting(GetSafeHwnd(), m_sMsysGitExtranPath, m_regMsysGitExtranPath);
		StoreSetting(GetSafeHwnd(), CGit::ms_bCygwinGit, m_regCygwinHack);
		StoreSetting(GetSafeHwnd(), CGit::ms_bMsys2Git, m_regMsys2Hack);
		SendCacheCommand(TGITCACHECOMMAND_END);
	}

	// only complete if the msysgit directory is ok
	bool needWorkarounds = (GetDlgItem(IDC_WORKAROUNDS)->IsWindowVisible() == TRUE);
	if (!CheckGitExe(GetSafeHwnd(), m_sMsysGitPath, m_sMsysGitExtranPath, IDC_MSYSGIT_VER, [&](UINT helpid) { HtmlHelp(0x20000 + helpid); }, &needWorkarounds))
	{
		if (needWorkarounds)
			ShowWorkarounds(true);
		return -1;
	}

	return __super::OnWizardNext();
}

void CFirstStartWizardGit::ShowWorkarounds(bool show)
{
	if (!(CGit::ms_bCygwinGit || CGit::ms_bMsys2Git || show))
		return;

	GetDlgItem(IDC_WORKAROUNDS)->ShowWindow(SW_SHOW);

	GetDlgItem(IDC_GITHACKS1)->ShowWindow(m_bEnableHacks ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_GITHACKS2)->ShowWindow(m_bEnableHacks ? SW_SHOW : SW_HIDE);

	if (CGit::ms_bCygwinGit)
		CheckRadioButton(IDC_GITHACKS1, IDC_GITHACKS2, IDC_GITHACKS1);
	else if (CGit::ms_bMsys2Git)
		CheckRadioButton(IDC_GITHACKS1, IDC_GITHACKS2, IDC_GITHACKS2);
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
	AdjustControlSize(IDC_WORKAROUNDS);
	AdjustControlSize(IDC_GITHACKS1);
	AdjustControlSize(IDC_GITHACKS2);

	CheckRadioButton(IDC_GITHACKS1, IDC_GITHACKS2, IDC_GITHACKS1);
	ShowWorkarounds();

	if (m_sMsysGitPath.IsEmpty())
		SetTimer(CHECK_NEWGIT_TIMER, 1000, nullptr);

	return TRUE;
}

void CFirstStartWizardGit::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == CHECK_NEWGIT_TIMER)
	{
		KillTimer(CHECK_NEWGIT_TIMER);
		UpdateData();
		if (m_sMsysGitPath.IsEmpty())
		{
			if (FindGitForWindows(m_sMsysGitPath))
			{
				UpdateData(FALSE);
				return;
			}
			SetTimer(CHECK_NEWGIT_TIMER, 1000, nullptr);
		}
		return;
	}
	__super::OnTimer(nIDEvent);
}

BOOL CFirstStartWizardGit::OnSetActive()
{
	auto wiz = static_cast<CFirstStartWizard*>(GetParent());

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

void CFirstStartWizardGit::SetGitHacks()
{
	CGit::ms_bCygwinGit = false;
	CGit::ms_bMsys2Git = false;
	if (m_bEnableHacks)
	{
		int id = GetCheckedRadioButton(IDC_GITHACKS1, IDC_GITHACKS2);
		CGit::ms_bCygwinGit = (id == IDC_GITHACKS1);
		CGit::ms_bMsys2Git = (id == IDC_GITHACKS2);
	}
}

void CFirstStartWizardGit::OnCheck()
{
	UpdateData();
	bool oldCygwinGit = CGit::ms_bCygwinGit;
	bool oldMsys2Git = CGit::ms_bMsys2Git;
	SCOPE_EXIT
	{
		CGit::ms_bCygwinGit = oldCygwinGit;
		CGit::ms_bMsys2Git = oldMsys2Git;
	};

	SetGitHacks();

	bool needWorkarounds = (GetDlgItem(IDC_WORKAROUNDS)->IsWindowVisible() == TRUE);
	CheckGitExe(GetSafeHwnd(), m_sMsysGitPath, m_sMsysGitExtranPath, IDC_MSYSGIT_VER, [&](UINT helpid) { HtmlHelp(0x20000 + helpid); }, &needWorkarounds);
	if (needWorkarounds)
		ShowWorkarounds(true);
}

BOOL CFirstStartWizardGit::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return __super::PreTranslateMessage(pMsg);
}

void CFirstStartWizardGit::OnClickedWorkarounds()
{
	UpdateData();
	ShowWorkarounds(true);
}
