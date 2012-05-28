// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2012 - TortoiseGit

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
#include "SVNIgnoreTypeDlg.h"
#include "AppUtils.h"

// CSVNIgnoreTypeDlg dialog

IMPLEMENT_DYNAMIC(CSVNIgnoreTypeDlg, CResizableStandAloneDialog)

CSVNIgnoreTypeDlg::CSVNIgnoreTypeDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CSVNIgnoreTypeDlg::IDD, pParent)
	, m_SVNIgnoreType(0)
{

}

CSVNIgnoreTypeDlg::~CSVNIgnoreTypeDlg()
{
}

void CSVNIgnoreTypeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Radio(pDX, IDC_RADIO_EXCLUDE, m_SVNIgnoreType);
}


BEGIN_MESSAGE_MAP(CSVNIgnoreTypeDlg, CResizableStandAloneDialog)
END_MESSAGE_MAP()


// CSVNIgnoreTypeDlg message handlers

BOOL CSVNIgnoreTypeDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	AddAnchor(IDC_RADIO_EXCLUDE,  TOP_LEFT);
	AddAnchor(IDC_RADIO_GITIGNORE,TOP_LEFT);
	AddAnchor(IDC_GROUP_TYPE,TOP_LEFT, BOTTOM_RIGHT);

	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDCANCEL,BOTTOM_RIGHT);

	AdjustControlSize(IDC_RADIO_EXCLUDE);
	AdjustControlSize(IDC_RADIO_GITIGNORE);

	CString sWindowTitle;
	GetWindowText(sWindowTitle);
	CAppUtils::SetWindowTitle(m_hWnd, g_Git.m_CurrentDir, sWindowTitle);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
