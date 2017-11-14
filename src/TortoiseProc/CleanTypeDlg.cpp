// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2017 - TortoiseGit

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
// CleanTypeDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "CleanTypeDlg.h"
#include "Git.h"
#include "AppUtils.h"

// CCleanTypeDlg dialog

IMPLEMENT_DYNAMIC(CCleanTypeDlg, CStateStandAloneDialog)

CCleanTypeDlg::CCleanTypeDlg(CWnd* pParent /*=nullptr*/)
	: CStateStandAloneDialog(CCleanTypeDlg::IDD, pParent)
	, m_bDryRun(BST_UNCHECKED)
	, m_bSubmodules(BST_UNCHECKED)
	, m_bNoRecycleBin(!CRegDWORD(L"Software\\TortoiseGit\\RevertWithRecycleBin", TRUE))
	, m_bDirUnmanagedRepo(BST_UNCHECKED)
{
	CString WorkingDir=g_Git.m_CurrentDir;
	WorkingDir.Replace(L':', L'_');
	this->m_regDir  = CRegDWORD(L"Software\\TortoiseGit\\History\\CleanDir\\" + WorkingDir, 1);
	this->m_regType = CRegDWORD(L"Software\\TortoiseGit\\History\\CleanType\\" + WorkingDir, 0);

	this->m_bDir = this->m_regDir;
	this->m_CleanType = this->m_regType;
}

CCleanTypeDlg::~CCleanTypeDlg()
{
}

void CCleanTypeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_CHECK_DIR, m_bDir);
	DDX_Check(pDX, IDC_CHECK_DIR_UNMANAGEDREPO, m_bDirUnmanagedRepo);
	DDX_Check(pDX, IDC_CHECK_NORECYCLEBIN, m_bNoRecycleBin);
	DDX_Check(pDX, IDC_CHECK_DRYRUN, m_bDryRun);
	DDX_Check(pDX, IDC_CHECKSUBMODULES, m_bSubmodules);
	DDX_Radio(pDX, IDC_RADIO_CLEAN_ALL,m_CleanType);
}


BEGIN_MESSAGE_MAP(CCleanTypeDlg, CStateStandAloneDialog)
	ON_BN_CLICKED(IDC_CHECK_DIR, &CCleanTypeDlg::OnBnClickedCheckDir)
END_MESSAGE_MAP()


// CCleanTypeDlg message handlers

BOOL CCleanTypeDlg::OnInitDialog()
{
	CStateStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	AdjustControlSize(IDC_RADIO_CLEAN_ALL);
	AdjustControlSize(IDC_RADIO_CLEAN_NO);
	AdjustControlSize(IDC_RADIO_CLEAN_IGNORE);
	AdjustControlSize(IDC_CHECK_DIR);
	AdjustControlSize(IDC_CHECK_DIR_UNMANAGEDREPO);
	AdjustControlSize(IDC_CHECK_NORECYCLEBIN);
	AdjustControlSize(IDC_CHECK_DRYRUN);
	AdjustControlSize(IDC_CHECKSUBMODULES);

	EnableSaveRestore(L"CleanTypeDlg");

	SetDlgTitle();

	DialogEnableWindow(IDC_CHECK_DIR_UNMANAGEDREPO, m_bDir);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CCleanTypeDlg::OnOK()
{
	this->UpdateData();

	this->m_regDir = this->m_bDir;
	this->m_regType = this->m_CleanType ;

	CStateStandAloneDialog::OnOK();
}

void CCleanTypeDlg::SetDlgTitle()
{
	if (m_sTitle.IsEmpty())
		GetWindowText(m_sTitle);

	if (m_pathList.GetCount() == 1)
		CAppUtils::SetWindowTitle(m_hWnd, g_Git.CombinePath(m_pathList[0].GetUIPathString()), m_sTitle);
	else
		CAppUtils::SetWindowTitle(m_hWnd, g_Git.CombinePath(m_pathList.GetCommonRoot().GetDirectory()), m_sTitle);
}

void CCleanTypeDlg::OnBnClickedCheckDir()
{
	UpdateData();
	if (!m_bDir && m_bDirUnmanagedRepo)
	{
		m_bDirUnmanagedRepo = BST_UNCHECKED;
		UpdateData(FALSE);
	}
	DialogEnableWindow(IDC_CHECK_DIR_UNMANAGEDREPO, m_bDir);
}
