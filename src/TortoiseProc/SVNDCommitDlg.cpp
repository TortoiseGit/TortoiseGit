// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2012-2016 - TortoiseGit

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
#include "SVNDCommitDlg.h"
#include "AppUtils.h"
#include "Git.h"

IMPLEMENT_DYNAMIC(CSVNDCommitDlg, CStandAloneDialog)

CSVNDCommitDlg::CSVNDCommitDlg(CWnd* pParent /*=nullptr*/)
	: CStandAloneDialog(CSVNDCommitDlg::IDD, pParent)
	, m_remember(FALSE)
	, m_rmdir(FALSE)
{
}

CSVNDCommitDlg::~CSVNDCommitDlg()
{
}

void CSVNDCommitDlg::DoDataExchange(CDataExchange* pDX)
{
	CStandAloneDialog::DoDataExchange(pDX);
	DDX_Check(pDX,IDC_RADIO_GIT_COMMIT,m_rmdir);
	DDX_Check(pDX,IDC_REMEMBER,m_remember);
}


BEGIN_MESSAGE_MAP(CSVNDCommitDlg, CStandAloneDialog)
	ON_BN_CLICKED(IDOK, &CSVNDCommitDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDHELP, OnHelp)
END_MESSAGE_MAP()

BOOL CSVNDCommitDlg::OnInitDialog()
{
	CStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	CString sWindowTitle;
	GetWindowText(sWindowTitle);
	CAppUtils::SetWindowTitle(m_hWnd, g_Git.m_CurrentDir, sWindowTitle);

	CheckRadioButton(IDC_RADIO_SVN_COMMIT, IDC_RADIO_GIT_COMMIT, IDC_RADIO_SVN_COMMIT);

	AdjustControlSize(IDC_RADIO_SVN_COMMIT);
	AdjustControlSize(IDC_RADIO_GIT_COMMIT);
	AdjustControlSize(IDC_REMEMBER);

	this->UpdateData(false);
	return TRUE;
}

void CSVNDCommitDlg::OnBnClickedOk()
{
	CStandAloneDialog::UpdateData(TRUE);

	CStandAloneDialog::OnOK();
}
