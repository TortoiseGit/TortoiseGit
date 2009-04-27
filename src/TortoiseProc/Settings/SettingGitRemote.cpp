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

CSettingGitRemote::CSettingGitRemote(CString cmdPath)
	: ISettingsPropPage(CSettingGitRemote::IDD)
    , m_strRemote(_T(""))
    , m_strUrl(_T(""))
    , m_strPuttyKeyfile(_T(""))
	, m_cmdPath(cmdPath)
{

	m_ChangedMask = 0;
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
    DDX_Text(pDX, IDC_EDIT_PUTTY_KEY, m_strPuttyKeyfile);
}


BEGIN_MESSAGE_MAP(CSettingGitRemote, CPropertyPage)
    ON_BN_CLICKED(IDC_BUTTON_BROWSE, &CSettingGitRemote::OnBnClickedButtonBrowse)
    ON_BN_CLICKED(IDC_BUTTON_ADD, &CSettingGitRemote::OnBnClickedButtonAdd)
    ON_LBN_SELCHANGE(IDC_LIST_REMOTE, &CSettingGitRemote::OnLbnSelchangeListRemote)
    ON_EN_CHANGE(IDC_EDIT_REMOTE, &CSettingGitRemote::OnEnChangeEditRemote)
    ON_EN_CHANGE(IDC_EDIT_URL, &CSettingGitRemote::OnEnChangeEditUrl)
    ON_EN_CHANGE(IDC_EDIT_PUTTY_KEY, &CSettingGitRemote::OnEnChangeEditPuttyKey)
END_MESSAGE_MAP()

BOOL CSettingGitRemote::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

	//CString str=((CSettings*)GetParent())->m_CmdPath.GetWinPath();
	CString proj;
	if(	g_GitAdminDir.HasAdminDir(m_cmdPath,&proj) )
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
	
	this->GetDlgItem(IDC_EDIT_REMOTE)->EnableWindow(FALSE);
	this->UpdateData(FALSE);
	return TRUE;
}
// CSettingGitRemote message handlers

void CSettingGitRemote::OnBnClickedButtonBrowse()
{
    // TODO: Add your control notification handler code here
	CFileDialog dlg(TRUE,NULL,
					NULL,
					OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
					_T("Putty Private Key(*.ppk)|*.ppk|All Files(*.*)|*.*||"));
	
	this->UpdateData();
	if(dlg.DoModal()==IDOK)
	{
		this->m_strPuttyKeyfile = dlg.GetPathName();
		this->UpdateData(FALSE);
		OnEnChangeEditPuttyKey();
	}
	

}

void CSettingGitRemote::OnBnClickedButtonAdd()
{
    // TODO: Add your control notification handler code here
	this->m_strRemote.Empty();
	this->m_strUrl.Empty();
	this->m_strPuttyKeyfile.Empty();
	this->UpdateData(FALSE);
	this->GetDlgItem(IDC_EDIT_REMOTE)->EnableWindow(TRUE);
	this->m_ChangedMask |= REMOTE_NAME;
	this->m_ctrlRemoteList.SetCurSel(-1);
	GetDlgItem(IDC_BUTTON_ADD)->EnableWindow(FALSE);
}

void CSettingGitRemote::OnLbnSelchangeListRemote()
{
    // TODO: Add your control notification handler code here
	CWaitCursor wait;

	if(m_ChangedMask)
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


	cmd.Format(_T("git.exe config remote.%s.puttykeyfile"),remote);
	this->m_strPuttyKeyfile.Empty();
	if( g_Git.Run(cmd,&m_strPuttyKeyfile,CP_ACP) )
	{
		//CMessageBox::Show(NULL,output,_T("TortoiseGit"),MB_OK|MB_ICONERROR);
		//return;
	}
	start=0;
	m_strPuttyKeyfile = m_strPuttyKeyfile.Tokenize(_T("\n"),start);
	

	m_ChangedMask=0;

	GetDlgItem(IDC_BUTTON_ADD)->EnableWindow(TRUE);
	this->UpdateData(FALSE);

}

void CSettingGitRemote::OnEnChangeEditRemote()
{
    // TODO:  If this is a RICHEDIT control, the control will not
    // send this notification unless you override the ISettingsPropPage::OnInitDialog()
    // function and call CRichEditCtrl().SetEventMask()
    // with the ENM_CHANGE flag ORed into the mask.

    // TODO:  Add your control notification handler code here

	m_ChangedMask|=REMOTE_NAME;

	this->SetModified();
}

void CSettingGitRemote::OnEnChangeEditUrl()
{
    // TODO:  If this is a RICHEDIT control, the control will not
    // send this notification unless you override the ISettingsPropPage::OnInitDialog()
    // function and call CRichEditCtrl().SetEventMask()
    // with the ENM_CHANGE flag ORed into the mask.

    // TODO:  Add your control notification handler code here
	m_ChangedMask|=REMOTE_URL;
	this->SetModified();
}

void CSettingGitRemote::OnEnChangeEditPuttyKey()
{
    // TODO:  If this is a RICHEDIT control, the control will not
    // send this notification unless you override the ISettingsPropPage::OnInitDialog()
    // function and call CRichEditCtrl().SetEventMask()
    // with the ENM_CHANGE flag ORed into the mask.

    // TODO:  Add your control notification handler code here
	m_ChangedMask|=REMOTE_PUTTYKEY;
	this->SetModified();
}
void CSettingGitRemote::Save(CString key,CString value)
{
	CString cmd,out;
	cmd.Format(_T("git.exe config remote.%s.%s \"%s\""),this->m_strRemote,key,value);
	if(g_Git.Run(cmd,&out,CP_ACP))
	{
		CMessageBox::Show(NULL,out,_T("TortoiseGit"),MB_OK|MB_ICONERROR);
	}
}
BOOL CSettingGitRemote::OnApply()
{
	CWaitCursor wait;
	this->UpdateData();
	if(m_ChangedMask & REMOTE_NAME)
	{
		//Add Remote
		if( this->m_strRemote.IsEmpty() || this->m_strUrl.IsEmpty() )
		{
			CMessageBox::Show(NULL,_T("Remote Name and URL can't be Empty"),_T("TortoiseGit"),MB_OK|MB_ICONERROR);
			return FALSE;
		}
		CString cmd,out;
		cmd.Format(_T("git.exe remote add \"%s\" \"%s\""),m_strRemote,m_strUrl);
		if(g_Git.Run(cmd,&out,CP_ACP))
		{
			CMessageBox::Show(NULL,out,_T("TorotiseGit"),MB_OK|MB_ICONERROR);
			return FALSE;
		}
		m_ChangedMask &= ~REMOTE_URL;

		this->m_ctrlRemoteList.AddString(m_strRemote);
		GetDlgItem(IDC_BUTTON_ADD)->EnableWindow(TRUE);
	}
	if(m_ChangedMask & REMOTE_URL)
	{
		Save(_T("url"),this->m_strUrl);
	}

	if(m_ChangedMask & REMOTE_PUTTYKEY)
	{
		Save(_T("puttykeyfile"),this->m_strPuttyKeyfile);
	}

	this->GetDlgItem(IDC_EDIT_REMOTE)->EnableWindow(FALSE);
    SetModified(FALSE);

	m_ChangedMask = 0;
	return ISettingsPropPage::OnApply();
}