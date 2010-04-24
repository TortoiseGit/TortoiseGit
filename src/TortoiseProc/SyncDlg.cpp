// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2009 - TortoiseGit

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
#include "progressdlg.h"
#include "MessageBox.h"
#include "ImportPatchDlg.h"
#include "PathUtils.h"
#include "RebaseDlg.h"

// CSyncDlg dialog

IMPLEMENT_DYNAMIC(CSyncDlg, CResizableStandAloneDialog)

CSyncDlg::CSyncDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CSyncDlg::IDD, pParent)
{
	m_pTooltip=&this->m_tooltips;
	m_bInited=false;
	m_CmdOutCurrentPos=0;
	m_bAutoLoadPuttyKey = CAppUtils::IsSSHPutty();
	m_bForce=false;
	m_bBlock = false;
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
	BRANCH_COMBOX_DDX;
}


BEGIN_MESSAGE_MAP(CSyncDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_BUTTON_PULL, &CSyncDlg::OnBnClickedButtonPull)
	ON_BN_CLICKED(IDC_BUTTON_PUSH, &CSyncDlg::OnBnClickedButtonPush)
	ON_BN_CLICKED(IDC_BUTTON_APPLY, &CSyncDlg::OnBnClickedButtonApply)
	ON_BN_CLICKED(IDC_BUTTON_EMAIL, &CSyncDlg::OnBnClickedButtonEmail)
	ON_BN_CLICKED(IDC_BUTTON_MANAGE, &CSyncDlg::OnBnClickedButtonManage)
	BRANCH_COMBOX_EVENT
	ON_NOTIFY(CBEN_ENDEDIT, IDC_COMBOBOXEX_URL, &CSyncDlg::OnCbenEndeditComboboxexUrl)
	ON_CBN_EDITCHANGE(IDC_COMBOBOXEX_URL, &CSyncDlg::OnCbnEditchangeComboboxex)
	ON_CBN_EDITCHANGE(IDC_COMBOBOXEX_REMOTE_BRANCH, &CSyncDlg::OnCbnEditchangeComboboxex)
	ON_MESSAGE(MSG_PROGRESSDLG_UPDATE_UI, OnProgressUpdateUI)
	ON_BN_CLICKED(IDC_BUTTON_COMMIT, &CSyncDlg::OnBnClickedButtonCommit)
	ON_BN_CLICKED(IDC_BUTTON_SUBMODULE, &CSyncDlg::OnBnClickedButtonSubmodule)
	ON_WM_TIMER()
END_MESSAGE_MAP()


void CSyncDlg::EnableControlButton(bool bEnabled)
{
	GetDlgItem(IDC_BUTTON_PULL)->EnableWindow(bEnabled);
	GetDlgItem(IDC_BUTTON_PUSH)->EnableWindow(bEnabled);
	GetDlgItem(IDC_BUTTON_APPLY)->EnableWindow(bEnabled);
	GetDlgItem(IDC_BUTTON_EMAIL)->EnableWindow(bEnabled);
	GetDlgItem(IDOK)->EnableWindow(bEnabled);
	GetDlgItem(IDC_BUTTON_SUBMODULE)->EnableWindow(bEnabled);
}
// CSyncDlg message handlers

void CSyncDlg::OnBnClickedButtonPull()
{
	// TODO: Add your control notification handler code here
	int CurrentEntry;
	CurrentEntry = this->m_ctrlPull.GetCurrentEntry();                   
	this->m_regPullButton = CurrentEntry;


	this->m_bAbort=false;
	this->m_GitCmdList.clear();

	this->UpdateData();
	UpdateCombox();

	m_oldHash = g_Git.GetHash(CString(_T("HEAD")));

	if( CurrentEntry == 0)
	{
		if( g_Git.GetHash(this->m_strLocalBranch) != m_oldHash)
		{
			CMessageBox::Show(NULL,_T("Pull require local branch must be current branch"),_T("TortoiseGit"),MB_OK|MB_ICONERROR);
			return;							
		}
	}

	if(this->m_strURL.IsEmpty())
	{
		CMessageBox::Show(NULL,_T("URL can't Empty"),_T("TortoiseGit"),MB_OK|MB_ICONERROR);
		return;
	}

	if(this->m_bAutoLoadPuttyKey)
	{
		CAppUtils::LaunchPAgent(NULL,&this->m_strURL);
	}

	this->SwitchToRun();

	CString force;
	if(this->m_bForce)
		force = _T(" --force ");

	CString cmd;

	ShowTab(IDC_CMD_LOG);

	this->m_ctrlTabCtrl.ShowTab(IDC_IN_LOGLIST-1,false);
	this->m_ctrlTabCtrl.ShowTab(IDC_IN_CHANGELIST-1,false);
	this->m_ctrlTabCtrl.ShowTab(IDC_IN_CONFLICT-1,false);

	this->GetDlgItem(IDC_BUTTON_COMMIT)->ShowWindow(SW_HIDE);

	///Pull 
	if(CurrentEntry == 0) //Pull
	{
		cmd.Format(_T("git.exe pull %s \"%s\" %s"),
				force,
				m_strURL,
				this->m_strRemoteBranch);

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

		}else
		{
			
			remotebranch.Format(_T("remotes/%s/%s"),
								m_strURL,m_strRemoteBranch);
			if(g_Git.GetHash(remotebranch).IsEmpty())
				remotebranch=m_strRemoteBranch;
			else
				remotebranch=m_strRemoteBranch+_T(":")+remotebranch;
		}

		cmd.Format(_T("git.exe fetch %s \"%s\" %s"),
				force,
				m_strURL,
				remotebranch);

		if(CurrentEntry == 1) 
			m_CurrentCmd = GIT_COMMAND_FETCH;
		else
			m_CurrentCmd = GIT_COMMAND_FETCHANDREBASE;
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

	///Remote Update
	if(CurrentEntry == 3)
	{
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
	
}

void CSyncDlg::PullComplete()
{
	EnableControlButton(true);
	SwitchToInput();
	this->FetchOutList(true);

	CString newhash;
	newhash = g_Git.GetHash(CString(_T("HEAD")));

	

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

		if(list.GetCount()>0)
		{
			this->m_ConflictFileList.Clear();	
			CTGitPathList list;
			CTGitPath path;
			list.AddPath(path);

			this->m_ConflictFileList.GetStatus(&list,true);
			this->m_ConflictFileList.Show(CTGitPath::LOGACTIONS_UNMERGED,
									  CTGitPath::LOGACTIONS_UNMERGED);
			
			this->ShowTab(IDC_IN_CONFLICT);

			this->GetDlgItem(IDC_BUTTON_COMMIT)->ShowWindow(SW_NORMAL);
		}
		else
			this->ShowTab(IDC_CMD_LOG);

	}else
	{
		if(newhash == this->m_oldHash)
		{
			this->m_ctrlTabCtrl.ShowTab(IDC_IN_CHANGELIST-1,false);
			this->m_InLogList.ShowText(_T("No commits get after pull"));
			this->m_ctrlTabCtrl.ShowTab(IDC_IN_LOGLIST-1,true);
		}
		else
		{
			this->m_ctrlTabCtrl.ShowTab(IDC_IN_CHANGELIST-1,true);
			this->m_ctrlTabCtrl.ShowTab(IDC_IN_LOGLIST-1,true);
			
			this->AddDiffFileList(&m_InChangeFileList,&m_arInChangeList,newhash,m_oldHash);
		
			m_InLogList.FillGitLog(NULL,CGit::	LOG_INFO_STAT| CGit::LOG_INFO_FILESTATE | CGit::LOG_INFO_SHOW_MERGEDFILE,
				&this->m_oldHash,&newhash);
		}
		this->ShowTab(IDC_IN_LOGLIST);
	}
}

void CSyncDlg::FetchComplete()
{
	EnableControlButton(true);
	SwitchToInput();
	this->FetchOutList(true);

	ShowTab(IDC_CMD_LOG);
	if( (!this->m_GitCmdStatus) && this->m_CurrentCmd == GIT_COMMAND_FETCHANDREBASE)
	{
		CRebaseDlg dlg;
		dlg.m_PostButtonTexts.Add(_T("Email &Patch..."));
		int response = dlg.DoModal();
		if(response == IDOK)
		{
			return ;
		}

		if(response == IDC_REBASE_POST_BUTTON)
		{
			CString cmd,out;
			cmd.Format(_T("git.exe  format-patch -o \"%s\" %s..%s"),
					g_Git.m_CurrentDir,
					dlg.m_Upstream,dlg.m_Branch);
			if(g_Git.Run(cmd,&out,CP_ACP))
			{
				CMessageBox::Show(NULL,out,_T("TortoiseGit"),MB_OK|MB_ICONERROR);
				return ;
			}

			CAppUtils::SendPatchMail(cmd,out);
		}
	}
}

void CSyncDlg::OnBnClickedButtonPush()
{
	// TODO: Add your control notification handler code here
	this->UpdateData();
	UpdateCombox();

	if(this->m_strURL.IsEmpty())
	{
		CMessageBox::Show(NULL,_T("URL can't Empty"),_T("TortoiseGit"),MB_OK|MB_ICONERROR);
		return;
	}

	this->m_regPushButton=this->m_ctrlPush.GetCurrentEntry();
	this->SwitchToRun();
	this->m_bAbort=false;
	this->m_GitCmdList.clear();

	ShowTab(IDC_CMD_LOG);

	CString cmd;
	CString tags;
	CString force;
	CString all;



	switch (m_ctrlPush.GetCurrentEntry())
	{
	case 1:
		tags = _T(" --tags ");
		break;
	case 2:
		all = _T(" --all ");
		break;
	}

	if(this->m_bForce)
		force = _T(" --force ");

	cmd.Format(_T("git.exe push %s %s %s \"%s\" %s"),
				tags,force,all,
				m_strURL,
				m_strLocalBranch);

	if (!m_strRemoteBranch.IsEmpty())
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
	// TODO: Add your control notification handler code here
	CString oldhash;
	oldhash=g_Git.GetHash(CString(_T("HEAD")));
	
	CImportPatchDlg dlg;
	CString cmd,output;

	if(dlg.DoModal() == IDOK)
	{
		int err=0;
		for(int i=0;i<dlg.m_PathList.GetCount();i++)
		{			
			cmd.Format(_T("git.exe am \"%s\""),dlg.m_PathList[i].GetGitPathString());
			
			if(g_Git.Run(cmd,&output,CP_ACP))
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
		

		CString newhash=g_Git.GetHash(CString(_T("HEAD")));		

		this->m_InLogList.Clear();
		this->m_InChangeFileList.Clear();

		if(newhash == oldhash)
		{
			this->m_ctrlTabCtrl.ShowTab(IDC_IN_CHANGELIST-1,false);
			this->m_InLogList.ShowText(_T("No commits get from patch"));
			this->m_ctrlTabCtrl.ShowTab(IDC_IN_LOGLIST-1,true);

		}else
		{
			this->m_ctrlTabCtrl.ShowTab(IDC_IN_CHANGELIST-1,true);
			this->m_ctrlTabCtrl.ShowTab(IDC_IN_LOGLIST-1,true);
			
			this->AddDiffFileList(&m_InChangeFileList,&m_arInChangeList,newhash,oldhash);
			m_InLogList.FillGitLog(NULL,CGit::	LOG_INFO_STAT| CGit::LOG_INFO_FILESTATE | CGit::LOG_INFO_SHOW_MERGEDFILE,
				&oldhash,&newhash);

			this->FetchOutList(true);

		}

		this->m_ctrlTabCtrl.ShowTab(IDC_CMD_LOG-1,true);

		if(err)
		{
			this->ShowTab(IDC_CMD_LOG);

		}else
		{
			this->ShowTab(IDC_IN_LOGLIST);
		}
	}
}

void CSyncDlg::OnBnClickedButtonEmail()
{
	// TODO: Add your control notification handler code here
	CString cmd,out;
	
	this->m_strLocalBranch = this->m_ctrlLocalBranch.GetString();
	this->m_ctrlRemoteBranch.GetWindowText(this->m_strRemoteBranch);
	this->m_ctrlURL.GetWindowText(this->m_strURL);
	m_strURL=m_strURL.Trim();
	m_strRemoteBranch=m_strRemoteBranch.Trim();
	
	cmd.Format(_T("git.exe  format-patch -o \"%s\" %s..%s"),
					g_Git.m_CurrentDir,
					m_strURL+_T('/')+m_strRemoteBranch,m_strLocalBranch);
	
	if(g_Git.Run(cmd,&out,CP_ACP))
	{
		CMessageBox::Show(NULL,out,_T("TortoiseGit"),MB_OK|MB_ICONERROR);
		return ;
	}

	CAppUtils::SendPatchMail(cmd,out);	

}
void CSyncDlg::ShowProgressCtrl(bool bShow)
{
	int b=bShow?SW_NORMAL:SW_HIDE;
	this->m_ctrlAnimate.ShowWindow(b);
	this->m_ctrlProgress.ShowWindow(b);
	this->m_ctrlAnimate.Open(IDR_DOWNLOAD);
	if(b == SW_NORMAL)
		this->m_ctrlAnimate.Play(0,-1,-1);
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

	m_ctrlTabCtrl.InsertTab(&m_ctrlCmdOut,_T("Log"),-1);
	
	//m_ctrlCmdOut.ReplaceSel(_T("Hello"));

	//----------  Create in coming list ctrl -----------
	dwStyle =LVS_REPORT | LVS_SHOWSELALWAYS | LVS_ALIGNLEFT | LVS_OWNERDATA | WS_BORDER | WS_TABSTOP | WS_CHILD | WS_VISIBLE;;

	if( !m_InLogList.Create(dwStyle,rectDummy,&m_ctrlTabCtrl,IDC_IN_LOGLIST))
	{
		TRACE0("Failed to create output commits window\n");
		return FALSE;      // fail to create

	}

	m_ctrlTabCtrl.InsertTab(&m_InLogList,_T("In Commits"),-1);

	m_InLogList.InsertGitColumn();

	//----------- Create In Change file list -----------
	dwStyle = LVS_REPORT | LVS_SHOWSELALWAYS | LVS_ALIGNLEFT | WS_BORDER | WS_TABSTOP |LVS_SINGLESEL |WS_CHILD | WS_VISIBLE;
	
	if( !m_InChangeFileList.Create(dwStyle,rectDummy,&m_ctrlTabCtrl,IDC_IN_CHANGELIST))
	{
		TRACE0("Failed to create output change files window\n");
		return FALSE;      // fail to create
	}
	m_ctrlTabCtrl.InsertTab(&m_InChangeFileList,_T("In ChangeList"),-1);

	m_InChangeFileList.Init(SVNSLC_COLEXT | SVNSLC_COLSTATUS |SVNSLC_COLADD|SVNSLC_COLDEL , _T("OutSyncDlg"),
		                    (CGitStatusListCtrl::GetContextMenuBit(CGitStatusListCtrl::IDSVNLC_COMPARETWO)|
							CGitStatusListCtrl::GetContextMenuBit(CGitStatusListCtrl::IDSVNLC_GNUDIFF2)),false);


	//---------- Create Conflict List Ctrl -----------------
	dwStyle = LVS_REPORT | LVS_SHOWSELALWAYS | LVS_ALIGNLEFT | WS_BORDER | WS_TABSTOP |LVS_SINGLESEL |WS_CHILD | WS_VISIBLE;
	
	if( !m_ConflictFileList.Create(dwStyle,rectDummy,&m_ctrlTabCtrl,IDC_IN_CONFLICT))
	{
		TRACE0("Failed to create output change files window\n");
		return FALSE;      // fail to create
	}
	m_ctrlTabCtrl.InsertTab(&m_ConflictFileList,_T("Conflict"),-1);

	m_ConflictFileList.Init(SVNSLC_COLEXT | SVNSLC_COLSTATUS |SVNSLC_COLADD|SVNSLC_COLDEL , _T("OutSyncDlg"),
		                    (CGitStatusListCtrl::GetContextMenuBit(CGitStatusListCtrl::IDSVNLC_COMPARETWO)|
							CGitStatusListCtrl::GetContextMenuBit(CGitStatusListCtrl::IDSVNLC_GNUDIFF2)|
							SVNSLC_POPCONFLICT|SVNSLC_POPRESOLVE),false);


	//----------  Create Commit Out List Ctrl---------------
			
	dwStyle =LVS_REPORT | LVS_SHOWSELALWAYS | LVS_ALIGNLEFT | LVS_OWNERDATA | WS_BORDER | WS_TABSTOP | WS_CHILD | WS_VISIBLE;;

	if( !m_OutLogList.Create(dwStyle,rectDummy,&m_ctrlTabCtrl,IDC_OUT_LOGLIST))
	{
		TRACE0("Failed to create output commits window\n");
		return FALSE;      // fail to create

	}

	m_ctrlTabCtrl.InsertTab(&m_OutLogList,_T("Out Commits"),-1);
	

	m_OutLogList.InsertGitColumn();

	//------------- Create Change File List Control ----------------

	dwStyle = LVS_REPORT | LVS_SHOWSELALWAYS | LVS_ALIGNLEFT | WS_BORDER | WS_TABSTOP |LVS_SINGLESEL |WS_CHILD | WS_VISIBLE;
	
	if( !m_OutChangeFileList.Create(dwStyle,rectDummy,&m_ctrlTabCtrl,IDC_OUT_CHANGELIST))
	{
		TRACE0("Failed to create output change files window\n");
		return FALSE;      // fail to create
	}
	m_ctrlTabCtrl.InsertTab(&m_OutChangeFileList,_T("Out ChangeList"),-1);

	m_OutChangeFileList.Init(SVNSLC_COLEXT | SVNSLC_COLSTATUS |SVNSLC_COLADD|SVNSLC_COLDEL , _T("OutSyncDlg"),
		                    (CGitStatusListCtrl::GetContextMenuBit(CGitStatusListCtrl::IDSVNLC_COMPARETWO)|
							CGitStatusListCtrl::GetContextMenuBit(CGitStatusListCtrl::IDSVNLC_GNUDIFF2)),false);

	this->m_tooltips.Create(this);

	AddAnchor(IDC_SYNC_TAB,TOP_LEFT,BOTTOM_RIGHT);

	AddAnchor(IDC_GROUP_INFO,TOP_LEFT,TOP_RIGHT);
	AddAnchor(IDC_COMBOBOXEX_URL,TOP_LEFT,TOP_RIGHT);
	AddAnchor(IDC_BUTTON_MANAGE,TOP_RIGHT);
	AddAnchor(IDC_BUTTON_PULL,BOTTOM_LEFT);
	AddAnchor(IDC_BUTTON_PUSH,BOTTOM_LEFT);
	AddAnchor(IDC_BUTTON_SUBMODULE,BOTTOM_LEFT);
	AddAnchor(IDC_BUTTON_APPLY,BOTTOM_RIGHT);
	AddAnchor(IDC_BUTTON_EMAIL,BOTTOM_RIGHT);
	AddAnchor(IDC_PROGRESS_SYNC,TOP_LEFT,TOP_RIGHT);
	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDHELP,BOTTOM_RIGHT);
	AddAnchor(IDC_STATIC_STATUS,BOTTOM_LEFT);
	AddAnchor(IDC_ANIMATE_SYNC,TOP_LEFT);
	AddAnchor(IDC_BUTTON_COMMIT,BOTTOM_LEFT);

	BRANCH_COMBOX_ADD_ANCHOR();

	this->GetDlgItem(IDC_BUTTON_COMMIT)->ShowWindow(SW_HIDE);

	CString WorkingDir=g_Git.m_CurrentDir;
	WorkingDir.Replace(_T(':'),_T('_'));
	m_RegKeyRemoteBranch = CString(_T("Software\\TortoiseGit\\History\\SyncBranch\\"))+WorkingDir;


	this->AddOthersToAnchor();
	// TODO:  Add extra initialization here

	this->m_ctrlPush.AddEntry(CString(_T("Pus&h")));
	this->m_ctrlPush.AddEntry(CString(_T("Push ta&gs")));
	///this->m_ctrlPush.AddEntry(CString(_T("Push All")));

	this->m_ctrlPull.AddEntry(CString(_T("&Pull")));
	this->m_ctrlPull.AddEntry(CString(_T("Fetc&h")));
	this->m_ctrlPull.AddEntry(CString(_T("Fetch&&Re&base")));
	this->m_ctrlPull.AddEntry(CString(_T("Remote Update")));

	this->m_ctrlSubmodule.AddEntry(CString(_T("Submodule Update")));
	this->m_ctrlSubmodule.AddEntry(CString(_T("Submodule Init")));
	this->m_ctrlSubmodule.AddEntry(CString(_T("Submodule Sync")));
	
	WorkingDir.Replace(_T(':'),_T('_'));

	CString regkey ;
	regkey.Format(_T("Software\\TortoiseGit\\TortoiseProc\\Sync\\%s"),WorkingDir);

	this->m_regPullButton = CRegDWORD(regkey+_T("\\Pull"),0);
	this->m_regPushButton = CRegDWORD(regkey+_T("\\Push"),0);
	this->m_regSubmoduleButton = CRegDWORD(regkey+_T("\\Submodule"));

	this->m_ctrlPull.SetCurrentEntry(this->m_regPullButton);
	this->m_ctrlPush.SetCurrentEntry(this->m_regPushButton);
	this->m_ctrlSubmodule.SetCurrentEntry(this->m_regSubmoduleButton);

	CString str;
	this->GetWindowText(str);
	str += _T(" - ") + g_Git.m_CurrentDir;
	this->SetWindowText(str);

	EnableSaveRestore(_T("SyncDlg"));

	this->m_ctrlURL.LoadHistory(CString(_T("Software\\TortoiseGit\\History\\SyncURL\\"))+WorkingDir, _T("url"));

	STRING_VECTOR list;
	
	if(!g_Git.GetRemoteList(list))
	{	
		for(unsigned int i=0;i<list.size();i++)
		{
			m_ctrlURL.AddString(list[i]);
		}
	}	
	m_ctrlURL.SetCurSel(0);
	m_ctrlRemoteBranch.SetCurSel(0);
	m_ctrlURL.SetURLHistory(true);
	
	this->LoadBranchInfo();

	this->m_bInited=true;
	FetchOutList();
	
	m_ctrlTabCtrl.ShowTab(IDC_CMD_LOG-1,false);
	m_ctrlTabCtrl.ShowTab(IDC_IN_LOGLIST-1,false);
	m_ctrlTabCtrl.ShowTab(IDC_IN_CHANGELIST-1,false);
	m_ctrlTabCtrl.ShowTab(IDC_IN_CONFLICT-1,false);
		

	m_ctrlRemoteBranch.m_bWantReturn = TRUE;
	m_ctrlURL.m_bWantReturn = TRUE;

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CSyncDlg::OnBnClickedButtonManage()
{
	// TODO: Add your control notification handler code here
	CAppUtils::LaunchRemoteSetting();
}

void CSyncDlg::Refresh()
{
	theApp.DoWaitCursor(1);
	
	CString local;
	CString remote;
	CString url;
	this->m_ctrlLocalBranch.GetWindowText(local);
	this->m_ctrlRemoteBranch.GetWindowText(remote);
	this->m_ctrlURL.GetWindowText(url);

	this->LoadBranchInfo();

	this->m_ctrlLocalBranch.AddString(local);
	this->m_ctrlRemoteBranch.AddString(remote);
	this->m_ctrlURL.AddString(url);

	m_OutLogList.ShowText(_T("Refresh ..."));
	this->FetchOutList(true);
	theApp.DoWaitCursor(-1);
}

BOOL CSyncDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
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

	if(IsURL())
	{
		CString str;
		str=_T("Don't know what will push because you enter URL");
		m_OutLogList.ShowText(str);
		this->m_ctrlTabCtrl.ShowTab(m_OutChangeFileList.GetDlgCtrlID()-1,FALSE);
		m_OutLocalBranch.Empty();
		m_OutRemoteBranch.Empty();

		this->GetDlgItem(IDC_BUTTON_EMAIL)->EnableWindow(FALSE);
		return ;
	
	}else if(g_Git.GetHash(remotebranch).GetLength()<40)
	{
		CString str;
		str.Format(_T("Don't know what will push because unknown \"%s\""),remotebranch);
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
			m_OutLogList.FillGitLog(NULL,CGit::	LOG_INFO_STAT| CGit::LOG_INFO_FILESTATE | CGit::LOG_INFO_SHOW_MERGEDFILE,
				&remotebranch,&localbranch);
			
			CString str;
			if(m_OutLogList.GetItemCount() == 0)
			{			
				str.Format(_T("No commits ahead \"%s\""),remotebranch);
				m_OutLogList.ShowText(str);
				this->m_ctrlStatus.SetWindowText(str);
				this->m_ctrlTabCtrl.ShowTab(m_OutChangeFileList.GetDlgCtrlID()-1,FALSE);
				this->GetDlgItem(IDC_BUTTON_EMAIL)->EnableWindow(FALSE);
			}
			else
			{
				str.Format(_T("%d commits ahead \"%s\""),m_OutLogList.GetItemCount(),remotebranch);
				this->m_ctrlStatus.SetWindowText(str);

				AddDiffFileList(&m_OutChangeFileList,&m_arOutChangeList,localbranch,remotebranch);
				
				this->m_ctrlTabCtrl.ShowTab(m_OutChangeFileList.GetDlgCtrlID()-1,TRUE);
				this->GetDlgItem(IDC_BUTTON_EMAIL)->EnableWindow(TRUE);
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
void CSyncDlg::OnCbenEndeditComboboxexUrl(NMHDR *pNMHDR, LRESULT *pResult)
{
	// TODO: Add your control notification handler code here
	*pResult = 0;
}

void CSyncDlg::OnCbnEditchangeComboboxex()
{
	SetTimer(IDT_INPUT, 1000, NULL);
	this->m_OutLogList.ShowText(_T("Wait for input"));

	//this->FetchOutList();
	// TODO: Add your control notification handler code here
}

UINT CSyncDlg::ProgressThread()
{
	m_GitCmdStatus=CProgressDlg::RunCmdList(this,m_GitCmdList,true,NULL,&this->m_bAbort);
	InterlockedExchange(&m_bBlock, FALSE);
	return 0;
}


LRESULT CSyncDlg::OnProgressUpdateUI(WPARAM wParam,LPARAM lParam)
{
	if(wParam == MSG_PROGRESSDLG_START)
	{
		m_ctrlAnimate.Play(0,-1,-1);
		this->m_ctrlProgress.SetPos(0);
	}

	if(wParam == MSG_PROGRESSDLG_END || wParam == MSG_PROGRESSDLG_FAILED)
	{
		//m_bDone = true;
		m_ctrlAnimate.Stop();
		m_ctrlProgress.SetPos(100);
		//this->DialogEnableWindow(IDOK,TRUE);

		//if(wParam == MSG_PROGRESSDLG_END)
		if(this->m_CurrentCmd == GIT_COMMAND_PUSH )
		{
			EnableControlButton(true);
			SwitchToInput();
			this->FetchOutList(true);
		}
		if(this->m_CurrentCmd == GIT_COMMAND_PULL )
		{
			PullComplete();
		}
		if(this->m_CurrentCmd == GIT_COMMAND_FETCH || this->m_CurrentCmd == GIT_COMMAND_FETCHANDREBASE)
		{
			FetchComplete();
		}
		if(this->m_CurrentCmd == GIT_COMMAND_SUBMODULE)
		{
			//this->m_ctrlCmdOut.SetSel(-1,-1);
			//this->m_ctrlCmdOut.ReplaceSel(_T("Done\r\n"));
			//this->m_ctrlCmdOut.SetSel(-1,-1);
			EnableControlButton(true);
			SwitchToInput();
		}
		if(this->m_CurrentCmd == GIT_COMMAND_REMOTE)
		{
			this->FetchOutList(true);
			EnableControlButton(true);
			SwitchToInput();
		}
	}

	if(lParam != 0)
		ParserCmdOutput((TCHAR)lParam);

	return 0;
}


void CSyncDlg::ParserCmdOutput(char ch)
{
	CProgressDlg::ParserCmdOutput(m_ctrlCmdOut,m_ctrlProgress,m_LogText,ch);
}
void CSyncDlg::OnBnClickedButtonCommit()
{
	// TODO: Add your control notification handler code here
    CString proc=CPathUtils::GetAppDirectory();
    proc += _T("TortoiseProc.exe /command:commit");
    proc += _T(" /path:\"");
    proc += g_Git.m_CurrentDir;
    
	CAppUtils::LaunchApplication(proc,IDS_ERROR_CANNON_FIND_TORTOISEPROC,false);
}

void CSyncDlg::OnOK()
{
	// TODO: Add your specialized code here and/or call the base class
	UpdateCombox();
	m_ctrlURL.SaveHistory();
	SaveHistory();
	__super::OnOK();
}

void CSyncDlg::OnBnClickedButtonSubmodule()
{
	// TODO: Add your control notification handler code here
		// TODO: Add your control notification handler code here
	this->UpdateData();
	UpdateCombox();

	this->m_regSubmoduleButton = this->m_ctrlSubmodule.GetCurrentEntry();

	this->SwitchToRun();
	
	this->m_bAbort=false;
	this->m_GitCmdList.clear();

	ShowTab(IDC_CMD_LOG);

	CString cmd;

	switch (m_ctrlSubmodule.GetCurrentEntry())
	{
	case 0:
		cmd=_T("git.exe submodule update");
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


void CSyncDlg::OnTimer(UINT_PTR nIDEvent)
{
	if( nIDEvent == IDT_INPUT)
	{
		KillTimer(IDT_INPUT);
		this->FetchOutList(true);
	}
}