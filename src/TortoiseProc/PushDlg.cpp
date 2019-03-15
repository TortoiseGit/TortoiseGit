// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2019 - TortoiseGit

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
#include "StringUtils.h"
#include "Git.h"
#include "registry.h"
#include "AppUtils.h"
#include "LogDlg.h"
#include "BrowseRefsDlg.h"
#include "RefLogDlg.h"
#include "MessageBox.h"

// CPushDlg dialog

IMPLEMENT_DYNAMIC(CPushDlg, CHorizontalResizableStandAloneDialog)

CPushDlg::CPushDlg(CWnd* pParent /*=nullptr*/)
	: CHorizontalResizableStandAloneDialog(CPushDlg::IDD, pParent)
	, m_bPushAllBranches(FALSE)
	, m_bForce(FALSE)
	, m_bForceWithLease(FALSE)
	, m_bTags(FALSE)
	, m_bPushAllRemotes(FALSE)
	, m_bSetPushBranch(FALSE)
	, m_bSetPushRemote(FALSE)
	, m_bSetUpstream(FALSE)
	, m_RecurseSubmodules(0)
	, m_bAutoLoad(CAppUtils::IsSSHPutty())
{
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
	DDX_Control(pDX, IDC_BUTTON_BROWSE_SOURCE_BRANCH, m_BrowseLocalRef);
	DDX_Control(pDX, IDC_COMBOBOX_RECURSE_SUBMODULES, m_RecurseSubmodulesCombo);
	DDX_Check(pDX,IDC_FORCE,this->m_bForce);
	DDX_Check(pDX, IDC_FORCE_WITH_LEASE, m_bForceWithLease);
	DDX_Check(pDX,IDC_PUSHALL,this->m_bPushAllBranches);
	DDX_Check(pDX,IDC_TAGS,this->m_bTags);
	DDX_Check(pDX,IDC_PUTTYKEY_AUTOLOAD,this->m_bAutoLoad);
	DDX_Check(pDX, IDC_PROC_PUSH_SET_PUSHREMOTE, m_bSetPushRemote);
	DDX_Check(pDX, IDC_PROC_PUSH_SET_PUSHBRANCH, m_bSetPushBranch);
	DDX_Check(pDX, IDC_PROC_PUSH_SET_UPSTREAM, m_bSetUpstream);
}

BEGIN_MESSAGE_MAP(CPushDlg, CHorizontalResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_RD_REMOTE, &CPushDlg::OnBnClickedRd)
	ON_BN_CLICKED(IDC_RD_URL, &CPushDlg::OnBnClickedRd)
	ON_CBN_SELCHANGE(IDC_BRANCH_SOURCE, &CPushDlg::OnCbnSelchangeBranchSource)
	ON_CBN_SELCHANGE(IDC_REMOTE, &CPushDlg::EnDisablePushRemoteArchiveBranch)
	ON_BN_CLICKED(IDOK, &CPushDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_REMOTE_MANAGE, &CPushDlg::OnBnClickedRemoteManage)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE_SOURCE_BRANCH, &CPushDlg::OnBnClickedButtonBrowseSourceBranch)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE_DEST_BRANCH, &CPushDlg::OnBnClickedButtonBrowseDestBranch)
	ON_BN_CLICKED(IDC_PUSHALL, &CPushDlg::OnBnClickedPushall)
	ON_BN_CLICKED(IDC_FORCE, &CPushDlg::OnBnClickedForce)
	ON_BN_CLICKED(IDC_FORCE_WITH_LEASE, &CPushDlg::OnBnClickedForceWithLease)
	ON_BN_CLICKED(IDC_TAGS, &CPushDlg::OnBnClickedTags)
	ON_BN_CLICKED(IDC_PROC_PUSH_SET_UPSTREAM, &CPushDlg::OnBnClickedProcPushSetUpstream)
	ON_BN_CLICKED(IDC_PROC_PUSH_SET_PUSHREMOTE, &CPushDlg::OnBnClickedProcPushSetPushremote)
	ON_BN_CLICKED(IDC_PROC_PUSH_SET_PUSHBRANCH, &CPushDlg::OnBnClickedProcPushSetPushremote)
END_MESSAGE_MAP()

BOOL CPushDlg::OnInitDialog()
{
	CHorizontalResizableStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	AdjustControlSize(IDC_RD_REMOTE);
	AdjustControlSize(IDC_RD_URL);
	AdjustControlSize(IDC_PUSHALL);
	AdjustControlSize(IDC_FORCE);
	AdjustControlSize(IDC_FORCE_WITH_LEASE);
	AdjustControlSize(IDC_TAGS);
	AdjustControlSize(IDC_PUTTYKEY_AUTOLOAD);
	AdjustControlSize(IDC_PROC_PUSH_SET_PUSHBRANCH);
	AdjustControlSize(IDC_PROC_PUSH_SET_PUSHREMOTE);
	AdjustControlSize(IDC_PROC_PUSH_SET_UPSTREAM);
	AdjustControlSize(IDC_STATIC_RECURSE_SUBMODULES);

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
	AddAnchor(IDC_FORCE_WITH_LEASE, TOP_LEFT);
	AddAnchor(IDC_TAGS, TOP_LEFT);
	AddAnchor(IDC_PUTTYKEY_AUTOLOAD,TOP_LEFT);
	AddAnchor(IDC_PROC_PUSH_SET_PUSHBRANCH, TOP_LEFT);
	AddAnchor(IDC_PROC_PUSH_SET_PUSHREMOTE, TOP_LEFT);
	AddAnchor(IDC_PROC_PUSH_SET_UPSTREAM, TOP_LEFT);
	AddAnchor(IDC_STATIC_RECURSE_SUBMODULES, TOP_LEFT);
	AddAnchor(IDC_COMBOBOX_RECURSE_SUBMODULES, TOP_LEFT);

	AddAnchor(IDC_REMOTE_MANAGE,TOP_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);

	AddOthersToAnchor();

	CString sWindowTitle;
	GetWindowText(sWindowTitle);
	CAppUtils::SetWindowTitle(m_hWnd, g_Git.m_CurrentDir, sWindowTitle);

	this->GetDlgItem(IDC_PUTTYKEY_AUTOLOAD)->EnableWindow(CAppUtils::IsSSHPutty());

	EnableSaveRestore(L"PushDlg");

	m_Remote.SetMaxHistoryItems(0x7FFFFFFF);
	m_RemoteURL.SetCaseSensitive(TRUE);
	m_RemoteURL.SetURLHistory(TRUE);

	CString WorkingDir=g_Git.m_CurrentDir;
	WorkingDir.Replace(L':', L'_');
	m_regPushAllRemotes = CRegDWORD(L"Software\\TortoiseGit\\TortoiseProc\\Push\\" + WorkingDir + L"\\AllRemotes", FALSE);
	m_bPushAllRemotes = m_regPushAllRemotes;
	m_regPushAllBranches = CRegDWORD(L"Software\\TortoiseGit\\TortoiseProc\\Push\\" + WorkingDir + L"\\AllBranches", FALSE);
	m_bPushAllBranches = m_regPushAllBranches;
	m_RemoteURL.LoadHistory(L"Software\\TortoiseGit\\History\\PushURLS\\" + WorkingDir, L"url");
	m_RemoteURL.EnableWindow(FALSE);
	CheckRadioButton(IDC_RD_REMOTE,IDC_RD_URL,IDC_RD_REMOTE);

	this->m_regAutoLoad = CRegDWORD(L"Software\\TortoiseGit\\History\\PushDlgAutoLoad\\" + WorkingDir,
									m_bAutoLoad);
	m_bAutoLoad = this->m_regAutoLoad;
	if(!CAppUtils::IsSSHPutty())
		m_bAutoLoad = false;

	m_BrowseLocalRef.m_bRightArrow = TRUE;
	m_BrowseLocalRef.m_bDefaultClick = FALSE;
	m_BrowseLocalRef.m_bMarkDefault = FALSE;
	m_BrowseLocalRef.m_bShowCurrentItem = FALSE;
	m_BrowseLocalRef.AddEntry(CString(MAKEINTRESOURCE(IDS_REFBROWSE)));
	m_BrowseLocalRef.AddEntry(CString(MAKEINTRESOURCE(IDS_LOG)));
	m_BrowseLocalRef.AddEntry(CString(MAKEINTRESOURCE(IDS_REFLOG)));

	m_tooltips.AddTool(IDC_PROC_PUSH_SET_PUSHBRANCH, IDS_PUSHDLG_PUSHBRANCH_TT);
	m_tooltips.AddTool(IDC_PROC_PUSH_SET_PUSHREMOTE, IDS_PUSHDLG_PUSHREMOTE_TT);
	m_tooltips.AddTool(IDC_FORCE, IDS_FORCE_TT);
	m_tooltips.AddTool(IDC_FORCE_WITH_LEASE, IDS_FORCE_WITH_LEASE_TT);

	CString recurseSubmodules = g_Git.GetConfigValue(L"push.recurseSubmodules");
	if (recurseSubmodules == L"check")
		m_RecurseSubmodules = 1;
	else if (recurseSubmodules == L"on-demand")
		m_RecurseSubmodules = 2;
	else
		m_RecurseSubmodules = 0;
	m_regRecurseSubmodules = CRegDWORD(L"Software\\TortoiseGit\\History\\PushRecurseSubmodules\\" + WorkingDir, m_RecurseSubmodules);
	m_RecurseSubmodules = m_regRecurseSubmodules;
	m_RecurseSubmodulesCombo.AddString(CString(MAKEINTRESOURCE(IDS_NONE)));
	m_RecurseSubmodulesCombo.AddString(CString(MAKEINTRESOURCE(IDS_RECURSE_SUBMODULES_CHECK)));
	m_RecurseSubmodulesCombo.AddString(CString(MAKEINTRESOURCE(IDS_RECURSE_SUBMODULES_ONDEMAND)));
	m_RecurseSubmodulesCombo.SetCurSel(m_RecurseSubmodules);

	Refresh();

	this->UpdateData(false);

	OnBnClickedPushall();
	return TRUE;
}

void CPushDlg::Refresh()
{
	CString WorkingDir=g_Git.m_CurrentDir;
	WorkingDir.Replace(L':', L'_');

	CRegString remote(L"Software\\TortoiseGit\\History\\PushRemote\\" + WorkingDir);
	m_RemoteReg = remote;
	int sel = -1;

	STRING_VECTOR list;
	m_Remote.Reset();

	if(!g_Git.GetRemoteList(list))
	{
		if (list.size() > 1)
			m_Remote.AddString(CString(MAKEINTRESOURCE(IDS_PROC_PUSHFETCH_ALLREMOTES)));
		for (unsigned int i = 0; i < list.size(); ++i)
		{
			m_Remote.AddString(list[i]);
			if(list[i] == remote)
				sel = i + (list.size() > 1 ? 1 : 0);
		}
	}
	else
		MessageBox(g_Git.GetGitLastErr(L"Could not get list of remotes."), L"TortoiseGit", MB_ICONERROR);
	// if the last selected remote was "- All -" and "- All -" is still in the list -> select it
	if (list.size() > 1 && remote == CString(MAKEINTRESOURCE(IDS_PROC_PUSHFETCH_ALLREMOTES)))
		sel = 0;
	m_Remote.SetCurSel(sel);

	int current = -1;
	list.clear();
	m_BranchSource.Reset();
	m_BranchSource.SetMaxHistoryItems(0x7FFFFFFF);
	if(!g_Git.GetBranchList(list,&current))
		m_BranchSource.SetList(list);
	else
		MessageBox(g_Git.GetGitLastErr(L"Could not get list of local branches."), L"TortoiseGit", MB_ICONERROR);
	if (CStringUtils::StartsWith(m_BranchSourceName, L"refs/heads/"))
	{
		m_BranchSourceName = m_BranchSourceName.Mid(static_cast<int>(wcslen(L"refs/heads/")));
		m_BranchSource.SetCurSel(m_BranchSource.FindStringExact(-1, m_BranchSourceName));
	}
	else if (CStringUtils::StartsWith(m_BranchSourceName, L"refs/remotes/") || CStringUtils::StartsWith(m_BranchSourceName, L"remotes/"))
	{
		if (CStringUtils::StartsWith(m_BranchSourceName, L"refs/"))
			m_BranchSourceName = m_BranchSourceName.Mid(static_cast<int>(wcslen(L"refs/")));
		m_BranchSource.SetCurSel(m_BranchSource.FindStringExact(-1, m_BranchSourceName));
	}
	else if (m_BranchSourceName.IsEmpty() && current == -1 && !g_Git.IsInitRepos())
		m_BranchSource.SetWindowText(L"HEAD");
	else if (m_BranchSourceName.IsEmpty())
		m_BranchSource.SetCurSel(current);
	else
		m_BranchSource.SetWindowText(m_BranchSourceName);

	GetRemoteBranch(m_BranchSource.GetString());

	if (list.size() > 1 && m_bPushAllRemotes)
		m_Remote.SetCurSel(0);
	m_bPushAllRemotes = FALSE; // reset to FALSE, so that a refresh does not reselect all even if it was already deselected by user; correct value will be set in OnBnClickedOk method
}

void CPushDlg::GetRemoteBranch(CString currentBranch)
{
	CString WorkingDir=g_Git.m_CurrentDir;
	WorkingDir.Replace(L':', L'_');

	if (currentBranch.IsEmpty())
	{
		EnDisablePushRemoteArchiveBranch();
		return;
	}

	CString pushRemote;
	CString pushBranch;
	g_Git.GetRemotePushBranch(currentBranch, pushRemote, pushBranch);

	CRegString remote(L"Software\\TortoiseGit\\History\\PushRemote\\" + WorkingDir);

	if (!pushRemote.IsEmpty())
	{
		remote = pushRemote;
		// if a pushRemote exists, select it
		for (int i = 0; i < m_Remote.GetCount(); ++i)
		{
			CString str;
			int n = m_Remote.GetLBTextLen(i);
			m_Remote.GetLBText(i, CStrBuf(str, n));
			if (str == pushRemote)
			{
				m_Remote.SetCurSel(i);
				break;
			}
		}
	}
	// select the only remote if only one exists
	else if (m_Remote.GetCount() == 1)
		m_Remote.SetCurSel(0);
	// select no remote if no push-remote is specified AND push to all remotes is not selected
	else if (!(m_Remote.GetCount() > 1 && m_Remote.GetCurSel() == 0))
		m_Remote.SetCurSel(-1);

	m_BranchRemote.LoadHistory(L"Software\\TortoiseGit\\History\\RemoteBranch\\" + WorkingDir, L"branch");
	if( !pushBranch.IsEmpty() )
		m_BranchRemote.AddString(pushBranch);

	EnDisablePushRemoteArchiveBranch();
}

void CPushDlg::EnDisablePushRemoteArchiveBranch()
{
	if ((m_Remote.GetCount() > 1 && m_Remote.GetCurSel() == 0) || m_bPushAllBranches || GetCheckedRadioButton(IDC_RD_REMOTE,IDC_RD_URL) == IDC_RD_URL || m_BranchSource.GetString().Trim().IsEmpty())
	{
		DialogEnableWindow(IDC_PROC_PUSH_SET_PUSHBRANCH, FALSE);
		DialogEnableWindow(IDC_PROC_PUSH_SET_PUSHREMOTE, FALSE);
		bool enableUpstream = (GetCheckedRadioButton(IDC_RD_REMOTE, IDC_RD_URL) != IDC_RD_URL && (!m_BranchSource.GetString().Trim().IsEmpty() || m_bPushAllBranches));
		DialogEnableWindow(IDC_PROC_PUSH_SET_UPSTREAM, enableUpstream);
		if (m_bSetUpstream && !enableUpstream)
			m_bSetUpstream = FALSE;
		m_bSetPushRemote = FALSE;
		m_bSetPushBranch = FALSE;
		UpdateData(FALSE);
	}
	else
	{
		DialogEnableWindow(IDC_PROC_PUSH_SET_UPSTREAM, TRUE);
		DialogEnableWindow(IDC_PROC_PUSH_SET_PUSHBRANCH, !m_bSetUpstream);
		DialogEnableWindow(IDC_PROC_PUSH_SET_PUSHREMOTE, !m_bSetUpstream);
	}
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
		CString clippath = CAppUtils::GetClipboardLink();
		if (clippath.IsEmpty())
			m_RemoteURL.SetCurSel(0);
		else
			m_RemoteURL.SetWindowText(clippath);
		m_Remote.EnableWindow(FALSE);
		GetDlgItem(IDC_REMOTE_MANAGE)->EnableWindow(FALSE);
		m_RemoteURL.EnableWindow(TRUE);
	}
	EnDisablePushRemoteArchiveBranch();
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
		if (m_URL.IsEmpty())
		{
			CMessageBox::Show(GetSafeHwnd(), IDS_PROC_GITCONFIG_REMOTEEMPTY, IDS_APPNAME, MB_OK | MB_ICONEXCLAMATION);
			return;
		}
		m_bPushAllRemotes = (m_Remote.GetCurSel() == 0 && m_Remote.GetCount() > 1);
	}
	if( GetCheckedRadioButton(IDC_RD_REMOTE,IDC_RD_URL) == IDC_RD_URL)
		m_URL=m_RemoteURL.GetString();

	if (m_bPushAllBranches)
	{
		BOOL dontaskagainchecked = FALSE;
		if (CMessageBox::ShowCheck(GetSafeHwnd(), IDS_PROC_PUSH_ALLBRANCHES, IDS_APPNAME, MB_ICONQUESTION | MB_DEFBUTTON2 | MB_YESNO, L"PushAllBranches", IDS_MSGBOX_DONOTSHOWAGAIN, &dontaskagainchecked) == IDNO)
		{
			if (dontaskagainchecked)
				CMessageBox::SetRegistryValue(L"PushAllBranches", IDYES);
			return;
		}
	}
	else
	{
		this->m_BranchRemoteName=m_BranchRemote.GetString().Trim();
		this->m_BranchSourceName=m_BranchSource.GetString().Trim();

		if (m_BranchSourceName.IsEmpty() && m_BranchRemoteName.IsEmpty())
		{
			if (CMessageBox::Show(GetSafeHwnd(), IDS_B_T_BOTHEMPTY, IDS_APPNAME, MB_ICONQUESTION | MB_YESNO) != IDYES)
				return;
		}
		if (m_BranchSourceName.IsEmpty() && !m_BranchRemoteName.IsEmpty())
		{
			if (CMessageBox::Show(GetSafeHwnd(), IDS_B_T_LOCALEMPTY, IDS_APPNAME, MB_ICONEXCLAMATION | MB_YESNO) != IDYES)
				return;
		}
		else if (!m_BranchRemoteName.IsEmpty() && !g_Git.IsBranchNameValid(this->m_BranchRemoteName))
		{
			CMessageBox::Show(GetSafeHwnd(), IDS_B_T_INVALID, IDS_APPNAME, MB_OK | MB_ICONEXCLAMATION);
			return;
		}
		else if (!m_BranchSourceName.IsEmpty() && !g_Git.IsBranchTagNameUnique(m_BranchSourceName))
		{
			CMessageBox::Show(GetSafeHwnd(), IDS_B_T_NOT_UNIQUE, IDS_APPNAME, MB_OK | MB_ICONEXCLAMATION);
			return;
		}
		else
		{
			// do not store branch names on removal
			if (m_RemoteURL.IsWindowEnabled())
				m_RemoteURL.SaveHistory(); // do not store Remote URLs if not used
			this->m_BranchRemote.SaveHistory();
			m_RemoteReg = m_Remote.GetString();

			if (!m_BranchSourceName.IsEmpty() && g_Git.IsLocalBranch(m_BranchSourceName))
			{
				CString configName;
				if (m_bSetPushBranch)
				{
					configName.Format(L"branch.%s.pushbranch", static_cast<LPCTSTR>(m_BranchSourceName));
					if (!m_BranchRemoteName.IsEmpty())
						g_Git.SetConfigValue(configName, m_BranchRemoteName);
					else
						g_Git.UnsetConfigValue(configName);
				}
				if (m_bSetPushRemote)
				{
					configName.Format(L"branch.%s.pushremote", static_cast<LPCTSTR>(m_BranchSourceName));
					if (!m_URL.IsEmpty())
						g_Git.SetConfigValue(configName, m_URL);
					else
						g_Git.UnsetConfigValue(configName);
				}
			}
		}
	}

	m_regPushAllBranches = m_bPushAllBranches;
	m_regPushAllRemotes = m_bPushAllRemotes;
	this->m_regAutoLoad = m_bAutoLoad ;
	m_RecurseSubmodules = m_RecurseSubmodulesCombo.GetCurSel();
	m_regRecurseSubmodules = m_RecurseSubmodules;

	CHorizontalResizableStandAloneDialog::OnOK();
}

void CPushDlg::OnBnClickedRemoteManage()
{
	CAppUtils::LaunchRemoteSetting();
	Refresh();
}

void CPushDlg::OnBnClickedButtonBrowseSourceBranch()
{
	switch (m_BrowseLocalRef.GetCurrentEntry())
	{
	case 0: /* Browse Refence*/
		{
			if (CBrowseRefsDlg::PickRefForCombo(m_BranchSource, gPickRef_Head | gPickRef_Tag))
				OnCbnSelchangeBranchSource();
		}
		break;

	case 1: /* Log */
		{
			CLogDlg dlg;
			CString revision;
			m_BranchSource.GetWindowText(revision);
			dlg.SetParams(CTGitPath(), CTGitPath(), revision, revision, 0);
			dlg.SetSelect(true);
			dlg.ShowWorkingTreeChanges(false);
			if (dlg.DoModal() == IDOK && !dlg.GetSelectedHash().empty())
			{
				m_BranchSource.SetWindowText(dlg.GetSelectedHash().at(0).ToString());
				OnCbnSelchangeBranchSource();
			}
		}
		break;

	case 2: /*RefLog*/
		{
			CRefLogDlg dlg;
			if(dlg.DoModal() == IDOK)
			{
				m_BranchSource.SetWindowText(dlg.m_SelectedHash.ToString());
				OnCbnSelchangeBranchSource();
			}
		}
		break;
	}
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
	remoteBranchName = remoteBranchName.Mid(static_cast<int>(wcslen(L"refs/remotes/"))); //Strip 'refs/remotes/'
	int slashPlace = remoteBranchName.Find('/');
	remoteName = remoteBranchName.Left(slashPlace);
	remoteBranchName = remoteBranchName.Mid(slashPlace + 1); //Strip remote name (for example 'origin/')

	//Select remote
	int remoteSel = m_Remote.FindStringExact(0,remoteName);
	if(remoteSel >= 0)
		m_Remote.SetCurSel(remoteSel);

	//Select branch
	m_BranchRemote.AddString(remoteBranchName, 0);
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
	EnDisablePushRemoteArchiveBranch();
}

void CPushDlg::OnBnClickedForce()
{
	UpdateData();
	GetDlgItem(IDC_FORCE_WITH_LEASE)->EnableWindow(m_bTags || m_bForce ? FALSE : TRUE);
}

void CPushDlg::OnBnClickedForceWithLease()
{
	UpdateData();
	GetDlgItem(IDC_FORCE)->EnableWindow(m_bForceWithLease ? FALSE : TRUE);
	GetDlgItem(IDC_TAGS)->EnableWindow(m_bForceWithLease ? FALSE : TRUE);
}

void CPushDlg::OnBnClickedTags()
{
	UpdateData();
	GetDlgItem(IDC_FORCE_WITH_LEASE)->EnableWindow(m_bTags || m_bForce ? FALSE : TRUE);
}

void CPushDlg::OnBnClickedProcPushSetUpstream()
{
	UpdateData();
	this->GetDlgItem(IDC_PROC_PUSH_SET_PUSHREMOTE)->EnableWindow(!m_bSetUpstream);
	this->GetDlgItem(IDC_PROC_PUSH_SET_PUSHBRANCH)->EnableWindow(!m_bSetUpstream);
	m_bSetPushBranch = FALSE;
	m_bSetPushRemote = FALSE;
	UpdateData(FALSE);
}

void CPushDlg::OnBnClickedProcPushSetPushremote()
{
	UpdateData();
	this->GetDlgItem(IDC_PROC_PUSH_SET_UPSTREAM)->EnableWindow(!(m_bSetPushBranch || m_bSetPushRemote));
	m_bSetUpstream = FALSE;
	UpdateData(FALSE);
}
