// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2011, 2016 - TortoiseGit

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

// EditGoto.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseGitBlame.h"
#include "EditGotoDlg.h"


// CEditGotoDlg dialog

IMPLEMENT_DYNAMIC(CEditGotoDlg, CDialog)

CEditGotoDlg::CEditGotoDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(CEditGotoDlg::IDD, pParent)
	, m_LineNumber(0)
{
}

CEditGotoDlg::~CEditGotoDlg()
{
}

void CEditGotoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_LINENUMBER, m_LineNumber);
	DDV_MinMaxUInt(pDX, m_LineNumber, 0, 40000000);
}


BEGIN_MESSAGE_MAP(CEditGotoDlg, CDialog)

END_MESSAGE_MAP()


// CEditGotoDlg message handlers
