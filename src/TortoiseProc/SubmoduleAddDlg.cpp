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

// SubmoduleAddDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SubmoduleAddDlg.h"
#include "BrowseFolder.h"
#include "MessageBox.h"
#include "AppUtils.h"

// CSubmoduleAddDlg dialog

IMPLEMENT_DYNAMIC(CSubmoduleAddDlg, CHorizontalResizableStandAloneDialog)

CSubmoduleAddDlg::CSubmoduleAddDlg(CWnd* pParent /*=NULL*/)
	: CHorizontalResizableStandAloneDialog(CSubmoduleAddDlg::IDD, pParent)
	, m_bBranch(FALSE)
	, m_strBranch(_T(""))
{

}

CSubmoduleAddDlg::~CSubmoduleAddDlg()
{
}

void CSubmoduleAddDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBOBOXEX_REPOSITORY, m_Repository);
	DDX_Control(pDX, IDC_COMBOBOXEX_PATH, m_PathCtrl);
	DDX_Check(pDX, IDC_BRANCH_CHECK, m_bBranch);
	DDX_Text(pDX, IDC_SUBMODULE_BRANCH, m_strBranch);
}


BEGIN_MESSAGE_MAP(CSubmoduleAddDlg, CHorizontalResizableStandAloneDialog)
	ON_COMMAND(IDC_REP_BROWSE,			OnRepBrowse)
	ON_COMMAND(IDC_BUTTON_PATH_BROWSE,	OnPathBrowse)
	ON_COMMAND(IDC_BRANCH_CHECK,		OnBranchCheck)
END_MESSAGE_MAP()


// CSubmoduleAddDlg message handlers

BOOL CSubmoduleAddDlg::OnInitDialog()
{
	CHorizontalResizableStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDCANCEL,BOTTOM_RIGHT);
	AddAnchor(IDC_GROUP_SUBMODULE,TOP_LEFT,BOTTOM_RIGHT);
	AddAnchor(IDC_COMBOBOXEX_REPOSITORY,TOP_LEFT,TOP_RIGHT);
	AddAnchor(IDC_COMBOBOXEX_PATH,TOP_LEFT,TOP_RIGHT);
	AddAnchor(IDC_REP_BROWSE,TOP_RIGHT);
	AddAnchor(IDC_BUTTON_PATH_BROWSE,TOP_RIGHT);
	AddAnchor(IDC_BRANCH_CHECK,BOTTOM_LEFT);
	AddAnchor(IDC_SUBMODULE_BRANCH,BOTTOM_LEFT,BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);

	AddOthersToAnchor();

	EnableSaveRestore(_T("SubmoduleAddDlg"));

	CString sWindowTitle;
	GetWindowText(sWindowTitle);
	CAppUtils::SetWindowTitle(m_hWnd, (g_Git.m_CurrentDir + _T("\\") + m_strPath).TrimRight('\\'), sWindowTitle);

	m_Repository.SetURLHistory(true);
	m_PathCtrl.SetPathHistory(true);

	m_Repository.LoadHistory(_T("Software\\TortoiseGit\\History\\SubModuleRepoURLS"), _T("url"));
	m_PathCtrl.LoadHistory(_T("Software\\TortoiseGit\\History\\SubModulePath"), _T("url"));
	m_PathCtrl.SetWindowText(m_strPath);
	m_Repository.SetCurSel(0);

	GetDlgItem(IDC_GROUP_SUBMODULE)->SetWindowText(CString(_T("Submodule of Project: "))+m_strProject);

	m_tooltips.Create(this);

	return TRUE;
}

void CSubmoduleAddDlg::OnRepBrowse()
{
	CBrowseFolder browseFolder;
	browseFolder.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
	CString strDirectory;
	this->m_Repository.GetWindowTextW(strDirectory);
	if (browseFolder.Show(GetSafeHwnd(), strDirectory) == CBrowseFolder::OK)
	{
		this->m_Repository.SetWindowTextW(strDirectory);
	}
}
void CSubmoduleAddDlg::OnPathBrowse()
{
	CBrowseFolder browseFolder;
	browseFolder.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
	CString strDirectory;
	this->m_PathCtrl.GetWindowTextW(strDirectory);
	if (browseFolder.Show(GetSafeHwnd(), strDirectory,g_Git.m_CurrentDir) == CBrowseFolder::OK)
	{
		this->m_PathCtrl.SetWindowTextW(strDirectory);
	}
}
void CSubmoduleAddDlg::OnBranchCheck()
{
	this->UpdateData();
	if(this->m_bBranch)
	{
		this->GetDlgItem(IDC_SUBMODULE_BRANCH)->ShowWindow(TRUE);
	}
	else
	{
		this->GetDlgItem(IDC_SUBMODULE_BRANCH)->ShowWindow(FALSE);
	}
}

void CSubmoduleAddDlg::OnOK()
{
	this->UpdateData();
	if(m_bBranch)
	{
		m_strBranch.Trim();
		if(m_strBranch.IsEmpty())
		{
			m_tooltips.ShowBalloon(IDC_SUBMODULE_BRANCH, IDS_ERR_MISSINGVALUE, IDS_ERR_ERROR, TTI_ERROR);
			return;
		}
	}
	m_Repository.SaveHistory();
	m_PathCtrl.SaveHistory();

	this->m_strPath=m_PathCtrl.GetString();
	this->m_strRepos=m_Repository.GetString();

	m_strPath.Trim();
	m_strRepos.Trim();
	if(m_strPath.IsEmpty())
	{
		m_tooltips.ShowBalloon(IDC_COMBOBOXEX_PATH, IDS_ERR_MISSINGVALUE, IDS_ERR_ERROR, TTI_ERROR);
		return;
	}
	if(m_strRepos.IsEmpty())
	{
		m_tooltips.ShowBalloon(IDC_COMBOBOXEX_REPOSITORY, IDS_ERR_MISSINGVALUE, IDS_ERR_ERROR, TTI_ERROR);
		return;
	}
	__super::OnOK();
}
