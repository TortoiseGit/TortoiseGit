// SettingGitConfig.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "SettingGitConfig.h"
#include "Git.h"
#include "Settings.h"
#include "GitAdminDir.h"
// CSettingGitConfig dialog

IMPLEMENT_DYNAMIC(CSettingGitConfig, ISettingsPropPage)

CSettingGitConfig::CSettingGitConfig()
	: ISettingsPropPage(CSettingGitConfig::IDD)
    , m_UserName(_T(""))
    , m_UserEmail(_T(""))
    , m_bGlobal(FALSE)
{

}

CSettingGitConfig::~CSettingGitConfig()
{
}

void CSettingGitConfig::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_GIT_USERNAME, m_UserName);
    DDX_Text(pDX, IDC_GIT_USEREMAIL, m_UserEmail);
    DDX_Check(pDX, IDC_CHECK_GLOBAL, m_bGlobal);
}


BEGIN_MESSAGE_MAP(CSettingGitConfig, CPropertyPage)
END_MESSAGE_MAP()

BOOL CSettingGitConfig::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

	m_UserName=g_Git.GetUserName();
	m_UserEmail=g_Git.GetUserEmail();
	
	CString str=((CSettings*)GetParent())->m_CmdPath.GetWinPath();
	CString proj;
	if(	g_GitAdminDir.HasAdminDir(str,&proj) )
	{
		this->SetWindowText(CString(_T("Config - "))+proj);
		this->GetDlgItem(IDC_CHECK_GLOBAL)->EnableWindow(TRUE);
	}
	else
	{
		m_bGlobal = TRUE;
		this->GetDlgItem(IDC_CHECK_GLOBAL)->EnableWindow(FALSE);
	}
	
	this->UpdateData(FALSE);
	return TRUE;
}
// CSettingGitConfig message handlers
