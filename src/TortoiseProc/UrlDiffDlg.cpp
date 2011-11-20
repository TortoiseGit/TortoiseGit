// TortoiseGit - a Windows shell extension for easy version control

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
#include "UrlDiffDlg.h"
#include "RepositoryBrowser.h"
#include "BrowseFolder.h"
#include "TSVNPath.h"
#include "AppUtils.h"

IMPLEMENT_DYNAMIC(CUrlDiffDlg, CResizableStandAloneDialog)
CUrlDiffDlg::CUrlDiffDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CUrlDiffDlg::IDD, pParent)
	, m_URL(_T(""))
	, Revision(_T("HEAD"))
	, m_pLogDlg(NULL)
{
}

CUrlDiffDlg::~CUrlDiffDlg()
{
	if (m_pLogDlg)
		delete m_pLogDlg;
}

void CUrlDiffDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_URLCOMBO, m_URLCombo);
	DDX_Text(pDX, IDC_REVISION_NUM, m_rev);
}


BEGIN_MESSAGE_MAP(CUrlDiffDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_BROWSE, OnBnClickedBrowse)
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
	ON_EN_CHANGE(IDC_REVISION_NUM, &CUrlDiffDlg::OnEnChangeRevisionNum)
	ON_BN_CLICKED(IDC_LOG, &CUrlDiffDlg::OnBnClickedLog)
	ON_REGISTERED_MESSAGE(WM_REVSELECTED, &CUrlDiffDlg::OnRevSelected)
END_MESSAGE_MAP()


BOOL CUrlDiffDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	CTSVNPath svnPath(m_path);
	m_bFolder = svnPath.IsDirectory();
	SVN svn;
	CString url = svn.GetURLFromPath(svnPath);
	CString sUUID = svn.GetUUIDFromPath(svnPath);
	m_URLCombo.SetURLHistory(TRUE);
	m_URLCombo.LoadHistory(_T("Software\\TortoiseGit\\History\\repoURLS\\")+sUUID, _T("url"));
	m_URLCombo.SetCurSel(0);

	if (!url.IsEmpty())
	{
		m_path = url;
		m_URLCombo.AddString(CTSVNPath(url).GetUIPathString(), 0);
		m_URLCombo.SelectString(-1, CTSVNPath(url).GetUIPathString());
		m_URL = m_path;
	}

	// set head revision as default revision
	SetRevision(SVNRev::REV_HEAD);

	AddAnchor(IDC_URLLABEL, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_URLCOMBO, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_BROWSE, TOP_RIGHT);
	AddAnchor(IDC_REVGROUP, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_REVISION_HEAD, TOP_LEFT);
	AddAnchor(IDC_REVISION_N, TOP_LEFT);
	AddAnchor(IDC_REVISION_NUM, TOP_LEFT);
	AddAnchor(IDC_LOG, TOP_LEFT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);

	if ((m_pParentWnd==NULL)&&(hWndExplorer))
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	EnableSaveRestore(_T("SwitchDlg"));
	return TRUE;
}

void CUrlDiffDlg::OnBnClickedBrowse()
{
	UpdateData();
	SVNRev rev;
	if (GetCheckedRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N) == IDC_REVISION_HEAD)
	{
		rev = SVNRev::REV_HEAD;
	}
	else
		rev = SVNRev(m_rev);
	if (!rev.IsValid())
		rev = SVNRev::REV_HEAD;
	CAppUtils::BrowseRepository(m_URLCombo, this, rev);
	SetRevision(rev);
}

void CUrlDiffDlg::OnOK()
{
	if (!UpdateData(TRUE))
		return; // don't dismiss dialog (error message already shown by MFC framework)

	// if head revision, set revision as -1
	if (GetCheckedRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N) == IDC_REVISION_HEAD)
	{
		m_rev = _T("HEAD");
	}
	Revision = SVNRev(m_rev);
	if (!Revision.IsValid())
	{
		ShowBalloon(IDC_REVISION_NUM, IDS_ERR_INVALIDREV);
		return;
	}

	m_URLCombo.SaveHistory();
	m_URL = m_URLCombo.GetString();

	UpdateData(FALSE);
	CResizableStandAloneDialog::OnOK();
}

void CUrlDiffDlg::OnBnClickedHelp()
{
	OnHelp();
}

void CUrlDiffDlg::OnEnChangeRevisionNum()
{
	UpdateData();
	if (m_rev.IsEmpty())
		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_HEAD);
	else
		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
}

void CUrlDiffDlg::SetRevision(const SVNRev& rev)
{
	if (rev.IsHead())
		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_HEAD);
	else
	{
		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
		m_rev = rev.ToString();
		UpdateData(FALSE);
	}
}

void CUrlDiffDlg::OnBnClickedLog()
{
	UpdateData(TRUE);
	if (::IsWindow(m_pLogDlg->GetSafeHwnd())&&(m_pLogDlg->IsWindowVisible()))
		return;
	if (!m_path.IsEmpty())
	{
		delete m_pLogDlg;
		m_pLogDlg = new CLogDlg();
		CRegDWORD reg = CRegDWORD(_T("Software\\TortoiseGit\\NumberOfLogs"), 100);
		int limit = (int)(LONG)reg;
		m_pLogDlg->SetSelect(true);
		m_pLogDlg->m_pNotifyWindow = this;
		m_pLogDlg->m_wParam = 0;
		m_pLogDlg->SetParams(CTSVNPath(m_path), SVNRev::REV_HEAD, SVNRev::REV_HEAD, 1, limit, TRUE);
		m_pLogDlg->ContinuousSelection(true);
		m_pLogDlg->Create(IDD_LOGMESSAGE, this);
		m_pLogDlg->ShowWindow(SW_SHOW);
	}
	AfxGetApp()->DoWaitCursor(-1);
}

LPARAM CUrlDiffDlg::OnRevSelected(WPARAM /*wParam*/, LPARAM lParam)
{
	CString temp;
	temp.Format(_T("%ld"), lParam);
	SetDlgItemText(IDC_REVISION_NUM, temp);
	CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
	return 0;
}
