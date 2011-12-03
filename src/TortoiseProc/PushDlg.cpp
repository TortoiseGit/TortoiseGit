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

// PushDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "PushDlg.h"

#include "Git.h"
#include "registry.h"
#include "AppUtils.h"
#include "BrowseRefsDlg.h"
#include "MessageBox.h"

// CPushDlg dialog

IMPLEMENT_DYNAMIC(CPushDlg, CHorizontalResizableStandAloneDialog)

CPushDlg::CPushDlg(CWnd* pParent /*=NULL*/)
	: CHorizontalResizableStandAloneDialog(CPushDlg::IDD, pParent)
	, m_bPushAllBranches(FALSE)
	, m_bForce(FALSE)
	, m_bPack(FALSE)
	, m_bTags(FALSE)
	, m_bAutoLoad(FALSE)
{
	m_bAutoLoad = CAppUtils::IsSSHPutty();
}

CPushDlg::~CPushDlg()
{
}

void CPushDlg::DoDataExchange(CDataExchange* pDX)
{
	CHorizontalResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BRANCH_REMOTE, this->m_BranchRemote);
	DDX_Control(pDX, IDC_BRANCH_SOURCE, this->m_BranchSource);
	DDX_Control(pDX, IDC_REMOTE, this->m_Remote);
	DDX_Control(pDX, IDC_URL, this->m_RemoteURL);
	DDX_Check(pDX,IDC_FORCE,this->m_bForce);
	DDX_Check(pDX,IDC_PUSHALL,this->m_bPushAllBranches);
	DDX_Check(pDX,IDC_PACK,this->m_bPack);
	DDX_Check(pDX,IDC_TAGS,this->m_bTags);
	DDX_Check(pDX,IDC_PUTTYKEY_AUTOLOAD,this->m_bAutoLoad);
}

BEGIN_MESSAGE_MAP(CPushDlg, CHorizontalResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_RD_REMOTE, &CPushDlg::OnBnClickedRd)
	ON_BN_CLICKED(IDC_RD_URL, &CPushDlg::OnBnClickedRd)
	ON_CBN_SELCHANGE(IDC_BRANCH_SOURCE, &CPushDlg::OnCbnSelchangeBranchSource)
	ON_BN_CLICKED(IDOK, &CPushDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_REMOTE_MANAGE, &CPushDlg::OnBnClickedRemoteManage)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE_SOURCE_BRANCH, &CPushDlg::OnBnClickedButtonBrowseSourceBranch)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE_DEST_BRANCH, &CPushDlg::OnBnClickedButtonBrowseDestBranch)
	ON_BN_CLICKED(IDC_PUSHALL, &CPushDlg::OnBnClickedPushall)
END_MESSAGE_MAP()

BOOL CPushDlg::OnInitDialog()
{
	CHorizontalResizableStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDCANCEL,BOTTOM_RIGHT);
	AddAnchor(IDC_BRANCH_GROUP, TOP_LEFT,TOP_RIGHT);
	AddAnchor(IDC_STATIC_REMOTE, TOP_LEFT);
	AddAnchor(IDC_STATIC_SOURCE, TOP_LEFT);

	AddAnchor(IDC_PUSHALL, TOP_LEFT);
	AddAnchor(IDC_BRANCH_REMOTE, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_BUTTON_BROWSE_DEST_BRANCH, TOP_RIGHT);
	AddAnchor(IDC_BRANCH_SOURCE, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_BUTTON_BROWSE_SOURCE_BRANCH, TOP_RIGHT);

	AddAnchor(IDC_URL_GROUP, TOP_LEFT,TOP_RIGHT);
	AddAnchor(IDC_RD_REMOTE, TOP_LEFT);
	AddAnchor(IDC_RD_URL, TOP_LEFT);

	AddAnchor(IDC_REMOTE, TOP_LEFT, TOP_RIGHT);

	AddAnchor(IDC_URL, TOP_LEFT,TOP_RIGHT);

	AddAnchor(IDC_OPTION_GROUP, TOP_LEFT,TOP_RIGHT);

	AddAnchor(IDC_FORCE, TOP_LEFT);
	AddAnchor(IDC_PACK, TOP_LEFT);
	AddAnchor(IDC_TAGS, TOP_LEFT);
	AddAnchor(IDC_PUTTYKEY_AUTOLOAD,TOP_LEFT);

	AddAnchor(IDC_REMOTE_MANAGE,TOP_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);

	AddOthersToAnchor();

	CString sWindowTitle;
	GetWindowText(sWindowTitle);
	CAppUtils::SetWindowTitle(m_hWnd, g_Git.m_CurrentDir, sWindowTitle);

	this->GetDlgItem(IDC_PUTTYKEY_AUTOLOAD)->EnableWindow(CAppUtils::IsSSHPutty());

	EnableSaveRestore(_T("PushDlg"));

	m_RemoteURL.SetURLHistory(TRUE);

	CString WorkingDir=g_Git.m_CurrentDir;
	WorkingDir.Replace(_T(':'),_T('_'));
	m_RemoteURL.LoadHistory(CString(_T("Software\\TortoiseGit\\History\\PushURLS\\"))+WorkingDir, _T("url"));
	CString clippath=CAppUtils::GetClipboardLink();
	if(clippath.IsEmpty())
		m_RemoteURL.SetCurSel(0);
	else
		m_RemoteURL.SetWindowText(clippath);

	m_RemoteURL.EnableWindow(FALSE);
	CheckRadioButton(IDC_RD_REMOTE,IDC_RD_URL,IDC_RD_REMOTE);


	this->m_regAutoLoad = CRegDWORD(CString(_T("Software\\TortoiseGit\\History\\PushDlgAutoLoad\\"))+WorkingDir,
									m_bAutoLoad);
	m_bAutoLoad = this->m_regAutoLoad;
	if(!CAppUtils::IsSSHPutty())
		m_bAutoLoad = false;

	Refresh();


	//m_BranchRemote.SetWindowTextW(m_BranchSource.GetString());

	this->UpdateData(false);
	return TRUE;
}

void CPushDlg::Refresh()
{
	CString WorkingDir=g_Git.m_CurrentDir;
	WorkingDir.Replace(_T(':'),_T('_'));

	CRegString remote(CString(_T("Software\\TortoiseGit\\History\\PushRemote\\")+WorkingDir));
	m_RemoteReg = remote;
	int sel=0;

	STRING_VECTOR list;
	m_Remote.Reset();

	if(!g_Git.GetRemoteList(list))
	{
		for(unsigned int i=0;i<list.size();i++)
		{
			m_Remote.AddString(list[i]);
			if(list[i] == remote)
				sel = i;
		}
	}
	m_Remote.SetCurSel(sel);

	int current=0;
	list.clear();
	m_BranchSource.Reset();
	m_BranchSource.AddString(_T(" ")); // empty string does not work, for removal of remote branches/tags
	m_BranchSource.SetMaxHistoryItems(0x7FFFFFFF);
	if(!g_Git.GetBranchList(list,&current))
	{
		for(unsigned int i=0;i<list.size();i++)
			m_BranchSource.AddString(list[i]);
		current++; // shift for " "
	}
	if (wcsncmp(m_BranchSourceName, _T("refs/"), 5) == 0)
		m_BranchSourceName = m_BranchSourceName.Mid(5);
	if (wcsncmp(m_BranchSourceName, _T("heads/"), 6) == 0)
	{
		m_BranchSourceName = m_BranchSourceName.Mid(6);
		m_BranchSource.SetCurSel(m_BranchSource.FindStringExact(-1, m_BranchSourceName));
	}
	else if (wcsncmp(m_BranchSourceName, _T("remotes/"), 8) == 0)
		m_BranchSource.SetCurSel(m_BranchSource.FindStringExact(-1, m_BranchSourceName));
	else
		m_BranchSource.SetCurSel(current);

	GetRemoteBranch(m_BranchSource.GetString());

	this->GetDlgItem(IDOK)->EnableWindow(m_BranchSource.GetCount() != 0);
}

void CPushDlg::GetRemoteBranch(CString currentBranch)
{
	CString WorkingDir=g_Git.m_CurrentDir;
	WorkingDir.Replace(_T(':'), _T('_'));

	if (currentBranch.IsEmpty())
		return;

	CString configName;

	configName.Format(L"branch.%s.pushremote", currentBranch);
	CString pushRemote = g_Git.GetConfigValue(configName);
	if( pushRemote.IsEmpty() )
	{
		configName.Format(L"branch.%s.remote", currentBranch);
		pushRemote = g_Git.GetConfigValue(configName);
	}

	CRegString remote(CString(_T("Software\\TortoiseGit\\History\\PushRemote\\")+WorkingDir));

	if( !pushRemote.IsEmpty() )
		remote=pushRemote;

	//Select pull-branch from current branch
	configName.Format(L"branch.%s.pushbranch", currentBranch);
	CString pushBranch = CGit::StripRefName(g_Git.GetConfigValue(configName));
	if( pushBranch.IsEmpty() )
	{
		configName.Format(L"branch.%s.merge", currentBranch);
		pushBranch = CGit::StripRefName(g_Git.GetConfigValue(configName));
	}

	m_BranchRemote.LoadHistory(CString(_T("Software\\TortoiseGit\\History\\RemoteBranch\\"))+WorkingDir, _T("branch"));
	if( !pushBranch.IsEmpty() )
		m_BranchRemote.AddString(pushBranch);
}

// CPushDlg message handlers

void CPushDlg::OnBnClickedRd()
{
	if( GetCheckedRadioButton(IDC_RD_REMOTE,IDC_RD_URL) == IDC_RD_REMOTE)
	{
		m_Remote.EnableWindow(TRUE);
		GetDlgItem(IDC_REMOTE_MANAGE)->EnableWindow(TRUE);
		m_RemoteURL.EnableWindow(FALSE);
	}
	if( GetCheckedRadioButton(IDC_RD_REMOTE,IDC_RD_URL) == IDC_RD_URL)
	{
		m_Remote.EnableWindow(FALSE);
		GetDlgItem(IDC_REMOTE_MANAGE)->EnableWindow(FALSE);
		m_RemoteURL.EnableWindow(TRUE);
	}
}


void CPushDlg::OnCbnSelchangeBranchSource()
{
	GetRemoteBranch(m_BranchSource.GetString());
}

void CPushDlg::OnBnClickedOk()
{
	CHorizontalResizableStandAloneDialog::UpdateData(TRUE);

	if( GetCheckedRadioButton(IDC_RD_REMOTE,IDC_RD_URL) == IDC_RD_REMOTE)
	{
		m_URL=m_Remote.GetString();
	}
	if( GetCheckedRadioButton(IDC_RD_REMOTE,IDC_RD_URL) == IDC_RD_URL)
	{
		m_URL=m_RemoteURL.GetString();
	}

	if (!m_bPushAllBranches)
	{
		this->m_BranchRemoteName=m_BranchRemote.GetString().Trim();
		this->m_BranchSourceName=m_BranchSource.GetString().Trim();

		if (m_BranchSourceName.IsEmpty() && m_BranchRemoteName.IsEmpty())
		{
			if (CMessageBox::Show(NULL, IDS_B_T_BOTHEMPTY, IDS_TORTOISEGIT, MB_ICONQUESTION | MB_YESNO) == IDNO)
				return;
		}
		if (m_BranchSourceName.IsEmpty() && !m_BranchRemoteName.IsEmpty())
		{
			if (CMessageBox::Show(NULL, IDS_B_T_LOCALEMPTY, IDS_TORTOISEGIT, MB_ICONEXCLAMATION | MB_YESNO) == IDNO)
				return;
		}
		else if (!m_BranchRemoteName.IsEmpty() && !g_Git.IsBranchNameValid(this->m_BranchRemoteName))
		{
			CMessageBox::Show(NULL, IDS_B_T_INVALID, IDS_TORTOISEGIT, MB_OK);
			return;
		}
		else
		{
			// do not store branch names on removal
			this->m_RemoteURL.SaveHistory();
			this->m_BranchRemote.SaveHistory();
			m_RemoteReg = m_Remote.GetString();
		}
	}

	this->m_regAutoLoad = m_bAutoLoad ;

	CHorizontalResizableStandAloneDialog::OnOK();
}

void CPushDlg::OnBnClickedRemoteManage()
{
	CAppUtils::LaunchRemoteSetting();
	Refresh();
}

void CPushDlg::OnBnClickedButtonBrowseSourceBranch()
{
	if(CBrowseRefsDlg::PickRefForCombo(&m_BranchSource, gPickRef_Head))
		OnCbnSelchangeBranchSource();
}

void CPushDlg::OnBnClickedButtonBrowseDestBranch()
{
	CString remoteBranchName;
	CString remoteName;
	m_BranchRemote.GetWindowText(remoteBranchName);
	remoteName = m_Remote.GetString();
	remoteBranchName = remoteName + '/' + remoteBranchName;
	remoteBranchName = CBrowseRefsDlg::PickRef(false, remoteBranchName, gPickRef_Remote);
	if(remoteBranchName.IsEmpty())
		return; //Canceled
	remoteBranchName = remoteBranchName.Mid(13);//Strip 'refs/remotes/'
	int slashPlace = remoteBranchName.Find('/');
	remoteName = remoteBranchName.Left(slashPlace);
	remoteBranchName = remoteBranchName.Mid(slashPlace + 1); //Strip remote name (for example 'origin/')

	//Select remote
	int remoteSel = m_Remote.FindStringExact(0,remoteName);
	if(remoteSel >= 0)
		m_Remote.SetCurSel(remoteSel);

	//Select branch
	m_BranchRemote.AddString(remoteBranchName);
}

BOOL CPushDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case VK_F5:
			{
				Refresh();
			}
			break;
		}
	}

	return CHorizontalResizableStandAloneDialog::PreTranslateMessage(pMsg);
}

void CPushDlg::OnBnClickedPushall()
{
	this->UpdateData();
	this->GetDlgItem(IDC_BRANCH_REMOTE)->EnableWindow(!m_bPushAllBranches);
	this->GetDlgItem(IDC_BRANCH_SOURCE)->EnableWindow(!m_bPushAllBranches);
	this->GetDlgItem(IDC_BUTTON_BROWSE_SOURCE_BRANCH)->EnableWindow(!m_bPushAllBranches);
	this->GetDlgItem(IDC_BUTTON_BROWSE_DEST_BRANCH)->EnableWindow(!m_bPushAllBranches);
	this->GetDlgItem(IDC_TAGS)->EnableWindow(!m_bPushAllBranches);
	if (m_bTags && m_bPushAllBranches)
	{
		m_bTags = FALSE;
		UpdateData(FALSE);
	}
}
