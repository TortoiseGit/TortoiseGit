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
#include "AppUtils.h"

// CResetDlg dialog

IMPLEMENT_DYNAMIC(CResetDlg, CHorizontalResizableStandAloneDialog)

CResetDlg::CResetDlg(CWnd* pParent /*=NULL*/)
	: CHorizontalResizableStandAloneDialog(CResetDlg::IDD, pParent)
	, CChooseVersion(this)
	, m_ResetType(1)
{

}

CResetDlg::~CResetDlg()
{
}

void CResetDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	CHOOSE_VERSION_DDX;
}


BEGIN_MESSAGE_MAP(CResetDlg, CHorizontalResizableStandAloneDialog)
	CHOOSE_VERSION_EVENT
	ON_BN_CLICKED(IDHELP, &CResetDlg::OnBnClickedHelp)
	ON_BN_CLICKED(IDC_SHOW_MODIFIED_FILES, &CResetDlg::OnBnClickedShowModifiedFiles)
END_MESSAGE_MAP()


// CResetDlg message handlers
BOOL CResetDlg::OnInitDialog()
{
	CHorizontalResizableStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	AddAnchor(IDC_SHOW_MODIFIED_FILES, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_GROUP_RESET_TYPE, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);

	CHOOSE_VERSION_ADDANCHOR;
	this->AddOthersToAnchor();

	AdjustControlSize(IDC_RADIO_BRANCH);
	AdjustControlSize(IDC_RADIO_TAGS);
	AdjustControlSize(IDC_RADIO_VERSION);
	AdjustControlSize(IDC_RADIO_RESET_SOFT);
	AdjustControlSize(IDC_RADIO_RESET_MIXED);
	AdjustControlSize(IDC_RADIO_RESET_HARD);

	EnableSaveRestore(_T("ResetDlg"));

	CString resetTo;
	CString currentBranch = g_Git.GetCurrentBranch();
	resetTo.Format(IDS_PROC_RESETBRANCH, currentBranch);
	GetDlgItem(IDC_GROUP_BASEON)->SetWindowTextW(resetTo);

	this->CheckRadioButton(IDC_RADIO_RESET_SOFT,IDC_RADIO_RESET_HARD,IDC_RADIO_RESET_SOFT+m_ResetType);

	Init();
	SetDefaultChoose(IDC_RADIO_BRANCH);
	GetDlgItem(IDC_RADIO_BRANCH)->SetFocus();

	return TRUE;
}

void CResetDlg::OnBnClickedChooseRadioHost()
{
	OnBnClickedChooseRadio();
}

void CResetDlg::OnBnClickedShow()
{
	OnBnClickedChooseVersion();
}

void CResetDlg::OnVersionChanged()
{
	UpdateData(TRUE);
	UpdateRevsionName();
	UpdateData(FALSE);
}

void CResetDlg::OnOK()
{
	m_ResetToVersion = m_VersionName;
	m_ResetType=this->GetCheckedRadioButton(IDC_RADIO_RESET_SOFT,IDC_RADIO_RESET_HARD)-IDC_RADIO_RESET_SOFT;
	return CHorizontalResizableStandAloneDialog::OnOK();
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
