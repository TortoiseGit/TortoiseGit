// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009, 2015-2016 - TortoiseGit

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
// AddRemoteDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "AddRemoteDlg.h"


// CAddRemoteDlg dialog

IMPLEMENT_DYNAMIC(CAddRemoteDlg, CDialog)

CAddRemoteDlg::CAddRemoteDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(CAddRemoteDlg::IDD, pParent)
{
}

CAddRemoteDlg::~CAddRemoteDlg()
{
}

void CAddRemoteDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_NAME, m_Name);
	DDX_Text(pDX, IDC_EDIT_URL, m_Url);
}

BEGIN_MESSAGE_MAP(CAddRemoteDlg, CDialog)
	ON_BN_CLICKED(IDOK, &CAddRemoteDlg::OnBnClickedOk)
END_MESSAGE_MAP()

// CAddRemoteDlg message handlers

void CAddRemoteDlg::OnBnClickedOk()
{
	UpdateData();

	OnOK();
}
