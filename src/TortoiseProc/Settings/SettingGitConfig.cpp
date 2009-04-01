// SettingGitConfig.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "SettingGitConfig.h"
#include "Git.h"
#include "Settings.h"
#include "GitAdminDir.h"
#include "MessageBox.h"
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
    ON_BN_CLICKED(IDC_CHECK_GLOBAL, &CSettingGitConfig::OnBnClickedCheckGlobal)
    ON_EN_CHANGE(IDC_GIT_USERNAME, &CSettingGitConfig::OnEnChangeGitUsername)
    ON_EN_CHANGE(IDC_GIT_USEREMAIL, &CSettingGitConfig::OnEnChangeGitUseremail)
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

void CSettingGitConfig::OnBnClickedCheckGlobal()
{
    // TODO: Add your control notification handler code here
    SetModified();
}

void CSettingGitConfig::OnEnChangeGitUsername()
{
    // TODO:  If this is a RICHEDIT control, the control will not
    // send this notification unless you override the ISettingsPropPage::OnInitDialog()
    // function and call CRichEditCtrl().SetEventMask()
    // with the ENM_CHANGE flag ORed into the mask.

    // TODO:  Add your control notification handler code here
    SetModified();
}

void CSettingGitConfig::OnEnChangeGitUseremail()
{
    // TODO:  If this is a RICHEDIT control, the control will not
    // send this notification unless you override the ISettingsPropPage::OnInitDialog()
    // function and call CRichEditCtrl().SetEventMask()
    // with the ENM_CHANGE flag ORed into the mask.

    // TODO:  Add your control notification handler code here
    SetModified();
}

BOOL CSettingGitConfig::OnApply()
{
    CString cmd, out,global;
    this->UpdateData(FALSE);
	
	if(this->m_bGlobal)
		global = _T(" --global ");

    cmd.Format(_T("git.exe config %s user.name \"%s\""),global,this->m_UserName);
	if(g_Git.Run(cmd,&out,CP_ACP))
	{
		CMessageBox::Show(NULL,out,_T("TortoiseGit"),MB_OK|MB_ICONERROR);
		return FALSE;
	}
	out.Empty();
    cmd.Format(_T("git.exe config %s user.email \"%s\""),global,this->m_UserEmail);
	if(g_Git.Run(cmd,&out,CP_ACP))
	{
		CMessageBox::Show(NULL,out,_T("TortoiseGit"),MB_OK|MB_ICONERROR);
		return FALSE;
	}

    SetModified(FALSE);
	return ISettingsPropPage::OnApply();
}