// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2020 - TortoiseGit
// Copyright (C) 2003-2006 - Stefan Kueng

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
#include "CreateChangelistDlg.h"


// CCreateChangelistDlg dialog

IMPLEMENT_DYNAMIC(CCreateChangelistDlg, CStandAloneDialog)

CCreateChangelistDlg::CCreateChangelistDlg(CWnd* pParent /*=nullptr*/)
	: CStandAloneDialog(CCreateChangelistDlg::IDD, pParent)
{
}

CCreateChangelistDlg::~CCreateChangelistDlg()
{
}

void CCreateChangelistDlg::DoDataExchange(CDataExchange* pDX)
{
	CStandAloneDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_NAME, m_sName);
}

BEGIN_MESSAGE_MAP(CCreateChangelistDlg, CStandAloneDialog)
	ON_EN_CHANGE(IDC_NAME, &CCreateChangelistDlg::OnEnChangeName)
END_MESSAGE_MAP()

// CCreateChangelistDlg message handlers

BOOL CCreateChangelistDlg::OnInitDialog()
{
	CStandAloneDialog::OnInitDialog();

	GetDlgItem(IDOK)->EnableWindow(FALSE);
	GetDlgItem(IDC_NAME)->SetFocus();

	return FALSE;
}

void CCreateChangelistDlg::OnEnChangeName()
{
	UpdateData();
	GetDlgItem(IDOK)->EnableWindow(!m_sName.IsEmpty());
}
