// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2012 - TortoiseGit

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
// DeleteConflictDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "DeleteConflictDlg.h"
#include "AppUtils.h"

// CDeleteConflictDlg dialog

IMPLEMENT_DYNAMIC(CDeleteConflictDlg, CStandAloneDialog)

CDeleteConflictDlg::CDeleteConflictDlg(CWnd* pParent /*=NULL*/)
	: CStandAloneDialog(CDeleteConflictDlg::IDD, pParent)

	, m_LocalStatus(_T(""))
	, m_RemoteStatus(_T(""))
{
	m_bIsDelete =FALSE;
}

CDeleteConflictDlg::~CDeleteConflictDlg()
{
}

void CDeleteConflictDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Text(pDX, IDC_LOCAL_STATUS, m_LocalStatus);
	DDX_Text(pDX, IDC_REMOTE_STATUS, m_RemoteStatus);
}


BEGIN_MESSAGE_MAP(CDeleteConflictDlg, CStandAloneDialog)
	ON_BN_CLICKED(IDC_DELETE, &CDeleteConflictDlg::OnBnClickedDelete)
	ON_BN_CLICKED(IDC_MODIFY, &CDeleteConflictDlg::OnBnClickedModify)
END_MESSAGE_MAP()


BOOL CDeleteConflictDlg::OnInitDialog()
{
	CStandAloneDialog::OnInitDialog();

	if(this->m_bShowModifiedButton )
		this->GetDlgItem(IDC_MODIFY)->SetWindowText(CString(MAKEINTRESOURCE(IDS_SVNACTION_MODIFIED)));
	else
		this->GetDlgItem(IDC_MODIFY)->SetWindowText(CString(MAKEINTRESOURCE(IDS_PROC_CREATED)));

	CString sWindowTitle;
	GetWindowText(sWindowTitle);
	CAppUtils::SetWindowTitle(m_hWnd, this->m_File, sWindowTitle);

	return TRUE;
}
// CDeleteConflictDlg message handlers

void CDeleteConflictDlg::OnBnClickedDelete()
{
	m_bIsDelete = TRUE;
	OnOK();
}

void CDeleteConflictDlg::OnBnClickedModify()
{
	m_bIsDelete = FALSE;
	OnOK();
}
