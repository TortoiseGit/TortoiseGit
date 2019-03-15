// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2017, 2019 - TortoiseGit

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

CResetDlg::CResetDlg(CWnd* pParent /*=nullptr*/)
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
	ON_BN_CLICKED(IDC_SHOW_MODIFIED_FILES, &CResetDlg::OnBnClickedShowModifiedFiles)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

// CResetDlg message handlers
BOOL CResetDlg::OnInitDialog()
{
	CHorizontalResizableStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	AdjustControlSize(IDC_RADIO_BRANCH);
	AdjustControlSize(IDC_RADIO_TAGS);
	AdjustControlSize(IDC_RADIO_VERSION);
	AdjustControlSize(IDC_RADIO_RESET_SOFT);
	AdjustControlSize(IDC_RADIO_RESET_MIXED);
	AdjustControlSize(IDC_RADIO_RESET_HARD);

	AddAnchor(IDC_SHOW_MODIFIED_FILES, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_GROUP_RESET_TYPE, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);

	CHOOSE_VERSION_ADDANCHOR;
	this->AddOthersToAnchor();

	EnableSaveRestore(L"ResetDlg");

	CString resetTo;
	CString currentBranch = g_Git.GetCurrentBranch();
	resetTo.Format(IDS_PROC_RESETBRANCH, static_cast<LPCTSTR>(currentBranch));
	GetDlgItem(IDC_GROUP_BASEON)->SetWindowTextW(resetTo);

	if (GitAdminDir::IsBareRepo(g_Git.m_CurrentDir))
	{
		m_ResetType = 0;
		DialogEnableWindow(IDC_RADIO_RESET_MIXED, FALSE);
		DialogEnableWindow(IDC_RADIO_RESET_HARD, FALSE);
	}
	this->CheckRadioButton(IDC_RADIO_RESET_SOFT,IDC_RADIO_RESET_HARD,IDC_RADIO_RESET_SOFT+m_ResetType);

	InitChooseVersion();
	SetDefaultChoose(IDC_RADIO_BRANCH);
	GetDlgItem(IDC_RADIO_RESET_SOFT + m_ResetType)->SetFocus();

	return FALSE;
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
	this->UpdateData(TRUE);
	UpdateRevsionName();
	m_ResetToVersion = m_VersionName;
	m_ResetType=this->GetCheckedRadioButton(IDC_RADIO_RESET_SOFT,IDC_RADIO_RESET_HARD)-IDC_RADIO_RESET_SOFT;
	return CHorizontalResizableStandAloneDialog::OnOK();
}

void CResetDlg::OnBnClickedShowModifiedFiles()
{
		CFileDiffDlg dlg;

		dlg.m_strRev1 = L"HEAD";
		dlg.m_strRev2 = GIT_REV_ZERO;

		dlg.DoModal();
}

void CResetDlg::OnDestroy()
{
	WaitForFinishLoading();
	__super::OnDestroy();
}
