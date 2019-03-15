// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2011-2014, 2016-2019 - TortoiseGit

// with code of PullFetchDlg.cpp

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
#include "RequestPullDlg.h"
#include "Git.h"
#include "LogDlg.h"
#include "MessageBox.h"
#include "AppUtils.h"
#include "StringUtils.h"

IMPLEMENT_DYNAMIC(CRequestPullDlg, CHorizontalResizableStandAloneDialog)

CRequestPullDlg::CRequestPullDlg(CWnd* pParent /*=nullptr*/)
	: CHorizontalResizableStandAloneDialog(CRequestPullDlg::IDD, pParent)
	, m_regSendMail(L"Software\\TortoiseGit\\TortoiseProc\\RequestPull\\SendMail", FALSE)
{
	m_bSendMail = m_regSendMail;
}

CRequestPullDlg::~CRequestPullDlg()
{
}

void CRequestPullDlg::DoDataExchange(CDataExchange* pDX)
{
	CHorizontalResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBOBOXEX_LOCAL_BRANCH, m_cStartRevision);
	DDX_Control(pDX, IDC_COMBOBOXEX_URL, m_cRepositoryURL);
	DDX_Control(pDX, IDC_REMOTE_BRANCH, m_cEndRevision);
	DDX_Check(pDX, IDC_CHECK_SENDMAIL, m_bSendMail);
}


BEGIN_MESSAGE_MAP(CRequestPullDlg, CHorizontalResizableStandAloneDialog)
	ON_BN_CLICKED(IDOK, &CRequestPullDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BUTTON_LOCAL_BRANCH, &CRequestPullDlg::OnBnClickedButtonLocalBranch)
END_MESSAGE_MAP()

BOOL CRequestPullDlg::OnInitDialog()
{
	CHorizontalResizableStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDCANCEL,BOTTOM_RIGHT);
	AddAnchor(IDHELP,BOTTOM_RIGHT);

	AddAnchor(IDC_BUTTON_LOCAL_BRANCH, TOP_RIGHT);
	AddAnchor(IDC_COMBOBOXEX_LOCAL_BRANCH, TOP_LEFT,TOP_RIGHT);
	AddAnchor(IDC_COMBOBOXEX_URL, TOP_LEFT,TOP_RIGHT);
	AddAnchor(IDC_REMOTE_BRANCH, TOP_LEFT,TOP_RIGHT);

	EnableSaveRestore(L"RequestPullDlg");

	CString sWindowTitle;
	GetWindowText(sWindowTitle);
	CAppUtils::SetWindowTitle(m_hWnd, g_Git.m_CurrentDir, sWindowTitle);

	STRING_VECTOR list;
	g_Git.GetBranchList(list, nullptr, CGit::BRANCH_ALL);
	m_cStartRevision.SetMaxHistoryItems(0x7FFFFFFF);
	m_cStartRevision.SetList(list);

	CString WorkingDir=g_Git.m_CurrentDir;
	WorkingDir.Replace(L':', L'_');

	m_RegStartRevision = CRegString(L"Software\\TortoiseGit\\History\\RequestPull\\" + WorkingDir + L"\\startrevision");
	if (m_StartRevision.IsEmpty())
		m_StartRevision = m_RegStartRevision;
	m_cStartRevision.SetWindowTextW(m_StartRevision);

	// store URLs in global history, but save last used local url separately,
	// because one normally has only one writable repository
	m_cRepositoryURL.SetCaseSensitive(TRUE);
	m_cRepositoryURL.SetURLHistory(TRUE);
	m_cRepositoryURL.LoadHistory(L"Software\\TortoiseGit\\History\\RequestPull", L"url");
	m_RegRepositoryURL = CRegString(L"Software\\TortoiseGit\\History\\RequestPull\\" + WorkingDir + L"\\repositoryurl");
	if(m_RepositoryURL.IsEmpty())
		m_RepositoryURL = m_RegRepositoryURL;
	m_cRepositoryURL.SetWindowTextW(m_RepositoryURL);

	m_RegEndRevision = CRegString(L"Software\\TortoiseGit\\History\\RequestPull\\" + WorkingDir + L"\\endrevision", L"HEAD");
	if (m_EndRevision.IsEmpty())
		m_EndRevision = m_RegEndRevision;
	m_cEndRevision.SetWindowTextW(m_EndRevision);

	this->UpdateData(FALSE);

	return TRUE;
}

void CRequestPullDlg::OnBnClickedOk()
{
	CHorizontalResizableStandAloneDialog::UpdateData(TRUE);

	m_cRepositoryURL.SaveHistory();

	m_cStartRevision.GetWindowTextW(m_StartRevision);
	m_RepositoryURL = m_cRepositoryURL.GetString();
	m_cEndRevision.GetWindowTextW(m_EndRevision);

	m_RegStartRevision = m_StartRevision;
	m_RegRepositoryURL = m_RepositoryURL;
	m_RegEndRevision = m_EndRevision.Trim();

	if(!g_Git.IsBranchNameValid(m_EndRevision))
	{
		CMessageBox::Show(GetSafeHwnd(), IDS_B_T_NOTEMPTY, IDS_APPNAME, MB_OK | MB_ICONEXCLAMATION);
		return;
	}

	if (CStringUtils::StartsWith(m_StartRevision, L"remotes/"))
		m_StartRevision = m_StartRevision.Mid(static_cast<int>(wcslen(L"remotes/")));

	m_regSendMail = m_bSendMail;

	CHorizontalResizableStandAloneDialog::OnOK();
}

void CRequestPullDlg::OnBnClickedButtonLocalBranch()
{
	// use the git log to allow selection of a version
	CLogDlg dlg;
	CString revision;
	m_cStartRevision.GetWindowText(revision);
	dlg.SetParams(CTGitPath(), CTGitPath(), revision, revision, 0);
	// tell the dialog to use mode for selecting revisions
	dlg.SetSelect(true);
	dlg.ShowWorkingTreeChanges(false);
	// only one revision must be selected however
	dlg.SingleSelection(true);
	if (dlg.DoModal() == IDOK && !dlg.GetSelectedHash().empty())
		m_cStartRevision.SetWindowText(dlg.GetSelectedHash().at(0).ToString());
}
