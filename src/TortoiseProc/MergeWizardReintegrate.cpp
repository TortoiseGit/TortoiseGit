// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2008 - TortoiseSVN

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
#include "MergeWizard.h"
#include "MergeWizardReintegrate.h"

#include "Balloon.h"
#include "AppUtils.h"
#include "PathUtils.h"


IMPLEMENT_DYNAMIC(CMergeWizardReintegrate, CMergeWizardBasePage)

CMergeWizardReintegrate::CMergeWizardReintegrate()
	: CMergeWizardBasePage(CMergeWizardReintegrate::IDD)
	, m_URL(_T(""))
	, m_pLogDlg(NULL)
	, m_pLogDlg2(NULL)
{
	m_psp.dwFlags |= PSP_DEFAULT|PSP_USEHEADERTITLE|PSP_USEHEADERSUBTITLE;
	m_psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_MERGEWIZARD_TREETITLE);
	m_psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_MERGEWIZARD_TREESUBTITLE);
}

CMergeWizardReintegrate::~CMergeWizardReintegrate()
{
	if (m_pLogDlg)
	{
		m_pLogDlg->DestroyWindow();
		delete m_pLogDlg;
	}
	if (m_pLogDlg2)
	{
		m_pLogDlg2->DestroyWindow();
		delete m_pLogDlg2;
	}
}

void CMergeWizardReintegrate::DoDataExchange(CDataExchange* pDX)
{
	CMergeWizardBasePage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_URLCOMBO, m_URLCombo);
}


BEGIN_MESSAGE_MAP(CMergeWizardReintegrate, CMergeWizardBasePage)
	ON_BN_CLICKED(IDC_SHOWMERGELOG, &CMergeWizardReintegrate::OnBnClickedShowmergelog)
	ON_BN_CLICKED(IDC_BROWSE, &CMergeWizardReintegrate::OnBnClickedBrowse)
	ON_BN_CLICKED(IDC_SHOWLOGWC, &CMergeWizardReintegrate::OnBnClickedShowlogwc)
END_MESSAGE_MAP()


BOOL CMergeWizardReintegrate::OnInitDialog()
{
	CMergeWizardBasePage::OnInitDialog();

	CString sUUID = ((CMergeWizard*)GetParent())->sUUID;
	m_URLCombo.SetURLHistory(TRUE);
	m_URLCombo.LoadHistory(_T("Software\\TortoiseGit\\History\\repoURLS\\")+sUUID, _T("url"));
	if (!(DWORD)CRegDWORD(_T("Software\\TortoiseGit\\MergeWCURL"), FALSE))
		m_URLCombo.SetCurSel(0);

	SetDlgItemText(IDC_WCEDIT, ((CMergeWizard*)GetParent())->wcPath.GetWinPath());

	return TRUE;
}

BOOL CMergeWizardReintegrate::CheckData(bool bShowErrors /* = true */)
{
	UNREFERENCED_PARAMETER(bShowErrors);

	if (!UpdateData(TRUE))
		return FALSE;


	m_URLCombo.SaveHistory();
	m_URL = m_URLCombo.GetString();

	((CMergeWizard*)GetParent())->URL1 = m_URL;

	UpdateData(FALSE);
	return TRUE;
}

LRESULT CMergeWizardReintegrate::OnWizardNext()
{
	if (!CheckData(true))
		return -1;

	return IDD_MERGEWIZARD_OPTIONS;
}

LRESULT CMergeWizardReintegrate::OnWizardBack()
{
	return IDD_MERGEWIZARD_START;
}

BOOL CMergeWizardReintegrate::OnSetActive()
{
	CPropertySheet* psheet = (CPropertySheet*) GetParent();   
	psheet->SetWizardButtons(PSWIZB_NEXT|PSWIZB_BACK);
	SetButtonTexts();

	return CMergeWizardBasePage::OnSetActive();
}


void CMergeWizardReintegrate::OnBnClickedShowmergelog()
{
	if (::IsWindow(m_pLogDlg->GetSafeHwnd())&&(m_pLogDlg->IsWindowVisible()))
		return;
	CString url;
	m_URLCombo.GetWindowText(url);

	if (!url.IsEmpty())
	{
		CTSVNPath wcPath = ((CMergeWizard*)GetParent())->wcPath;
		if (m_pLogDlg)
			m_pLogDlg->DestroyWindow();
		delete m_pLogDlg;
		m_pLogDlg = new CLogDlg();
		CRegDWORD reg = CRegDWORD(_T("Software\\TortoiseGit\\NumberOfLogs"), 100);
		int limit = (int)(LONG)reg;
		m_pLogDlg->SetDialogTitle(CString(MAKEINTRESOURCE(IDS_MERGE_SELECTRANGE)));

		m_pLogDlg->SetSelect(true);
		m_pLogDlg->m_pNotifyWindow = this;
		m_pLogDlg->SetParams(CTSVNPath(url), SVNRev::REV_HEAD, SVNRev::REV_HEAD, 1, limit, TRUE, FALSE);
		m_pLogDlg->SetProjectPropertiesPath(wcPath);
		m_pLogDlg->SetMergePath(wcPath);
		m_pLogDlg->Create(IDD_LOGMESSAGE, this);
		m_pLogDlg->ShowWindow(SW_SHOW);
	}
}

void CMergeWizardReintegrate::OnBnClickedBrowse()
{
	SVNRev rev(SVNRev::REV_HEAD);
	CAppUtils::BrowseRepository(m_URLCombo, this, rev);
}

void CMergeWizardReintegrate::OnBnClickedShowlogwc()
{
	CTSVNPath wcPath = ((CMergeWizard*)GetParent())->wcPath;
	if (m_pLogDlg2)
		m_pLogDlg2->DestroyWindow();
	delete m_pLogDlg2;
	m_pLogDlg2 = new CLogDlg();
	CRegDWORD reg = CRegDWORD(_T("Software\\TortoiseGit\\NumberOfLogs"), 100);
	int limit = (int)(LONG)reg;
	m_pLogDlg2->SetDialogTitle(CString(MAKEINTRESOURCE(IDS_MERGE_SELECTRANGE)));

	m_pLogDlg2->m_pNotifyWindow = NULL;
	m_pLogDlg2->SetParams(wcPath, SVNRev::REV_HEAD, SVNRev::REV_HEAD, 1, limit, TRUE, FALSE);
	m_pLogDlg2->SetProjectPropertiesPath(wcPath);
	m_pLogDlg2->SetMergePath(wcPath);
	m_pLogDlg2->Create(IDD_LOGMESSAGE, this);
	m_pLogDlg2->ShowWindow(SW_SHOW);
}
