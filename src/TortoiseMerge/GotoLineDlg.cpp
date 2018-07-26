// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2011, 2013 - TortoiseSVN

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
#include "GotoLineDlg.h"
#include <afxdialogex.h>


// CGotoLineDlg dialog

IMPLEMENT_DYNAMIC(CGotoLineDlg, CDialogEx)

CGotoLineDlg::CGotoLineDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(CGotoLineDlg::IDD, pParent)
	, m_nLine(0)
	, m_nLow(-1)
	, m_nHigh(-1)
{
}

CGotoLineDlg::~CGotoLineDlg()
{
}

void CGotoLineDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_NUMBER, m_nLine);
	DDX_Control(pDX, IDC_NUMBER, m_cNumber);
}


BEGIN_MESSAGE_MAP(CGotoLineDlg, CDialogEx)
END_MESSAGE_MAP()




BOOL CGotoLineDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	if (!m_sLabel.IsEmpty())
	{
		SetDlgItemText(IDC_LINELABEL, m_sLabel);
	}
	SetDlgItemText(IDC_NUMBER, L"");
	GetDlgItem(IDC_NUMBER)->SetFocus();

	return FALSE;
}


void CGotoLineDlg::OnOK()
{
	UpdateData();
	if ((m_nLine < m_nLow)||(m_nLine > m_nHigh))
	{
		CString sError;
		sError.FormatMessage(IDS_GOTO_OUTOFRANGE, m_nLow, m_nHigh);
		m_cNumber.ShowBalloonTip(L"", sError);
		m_cNumber.SetSel(0, -1);
		return;
	}
	CDialogEx::OnOK();
}
