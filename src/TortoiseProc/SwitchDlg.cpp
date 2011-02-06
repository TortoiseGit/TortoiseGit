// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN

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
#include "SwitchDlg.h"
//#include "RepositoryBrowser.h"
#include "BrowseFolder.h"
#include "TGitPath.h"
#include "AppUtils.h"

IMPLEMENT_DYNAMIC(CSwitchDlg, CResizableStandAloneDialog)
CSwitchDlg::CSwitchDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CSwitchDlg::IDD, pParent)
	, m_URL(_T(""))
//	, Revision(_T("HEAD"))
	, m_pLogDlg(NULL)
{
}

CSwitchDlg::~CSwitchDlg()
{
	if (m_pLogDlg)
		delete m_pLogDlg;
}

void CSwitchDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_URLCOMBO, m_URLCombo);
	DDX_Text(pDX, IDC_REVISION_NUM, m_rev);
}


BEGIN_MESSAGE_MAP(CSwitchDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_BROWSE, OnBnClickedBrowse)
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
	ON_EN_CHANGE(IDC_REVISION_NUM, &CSwitchDlg::OnEnChangeRevisionNum)
	ON_BN_CLICKED(IDC_LOG, &CSwitchDlg::OnBnClickedLog)
	ON_REGISTERED_MESSAGE(WM_REVSELECTED, &CSwitchDlg::OnRevSelected)
	ON_WM_SIZING()
END_MESSAGE_MAP()

void CSwitchDlg::SetDialogTitle(const CString& sTitle)
{
	m_sTitle = sTitle;
}

void CSwitchDlg::SetUrlLabel(const CString& sLabel)
{
	m_sLabel = sLabel;
}

BOOL CSwitchDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	CTGitPath GitPath(m_path);
	SetDlgItemText(IDC_SWITCHPATH, m_path);
	m_bFolder = GitPath.IsDirectory();
	//Git Git;
	//CString url = Git.GetURLFromPath(GitPath);
	//CString sUUID = Git.GetUUIDFromPath(GitPath);
	//m_URLCombo.SetURLHistory(TRUE);
	//m_URLCombo.LoadHistory(_T("Software\\TortoiseGit\\History\\repoURLS\\")+sUUID, _T("url"));
	//m_URLCombo.SetCurSel(0);

//	if (!url.IsEmpty())
//	{
//		m_path = url;
//		m_URLCombo.AddString(CTGitPath(url).GetUIPathString(), 0);
//		m_URLCombo.SelectString(-1, CTGitPath(url).GetUIPathString());
//		m_URL = m_path;
//	}

	if (m_sTitle.IsEmpty())
		GetWindowText(m_sTitle);
	SetWindowText(m_sTitle);
	if (m_sLabel.IsEmpty())
		GetDlgItemText(IDC_URLLABEL, m_sLabel);
	SetDlgItemText(IDC_URLLABEL, m_sLabel);

	// set head revision as default revision
//	SetRevision(GitRev::REV_HEAD);

	RECT rect;
	GetWindowRect(&rect);
	m_height = rect.bottom - rect.top;

	AddAnchor(IDC_SWITCHLABEL, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_SWITCHPATH, TOP_LEFT, TOP_RIGHT);
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

void CSwitchDlg::OnBnClickedBrowse()
{
	UpdateData();
	GitRev rev;
	if (GetCheckedRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N) == IDC_REVISION_HEAD)
	{
//		rev = GitRev::REV_HEAD;
	}
//	else
//		rev = GitRev(m_rev);
//	if (!rev.IsValid())
//		rev = GitRev::REV_HEAD;
//	CAppUtils::BrowseRepository(m_URLCombo, this, rev);
//	SetRevision(rev);
}

void CSwitchDlg::OnOK()
{
	if (!UpdateData(TRUE))
		return; // don't dismiss dialog (error message already shown by MFC framework)

	// if head revision, set revision as HEAD
	if (GetCheckedRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N) == IDC_REVISION_HEAD)
	{
		m_rev = _T("HEAD");
	}
//	Revision = GitRev(m_rev);
//	if (!Revision.IsValid())
//	{
//		ShowBalloon(IDC_REVISION_NUM, IDS_ERR_INVALIDREV);
//		return;
//	}

	m_URLCombo.SaveHistory();
	m_URL = m_URLCombo.GetString();

	UpdateData(FALSE);
	CResizableStandAloneDialog::OnOK();
}

void CSwitchDlg::OnBnClickedHelp()
{
	OnHelp();
}

void CSwitchDlg::OnEnChangeRevisionNum()
{
	UpdateData();
	if (m_rev.IsEmpty())
		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_HEAD);
	else
		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
}

void CSwitchDlg::SetRevision(const GitRev& rev)
{
//	if (rev.IsHead())
//		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_HEAD);
//	else
//	{
//		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
//		m_rev = rev.ToString();
//		UpdateData(FALSE);
//	}
}

void CSwitchDlg::OnBnClickedLog()
{
	UpdateData(TRUE);
	if (::IsWindow(m_pLogDlg->GetSafeHwnd())&&(m_pLogDlg->IsWindowVisible()))
		return;
	m_URL = m_URLCombo.GetString();
	if (!m_URL.IsEmpty())
	{
		delete m_pLogDlg;
		m_pLogDlg = new CLogDlg();
//		CRegDWORD reg = CRegDWORD(_T("Software\\TortoiseGit\\NumberOfLogs"), 100);
//		int limit = (int)(LONG)reg;
		m_pLogDlg->SetSelect(true);
		m_pLogDlg->m_pNotifyWindow = this;
		m_pLogDlg->m_wParam = 0;
//		m_pLogDlg->SetParams(CTGitPath(m_URL), GitRev::REV_HEAD, GitRev::REV_HEAD, 1, limit, TRUE);
		m_pLogDlg->ContinuousSelection(true);
		m_pLogDlg->Create(IDD_LOGMESSAGE, this);
		m_pLogDlg->ShowWindow(SW_SHOW);
	}
	AfxGetApp()->DoWaitCursor(-1);
}

LPARAM CSwitchDlg::OnRevSelected(WPARAM /*wParam*/, LPARAM lParam)
{
	CString temp;
	temp.Format(_T("%ld"), lParam);
	SetDlgItemText(IDC_REVISION_NUM, temp);
	CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
	return 0;
}

void CSwitchDlg::OnSizing(UINT fwSide, LPRECT pRect)
{
	// don't allow the dialog to be changed in height
	switch (fwSide)
	{
	case WMSZ_BOTTOM:
	case WMSZ_BOTTOMLEFT:
	case WMSZ_BOTTOMRIGHT:
		pRect->bottom = pRect->top + m_height;
		break;
	case WMSZ_TOP:
	case WMSZ_TOPLEFT:
	case WMSZ_TOPRIGHT:
		pRect->top = pRect->bottom - m_height;
		break;
	}
	CResizableStandAloneDialog::OnSizing(fwSide, pRect);
}
