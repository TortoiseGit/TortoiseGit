// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2012 - TortoiseGit
// Copyright (C) 2012 - Sven Strickroth <email@cs-ware.de>

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
// IgnoreDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "IgnoreDlg.h"

// CIgnoreDlg dialog

IMPLEMENT_DYNAMIC(CIgnoreDlg, CStandAloneDialog)

CIgnoreDlg::CIgnoreDlg(CWnd* pParent /*=NULL*/)
	: CStandAloneDialog(CIgnoreDlg::IDD, pParent)
	, m_IgnoreType(0)
	, m_IgnoreFile(1)
{

}

CIgnoreDlg::~CIgnoreDlg()
{
}

void CIgnoreDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CIgnoreDlg, CStandAloneDialog)
END_MESSAGE_MAP()


// CIgnoreDlg message handlers
BOOL CIgnoreDlg::OnInitDialog()
{
	CStandAloneDialog::OnInitDialog();

	this->CheckRadioButton(IDC_RADIO_IGNOREFILE_GLOBALGITIGNORE, IDC_RADIO_IGNOREFILE_GITINFOEXCLUDE, IDC_RADIO_IGNOREFILE_GLOBALGITIGNORE + m_IgnoreType);

	AdjustControlSize(IDC_RADIO_IGNOREFILE_GLOBALGITIGNORE);
	AdjustControlSize(IDC_RADIO_IGNOREFILE_LOCALGITIGNORES);
	AdjustControlSize(IDC_RADIO_IGNOREFILE_GITINFOEXCLUDE);

	this->CheckRadioButton(IDC_RADIO_IGNORETYPE_ONLYINFOLDER, IDC_RADIO_IGNORETYPE_RECURSIVELY, IDC_RADIO_IGNORETYPE_ONLYINFOLDER + m_IgnoreType);

	AdjustControlSize(IDC_RADIO_IGNORETYPE_ONLYINFOLDER);
	AdjustControlSize(IDC_RADIO_IGNORETYPE_RECURSIVELY);

	return TRUE;
}

void CIgnoreDlg::OnOK()
{
	m_IgnoreFile = this->GetCheckedRadioButton(IDC_RADIO_IGNOREFILE_GLOBALGITIGNORE, IDC_RADIO_IGNOREFILE_GITINFOEXCLUDE) - IDC_RADIO_IGNOREFILE_GLOBALGITIGNORE;
	m_IgnoreType = this->GetCheckedRadioButton(IDC_RADIO_IGNORETYPE_ONLYINFOLDER, IDC_RADIO_IGNORETYPE_RECURSIVELY) - IDC_RADIO_IGNORETYPE_ONLYINFOLDER;
	return CStandAloneDialog::OnOK();
}
