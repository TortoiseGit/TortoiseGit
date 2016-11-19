// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013, 2015, 2016 - TortoiseGit

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
// UserPassword.cpp : implementation file
//
#include "stdafx.h"
#include "resource.h"
#include "UserPassword.h"

// CUserPassword dialog

IMPLEMENT_DYNAMIC(CUserPassword, CDialog)

CUserPassword::CUserPassword(CWnd* pParent /*=nullptr*/)
	: CDialog(CUserPassword::IDD, pParent)
{
}

CUserPassword::~CUserPassword()
{
}

void CUserPassword::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_USER_NAME, m_UserName);
	DDX_Text(pDX, IDC_USER_PASSWORD, m_Password);
}

BEGIN_MESSAGE_MAP(CUserPassword, CDialog)
END_MESSAGE_MAP()

// CUserPassword message handlers

BOOL CUserPassword::OnInitDialog()
{
	CDialog::OnInitDialog();
	if (!m_URL.IsEmpty())
	{
		CString title;
		this->GetWindowText(title);
		title += L" - ";
		title += m_URL;
		this->SetWindowText(title);
	}
	if (GetDlgItem(IDC_USER_NAME)->GetWindowTextLength())
		GetDlgItem(IDC_USER_PASSWORD)->SetFocus();
	else
		GetDlgItem(IDC_USER_NAME)->SetFocus();
	return FALSE; // we set focus to the username/password textfield
}
