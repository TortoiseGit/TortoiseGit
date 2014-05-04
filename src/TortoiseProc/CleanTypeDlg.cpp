// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2014 - TortoiseGit

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

CCleanTypeDlg::CCleanTypeDlg(CWnd* pParent /*=NULL*/)
	: CStateStandAloneDialog(CCleanTypeDlg::IDD, pParent)

{
	CString WorkingDir=g_Git.m_CurrentDir;
	WorkingDir.Replace(_T(':'),_T('_'));
	this->m_regDir  = CRegDWORD( CString(_T("Software\\TortoiseGit\\History\\CleanDir\\"))+WorkingDir, 1);
	this->m_regType = CRegDWORD( CString(_T("Software\\TortoiseGit\\History\\CleanType\\"))+WorkingDir, 0);

	this->m_bDir = this->m_regDir;
	this->m_CleanType = this->m_regType;
	m_bNoRecycleBin = FALSE;
	m_bDryRun = FALSE;
	m_bSubmodules = FALSE;
}

CCleanTypeDlg::~CCleanTypeDlg()
{
}

void CCleanTypeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_CHECK_DIR, m_bDir);
	DDX_Check(pDX, IDC_CHECK_NORECYCLEBIN, m_bNoRecycleBin);
	DDX_Check(pDX, IDC_CHECK_DRYRUN, m_bDryRun);
	DDX_Check(pDX, IDC_CHECKSUBMODULES, m_bSubmodules);
	DDX_Radio(pDX, IDC_RADIO_CLEAN_ALL,m_CleanType);
}


BEGIN_MESSAGE_MAP(CCleanTypeDlg, CStateStandAloneDialog)
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
	AdjustControlSize(IDC_CHECK_NORECYCLEBIN);
	AdjustControlSize(IDC_CHECK_DRYRUN);
	AdjustControlSize(IDC_CHECKSUBMODULES);

	EnableSaveRestore(_T("CleanTypeDlg"));

	CString sWindowTitle;
	GetWindowText(sWindowTitle);
	CAppUtils::SetWindowTitle(m_hWnd, g_Git.m_CurrentDir, sWindowTitle);

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
