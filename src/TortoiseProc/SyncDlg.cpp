// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2014 - TortoiseGit

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
#include "progressdlg.h"
#include "MessageBox.h"
#include "ImportPatchDlg.h"
#include "RebaseDlg.h"
#include "hooks.h"
#include "SmartHandle.h"
#include "ProgressCommands/FetchProgressCommand.h"

// CSyncDlg dialog

IMPLEMENT_DYNAMIC(CSyncDlg, CResizableStandAloneDialog)

CSyncDlg::CSyncDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CSyncDlg::IDD, pParent)
{
	m_CurrentCmd = 0;
	m_pTooltip=&this->m_tooltips;
	m_bInited=false;
	m_CmdOutCurrentPos=0;
	m_bAutoLoadPuttyKey = CAppUtils::IsSSHPutty();
	m_bForce=false;
	m_Gitverion = 0;
	m_bBlock = false;
	m_BufStart = 0;
	m_pThread = NULL;
	m_bAbort = false;
	m_bDone = false;
	m_bWantToExit = false;
	m_GitCmdStatus = -1;
	m_startTick = GetTickCount();
	m_seq = 0;
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
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_IN_LOGLIST, OnLvnInLogListColumnClick)
	ON_BN_CLICKED(IDC_BUTTON_COMMIT, &CSyncDlg::OnBnClickedButtonCommit)
	ON_BN_CLICKED(IDC_BUTTON_SUBMODULE, &CSyncDlg::OnBnClickedButtonSubmodule)
	ON_BN_CLICKED(IDC_BUTTON_STASH, &CSyncDlg::OnBnClickedButtonStash)
	ON_WM_TIMER()
	ON_REGISTERED_MESSAGE(WM_TASKBARBTNCREATED, OnTaskbarBtnCreated)
	ON_BN_CLICKED(IDC_CHECK_FORCE, &CSyncDlg::OnBnClickedCheckForce)
	ON_BN_CLICKED(IDC_LOG, &CSyncDlg::OnBnClickedLog)
	ON_WM_DESTROY()
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

void CSyncDlg::OnBnClickedButtonPull()
{
	int CurrentEntry;
	CurrentEntry = (int)this->m_ctrlPull.GetCurrentEntry();
	this->m_regPullButton = CurrentEntry;

	this->m_bAbort=false;
	this->m_GitCmdList.clear();
	m_ctrlCmdOut.SetWindowTextW(_T(""));
	m_LogText = "";

	this->UpdateData();
	UpdateCombox();

	if (g_Git.GetHash(m_oldHash, _T("HEAD")))
	{
		MessageBox(g_Git.GetGitLastErr(_T("Could not get HEAD hash.")), _T("TortoiseGit"), MB_ICONERROR);
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
			MessageBox(g_Git.GetGitLastErr(_T("Could not get hash of \"") + m_strLocalBranch + _T("\".")), _T("TortoiseGit"), MB_ICONERROR);
			return;
		}
		if (localBranchHash != m_oldHash)
		{
			CMessageBox::Show(NULL, IDS_PROC_SYNC_PULLWRONGBRANCH, IDS_APPNAME, MB_OK | MB_ICONERROR);
			return;
		}
	}

	if(this->m_strURL.IsEmpty())
	{
		CMessageBox::Show(NULL, IDS_PROC_GITCONFIG_URLEMPTY, IDS_APPNAME, MB_OK | MB_ICONERROR);
		return;
	}

	if (m_bAutoLoadPuttyKey && CurrentEntry != 3) // CurrentEntry (Remote Update) handles this on its own)
	{
		CAppUtils::LaunchPAgent(NULL,&this->m_strURL);
	}

	this->SwitchToRun();
	if (g_Git.GetMapHashToFriendName(m_oldHashMap))
		MessageBox(g_Git.GetGitLastErr(_T("Could not get all refs.")), _T("TortoiseGit"), MB_ICONERROR);

	CString force;
	if(this->m_bForce)
		force = _T(" --force ");

	CString cmd;

	ShowTab(IDC_CMD_LOG);

	this->m_ctrlTabCtrl.ShowTab(IDC_REFLIST-1,true);
	this->m_ctrlTabCtrl.ShowTab(IDC_IN_LOGLIST-1,false);
	this->m_ctrlTabCtrl.ShowTab(IDC_IN_CHANGELIST-1,false);
	this->m_ctrlTabCtrl.ShowTab(IDC_IN_CONFLICT-1,false);

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

		if(m_Gitverion >= 0x01070203) //above 1.7.0.2
			force += _T("--progress ");

		cmd.Format(_T("git.exe pull -v %s \"%s\" %s"),
				force,
				m_strURL,
				remotebranch);

		m_CurrentCmd = GIT_COMMAND_PULL;
		m_GitCmdList.push_back(cmd);

		m_pThread = AfxBeginThread(ProgressThreadEntry, this, THREAD_PRIORITY_NORMAL,0,CREATE_SUSPENDED);
		if (m_pThread==NULL)
		{
		//		ReportError(CString(MAKEINTRESOURCE(IDS_ERR_THREADSTARTFAILED)));
		}
		else
		{
			m_pThread->m_bAutoDelete = TRUE;
			m_pThread->ResumeThread();
		}

	}

	///Fetch
	if(CurrentEntry == 1 || CurrentEntry ==2 ) //Fetch
	{
		CString remotebranch;
		if(this->IsURL() || m_strRemoteBranch.IsEmpty())
		{
			remotebranch=this->m_strRemoteBranch;

		}
		else
		{
			remotebranch.Format(_T("remotes/%s/%s"),
								m_strURL,m_strRemoteBranch);
			CGitHash remoteBranchHash;
			g_Git.GetHash(remoteBranchHash, remotebranch);
			if (remoteBranchHash.IsEmpty())
				remotebranch=m_strRemoteBranch;
			else
				remotebranch=m_strRemoteBranch+_T(":")+remotebranch;
		}

		if(CurrentEntry == 1)
			m_CurrentCmd = GIT_COMMAND_FETCH;
		else
			m_CurrentCmd = GIT_COMMAND_FETCHANDREBASE;

		if (g_Git.UsingLibGit2(CGit::GIT_CMD_FETCH))
		{
			CString refspec;
			// current libgit2 only supports well formated refspec
			refspec.Format(_T("refs/heads/%s:refs/remotes/%s/%s"), m_strRemoteBranch, m_strURL, m_strRemoteBranch);

			FetchProgressCommand fetchProgressCommand;
			fetchProgressCommand.SetUrl(m_strURL);
			fetchProgressCommand.SetRefSpec(refspec);
			m_GitProgressList.SetCommand(&fetchProgressCommand);
			m_GitProgressList.Init();
			ShowTab(IDC_CMD_GIT_PROG);
		}
		else
		{
			if(m_Gitverion >= 0x01070203) //above 1.7.0.2
				force += _T("--progress ");

			cmd.Format(_T("git.exe fetch -v %s \"%s\" %s"),
					force,
					m_strURL,
					remotebranch);

			m_GitCmdList.push_back(cmd);

			m_pThread = AfxBeginThread(ProgressThreadEntry, this, THREAD_PRIORITY_NORMAL,0,CREATE_SUSPENDED);
			if (m_pThread==NULL)
			{
			//		ReportError(CString(MAKEINTRESOURCE(IDS_ERR_THREADSTARTFAILED)));
			}
			else
			{
				m_pThread->m_bAutoDelete = TRUE;
				m_pThread->ResumeThread();
			}
		}
	}

	///Remote Update
	if(CurrentEntry == 3)
	{
		if (m_bAutoLoadPuttyKey)
		{
			STRING_VECTOR list;
			if (!g_Git.GetRemoteList(list))
			{
				for (size_t i = 0; i < list.size(); ++i)
					CAppUtils::LaunchPAgent(NULL, &list[i]);
			}
		}

		m_CurrentCmd = GIT_COMMAND_REMOTE;
		cmd=_T("git.exe remote update");
		m_GitCmdList.push_back(cmd);

		InterlockedExchange(&m_bBlock, TRUE);

		m_pThread = AfxBeginThread(ProgressThreadEntry, this, THREAD_PRIORITY_NORMAL,0,CREATE_SUSPENDED);
		if (m_pThread==NULL)
		{
		//		ReportError(CString(MAKEINTRESOURCE(IDS_ERR_THREADSTARTFAILED)));
			InterlockedExchange(&m_bBlock, FALSE);
		}
		else
		{
			m_pThread->m_bAutoDelete = TRUE;
			m_pThread->ResumeThread();
		}
	}

	///Cleanup stale remote banches
	if(CurrentEntry == 4)
	{
		m_CurrentCmd = GIT_COMMAND_REMOTE;
		cmd.Format(_T("git.exe remote prune \"%s\""), m_strURL);
		m_GitCmdList.push_back(cmd);

		InterlockedExchange(&m_bBlock, TRUE);

		m_pThread = AfxBeginThread(ProgressThreadEntry, this, THREAD_PRIORITY_NORMAL,0,CREATE_SUSPENDED);
		if (m_pThread==NULL)
		{
		//		ReportError(CString(MAKEINTRESOURCE(IDS_ERR_THREADSTARTFAILED)));
			InterlockedExchange(&m_bBlock, FALSE);
		}
		else
		{
			m_pThread->m_bAutoDelete = TRUE;
			m_pThread->ResumeThread();
		}
	}
}

void CSyncDlg::PullComplete()
{
	EnableControlButton(true);
	SwitchToInput();
	this->FetchOutList(true);

	CGitHash newhash;
	if (g_Git.GetHash(newhash, _T("HEAD")))
		MessageBox(g_Git.GetGitLastErr(_T("Could not get HEAD hash after pulling.")), _T("TortoiseGit"), MB_ICONERROR);

	if( this ->m_GitCmdStatus )
	{
		CTGitPathList list;
		if(g_Git.ListConflictFile(list))
		{
			this->m_ctrlCmdOut.SetSel(-1,-1);
			this->m_ctrlCmdOut.ReplaceSel(_T("Get conflict files fail\n"));

			this->ShowTab(IDC_CMD_LOG);
			return;
		}

		if (!list.IsEmpty())
		{
			this->m_ConflictFileList.Clear();
			CTGitPathList list;
			CTGitPath path;
			list.AddPath(path);

			this->m_ConflictFileList.GetStatus(&list,true);
			this->m_ConflictFileList.Show(CTGitPath::LOGACTIONS_UNMERGED,
											CTGitPath::LOGACTIONS_UNMERGED);

			this->ShowTab(IDC_IN_CONFLICT);
		}
		else
			this->ShowTab(IDC_CMD_LOG);

	}
	else
	{
		if(newhash == this->m_oldHash)
		{
			this->m_ctrlTabCtrl.ShowTab(IDC_IN_CHANGELIST-1,false);
			this->m_InLogList.ShowText(CString(MAKEINTRESOURCE(IDS_UPTODATE)));
			this->m_ctrlTabCtrl.ShowTab(IDC_IN_LOGLIST-1,true);
			this->ShowTab(IDC_REFLIST);
		}
		else
		{
			this->m_ctrlTabCtrl.ShowTab(IDC_IN_CHANGELIST-1,true);
			this->m_ctrlTabCtrl.ShowTab(IDC_IN_LOGLIST-1,true);

			this->AddDiffFileList(&m_InChangeFileList, &m_arInChangeList, newhash.ToString(), m_oldHash.ToString());

			CString range;
			range.Format(_T("%s..%s"), m_oldHash.ToString(), newhash.ToString());
			m_InLogList.FillGitLog(nullptr, &range, CGit::LOG_INFO_STAT| CGit::LOG_INFO_FILESTATE | CGit::LOG_INFO_SHOW_MERGEDFILE);
			this->ShowTab(IDC_IN_LOGLIST);
		}
	}
}

void CSyncDlg::FetchComplete()
{
	EnableControlButton(true);
	SwitchToInput();
	this->FetchOutList(true);

	if (g_Git.UsingLibGit2(CGit::GIT_CMD_FETCH))
		ShowTab(IDC_CMD_GIT_PROG);
	else
		ShowTab(IDC_REFLIST);

	if (m_GitCmdStatus)
		return;

	if (m_CurrentCmd != GIT_COMMAND_FETCHANDREBASE)
		return;

	CString remote;
	CString remotebranch;
	CString upstream;
	m_ctrlURL.GetWindowText(remote);
	if (!remote.IsEmpty())
	{
		STRING_VECTOR remotes;
		g_Git.GetRemoteList(remotes);
		if (std::find(remotes.begin(), remotes.end(), remote) == remotes.end())
			remote.Empty();
	}
	m_ctrlRemoteBranch.GetWindowText(remotebranch);
	if (!remote.IsEmpty() && !remotebranch.IsEmpty())
		upstream = _T("remotes/") + remote + _T("/") + remotebranch;

	CAppUtils::RebaseAfterFetch(upstream);
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
		CTGitPathList list;
		if (g_Git.ListConflictFile(list))
		{
			m_ctrlCmdOut.SetSel(-1, -1);
			m_ctrlCmdOut.ReplaceSel(_T("Get conflict files fail\n"));

			ShowTab(IDC_CMD_LOG);
			return;
		}

		if (!list.IsEmpty())
		{
			m_ConflictFileList.Clear();
			CTGitPathList list;
			CTGitPath path;
			list.AddPath(path);

			m_ConflictFileList.GetStatus(&list,true);
			m_ConflictFileList.Show(CTGitPath::LOGACTIONS_UNMERGED, CTGitPath::LOGACTIONS_UNMERGED);

			ShowTab(IDC_IN_CONFLICT);
		}
		else
			ShowTab(IDC_CMD_LOG);
	}
}

void CSyncDlg::OnBnClickedButtonPush()
{
	this->UpdateData();
	UpdateCombox();
	m_ctrlCmdOut.SetWindowTextW(_T(""));
	m_LogText = "";

	if(this->m_strURL.IsEmpty())
	{
		CMessageBox::Show(NULL, IDS_PROC_GITCONFIG_URLEMPTY, IDS_APPNAME, MB_OK | MB_ICONERROR);
		return;
	}

	this->m_regPushButton=(DWORD)this->m_ctrlPush.GetCurrentEntry();
	this->SwitchToRun();
	this->m_bAbort=false;
	this->m_GitCmdList.clear();

	ShowTab(IDC_CMD_LOG);

	CString cmd;
	CString arg;

	CString error;
	DWORD exitcode = 0xFFFFFFFF;
	if (CHooks::Instance().PrePush(g_Git.m_CurrentDir, exitcode, error))
	{
		if (exitcode)
		{
			CString temp;
			temp.Format(IDS_ERR_HOOKFAILED, (LPCTSTR)error);
			CMessageBox::Show(NULL,temp,_T("TortoiseGit"),MB_OK|MB_ICONERROR);
			return ;
		}
	}

	CString refName = g_Git.FixBranchName(m_strLocalBranch);
	switch (m_ctrlPush.GetCurrentEntry())
	{
	case 1:
		arg += _T(" --tags ");
		break;
	case 2:
		refName = _T("refs/notes/commits");	//default ref for notes
		break;
	}

	if(this->m_bForce)
		arg += _T(" --force ");

	if(m_Gitverion >= 0x01070203) //above 1.7.0.2
		arg += _T("--progress ");

	cmd.Format(_T("git.exe push -v %s \"%s\" %s"),
				arg,
				m_strURL,
				refName);

	if (!m_strRemoteBranch.IsEmpty() && m_ctrlPush.GetCurrentEntry() != 2)
	{
		cmd += _T(":") + m_strRemoteBranch;
	}

	m_GitCmdList.push_back(cmd);

	m_CurrentCmd = GIT_COMMAND_PUSH;

	if(this->m_bAutoLoadPuttyKey)
	{
		CAppUtils::LaunchPAgent(NULL,&this->m_strURL);
	}

	m_pThread = AfxBeginThread(ProgressThreadEntry, this, THREAD_PRIORITY_NORMAL,0,CREATE_SUSPENDED);
	if (m_pThread==NULL)
	{
//		ReportError(CString(MAKEINTRESOURCE(IDS_ERR_THREADSTARTFAILED)));
	}
	else
	{
		m_pThread->m_bAutoDelete = TRUE;
		m_pThread->ResumeThread();
	}
}

void CSyncDlg::OnBnClickedButtonApply()
{
	CGitHash oldhash;
	if (g_Git.GetHash(oldhash, _T("HEAD")))
	{
		MessageBox(g_Git.GetGitLastErr(_T("Could not get HEAD hash.")), _T("TortoiseGit"), MB_ICONERROR);
		return;
	}

	CImportPatchDlg dlg;
	CString cmd,output;

	if(dlg.DoModal() == IDOK)
	{
		int err=0;
		for (int i = 0; i < dlg.m_PathList.GetCount(); ++i)
		{
			cmd.Format(_T("git.exe am \"%s\""),dlg.m_PathList[i].GetGitPathString());

			if (g_Git.Run(cmd, &output, CP_UTF8))
			{
				CMessageBox::Show(NULL,output,_T("TortoiseGit"),MB_OK);

				err=1;
				break;
			}
			this->m_ctrlCmdOut.SetSel(-1,-1);
			this->m_ctrlCmdOut.ReplaceSel(cmd+_T("\n"));
			this->m_ctrlCmdOut.SetSel(-1,-1);
			this->m_ctrlCmdOut.ReplaceSel(output);
		}

		CGitHash newhash;
		if (g_Git.GetHash(newhash, _T("HEAD")))
		{
			MessageBox(g_Git.GetGitLastErr(_T("Could not get HEAD hash after applying patches.")), _T("TortoiseGit"), MB_ICONERROR);
			return;
		}

		this->m_InLogList.Clear();
		this->m_InChangeFileList.Clear();

		if(newhash == oldhash)
		{
			this->m_ctrlTabCtrl.ShowTab(IDC_IN_CHANGELIST-1,false);
			this->m_InLogList.ShowText(_T("No commits get from patch"));
			this->m_ctrlTabCtrl.ShowTab(IDC_IN_LOGLIST-1,true);

		}
		else
		{
			this->m_ctrlTabCtrl.ShowTab(IDC_IN_CHANGELIST-1,true);
			this->m_ctrlTabCtrl.ShowTab(IDC_IN_LOGLIST-1,true);

			CString range;
			range.Format(_T("%s..%s"), m_oldHash.ToString(), newhash.ToString());
			this->AddDiffFileList(&m_InChangeFileList, &m_arInChangeList, newhash.ToString(), oldhash.ToString());
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

	cmd.Format(_T("git.exe format-patch -o \"%s\" %s..%s"),
					g_Git.m_CurrentDir,
					m_strURL+_T('/')+m_strRemoteBranch,g_Git.FixBranchName(m_strLocalBranch));

	if (g_Git.Run(cmd, &out, &err, CP_UTF8))
	{
		CMessageBox::Show(NULL, out + L"\n" + err, _T("TortoiseGit"), MB_OK|MB_ICONERROR);
		return ;
	}

	CAppUtils::SendPatchMail(cmd,out);

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
	CAutoLibrary hUser = AtlLoadSystemLibraryUsingFullPath(_T("user32.dll"));
	if (hUser)
	{
		ChangeWindowMessageFilterExDFN *pfnChangeWindowMessageFilterEx = (ChangeWindowMessageFilterExDFN*)GetProcAddress(hUser, "ChangeWindowMessageFilterEx");
		if (pfnChangeWindowMessageFilterEx)
		{
			pfnChangeWindowMessageFilterEx(m_hWnd, WM_TASKBARBTNCREATED, MSGFLT_ALLOW, &cfs);
		}
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
	DWORD dwStyle;
	dwStyle= ES_MULTILINE | ES_READONLY | WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_AUTOVSCROLL |WS_VSCROLL  ;

	if( !m_ctrlCmdOut.Create(dwStyle,rectDummy,&m_ctrlTabCtrl,IDC_CMD_LOG))
	{
		TRACE0("Failed to create Log commits window\n");
		return FALSE;      // fail to create
	}

	// set the font to use in the log message view, configured in the settings dialog
	CFont m_logFont;
	CAppUtils::CreateFontForLogs(m_logFont);
	//GetDlgItem(IDC_CMD_LOG)->SetFont(&m_logFont);
	m_ctrlCmdOut.SetFont(&m_logFont);
	m_ctrlTabCtrl.InsertTab(&m_ctrlCmdOut, CString(MAKEINTRESOURCE(IDS_LOG)), -1);

	//m_ctrlCmdOut.ReplaceSel(_T("Hello"));

	//----------  Create in coming list ctrl -----------
	dwStyle =LVS_REPORT | LVS_SHOWSELALWAYS | LVS_ALIGNLEFT | LVS_OWNERDATA | WS_BORDER | WS_TABSTOP | WS_CHILD | WS_VISIBLE;;

	if( !m_InLogList.Create(dwStyle,rectDummy,&m_ctrlTabCtrl,IDC_IN_LOGLIST))
	{
		TRACE0("Failed to create output commits window\n");
		return FALSE;      // fail to create
	}

	m_ctrlTabCtrl.InsertTab(&m_InLogList, CString(MAKEINTRESOURCE(IDS_PROC_SYNC_INCOMMITS)), -1);

	m_InLogList.m_ColumnRegKey=_T("SyncIn");
	m_InLogList.InsertGitColumn();

	//----------- Create In Change file list -----------
	dwStyle = LVS_REPORT | LVS_SHOWSELALWAYS | LVS_ALIGNLEFT | WS_BORDER | WS_TABSTOP |LVS_SINGLESEL |WS_CHILD | WS_VISIBLE;

	if( !m_InChangeFileList.Create(dwStyle,rectDummy,&m_ctrlTabCtrl,IDC_IN_CHANGELIST))
	{
		TRACE0("Failed to create output change files window\n");
		return FALSE;      // fail to create
	}
	m_ctrlTabCtrl.InsertTab(&m_InChangeFileList, CString(MAKEINTRESOURCE(IDS_PROC_SYNC_INCHANGELIST)), -1);

	m_InChangeFileList.Init(GITSLC_COLEXT | GITSLC_COLSTATUS |GITSLC_COLADD|GITSLC_COLDEL , _T("InSyncDlg"),
							(CGitStatusListCtrl::GetContextMenuBit(CGitStatusListCtrl::IDGITLC_COMPARETWOREVISIONS) |
							CGitStatusListCtrl::GetContextMenuBit(CGitStatusListCtrl::IDGITLC_GNUDIFF2REVISIONS)), false, true, GITSLC_COLEXT | GITSLC_COLSTATUS | GITSLC_COLADD| GITSLC_COLDEL);


	//---------- Create Conflict List Ctrl -----------------
	dwStyle = LVS_REPORT | LVS_SHOWSELALWAYS | LVS_ALIGNLEFT | WS_BORDER | WS_TABSTOP | WS_CHILD | WS_VISIBLE;

	if( !m_ConflictFileList.Create(dwStyle,rectDummy,&m_ctrlTabCtrl,IDC_IN_CONFLICT))
	{
		TRACE0("Failed to create output change files window\n");
		return FALSE;      // fail to create
	}
	m_ctrlTabCtrl.InsertTab(&m_ConflictFileList, CString(MAKEINTRESOURCE(IDS_PROC_SYNC_CONFLICTS)), -1);

	m_ConflictFileList.Init(GITSLC_COLEXT | GITSLC_COLSTATUS |GITSLC_COLADD|GITSLC_COLDEL , _T("ConflictSyncDlg"),
							(CGitStatusListCtrl::GetContextMenuBit(CGitStatusListCtrl::IDGITLC_COMPARETWOREVISIONS) |
							CGitStatusListCtrl::GetContextMenuBit(CGitStatusListCtrl::IDGITLC_GNUDIFF2REVISIONS) |
							GITSLC_POPCONFLICT|GITSLC_POPRESOLVE),false);


	//----------  Create Commit Out List Ctrl---------------

	dwStyle =LVS_REPORT | LVS_SHOWSELALWAYS | LVS_ALIGNLEFT | LVS_OWNERDATA | WS_BORDER | WS_TABSTOP | WS_CHILD | WS_VISIBLE;;

	if( !m_OutLogList.Create(dwStyle,rectDummy,&m_ctrlTabCtrl,IDC_OUT_LOGLIST))
	{
		TRACE0("Failed to create output commits window\n");
		return FALSE;      // fail to create

	}

	m_ctrlTabCtrl.InsertTab(&m_OutLogList, CString(MAKEINTRESOURCE(IDS_PROC_SYNC_OUTCOMMITS)), -1);

	m_OutLogList.m_ColumnRegKey = _T("SyncOut");
	m_OutLogList.InsertGitColumn();

	//------------- Create Change File List Control ----------------

	dwStyle = LVS_REPORT | LVS_SHOWSELALWAYS | LVS_ALIGNLEFT | WS_BORDER | WS_TABSTOP |LVS_SINGLESEL |WS_CHILD | WS_VISIBLE;

	if( !m_OutChangeFileList.Create(dwStyle,rectDummy,&m_ctrlTabCtrl,IDC_OUT_CHANGELIST))
	{
		TRACE0("Failed to create output change files window\n");
		return FALSE;      // fail to create
	}
	m_ctrlTabCtrl.InsertTab(&m_OutChangeFileList, CString(MAKEINTRESOURCE(IDS_PROC_SYNC_OUTCHANGELIST)), -1);

	m_OutChangeFileList.Init(GITSLC_COLEXT | GITSLC_COLSTATUS | GITSLC_COLADD | GITSLC_COLDEL, _T("OutSyncDlg"),
							(CGitStatusListCtrl::GetContextMenuBit(CGitStatusListCtrl::IDGITLC_COMPARETWOREVISIONS) |
							CGitStatusListCtrl::GetContextMenuBit(CGitStatusListCtrl::IDGITLC_GNUDIFF2REVISIONS)), false, true, GITSLC_COLEXT | GITSLC_COLSTATUS | GITSLC_COLADD | GITSLC_COLDEL);

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

	dwStyle = LVS_REPORT | LVS_SHOWSELALWAYS | LVS_ALIGNLEFT | WS_BORDER | WS_TABSTOP | WS_CHILD | WS_VISIBLE;
	DWORD exStyle = LVS_EX_HEADERDRAGDROP | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP | LVS_EX_FULLROWSELECT;
	m_refList.Create(dwStyle, rectDummy, &m_ctrlTabCtrl, IDC_REFLIST);
	m_refList.SetExtendedStyle(exStyle);
	m_refList.Init();
	m_ctrlTabCtrl.InsertTab(&m_refList, CString(MAKEINTRESOURCE(IDS_REFLIST)), -1);

	this->m_tooltips.Create(this);

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

	AdjustControlSize(IDC_CHECK_PUTTY_KEY);
	AdjustControlSize(IDC_CHECK_FORCE);

	CString WorkingDir=g_Git.m_CurrentDir;
	WorkingDir.Replace(_T(':'),_T('_'));
	m_RegKeyRemoteBranch = CString(_T("Software\\TortoiseGit\\History\\SyncBranch\\"))+WorkingDir;


	this->AddOthersToAnchor();

	this->m_ctrlPush.AddEntry(CString(MAKEINTRESOURCE(IDS_PROC_SYNC_PUSH)));
	this->m_ctrlPush.AddEntry(CString(MAKEINTRESOURCE(IDS_PROC_SYNC_PUSHTAGS)));
	this->m_ctrlPush.AddEntry(CString(MAKEINTRESOURCE(IDS_PROC_SYNC_PUSHNOTES)));

	this->m_ctrlPull.AddEntry(CString(MAKEINTRESOURCE(IDS_PROC_SYNC_PULL)));
	this->m_ctrlPull.AddEntry(CString(MAKEINTRESOURCE(IDS_PROC_SYNC_FETCH)));
	this->m_ctrlPull.AddEntry(CString(MAKEINTRESOURCE(IDS_PROC_SYNC_FETCHREBASE)));
	this->m_ctrlPull.AddEntry(CString(MAKEINTRESOURCE(IDS_PROC_SYNC_REMOTEUPDATE)));
	this->m_ctrlPull.AddEntry(CString(MAKEINTRESOURCE(IDS_PROC_SYNC_CLEANUPSTALEBRANCHES)));

	this->m_ctrlSubmodule.AddEntry(CString(MAKEINTRESOURCE(IDS_PROC_SYNC_SUBKODULEUPDATE)));
	this->m_ctrlSubmodule.AddEntry(CString(MAKEINTRESOURCE(IDS_PROC_SYNC_SUBKODULEINIT)));
	this->m_ctrlSubmodule.AddEntry(CString(MAKEINTRESOURCE(IDS_PROC_SYNC_SUBKODULESYNC)));

	this->m_ctrlStash.AddEntry(CString(MAKEINTRESOURCE(IDS_MENUSTASHSAVE)));
	this->m_ctrlStash.AddEntry(CString(MAKEINTRESOURCE(IDS_MENUSTASHPOP)));
	this->m_ctrlStash.AddEntry(CString(MAKEINTRESOURCE(IDS_MENUSTASHAPPLY)));

	WorkingDir.Replace(_T(':'),_T('_'));

	CString regkey ;
	regkey.Format(_T("Software\\TortoiseGit\\TortoiseProc\\Sync\\%s"),WorkingDir);

	this->m_regPullButton = CRegDWORD(regkey+_T("\\Pull"),0);
	this->m_regPushButton = CRegDWORD(regkey+_T("\\Push"),0);
	this->m_regSubmoduleButton = CRegDWORD(regkey+_T("\\Submodule"));
	this->m_regAutoLoadPutty = CRegDWORD(regkey + _T("\\AutoLoadPutty"), CAppUtils::IsSSHPutty());

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

	EnableSaveRestore(_T("SyncDlg"));

	m_ctrlURL.SetCaseSensitive(TRUE);
	m_ctrlURL.SetURLHistory(true);
	this->m_ctrlURL.LoadHistory(CString(_T("Software\\TortoiseGit\\History\\SyncURL\\"))+WorkingDir, _T("url"));

	STRING_VECTOR list;

	if(!g_Git.GetRemoteList(list))
	{
		for (unsigned int i = 0; i < list.size(); ++i)
		{
			m_ctrlURL.AddString(list[i]);
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

	m_ctrlRemoteBranch.m_bWantReturn = TRUE;
	m_ctrlURL.m_bWantReturn = TRUE;

	this->m_Gitverion = CAppUtils::GetMsysgitVersion();

	if (m_seq > 0 && (DWORD)CRegDWORD(_T("Software\\TortoiseGit\\SyncDialogRandomPos")))
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
	workingDir.Replace(_T(':'), _T('_'));
	this->m_ctrlURL.LoadHistory(_T("Software\\TortoiseGit\\History\\SyncURL\\") + workingDir, _T("url"));

	STRING_VECTOR list;
	bool found = false;
	if (!g_Git.GetRemoteList(list))
	{
		for (size_t i = 0; i < list.size(); ++i)
		{
			m_ctrlURL.AddString(list[i]);
			if (list[i] == url)
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
				TCHAR buff[128] = { 0 };
				::GetClassName(pMsg->hwnd,buff,128);

				/* Use MSFTEDIT_CLASS http://msdn.microsoft.com/en-us/library/bb531344.aspx */
				if (_tcsnicmp(buff, MSFTEDIT_CLASS, 128) == 0 ||	//Unicode and MFC 2012 and later
					_tcsnicmp(buff, RICHEDIT_CLASS, 128) == 0 ||	//ANSI or MFC 2010
					_tcsnicmp(buff, _T("SysListView32"), 128) == 0)
				{
					this->PostMessage(WM_KEYDOWN,VK_ESCAPE,0);
					return TRUE;
				}
			}
		}
	}
	m_tooltips.RelayEvent(pMsg);
	return __super::PreTranslateMessage(pMsg);
}
void CSyncDlg::FetchOutList(bool force)
{
	if(!m_bInited)
		return;
	m_OutChangeFileList.Clear();
	this->m_OutLogList.Clear();

	CString remote;
	this->m_ctrlURL.GetWindowText(remote);
	CString remotebranch;
	this->m_ctrlRemoteBranch.GetWindowText(remotebranch);
	remotebranch=remote+_T("/")+remotebranch;
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
		str.Format(IDS_PROC_SYNC_PUSH_UNKNOWNBRANCH, remotebranch);
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
				MessageBox(g_Git.GetGitLastErr(_T("Could not get hash of \"") + localbranch + _T("\".")), _T("TortoiseGit"), MB_ICONERROR);
				return;
			}
			if (remotebranchHash == localBranchHash)
			{
				CString str;
				str.Format(IDS_PROC_SYNC_COMMITSAHEAD, 0, remotebranch);
				m_OutLogList.ShowText(str);
				this->m_ctrlStatus.SetWindowText(str);
				this->m_ctrlTabCtrl.ShowTab(m_OutChangeFileList.GetDlgCtrlID()-1,FALSE);
				this->GetDlgItem(IDC_BUTTON_EMAIL)->EnableWindow(FALSE);
			}
			else if (isFastForward || m_bForce)
			{
				CString range;
				range.Format(_T("%s..%s"), g_Git.FixBranchName(remotebranch), g_Git.FixBranchName(localbranch));
				//fast forward
				m_OutLogList.FillGitLog(nullptr, &range, CGit::LOG_INFO_STAT | CGit::LOG_INFO_FILESTATE | CGit::LOG_INFO_SHOW_MERGEDFILE);
				CString str;
				str.Format(IDS_PROC_SYNC_COMMITSAHEAD, m_OutLogList.GetItemCount(), remotebranch);
				this->m_ctrlStatus.SetWindowText(str);

				if (isFastForward)
					AddDiffFileList(&m_OutChangeFileList, &m_arOutChangeList, localbranch, remotebranch);
				else
				{
					AddDiffFileList(&m_OutChangeFileList, &m_arOutChangeList, localbranch, base.ToString());
				}

				this->m_ctrlTabCtrl.ShowTab(m_OutChangeFileList.GetDlgCtrlID()-1,TRUE);
				this->GetDlgItem(IDC_BUTTON_EMAIL)->EnableWindow(TRUE);
			}
			else
			{
				CString str;
				str.Format(IDS_PROC_SYNC_NOFASTFORWARD, localbranch, remotebranch);
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
	if(str.Find(_T('\\'))>=0 || str.Find(_T('/'))>=0)
		return true;
	else
		return false;
}
void CSyncDlg::OnCbnEditchangeComboboxex()
{
	SetTimer(IDT_INPUT, 1000, NULL);
	this->m_OutLogList.ShowText(CString(MAKEINTRESOURCE(IDS_PROC_SYNC_WAINTINPUT)));

	//this->FetchOutList();
}

UINT CSyncDlg::ProgressThread()
{
	m_startTick = GetTickCount();
	m_bDone = false;
	STRING_VECTOR list;
	CProgressDlg::RunCmdList(this, m_GitCmdList, list, true, nullptr, &this->m_bAbort, &this->m_Databuf);
	InterlockedExchange(&m_bBlock, FALSE);
	return 0;
}


LRESULT CSyncDlg::OnProgressUpdateUI(WPARAM wParam,LPARAM lParam)
{
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
		DWORD tickSpent = GetTickCount() - m_startTick;
		CString strEndTime = CLoglistUtils::FormatDateAndTime(CTime::GetCurrentTime(), DATE_SHORTDATE, true, false);

		m_BufStart = 0;
		m_Databuf.m_critSec.Lock();
		m_Databuf.clear();
		m_Databuf.m_critSec.Unlock();

		m_bDone = true;
		m_ctrlAnimate.Stop();
		m_ctrlProgress.SetPos(100);
		//this->DialogEnableWindow(IDOK,TRUE);

		DWORD exitCode = (DWORD)lParam;
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
			err.Format(_T("\r\n\r\n%s (%lu ms @ %s)\r\n"), log, tickSpent, strEndTime);
			CProgressDlg::InsertColorText(this->m_ctrlCmdOut, err, RGB(255,0,0));
			PlaySound((LPCTSTR)SND_ALIAS_SYSTEMEXCLAMATION, NULL, SND_ALIAS_ID | SND_ASYNC);
		}
		else
		{
			if (m_pTaskbarList)
				m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NOPROGRESS);
			CString temp;
			temp.LoadString(IDS_SUCCESS);
			CString log;
			log.Format(_T("\r\n%s (%lu ms @ %s)\r\n"), temp, tickSpent, strEndTime);
			CProgressDlg::InsertColorText(this->m_ctrlCmdOut, log, RGB(0,0,255));
		}
		m_GitCmdStatus = exitCode;

		//if(wParam == MSG_PROGRESSDLG_END)
		RunPostAction();
	}

	if(lParam != 0)
		ParserCmdOutput((char)lParam);
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
			m_Databuf.erase(m_Databuf.begin(), m_Databuf.begin() + m_BufStart);
			m_BufStart = 0;
		}
		m_Databuf.m_critSec.Unlock();
	}

	return 0;
}

static std::map<CString, CGitHash> * HashMapToRefMap(MAP_HASH_NAME &map)
{
	auto rmap = new std::map<CString, CGitHash>();
	for (auto mit = map.begin(); mit != map.end(); ++mit)
	{
		for (auto rit = mit->second.begin(); rit != mit->second.end(); ++rit)
		{
			rmap->insert(std::make_pair(*rit, mit->first));
		}
	}
	return rmap;
}

void CSyncDlg::FillNewRefMap()
{
	m_refList.Clear();
	m_newHashMap.clear();
	CAutoRepository repo(g_Git.GetGitRepository());
	if (!repo)
	{
		CMessageBox::Show(m_hWnd, CGit::GetLibGit2LastErr(_T("Could not open repository.")), _T("TortoiseGit"), MB_OK | MB_ICONERROR);
		return;
	}

	if (CGit::GetMapHashToFriendName(repo, m_newHashMap))
	{
		MessageBox(CGit::GetLibGit2LastErr(_T("Could not get all refs.")), _T("TortoiseGit"), MB_ICONERROR);
		return;
	}

	auto oldRefMap = HashMapToRefMap(m_oldHashMap);
	auto newRefMap = HashMapToRefMap(m_newHashMap);
	for (auto oit = oldRefMap->begin(); oit != oldRefMap->end(); ++oit)
	{
		bool found = false;
		for (auto nit = newRefMap->begin(); nit != newRefMap->end(); ++nit)
		{
			// changed ref
			if (oit->first == nit->first)
			{
				found = true;
				m_refList.AddEntry(repo, oit->first, &oit->second, &nit->second);
				break;
			}
		}
		// deleted ref
		if (!found)
		{
			m_refList.AddEntry(repo, oit->first, &oit->second, nullptr);
		}
	}
	for (auto nit = newRefMap->begin(); nit != newRefMap->end(); ++nit)
	{
		bool found = false;
		for (auto oit = oldRefMap->begin(); oit != oldRefMap->end(); ++oit)
		{
			if (oit->first == nit->first)
			{
				found = true;
				break;
			}
		}
		// new ref
		if (!found)
		{
			m_refList.AddEntry(repo, nit->first, nullptr, &nit->second);
		}
	}
	delete oldRefMap;
	delete newRefMap;
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
		if (CHooks::Instance().PostPush(g_Git.m_CurrentDir, exitcode, error))
		{
			if (exitcode)
			{
				CString temp;
				temp.Format(IDS_ERR_HOOKFAILED, (LPCTSTR)error);
				CMessageBox::Show(nullptr, temp,_T("TortoiseGit"), MB_OK | MB_ICONERROR);
				return;
			}
		}

		EnableControlButton(true);
		SwitchToInput();
		this->FetchOutList(true);
	}
	else if (this->m_CurrentCmd == GIT_COMMAND_PULL)
	{
		PullComplete();
	}
	else if (this->m_CurrentCmd == GIT_COMMAND_FETCH || this->m_CurrentCmd == GIT_COMMAND_FETCHANDREBASE)
	{
		FetchComplete();
	}
	else if (this->m_CurrentCmd == GIT_COMMAND_SUBMODULE)
	{
		//this->m_ctrlCmdOut.SetSel(-1,-1);
		//this->m_ctrlCmdOut.ReplaceSel(_T("Done\r\n"));
		//this->m_ctrlCmdOut.SetSel(-1,-1);
		EnableControlButton(true);
		SwitchToInput();
	}
	else if (this->m_CurrentCmd == GIT_COMMAND_STASH)
	{
		StashComplete();
	}
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
	CProgressDlg::ParserCmdOutput(m_ctrlCmdOut,m_ctrlProgress,m_hWnd,m_pTaskbarList,m_LogText,ch);
}
void CSyncDlg::OnBnClickedButtonCommit()
{
	CString cmd = _T("/command:commit");
	cmd += _T(" /path:\"");
	cmd += g_Git.m_CurrentDir;
	cmd += _T("\"");

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
	if (m_bDone)
	{
		CResizableStandAloneDialog::OnCancel();
		return;
	}

	if (g_Git.m_CurrentGitPi.hProcess)
	{
		DWORD dwConfirmKillProcess = CRegDWORD(_T("Software\\TortoiseGit\\ConfirmKillProcess"));
		if (dwConfirmKillProcess && CMessageBox::Show(m_hWnd, IDS_PROC_CONFIRMKILLPROCESS, IDS_APPNAME, MB_YESNO | MB_ICONQUESTION) != IDYES)
			return;
		if (::GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0))
		{
			::WaitForSingleObject(g_Git.m_CurrentGitPi.hProcess, 10000);
		}
		else
		{
			GetLastError();
		}

		CProgressDlg::KillProcessTree(g_Git.m_CurrentGitPi.dwProcessId);
	}

	::WaitForSingleObject(g_Git.m_CurrentGitPi.hProcess ,10000);
	m_tooltips.Pop();
	CResizableStandAloneDialog::OnCancel();
}

void CSyncDlg::OnBnClickedButtonSubmodule()
{
	this->UpdateData();
	UpdateCombox();
	m_ctrlCmdOut.SetWindowTextW(_T(""));
	m_LogText = "";

	this->m_regSubmoduleButton = (DWORD)this->m_ctrlSubmodule.GetCurrentEntry();

	this->SwitchToRun();

	this->m_bAbort=false;
	this->m_GitCmdList.clear();

	ShowTab(IDC_CMD_LOG);

	CString cmd;

	switch (m_ctrlSubmodule.GetCurrentEntry())
	{
	case 0:
		cmd=_T("git.exe submodule update --init");
		break;
	case 1:
		cmd=_T("git.exe submodule init");
		break;
	case 2:
		cmd=_T("git.exe submodule sync");
		break;
	}

	m_GitCmdList.push_back(cmd);

	m_CurrentCmd = GIT_COMMAND_SUBMODULE;

	m_pThread = AfxBeginThread(ProgressThreadEntry, this, THREAD_PRIORITY_NORMAL,0,CREATE_SUSPENDED);
	if (m_pThread==NULL)
	{
//		ReportError(CString(MAKEINTRESOURCE(IDS_ERR_THREADSTARTFAILED)));
	}
	else
	{
		m_pThread->m_bAutoDelete = TRUE;
		m_pThread->ResumeThread();
	}
}

void CSyncDlg::OnBnClickedButtonStash()
{
	UpdateData();
	UpdateCombox();
	m_ctrlCmdOut.SetWindowTextW(_T(""));
	m_LogText = "";

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
		cmd = _T("git.exe stash save");
		break;
	case 1:
		cmd = _T("git.exe stash pop");
		break;
	case 2:
		cmd = _T("git.exe stash apply");
		break;
	}

	m_GitCmdList.push_back(cmd);
	m_CurrentCmd = GIT_COMMAND_STASH;

	m_pThread = AfxBeginThread(ProgressThreadEntry, this, THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
	if (!m_pThread)
	{
		//ReportError(CString(MAKEINTRESOURCE(IDS_ERR_THREADSTARTFAILED)));
	}
	else
	{
		m_pThread->m_bAutoDelete = TRUE;
		m_pThread->ResumeThread();
	}
}

void CSyncDlg::OnTimer(UINT_PTR nIDEvent)
{
	if( nIDEvent == IDT_INPUT)
	{
		KillTimer(IDT_INPUT);
		this->FetchOutList(true);
	}
}


void CSyncDlg::OnLvnInLogListColumnClick(NMHDR * /* pNMHDR */, LRESULT *pResult)
{
	*pResult = 0;
}

LRESULT CSyncDlg::OnTaskbarBtnCreated(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	m_pTaskbarList.Release();
	m_pTaskbarList.CoCreateInstance(CLSID_TaskbarList);
	m_GitProgressList.m_pTaskbarList = m_pTaskbarList;
	SetUUIDOverlayIcon(m_hWnd);
	return 0;
}

void CSyncDlg::OnBnClickedCheckForce()
{
	UpdateData();
}

void CSyncDlg::OnBnClickedLog()
{
	CString cmd = _T("/command:log");
	cmd += _T(" /path:\"");
	cmd += g_Git.m_CurrentDir;
	cmd += _T("\"");

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
