// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013-2016, 2018-2019 - TortoiseGit

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
	, CommonDialogFunctions(this)
{
	SecureZeroMemory(&m_password, sizeof(m_password));
	SecureZeroMemory(&m_passwordA, sizeof(m_passwordA));
}

CUserPassword::~CUserPassword()
{
	SecureZeroMemory(&m_password, sizeof(m_password));
	SecureZeroMemory(&m_passwordA, sizeof(m_passwordA));
}

void CUserPassword::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_USER_NAME, m_UserName);
}

BEGIN_MESSAGE_MAP(CUserPassword, CDialog)
	ON_BN_CLICKED(IDOK, &CUserPassword::OnBnClickedOk)
	ON_WM_DESTROY()
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
	GetDlgItem(IDC_USER_PASSWORD)->SendMessage(EM_SETLIMITTEXT, MAX_LENGTH_PASSWORD - 1, 0);
	if (GetDlgItem(IDC_USER_NAME)->GetWindowTextLength())
		GetDlgItem(IDC_USER_PASSWORD)->SetFocus();
	else
		GetDlgItem(IDC_USER_NAME)->SetFocus();
	return FALSE; // we set focus to the username/password textfield
}

void CUserPassword::OnBnClickedOk()
{
	UpdateData();
	if (m_UserName.IsEmpty())
	{
		GetDlgItem(IDC_USER_NAME)->SetFocus();
		ShowEditBalloon(IDC_USER_NAME, IDS_ERR_MISSINGVALUE, IDS_ERR_ERROR, TTI_ERROR);
		return;
	}

	GetDlgItem(IDC_USER_PASSWORD)->GetWindowText(m_password, _countof(m_password));

	auto lengthIncTerminator = WideCharToMultiByte(CP_UTF8, 0, m_password, static_cast<int>(wcslen(m_password)), m_passwordA, sizeof(m_passwordA) - 1, nullptr, nullptr);
	m_passwordA[lengthIncTerminator] = '\0';

	CDialog::OnOK();
}

void CUserPassword::OnDestroy()
{
	// overwrite password textfield contents with garbage in order to wipe the cache
	TCHAR gargabe[MAX_LENGTH_PASSWORD];
	wmemset(gargabe, L'*', _countof(gargabe));
	gargabe[_countof(gargabe) - 1] = L'\0';
	GetDlgItem(IDC_USER_PASSWORD)->SetWindowText(gargabe);
	gargabe[0] = L'\0';
	GetDlgItem(IDC_USER_PASSWORD)->SetWindowText(gargabe);

	__super::OnDestroy();
}
