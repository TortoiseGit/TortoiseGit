// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2012 Sven Strickroth, <email@cs-ware.de>

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
#include "SubmoduleUpdateDlg.h"
#include "AppUtils.h"

IMPLEMENT_DYNAMIC(CSubmoduleUpdateDlg, CStandAloneDialog)

CSubmoduleUpdateDlg::CSubmoduleUpdateDlg(CWnd* pParent /*=NULL*/)
	: CStandAloneDialog(CSubmoduleUpdateDlg::IDD, pParent)
	, m_bInit(true)
	, m_bRecursive(FALSE)
	, m_bForce(FALSE)
{
}

CSubmoduleUpdateDlg::~CSubmoduleUpdateDlg()
{
}

void CSubmoduleUpdateDlg::DoDataExchange(CDataExchange* pDX)
{
	CStandAloneDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_CHECK_SUBMODULE_INIT, m_bInit);
	DDX_Check(pDX, IDC_CHECK_SUBMODULE_RECURSIVE, m_bRecursive);
	DDX_Check(pDX, IDC_FORCE, m_bForce);
}


BEGIN_MESSAGE_MAP(CSubmoduleUpdateDlg, CStandAloneDialog)
	ON_BN_CLICKED(IDOK, &CSubmoduleUpdateDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDHELP, &CSubmoduleUpdateDlg::OnBnClickedHelp)
END_MESSAGE_MAP()

BOOL CSubmoduleUpdateDlg::OnInitDialog()
{
	CStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	CString sWindowTitle;
	GetWindowText(sWindowTitle);
	CAppUtils::SetWindowTitle(m_hWnd, g_Git.m_CurrentDir, sWindowTitle);

	AdjustControlSize(IDC_CHECK_SUBMODULE_INIT);
	AdjustControlSize(IDC_CHECK_SUBMODULE_RECURSIVE);

	UpdateData(FALSE);

	return TRUE;
}

void CSubmoduleUpdateDlg::OnBnClickedOk()
{
	CStandAloneDialog::UpdateData(TRUE);

	CStandAloneDialog::OnOK();
}

void CSubmoduleUpdateDlg::OnBnClickedHelp()
{
	OnHelp();
}
