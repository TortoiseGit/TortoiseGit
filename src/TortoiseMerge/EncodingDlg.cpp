// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2023 - TortoiseGit
// Copyright (C) 2013, 2020 - TortoiseSVN

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
#include "EncodingDlg.h"


const CFileTextLines::UnicodeType uctArray[] =
{
	CFileTextLines::UnicodeType::ASCII,
	CFileTextLines::UnicodeType::UTF16_LE,
	CFileTextLines::UnicodeType::UTF16_BE,
	CFileTextLines::UnicodeType::UTF16_LEBOM,
	CFileTextLines::UnicodeType::UTF16_BEBOM,
	CFileTextLines::UnicodeType::UTF32_LE,
	CFileTextLines::UnicodeType::UTF32_BE,
	CFileTextLines::UnicodeType::UTF8,
	CFileTextLines::UnicodeType::UTF8BOM
};

const EOL eolArray[] =
{
	EOL::AutoLine,
	EOL::CRLF,
	EOL::LF,
	EOL::CR,
	EOL::LFCR,
	EOL::VT,
	EOL::FF,
	EOL::NEL,
	EOL::LS,
	EOL::PS
};

// dialog

IMPLEMENT_DYNAMIC(CEncodingDlg, CStandAloneDialog)

CEncodingDlg::CEncodingDlg(CWnd* pParent)
	: CStandAloneDialog(CEncodingDlg::IDD, pParent)
	, texttype(CFileTextLines::UnicodeType::ASCII)
	, lineendings(EOL::AutoLine)
{
}

CEncodingDlg::~CEncodingDlg()
{
}

void CEncodingDlg::DoDataExchange(CDataExchange* pDX)
{
	CStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_ENCODING, m_Encoding);
	DDX_Control(pDX, IDC_COMBO_EOL, m_EOL);
}


BEGIN_MESSAGE_MAP(CEncodingDlg, CStandAloneDialog)
END_MESSAGE_MAP()


// message handlers

void CEncodingDlg::OnCancel()
{
	__super::OnCancel();
}

void CEncodingDlg::OnOK()
{
	UpdateData();
	texttype = uctArray[m_Encoding.GetCurSel()];
	lineendings = eolArray[m_EOL.GetCurSel()];
	__super::OnOK();
}

BOOL CEncodingDlg::OnInitDialog()
{
	CStandAloneDialog::OnInitDialog();

	CString sTitle;
	GetWindowText(sTitle);
	sTitle = view + sTitle;
	SetWindowText(sTitle);
	SetDlgItemText(IDC_VIEW, view);

	for (int i = 0; i < _countof(uctArray); i++)
	{
		m_Encoding.AddString(CFileTextLines::GetEncodingName(uctArray[i]));
		if (texttype == uctArray[i])
		{
			m_Encoding.SetCurSel(i);
		}
	}

	for (int i = 0; i < _countof(eolArray); i++)
	{
		m_EOL.AddString(GetEolName(eolArray[i]));
		if (lineendings == eolArray[i])
		{
			m_EOL.SetCurSel(i);
		}
	}

	return FALSE;
}
