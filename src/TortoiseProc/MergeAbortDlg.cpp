// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2013, 2016-2017 - TortoiseGit

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
// MergeAbort.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "MergeAbortDlg.h"
#include "FileDiffDlg.h"
#include "AppUtils.h"

// CMergeAbortDlg dialog

IMPLEMENT_DYNAMIC(CMergeAbortDlg, CStateStandAloneDialog)

CMergeAbortDlg::CMergeAbortDlg(CWnd* pParent /*=nullptr*/)
	: CStateStandAloneDialog(CMergeAbortDlg::IDD, pParent)
	, m_ResetType(0)
{
}

CMergeAbortDlg::~CMergeAbortDlg()
{
}

void CMergeAbortDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CMergeAbortDlg, CStateStandAloneDialog)
	ON_BN_CLICKED(IDC_SHOW_MODIFIED_FILES, &CMergeAbortDlg::OnBnClickedShowModifiedFiles)
END_MESSAGE_MAP()


// CMergeAbortDlg message handlers
BOOL CMergeAbortDlg::OnInitDialog()
{
	CStateStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	CString sWindowTitle;
	GetWindowText(sWindowTitle);
	CAppUtils::SetWindowTitle(m_hWnd, g_Git.m_CurrentDir, sWindowTitle);

	AdjustControlSize(IDC_RADIO_RESET_MERGE);
	AdjustControlSize(IDC_RADIO_RESET_MIXED);
	AdjustControlSize(IDC_RADIO_RESET_HARD);

	EnableSaveRestore(L"MergeAbortDlg");

	this->CheckRadioButton(IDC_RADIO_RESET_MERGE, IDC_RADIO_RESET_HARD, IDC_RADIO_RESET_MERGE + m_ResetType);

	return FALSE;
}

void CMergeAbortDlg::OnOK()
{
	this->UpdateData(TRUE);
	m_ResetType = this->GetCheckedRadioButton(IDC_RADIO_RESET_MERGE, IDC_RADIO_RESET_HARD) - IDC_RADIO_RESET_MERGE;
	return CStateStandAloneDialog::OnOK();
}

void CMergeAbortDlg::OnBnClickedShowModifiedFiles()
{
		CFileDiffDlg dlg;

		dlg.m_strRev1 = L"HEAD";
		dlg.m_strRev2 = GIT_REV_ZERO;

		dlg.DoModal();
}
