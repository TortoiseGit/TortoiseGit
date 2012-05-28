// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2011-2012 Sven Strickroth, <email@cs-ware.de>

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
#include "StashSave.h"
#include "AppUtils.h"

IMPLEMENT_DYNAMIC(CStashSaveDlg, CHorizontalResizableStandAloneDialog)

CStashSaveDlg::CStashSaveDlg(CWnd* pParent /*=NULL*/)
	: CHorizontalResizableStandAloneDialog(CStashSaveDlg::IDD, pParent)
	, m_bIncludeUntracked(FALSE)
{
}

CStashSaveDlg::~CStashSaveDlg()
{
}

void CStashSaveDlg::DoDataExchange(CDataExchange* pDX)
{
	CHorizontalResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_STASHMESSAGE, m_sMessage);
	DDX_Check(pDX, IDC_CHECK_UNTRACKED, m_bIncludeUntracked);
}

BEGIN_MESSAGE_MAP(CStashSaveDlg, CHorizontalResizableStandAloneDialog)
	ON_BN_CLICKED(IDOK, &CStashSaveDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDHELP, &CStashSaveDlg::OnBnClickedHelp)
END_MESSAGE_MAP()

BOOL CStashSaveDlg::OnInitDialog()
{
	CHorizontalResizableStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	AddAnchor(IDC_GROUP_STASHMESSAGE, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_STASHMESSAGE, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_GROUP_OPTION, TOP_LEFT, TOP_RIGHT);

	AdjustControlSize(IDC_CHECK_UNTRACKED);

	CString sWindowTitle;
	GetWindowText(sWindowTitle);
	CAppUtils::SetWindowTitle(m_hWnd, g_Git.m_CurrentDir, sWindowTitle);

	this->UpdateData(false);

	if (CAppUtils::GetMsysgitVersion() < 0x01070700)
		GetDlgItem(IDC_CHECK_UNTRACKED)->EnableWindow(FALSE);

	return TRUE;
}

void CStashSaveDlg::OnBnClickedOk()
{
	CHorizontalResizableStandAloneDialog::UpdateData(TRUE);

	CHorizontalResizableStandAloneDialog::OnOK();
}

void CStashSaveDlg::OnBnClickedHelp()
{
	OnHelp();
}
