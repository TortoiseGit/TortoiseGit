// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2011 Sven Strickroth, <email@cs-ware.de>

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
#include "git.h"
#include "LogDlg.h"

IMPLEMENT_DYNAMIC(CRequestPullDlg, CResizableStandAloneDialog)

CRequestPullDlg::CRequestPullDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CRequestPullDlg::IDD, pParent)
{
}

CRequestPullDlg::~CRequestPullDlg()
{
}

void CRequestPullDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBOBOXEX_LOCAL_BRANCH, m_cStartRevision);
	DDX_Control(pDX, IDC_COMBOBOXEX_URL, m_cRepositoryURL);
	DDX_Control(pDX, IDC_REMOTE_BRANCH, m_cEndRevision);
}


BEGIN_MESSAGE_MAP(CRequestPullDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDOK, &CRequestPullDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BUTTON_LOCAL_BRANCH, &CRequestPullDlg::OnBnClickedButtonLocalBranch)
END_MESSAGE_MAP()

BOOL CRequestPullDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDCANCEL,BOTTOM_RIGHT);
	AddAnchor(IDHELP,BOTTOM_RIGHT);

	AddAnchor(IDC_BUTTON_LOCAL_BRANCH, TOP_RIGHT);
	AddAnchor(IDC_COMBOBOXEX_LOCAL_BRANCH, TOP_LEFT,TOP_RIGHT);
	AddAnchor(IDC_COMBOBOXEX_URL, TOP_LEFT,TOP_RIGHT);
	AddAnchor(IDC_REMOTE_BRANCH, TOP_LEFT,TOP_RIGHT);

	EnableSaveRestore(_T("RequestPullDlg"));

	STRING_VECTOR list;
	int current;
	g_Git.GetBranchList(list, &current, CGit::BRANCH_ALL);
	m_cStartRevision.SetMaxHistoryItems(0x7FFFFFFF);
	for (unsigned int i = 0; i < list.size(); i++)
	{
		m_cStartRevision.AddString(list[i]);
	}

	m_cRepositoryURL.SetURLHistory(TRUE);
	m_cRepositoryURL.LoadHistory(_T("Software\\TortoiseGit\\History\\RequestPull"), _T("url"));

	this->UpdateData(FALSE);

	return TRUE;
}

void CRequestPullDlg::OnBnClickedOk()
{
	CResizableStandAloneDialog::UpdateData(TRUE);

	m_cRepositoryURL.SaveHistory();

	m_cStartRevision.GetWindowTextW(m_StartRevision);
	m_RepositoryURL = m_cRepositoryURL.GetString();
	m_cEndRevision.GetWindowTextW(m_EndRevision);

	CResizableStandAloneDialog::OnOK();
}

void CRequestPullDlg::OnBnClickedButtonLocalBranch()
{
	// use the git log to allow selection of a version
	CLogDlg dlg;
	// tell the dialog to use mode for selecting revisions
	dlg.SetSelect(true);
	// only one revision must be selected however
	dlg.SingleSelection(true);
	if ( dlg.DoModal() == IDOK )
	{
		// get selected hash if any
		CString selectedHash = dlg.GetSelectedHash();
		// load into window, do this even if empty so that it is clear that nothing has been selected
		m_cStartRevision.SetWindowText( selectedHash );
	}
}
