// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008 - TortoiseSVN

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
#include "MergeWizardTree.h"

#include "Balloon.h"
#include "AppUtils.h"
#include "PathUtils.h"


IMPLEMENT_DYNAMIC(CMergeWizardTree, CMergeWizardBasePage)

CMergeWizardTree::CMergeWizardTree()
	: CMergeWizardBasePage(CMergeWizardTree::IDD)
	, m_URLFrom(_T(""))
	, m_URLTo(_T(""))
	, StartRev(0)
	, EndRev(_T("HEAD"))
	, m_pLogDlg(NULL)
	, m_pLogDlg2(NULL)
	, m_pLogDlg3(NULL)
{
	m_psp.dwFlags |= PSP_DEFAULT|PSP_USEHEADERTITLE|PSP_USEHEADERSUBTITLE;
	m_psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_MERGEWIZARD_TREETITLE);
	m_psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_MERGEWIZARD_TREESUBTITLE);
}

CMergeWizardTree::~CMergeWizardTree()
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
	if (m_pLogDlg3)
	{
		m_pLogDlg3->DestroyWindow();
		delete m_pLogDlg3;
	}
}

void CMergeWizardTree::DoDataExchange(CDataExchange* pDX)
{
	CMergeWizardBasePage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_URLCOMBO, m_URLCombo);
	DDX_Text(pDX, IDC_REVISION_START, m_sStartRev);
	DDX_Text(pDX, IDC_REVISION_END, m_sEndRev);
	DDX_Control(pDX, IDC_URLCOMBO2, m_URLCombo2);
}


BEGIN_MESSAGE_MAP(CMergeWizardTree, CMergeWizardBasePage)
	ON_REGISTERED_MESSAGE(WM_REVSELECTED, OnRevSelected)
	ON_BN_CLICKED(IDC_BROWSE, OnBnClickedBrowse)
	ON_BN_CLICKED(IDC_BROWSE2, OnBnClickedBrowse2)
	ON_BN_CLICKED(IDC_FINDBRANCHSTART, OnBnClickedFindbranchstart)
	ON_BN_CLICKED(IDC_FINDBRANCHEND, OnBnClickedFindbranchend)
	ON_EN_CHANGE(IDC_REVISION_END, &CMergeWizardTree::OnEnChangeRevisionEnd)
	ON_EN_CHANGE(IDC_REVISION_START, &CMergeWizardTree::OnEnChangeRevisionStart)
	ON_BN_CLICKED(IDC_SHOWLOGWC, &CMergeWizardTree::OnBnClickedShowlogwc)
END_MESSAGE_MAP()


BOOL CMergeWizardTree::OnInitDialog()
{
	CMergeWizardBasePage::OnInitDialog();

	CMergeWizard * pWizard = (CMergeWizard*)GetParent();
	CString sUUID = pWizard->sUUID;
	m_URLCombo.SetURLHistory(TRUE);
	m_URLCombo.LoadHistory(_T("Software\\TortoiseGit\\History\\repoURLS\\")+sUUID, _T("url"));
	if (!(DWORD)CRegDWORD(_T("Software\\TortoiseGit\\MergeWCURL"), FALSE))
		m_URLCombo.SetCurSel(0);
	// Only set the "From" Url if there is no url history available
	if (m_URLCombo.GetString().IsEmpty())
		m_URLCombo.SetWindowText(CPathUtils::PathUnescape(pWizard->url));
	m_URLCombo2.SetURLHistory(TRUE);
	m_URLCombo2.LoadHistory(_T("Software\\TortoiseGit\\History\\repoURLS\\")+sUUID, _T("url"));
	m_URLCombo2.SetCurSel(0);
	if (m_URLCombo2.GetString().IsEmpty())
		m_URLCombo2.SetWindowText(CPathUtils::PathUnescape(pWizard->url));
	if (!pWizard->URL1.IsEmpty())
		m_URLCombo.SetWindowText(CPathUtils::PathUnescape(pWizard->URL1));
	if (!pWizard->URL2.IsEmpty())
		m_URLCombo2.SetWindowText(CPathUtils::PathUnescape(pWizard->URL2));

	SetDlgItemText(IDC_WCEDIT, ((CMergeWizard*)GetParent())->wcPath.GetWinPath());

	// set head revision as default revision
	if (pWizard->startRev.IsHead() || !pWizard->startRev.IsValid())
		CheckRadioButton(IDC_REVISION_HEAD1, IDC_REVISION_N1, IDC_REVISION_HEAD1);
	else
	{
		CheckRadioButton(IDC_REVISION_HEAD1, IDC_REVISION_N1, IDC_REVISION_N1);
		m_sStartRev = pWizard->startRev.ToString();
		SetDlgItemText(IDC_REVISION_START, m_sStartRev);
	}
	if (pWizard->endRev.IsHead() || !pWizard->endRev.IsValid())
		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_HEAD);
	else
	{
		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
		m_sEndRev = pWizard->endRev.ToString();
		SetDlgItemText(IDC_REVISION_END, m_sEndRev);
	}

	AdjustControlSize(IDC_REVISION_HEAD1);
	AdjustControlSize(IDC_REVISION_N1);
	AdjustControlSize(IDC_REVISION_HEAD);
	AdjustControlSize(IDC_REVISION_N);

	return TRUE;
}

BOOL CMergeWizardTree::CheckData(bool bShowErrors /* = true */)
{
	if (!UpdateData(TRUE))
		return FALSE;

	StartRev = SVNRev(m_sStartRev);
	EndRev = SVNRev(m_sEndRev);
	if (GetCheckedRadioButton(IDC_REVISION_HEAD1, IDC_REVISION_N1) == IDC_REVISION_HEAD1)
	{
		StartRev = SVNRev(_T("HEAD"));
	}
	if (!StartRev.IsValid())
	{
		if (bShowErrors)
			CBalloon::ShowBalloon(this, CBalloon::GetCtrlCentre(this, IDC_REVISION_START), IDS_ERR_INVALIDREV, TRUE, IDI_EXCLAMATION);
		return FALSE;
	}

	// if head revision, set revision as -1
	if (GetCheckedRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N) == IDC_REVISION_HEAD)
	{
		EndRev = SVNRev(_T("HEAD"));
	}
	if (!EndRev.IsValid())
	{
		if (bShowErrors)
			CBalloon::ShowBalloon(this, CBalloon::GetCtrlCentre(this, IDC_REVISION_END), IDS_ERR_INVALIDREV, TRUE, IDI_EXCLAMATION);
		return FALSE;
	}

	m_URLCombo.SaveHistory();
	m_URLFrom = m_URLCombo.GetString();

	m_URLCombo2.SaveHistory();
	m_URLTo = m_URLCombo2.GetString();

	((CMergeWizard*)GetParent())->URL1 = m_URLFrom;
	((CMergeWizard*)GetParent())->URL2 = m_URLTo;
	((CMergeWizard*)GetParent())->startRev = StartRev;
	((CMergeWizard*)GetParent())->endRev = EndRev;

	UpdateData(FALSE);
	return TRUE;
}

void CMergeWizardTree::OnEnChangeRevisionEnd()
{
	UpdateData();
	if (m_sEndRev.IsEmpty())
		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_HEAD);
	else
		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
}

void CMergeWizardTree::OnEnChangeRevisionStart()
{
	UpdateData();
	if (m_sStartRev.IsEmpty())
		CheckRadioButton(IDC_REVISION_HEAD1, IDC_REVISION_N1, IDC_REVISION_HEAD1);
	else
		CheckRadioButton(IDC_REVISION_HEAD1, IDC_REVISION_N1, IDC_REVISION_N1);
}

void CMergeWizardTree::SetStartRevision(const SVNRev& rev)
{
	if (rev.IsHead())
		CheckRadioButton(IDC_REVISION_HEAD1, IDC_REVISION_N1, IDC_REVISION_HEAD1);
	else
	{
		CheckRadioButton(IDC_REVISION_HEAD1, IDC_REVISION_N1, IDC_REVISION_N1);
		m_sStartRev = rev.ToString();
		UpdateData(FALSE);
	}
}

void CMergeWizardTree::SetEndRevision(const SVNRev& rev)
{
	if (rev.IsHead())
		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_HEAD);
	else
	{
		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
		m_sEndRev = rev.ToString();
		UpdateData(FALSE);
	}
}

void CMergeWizardTree::OnBnClickedBrowse()
{
	CheckData(false);
	if ((!StartRev.IsValid())||(StartRev == 0))
		StartRev = SVNRev::REV_HEAD;
	if (CAppUtils::BrowseRepository(m_URLCombo, this, StartRev))
	{
		SetStartRevision(StartRev);
	}
}

void CMergeWizardTree::OnBnClickedBrowse2()
{
	CheckData(false);

	if ((!EndRev.IsValid())||(EndRev == 0))
		EndRev = SVNRev::REV_HEAD;

	CAppUtils::BrowseRepository(m_URLCombo2, this, EndRev);
	SetEndRevision(EndRev);
}

void CMergeWizardTree::OnBnClickedFindbranchstart()
{
	CheckData(false);
	if ((!StartRev.IsValid())||(StartRev == 0))
		StartRev = SVNRev::REV_HEAD;
	if (::IsWindow(m_pLogDlg->GetSafeHwnd())&&(m_pLogDlg->IsWindowVisible()))
		return;
	CString url;
	m_URLCombo.GetWindowText(url);
	//now show the log dialog for the main trunk
	if (!url.IsEmpty())
	{
		CTSVNPath wcPath = ((CMergeWizard*)GetParent())->wcPath;
		if (m_pLogDlg)
		{
			m_pLogDlg->DestroyWindow();
			delete m_pLogDlg;
		}
		m_pLogDlg = new CLogDlg();
		CRegDWORD reg = CRegDWORD(_T("Software\\TortoiseGit\\NumberOfLogs"), 100);
		int limit = (int)(LONG)reg;
		m_pLogDlg->m_wParam = MERGE_REVSELECTSTART;
		m_pLogDlg->SetDialogTitle(CString(MAKEINTRESOURCE(IDS_MERGE_SELECTSTARTREVISION)));
		m_pLogDlg->SetSelect(true);
		m_pLogDlg->m_pNotifyWindow = this;
		m_pLogDlg->SetParams(CTSVNPath(url), StartRev, StartRev, 1, limit, TRUE, FALSE);
		m_pLogDlg->SetProjectPropertiesPath(wcPath);
		m_pLogDlg->ContinuousSelection(true);
		m_pLogDlg->SetMergePath(wcPath);
		m_pLogDlg->Create(IDD_LOGMESSAGE, this);
		m_pLogDlg->ShowWindow(SW_SHOW);
	}
}

void CMergeWizardTree::OnBnClickedFindbranchend()
{
	CheckData(false);

	if ((!EndRev.IsValid())||(EndRev == 0))
		EndRev = SVNRev::REV_HEAD;
	if (::IsWindow(m_pLogDlg2->GetSafeHwnd())&&(m_pLogDlg2->IsWindowVisible()))
		return;
	CString url;

	m_URLCombo2.GetWindowText(url);
	//now show the log dialog for the main trunk
	if (!url.IsEmpty())
	{
		CTSVNPath wcPath = ((CMergeWizard*)GetParent())->wcPath;
		if (m_pLogDlg2)
		{
			m_pLogDlg2->DestroyWindow();
			delete m_pLogDlg2;
		}
		m_pLogDlg2 = new CLogDlg();
		CRegDWORD reg = CRegDWORD(_T("Software\\TortoiseGit\\NumberOfLogs"), 100);
		int limit = (int)(LONG)reg;
		m_pLogDlg2->m_wParam = MERGE_REVSELECTEND;
		m_pLogDlg2->SetDialogTitle(CString(MAKEINTRESOURCE(IDS_MERGE_SELECTENDREVISION)));
		m_pLogDlg2->SetSelect(true);
		m_pLogDlg2->m_pNotifyWindow = this;
		m_pLogDlg2->SetProjectPropertiesPath(wcPath);
		m_pLogDlg2->SetParams(CTSVNPath(url), EndRev, EndRev, 1, limit, TRUE, FALSE);
		m_pLogDlg2->ContinuousSelection(true);
		m_pLogDlg2->SetMergePath(wcPath);
		m_pLogDlg2->Create(IDD_LOGMESSAGE, this);
		m_pLogDlg2->ShowWindow(SW_SHOW);
	}
}

LPARAM CMergeWizardTree::OnRevSelected(WPARAM wParam, LPARAM lParam)
{
	CString temp;

	if (wParam & MERGE_REVSELECTSTART)
	{
		if (wParam & MERGE_REVSELECTMINUSONE)
			lParam--;
		temp.Format(_T("%ld"), lParam);
		SetDlgItemText(IDC_REVISION_START, temp);
		CheckRadioButton(IDC_REVISION_HEAD1, IDC_REVISION_N1, IDC_REVISION_N1);
	}
	if (wParam & MERGE_REVSELECTEND)
	{
		temp.Format(_T("%ld"), lParam);
		SetDlgItemText(IDC_REVISION_END, temp);
		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
	}
	return 0;
}


LRESULT CMergeWizardTree::OnWizardNext()
{
	if (!CheckData(true))
		return -1;

	return IDD_MERGEWIZARD_OPTIONS;
}

LRESULT CMergeWizardTree::OnWizardBack()
{
	return IDD_MERGEWIZARD_START;
}

BOOL CMergeWizardTree::OnSetActive()
{
	CPropertySheet* psheet = (CPropertySheet*) GetParent();
	psheet->SetWizardButtons(PSWIZB_NEXT|PSWIZB_BACK);
	SetButtonTexts();

	return CMergeWizardBasePage::OnSetActive();
}

void CMergeWizardTree::OnBnClickedShowlogwc()
{
	CTSVNPath wcPath = ((CMergeWizard*)GetParent())->wcPath;
	if (m_pLogDlg3)
		m_pLogDlg3->DestroyWindow();
	delete m_pLogDlg3;
	m_pLogDlg3 = new CLogDlg();
	CRegDWORD reg = CRegDWORD(_T("Software\\TortoiseGit\\NumberOfLogs"), 100);
	int limit = (int)(LONG)reg;
	m_pLogDlg3->SetDialogTitle(CString(MAKEINTRESOURCE(IDS_MERGE_SELECTRANGE)));

	m_pLogDlg3->m_pNotifyWindow = NULL;
	m_pLogDlg3->SetParams(wcPath, SVNRev::REV_HEAD, SVNRev::REV_HEAD, 1, limit, TRUE, FALSE);
	m_pLogDlg3->SetProjectPropertiesPath(wcPath);
	m_pLogDlg3->SetMergePath(wcPath);
	m_pLogDlg3->Create(IDD_LOGMESSAGE, this);
	m_pLogDlg3->ShowWindow(SW_SHOW);
}
