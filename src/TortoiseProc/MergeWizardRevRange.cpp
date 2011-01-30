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
#include "MergeWizardRevRange.h"
#include "AppUtils.h"
#include "PathUtils.h"


IMPLEMENT_DYNAMIC(CMergeWizardRevRange, CMergeWizardBasePage)

CMergeWizardRevRange::CMergeWizardRevRange()
	: CMergeWizardBasePage(CMergeWizardRevRange::IDD)
	, m_sRevRange(_T(""))
	, m_pLogDlg(NULL)
	, m_pLogDlg2(NULL)
{
	m_psp.dwFlags |= PSP_DEFAULT|PSP_USEHEADERTITLE|PSP_USEHEADERSUBTITLE;
	m_psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_MERGEWIZARD_REVRANGETITLE);
	m_psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_MERGEWIZARD_REVRANGESUBTITLE);
}

CMergeWizardRevRange::~CMergeWizardRevRange()
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

void CMergeWizardRevRange::DoDataExchange(CDataExchange* pDX)
{
	CMergeWizardBasePage::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_REVISION_RANGE, m_sRevRange);
	DDX_Control(pDX, IDC_URLCOMBO, m_URLCombo);
	DDX_Check(pDX, IDC_REVERSEMERGE, ((CMergeWizard*)GetParent())->bReverseMerge);
}


BEGIN_MESSAGE_MAP(CMergeWizardRevRange, CMergeWizardBasePage)
	ON_REGISTERED_MESSAGE(WM_REVLIST, OnRevSelected)
	ON_REGISTERED_MESSAGE(WM_REVLISTONERANGE, OnRevSelectedOneRange)
	ON_BN_CLICKED(IDC_SELLOG, &CMergeWizardRevRange::OnBnClickedShowlog)
	ON_BN_CLICKED(IDC_BROWSE, &CMergeWizardRevRange::OnBnClickedBrowse)
	ON_BN_CLICKED(IDC_SHOWLOGWC, &CMergeWizardRevRange::OnBnClickedShowlogwc)
END_MESSAGE_MAP()


LRESULT CMergeWizardRevRange::OnWizardBack()
{
	return IDD_MERGEWIZARD_START;
}

LRESULT CMergeWizardRevRange::OnWizardNext()
{
	UpdateData();
	m_URLCombo.SaveHistory();
	((CMergeWizard*)GetParent())->URL1 = m_URLCombo.GetString();
	((CMergeWizard*)GetParent())->URL2 = m_URLCombo.GetString();
	// if the revision range has HEAD as a revision specified, we have to
	// ask the server what the HEAD revision is: the SVNRevList can only deal
	// with numerical revisions because we have to sort the list to get the
	// ranges correctly
	if (m_sRevRange.Find(_T("HEAD")) >= 0)
	{
		if (!m_HEAD.IsValid())
		{
			SVN svn;
			m_HEAD = svn.GetHEADRevision(CTSVNPath(((CMergeWizard*)GetParent())->URL1));
		}
		m_sRevRange.Replace(_T("HEAD"), m_HEAD.ToString());
	}
	if (!((CMergeWizard*)GetParent())->revRangeArray.FromListString(m_sRevRange))
	{
		CBalloon::ShowBalloon(this, CBalloon::GetCtrlCentre(this, IDC_REVISION_RANGE), IDS_ERR_INVALIDREVRANGE, TRUE, IDI_EXCLAMATION);
		return -1;
	}
	return IDD_MERGEWIZARD_OPTIONS;
}


BOOL CMergeWizardRevRange::OnInitDialog()
{
	CMergeWizardBasePage::OnInitDialog();

	CMergeWizard * pWizard = (CMergeWizard*)GetParent();

	CString sUUID = pWizard->sUUID;
	m_URLCombo.SetURLHistory(TRUE);
	m_URLCombo.LoadHistory(_T("Software\\TortoiseGit\\History\\repoURLS\\")+sUUID, _T("url"));
	if (!(DWORD)CRegDWORD(_T("Software\\TortoiseGit\\MergeWCURL"), FALSE))
		m_URLCombo.SetCurSel(0);
	if (!pWizard->url.IsEmpty())
		m_URLCombo.SetWindowText(CPathUtils::PathUnescape(pWizard->url));
	if (pWizard->revRangeArray.GetCount())
	{
		m_sRevRange = pWizard->revRangeArray.ToListString();
		SetDlgItemText(IDC_REVISION_RANGE, m_sRevRange);
	}

	CString sLabel;
	sLabel.LoadString(IDS_MERGEWIZARD_REVRANGESTRING);
	SetDlgItemText(IDC_REVRANGELABEL, sLabel);

	SetDlgItemText(IDC_WCEDIT, ((CMergeWizard*)GetParent())->wcPath.GetWinPath());

	AdjustControlSize(IDC_REVERSEMERGE);

	return TRUE;
}

void CMergeWizardRevRange::OnBnClickedShowlog()
{
	if (::IsWindow(m_pLogDlg->GetSafeHwnd())&&(m_pLogDlg->IsWindowVisible()))
		return;
	CString url = m_URLCombo.GetString();

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

LPARAM CMergeWizardRevRange::OnRevSelected(WPARAM wParam, LPARAM lParam)
{
	((CMergeWizard*)GetParent())->revRangeArray.Clear();

	// lParam is a pointer to an SVNRevList, wParam contains the number of elements in that list.
	if ((lParam)&&(wParam))
	{
		UpdateData(TRUE);
		CMergeWizard* dlg = (CMergeWizard*)GetParent();
		dlg->revRangeArray = *((SVNRevRangeArray*)lParam);
		bool bReverse = !!dlg->bReverseMerge;
		m_sRevRange = dlg->revRangeArray.ToListString(bReverse);
		UpdateData(FALSE);
	}
	return 0;
}

LPARAM CMergeWizardRevRange::OnRevSelectedOneRange(WPARAM /*wParam*/, LPARAM lParam)
{
	((CMergeWizard*)GetParent())->revRangeArray.Clear();

	// lParam is a pointer to an SVNRevList
	if ((lParam))
	{
		UpdateData(TRUE);
		CMergeWizard* dlg = (CMergeWizard*)GetParent();
		dlg->revRangeArray = *((SVNRevRangeArray*)lParam);
		bool bReverse = !!dlg->bReverseMerge;
		m_sRevRange = dlg->revRangeArray.ToListString(bReverse);
		UpdateData(FALSE);
	}
	return 0;
}

void CMergeWizardRevRange::OnBnClickedBrowse()
{
	SVNRev rev(SVNRev::REV_HEAD);
	CAppUtils::BrowseRepository(m_URLCombo, this, rev);
}


BOOL CMergeWizardRevRange::OnSetActive()
{
	CPropertySheet* psheet = (CPropertySheet*) GetParent();
	psheet->SetWizardButtons(PSWIZB_NEXT|PSWIZB_BACK);
	SetButtonTexts();

	return CMergeWizardBasePage::OnSetActive();
}

void CMergeWizardRevRange::OnBnClickedShowlogwc()
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
