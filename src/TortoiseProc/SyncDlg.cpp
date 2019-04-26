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

// SyncDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "SyncDlg.h"
#include "AppUtils.h"
#include "ProgressDlg.h"
#include "MessageBox.h"
#include "ImportPatchDlg.h"
#include "RebaseDlg.h"
#include "Hooks.h"
#include "SmartHandle.h"
#include "ProgressCommands/FetchProgressCommand.h"
#include "SyncTabCtrl.h"
#include "SysProgressDlg.h"

// CSyncDlg dialog

IMPLEMENT_DYNAMIC(CSyncDlg, CResizableStandAloneDialog)

CSyncDlg::CSyncDlg(CWnd* pParent /*=nullptr*/)
: CResizableStandAloneDialog(CSyncDlg::IDD, pParent)
, m_iPullRebase(0)
, m_CurrentCmd(0)
, m_bInited(false)
, m_CmdOutCurrentPos(0)
, m_bAutoLoadPuttyKey(CAppUtils::IsSSHPutty())
, m_bForce(BST_UNCHECKED)
, m_bBlock(false)
, m_BufStart(0)
, m_pThread(nullptr)
, m_bAbort(false)
, m_bDone(false)
, m_bWantToExit(false)
, m_GitCmdStatus(-1)
, m_startTick(GetTickCount64())
, m_seq(0)
{
	m_pTooltip = &m_tooltips;
}

CSyncDlg::~CSyncDlg()
{
}

void CSyncDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_CHECK_PUTTY_KEY, m_bAutoLoadPuttyKey);
	DDX_Check(pDX, IDC_CHECK_FORCE,m_bForce);
	DDX_Control(pDX, IDC_COMBOBOXEX_URL, m_ctrlURL);
	DDX_Control(pDX, IDC_BUTTON_TABCTRL, m_ctrlDumyButton);
	DDX_Control(pDX, IDC_BUTTON_PULL, m_ctrlPull);
	DDX_Control(pDX, IDC_BUTTON_PUSH, m_ctrlPush);
	DDX_Control(pDX, IDC_STATIC_STATUS, m_ctrlStatus);
	DDX_Control(pDX, IDC_PROGRESS_SYNC, m_ctrlProgress);
	DDX_Control(pDX, IDC_ANIMATE_SYNC, m_ctrlAnimate);
	DDX_Control(pDX, IDC_BUTTON_SUBMODULE,m_ctrlSubmodule);
	DDX_Control(pDX, IDC_BUTTON_STASH, m_ctrlStash);
	DDX_Control(pDX, IDC_PROG_LABEL, m_ctrlProgLabel);
	BRANCH_COMBOX_DDX;
}

BEGIN_MESSAGE_MAP(CSyncDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_BUTTON_PULL, &CSyncDlg::OnBnClickedButtonPull)
	ON_BN_CLICKED(IDC_BUTTON_PUSH, &CSyncDlg::OnBnClickedButtonPush)
	ON_BN_CLICKED(IDC_BUTTON_APPLY, &CSyncDlg::OnBnClickedButtonApply)
	ON_BN_CLICKED(IDC_BUTTON_EMAIL, &CSyncDlg::OnBnClickedButtonEmail)
	ON_BN_CLICKED(IDC_BUTTON_MANAGE, &CSyncDlg::OnBnClickedButtonManage)
	BRANCH_COMBOX_EVENT
	ON_CBN_EDITCHANGE(IDC_COMBOBOXEX_URL, &CSyncDlg::OnCbnEditchangeComboboxex)
	ON_CBN_EDITCHANGE(IDC_COMBOBOXEX_REMOTE_BRANCH, &CSyncDlg::OnCbnEditchangeComboboxex)
	ON_MESSAGE(MSG_PROGRESSDLG_UPDATE_UI, OnProgressUpdateUI)
	ON_MESSAGE(WM_PROG_CMD_FINISH, OnProgCmdFinish)
	ON_BN_CLICKED(IDC_BUTTON_COMMIT, &CSyncDlg::OnBnClickedButtonCommit)
	ON_BN_CLICKED(IDC_BUTTON_SUBMODULE, &CSyncDlg::OnBnClickedButtonSubmodule)
	ON_BN_CLICKED(IDC_BUTTON_STASH, &CSyncDlg::OnBnClickedButtonStash)
	ON_WM_TIMER()
	ON_REGISTERED_MESSAGE(TaskBarButtonCreated, OnTaskbarBtnCreated)
	ON_BN_CLICKED(IDC_CHECK_FORCE, &CSyncDlg::OnBnClickedCheckForce)
	ON_BN_CLICKED(IDC_LOG, &CSyncDlg::OnBnClickedLog)
	ON_WM_DESTROY()
	ON_WM_THEMECHANGED()
END_MESSAGE_MAP()

void CSyncDlg::EnableControlButton(bool bEnabled)
{
	GetDlgItem(IDC_BUTTON_PULL)->EnableWindow(bEnabled);
	GetDlgItem(IDC_BUTTON_PUSH)->EnableWindow(bEnabled);
	GetDlgItem(IDC_BUTTON_APPLY)->EnableWindow(bEnabled);
	GetDlgItem(IDC_BUTTON_EMAIL)->EnableWindow(bEnabled);
	GetDlgItem(IDOK)->EnableWindow(bEnabled);
	GetDlgItem(IDC_BUTTON_SUBMODULE)->EnableWindow(bEnabled);
	GetDlgItem(IDC_BUTTON_STASH)->EnableWindow(bEnabled);
}
// CSyncDlg message handlers

bool CSyncDlg::AskSetTrackedBranch()
{
	CString remote, remoteBranch;
	g_Git.GetRemoteTrackedBranch(m_strLocalBranch, remote, remoteBranch);
	if (remoteBranch.IsEmpty())
	{
		remoteBranch = m_strRemoteBranch;
		if (remoteBranch.IsEmpty())
			remoteBranch = m_strLocalBranch;
		CString temp;
		temp.FormatMessage(IDS_NOTYET_SETTRACKEDBRANCH, static_cast<LPCTSTR>(m_strLocalBranch), static_cast<LPCTSTR>(remoteBranch));
		BOOL dontShowAgain = FALSE;
		auto ret = CMessageBox::ShowCheck(GetSafeHwnd(), temp, L"TortoiseGit", MB_ICONQUESTION | MB_YESNOCANCEL, nullptr, CString(MAKEINTRESOURCE(IDS_MSGBOX_DONOTSHOW)), &dontShowAgain);
		if (dontShowAgain)
			CRegDWORD(L"Software\\TortoiseGit\\AskSetTrackedBranch") = FALSE;
		if (ret == IDCANCEL)
			return false;
		if (ret == IDYES)
		{
			CString key;
			key.Format(L"branch.%s.remote", static_cast<LPCTSTR>(m_strLocalBranch));
			g_Git.SetConfigValue(key, m_strURL);
			key.Format(L"branch.%s.merge", static_cast<LPCTSTR>(m_strLocalBranch));
			g_Git.SetConfigValue(key, L"refs/heads/" + g_Git.StripRefName(remoteBranch));
		}
	}
	return true;
}

void CSyncDlg::OnBnClickedButtonPull()
{
	bool bShift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;

	int CurrentEntry = static_cast<int>(this->m_ctrlPull.GetCurrentEntry());
	this->m_regPullButton = CurrentEntry;

	if (bShift && CurrentEntry > 1)
		return;

	this->m_bAbort=false;
	this->m_GitCmdList.clear();
	m_ctrlCmdOut.SetWindowText(L"");
	m_LogText.Empty();

	this->UpdateData();
	UpdateCombox();

	if (g_Git.GetHash(m_oldHash, L"HEAD"))
	{
		MessageBox(g_Git.GetGitLastErr(L"Could not get HEAD hash."), L"TortoiseGit", MB_ICONERROR);
		return;
	}

	m_refList.Clear();
	m_newHashMap.clear();
	m_oldHashMap.clear();

	if( CurrentEntry == 0)
	{
		CGitHash localBranchHash;
		if (g_Git.GetHash(localBranchHash, m_strLocalBranch))
		{
			MessageBox(g_Git.GetGitLastErr(L"Could not get hash of \"" + m_strLocalBranch + L"\"."), L"TortoiseGit", MB_ICONERROR);
			return;
		}
		if (localBranchHash != m_oldHash)
		{
			CMessageBox::Show(GetSafeHwnd(), IDS_PROC_SYNC_PULLWRONGBRANCH, IDS_APPNAME, MB_OK | MB_ICONERROR);
			return;
		}
	}

	if(this->m_strURL.IsEmpty())
	{
		CMessageBox::Show(GetSafeHwnd(), IDS_PROC_GITCONFIG_URLEMPTY, IDS_APPNAME, MB_OK | MB_ICONERROR);
		return;
	}

	if (CurrentEntry == 6)
	{
		SwitchToRun();
		m_ctrlTabCtrl.ShowTab(IDC_LOG - 1, false);
		m_ctrlTabCtrl.ShowTab(IDC_REFLIST - 1, false);
		m_ctrlTabCtrl.ShowTab(IDC_OUT_LOGLIST - 1, false);
		m_ctrlTabCtrl.ShowTab(IDC_OUT_CHANGELIST - 1, false);
		m_ctrlTabCtrl.ShowTab(IDC_IN_LOGLIST - 1, false);
		m_ctrlTabCtrl.ShowTab(IDC_IN_CHANGELIST - 1, false);
		m_ctrlTabCtrl.ShowTab(IDC_IN_CONFLICT - 1, false);
		m_ctrlTabCtrl.ShowTab(IDC_TAGCOMPARELIST - 1, true);

		CSysProgressDlg sysProgressDlg;
		sysProgressDlg.SetTitle(CString(MAKEINTRESOURCE(IDS_APPNAME)));
		sysProgressDlg.SetLine(1, CString(MAKEINTRESOURCE(IDS_LOADING)));
		sysProgressDlg.SetLine(2, CString(MAKEINTRESOURCE(IDS_PROGRESSWAIT)));
		sysProgressDlg.SetShowProgressBar(false);
		sysProgressDlg.ShowModal(this, true);
		CString err;
		auto ret = m_tagCompareList.Fill(m_strURL, err);
		sysProgressDlg.Stop();
		if (ret)
			MessageBox(err, L"TortoiseGit", MB_ICONERROR);

		BringWindowToTop();
		SwitchToInput();
		EnableControlButton();
		return;
	}

	if (!IsURL() && !m_strRemoteBranch.IsEmpty() && CurrentEntry == 0 && CRegDWORD(L"Software\\TortoiseGit\\AskSetTrackedBranch", TRUE) == TRUE)
	{
		if (!AskSetTrackedBranch())
			return;
	}

	if (m_bAutoLoadPuttyKey && CurrentEntry != 4) // CurrentEntry (Remote Update) handles this on its own)
	{
		CAppUtils::LaunchPAgent(this->GetSafeHwnd(), nullptr, &m_strURL);
	}

	if (g_Git.GetMapHashToFriendName(m_oldHashMap))
		MessageBox(g_Git.GetGitLastErr(L"Could not get all refs."), L"TortoiseGit", MB_ICONERROR);

	if (bShift && (CurrentEntry == 0 || CurrentEntry == 1))
	{
		if (CurrentEntry == 1 || CurrentEntry == 2 || CurrentEntry == 3)
			CAppUtils::Fetch(GetSafeHwnd(), !IsURL() ? m_strURL : L"");
		else
			CAppUtils::Pull(GetSafeHwnd());

		FillNewRefMap();
		FetchOutList(true);

		int hasConflicts = g_Git.HasWorkingTreeConflicts();
		if (hasConflicts < 0)
		{
			this->m_ctrlCmdOut.SetSel(-1, -1);
			this->m_ctrlCmdOut.ReplaceSel(g_Git.GetGitLastErr(L"Checking for conflicts failed.", CGit::GIT_CMD_CHECKCONFLICTS));

			this->ShowTab(IDC_CMD_LOG);
			return;
		}

		if (hasConflicts)
		{
			this->m_ConflictFileList.Clear();
			this->m_ConflictFileList.GetStatus(nullptr, true);
			this->m_ConflictFileList.Show(CTGitPath::LOGACTIONS_UNMERGED,
				CTGitPath::LOGACTIONS_UNMERGED);

			this->ShowTab(IDC_IN_CONFLICT);
			CMessageBox::ShowCheck(GetSafeHwnd(), IDS_NEED_TO_RESOLVE_CONFLICTS_HINT, IDS_APPNAME, MB_ICONINFORMATION, L"MergeConflictsNeedsCommit", IDS_MSGBOX_DONOTSHOWAGAIN);
		}
		else
			ShowInCommits(L"HEAD");

		return;
	}

	CString force;
	if(this->m_bForce)
		force = L" --force";

	CString cmd;

	m_iPullRebase = 0;
	if (CurrentEntry == 0) // check whether we need to override Pull if pull.rebase is set
	{
		CAutoRepository repo(g_Git.GetGitRepository());
		if (!repo)
			MessageBox(CGit::GetLibGit2LastErr(L"Could not open repository."), L"TortoiseGit", MB_OK | MB_ICONERROR);

		// Check config branch.<name>.rebase and pull.reabse
		do
		{
			if (!repo)
				break;

			if (git_repository_head_detached(repo) == 1)
				break;

			CAutoConfig config(true);
			if (git_repository_config(config.GetPointer(), repo))
				break;

			// branch.<name>.rebase overrides pull.rebase
			if (config.GetBOOL(L"branch." + g_Git.GetCurrentBranch() + L".rebase", m_iPullRebase) == GIT_ENOTFOUND)
			{
				if (config.GetBOOL(L"pull.rebase", m_iPullRebase) == GIT_ENOTFOUND)
					break;
				else
				{
					CString value;
					config.GetString(L"pull.rebase", value);
					if (value == L"preserve")
					{
						m_iPullRebase = 2;
						break;
					}
				}
			}
			else
			{
				CString value;
				config.GetString(L"branch." + g_Git.GetCurrentBranch() + L".rebase", value);
				if (value == L"preserve")
				{
					m_iPullRebase = 2;
					break;
				}
			}
		} while (0);
		if (m_iPullRebase > 0)
		{
			CurrentEntry = 1;
			if (m_strRemoteBranch.IsEmpty())
			{
				CMessageBox::Show(GetSafeHwnd(), IDS_PROC_PULL_EMPTYBRANCH, IDS_APPNAME, MB_ICONEXCLAMATION);
				return;
			}
		}
	}

	SwitchToRun();

	ShowTab(IDC_CMD_LOG);

	m_ctrlTabCtrl.ShowTab(IDC_REFLIST - 1, true);
	m_ctrlTabCtrl.ShowTab(IDC_IN_LOGLIST - 1, false);
	m_ctrlTabCtrl.ShowTab(IDC_IN_CHANGELIST - 1, false);
	m_ctrlTabCtrl.ShowTab(IDC_IN_CONFLICT - 1, false);
	m_ctrlTabCtrl.ShowTab(IDC_TAGCOMPARELIST - 1, false);

	///Pull
	if(CurrentEntry == 0) //Pull
	{
		CString remotebranch;
		remotebranch = m_strRemoteBranch;

		if(!IsURL())
		{
			CString pullRemote, pullBranch;
			g_Git.GetRemoteTrackedBranch(m_strLocalBranch, pullRemote, pullBranch);
			if(pullBranch == remotebranch && pullRemote == this->m_strURL)
				remotebranch.Empty();
		}

		cmd.Format(L"git.exe pull -v --progress%s \"%s\" %s",
				static_cast<LPCTSTR>(force),
				static_cast<LPCTSTR>(m_strURL),
				static_cast<LPCTSTR>(remotebranch));

		m_CurrentCmd = GIT_COMMAND_PULL;
		m_GitCmdList.push_back(cmd);

		StartWorkerThread();
	}

	///Fetch
	if (CurrentEntry == 1 || CurrentEntry == 2 || CurrentEntry == 3)
	{
		m_oldRemoteHash.Empty();
		CString remotebranch;
		if (CurrentEntry == 3)
			m_strRemoteBranch.Empty();
		else if (IsURL() || m_strRemoteBranch.IsEmpty())
		{
			remotebranch=this->m_strRemoteBranch;

		}
		else
		{
			remotebranch.Format(L"remotes/%s/%s", static_cast<LPCTSTR>(m_strURL), static_cast<LPCTSTR>(m_strRemoteBranch));
			g_Git.GetHash(m_oldRemoteHash, remotebranch);
			if (m_oldRemoteHash.IsEmpty())
				remotebranch=m_strRemoteBranch;
			else
				remotebranch = m_strRemoteBranch + L':' + remotebranch;
		}

		if (CurrentEntry == 1 || CurrentEntry == 3)
			m_CurrentCmd = GIT_COMMAND_FETCH;
		else
			m_CurrentCmd = GIT_COMMAND_FETCHANDREBASE;

		if (g_Git.UsingLibGit2(CGit::GIT_CMD_FETCH))
		{
			CString refspec;
			if (!remotebranch.IsEmpty())
				refspec.Format(L"refs/heads/%s:refs/remotes/%s/%s", static_cast<LPCTSTR>(m_strRemoteBranch), static_cast<LPCTSTR>(m_strURL), static_cast<LPCTSTR>(m_strRemoteBranch));

			progressCommand = std::make_unique<FetchProgressCommand>();
			FetchProgressCommand* fetchProgressCommand = static_cast<FetchProgressCommand*>(progressCommand.get());
			fetchProgressCommand->SetUrl(m_strURL);
			fetchProgressCommand->SetRefSpec(refspec);
			m_GitProgressList.SetCommand(progressCommand.get());
			m_GitProgressList.Init();
			ShowTab(IDC_CMD_GIT_PROG);
		}
		else
		{
			cmd.Format(L"git.exe fetch --progress -v%s \"%s\" %s",
					static_cast<LPCTSTR>(force),
					static_cast<LPCTSTR>(m_strURL),
					static_cast<LPCTSTR>(remotebranch));

			m_GitCmdList.push_back(cmd);

			StartWorkerThread();
		}
	}

	///Remote Update
	if (CurrentEntry == 4)
	{
		if (m_bAutoLoadPuttyKey)
		{
			for (size_t i = 0; i < m_remotelist.size(); ++i)
				CAppUtils::LaunchPAgent(this->GetSafeHwnd(), nullptr, &m_remotelist[i]);
		}

		m_CurrentCmd = GIT_COMMAND_REMOTE;
		cmd = L"git.exe remote update";
		m_GitCmdList.push_back(cmd);

		StartWorkerThread();
	}

	///Cleanup stale remote banches
	if (CurrentEntry == 5)
	{
		m_CurrentCmd = GIT_COMMAND_REMOTE;
		cmd.Format(L"git.exe remote prune \"%s\"", static_cast<LPCTSTR>(m_strURL));
		m_GitCmdList.push_back(cmd);

		StartWorkerThread();
	}
}

void CSyncDlg::ShowInCommits(const CString& friendname)
{
	CGitHash newHash;

	if (g_Git.GetHash(newHash, friendname))
	{
		MessageBox(g_Git.GetGitLastErr(L"Could not get " + friendname + L" hash."), L"TortoiseGit", MB_ICONERROR);
		return;
	}

	if (newHash == m_oldHash)
	{
		m_ctrlTabCtrl.ShowTab(IDC_IN_CHANGELIST - 1, false);
		m_InLogList.ShowText(CString(MAKEINTRESOURCE(IDS_UPTODATE)));
		m_ctrlTabCtrl.ShowTab(IDC_IN_LOGLIST - 1, true);
		ShowTab(IDC_REFLIST);
	}
	else
	{
		m_ctrlTabCtrl.ShowTab(IDC_IN_CHANGELIST - 1, true);
		m_ctrlTabCtrl.ShowTab(IDC_IN_LOGLIST - 1, true);

		AddDiffFileList(&m_InChangeFileList, &m_arInChangeList, newHash, m_oldHash);

		CString range;
		range.Format(L"%s..%s", static_cast<LPCTSTR>(m_oldHash.ToString()), static_cast<LPCTSTR>(newHash.ToString()));
		m_InLogList.FillGitLog(nullptr, &range, CGit::LOG_INFO_STAT | CGit::LOG_INFO_FILESTATE | CGit::LOG_INFO_SHOW_MERGEDFILE);
		ShowTab(IDC_IN_LOGLIST);
	}
}

void CSyncDlg::PullComplete()
{
	EnableControlButton(true);
	SwitchToInput();
	this->FetchOutList(true);

	if( this ->m_GitCmdStatus )
	{
		int hasConflicts = g_Git.HasWorkingTreeConflicts();
		if (hasConflicts < 0)
		{
			this->m_ctrlCmdOut.SetSel(-1,-1);
			this->m_ctrlCmdOut.ReplaceSel(g_Git.GetGitLastErr(L"Checking for conflicts failed.", CGit::GIT_CMD_CHECKCONFLICTS));

			this->ShowTab(IDC_CMD_LOG);
			return;
		}

		if (hasConflicts)
		{
			this->m_ConflictFileList.Clear();
			this->m_ConflictFileList.GetStatus(nullptr, true);
			this->m_ConflictFileList.Show(CTGitPath::LOGACTIONS_UNMERGED,
											CTGitPath::LOGACTIONS_UNMERGED);

			this->ShowTab(IDC_IN_CONFLICT);
			CMessageBox::ShowCheck(GetSafeHwnd(), IDS_NEED_TO_RESOLVE_CONFLICTS_HINT, IDS_APPNAME, MB_ICONINFORMATION, L"MergeConflictsNeedsCommit", IDS_MSGBOX_DONOTSHOWAGAIN);
		}
		else
			this->ShowTab(IDC_CMD_LOG);

	}
	else
		ShowInCommits(L"HEAD");
}

void CSyncDlg::FetchComplete()
{
	EnableControlButton(true);
	SwitchToInput();

	if (g_Git.UsingLibGit2(CGit::GIT_CMD_FETCH))
		ShowTab(IDC_CMD_GIT_PROG);
	else
		ShowTab(IDC_REFLIST);

	if (m_GitCmdStatus || (m_CurrentCmd != GIT_COMMAND_FETCHANDREBASE && m_iPullRebase == 0))
	{
		FetchOutList(true);
		return;
	}

	CString remote;
	CString remotebranch;
	CString upstream = L"FETCH_HEAD";
	m_ctrlURL.GetWindowText(remote);
	if (!remote.IsEmpty())
	{
		if (std::find(m_remotelist.cbegin(), m_remotelist.cend(), remote) == m_remotelist.cend())
			remote.Empty();
	}
	m_ctrlRemoteBranch.GetWindowText(remotebranch);
	if (!remote.IsEmpty() && !remotebranch.IsEmpty())
		upstream = L"remotes/" + remote + L'/' + remotebranch;

	if (m_iPullRebase > 0)
	{
		CAppUtils::RebaseAfterFetch(GetSafeHwnd(), upstream, m_iPullRebase ? 2 : 0, m_iPullRebase == 2);
		FillNewRefMap();
		FetchOutList(true);

		ShowInCommits(L"HEAD");

		return;
	}

	CGitHash remoteBranchHash;
	g_Git.GetHash(remoteBranchHash, upstream);
	if (remoteBranchHash == m_oldRemoteHash && !m_oldRemoteHash.IsEmpty() && CMessageBox::ShowCheck(this->GetSafeHwnd(), IDS_REBASE_BRANCH_UNCHANGED, IDS_APPNAME, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2, L"OpenRebaseRemoteBranchUnchanged", IDS_MSGBOX_DONOTSHOWAGAIN) == IDNO)
	{
		ShowInCommits(upstream);
		return;
	}

	if (g_Git.IsFastForward(L"HEAD", upstream))
	{
		UINT ret = CMessageBox::ShowCheck(GetSafeHwnd(), IDS_REBASE_BRANCH_FF, IDS_APPNAME, 2, IDI_QUESTION, IDS_MERGEBUTTON, IDS_REBASEBUTTON, IDS_ABORTBUTTON, L"OpenRebaseRemoteBranchFastForwards", IDS_MSGBOX_DONOTSHOWAGAIN);
		if (ret == 3)
			return;
		if (ret == 1)
		{
			CProgressDlg mergeProgress;
			mergeProgress.m_GitCmd = L"git.exe merge --ff-only " + upstream;
			mergeProgress.m_AutoClose = AUTOCLOSE_IF_NO_ERRORS;
			mergeProgress.m_PostCmdCallback = [](DWORD status, PostCmdList& postCmdList)
			{
				if (status && g_Git.HasWorkingTreeConflicts())
				{
					// there are conflict files
					postCmdList.emplace_back(IDI_RESOLVE, IDS_PROGRS_CMD_RESOLVE, []
					{
						CString sCmd;
						sCmd.Format(L"/command:commit /path:\"%s\"", static_cast<LPCTSTR>(g_Git.m_CurrentDir));
						CAppUtils::RunTortoiseGitProc(sCmd);
					});
				}
			};
			mergeProgress.DoModal();
			FillNewRefMap();
			FetchOutList(true);

			ShowInCommits(L"HEAD");

			return;
		}
	}

	CAppUtils::RebaseAfterFetch(GetSafeHwnd(), upstream);
	FillNewRefMap();
	FetchOutList(true);

	ShowInCommits(L"HEAD");
}

void CSyncDlg::StashComplete()
{
	EnableControlButton(true);
	INT_PTR entry = m_ctrlStash.GetCurrentEntry();
	if (entry != 1 && entry != 2)
		return;

	SwitchToInput();
	if (m_GitCmdStatus)
	{
		int hasConflicts = g_Git.HasWorkingTreeConflicts();
		if (hasConflicts < 0)
		{
			m_ctrlCmdOut.SetSel(-1, -1);
			m_ctrlCmdOut.ReplaceSel(g_Git.GetGitLastErr(L"Checking for conflicts failed.", CGit::GIT_CMD_CHECKCONFLICTS));

			ShowTab(IDC_CMD_LOG);
			return;
		}

		if (hasConflicts)
		{
			m_ConflictFileList.Clear();
			m_ConflictFileList.GetStatus(nullptr, true);
			m_ConflictFileList.Show(CTGitPath::LOGACTIONS_UNMERGED, CTGitPath::LOGACTIONS_UNMERGED);

			ShowTab(IDC_IN_CONFLICT);
		}
		else
			ShowTab(IDC_CMD_LOG);
	}
}

void CSyncDlg::OnBnClickedButtonPush()
{
	bool bShift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
	this->UpdateData();
	UpdateCombox();

	if (bShift)
	{
		if (m_ctrlPush.GetCurrentEntry() == 0)
		{
			CAppUtils::Push(GetSafeHwnd(), g_Git.FixBranchName(m_strLocalBranch));
			FillNewRefMap();
			FetchOutList(true);
		}
		return;
	}

	m_ctrlCmdOut.SetWindowText(L"");
	m_LogText.Empty();

	if(this->m_strURL.IsEmpty())
	{
		CMessageBox::Show(GetSafeHwnd(), IDS_PROC_GITCONFIG_URLEMPTY, IDS_APPNAME, MB_OK | MB_ICONERROR);
		return;
	}

	if (!IsURL() && m_ctrlPush.GetCurrentEntry() == 0 && CRegDWORD(L"Software\\TortoiseGit\\AskSetTrackedBranch", TRUE) == TRUE)
	{
		if (!AskSetTrackedBranch())
			return;
	}

	this->m_regPushButton = static_cast<DWORD>(this->m_ctrlPush.GetCurrentEntry());
	this->SwitchToRun();
	this->m_bAbort=false;
	this->m_GitCmdList.clear();

	ShowTab(IDC_CMD_LOG);

	CString cmd;
	CString arg;

	CString error;
	DWORD exitcode = 0xFFFFFFFF;
	CHooks::Instance().SetProjectProperties(g_Git.m_CurrentDir, m_ProjectProperties);
	if (CHooks::Instance().PrePush(GetSafeHwnd(), g_Git.m_CurrentDir, exitcode, error))
	{
		if (exitcode)
		{
			CString sErrorMsg;
			sErrorMsg.Format(IDS_HOOK_ERRORMSG, static_cast<LPCWSTR>(error));
			CTaskDialog taskdlg(sErrorMsg, CString(MAKEINTRESOURCE(IDS_HOOKFAILED_TASK2)), L"TortoiseGit", 0, TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
			taskdlg.AddCommandControl(101, CString(MAKEINTRESOURCE(IDS_HOOKFAILED_TASK3)));
			taskdlg.AddCommandControl(102, CString(MAKEINTRESOURCE(IDS_HOOKFAILED_TASK4)));
			taskdlg.SetDefaultCommandControl(101);
			taskdlg.SetMainIcon(TD_ERROR_ICON);
			if (taskdlg.DoModal(GetSafeHwnd()) != 102)
				return;
		}
	}

	CString refName = g_Git.FixBranchName(m_strLocalBranch);
	switch (m_ctrlPush.GetCurrentEntry())
	{
	case 1:
		arg += L" --tags";
		break;
	case 2:
		refName = L"refs/notes/commits";	//default ref for notes
		break;
	}

	if(this->m_bForce)
		arg += L" --force";

	cmd.Format(L"git.exe push -v --progress%s \"%s\" %s",
				static_cast<LPCTSTR>(arg),
				static_cast<LPCTSTR>(m_strURL),
				static_cast<LPCTSTR>(refName));

	if (!m_strRemoteBranch.IsEmpty() && m_ctrlPush.GetCurrentEntry() != 2)
	{
		cmd += L':' + m_strRemoteBranch;
	}

	m_GitCmdList.push_back(cmd);

	m_CurrentCmd = GIT_COMMAND_PUSH;

	if(this->m_bAutoLoadPuttyKey)
	{
		CAppUtils::LaunchPAgent(this->GetSafeHwnd(), nullptr, &m_strURL);
	}

	StartWorkerThread();
}

void CSyncDlg::OnBnClickedButtonApply()
{
	CGitHash oldhash;
	if (g_Git.GetHash(oldhash, L"HEAD"))
	{
		MessageBox(g_Git.GetGitLastErr(L"Could not get HEAD hash."), L"TortoiseGit", MB_ICONERROR);
		return;
	}

	CImportPatchDlg dlg;
	CString cmd,output;

	if(dlg.DoModal() == IDOK)
	{
		int err=0;
		for (int i = 0; i < dlg.m_PathList.GetCount(); ++i)
		{
			cmd.Format(L"git.exe am \"%s\"", static_cast<LPCTSTR>(dlg.m_PathList[i].GetGitPathString()));

			if (g_Git.Run(cmd, &output, CP_UTF8))
			{
				CMessageBox::Show(GetSafeHwnd(), output, L"TortoiseGit", MB_OK | MB_ICONERROR);

				err=1;
				break;
			}
			this->m_ctrlCmdOut.SetSel(-1,-1);
			this->m_ctrlCmdOut.ReplaceSel(cmd + L'\n');
			this->m_ctrlCmdOut.SetSel(-1,-1);
			this->m_ctrlCmdOut.ReplaceSel(output);
		}

		CGitHash newhash;
		if (g_Git.GetHash(newhash, L"HEAD"))
		{
			MessageBox(g_Git.GetGitLastErr(L"Could not get HEAD hash after applying patches."), L"TortoiseGit", MB_ICONERROR);
			return;
		}

		this->m_InLogList.Clear();
		this->m_InChangeFileList.Clear();

		if(newhash == oldhash)
		{
			this->m_ctrlTabCtrl.ShowTab(IDC_IN_CHANGELIST-1,false);
			this->m_InLogList.ShowText(L"No commits get from patch");
			this->m_ctrlTabCtrl.ShowTab(IDC_IN_LOGLIST-1,true);

		}
		else
		{
			this->m_ctrlTabCtrl.ShowTab(IDC_IN_CHANGELIST-1,true);
			this->m_ctrlTabCtrl.ShowTab(IDC_IN_LOGLIST-1,true);

			CString range;
			range.Format(L"%s..%s", static_cast<LPCTSTR>(m_oldHash.ToString()), static_cast<LPCTSTR>(newhash.ToString()));
			this->AddDiffFileList(&m_InChangeFileList, &m_arInChangeList, newhash, oldhash);
			m_InLogList.FillGitLog(nullptr, &range, CGit::LOG_INFO_STAT| CGit::LOG_INFO_FILESTATE | CGit::LOG_INFO_SHOW_MERGEDFILE);

			this->FetchOutList(true);
		}

		this->m_ctrlTabCtrl.ShowTab(IDC_CMD_LOG-1,true);

		if(err)
		{
			this->ShowTab(IDC_CMD_LOG);
		}
		else
		{
			this->ShowTab(IDC_IN_LOGLIST);
		}
	}
}

void CSyncDlg::OnBnClickedButtonEmail()
{
	CString cmd, out, err;

	this->m_strLocalBranch = this->m_ctrlLocalBranch.GetString();
	this->m_ctrlRemoteBranch.GetWindowText(this->m_strRemoteBranch);
	this->m_ctrlURL.GetWindowText(this->m_strURL);
	m_strURL=m_strURL.Trim();
	m_strRemoteBranch=m_strRemoteBranch.Trim();

	cmd.Format(L"git.exe format-patch -o \"%s\" %s/%s..%s",
					static_cast<LPCTSTR>(g_Git.m_CurrentDir),
					static_cast<LPCTSTR>(m_strURL), static_cast<LPCTSTR>(m_strRemoteBranch), static_cast<LPCTSTR>(g_Git.FixBranchName(m_strLocalBranch)));

	if (g_Git.Run(cmd, &out, &err, CP_UTF8))
	{
		CMessageBox::Show(GetSafeHwnd(), out + L'\n' + err, L"TortoiseGit", MB_OK | MB_ICONERROR);
		return ;
	}

	CAppUtils::SendPatchMail(GetSafeHwnd(), cmd, out);
}
void CSyncDlg::ShowProgressCtrl(bool bShow)
{
	int b=bShow?SW_NORMAL:SW_HIDE;
	this->m_ctrlAnimate.ShowWindow(b);
	this->m_ctrlProgress.ShowWindow(b);
	this->m_ctrlProgLabel.ShowWindow(b);
	this->m_ctrlAnimate.Open(IDR_DOWNLOAD);
	if(b == SW_NORMAL)
		this->m_ctrlAnimate.Play(0, UINT_MAX, UINT_MAX);
	else
		this->m_ctrlAnimate.Stop();
}
void CSyncDlg::ShowInputCtrl(bool bShow)
{
	int b=bShow?SW_NORMAL:SW_HIDE;
	this->m_ctrlURL.ShowWindow(b);
	this->m_ctrlLocalBranch.ShowWindow(b);
	this->m_ctrlRemoteBranch.ShowWindow(b);
	this->GetDlgItem(IDC_BUTTON_LOCAL_BRANCH)->ShowWindow(b);
	this->GetDlgItem(IDC_BUTTON_REMOTE_BRANCH)->ShowWindow(b);
	this->GetDlgItem(IDC_STATIC_LOCAL_BRANCH)->ShowWindow(b);
	this->GetDlgItem(IDC_STATIC_REMOTE_BRANCH)->ShowWindow(b);
	this->GetDlgItem(IDC_BUTTON_MANAGE)->ShowWindow(b);
	this->GetDlgItem(IDC_CHECK_PUTTY_KEY)->ShowWindow(b);
	this->GetDlgItem(IDC_CHECK_FORCE)->ShowWindow(b);
	this->GetDlgItem(IDC_STATIC_REMOTE_URL)->ShowWindow(b);
}
BOOL CSyncDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	// Let the TaskbarButtonCreated message through the UIPI filter. If we don't
	// do this, Explorer would be unable to send that message to our window if we
	// were running elevated. It's OK to make the call all the time, since if we're
	// not elevated, this is a no-op.
	CHANGEFILTERSTRUCT cfs = { sizeof(CHANGEFILTERSTRUCT) };
	typedef BOOL STDAPICALLTYPE ChangeWindowMessageFilterExDFN(HWND hWnd, UINT message, DWORD action, PCHANGEFILTERSTRUCT pChangeFilterStruct);
	CAutoLibrary hUser = AtlLoadSystemLibraryUsingFullPath(L"user32.dll");
	if (hUser)
	{
		auto pfnChangeWindowMessageFilterEx = reinterpret_cast<ChangeWindowMessageFilterExDFN*>(GetProcAddress(hUser, "ChangeWindowMessageFilterEx"));
		if (pfnChangeWindowMessageFilterEx)
			pfnChangeWindowMessageFilterEx(m_hWnd, TaskBarButtonCreated, MSGFLT_ALLOW, &cfs);
	}
	m_pTaskbarList.Release();
	if (FAILED(m_pTaskbarList.CoCreateInstance(CLSID_TaskbarList)))
		m_pTaskbarList = nullptr;

	this->GetDlgItem(IDC_CHECK_PUTTY_KEY)->EnableWindow(CAppUtils::IsSSHPutty());

	/*
	this->m_ctrlAnimate.ShowWindow(SW_NORMAL);
	this->m_ctrlAnimate.Open(IDR_DOWNLOAD);
	this->m_ctrlAnimate.Play(0,-1,-1);
	*/

	// ------------------ Create Tabctrl -----------
	CWnd *pwnd=this->GetDlgItem(IDC_BUTTON_TABCTRL);
	CRect rectDummy;
	pwnd->GetWindowRect(&rectDummy);
	this->ScreenToClient(rectDummy);

	if (!m_ctrlTabCtrl.Create(CMFCTabCtrl::STYLE_FLAT, rectDummy, this, IDC_SYNC_TAB))
	{
		TRACE0("Failed to create output tab window\n");
		return FALSE;      // fail to create
	}
	m_ctrlTabCtrl.SetResizeMode(CMFCTabCtrl::RESIZE_NO);

	// -------------Create Command Log Ctrl ---------
	DWORD dwStyle = ES_MULTILINE | ES_READONLY | WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_AUTOVSCROLL |WS_VSCROLL;

	if( !m_ctrlCmdOut.Create(dwStyle,rectDummy,&m_ctrlTabCtrl,IDC_CMD_LOG))
	{
		TRACE0("Failed to create Log commits window\n");
		return FALSE;      // fail to create
	}

	// set the font to use in the log message view, configured in the settings dialog
	CFont m_logFont;
	CAppUtils::CreateFontForLogs(m_logFont);
	m_ctrlCmdOut.SetFont(&m_logFont);
	m_ctrlTabCtrl.InsertTab(&m_ctrlCmdOut, CString(MAKEINTRESOURCE(IDS_LOG)), -1);
	// make the log message rich edit control send a message when the mouse pointer is over a link
	m_ctrlCmdOut.SendMessage(EM_SETEVENTMASK, NULL, ENM_LINK | ENM_SCROLL);

	//----------  Create in coming list ctrl -----------
	dwStyle =LVS_REPORT | LVS_SHOWSELALWAYS | LVS_ALIGNLEFT | LVS_OWNERDATA | WS_BORDER | WS_TABSTOP | WS_CHILD | WS_VISIBLE;;

	if( !m_InLogList.Create(dwStyle,rectDummy,&m_ctrlTabCtrl,IDC_IN_LOGLIST))
	{
		TRACE0("Failed to create output commits window\n");
		return FALSE;      // fail to create
	}
	// for some unknown reason, the SetExtendedStyle in OnCreate/PreSubclassWindow is not working here
	m_InLogList.SetStyle();

	m_ctrlTabCtrl.InsertTab(&m_InLogList, CString(MAKEINTRESOURCE(IDS_PROC_SYNC_INCOMMITS)), -1);

	m_InLogList.m_ColumnRegKey = L"SyncIn";
	m_InLogList.InsertGitColumn();

	//----------- Create In Change file list -----------
	dwStyle = LVS_REPORT | LVS_SHOWSELALWAYS | LVS_ALIGNLEFT | WS_BORDER | WS_TABSTOP | WS_CHILD | WS_VISIBLE;

	if( !m_InChangeFileList.Create(dwStyle,rectDummy,&m_ctrlTabCtrl,IDC_IN_CHANGELIST))
	{
		TRACE0("Failed to create output change files window\n");
		return FALSE;      // fail to create
	}
	m_ctrlTabCtrl.InsertTab(&m_InChangeFileList, CString(MAKEINTRESOURCE(IDS_PROC_SYNC_INCHANGELIST)), -1);

	m_InChangeFileList.Init(GITSLC_COLEXT | GITSLC_COLSTATUS |GITSLC_COLADD|GITSLC_COLDEL, L"InSyncDlg",
							(CGitStatusListCtrl::GetContextMenuBit(CGitStatusListCtrl::IDGITLC_COMPARETWOREVISIONS) |
							CGitStatusListCtrl::GetContextMenuBit(CGitStatusListCtrl::IDGITLC_GNUDIFF2REVISIONS)), false, false, GITSLC_COLEXT | GITSLC_COLSTATUS | GITSLC_COLADD | GITSLC_COLDEL);


	//---------- Create Conflict List Ctrl -----------------
	dwStyle = LVS_REPORT | LVS_SHOWSELALWAYS | LVS_ALIGNLEFT | WS_BORDER | WS_TABSTOP | WS_CHILD | WS_VISIBLE;

	if( !m_ConflictFileList.Create(dwStyle,rectDummy,&m_ctrlTabCtrl,IDC_IN_CONFLICT))
	{
		TRACE0("Failed to create output change files window\n");
		return FALSE;      // fail to create
	}
	m_ctrlTabCtrl.InsertTab(&m_ConflictFileList, CString(MAKEINTRESOURCE(IDS_PROC_SYNC_CONFLICTS)), -1);

	m_ConflictFileList.Init(GITSLC_COLEXT | GITSLC_COLSTATUS |GITSLC_COLADD|GITSLC_COLDEL, L"ConflictSyncDlg",
							(GITSLC_POPEXPLORE | GITSLC_POPOPEN | GITSLC_POPSHOWLOG |
							GITSLC_POPCONFLICT|GITSLC_POPRESOLVE),false);


	//----------  Create Commit Out List Ctrl---------------

	dwStyle =LVS_REPORT | LVS_SHOWSELALWAYS | LVS_ALIGNLEFT | LVS_OWNERDATA | WS_BORDER | WS_TABSTOP | WS_CHILD | WS_VISIBLE;;

	if( !m_OutLogList.Create(dwStyle,rectDummy,&m_ctrlTabCtrl,IDC_OUT_LOGLIST))
	{
		TRACE0("Failed to create output commits window\n");
		return FALSE;      // fail to create

	}
	// for some unknown reason, the SetExtendedStyle in OnCreate/PreSubclassWindow is not working here
	m_OutLogList.SetStyle();

	m_ctrlTabCtrl.InsertTab(&m_OutLogList, CString(MAKEINTRESOURCE(IDS_PROC_SYNC_OUTCOMMITS)), -1);

	m_OutLogList.m_ColumnRegKey = L"SyncOut";
	m_OutLogList.InsertGitColumn();

	//------------- Create Change File List Control ----------------

	dwStyle = LVS_REPORT | LVS_SHOWSELALWAYS | LVS_ALIGNLEFT | WS_BORDER | WS_TABSTOP | WS_CHILD | WS_VISIBLE;

	if( !m_OutChangeFileList.Create(dwStyle,rectDummy,&m_ctrlTabCtrl,IDC_OUT_CHANGELIST))
	{
		TRACE0("Failed to create output change files window\n");
		return FALSE;      // fail to create
	}
	m_ctrlTabCtrl.InsertTab(&m_OutChangeFileList, CString(MAKEINTRESOURCE(IDS_PROC_SYNC_OUTCHANGELIST)), -1);

	m_OutChangeFileList.Init(GITSLC_COLEXT | GITSLC_COLSTATUS | GITSLC_COLADD | GITSLC_COLDEL, L"OutSyncDlg",
							(CGitStatusListCtrl::GetContextMenuBit(CGitStatusListCtrl::IDGITLC_COMPARETWOREVISIONS) |
							CGitStatusListCtrl::GetContextMenuBit(CGitStatusListCtrl::IDGITLC_GNUDIFF2REVISIONS)), false, false, GITSLC_COLEXT | GITSLC_COLSTATUS | GITSLC_COLADD | GITSLC_COLDEL);

	dwStyle = LVS_REPORT | LVS_SHOWSELALWAYS | LVS_ALIGNLEFT | WS_BORDER | WS_TABSTOP | LVS_SINGLESEL | WS_CHILD | WS_VISIBLE;
	if (!m_GitProgressList.Create(dwStyle | LVS_OWNERDATA, rectDummy, &m_ctrlTabCtrl, IDC_CMD_GIT_PROG))
	{
		TRACE0("Failed to create Git Progress List Window\n");
		return FALSE;      // fail to create
	}
	m_ctrlTabCtrl.InsertTab(&m_GitProgressList, CString(MAKEINTRESOURCE(IDS_LOG)), -1);
	m_GitProgressList.m_pAnimate = &m_ctrlAnimate;
	m_GitProgressList.m_pPostWnd = this;
	m_GitProgressList.m_pProgressLabelCtrl = &m_ctrlProgLabel;
	m_GitProgressList.m_pProgControl = &m_ctrlProgress;
	m_GitProgressList.m_pTaskbarList = m_pTaskbarList;

	dwStyle = LVS_REPORT | LVS_SHOWSELALWAYS | LVS_ALIGNLEFT | WS_BORDER | WS_TABSTOP | WS_CHILD | WS_VISIBLE | LVS_SINGLESEL;
	DWORD exStyle = LVS_EX_HEADERDRAGDROP | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP;
	if (CRegDWORD(L"Software\\TortoiseGit\\FullRowSelect", TRUE))
		exStyle |= LVS_EX_FULLROWSELECT;
	if (g_Git.m_IsUseLibGit2)
	{
		m_refList.Create(dwStyle, rectDummy, &m_ctrlTabCtrl, IDC_REFLIST);
		m_refList.SetExtendedStyle(exStyle);
		m_refList.Init();
		m_ctrlTabCtrl.InsertTab(&m_refList, CString(MAKEINTRESOURCE(IDS_REFLIST)), -1);

		m_tagCompareList.Create(dwStyle, rectDummy, &m_ctrlTabCtrl, IDC_TAGCOMPARELIST);
		m_tagCompareList.SetExtendedStyle(exStyle);
		m_tagCompareList.Init();
		m_ctrlTabCtrl.InsertTab(&m_tagCompareList, CString(MAKEINTRESOURCE(IDS_PROC_SYNC_COMPARETAGS)), -1);
	}
	m_ProjectProperties.ReadProps();

	AdjustControlSize(IDC_CHECK_PUTTY_KEY);
	AdjustControlSize(IDC_CHECK_FORCE);

	AddAnchor(IDC_SYNC_TAB,TOP_LEFT,BOTTOM_RIGHT);

	AddAnchor(IDC_GROUP_INFO,TOP_LEFT,TOP_RIGHT);
	AddAnchor(IDC_COMBOBOXEX_URL,TOP_LEFT,TOP_RIGHT);
	AddAnchor(IDC_BUTTON_MANAGE,TOP_RIGHT);
	AddAnchor(IDC_BUTTON_PULL,BOTTOM_LEFT);
	AddAnchor(IDC_BUTTON_PUSH,BOTTOM_LEFT);
	AddAnchor(IDC_BUTTON_SUBMODULE,BOTTOM_LEFT);
	AddAnchor(IDC_BUTTON_STASH,BOTTOM_LEFT);
	AddAnchor(IDC_BUTTON_APPLY,BOTTOM_RIGHT);
	AddAnchor(IDC_BUTTON_EMAIL,BOTTOM_RIGHT);
	AddAnchor(IDC_PROGRESS_SYNC,TOP_LEFT,TOP_RIGHT);
	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDHELP,BOTTOM_RIGHT);
	AddAnchor(IDC_STATIC_STATUS, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_ANIMATE_SYNC,TOP_LEFT);
	AddAnchor(IDC_BUTTON_COMMIT,BOTTOM_LEFT);
	AddAnchor(IDC_LOG, BOTTOM_LEFT);

	// do not use BRANCH_COMBOX_ADD_ANCHOR here, we want to have different stylings
	AddAnchor(IDC_COMBOBOXEX_LOCAL_BRANCH, TOP_LEFT,TOP_CENTER);
	AddAnchor(IDC_COMBOBOXEX_REMOTE_BRANCH, TOP_CENTER, TOP_RIGHT);
	AddAnchor(IDC_BUTTON_LOCAL_BRANCH, TOP_CENTER);
	AddAnchor(IDC_BUTTON_REMOTE_BRANCH, TOP_RIGHT);
	AddAnchor(IDC_STATIC_REMOTE_BRANCH, TOP_CENTER);
	AddAnchor(IDC_PROG_LABEL, TOP_LEFT);

	CString WorkingDir=g_Git.m_CurrentDir;
	WorkingDir.Replace(L':', L'_');
	m_RegKeyRemoteBranch = L"Software\\TortoiseGit\\History\\SyncBranch\\" + WorkingDir;


	this->AddOthersToAnchor();

	this->m_ctrlPush.AddEntry(CString(MAKEINTRESOURCE(IDS_PROC_SYNC_PUSH)));
	this->m_ctrlPush.AddEntry(CString(MAKEINTRESOURCE(IDS_PROC_SYNC_PUSHTAGS)));
	this->m_ctrlPush.AddEntry(CString(MAKEINTRESOURCE(IDS_PROC_SYNC_PUSHNOTES)));

	this->m_ctrlPull.AddEntry(CString(MAKEINTRESOURCE(IDS_PROC_SYNC_PULL)));
	this->m_ctrlPull.AddEntry(CString(MAKEINTRESOURCE(IDS_PROC_SYNC_FETCH)));
	this->m_ctrlPull.AddEntry(CString(MAKEINTRESOURCE(IDS_PROC_SYNC_FETCHREBASE)));
	this->m_ctrlPull.AddEntry(CString(MAKEINTRESOURCE(IDS_PROC_SYNC_FETCHALL)));
	this->m_ctrlPull.AddEntry(CString(MAKEINTRESOURCE(IDS_PROC_SYNC_REMOTEUPDATE)));
	this->m_ctrlPull.AddEntry(CString(MAKEINTRESOURCE(IDS_PROC_SYNC_CLEANUPSTALEBRANCHES)));
	this->m_ctrlPull.AddEntry(CString(MAKEINTRESOURCE(IDS_PROC_SYNC_COMPARETAGS)));

	this->m_ctrlSubmodule.AddEntry(CString(MAKEINTRESOURCE(IDS_PROC_SYNC_SUBKODULEUPDATE)));
	this->m_ctrlSubmodule.AddEntry(CString(MAKEINTRESOURCE(IDS_PROC_SYNC_SUBKODULEINIT)));
	this->m_ctrlSubmodule.AddEntry(CString(MAKEINTRESOURCE(IDS_PROC_SYNC_SUBKODULESYNC)));

	this->m_ctrlStash.AddEntry(CString(MAKEINTRESOURCE(IDS_MENUSTASHSAVE)));
	this->m_ctrlStash.AddEntry(CString(MAKEINTRESOURCE(IDS_MENUSTASHPOP)));
	this->m_ctrlStash.AddEntry(CString(MAKEINTRESOURCE(IDS_MENUSTASHAPPLY)));

	WorkingDir.Replace(L':', L'_');

	CString regkey ;
	regkey.Format(L"Software\\TortoiseGit\\TortoiseProc\\Sync\\%s", static_cast<LPCTSTR>(WorkingDir));

	this->m_regPullButton = CRegDWORD(regkey + L"\\Pull", 0);
	this->m_regPushButton = CRegDWORD(regkey + L"\\Push", 0);
	this->m_regSubmoduleButton = CRegDWORD(regkey + L"\\Submodule");
	this->m_regAutoLoadPutty = CRegDWORD(regkey + L"\\AutoLoadPutty", CAppUtils::IsSSHPutty());

	this->UpdateData();
	this->m_bAutoLoadPuttyKey  = m_regAutoLoadPutty;
	if(!CAppUtils::IsSSHPutty())
		m_bAutoLoadPuttyKey = false;
	this->UpdateData(FALSE);

	this->m_ctrlPull.SetCurrentEntry(this->m_regPullButton);
	this->m_ctrlPush.SetCurrentEntry(this->m_regPushButton);
	this->m_ctrlSubmodule.SetCurrentEntry(this->m_regSubmoduleButton);

	CString sWindowTitle;
	GetWindowText(sWindowTitle);
	CAppUtils::SetWindowTitle(m_hWnd, g_Git.m_CurrentDir, sWindowTitle);

	EnableSaveRestore(L"SyncDlg");

	m_ctrlURL.SetCaseSensitive(TRUE);

	m_ctrlURL.SetCustomAutoSuggest(true, true, true);
	m_ctrlURL.SetMaxHistoryItems(0x7FFFFFFF);
	this->m_ctrlURL.LoadHistory(L"Software\\TortoiseGit\\History\\SyncURL\\" + WorkingDir, L"url");

	m_remotelist.clear();
	if(!g_Git.GetRemoteList(m_remotelist))
	{
		for (unsigned int i = 0; i < m_remotelist.size(); ++i)
		{
			m_ctrlURL.AddString(m_remotelist[i]);
		}
	}
	m_ctrlURL.SetCurSel(0);
	m_ctrlRemoteBranch.SetCurSel(0);

	this->LoadBranchInfo();

	this->m_bInited=true;
	FetchOutList();

	m_ctrlTabCtrl.ShowTab(IDC_CMD_LOG-1,false);
	m_ctrlTabCtrl.ShowTab(IDC_IN_LOGLIST-1,false);
	m_ctrlTabCtrl.ShowTab(IDC_IN_CHANGELIST-1,false);
	m_ctrlTabCtrl.ShowTab(IDC_IN_CONFLICT-1,false);
	m_ctrlTabCtrl.ShowTab(IDC_CMD_GIT_PROG-1, false);
	m_ctrlTabCtrl.ShowTab(IDC_REFLIST-1, false);
	m_ctrlTabCtrl.ShowTab(IDC_TAGCOMPARELIST - 1, false);

	m_ctrlRemoteBranch.m_bWantReturn = TRUE;
	m_ctrlURL.m_bWantReturn = TRUE;

	if (m_seq > 0 && static_cast<DWORD>(CRegDWORD(L"Software\\TortoiseGit\\SyncDialogRandomPos")))
	{
		m_seq %= 5;
		RECT rect;
		GetWindowRect(&rect);
		rect.top -= m_seq * 30;
		rect.bottom -= m_seq * 30;
		if (rect.top < 0)
		{
			rect.top += 150;
			rect.bottom += 150;
		}
		MoveWindow(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CSyncDlg::OnBnClickedButtonManage()
{
	CAppUtils::LaunchRemoteSetting();
	Refresh();
}

void CSyncDlg::Refresh()
{
	theApp.DoWaitCursor(1);

	int lastSelected = m_ctrlURL.GetCurSel();
	CString url;
	this->m_ctrlURL.GetWindowText(url);

	this->m_ctrlURL.Reset();
	CString workingDir = g_Git.m_CurrentDir;
	workingDir.Replace(L':', L'_');
	this->m_ctrlURL.LoadHistory(L"Software\\TortoiseGit\\History\\SyncURL\\" + workingDir, L"url");

	bool found = false;
	m_remotelist.clear();
	if (!g_Git.GetRemoteList(m_remotelist))
	{
		for (size_t i = 0; i < m_remotelist.size(); ++i)
		{
			m_ctrlURL.AddString(m_remotelist[i]);
			if (m_remotelist[i] == url)
				found = true;
		}
	}
	if (lastSelected >= 0 && !found)
	{
		m_ctrlURL.SetCurSel(0);
		m_ctrlURL.GetWindowText(url);
	}

	CString local;
	CString remote;
	this->m_ctrlLocalBranch.GetWindowText(local);
	this->m_ctrlRemoteBranch.GetWindowText(remote);

	this->LoadBranchInfo();

	this->m_ctrlLocalBranch.AddString(local);
	this->m_ctrlRemoteBranch.AddString(remote);
	this->m_ctrlURL.AddString(url);

	m_OutLogList.ShowText(CString(MAKEINTRESOURCE(IDS_PROC_SYNC_REFRESHING)));
	this->FetchOutList(true);
	theApp.DoWaitCursor(-1);
}

BOOL CSyncDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case VK_F5:
			{
				if (m_bBlock)
					return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
				Refresh();
			}
		break;

		/* Avoid TAB control destroy but dialog exist*/
		case VK_ESCAPE:
		case VK_CANCEL:
			{
				TCHAR buff[129];
				::GetClassName(pMsg->hwnd, buff, _countof(buff) - 1);

				/* Use MSFTEDIT_CLASS http://msdn.microsoft.com/en-us/library/bb531344.aspx */
				if (_wcsnicmp(buff, MSFTEDIT_CLASS, _countof(buff) - 1) == 0 ||	//Unicode and MFC 2012 and later
					_wcsnicmp(buff, RICHEDIT_CLASS, _countof(buff) - 1) == 0 ||	//ANSI or MFC 2010
					_wcsnicmp(buff, L"SysListView32", _countof(buff) - 1) == 0)
				{
					this->PostMessage(WM_KEYDOWN,VK_ESCAPE,0);
					return TRUE;
				}
			}
		}
	}
	return __super::PreTranslateMessage(pMsg);
}
void CSyncDlg::FetchOutList(bool force)
{
	if (!m_bInited || m_bWantToExit)
		return;
	m_OutChangeFileList.Clear();
	this->m_OutLogList.Clear();

	m_ctrlTabCtrl.ShowTab(IDC_OUT_LOGLIST - 1, true);
	m_ctrlTabCtrl.ShowTab(IDC_OUT_CHANGELIST - 1, true);

	CString remote;
	this->m_ctrlURL.GetWindowText(remote);
	CString remotebranch;
	this->m_ctrlRemoteBranch.GetWindowText(remotebranch);
	remotebranch = remote + L'/' + remotebranch;
	CGitHash remotebranchHash;
	g_Git.GetHash(remotebranchHash, remotebranch);

	if(IsURL())
	{
		CString str;
		str.LoadString(IDS_PROC_SYNC_PUSH_UNKNOWN);
		m_OutLogList.ShowText(str);
		this->m_ctrlTabCtrl.ShowTab(m_OutChangeFileList.GetDlgCtrlID()-1,FALSE);
		m_OutLocalBranch.Empty();
		m_OutRemoteBranch.Empty();

		this->GetDlgItem(IDC_BUTTON_EMAIL)->EnableWindow(FALSE);
		return ;

	}
	else if(remotebranchHash.IsEmpty())
	{
		CString str;
		str.Format(IDS_PROC_SYNC_PUSH_UNKNOWNBRANCH, static_cast<LPCTSTR>(remotebranch));
		m_OutLogList.ShowText(str);
		this->m_ctrlTabCtrl.ShowTab(m_OutChangeFileList.GetDlgCtrlID()-1,FALSE);
		m_OutLocalBranch.Empty();
		m_OutRemoteBranch.Empty();

		this->GetDlgItem(IDC_BUTTON_EMAIL)->EnableWindow(FALSE);
		return ;
	}
	else
	{
		CString localbranch;
		localbranch=this->m_ctrlLocalBranch.GetString();

		if(localbranch != m_OutLocalBranch || m_OutRemoteBranch != remotebranch || force)
		{
			m_OutLogList.ClearText();

			CGitHash base, localBranchHash;
			bool isFastForward = g_Git.IsFastForward(remotebranch, localbranch, &base);

			if (g_Git.GetHash(localBranchHash, localbranch))
			{
				MessageBox(g_Git.GetGitLastErr(L"Could not get hash of \"" + localbranch + L"\"."), L"TortoiseGit", MB_ICONERROR);
				return;
			}
			if (remotebranchHash == localBranchHash)
			{
				CString str;
				str.FormatMessage(IDS_PROC_SYNC_COMMITSAHEAD, 0, static_cast<LPCTSTR>(remotebranch));
				m_OutLogList.ShowText(str);
				this->m_ctrlStatus.SetWindowText(str);
				this->m_ctrlTabCtrl.ShowTab(m_OutChangeFileList.GetDlgCtrlID()-1,FALSE);
				this->GetDlgItem(IDC_BUTTON_EMAIL)->EnableWindow(FALSE);
			}
			else if (isFastForward || m_bForce)
			{
				CString range;
				range.Format(L"%s..%s", static_cast<LPCTSTR>(g_Git.FixBranchName(remotebranch)), static_cast<LPCTSTR>(g_Git.FixBranchName(localbranch)));
				//fast forward
				m_OutLogList.FillGitLog(nullptr, &range, CGit::LOG_INFO_STAT | CGit::LOG_INFO_FILESTATE | CGit::LOG_INFO_SHOW_MERGEDFILE);
				CString str;
				str.FormatMessage(IDS_PROC_SYNC_COMMITSAHEAD, m_OutLogList.GetItemCount(), static_cast<LPCTSTR>(remotebranch));
				this->m_ctrlStatus.SetWindowText(str);

				if (isFastForward)
					AddDiffFileList(&m_OutChangeFileList, &m_arOutChangeList, localBranchHash, remotebranchHash);
				else
				{
					AddDiffFileList(&m_OutChangeFileList, &m_arOutChangeList, localBranchHash, base);
				}

				this->m_ctrlTabCtrl.ShowTab(m_OutChangeFileList.GetDlgCtrlID()-1,TRUE);
				this->GetDlgItem(IDC_BUTTON_EMAIL)->EnableWindow(TRUE);
			}
			else
			{
				CString str;
				str.FormatMessage(IDS_PROC_SYNC_NOFASTFORWARD, static_cast<LPCTSTR>(localbranch), static_cast<LPCTSTR>(remotebranch));
				m_OutLogList.ShowText(str);
				this->m_ctrlStatus.SetWindowText(str);
				this->m_ctrlTabCtrl.ShowTab(m_OutChangeFileList.GetDlgCtrlID() - 1, FALSE);
				this->GetDlgItem(IDC_BUTTON_EMAIL)->EnableWindow(FALSE);
			}
		}
		this->m_OutLocalBranch=localbranch;
		this->m_OutRemoteBranch=remotebranch;
	}
}

bool CSyncDlg::IsURL()
{
	CString str;
	this->m_ctrlURL.GetWindowText(str);
	return str.Find(L'\\') >= 0 || str.Find(L'/') >= 0;
}

void CSyncDlg::OnCbnEditchangeComboboxex()
{
	SetTimer(IDT_INPUT, 1000, nullptr);
	this->m_OutLogList.ShowText(CString(MAKEINTRESOURCE(IDS_PROC_SYNC_WAINTINPUT)));

	//this->FetchOutList();
}

UINT CSyncDlg::ProgressThread()
{
	m_startTick = GetTickCount64();
	m_bDone = false;
	STRING_VECTOR list;
	CProgressDlg::RunCmdList(this, m_GitCmdList, list, true, nullptr, &this->m_bAbort, &this->m_Databuf);
	InterlockedExchange(&m_bBlock, FALSE);
	return 0;
}

LRESULT CSyncDlg::OnProgressUpdateUI(WPARAM wParam,LPARAM lParam)
{
	if (m_bWantToExit)
		return 0;
	if(wParam == MSG_PROGRESSDLG_START)
	{
		m_BufStart = 0;
		m_ctrlAnimate.Play(0, UINT_MAX, UINT_MAX);
		this->m_ctrlProgress.SetPos(0);
		if (m_pTaskbarList)
		{
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NORMAL);
			m_pTaskbarList->SetProgressValue(m_hWnd, 0, 100);
		}
	}

	if(wParam == MSG_PROGRESSDLG_END || wParam == MSG_PROGRESSDLG_FAILED)
	{
		ULONGLONG tickSpent = GetTickCount64() - m_startTick;
		CString strEndTime = CLoglistUtils::FormatDateAndTime(CTime::GetCurrentTime(), DATE_SHORTDATE, true, false);

		m_BufStart = 0;
		m_Databuf.m_critSec.Lock();
		m_Databuf.clear();
		m_Databuf.m_critSec.Unlock();

		m_bDone = true;
		m_ctrlAnimate.Stop();
		m_ctrlProgress.SetPos(100);
		//this->DialogEnableWindow(IDOK,TRUE);

		{
			CString text;
			m_ctrlCmdOut.GetWindowText(text);
			text.Remove('\r');
			CAppUtils::StyleURLs(text, &m_ctrlCmdOut);
		}

		auto exitCode = static_cast<DWORD>(lParam);
		if (exitCode)
		{
			if (m_pTaskbarList)
			{
				m_pTaskbarList->SetProgressState(m_hWnd, TBPF_ERROR);
				m_pTaskbarList->SetProgressValue(m_hWnd, 100, 100);
			}
			CString log;
			log.Format(IDS_PROC_PROGRESS_GITUNCLEANEXIT, exitCode);
			CString err;
			err.Format(L"\r\n\r\n%s (%I64u ms @ %s)\r\n", static_cast<LPCTSTR>(log), tickSpent, static_cast<LPCTSTR>(strEndTime));
			CProgressDlg::InsertColorText(this->m_ctrlCmdOut, err, RGB(255,0,0));
			if (CRegDWORD(L"Software\\TortoiseGit\\NoSounds", FALSE) == FALSE)
				PlaySound(reinterpret_cast<LPCTSTR>(SND_ALIAS_SYSTEMEXCLAMATION), nullptr, SND_ALIAS_ID | SND_ASYNC);
		}
		else
		{
			if (m_pTaskbarList)
				m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NOPROGRESS);
			CString temp;
			temp.LoadString(IDS_SUCCESS);
			CString log;
			log.Format(L"\r\n%s (%I64u ms @ %s)\r\n", static_cast<LPCTSTR>(temp), tickSpent, static_cast<LPCTSTR>(strEndTime));
			CProgressDlg::InsertColorText(this->m_ctrlCmdOut, log, RGB(0,0,255));
		}
		m_GitCmdStatus = exitCode;

		//if(wParam == MSG_PROGRESSDLG_END)
		RunPostAction();
	}

	if(lParam != 0)
		ParserCmdOutput(static_cast<char>(lParam));
	else
	{
		m_Databuf.m_critSec.Lock();
		for (size_t i = m_BufStart; i < m_Databuf.size(); ++i)
		{
			char c = m_Databuf[m_BufStart];
			++m_BufStart;
			m_Databuf.m_critSec.Unlock();
			ParserCmdOutput(c);

			m_Databuf.m_critSec.Lock();
		}

		if (m_BufStart > 1000)
		{
			m_Databuf.erase(m_Databuf.cbegin(), m_Databuf.cbegin() + m_BufStart);
			m_BufStart = 0;
		}
		m_Databuf.m_critSec.Unlock();
	}

	return 0;
}

static REF_VECTOR HashMapToRefMap(MAP_HASH_NAME& map)
{
	auto rmap = REF_VECTOR();
	for (auto mit = map.cbegin(); mit != map.cend(); ++mit)
	{
		for (auto rit = mit->second.cbegin(); rit != mit->second.cend(); ++rit)
		{
			rmap.emplace_back(TGitRef{ *rit, mit->first });
		}
	}
	return rmap;
}

void CSyncDlg::FillNewRefMap()
{
	m_refList.Clear();
	m_newHashMap.clear();

	if (!g_Git.m_IsUseLibGit2)
		return;

	CAutoRepository repo(g_Git.GetGitRepository());
	if (!repo)
	{
		CMessageBox::Show(m_hWnd, CGit::GetLibGit2LastErr(L"Could not open repository."), L"TortoiseGit", MB_OK | MB_ICONERROR);
		return;
	}

	if (CGit::GetMapHashToFriendName(repo, m_newHashMap))
	{
		MessageBox(CGit::GetLibGit2LastErr(L"Could not get all refs."), L"TortoiseGit", MB_ICONERROR);
		return;
	}

	auto oldRefMap = HashMapToRefMap(m_oldHashMap);
	auto newRefMap = HashMapToRefMap(m_newHashMap);
	for (auto oit = oldRefMap.cbegin(); oit != oldRefMap.cend(); ++oit)
	{
		bool found = false;
		for (auto nit = newRefMap.cbegin(); nit != newRefMap.cend(); ++nit)
		{
			// changed ref
			if (oit->name == nit->name)
			{
				found = true;
				m_refList.AddEntry(repo, oit->name, &oit->hash, &nit->hash);
				break;
			}
		}
		// deleted ref
		if (!found)
			m_refList.AddEntry(repo, oit->name, &oit->hash, nullptr);
	}
	for (auto nit = newRefMap.cbegin(); nit != newRefMap.cend(); ++nit)
	{
		bool found = false;
		for (auto oit = oldRefMap.cbegin(); oit != oldRefMap.cend(); ++oit)
		{
			if (oit->name == nit->name)
			{
				found = true;
				break;
			}
		}
		// new ref
		if (!found)
			m_refList.AddEntry(repo, nit->name, nullptr, &nit->hash);
	}
	m_refList.Show();
}

void CSyncDlg::RunPostAction()
{
	if (m_bWantToExit)
		return;

	FillNewRefMap();

	if (this->m_CurrentCmd == GIT_COMMAND_PUSH)
	{
		DWORD exitcode = 0xFFFFFFFF;
		CString error;
		CHooks::Instance().SetProjectProperties(g_Git.m_CurrentDir, m_ProjectProperties);
		if (CHooks::Instance().PostPush(GetSafeHwnd(), g_Git.m_CurrentDir, exitcode, error))
		{
			if (exitcode)
			{
				CString temp;
				temp.Format(IDS_ERR_HOOKFAILED, static_cast<LPCTSTR>(error));
				CMessageBox::Show(GetSafeHwnd(), temp, L"TortoiseGit", MB_OK | MB_ICONERROR);
				return;
			}
		}

		EnableControlButton(true);
		SwitchToInput();
		this->FetchOutList(true);
	}
	else if (this->m_CurrentCmd == GIT_COMMAND_PULL)
		PullComplete();
	else if (this->m_CurrentCmd == GIT_COMMAND_FETCH || this->m_CurrentCmd == GIT_COMMAND_FETCHANDREBASE)
		FetchComplete();
	else if (this->m_CurrentCmd == GIT_COMMAND_SUBMODULE)
	{
		//this->m_ctrlCmdOut.SetSel(-1,-1);
		//this->m_ctrlCmdOut.ReplaceSel(L"Done\r\n");
		//this->m_ctrlCmdOut.SetSel(-1,-1);
		EnableControlButton(true);
		SwitchToInput();
	}
	else if (this->m_CurrentCmd == GIT_COMMAND_STASH)
		StashComplete();
	else if (this->m_CurrentCmd == GIT_COMMAND_REMOTE)
	{
		this->FetchOutList(true);
		EnableControlButton(true);
		SwitchToInput();
		ShowTab(IDC_REFLIST);
	}
}
void CSyncDlg::ParserCmdOutput(char ch)
{
	if (m_bAbort)
		return;
	CProgressDlg::ParserCmdOutput(m_ctrlCmdOut,m_ctrlProgress,m_hWnd,m_pTaskbarList,m_LogText,ch);
}
void CSyncDlg::OnBnClickedButtonCommit()
{
	CString cmd = L"/command:commit";
	cmd += L" /path:\"";
	cmd += g_Git.m_CurrentDir;
	cmd += L'"';

	CAppUtils::RunTortoiseGitProc(cmd);
}

void CSyncDlg::OnOK()
{
	UpdateCombox();
	this->UpdateData();
	m_ctrlURL.SaveHistory();
	SaveHistory();
	m_regAutoLoadPutty = this->m_bAutoLoadPuttyKey;
	m_tooltips.Pop();
	__super::OnOK();
}

void CSyncDlg::OnCancel()
{
	m_bAbort = true;
	m_GitProgressList.Cancel();
	if (m_bDone && !m_GitProgressList.IsRunning())
	{
		CResizableStandAloneDialog::OnCancel();
		return;
	}
	if (m_GitProgressList.IsRunning())
		WaitForSingleObject(m_GitProgressList.m_pThread->m_hThread, 10000);

	if (g_Git.m_CurrentGitPi.hProcess)
	{
		DWORD dwConfirmKillProcess = CRegDWORD(L"Software\\TortoiseGit\\ConfirmKillProcess");
		if (dwConfirmKillProcess && CMessageBox::Show(m_hWnd, IDS_PROC_CONFIRMKILLPROCESS, IDS_APPNAME, MB_YESNO | MB_ICONQUESTION) != IDYES)
			return;
		if (::GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0))
			::WaitForSingleObject(g_Git.m_CurrentGitPi.hProcess, 10000);

		CProgressDlg::KillProcessTree(g_Git.m_CurrentGitPi.dwProcessId);
	}

	::WaitForSingleObject(g_Git.m_CurrentGitPi.hProcess ,10000);
	if (m_pThread)
	{
		if (::WaitForSingleObject(m_pThread->m_hThread, 5000) == WAIT_TIMEOUT)
			g_Git.KillRelatedThreads(m_pThread);
	}
	m_tooltips.Pop();
	CResizableStandAloneDialog::OnCancel();
}

void CSyncDlg::OnBnClickedButtonSubmodule()
{
	bool bShift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
	this->UpdateData();
	UpdateCombox();

	if (bShift)
	{
		switch (m_ctrlSubmodule.GetCurrentEntry())
		{
		case 0:
		case 1: // fall-through
			CAppUtils::RunTortoiseGitProc(L"/command:subupdate /bkpath:\"" + g_Git.m_CurrentDir + L"\"");
			break;
		case 2:
			CAppUtils::RunTortoiseGitProc(L"/command:subsync /bkpath:\"" + g_Git.m_CurrentDir + L"\"");
			break;
		}
		return;
	}

	m_ctrlCmdOut.SetWindowText(L"");
	m_LogText.Empty();

	this->m_regSubmoduleButton = static_cast<DWORD>(this->m_ctrlSubmodule.GetCurrentEntry());

	this->SwitchToRun();

	this->m_bAbort=false;
	this->m_GitCmdList.clear();

	ShowTab(IDC_CMD_LOG);

	CString cmd;

	switch (m_ctrlSubmodule.GetCurrentEntry())
	{
	case 0:
		cmd = L"git.exe submodule update --init";
		break;
	case 1:
		cmd = L"git.exe submodule init";
		break;
	case 2:
		cmd = L"git.exe submodule sync";
		break;
	}

	m_GitCmdList.push_back(cmd);

	m_CurrentCmd = GIT_COMMAND_SUBMODULE;

	StartWorkerThread();
}

void CSyncDlg::OnBnClickedButtonStash()
{
	bool bShift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
	UpdateData();
	UpdateCombox();

	if (bShift)
	{
		if (m_ctrlStash.GetCurrentEntry() == 0)
			CAppUtils::RunTortoiseGitProc(L"/command:stashsave");
		return;
	}

	m_ctrlCmdOut.SetWindowText(L"");
	m_LogText.Empty();

	SwitchToRun();

	m_bAbort = false;
	m_GitCmdList.clear();

	ShowTab(IDC_CMD_LOG);

	m_ctrlTabCtrl.ShowTab(IDC_IN_LOGLIST - 1, false);
	m_ctrlTabCtrl.ShowTab(IDC_IN_CHANGELIST -1, false);
	m_ctrlTabCtrl.ShowTab(IDC_IN_CONFLICT -1, false);

	CString cmd;
	switch (m_ctrlStash.GetCurrentEntry())
	{
	case 0:
		cmd = L"git.exe stash push";
		if (!CAppUtils::IsGitVersionNewerOrEqual(GetSafeHwnd(), 2, 14))
			cmd = L"git.exe stash save";
		break;
	case 1:
		cmd = L"git.exe stash pop";
		break;
	case 2:
		cmd = L"git.exe stash apply";
		break;
	}

	m_GitCmdList.push_back(cmd);
	m_CurrentCmd = GIT_COMMAND_STASH;

	StartWorkerThread();
}

void CSyncDlg::OnTimer(UINT_PTR nIDEvent)
{
	if( nIDEvent == IDT_INPUT)
	{
		KillTimer(IDT_INPUT);
		this->FetchOutList(true);
		m_ctrlTabCtrl.ShowTab(IDC_TAGCOMPARELIST - 1, false);
	}
}

LRESULT CSyncDlg::OnTaskbarBtnCreated(WPARAM wParam, LPARAM lParam)
{
	m_pTaskbarList.Release();
	m_pTaskbarList.CoCreateInstance(CLSID_TaskbarList);
	m_GitProgressList.m_pTaskbarList = m_pTaskbarList;
	return __super::OnTaskbarButtonCreated(wParam, lParam);
}

void CSyncDlg::OnBnClickedCheckForce()
{
	UpdateData();
}

void CSyncDlg::OnBnClickedLog()
{
	CString cmd = L"/command:log";
	cmd += L" /path:\"";
	cmd += g_Git.m_CurrentDir;
	cmd += L'"';

	CAppUtils::RunTortoiseGitProc(cmd);
}

LRESULT CSyncDlg::OnProgCmdFinish(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	RefreshCursor();
	RunPostAction();
	return 0;
}

void CSyncDlg::OnDestroy()
{
	m_bWantToExit = true;
	__super::OnDestroy();
}

LRESULT CSyncDlg::OnThemeChanged()
{
	CMFCVisualManager::GetInstance()->DestroyInstance();
	return 0;
}

void CSyncDlg::OnEnLinkLog(NMHDR *pNMHDR, LRESULT *pResult)
{
	// similar code in ProgressDlg.cpp and LogDlg.cpp
	ENLINK *pEnLink = reinterpret_cast<ENLINK *>(pNMHDR);
	if ((pEnLink->msg == WM_LBUTTONUP) || (pEnLink->msg == WM_SETCURSOR))
	{
		CString msg;
		m_ctrlCmdOut.GetWindowText(msg);
		msg.Replace(L"\r\n", L"\n");
		CString url = msg.Mid(pEnLink->chrg.cpMin, pEnLink->chrg.cpMax - pEnLink->chrg.cpMin);
		// check if it's an email address
		auto atpos = url.Find(L'@');
		if ((atpos > 0) && (url.ReverseFind(L'.') > atpos) && !::PathIsURL(url))
			url = L"mailto:" + url;
		if (::PathIsURL(url))
		{
			if (pEnLink->msg == WM_LBUTTONUP)
				ShellExecute(GetSafeHwnd(), L"open", url, nullptr, nullptr, SW_SHOWDEFAULT);
			else
			{
				static RECT prevRect = { 0 };
				CWnd* pMsgView = &m_ctrlCmdOut;
				if (pMsgView)
				{
					RECT rc;
					POINTL pt;
					pMsgView->SendMessage(EM_POSFROMCHAR, reinterpret_cast<WPARAM>(&pt), pEnLink->chrg.cpMin);
					rc.left = pt.x;
					rc.top = pt.y;
					pMsgView->SendMessage(EM_POSFROMCHAR, reinterpret_cast<WPARAM>(&pt), pEnLink->chrg.cpMax);
					rc.right = pt.x;
					rc.bottom = pt.y + 12;
					if ((prevRect.left != rc.left) || (prevRect.top != rc.top))
					{
						m_tooltips.DelTool(pMsgView, 1);
						m_tooltips.AddTool(pMsgView, url, &rc, 1);
						prevRect = rc;
					}
				}
			}
		}
	}
	*pResult = 0;
}

void CSyncDlg::OnEnscrollLog()
{
	m_tooltips.DelTool(&m_ctrlCmdOut, 1);
}

void CSyncDlg::StartWorkerThread()
{
	if (InterlockedExchange(&m_bBlock, TRUE))
		return;

	m_pThread = AfxBeginThread(ProgressThreadEntry, this, THREAD_PRIORITY_NORMAL);
	if (!m_pThread)
	{
		InterlockedExchange(&m_bBlock, FALSE);
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
		SwitchToInput();
		EnableControlButton(true);
	}
}
