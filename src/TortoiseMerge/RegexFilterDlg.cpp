// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2013 - TortoiseSVN

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
#include "TortoiseMerge.h"
#include "RegexFilterDlg.h"
#include <afxdialogex.h>


// CRegexFilterDlg dialog

IMPLEMENT_DYNAMIC(CRegexFilterDlg, CDialogEx)

CRegexFilterDlg::CRegexFilterDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CRegexFilterDlg::IDD, pParent)
	, m_sName(_T(""))
	, m_sRegex(_T(""))
	, m_sReplace(_T(""))
{
}

CRegexFilterDlg::~CRegexFilterDlg()
{
}

void CRegexFilterDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_NAME, m_sName);
	DDX_Text(pDX, IDC_REGEX, m_sRegex);
	DDX_Text(pDX, IDC_REPLACE, m_sReplace);
}


BEGIN_MESSAGE_MAP(CRegexFilterDlg, CDialogEx)
END_MESSAGE_MAP()


// CRegexFilterDlg message handlers


BOOL CRegexFilterDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	UpdateData(FALSE);

	GetDlgItem(IDC_NAME)->SetFocus();
	return FALSE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
