// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2011 - TortoiseGit

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
	ON_BN_CLICKED(IDC_BUTTON_REMOVE, &CSettingGitRemote::OnBnClickedButtonRemove)
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

	CString cmd, out, err;
	cmd=_T("git.exe remote");
	if(g_Git.Run(cmd, &out, &err, CP_ACP))
	{
		CMessageBox::Show(NULL, out + L"\n" + err, _T("TortoiseGit"), MB_OK|MB_ICONERROR);
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

	//this->GetDlgItem(IDC_EDIT_REMOTE)->EnableWindow(FALSE);
	this->UpdateData(FALSE);
	return TRUE;
}
// CSettingGitRemote message handlers

void CSettingGitRemote::OnBnClickedButtonBrowse()
{
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
	this->UpdateData();

	if(m_strRemote.IsEmpty())
	{
		CMessageBox::Show(NULL, _T("Remote name can't empty"), _T("TortoiseGit"),MB_OK|MB_ICONERROR);
		return;
	}
	if(m_strUrl.IsEmpty())
	{
		CMessageBox::Show(NULL, _T("Remote URL can't empty"),_T("TortoiseGit"), MB_OK|MB_ICONERROR);
		return;
	}

	m_strUrl.Replace(L'\\', L'/');
	UpdateData(FALSE);

	m_ChangedMask = REMOTE_NAME	|REMOTE_URL	|REMOTE_PUTTYKEY;
	if(IsRemoteExist(m_strRemote))
	{
		if(CMessageBox::Show(NULL, m_strRemote + _T(" is existed\n Do you want to overwrite it"),
						_T("TortoiseGit"), MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2) == IDYES)
		{
			m_ChangedMask &= ~REMOTE_NAME;
		}
		else
			return;
	}

	this->OnApply();
}

BOOL CSettingGitRemote::IsRemoteExist(CString &remote)
{
	CString str;
	for(int i=0;i<m_ctrlRemoteList.GetCount();i++)
	{
		m_ctrlRemoteList.GetText(i,str);
		if(str == remote)
		{
			return true;
		}
	}
	return false;
}

void CSettingGitRemote::OnLbnSelchangeListRemote()
{
	CWaitCursor wait;

	if(m_ChangedMask)
	{
		if(CMessageBox::Show(NULL,_T("Remote Config Changed\nDo you want to save change now or discard change"),
								 _T("TortoiseGit"),1,NULL,_T("Save"),_T("Discard")) == 1)
		{
			OnApply();
		}
	}
	SetModified(FALSE);

	CString cmd,output;
	int index;
	index = this->m_ctrlRemoteList.GetCurSel();
	if(index<0)
	{
		m_strUrl.Empty();
		m_strRemote.Empty();
		m_strPuttyKeyfile.Empty();
		this->UpdateData(FALSE);
		return;
	}
	CString remote;
	m_ctrlRemoteList.GetText(index,remote);
	this->m_strRemote=remote;

	cmd.Format(_T("remote.%s.url"),remote);
	m_strUrl.Empty();
	m_strUrl = g_Git.GetConfigValue(cmd,CP_ACP);

	cmd.Format(_T("remote.%s.puttykeyfile"),remote);

	this->m_strPuttyKeyfile=g_Git.GetConfigValue(cmd,CP_ACP);

	m_ChangedMask=0;

	GetDlgItem(IDC_BUTTON_ADD)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON_REMOVE)->EnableWindow(TRUE);
	this->UpdateData(FALSE);

}

void CSettingGitRemote::OnEnChangeEditRemote()
{
	m_ChangedMask|=REMOTE_NAME;

	this->UpdateData();
	if( (!this->m_strRemote.IsEmpty())&&(!this->m_strUrl.IsEmpty()) )
		this->SetModified();
	else
		this->SetModified(0);
}

void CSettingGitRemote::OnEnChangeEditUrl()
{
	m_ChangedMask|=REMOTE_URL;

	this->UpdateData();
	if( (!this->m_strRemote.IsEmpty())&&(!this->m_strUrl.IsEmpty()) )
		this->SetModified();
	else
		this->SetModified(0);
}

void CSettingGitRemote::OnEnChangeEditPuttyKey()
{
	m_ChangedMask|=REMOTE_PUTTYKEY;

	this->UpdateData();
	if( (!this->m_strRemote.IsEmpty())&&(!this->m_strUrl.IsEmpty()) )
		this->SetModified();
	else
		this->SetModified(0);
}
void CSettingGitRemote::Save(CString key,CString value)
{
	CString cmd,out;

	cmd.Format(_T("remote.%s.%s"),this->m_strRemote,key);
	if(g_Git.SetConfigValue(cmd, value, CONFIG_LOCAL, CP_ACP, &g_Git.m_CurrentDir))
	{
		CMessageBox::Show(NULL,_T("Fail to save config"),_T("TortoiseGit"),MB_OK|MB_ICONERROR);
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

	SetModified(FALSE);

	m_ChangedMask = 0;
	return ISettingsPropPage::OnApply();
}
void CSettingGitRemote::OnBnClickedButtonRemove()
{
	int index;
	index=m_ctrlRemoteList.GetCurSel();
	if(index>=0)
	{
		CString str;
		m_ctrlRemoteList.GetText(index,str);
		if(CMessageBox::Show(NULL,str + _T(" will be removed\n Are you sure?"),_T("TortoiseGit"),
						MB_YESNO|MB_ICONQUESTION) == IDYES)
		{
			CString cmd,out;
			cmd.Format(_T("git.exe remote rm %s"),str);
			if(g_Git.Run(cmd, &out, CP_ACP))
			{
				CMessageBox::Show(NULL, out,_T("TortoiseGit"),MB_OK|MB_ICONERROR);
				return;
			}

			m_ctrlRemoteList.DeleteString(index);
			OnLbnSelchangeListRemote();
		}
	}
}
