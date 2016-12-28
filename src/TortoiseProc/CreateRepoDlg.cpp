// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2010-2014, 2016 - TortoiseGit

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
// CreateRepoDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "CreateRepoDlg.h"
#include "BrowseFolder.h"
#include "AppUtils.h"
#include "StringUtils.h"

// CCreateRepoDlg dialog

IMPLEMENT_DYNCREATE(CCreateRepoDlg, CStandAloneDialog)

CCreateRepoDlg::CCreateRepoDlg(CWnd* pParent /*=nullptr*/)
	: CStandAloneDialog(CCreateRepoDlg::IDD, pParent)
	, m_bBare(BST_UNCHECKED)
{
}

CCreateRepoDlg::~CCreateRepoDlg()
{
}

void CCreateRepoDlg::DoDataExchange(CDataExchange* pDX)
{
	CStandAloneDialog::DoDataExchange(pDX);

	DDX_Check(pDX,IDC_CHECK_BARE, m_bBare);
}

BOOL CCreateRepoDlg::OnInitDialog()
{
	CStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	AdjustControlSize(IDC_CHECK_BARE);

	CString sWindowTitle;
	GetWindowText(sWindowTitle);
	CAppUtils::SetWindowTitle(m_hWnd, m_folder, sWindowTitle);

	// Check if the folder ends with .git this indicates the use probably want this to be a bare repository
	if (CStringUtils::EndsWith(m_folder, L".git"))
	{
		m_bBare = TRUE;
		UpdateData(FALSE);
	}

	return TRUE;  // return TRUE  unless you set the focus to a control
}

BEGIN_MESSAGE_MAP(CCreateRepoDlg, CStandAloneDialog)
	ON_BN_CLICKED(IDC_CHECK_BARE, &CCreateRepoDlg::OnBnClickedCheckBare)
	ON_BN_CLICKED(IDHELP, OnHelp)
END_MESSAGE_MAP()

// CCloneDlg message handlers

void CCreateRepoDlg::OnOK()
{
	UpdateData(TRUE);

	CStandAloneDialog::OnOK();
}

void CCreateRepoDlg::OnCancel()
{
	CStandAloneDialog::OnCancel();
}

void CCreateRepoDlg::OnBnClickedCheckBare()
{
	this->UpdateData();
}
