// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2012 - TortoiseGit

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
// ResetDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "ResetDlg.h"
#include "Git.h"
#include "FileDiffDlg.h"

// CResetDlg dialog

IMPLEMENT_DYNAMIC(CResetDlg, CStandAloneDialog)

CResetDlg::CResetDlg(CWnd* pParent /*=NULL*/)
	: CStandAloneDialog(CResetDlg::IDD, pParent)
	, m_ResetType(1)
{

}

CResetDlg::~CResetDlg()
{
}

void CResetDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CResetDlg, CStandAloneDialog)
	ON_BN_CLICKED(IDHELP, &CResetDlg::OnBnClickedHelp)
	ON_BN_CLICKED(IDC_SHOW_MODIFIED_FILES, &CResetDlg::OnBnClickedShowModifiedFiles)
END_MESSAGE_MAP()


// CResetDlg message handlers
BOOL CResetDlg::OnInitDialog()
{
	CStandAloneDialog::OnInitDialog();

	CString resetTo;
	CString currentBranch = g_Git.GetCurrentBranch();
	resetTo.Format(IDS_PROC_RESETBRANCH, currentBranch, m_ResetToVersion);
	GetDlgItem(IDC_RESET_BRANCH_NAME)->SetWindowTextW(resetTo);

	this->CheckRadioButton(IDC_RADIO_RESET_SOFT,IDC_RADIO_RESET_HARD,IDC_RADIO_RESET_SOFT+m_ResetType);

	AdjustControlSize(IDC_RADIO_RESET_SOFT);
	AdjustControlSize(IDC_RADIO_RESET_MIXED);
	AdjustControlSize(IDC_RADIO_RESET_HARD);

	return TRUE;
}

void CResetDlg::OnOK()
{
	m_ResetType=this->GetCheckedRadioButton(IDC_RADIO_RESET_SOFT,IDC_RADIO_RESET_HARD)-IDC_RADIO_RESET_SOFT;
	return CStandAloneDialog::OnOK();
}

void CResetDlg::OnBnClickedHelp()
{
	OnHelp();
}

void CResetDlg::OnBnClickedShowModifiedFiles()
{
		CFileDiffDlg dlg;

		dlg.m_strRev1 = _T("0000000000000000000000000000000000000000");
		dlg.m_strRev2 = _T("HEAD");

		dlg.DoModal();
}
