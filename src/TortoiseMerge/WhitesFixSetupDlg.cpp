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
#include "WhitesFixSetupDlg.h"
#include "WhitesFixDlg.h"

// dialog

IMPLEMENT_DYNAMIC(CWhitesFixSetupDlg, CDialog)

CWhitesFixSetupDlg::CWhitesFixSetupDlg(CWnd* pParent)
	: CDialog(CWhitesFixSetupDlg::IDD, pParent)
	, convertSpaces(false)
	, trimRight(false)
	, fixEols(false)
{
}

CWhitesFixSetupDlg::~CWhitesFixSetupDlg()
{
}

void CWhitesFixSetupDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TRIM, m_trimRight);
	DDX_Control(pDX, IDC_USESPACES, m_useSpaces);
	DDX_Control(pDX, IDC_FIXEOLS, m_fixEols);
}


BEGIN_MESSAGE_MAP(CWhitesFixSetupDlg, CDialog)
END_MESSAGE_MAP()


// message handlers

void CWhitesFixSetupDlg::OnCancel()
{
	__super::OnCancel();
}

void CWhitesFixSetupDlg::OnOK()
{
	UpdateData();
	// update config
	DWORD nTemp = CWhitesFixDlg::GetSettingsMap() & TMERGE_WSF_GLOBALCHECK;
	nTemp |= m_useSpaces.GetCheck() == BST_CHECKED ? TMERGE_WSF_ASKFIX_TABSPACE : 0;
	nTemp |= m_trimRight.GetCheck() == BST_CHECKED ? TMERGE_WSF_ASKFIX_TRAIL : 0;
	nTemp |= m_fixEols.GetCheck()   == BST_CHECKED ? TMERGE_WSF_ASKFIX_EOL : 0;
	CWhitesFixDlg::SetSettingsMap(nTemp);

	__super::OnOK();
}

BOOL CWhitesFixSetupDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	DWORD nMap = CWhitesFixDlg::GetSettingsMap();
	m_useSpaces.SetCheck(nMap & TMERGE_WSF_ASKFIX_TABSPACE ? BST_CHECKED : BST_UNCHECKED);
	m_trimRight.SetCheck(nMap & TMERGE_WSF_ASKFIX_TRAIL ? BST_CHECKED : BST_UNCHECKED);
	m_fixEols.SetCheck(nMap & TMERGE_WSF_ASKFIX_EOL ? BST_CHECKED : BST_UNCHECKED);

	return FALSE;
}

