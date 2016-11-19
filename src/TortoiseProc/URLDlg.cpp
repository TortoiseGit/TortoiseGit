// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN

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
#include "URLDlg.h"

IMPLEMENT_DYNAMIC(CURLDlg, CResizableStandAloneDialog)
CURLDlg::CURLDlg(CWnd* pParent /*=nullptr*/)
	: CResizableStandAloneDialog(CURLDlg::IDD, pParent)
{
	m_height = 0;
}

CURLDlg::~CURLDlg()
{
}

void CURLDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_URLCOMBO, m_URLCombo);
}


BEGIN_MESSAGE_MAP(CURLDlg, CResizableStandAloneDialog)
	ON_WM_SIZING()
END_MESSAGE_MAP()


BOOL CURLDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	m_URLCombo.SetURLHistory(TRUE);
	m_URLCombo.LoadHistory(L"Software\\TortoiseGit\\History\\repoURLS", L"url");
	m_URLCombo.SetCurSel(0);
	m_URLCombo.SetFocus();

	RECT rect;
	GetWindowRect(&rect);
	m_height = rect.bottom - rect.top;
	AddAnchor(IDC_LABEL, TOP_LEFT);
	AddAnchor(IDC_URLCOMBO, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	EnableSaveRestore(L"URLDlg");
	return FALSE;
}

void CURLDlg::OnOK()
{
	if (m_URLCombo.IsWindowEnabled())
	{
		m_URLCombo.SaveHistory();
		m_url = m_URLCombo.GetString();
		UpdateData();
	}

	CResizableStandAloneDialog::OnOK();
}


void CURLDlg::OnSizing(UINT fwSide, LPRECT pRect)
{
	// don't allow the dialog to be changed in height
	switch (fwSide)
	{
	case WMSZ_BOTTOM:
	case WMSZ_BOTTOMLEFT:
	case WMSZ_BOTTOMRIGHT:
		pRect->bottom = pRect->top + m_height;
		break;
	case WMSZ_TOP:
	case WMSZ_TOPLEFT:
	case WMSZ_TOPRIGHT:
		pRect->top = pRect->bottom - m_height;
		break;
	}
	CResizableStandAloneDialog::OnSizing(fwSide, pRect);
}
