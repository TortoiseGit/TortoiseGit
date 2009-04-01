// SettingGitRemote.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "SettingGitRemote.h"
#include "Settings.h"
#include "GitAdminDir.h"
#include "MessageBox.h"
#include "git.h"

// CSettingGitRemote dialog

IMPLEMENT_DYNAMIC(CSettingGitRemote, ISettingsPropPage)

CSettingGitRemote::CSettingGitRemote()
	: ISettingsPropPage(CSettingGitRemote::IDD)
    , m_strRemote(_T(""))
    , m_strUrl(_T(""))
    , m_strPuttyKeyfile(_T(""))
{
	m_bChanged=FALSE;
}

CSettingGitRemote::~CSettingGitRemote()
{
}

void CSettingGitRemote::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_REMOTE, m_ctrlRemoteList);
    DDX_Text(pDX, IDC_EDIT_REMOTE, m_strRemote);
    DDX_Text(pDX, IDC_EDIT_URL, m_strUrl);
    DDX_Control(pDX, IDC_CHECK_ISAUTOLOADPUTTYKEY, m_bAutoLoad);
    DDX_Text(pDX, IDC_EDIT_PUTTY_KEY, m_strPuttyKeyfile);
}


BEGIN_MESSAGE_MAP(CSettingGitRemote, CPropertyPage)
    ON_BN_CLICKED(IDC_BUTTON_BROWSE, &CSettingGitRemote::OnBnClickedButtonBrowse)
    ON_BN_CLICKED(IDC_BUTTON_ADD, &CSettingGitRemote::OnBnClickedButtonAdd)
    ON_LBN_SELCHANGE(IDC_LIST_REMOTE, &CSettingGitRemote::OnLbnSelchangeListRemote)
    ON_EN_CHANGE(IDC_EDIT_REMOTE, &CSettingGitRemote::OnEnChangeEditRemote)
    ON_EN_CHANGE(IDC_EDIT_URL, &CSettingGitRemote::OnEnChangeEditUrl)
    ON_BN_CLICKED(IDC_CHECK_ISAUTOLOADPUTTYKEY, &CSettingGitRemote::OnBnClickedCheckIsautoloadputtykey)
    ON_EN_CHANGE(IDC_EDIT_PUTTY_KEY, &CSettingGitRemote::OnEnChangeEditPuttyKey)
END_MESSAGE_MAP()

BOOL CSettingGitRemote::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

	CString str=((CSettings*)GetParent())->m_CmdPath.GetWinPath();
	CString proj;
	if(	g_GitAdminDir.HasAdminDir(str,&proj) )
	{
		this->SetWindowText(CString(_T("Config - "))+proj);
	}

	CString cmd,out;
	cmd=_T("git.exe remote");
	if(g_Git.Run(cmd,&out,CP_ACP))
	{
		CMessageBox::Show(NULL,out,_T("TortoiseGit"),MB_OK|MB_ICONERROR);
		return FALSE;
	}
	int start =0;
	m_ctrlRemoteList.ResetContent();
	do
	{
		CString one;
		one=out.Tokenize(_T("\n"),start);
		if(!one.IsEmpty())
			this->m_ctrlRemoteList.AddString(one);

	}while(start>=0);
		
	this->UpdateData(FALSE);
	return TRUE;
}
// CSettingGitRemote message handlers

void CSettingGitRemote::OnBnClickedButtonBrowse()
{
    // TODO: Add your control notification handler code here
}

void CSettingGitRemote::OnBnClickedButtonAdd()
{
    // TODO: Add your control notification handler code here
}

void CSettingGitRemote::OnLbnSelchangeListRemote()
{
    // TODO: Add your control notification handler code here
	if(m_bChanged)
	{
		if(CMessageBox::Show(NULL,_T("Remote Config Changed\nDo you want to save change now or discard change"),
								 _T("TortoiseGit"),MB_YESNO) == IDYES)
		{
			OnApply();
		}
	}
	SetModified(FALSE);
	
	CString cmd,output;
	int index;
	index = this->m_ctrlRemoteList.GetCurSel();
	if(index<0)
		return;

	CString remote;
	m_ctrlRemoteList.GetText(index,remote);
	this->m_strRemote=remote;

	cmd.Format(_T("git.exe config remote.%s.url"),remote);
	m_strUrl.Empty();
	if( g_Git.Run(cmd,&m_strUrl,CP_ACP) )
	{
		//CMessageBox::Show(NULL,output,_T("TortoiseGit"),MB_OK|MB_ICONERROR);
		//return;
	}
	
	int start=0;
	m_strUrl = m_strUrl.Tokenize(_T("\n"),start);


	cmd.Format(_T("git.exe config remote.%s.puttykey"),remote);
	this->m_strPuttyKeyfile.Empty();
	if( g_Git.Run(cmd,&m_strPuttyKeyfile,CP_ACP) )
	{
		//CMessageBox::Show(NULL,output,_T("TortoiseGit"),MB_OK|MB_ICONERROR);
		//return;
	}
	start=0;
	m_strPuttyKeyfile = m_strPuttyKeyfile.Tokenize(_T("\n"),start);
	

	cmd.Format(_T("git.exe config remote.%s.puttykeyautoload"),remote);
	CString autoload;
	if( g_Git.Run(cmd,&autoload,CP_ACP) )
	{
		//CMessageBox::Show(NULL,output,_T("TortoiseGit"),MB_OK|MB_ICONERROR);
		//return;
	}

	start=0;
	autoload = autoload.Tokenize(_T("\n"),start);
	m_bAutoLoad.SetCheck(false);
	if( autoload == _T("true"))
	{
		m_bAutoLoad.SetCheck(true);
	}
	
	this->UpdateData(FALSE);

}

void CSettingGitRemote::OnEnChangeEditRemote()
{
    // TODO:  If this is a RICHEDIT control, the control will not
    // send this notification unless you override the ISettingsPropPage::OnInitDialog()
    // function and call CRichEditCtrl().SetEventMask()
    // with the ENM_CHANGE flag ORed into the mask.

    // TODO:  Add your control notification handler code here
}

void CSettingGitRemote::OnEnChangeEditUrl()
{
    // TODO:  If this is a RICHEDIT control, the control will not
    // send this notification unless you override the ISettingsPropPage::OnInitDialog()
    // function and call CRichEditCtrl().SetEventMask()
    // with the ENM_CHANGE flag ORed into the mask.

    // TODO:  Add your control notification handler code here
}

void CSettingGitRemote::OnBnClickedCheckIsautoloadputtykey()
{
    // TODO: Add your control notification handler code here
}

void CSettingGitRemote::OnEnChangeEditPuttyKey()
{
    // TODO:  If this is a RICHEDIT control, the control will not
    // send this notification unless you override the ISettingsPropPage::OnInitDialog()
    // function and call CRichEditCtrl().SetEventMask()
    // with the ENM_CHANGE flag ORed into the mask.

    // TODO:  Add your control notification handler code here
}

BOOL CSettingGitRemote::OnApply()
{
  
    SetModified(FALSE);
	return ISettingsPropPage::OnApply();
}