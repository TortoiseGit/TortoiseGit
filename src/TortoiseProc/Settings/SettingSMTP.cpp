// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009, 2013 - TortoiseGit

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
// SettingSMTP.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "SettingSMTP.h"

// CSettingSMTP dialog

IMPLEMENT_DYNAMIC(CSettingSMTP, CPropertyPage)

CSettingSMTP::CSettingSMTP()
	: CPropertyPage(CSettingSMTP::IDD)
	, m_Server(_T(""))
	, m_Port(0)
	, m_From(_T(""))
	, m_bAuth(FALSE)
	, m_User(_T(""))
	, m_Password(_T(""))
{
}

CSettingSMTP::~CSettingSMTP()
{
}

void CSettingSMTP::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_SMTP_SERVER, m_Server);
	DDX_Text(pDX, IDC_SMTP_PORT, m_Port);
	DDX_Text(pDX, IDC_SEND_ADDRESS, m_From);
	DDX_Check(pDX, IDC_SMTP_AUTH, m_bAuth);
	DDX_Text(pDX, IDC_SMTP_USER, m_User);
	DDX_Text(pDX, IDC_SMTP_PASSWORD, m_Password);
}


BEGIN_MESSAGE_MAP(CSettingSMTP, CPropertyPage)
END_MESSAGE_MAP()

// CSettingSMTP message handlers
