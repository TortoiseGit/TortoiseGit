// SubmoduleAddDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SubmoduleAddDlg.h"
#include "BrowseFolder.h"
#include "MessageBox.h"

// CSubmoduleAddDlg dialog

IMPLEMENT_DYNAMIC(CSubmoduleAddDlg, CResizableStandAloneDialog)

CSubmoduleAddDlg::CSubmoduleAddDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CSubmoduleAddDlg::IDD, pParent)
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


BEGIN_MESSAGE_MAP(CSubmoduleAddDlg, CResizableStandAloneDialog)
	ON_COMMAND(IDC_REP_BROWSE,			OnRepBrowse)
	ON_COMMAND(IDC_BUTTON_PATH_BROWSE,	OnPathBrowse)
	ON_COMMAND(IDC_BRANCH_CHECK,		OnBranchCheck)
END_MESSAGE_MAP()


// CSubmoduleAddDlg message handlers

BOOL CSubmoduleAddDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

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

	m_Repository.SetURLHistory(true);
	m_PathCtrl.SetPathHistory(true);

	m_Repository.LoadHistory(_T("Software\\TortoiseGit\\History\\SubModuleRepoURLS"), _T("url"));
	m_PathCtrl.LoadHistory(_T("Software\\TortoiseGit\\History\\SubModulePath"), _T("url"));
	m_PathCtrl.SetWindowText(m_strPath);
	m_Repository.SetCurSel(0);

	GetDlgItem(IDC_GROUP_SUBMODULE)->SetWindowText(CString(_T("Submodule of Project: "))+m_strProject);
	
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
	}else
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
			CMessageBox::Show(NULL,_T("Branch can't be empty"),_T("TortoiseGit"),MB_OK|MB_ICONERROR);
			return ;
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
		CMessageBox::Show(NULL,_T("Path can't be empty"),_T("TortoiseGit"),MB_OK|MB_ICONERROR);
			return ;
	}
	if(m_strRepos.IsEmpty())
	{
		CMessageBox::Show(NULL,_T("Repository can't be empty"),_T("TortoiseGit"),MB_OK|MB_ICONERROR);
			return ;
	}
	__super::OnOK();
}