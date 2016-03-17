// TortoiseGit - a Windows shell extension for easy version control

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
#include "SimplePrompt.h"

IMPLEMENT_DYNAMIC(CSimplePrompt, CDialog)
CSimplePrompt::CSimplePrompt(CWnd* pParent /*=nullptr*/)
	: CDialog(CSimplePrompt::IDD, pParent)
	, m_bSaveAuthentication(FALSE)
	, m_hParentWnd(nullptr)
{
}

CSimplePrompt::~CSimplePrompt()
{
}

void CSimplePrompt::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_USEREDIT, m_sUsername);
	DDX_Text(pDX, IDC_PASSEDIT, m_sPassword);
	DDX_Check(pDX, IDC_SAVECHECK, m_bSaveAuthentication);
	DDX_Text(pDX, IDC_REALM, m_sRealm);
}


BEGIN_MESSAGE_MAP(CSimplePrompt, CDialog)
END_MESSAGE_MAP()

BOOL CSimplePrompt::OnInitDialog()
{
	CDialog::OnInitDialog();

	GetDlgItem(IDC_USEREDIT)->SetFocus();
	if (!m_hParentWnd && hWndExplorer)
		CenterWindow(CWnd::FromHandle(m_hParentWnd));
	return FALSE;
}

