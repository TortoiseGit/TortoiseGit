// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2013 - TortoiseGit

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
#include "AppUtils.h"
#include "git.h"

// CSettingGitRemote dialog

IMPLEMENT_DYNAMIC(CSettingGitRemote, ISettingsPropPage)

CSettingGitRemote::CSettingGitRemote(CString cmdPath)
	: ISettingsPropPage(CSettingGitRemote::IDD)
	, m_strRemote(_T(""))
	, m_strUrl(_T(""))
	, m_strPuttyKeyfile(_T(""))
	, m_cmdPath(cmdPath)
	, m_bNoFetch(false)
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
	DDX_Control(pDX, IDC_COMBO_TAGOPT, m_ctrlTagOpt);
}


BEGIN_MESSAGE_MAP(CSettingGitRemote, CPropertyPage)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE, &CSettingGitRemote::OnBnClickedButtonBrowse)
	ON_BN_CLICKED(IDC_BUTTON_ADD, &CSettingGitRemote::OnBnClickedButtonAdd)
	ON_LBN_SELCHANGE(IDC_LIST_REMOTE, &CSettingGitRemote::OnLbnSelchangeListRemote)
	ON_EN_CHANGE(IDC_EDIT_REMOTE, &CSettingGitRemote::OnEnChangeEditRemote)
	ON_EN_CHANGE(IDC_EDIT_URL, &CSettingGitRemote::OnEnChangeEditUrl)
	ON_EN_CHANGE(IDC_EDIT_PUTTY_KEY, &CSettingGitRemote::OnEnChangeEditPuttyKey)
	ON_CBN_SELCHANGE(IDC_COMBO_TAGOPT, &CSettingGitRemote::OnCbnSelchangeComboTagOpt)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE, &CSettingGitRemote::OnBnClickedButtonRemove)
	ON_BN_CLICKED(IDC_BUTTON_RENAME_REMOTE, &CSettingGitRemote::OnBnClickedButtonRenameRemote)
END_MESSAGE_MAP()

BOOL CSettingGitRemote::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

	//CString str=((CSettings*)GetParent())->m_CmdPath.GetWinPath();
	CString proj;
	if(	g_GitAdminDir.HasAdminDir(m_cmdPath,&proj) )
	{
		CString title;
		this->GetWindowText(title);
		this->SetWindowText(title + _T(" - ") + proj);
	}

	STRING_VECTOR remotes;
	g_Git.GetRemoteList(remotes);
	m_ctrlRemoteList.ResetContent();
	for (size_t i = 0; i < remotes.size(); i++)
		m_ctrlRemoteList.AddString(remotes[i]);

	m_ctrlTagOpt.AddString(CString(MAKEINTRESOURCE(IDS_FETCH_REACHABLE)));
	m_ctrlTagOpt.AddString(CString(MAKEINTRESOURCE(IDS_NONE)));
	m_ctrlTagOpt.AddString(CString(MAKEINTRESOURCE(IDS_FETCH_TAGS_ONLY)));
	m_ctrlTagOpt.SetCurSel(0);

	//this->GetDlgItem(IDC_EDIT_REMOTE)->EnableWindow(FALSE);
	this->UpdateData(FALSE);

	if (remotes.empty())
	{
		GetDlgItem(IDC_EDIT_REMOTE)->SetWindowText(_T("origin"));
		OnEnChangeEditRemote();
	}

	return TRUE;
}
// CSettingGitRemote message handlers

void CSettingGitRemote::OnBnClickedButtonBrowse()
{
	CFileDialog dlg(TRUE,NULL,
					NULL,
					OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
					CString(MAKEINTRESOURCE(IDS_PUTTYKEYFILEFILTER)));

	this->UpdateData();
	INT_PTR ret = dlg.DoModal();
	SetCurrentDirectory(g_Git.m_CurrentDir);
	if (ret == IDOK)
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
		CMessageBox::Show(NULL, IDS_PROC_GITCONFIG_REMOTEEMPTY, IDS_APPNAME, MB_OK |  MB_ICONERROR);
		return;
	}
	if(m_strUrl.IsEmpty())
	{
		CMessageBox::Show(NULL, IDS_PROC_GITCONFIG_URLEMPTY, IDS_APPNAME, MB_OK | MB_ICONERROR);
		return;
	}

	m_ChangedMask = REMOTE_NAME | REMOTE_URL | REMOTE_PUTTYKEY | REMOTE_TAGOPT;
	if(IsRemoteExist(m_strRemote))
	{
		CString msg;
		msg.Format(IDS_PROC_GITCONFIG_OVERWRITEREMOTE, m_strRemote);
		if(CMessageBox::Show(NULL, msg, _T("TortoiseGit"), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES)
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
		if(CMessageBox::Show(NULL, IDS_PROC_GITCONFIG_SAVEREMOTE, IDS_APPNAME, 1, IDI_QUESTION, IDS_SAVEBUTTON, IDS_DISCARDBUTTON) == 1)
			OnApply();
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
	m_strUrl = g_Git.GetConfigValue(cmd, CP_UTF8);

	cmd.Format(_T("remote.%s.puttykeyfile"),remote);

	this->m_strPuttyKeyfile = g_Git.GetConfigValue(cmd, CP_UTF8);

	m_ChangedMask=0;


	cmd.Format(_T("remote.%s.tagopt"), remote);
	CString tagopt = g_Git.GetConfigValue(cmd, CP_UTF8);
	index = 0;
	if (tagopt == "--no-tags")
		index = 1;
	else if (tagopt == "--tags")
		index = 2;
	m_ctrlTagOpt.SetCurSel(index);

	GetDlgItem(IDC_BUTTON_ADD)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON_REMOVE)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON_RENAME_REMOTE)->EnableWindow(TRUE);
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

void CSettingGitRemote::OnCbnSelchangeComboTagOpt()
{
	m_ChangedMask |= REMOTE_TAGOPT;

	this->UpdateData();
	if (this->m_ctrlTagOpt.GetCurSel() > 0)
		this->SetModified();
	else
		this->SetModified(0);
}

void CSettingGitRemote::Save(CString key,CString value)
{
	CString cmd,out;

	cmd.Format(_T("remote.%s.%s"),this->m_strRemote,key);
	if (value.IsEmpty())
	{
		// don't check result code. it fails if the entry not exist
		g_Git.UnsetConfigValue(cmd, CONFIG_LOCAL);
		if (!g_Git.GetConfigValue(cmd).IsEmpty())
		{
			CString msg;
			msg.Format(IDS_PROC_SAVECONFIGFAILED, cmd, value);
			CMessageBox::Show(NULL, msg, _T("TortoiseGit"), MB_OK | MB_ICONERROR);
		}
		return;
	}

	if (g_Git.SetConfigValue(cmd, value, CONFIG_LOCAL))
	{
		CString msg;
		msg.Format(IDS_PROC_SAVECONFIGFAILED, cmd, value);
		CMessageBox::Show(NULL, msg, _T("TortoiseGit"), MB_OK | MB_ICONERROR);
	}
}
BOOL CSettingGitRemote::OnApply()
{
	CWaitCursor wait;
	this->UpdateData();
	if(m_ChangedMask & REMOTE_NAME)
	{
		//Add Remote
		if(m_strRemote.IsEmpty())
		{
			CMessageBox::Show(NULL, IDS_PROC_GITCONFIG_REMOTEEMPTY, IDS_APPNAME, MB_OK |  MB_ICONERROR);
			return FALSE;
		}
		if(m_strUrl.IsEmpty())
		{
			CMessageBox::Show(NULL, IDS_PROC_GITCONFIG_URLEMPTY, IDS_APPNAME, MB_OK | MB_ICONERROR);
			return FALSE;
		}

		if (m_ctrlRemoteList.GetCount() > 0)
		{
			// tagopt not --no-tags
			if (m_ctrlTagOpt.GetCurSel() != 1)
			{
				if (CMessageBox::ShowCheck(GetSafeHwnd(), IDS_PROC_GITCONFIG_ASKTAGOPT, IDS_APPNAME, MB_YESNO | MB_ICONQUESTION, _T("TagOptNoTagsWarning"), IDS_MSGBOX_DONOTSHOWAGAIN) == IDYES)
					m_ctrlTagOpt.SetCurSel(1);
			}
		}

		m_strUrl.Replace(L'\\', L'/');
		CString cmd,out;
		cmd.Format(_T("git.exe remote add \"%s\" \"%s\""),m_strRemote,m_strUrl);
		if (g_Git.Run(cmd, &out, CP_UTF8))
		{
			CMessageBox::Show(NULL,out,_T("TorotiseGit"),MB_OK|MB_ICONERROR);
			return FALSE;
		}
		m_ChangedMask &= ~REMOTE_URL;

		m_ctrlRemoteList.SetCurSel(m_ctrlRemoteList.AddString(m_strRemote));
		GetDlgItem(IDC_BUTTON_ADD)->EnableWindow(TRUE);
		GetDlgItem(IDC_BUTTON_RENAME_REMOTE)->EnableWindow(TRUE);
		if (!m_bNoFetch && CMessageBox::Show(NULL, IDS_SETTINGS_FETCH_ADDEDREMOTE, IDS_APPNAME, MB_ICONQUESTION | MB_YESNO) == IDYES)
			CCommonAppUtils::RunTortoiseGitProc(_T("/command:fetch /path:\"") + g_Git.m_CurrentDir + _T("\" /remote:\"") + m_strRemote + _T("\""));
	}
	if(m_ChangedMask & REMOTE_URL)
	{
		m_strUrl.Replace(L'\\', L'/');
		Save(_T("url"),this->m_strUrl);
	}

	if(m_ChangedMask & REMOTE_PUTTYKEY)
	{
		Save(_T("puttykeyfile"),this->m_strPuttyKeyfile);
	}

	if (m_ChangedMask & REMOTE_TAGOPT)
	{
		CString tagopt;
		int index = m_ctrlTagOpt.GetCurSel();
		if (index == 1)
			tagopt = "--no-tags";
		else if (index == 2)
			tagopt = "--tags";
		Save(_T("tagopt"), tagopt);
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
		CString msg;
		msg.Format(IDS_PROC_GITCONFIG_DELETEREMOTE, str);
		if(CMessageBox::Show(NULL, msg, _T("TortoiseGit"), MB_YESNO | MB_ICONQUESTION) == IDYES)
		{
			CString cmd,out;
			cmd.Format(_T("git.exe remote rm %s"),str);
			if (g_Git.Run(cmd, &out, CP_UTF8))
			{
				CMessageBox::Show(NULL, out,_T("TortoiseGit"),MB_OK|MB_ICONERROR);
				return;
			}

			m_ctrlRemoteList.DeleteString(index);
			OnLbnSelchangeListRemote();
		}
	}
}

void CSettingGitRemote::OnBnClickedButtonRenameRemote()
{
	int sel = m_ctrlRemoteList.GetCurSel();
	if (sel >= 0)
	{
		CString oldRemote, newRemote;
		m_ctrlRemoteList.GetText(sel, oldRemote);
		GetDlgItem(IDC_EDIT_REMOTE)->GetWindowText(newRemote);
		CString cmd, out;
		cmd.Format(_T("git.exe remote rename %s %s"), oldRemote, newRemote);
		if (g_Git.Run(cmd, &out, CP_UTF8))
		{
			CMessageBox::Show(NULL, out,_T("TortoiseGit"), MB_OK | MB_ICONERROR);
			return;
		}

		m_ctrlRemoteList.DeleteString(sel);
		m_ctrlRemoteList.SetCurSel(m_ctrlRemoteList.AddString(newRemote));
		m_ChangedMask &= ~REMOTE_NAME;
		if (!m_ChangedMask)
			this->SetModified(FALSE);
	}
}
