// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2016 - TortoiseGit

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

// PullFetchDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "PullFetchDlg.h"
#include "Git.h"
#include "AppUtils.h"
#include "BrowseRefsDlg.h"
#include "MessageBox.h"
// CPullFetchDlg dialog

IMPLEMENT_DYNAMIC(CPullFetchDlg, CHorizontalResizableStandAloneDialog)

CPullFetchDlg::CPullFetchDlg(CWnd* pParent /*=NULL*/)
	: CHorizontalResizableStandAloneDialog(CPullFetchDlg::IDD, pParent)
{
	m_IsPull=TRUE;
	m_bRebaseActivatedInConfigForPull = false;
	m_bAutoLoad = CAppUtils::IsSSHPutty();
	m_bAutoLoadEnable=CAppUtils::IsSSHPutty();;
	m_regRebase = false;
	m_bNoFF = false;
	m_bRebase = FALSE;
	m_bRebasePreserveMerges = false;
	m_bSquash = false;
	m_bNoCommit = false;
	m_nDepth = 1;
	m_bDepth = FALSE;
	m_bFFonly = false;
	m_bFetchTags = 2;
	m_bAllRemotes = FALSE;
	m_bPrune = BST_INDETERMINATE;
}

CPullFetchDlg::~CPullFetchDlg()
{
}

void CPullFetchDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_REMOTE_COMBO, this->m_Remote);
	DDX_Control(pDX, IDC_OTHER, this->m_Other);
	DDX_Control(pDX, IDC_REMOTE_BRANCH, this->m_RemoteBranch);
	DDX_Control(pDX,IDC_REMOTE_MANAGE, this->m_RemoteManage);
	DDX_Check(pDX,IDC_CHECK_NOFF, this->m_bNoFF);
	DDX_Check(pDX,IDC_CHECK_SQUASH, this->m_bSquash);
	DDX_Check(pDX,IDC_CHECK_NOCOMMIT, this->m_bNoCommit);
	DDX_Check(pDX,IDC_PUTTYKEY_AUTOLOAD,m_bAutoLoad);
	DDX_Check(pDX,IDC_CHECK_REBASE,m_bRebase);
	DDX_Check(pDX,IDC_CHECK_PRUNE,m_bPrune);
	DDX_Check(pDX, IDC_CHECK_DEPTH, m_bDepth);
	DDX_Text(pDX, IDC_EDIT_DEPTH, m_nDepth);
	DDX_Check(pDX, IDC_CHECK_FFONLY, m_bFFonly);
	DDX_Check(pDX, IDC_CHECK_FETCHTAGS, m_bFetchTags);
}


BEGIN_MESSAGE_MAP(CPullFetchDlg, CHorizontalResizableStandAloneDialog)
	ON_CBN_SELCHANGE(IDC_REMOTE_COMBO, &CPullFetchDlg::OnCbnSelchangeRemote)
	ON_BN_CLICKED(IDC_REMOTE_RD, &CPullFetchDlg::OnBnClickedRd)
	ON_BN_CLICKED(IDC_OTHER_RD, &CPullFetchDlg::OnBnClickedRd)
	ON_BN_CLICKED(IDOK, &CPullFetchDlg::OnBnClickedOk)
	ON_STN_CLICKED(IDC_REMOTE_MANAGE, &CPullFetchDlg::OnStnClickedRemoteManage)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE_REF, &CPullFetchDlg::OnBnClickedButtonBrowseRef)
	ON_BN_CLICKED(IDC_CHECK_DEPTH, OnBnClickedCheckDepth)
	ON_BN_CLICKED(IDC_CHECK_FFONLY, OnBnClickedCheckFfonly)
	ON_BN_CLICKED(IDC_CHECK_NOFF, OnBnClickedCheckFfonly)
END_MESSAGE_MAP()

BOOL CPullFetchDlg::OnInitDialog()
{
	CHorizontalResizableStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	AddAnchor(IDC_REMOTE_COMBO, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_OTHER, TOP_LEFT,TOP_RIGHT);

	AddAnchor(IDC_REMOTE_BRANCH, TOP_LEFT,TOP_RIGHT);
	AddAnchor(IDC_BUTTON_BROWSE_REF,TOP_RIGHT);

	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDCANCEL,BOTTOM_RIGHT);
	AddAnchor(IDC_GROUPT_REMOTE,TOP_LEFT,TOP_RIGHT);
	AddAnchor(IDC_GROUP_OPTION,TOP_LEFT,TOP_RIGHT);
	AddAnchor(IDC_PUTTYKEY_AUTOLOAD,BOTTOM_LEFT);
	AddAnchor(IDC_CHECK_PRUNE,BOTTOM_LEFT);
	AddAnchor(IDC_CHECK_REBASE,BOTTOM_LEFT);
	AddAnchor(IDC_REMOTE_MANAGE,BOTTOM_LEFT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);

	AdjustControlSize(IDC_REMOTE_RD);
	AdjustControlSize(IDC_OTHER_RD);
	AdjustControlSize(IDC_CHECK_SQUASH);
	AdjustControlSize(IDC_CHECK_NOCOMMIT);
	AdjustControlSize(IDC_CHECK_DEPTH);
	AdjustControlSize(IDC_CHECK_NOFF);
	AdjustControlSize(IDC_CHECK_FFONLY);
	AdjustControlSize(IDC_CHECK_FETCHTAGS);
	AdjustControlSize(IDC_PUTTYKEY_AUTOLOAD);
	AdjustControlSize(IDC_CHECK_REBASE);
	AdjustControlSize(IDC_CHECK_PRUNE);

	CString WorkingDir=g_Git.m_CurrentDir;
	WorkingDir.Replace(_T(':'),_T('_'));

	m_RemoteReg = CRegString(CString(_T("Software\\TortoiseGit\\History\\PullRemote\\")+WorkingDir));
	CString regkey;
	regkey.Format(_T("Software\\TortoiseGit\\TortoiseProc\\PullFetch\\%s_%d\\rebase"), (LPCTSTR)WorkingDir, m_IsPull);
	m_regRebase=CRegDWORD(regkey,false);
	regkey.Format(_T("Software\\TortoiseGit\\TortoiseProc\\PullFetch\\%s_%d\\ffonly"), (LPCTSTR)WorkingDir, m_IsPull);
	m_regFFonly = CRegDWORD(regkey, false);
	regkey.Format(_T("Software\\TortoiseGit\\TortoiseProc\\PullFetch\\%s_%d\\autoload"), (LPCTSTR)WorkingDir, m_IsPull);

	m_regAutoLoadPutty = CRegDWORD(regkey,this->m_bAutoLoad);
	m_bAutoLoad = m_regAutoLoadPutty;

	if(!CAppUtils::IsSSHPutty())
		m_bAutoLoad = false;

	m_bRebase = m_regRebase;

	CAutoRepository repo(g_Git.GetGitRepository());
	if (!repo)
		MessageBox(CGit::GetLibGit2LastErr(_T("Could not open repository.")), _T("TortoiseGit"), MB_OK | MB_ICONERROR);

	// Check config branch.<name>.rebase and pull.reabse
	do
	{
		if (!m_IsPull)
			break;
		if (!repo)
			break;

		if (git_repository_head_detached(repo) == 1)
			break;

		CAutoConfig config(true);
		if (git_repository_config(config.GetPointer(), repo))
			break;

		BOOL rebase = FALSE;
		// branch.<name>.rebase overrides pull.rebase
		if (config.GetBOOL(_T("branch.") + g_Git.GetCurrentBranch() + _T(".rebase"), rebase) == GIT_ENOTFOUND)
		{
			if (config.GetBOOL(_T("pull.rebase"), rebase) == GIT_ENOTFOUND)
				break;
			else if (CRegDWORD(L"Software\\TortoiseGit\\PullRebaseBehaviorLike1816", FALSE) == FALSE)
			{
				CString value;
				config.GetString(_T("pull.rebase"), value);
				if (value == _T("preserve"))
				{
					rebase = TRUE;
					m_bRebasePreserveMerges = true;
				}
			}
		}
		else if (CRegDWORD(L"Software\\TortoiseGit\\PullRebaseBehaviorLike1816", FALSE) == FALSE)
		{
			CString value;
			config.GetString(_T("branch.") + g_Git.GetCurrentBranch() + _T(".rebase"), value);
			if (value == _T("preserve"))
			{
				rebase = TRUE;
				m_bRebasePreserveMerges = true;
			}
		}
		if (!rebase)
			break;

		// Since rebase = true in config, means that "git.exe pull" will ALWAYS rebase without "--rebase".
		// So, lock it, then let Fetch Rebase do the rest things.
		m_bRebase = TRUE;
		m_bRebaseActivatedInConfigForPull = (CRegDWORD(L"Software\\TortoiseGit\\PullRebaseBehaviorLike1816", FALSE) == FALSE);
	} while (0);

	this->UpdateData(FALSE);

	this->AddOthersToAnchor();

	this->GetDlgItem(IDC_PUTTYKEY_AUTOLOAD)->EnableWindow(m_bAutoLoadEnable);

	CheckRadioButton(IDC_REMOTE_RD,IDC_OTHER_RD,IDC_REMOTE_RD);
	m_Remote.EnableWindow(TRUE);
	m_Remote.SetMaxHistoryItems(0x7FFFFFFF);
	m_Other.EnableWindow(FALSE);
	if(!m_IsPull)
	{
		m_RemoteBranch.EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_BROWSE_REF)->EnableWindow(FALSE);
	}

	if(m_IsPull)
	{
		m_bFFonly = m_regFFonly;
		UpdateData(FALSE);
		OnBnClickedCheckFfonly();
	}
	else
	{
		this->GetDlgItem(IDC_GROUP_OPTION)->EnableWindow(FALSE);
		this->GetDlgItem(IDC_CHECK_SQUASH)->EnableWindow(FALSE);
		this->GetDlgItem(IDC_CHECK_NOFF)->EnableWindow(FALSE);
		this->GetDlgItem(IDC_CHECK_FFONLY)->EnableWindow(FALSE);
		this->GetDlgItem(IDC_CHECK_NOCOMMIT)->EnableWindow(FALSE);
	}

	if (GitAdminDir::IsBareRepo(g_Git.m_CurrentDir))
		this->GetDlgItem(IDC_CHECK_REBASE)->EnableWindow(FALSE);

	if (repo && git_repository_is_shallow(repo))
	{
		m_bDepth = TRUE;
		UpdateData(FALSE);
	}
	else
	{
		GetDlgItem(IDC_CHECK_DEPTH)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_EDIT_DEPTH)->ShowWindow(SW_HIDE);
	}
	OnBnClickedCheckDepth();

	m_Other.SetCaseSensitive(TRUE);
	m_Other.SetURLHistory(TRUE);
	m_Other.LoadHistory(_T("Software\\TortoiseGit\\History\\PullURLS"), _T("url"));

	m_RemoteBranch.LoadHistory(_T("Software\\TortoiseGit\\History\\PullRemoteBranch"), _T("br"));
	m_RemoteBranch.SetCurSel(0);

	CString sWindowTitle;
	if(m_IsPull)
		sWindowTitle.LoadString(IDS_PROGRS_TITLE_PULL);
	else
		sWindowTitle.LoadString(IDS_PROGRS_TITLE_FETCH);

	CAppUtils::SetWindowTitle(m_hWnd, g_Git.m_CurrentDir, sWindowTitle);

	Refresh();

	EnableSaveRestore(_T("PullFetchDlg"));
	this->m_RemoteManage.SetURL(CString());
	return TRUE;
}

void CPullFetchDlg::Refresh()
{
	//Select pull-remote from current branch
	CString currentBranch;
	if (g_Git.GetCurrentBranchFromFile(g_Git.m_CurrentDir, currentBranch))
		currentBranch.Empty();

	CString pullRemote, pullBranch;
	g_Git.GetRemoteTrackedBranch(currentBranch, pullRemote, pullBranch);
	m_configPullRemote = pullRemote;
	m_configPullBranch  = pullBranch;

	if (pullBranch.IsEmpty())
		m_RemoteBranch.AddString(currentBranch);
	else
		m_RemoteBranch.AddString(pullBranch);

	if(pullRemote.IsEmpty())
		pullRemote = m_RemoteReg;

	if (!m_PreSelectRemote.IsEmpty())
		pullRemote = m_PreSelectRemote;

	STRING_VECTOR list;
	m_Remote.Reset();
	int sel=0;
	if(!g_Git.GetRemoteList(list))
	{
		if (!m_IsPull && list.size() > 1)
			m_Remote.AddString(CString(MAKEINTRESOURCE(IDS_PROC_PUSHFETCH_ALLREMOTES)));

		for (unsigned int i = 0; i < list.size(); ++i)
		{
			m_Remote.AddString(list[i]);
			if (!m_bAllRemotes && list[i] == pullRemote)
				sel = i + (!m_IsPull && list.size() > 1 ? 1 : 0);
		}
	}
	m_Remote.SetCurSel(sel);
	OnCbnSelchangeRemote();
}
// CPullFetchDlg message handlers

void CPullFetchDlg::OnCbnSelchangeRemote()
{
	CString remote = m_Remote.GetString();
	if (remote.IsEmpty() || remote == CString(MAKEINTRESOURCE(IDS_PROC_PUSHFETCH_ALLREMOTES)))
	{
		GetDlgItem(IDC_STATIC_TAGOPT)->SetWindowText(_T(""));
		GetDlgItem(IDC_STATIC_PRUNE)->SetWindowText(_T(""));
		return;
	}

	CString key;
	key.Format(_T("remote.%s.tagopt"), (LPCTSTR)remote);
	CString tagopt = g_Git.GetConfigValue(key);
	if (tagopt == "--no-tags")
		tagopt.LoadString(IDS_NONE);
	else if (tagopt == "--tags")
		tagopt.LoadString(IDS_ALL);
	else
		tagopt.LoadString(IDS_FETCH_REACHABLE);
	CString value;
	value.Format(_T("%s: %s"), (LPCTSTR)CString(MAKEINTRESOURCE(IDS_DEFAULT)), (LPCTSTR)tagopt);
	GetDlgItem(IDC_STATIC_TAGOPT)->SetWindowText(value);

	key.Format(_T("remote.%s.prune"), (LPCTSTR)remote);
	CString prune = g_Git.GetConfigValue(key);
	if (prune.IsEmpty())
		prune = g_Git.GetConfigValue(_T("fetch.prune"));
	if (!prune.IsEmpty())
	{
		value.Format(_T("%s: %s"), (LPCTSTR)CString(MAKEINTRESOURCE(IDS_DEFAULT)), (LPCTSTR)prune);
		GetDlgItem(IDC_STATIC_PRUNE)->SetWindowText(value);
	}
	else
		GetDlgItem(IDC_STATIC_PRUNE)->SetWindowText(_T(""));
}

void CPullFetchDlg::OnBnClickedRd()
{
	if( GetCheckedRadioButton(IDC_REMOTE_RD,IDC_OTHER_RD) == IDC_REMOTE_RD)
	{
		m_Remote.EnableWindow(TRUE);
		m_Other.EnableWindow(FALSE);
		DialogEnableWindow(IDC_CHECK_REBASE, TRUE);
		m_bRebase = m_bRebaseActivatedInConfigForPull;
		UpdateData(FALSE);
		if(!m_IsPull)
			m_RemoteBranch.EnableWindow(FALSE);
	}
	if( GetCheckedRadioButton(IDC_REMOTE_RD,IDC_OTHER_RD) == IDC_OTHER_RD)
	{
		CString clippath = CAppUtils::GetClipboardLink(m_IsPull ? _T("git pull") : _T("git fetch"), 1);
		if (clippath.IsEmpty())
			m_Other.SetCurSel(0);
		else
		{
			int argSeparator = clippath.Find(' ');
			if (argSeparator > 1 && clippath.GetLength() > argSeparator + 2)
			{
				m_Other.SetWindowText(clippath.Left(argSeparator));
				m_RemoteBranch.SetWindowText(clippath.Mid(argSeparator + 1));
			}
			else
				m_Other.SetWindowText(clippath);
		}
		m_Remote.EnableWindow(FALSE);
		m_Other.EnableWindow(TRUE);;
		DialogEnableWindow(IDC_CHECK_REBASE, FALSE);
		m_bRebase = FALSE;
		UpdateData(FALSE);
		if(!m_IsPull)
			m_RemoteBranch.EnableWindow(TRUE);
	}
}

void CPullFetchDlg::OnBnClickedOk()
{
	this->UpdateData();
	if( GetCheckedRadioButton(IDC_REMOTE_RD,IDC_OTHER_RD) == IDC_REMOTE_RD)
	{
		m_RemoteURL=m_Remote.GetString();
		m_bAllRemotes = (m_Remote.GetCurSel() == 0 && m_Remote.GetCount() > 1 && !m_IsPull);
		if( !m_IsPull ||
			(m_configPullRemote == m_RemoteURL && m_configPullBranch == m_RemoteBranch.GetString() ))
			//When fetching or when pulling from the configured tracking branch, dont explicitly set the remote branch name,
			//because otherwise git will not update the remote tracking branches.
			m_RemoteBranchName.Empty();
		else
			m_RemoteBranchName=m_RemoteBranch.GetString();
	}
	if( GetCheckedRadioButton(IDC_REMOTE_RD,IDC_OTHER_RD) == IDC_OTHER_RD)
	{
		m_Other.GetWindowTextW(m_RemoteURL);
		m_RemoteBranchName=m_RemoteBranch.GetString();

		// only store URL in history if it's value was used
		m_Other.SaveHistory();
	}

	if (m_bRebase && m_RemoteBranch.GetString().IsEmpty() && m_IsPull)
	{
		CMessageBox::Show(GetSafeHwnd(), IDS_PROC_PULL_EMPTYBRANCH, IDS_APPNAME, MB_ICONEXCLAMATION);
		return;
	}

	m_RemoteReg = m_Remote.GetString();

	m_RemoteBranch.SaveHistory();
	this->m_regRebase=this->m_bRebase;
	m_regFFonly = m_bFFonly;

	m_regAutoLoadPutty = m_bAutoLoad;

	this->OnOK();
}

void CPullFetchDlg::OnStnClickedRemoteManage()
{
	CAppUtils::LaunchRemoteSetting();
	Refresh();
}

void CPullFetchDlg::OnBnClickedButtonBrowseRef()
{
	CString initialRef;
	initialRef.Format(L"refs/remotes/%s/%s", (LPCTSTR)m_Remote.GetString(), (LPCTSTR)m_RemoteBranch.GetString());
	CString selectedRef = CBrowseRefsDlg::PickRef(false, initialRef, gPickRef_Remote);
	if(selectedRef.Left(13) != "refs/remotes/")
		return;

	selectedRef = selectedRef.Mid(13);
	int ixSlash = selectedRef.Find('/');

	CString remoteName   = selectedRef.Left(ixSlash);
	CString remoteBranch = selectedRef.Mid(ixSlash + 1);

	int ixFound = m_Remote.FindStringExact(0, remoteName);
	if(ixFound >= 0)
		m_Remote.SetCurSel(ixFound);
	m_RemoteBranch.AddString(remoteBranch, 0);

	CheckRadioButton(IDC_REMOTE_RD,IDC_OTHER_RD,IDC_REMOTE_RD);
}

void CPullFetchDlg::OnBnClickedCheckDepth()
{
	UpdateData(TRUE);
	GetDlgItem(IDC_EDIT_DEPTH)->EnableWindow(m_bDepth);
}

void CPullFetchDlg::OnBnClickedCheckFfonly()
{
	UpdateData();
	if (m_bNoFF)
	{
		m_bFFonly = FALSE;
		GetDlgItem(IDC_CHECK_FFONLY)->EnableWindow(FALSE);
	}
	else
	{
		GetDlgItem(IDC_CHECK_FFONLY)->EnableWindow(TRUE);
	}
	if (m_bFFonly)
	{
		m_bNoFF = FALSE;
		GetDlgItem(IDC_CHECK_NOFF)->EnableWindow(FALSE);
	}
	else
	{
		GetDlgItem(IDC_CHECK_NOFF)->EnableWindow(TRUE);
	}
	UpdateData(FALSE);
}
