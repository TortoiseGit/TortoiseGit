// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2011 Sven Strickroth, <email@cs-ware.de>
//
// Based on PushDlg.cpp
// Copyright (C) 2003-2008 - TortoiseGit

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
#include "SVNDCommitDlg.h"

IMPLEMENT_DYNAMIC(CSVNDCommitDlg, CResizableStandAloneDialog)

CSVNDCommitDlg::CSVNDCommitDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CSVNDCommitDlg::IDD, pParent)
{
}

CSVNDCommitDlg::~CSVNDCommitDlg()
{
}

void CSVNDCommitDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Check(pDX,IDC_RADIO_GIT_COMMIT,m_rmdir);
}


BEGIN_MESSAGE_MAP(CSVNDCommitDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDOK, &CSVNDCommitDlg::OnBnClickedOk)
END_MESSAGE_MAP()

BOOL CSVNDCommitDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDCANCEL,BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);

	AddAnchor(IDC_GROUP_INFO, TOP_LEFT, BOTTOM_RIGHT);

	AddOthersToAnchor();

	CheckRadioButton(IDC_RADIO_SVN_COMMIT, IDC_RADIO_GIT_COMMIT, IDC_RADIO_SVN_COMMIT);

	this->UpdateData(false);
	return TRUE;
}

void CSVNDCommitDlg::OnBnClickedOk()
{
	CResizableStandAloneDialog::UpdateData(TRUE);

	CResizableStandAloneDialog::OnOK();
}
