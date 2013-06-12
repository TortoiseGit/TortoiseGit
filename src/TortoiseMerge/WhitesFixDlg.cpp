// TortoiseGitMerge - a Diff/Patch program

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
#include "WhitesFixDlg.h"
#include "EncodingDlg.h"

// dialog

IMPLEMENT_DYNAMIC(CWhitesFixDlg, CDialog)

CWhitesFixDlg::CWhitesFixDlg(CWnd* pParent)
	: CDialog(CWhitesFixDlg::IDD, pParent)
	, lineendings(EOL_AUTOLINE)
	, convertSpacesEnabled(false)
	, convertTabsEnabled(false)
	, trimRightEnabled(false)
	, fixEolsEnabled(false)
	, convertSpaces(false)
	, convertTabs(false)
	, trimRight(false)
	, fixEols(false)
	, stopAsking(false)
{
}

CWhitesFixDlg::~CWhitesFixDlg()
{
}

void CWhitesFixDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TRIM, m_trimRight);
	DDX_Control(pDX, IDC_USESPACES, m_convertTabs);
	DDX_Control(pDX, IDC_USETABS, m_convertSpaces);
	DDX_Control(pDX, IDC_FIXEOLS, m_fixEols);
	DDX_Control(pDX, IDC_COMBO_EOL, m_EOL);
	DDX_Control(pDX, IDC_STOPASKING, m_stopAsking);
}


BEGIN_MESSAGE_MAP(CWhitesFixDlg, CDialog)
END_MESSAGE_MAP()


// message handlers

void CWhitesFixDlg::OnCancel()
{
	__super::OnCancel();
}

void CWhitesFixDlg::OnOK()
{
	UpdateData();
	convertSpaces = m_convertSpaces.GetCheck();
	convertTabs = m_convertTabs.GetCheck();
	trimRight = m_trimRight.GetCheck();
	fixEols = m_fixEols.GetCheck();
	lineendings = eolArray[m_EOL.GetCurSel()+1];
	stopAsking = m_stopAsking.GetCheck();

	__super::OnOK();
}

BOOL CWhitesFixDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CString sTitle;
	GetWindowText(sTitle);
//	sTitle = view + sTitle;
	SetWindowText(sTitle);
//	SetDlgItemText(IDC_VIEW, view);

	m_trimRight.EnableWindow(trimRightEnabled);
	m_trimRight.SetCheck(trimRight);
	m_convertTabs.EnableWindow(convertTabsEnabled);
	m_convertTabs.SetCheck(convertTabs);
	m_convertSpaces.EnableWindow(convertSpacesEnabled);
	m_convertSpaces.SetCheck(convertSpaces);
	m_fixEols.EnableWindow(fixEolsEnabled);
	m_fixEols.SetCheck(fixEols);
	m_EOL.EnableWindow(fixEolsEnabled);
	for (int i = 1; i < _countof(eolArray); i++)
	{
		m_EOL.AddString(GetEolName(eolArray[i]));
		if (lineendings == eolArray[i])
		{
			m_EOL.SetCurSel(i-1);
		}
	}

	return FALSE;
}
