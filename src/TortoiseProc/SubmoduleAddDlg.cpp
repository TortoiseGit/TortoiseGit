// SubmoduleAddDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SubmoduleAddDlg.h"
#include "BrowseFolder.h"

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


	AddOthersToAnchor();

	EnableSaveRestore(_T("SubmoduleAddDlg"));

	m_Repository.SetURLHistory(true);
	m_PathCtrl.SetPathHistory(true);

	m_Repository.LoadHistory(_T("Software\\TortoiseGit\\History\\SubModuleRepoURLS"), _T("url"));
	m_PathCtrl.LoadHistory(_T("Software\\TortoiseGit\\History\\SubModulePath"), _T("url"));
	m_Repository.SetCurSel(0);

	
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
	if (browseFolder.Show(GetSafeHwnd(), strDirectory) == CBrowseFolder::OK) 
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
	m_Repository.SaveHistory();
	m_PathCtrl.SaveHistory();
	__super::OnOK();
}