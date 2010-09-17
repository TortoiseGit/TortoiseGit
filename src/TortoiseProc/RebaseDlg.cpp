// RebaseDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "RebaseDlg.h"
#include "AppUtils.h"
#include "MessageBox.h"
#include "UnicodeUtils.h"
#include "BrowseRefsDlg.h"
#include "ProgressDlg.h"
// CRebaseDlg dialog

IMPLEMENT_DYNAMIC(CRebaseDlg, CResizableStandAloneDialog)

CRebaseDlg::CRebaseDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CRebaseDlg::IDD, pParent)
    , m_bPickAll(FALSE)
    , m_bSquashAll(FALSE)
    , m_bEditAll(FALSE)
{
	m_RebaseStage=CHOOSE_BRANCH;
	m_CurrentRebaseIndex=-1;
	m_bThreadRunning =FALSE;
	this->m_IsCherryPick = FALSE;
	m_bForce=FALSE;
	m_IsFastForward=FALSE;
}

CRebaseDlg::~CRebaseDlg()
{
}

void CRebaseDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_REBASE_PROGRESS, m_ProgressBar);
    DDX_Control(pDX, IDC_STATUS_STATIC, m_CtrlStatusText);
    DDX_Check(pDX, IDC_PICK_ALL, m_bPickAll);
    DDX_Check(pDX, IDC_SQUASH_ALL, m_bSquashAll);
    DDX_Check(pDX, IDC_EDIT_ALL, m_bEditAll);
	DDX_Control(pDX, IDC_REBASE_SPLIT, m_wndSplitter);
	DDX_Control(pDX,IDC_COMMIT_LIST,m_CommitList);
	DDX_Control(pDX,IDC_REBASE_COMBOXEX_BRANCH, this->m_BranchCtrl);
	DDX_Control(pDX,IDC_REBASE_COMBOXEX_UPSTREAM,   this->m_UpstreamCtrl);
	DDX_Check(pDX, IDC_REBASE_CHECK_FORCE,m_bForce);
	DDX_Control(pDX,IDC_REBASE_POST_BUTTON,m_PostButton);
}


BEGIN_MESSAGE_MAP(CRebaseDlg, CResizableStandAloneDialog)
    ON_BN_CLICKED(IDC_PICK_ALL, &CRebaseDlg::OnBnClickedPickAll)
    ON_BN_CLICKED(IDC_SQUASH_ALL, &CRebaseDlg::OnBnClickedSquashAll)
    ON_BN_CLICKED(IDC_EDIT_ALL, &CRebaseDlg::OnBnClickedEditAll)
    ON_BN_CLICKED(IDC_REBASE_SPLIT, &CRebaseDlg::OnBnClickedRebaseSplit)
	ON_BN_CLICKED(IDC_REBASE_CONTINUE,OnBnClickedContinue)
	ON_BN_CLICKED(IDC_REBASE_ABORT,  OnBnClickedAbort)
	ON_WM_SIZE()
	ON_CBN_SELCHANGE(IDC_REBASE_COMBOXEX_BRANCH,   &CRebaseDlg::OnCbnSelchangeBranch)
	ON_CBN_SELCHANGE(IDC_REBASE_COMBOXEX_UPSTREAM, &CRebaseDlg::OnCbnSelchangeUpstream)
	ON_MESSAGE(MSG_REBASE_UPDATE_UI, OnRebaseUpdateUI)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE, &CRebaseDlg::OnBnClickedButtonBrowse)
	ON_BN_CLICKED(IDC_REBASE_CHECK_FORCE, &CRebaseDlg::OnBnClickedRebaseCheckForce)
	ON_STN_CLICKED(IDC_STATUS_STATIC, &CRebaseDlg::OnStnClickedStatusStatic)
	ON_BN_CLICKED(IDC_REBASE_POST_BUTTON, &CRebaseDlg::OnBnClickedRebasePostButton)
END_MESSAGE_MAP()

void CRebaseDlg::AddRebaseAnchor()
{
	AddAnchor(IDC_REBASE_TAB,TOP_LEFT,BOTTOM_RIGHT);
	AddAnchor(IDC_COMMIT_LIST,TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_REBASE_SPLIT,TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_STATUS_STATIC, BOTTOM_LEFT,BOTTOM_RIGHT);
	AddAnchor(IDC_REBASE_CONTINUE,BOTTOM_RIGHT);
	AddAnchor(IDC_REBASE_ABORT, BOTTOM_RIGHT);
	AddAnchor(IDC_REBASE_PROGRESS,BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_PICK_ALL,TOP_LEFT);
	AddAnchor(IDC_SQUASH_ALL,TOP_LEFT);
	AddAnchor(IDC_EDIT_ALL,TOP_LEFT);	
	AddAnchor(IDC_REBASE_COMBOXEX_UPSTREAM,TOP_LEFT);
	AddAnchor(IDC_REBASE_COMBOXEX_BRANCH,TOP_LEFT);
	AddAnchor(IDC_REBASE_STATIC_UPSTREAM,TOP_LEFT);
	AddAnchor(IDC_REBASE_STATIC_BRANCH,TOP_LEFT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	AddAnchor(IDC_REBASE_CHECK_FORCE,TOP_RIGHT);
	AddAnchor(IDC_REBASE_POST_BUTTON,BOTTOM_LEFT);
	
	this->AddOthersToAnchor();
}

BOOL CRebaseDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	CRect rectDummy;
	//IDC_REBASE_DUMY_TAB
	
	GetClientRect(m_DlgOrigRect);
	m_CommitList.GetClientRect(m_CommitListOrigRect);

	CWnd *pwnd=this->GetDlgItem(IDC_REBASE_DUMY_TAB);
	pwnd->GetWindowRect(&rectDummy);
	this->ScreenToClient(rectDummy);

	if (!m_ctrlTabCtrl.Create(CMFCTabCtrl::STYLE_FLAT, rectDummy, this, IDC_REBASE_TAB))
	{
		TRACE0("Failed to create output tab window\n");
		return FALSE;      // fail to create
	}
	m_ctrlTabCtrl.SetResizeMode(CMFCTabCtrl::RESIZE_NO);
	// Create output panes:
	//const DWORD dwStyle = LBS_NOINTEGRALHEIGHT | WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL;
	DWORD dwStyle =LVS_REPORT | LVS_SHOWSELALWAYS | LVS_ALIGNLEFT | WS_BORDER | WS_TABSTOP |LVS_SINGLESEL |WS_CHILD | WS_VISIBLE;

	if (! this->m_FileListCtrl.Create(dwStyle,rectDummy,&this->m_ctrlTabCtrl,0) )
	{
		TRACE0("Failed to create output windows\n");
		return FALSE;      // fail to create
	}

	if( ! this->m_LogMessageCtrl.Create(_T("Scintilla"),_T("source"),0,rectDummy,&m_ctrlTabCtrl,0,0) )
	{
		TRACE0("Failed to create log message control");
		return FALSE;
	}
	m_LogMessageCtrl.Init(0);

	dwStyle = LBS_NOINTEGRALHEIGHT | WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL;

	if (!m_wndOutputRebase.Create(_T("Scintilla"),_T("source"),0,rectDummy, &m_ctrlTabCtrl, 0,0) )
	{
		TRACE0("Failed to create output windows\n");
		return -1;      // fail to create
	}
	m_wndOutputRebase.Init(0);
	m_wndOutputRebase.Call(SCI_SETREADONLY, TRUE);
	
	m_tooltips.Create(this);
	
	m_tooltips.AddTool(IDC_REBASE_CHECK_FORCE,IDS_REBASE_FORCE_TT);
	m_tooltips.AddTool(IDC_REBASE_ABORT,IDS_REBASE_ABORT_TT);
	


	m_FileListCtrl.Init(SVNSLC_COLEXT | SVNSLC_COLSTATUS |SVNSLC_COLADD|SVNSLC_COLDEL , _T("RebaseDlg"),(SVNSLC_POPALL ^ SVNSLC_POPCOMMIT),false);

	m_ctrlTabCtrl.AddTab(&m_FileListCtrl,_T("Conflict File"));
	m_ctrlTabCtrl.AddTab(&m_LogMessageCtrl,_T("Commit Message"),1);
	m_ctrlTabCtrl.AddTab(&m_wndOutputRebase,_T("Log"),2);
	AddRebaseAnchor();


	EnableSaveRestore(_T("RebaseDlg"));

	DWORD yPos = CRegDWORD(_T("Software\\TortoiseGit\\TortoiseProc\\ResizableState\\RebaseDlgSizer"));
	RECT rcDlg, rcLogMsg, rcFileList;
	GetClientRect(&rcDlg);
	m_CommitList.GetWindowRect(&rcLogMsg);
	ScreenToClient(&rcLogMsg);
	this->m_ctrlTabCtrl.GetWindowRect(&rcFileList);
	ScreenToClient(&rcFileList);
	if (yPos)
	{
		RECT rectSplitter;
		m_wndSplitter.GetWindowRect(&rectSplitter);
		ScreenToClient(&rectSplitter);
		int delta = yPos - rectSplitter.top;
		if ((rcLogMsg.bottom + delta > rcLogMsg.top)&&(rcLogMsg.bottom + delta < rcFileList.bottom - 30))
		{
			m_wndSplitter.SetWindowPos(NULL, 0, yPos, 0, 0, SWP_NOSIZE);
			DoSize(delta);
		}
	}

	if( this->m_RebaseStage == CHOOSE_BRANCH)
	{
		this->LoadBranchInfo();

	}else
	{
		this->m_BranchCtrl.EnableWindow(FALSE);
		this->m_UpstreamCtrl.EnableWindow(FALSE);
	}
	
	m_CommitList.m_ColumnRegKey = _T("Rebase");
	m_CommitList.m_IsIDReplaceAction = TRUE;
//	m_CommitList.m_IsOldFirst = TRUE;
	m_CommitList.m_IsRebaseReplaceGraph = TRUE;

	m_CommitList.InsertGitColumn();

	this->SetControlEnable();

	if(!this->m_PreCmd.IsEmpty())
	{
		CProgressDlg progress;
		progress.m_GitCmd=m_PreCmd;
		progress.m_bAutoCloseOnSuccess=true;
		progress.DoModal();
	}

	if(m_IsCherryPick)
	{
		this->m_BranchCtrl.SetCurSel(-1);
		this->m_BranchCtrl.EnableWindow(FALSE);
		this->m_UpstreamCtrl.EnableWindow(FALSE);
		this->SetWindowText(_T("Cherry Pick"));
		this->m_CommitList.StartFilter();

	}else
	{
		SetContinueButtonText();
		m_CommitList.DeleteAllItems();
		FetchLogList();
	}

	m_CommitList.m_ContextMenuMask &= ~(m_CommitList.GetContextMenuBit(CGitLogListBase::ID_CHERRY_PICK)|
										m_CommitList.GetContextMenuBit(CGitLogListBase::ID_SWITCHTOREV)|
										m_CommitList.GetContextMenuBit(CGitLogListBase::ID_RESET)|
										m_CommitList.GetContextMenuBit(CGitLogListBase::ID_REVERTREV)|
										m_CommitList.GetContextMenuBit(CGitLogListBase::ID_REBASE_TO_VERSION)|
										m_CommitList.GetContextMenuBit(CGitLogListBase::ID_REVERTTOREV)|
										m_CommitList.GetContextMenuBit(CGitLogListBase::ID_COMBINE_COMMIT));

	if(m_CommitList.m_IsOldFirst)
		this->m_CurrentRebaseIndex = -1;
	else
		this->m_CurrentRebaseIndex = m_CommitList.m_logEntries.size();

	
	if(this->CheckRebaseCondition())
	{
		/* Disable Start Rebase */
		this->GetDlgItem(IDC_REBASE_CONTINUE)->EnableWindow(FALSE);
	}

	return TRUE;
}
// CRebaseDlg message handlers

void CRebaseDlg::OnBnClickedPickAll()
{
    // TODO: Add your control notification handler code here
	this->UpdateData();
	if(this->m_bPickAll)
		this->SetAllRebaseAction(CTGitPath::LOGACTIONS_REBASE_PICK);

	this->m_bEditAll=FALSE;
	this->m_bSquashAll=FALSE;
	this->UpdateData(FALSE);
	
}

void CRebaseDlg::OnBnClickedSquashAll()
{
    // TODO: Add your control notification handler code here
	this->UpdateData();
	if(this->m_bSquashAll)
		this->SetAllRebaseAction(CTGitPath::LOGACTIONS_REBASE_SQUASH);

	this->m_bEditAll=FALSE;
	this->m_bPickAll=FALSE;
	this->UpdateData(FALSE);

}

void CRebaseDlg::OnBnClickedEditAll()
{
    // TODO: Add your control notification handler code here
	this->UpdateData();
	if( this->m_bEditAll )
		this->SetAllRebaseAction(CTGitPath::LOGACTIONS_REBASE_EDIT);

	this->m_bPickAll=FALSE;
	this->m_bSquashAll=FALSE;
	this->UpdateData(FALSE);

}

void CRebaseDlg::SetAllRebaseAction(int action)
{
	for(int i=0;i<this->m_CommitList.m_logEntries.size();i++)
	{
		m_CommitList.m_logEntries.GetGitRevAt(i).m_Action=action;
	}
	m_CommitList.Invalidate();
}

void CRebaseDlg::OnBnClickedRebaseSplit()
{
	this->UpdateData();
    // TODO: Add your control notification handler code here
}

LRESULT CRebaseDlg::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_NOTIFY:
		if (wParam == IDC_REBASE_SPLIT)
		{ 
			SPC_NMHDR* pHdr = (SPC_NMHDR*) lParam;
			DoSize(pHdr->delta);
		}
		break;
	}

	return __super::DefWindowProc(message, wParam, lParam);
}

void CRebaseDlg::DoSize(int delta)
{
	
	this->RemoveAllAnchors();

	CSplitterControl::ChangeHeight(GetDlgItem(IDC_COMMIT_LIST), delta, CW_TOPALIGN);
	//CSplitterControl::ChangeHeight(GetDlgItem(), delta, CW_TOPALIGN);
	CSplitterControl::ChangeHeight(GetDlgItem(IDC_REBASE_TAB), -delta, CW_BOTTOMALIGN);
	//CSplitterControl::ChangeHeight(GetDlgItem(), -delta, CW_BOTTOMALIGN);
	CSplitterControl::ChangePos(GetDlgItem(IDC_SQUASH_ALL),0,delta);
	CSplitterControl::ChangePos(GetDlgItem(IDC_PICK_ALL),0,delta);
	CSplitterControl::ChangePos(GetDlgItem(IDC_EDIT_ALL),0,delta);
	CSplitterControl::ChangePos(GetDlgItem(IDC_REBASE_CHECK_FORCE),0,delta);
	
	this->AddRebaseAnchor();
	// adjust the minimum size of the dialog to prevent the resizing from
	// moving the list control too far down.
	CRect rcLogMsg;
	m_CommitList.GetClientRect(rcLogMsg);
	SetMinTrackSize(CSize(m_DlgOrigRect.Width(), m_DlgOrigRect.Height()-m_CommitListOrigRect.Height()+rcLogMsg.Height()));

	SetSplitterRange();
//	m_CommitList.Invalidate();

//	GetDlgItem(IDC_LOGMESSAGE)->Invalidate();

	this->m_ctrlTabCtrl.Invalidate();
	this->m_CommitList.Invalidate();
	this->m_FileListCtrl.Invalidate();
	this->m_LogMessageCtrl.Invalidate();

}

void CRebaseDlg::SetSplitterRange()
{
	if ((m_CommitList)&&(m_ctrlTabCtrl))
	{
		CRect rcTop;
		m_CommitList.GetWindowRect(rcTop);
		ScreenToClient(rcTop);
		CRect rcMiddle;
		m_ctrlTabCtrl.GetWindowRect(rcMiddle);
		ScreenToClient(rcMiddle);
		if (rcMiddle.Height() && rcMiddle.Width())
			m_wndSplitter.SetRange(rcTop.top+60, rcMiddle.bottom-80);
	}
}

void CRebaseDlg::OnSize(UINT nType,int cx, int cy)
{
	 // first, let the resizing take place
    __super::OnSize(nType, cx, cy);

    //set range
    SetSplitterRange();
}

void CRebaseDlg::SaveSplitterPos()
{
	if (!IsIconic())
	{
		CRegDWORD regPos = CRegDWORD(_T("Software\\TortoiseGit\\TortoiseProc\\ResizableState\\RebaseDlgSizer"));
		RECT rectSplitter;
		m_wndSplitter.GetWindowRect(&rectSplitter);
		ScreenToClient(&rectSplitter);
		regPos = rectSplitter.top;
	}
}

void CRebaseDlg::LoadBranchInfo()
{
	m_BranchCtrl.SetMaxHistoryItems(0x7FFFFFFF);
	m_UpstreamCtrl.SetMaxHistoryItems(0x7FFFFFFF);

	STRING_VECTOR list;
	list.clear();
	int current;
	g_Git.GetBranchList(list,&current,CGit::BRANCH_ALL);
	m_BranchCtrl.AddString(list);
	m_UpstreamCtrl.AddString(list);

	m_BranchCtrl.SetCurSel(current);

	AddBranchToolTips(&m_BranchCtrl);
	AddBranchToolTips(&m_UpstreamCtrl);

	if(!m_Upstream.IsEmpty())
	{
		m_UpstreamCtrl.AddString(m_Upstream);
		m_UpstreamCtrl.SetCurSel(m_UpstreamCtrl.GetCount()-1);
	}
	else
	{
		//Select pull-remote from current branch
		CString currentBranch = g_Git.GetSymbolicRef();
		CString configName;
		configName.Format(L"branch.%s.remote", currentBranch);
		CString pullRemote = g_Git.GetConfigValue(configName);

		//Select pull-branch from current branch
		configName.Format(L"branch.%s.merge", currentBranch);
		CString pullBranch = CGit::StripRefName(g_Git.GetConfigValue(configName));

		CString defaultUpstream;
		defaultUpstream.Format(L"remotes/%s/%s", pullRemote, pullBranch);
		int found = m_UpstreamCtrl.FindStringExact(0, defaultUpstream);
		if(found >= 0)
			m_UpstreamCtrl.SetCurSel(found);
	}
}

void CRebaseDlg::OnCbnSelchangeBranch()
{
	FetchLogList();
}

void CRebaseDlg::OnCbnSelchangeUpstream()
{
	FetchLogList();
}

void CRebaseDlg::FetchLogList()
{
	CString base,hash;
	CString cmd;
	m_IsFastForward=FALSE;
	cmd.Format(_T("git.exe merge-base %s %s"), m_UpstreamCtrl.GetString(),m_BranchCtrl.GetString());
	if(g_Git.Run(cmd,&base,CP_ACP))
	{
		CMessageBox::Show(NULL,base,_T("TortoiseGit"),MB_OK|MB_ICONERROR);
		return;
	}
	base=base.Left(40);

	hash=g_Git.GetHash(m_BranchCtrl.GetString());

	if(hash == g_Git.GetHash(this->m_UpstreamCtrl.GetString()))
	{
		m_CommitList.Clear();
		CString text,fmt;
		fmt.LoadString(IDS_REBASE_EQUAL_FMT);
		text.Format(fmt,m_BranchCtrl.GetString(),this->m_UpstreamCtrl.GetString());

		m_CommitList.ShowText(text);
		this->GetDlgItem(IDC_REBASE_CONTINUE)->EnableWindow(false);
		return;
	}

	hash=hash.Left(40);
	
	if(hash == base )
	{
		//fast forword
		this->m_IsFastForward=TRUE;

		m_CommitList.Clear();
		CString text,fmt;
		fmt.LoadString(IDS_REBASE_FASTFORWARD_FMT);
		text.Format(fmt,m_BranchCtrl.GetString(),this->m_UpstreamCtrl.GetString(),
						m_BranchCtrl.GetString(),this->m_UpstreamCtrl.GetString());

		m_CommitList.ShowText(text);
		this->GetDlgItem(IDC_REBASE_CONTINUE)->EnableWindow(true);
		SetContinueButtonText();
		
		return ;
	}

	hash.Empty();

	if(!this->m_bForce)
	{
		cmd.Format(_T("git.exe rev-parse %s"), m_UpstreamCtrl.GetString());
		if( g_Git.Run(cmd,&hash,CP_ACP))
		{
			CMessageBox::Show(NULL,base,_T("TortoiseGit"),MB_OK|MB_ICONERROR);
			return;
		}
		hash=hash.Left(40);
		
		if( base == hash )
		{
			m_CommitList.Clear();
			CString text,fmt;
			fmt.LoadString(IDS_REBASE_UPTODATE_FMT);
			text.Format(fmt,m_BranchCtrl.GetString());
			m_CommitList.ShowText(text);
			this->GetDlgItem(IDC_REBASE_CONTINUE)->EnableWindow(m_CommitList.GetItemCount());
			SetContinueButtonText();
			return;
		}
	}

	m_CommitList.Clear();
	this->m_CommitList.FillGitLog(NULL,0,&m_UpstreamCtrl.GetString(),&m_BranchCtrl.GetString());
	if( m_CommitList.GetItemCount() == 0 )
		m_CommitList.ShowText(_T("Nothing to Rebase"));

	hash=g_Git.GetHash(m_UpstreamCtrl.GetString());
	
#if 0
	if(m_CommitList.m_logEntries[m_CommitList.m_logEntries.size()-1].m_ParentHash.size() >=0 )
	{
		if(hash ==  m_CommitList.m_logEntries[m_CommitList.m_logEntries.size()-1].m_ParentHash[0])
		{
			m_CommitList.Clear();
			m_CommitList.ShowText(_T("Nothing Rebase"));
		}
	}
#endif

	m_tooltips.Pop();
	AddBranchToolTips(&this->m_BranchCtrl);
	AddBranchToolTips(&this->m_UpstreamCtrl);
	
	for(int i=0;i<m_CommitList.m_logEntries.size();i++)
	{
		m_CommitList.m_logEntries.GetGitRevAt(i).m_Action = CTGitPath::LOGACTIONS_REBASE_PICK;
	}

	m_CommitList.Invalidate();

	if(m_CommitList.m_IsOldFirst)
		this->m_CurrentRebaseIndex = -1;
	else
		this->m_CurrentRebaseIndex = m_CommitList.m_logEntries.size();
	
	this->GetDlgItem(IDC_REBASE_CONTINUE)->EnableWindow(m_CommitList.GetItemCount());
	SetContinueButtonText();
}

void CRebaseDlg::AddBranchToolTips(CHistoryCombo *pBranch)
{
	if(pBranch)
	{
		CString text=pBranch->GetString();
		CString tooltip;
		BYTE_VECTOR data;
		g_Git.GetLog(data,text,NULL,1,0);
		GitRev rev;
		rev.ParserFromLog(data);
		tooltip.Format(_T("CommitHash:%s\nCommit by: %s  %s\n <b>%s</b> \n %s"),
			rev.m_CommitHash.ToString(),
			rev.m_AuthorName,
			CAppUtils::FormatDateAndTime(rev.m_AuthorDate,DATE_LONGDATE),
			rev.m_Subject,
			rev.m_Body);

		pBranch->DisableTooltip();
		this->m_tooltips.AddTool(pBranch->GetComboBoxCtrl(),tooltip);
	}
}

BOOL CRebaseDlg::PreTranslateMessage(MSG*pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		
		case VK_F5:
			{
				Refresh();
				return TRUE;
			}
			break;
		/* Avoid TAB control destroy but dialog exist*/
		case VK_ESCAPE:
		case VK_CANCEL:
			{
				TCHAR buff[128];
				::GetClassName(pMsg->hwnd,buff,128);
				
				
				if(_tcsnicmp(buff,_T("RichEdit20W"),128)==0 ||
				   _tcsnicmp(buff,_T("Scintilla"),128)==0 ||
				   _tcsnicmp(buff,_T("SysListView32"),128)==0||
				   ::GetParent(pMsg->hwnd) == this->m_ctrlTabCtrl.m_hWnd)	
				{
					this->PostMessage(WM_KEYDOWN,VK_ESCAPE,0);
					return TRUE;
				}
			}		
		}
	}
	m_tooltips.RelayEvent(pMsg);
	return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}
int CRebaseDlg::CheckRebaseCondition()
{
	this->m_ctrlTabCtrl.SetActiveTab(REBASE_TAB_LOG);

	if( !g_Git.CheckCleanWorkTree()  )
	{
		if(CMessageBox::Show(NULL,	IDS_ERROR_NOCLEAN_STASH,IDS_APPNAME,MB_YESNO|MB_ICONINFORMATION)==IDYES)
		{
			CString cmd,out;
			cmd=_T("git.exe stash");
			this->AddLogString(cmd);
			if(g_Git.Run(cmd,&out,CP_ACP))
			{
				CMessageBox::Show(NULL,out,_T("TortoiseGit"),MB_OK);
				return -1;
			}

		}else
			return -1;
	}
	//Todo Check $REBASE_ROOT
	//Todo Check $DOTEST

	CString cmd;
	cmd=_T("git.exe var GIT_COMMITTER_IDENT");
	if(g_Git.Run(cmd,NULL,CP_UTF8))
		return -1;

	//Todo call pre_rebase_hook
	return 0;
}
int CRebaseDlg::StartRebase()
{
	CString cmd,out;
	m_FileListCtrl.m_bIsRevertTheirMy = !m_IsCherryPick;
	if(!this->m_IsCherryPick)
	{
		//Todo call comment_for_reflog
		cmd.Format(_T("git.exe checkout %s"),this->m_BranchCtrl.GetString());
		this->AddLogString(cmd);

		if(g_Git.Run(cmd,&out,CP_UTF8))
			return -1;

		this->AddLogString(out);
	}

	cmd=_T("git.exe rev-parse --verify HEAD");
	if(g_Git.Run(cmd,&out,CP_UTF8))
	{
		AddLogString(_T("No Head"));
		return -1;
	}
	//Todo 
	//git symbolic-ref HEAD > "$DOTEST"/head-name 2> /dev/null ||
	//		echo "detached HEAD" > "$DOTEST"/head-name

	cmd.Format(_T("git.exe update-ref ORIG_HEAD HEAD"));
	if(g_Git.Run(cmd,&out,CP_UTF8))
	{
		AddLogString(_T("update ORIG_HEAD Fail"));
		return -1;
	}
	
	m_OrigUpstreamHash.Empty();
	m_OrigUpstreamHash= g_Git.GetHash(this->m_UpstreamCtrl.GetString());
	if(m_OrigUpstreamHash.IsEmpty())
	{
		this->AddLogString(m_OrigUpstreamHash);
		return -1;
	}

	if( !this->m_IsCherryPick )
	{
		cmd.Format(_T("git.exe checkout -f %s"), m_OrigUpstreamHash);
		this->AddLogString(cmd);

		out.Empty();
		if(g_Git.Run(cmd,&out,CP_UTF8))
		{
			this->AddLogString(out);
			return -1;
		}
	}
	
	
	if( !this->m_IsCherryPick )
	{
		cmd.Format(_T("git.exe rev-parse %s"),this->m_BranchCtrl.GetString());
		if(g_Git.Run(cmd,&this->m_OrigBranchHash,CP_UTF8))
		{
			this->AddLogString(m_OrigBranchHash);
			return -1;
		}
		this->AddLogString(_T("Start Rebase\r\n"));

	}else
		this->AddLogString(_T("Start Cherry-pick\r\n"));
	
	return 0;
}
int  CRebaseDlg::VerifyNoConflict()
{
	CTGitPathList list;
	if(g_Git.ListConflictFile(list))
	{
		AddLogString(_T("Get conflict files fail"));
		return -1;
	}
	if( list.GetCount() != 0 )
	{
		CMessageBox::Show(NULL,_T("There are conflict file, you should mark it resolve"),_T("TortoiseGit"),MB_OK);
		return -1;
	}
	return 0;

}
int CRebaseDlg::FinishRebase()
{
	if(this->m_IsCherryPick) //cherry pick mode no "branch", working at upstream branch
		return 0;

	git_revnum_t head = g_Git.GetHash(CString(_T("HEAD")));
	CString out,cmd;

	out.Empty();
	cmd.Format(_T("git.exe checkout -f %s"),this->m_BranchCtrl.GetString());
	AddLogString(cmd);
	if(g_Git.Run(cmd,&out,CP_UTF8))
	{
		AddLogString(out);
		return -1;
	}
	AddLogString(out);

	out.Empty();
	cmd.Format(_T("git.exe reset --hard %s"),head);
	AddLogString(cmd);
	if(g_Git.Run(cmd,&out,CP_UTF8))
	{
		AddLogString(out);
		return -1;
	}
	AddLogString(out);

	return 0;
}
void CRebaseDlg::OnBnClickedContinue()
{
	if( m_RebaseStage == REBASE_DONE)
	{
		OnOK();
		return;
	}

	if( this->m_IsFastForward )
	{
		CString cmd,out;
		CString oldbranch = g_Git.GetCurrentBranch();
		if( oldbranch != m_BranchCtrl.GetString() )
		{
			cmd.Format(_T("git.exe checkout %s"),m_BranchCtrl.GetString());
			AddLogString(cmd);
			if( g_Git.Run(cmd,&out,CP_ACP) )
			{
				this->m_ctrlTabCtrl.SetActiveTab(REBASE_TAB_LOG);
				AddLogString(out);
				return;
			}
		}
		AddLogString(out);
		out.Empty();
		m_OrigBranchHash = g_Git.GetHash(m_BranchCtrl.GetString());
		m_OrigUpstreamHash = g_Git.GetHash(this->m_UpstreamCtrl.GetString());
			
		if(!g_Git.IsFastForward(this->m_BranchCtrl.GetString(),this->m_UpstreamCtrl.GetString()))
		{
			this->m_ctrlTabCtrl.SetActiveTab(REBASE_TAB_LOG);
			AddLogString(_T("No fast forward\r\nMaybe repository changed"));
			return;
		}
		
		cmd.Format(_T("git.exe reset --hard %s"),this->m_UpstreamCtrl.GetString());
		this->AddLogString(CString(_T("Fast forward to "))+m_UpstreamCtrl.GetString());

		AddLogString(cmd);
		this->m_ctrlTabCtrl.SetActiveTab(REBASE_TAB_LOG);
		if(g_Git.Run(cmd,&out,CP_ACP))
		{
			AddLogString(_T("Fail"));
			AddLogString(out);
			return;
		}
		AddLogString(out);
		AddLogString(_T("Done"));
		m_RebaseStage = REBASE_DONE;
		UpdateCurrentStatus();
		return;

	}
	if( m_RebaseStage == CHOOSE_BRANCH|| m_RebaseStage == CHOOSE_COMMIT_PICK_MODE )
	{
		if(CheckRebaseCondition())
			return ;
		m_RebaseStage = REBASE_START;
	}


	if( m_RebaseStage == REBASE_FINISH )
	{
		if(FinishRebase())
			return ;

		OnOK();
	}

	if( m_RebaseStage == REBASE_SQUASH_CONFLICT)
	{
		if(VerifyNoConflict())
			return;
		GitRev *curRev=(GitRev*)m_CommitList.m_arShownList[m_CurrentRebaseIndex];
		if(this->CheckNextCommitIsSquash())
		{//next commit is not squash;
			m_RebaseStage = REBASE_SQUASH_EDIT;
			this->OnRebaseUpdateUI(0,0);
			this->UpdateCurrentStatus();
			return ;

		}
		m_RebaseStage=REBASE_CONTINUE;
		curRev->m_Action|=CTGitPath::LOGACTIONS_REBASE_DONE;
		this->UpdateCurrentStatus();

	}

	if( m_RebaseStage == REBASE_CONFLICT )
	{
		if(VerifyNoConflict())
			return;

		GitRev *curRev=(GitRev*)m_CommitList.m_arShownList[m_CurrentRebaseIndex];
		
		CString out =_T("");
		CString cmd;
		cmd.Format(_T("git.exe commit -C %s"), curRev->m_CommitHash.ToString());

		AddLogString(cmd);

		if(g_Git.Run(cmd,&out,CP_UTF8))
		{
			AddLogString(out);
			if(!g_Git.CheckCleanWorkTree())
			{
				CMessageBox::Show(NULL,out,_T("TortoiseGit"),MB_OK|MB_ICONERROR);
				return;
			}
		}

		AddLogString(out);
		this->m_ctrlTabCtrl.SetActiveTab(REBASE_TAB_LOG);
		if( curRev->m_Action & CTGitPath::LOGACTIONS_REBASE_EDIT)
		{
			m_RebaseStage=REBASE_EDIT;
			this->m_ctrlTabCtrl.SetActiveTab(REBASE_TAB_MESSAGE);
			this->UpdateCurrentStatus();
			return;
		}
		else
		{
			m_RebaseStage=REBASE_CONTINUE;
			curRev->m_Action|=CTGitPath::LOGACTIONS_REBASE_DONE;
			this->UpdateCurrentStatus();
		}
		
	}

	if( m_RebaseStage == REBASE_EDIT ||  m_RebaseStage == REBASE_SQUASH_EDIT )
	{
		CString str;
		GitRev *curRev=(GitRev*)m_CommitList.m_arShownList[m_CurrentRebaseIndex];
	
		str=this->m_LogMessageCtrl.GetText();
		if(str.Trim().IsEmpty())
		{
			CMessageBox::Show(NULL,_T("Commit Message Is Empty"),_T("TortoiseGit"),MB_OK|MB_ICONERROR);
				return;
		}

		CString tempfile=::GetTempFile();
		CFile file(tempfile,CFile::modeReadWrite|CFile::modeCreate );
		CStringA log=CUnicodeUtils::GetUTF8( str);
		file.Write(log,log.GetLength());
		//file.WriteString(m_sLogMessage);
		file.Close();
	
		CString out,cmd;
		
		if(  m_RebaseStage == REBASE_SQUASH_EDIT )
			cmd.Format(_T("git.exe commit -F \"%s\""), tempfile);
		else
			cmd.Format(_T("git.exe commit --amend -F \"%s\""), tempfile);

		if(g_Git.Run(cmd,&out,CP_UTF8))
		{
			if(!g_Git.CheckCleanWorkTree())
			{
				CMessageBox::Show(NULL,out,_T("TortoiseGit"),MB_OK|MB_ICONERROR);
				return;
			}
		}

		CFile::Remove(tempfile);
		AddLogString(out);
		this->m_ctrlTabCtrl.SetActiveTab(REBASE_TAB_LOG);
		m_RebaseStage=REBASE_CONTINUE;
		curRev->m_Action|=CTGitPath::LOGACTIONS_REBASE_DONE;
		this->UpdateCurrentStatus();
	}


	InterlockedExchange(&m_bThreadRunning, TRUE);
	SetControlEnable();
	
	if (AfxBeginThread(RebaseThreadEntry, this)==NULL)
	{
		InterlockedExchange(&m_bThreadRunning, FALSE);
		CMessageBox::Show(NULL, _T("Create Rebase Thread Fail"), _T("TortoiseGit"), MB_OK | MB_ICONERROR);
		SetControlEnable();
	}
}
int CRebaseDlg::CheckNextCommitIsSquash()
{
	int index;
	if(m_CommitList.m_IsOldFirst)
		index=m_CurrentRebaseIndex+1;
	else
		index=m_CurrentRebaseIndex-1;

	GitRev *curRev;
	do
	{
		if(index<0)
			return -1;
		if(index>= m_CommitList.GetItemCount())
			return -1;

		curRev=(GitRev*)m_CommitList.m_arShownList[index];
		
		if( curRev->m_Action&CTGitPath::LOGACTIONS_REBASE_SQUASH )
			return 0;
		if( curRev->m_Action&CTGitPath::LOGACTIONS_REBASE_SKIP)
		{
			if(m_CommitList.m_IsOldFirst)
				index++;
			else
				index--;
		}else
			return -1;		

	}while(curRev->m_Action&CTGitPath::LOGACTIONS_REBASE_SKIP);
	
	return -1;

}
int CRebaseDlg::GoNext()
{
	if(m_CommitList.m_IsOldFirst)
		m_CurrentRebaseIndex++;
	else
		m_CurrentRebaseIndex--;	
	return 0;

}
int CRebaseDlg::StateAction()
{
	switch(this->m_RebaseStage)
	{
	case CHOOSE_BRANCH:
	case CHOOSE_COMMIT_PICK_MODE:
		if(StartRebase())
			return -1;
		m_RebaseStage = REBASE_START;
		GoNext();
		break;
	}

	return 0;	
}
void CRebaseDlg::SetContinueButtonText()
{
	CString Text;
	switch(this->m_RebaseStage)
	{
	case CHOOSE_BRANCH:
	case CHOOSE_COMMIT_PICK_MODE:
		if(this->m_IsFastForward)
			Text = _T("Start(FastFwd)");
		else
			Text = _T("Start");
		break;

	case REBASE_START:
	case REBASE_CONTINUE:
	case REBASE_SQUASH_CONFLICT:
		Text = _T("Continue");
		break;

	case REBASE_CONFLICT:
		Text = _T("Commit");
		break;
	case REBASE_EDIT:
		Text = _T("Amend");
		break;

	case REBASE_SQUASH_EDIT:
		Text = _T("Commit");
		break;

	case REBASE_ABORT:
	case REBASE_FINISH:
		Text = _T("Finish");
		break;

	case REBASE_DONE:
		Text = _T("Done");
		break;
	}
	this->GetDlgItem(IDC_REBASE_CONTINUE)->SetWindowText(Text);
}

void CRebaseDlg::SetControlEnable()
{
	switch(this->m_RebaseStage)
	{
	case CHOOSE_BRANCH:
	case CHOOSE_COMMIT_PICK_MODE:
		
		this->GetDlgItem(IDC_PICK_ALL)->EnableWindow(TRUE);
		this->GetDlgItem(IDC_EDIT_ALL)->EnableWindow(TRUE);
		this->GetDlgItem(IDC_SQUASH_ALL)->EnableWindow(TRUE);
		
		if(!m_IsCherryPick)
		{
			this->GetDlgItem(IDC_REBASE_COMBOXEX_BRANCH)->EnableWindow(TRUE);
			this->GetDlgItem(IDC_REBASE_COMBOXEX_UPSTREAM)->EnableWindow(TRUE);
			this->GetDlgItem(IDC_REBASE_CHECK_FORCE)->EnableWindow(TRUE);			
		}
		//this->m_CommitList.m_IsEnableRebaseMenu=TRUE;
		this->m_CommitList.m_ContextMenuMask |= m_CommitList.GetContextMenuBit(CGitLogListBase::ID_REBASE_PICK)|
												m_CommitList.GetContextMenuBit(CGitLogListBase::ID_REBASE_SQUASH)|
												m_CommitList.GetContextMenuBit(CGitLogListBase::ID_REBASE_EDIT)|
												m_CommitList.GetContextMenuBit(CGitLogListBase::ID_REBASE_SKIP);
		break;

	case REBASE_START:
	case REBASE_CONTINUE:
	case REBASE_ABORT:
	case REBASE_FINISH:
	case REBASE_CONFLICT:
	case REBASE_EDIT:
	case REBASE_SQUASH_CONFLICT:
	case REBASE_DONE:
		this->GetDlgItem(IDC_PICK_ALL)->EnableWindow(FALSE);
		this->GetDlgItem(IDC_EDIT_ALL)->EnableWindow(FALSE);
		this->GetDlgItem(IDC_SQUASH_ALL)->EnableWindow(FALSE);
		this->GetDlgItem(IDC_REBASE_COMBOXEX_BRANCH)->EnableWindow(FALSE);
		this->GetDlgItem(IDC_REBASE_COMBOXEX_UPSTREAM)->EnableWindow(FALSE);
		this->GetDlgItem(IDC_REBASE_CHECK_FORCE)->EnableWindow(FALSE);
		//this->m_CommitList.m_IsEnableRebaseMenu=FALSE;
		this->m_CommitList.m_ContextMenuMask &= ~(m_CommitList.GetContextMenuBit(CGitLogListBase::ID_REBASE_PICK)|
												m_CommitList.GetContextMenuBit(CGitLogListBase::ID_REBASE_SQUASH)|
												m_CommitList.GetContextMenuBit(CGitLogListBase::ID_REBASE_EDIT)|
												m_CommitList.GetContextMenuBit(CGitLogListBase::ID_REBASE_SKIP));

		if( m_RebaseStage == REBASE_DONE && (this->m_PostButtonTexts.GetCount() != 0) )
		{
			this->GetDlgItem(IDC_STATUS_STATIC)->ShowWindow(SW_HIDE);
			this->GetDlgItem(IDC_REBASE_POST_BUTTON)->ShowWindow(SW_SHOWNORMAL);
			this->m_PostButton.RemoveAll();
			this->m_PostButton.AddEntries(m_PostButtonTexts);
			//this->GetDlgItem(IDC_REBASE_POST_BUTTON)->SetWindowText(this->m_PostButtonText);
		}
		break;
	}

	if(m_bThreadRunning)
	{
		this->GetDlgItem(IDC_REBASE_CONTINUE)->EnableWindow(FALSE);
		this->GetDlgItem(IDC_REBASE_ABORT)->EnableWindow(FALSE);

	}else
	{
		this->GetDlgItem(IDC_REBASE_CONTINUE)->EnableWindow(TRUE);
		this->GetDlgItem(IDC_REBASE_ABORT)->EnableWindow(TRUE);
	}
}

void CRebaseDlg::UpdateProgress()
{
	int index;
	CRect rect;

	if(m_CommitList.m_IsOldFirst)
		index = m_CurrentRebaseIndex+1;
	else
		index = m_CommitList.GetItemCount()-m_CurrentRebaseIndex;

	m_ProgressBar.SetRange(1,m_CommitList.GetItemCount());
	m_ProgressBar.SetPos(index);

	if(m_CurrentRebaseIndex>=0 && m_CurrentRebaseIndex< m_CommitList.GetItemCount())
	{
		CString text;
		text.Format(_T("Rebasing...(%d/%d)"),index,m_CommitList.GetItemCount());
		m_CtrlStatusText.SetWindowText(text);

	}

	GitRev *prevRev=NULL, *curRev=NULL;

	if( m_CurrentRebaseIndex >= 0 && m_CurrentRebaseIndex< m_CommitList.m_arShownList.GetSize())
	{
		curRev=(GitRev*)m_CommitList.m_arShownList[m_CurrentRebaseIndex];
	}
	
	for(int i=0;i<m_CommitList.m_arShownList.GetSize();i++)
	{
		prevRev=(GitRev*)m_CommitList.m_arShownList[i];
		if(prevRev->m_Action & CTGitPath::LOGACTIONS_REBASE_CURRENT)
		{	
			prevRev->m_Action &= ~ CTGitPath::LOGACTIONS_REBASE_CURRENT;
			m_CommitList.GetItemRect(i,&rect,LVIR_BOUNDS);
			m_CommitList.InvalidateRect(rect);
		}
	}

	if(curRev)
	{
		curRev->m_Action |= CTGitPath::LOGACTIONS_REBASE_CURRENT;
		m_CommitList.GetItemRect(m_CurrentRebaseIndex,&rect,LVIR_BOUNDS);
		m_CommitList.InvalidateRect(rect);
	}
	m_CommitList.EnsureVisible(m_CurrentRebaseIndex,FALSE);

}

void CRebaseDlg::UpdateCurrentStatus()
{
	if( m_CurrentRebaseIndex < 0 && m_RebaseStage!= REBASE_DONE)
	{
		if(m_CommitList.m_IsOldFirst)
			m_RebaseStage = CRebaseDlg::REBASE_START;
		else
			m_RebaseStage = CRebaseDlg::REBASE_FINISH;
	}

	if( m_CurrentRebaseIndex == m_CommitList.m_arShownList.GetSize() && m_RebaseStage!= REBASE_DONE)
	{
		if(m_CommitList.m_IsOldFirst)
			m_RebaseStage = CRebaseDlg::REBASE_DONE;
		else
			m_RebaseStage = CRebaseDlg::REBASE_FINISH;
	}

	SetContinueButtonText();
	SetControlEnable();
	UpdateProgress();
}

void CRebaseDlg::AddLogString(CString str)
{
	this->m_wndOutputRebase.SendMessage(SCI_SETREADONLY, FALSE);
	CStringA sTextA = m_wndOutputRebase.StringForControl(str);//CUnicodeUtils::GetUTF8(str);
	this->m_wndOutputRebase.SendMessage(SCI_REPLACESEL, 0, (LPARAM)(LPCSTR)sTextA);
	this->m_wndOutputRebase.SendMessage(SCI_REPLACESEL, 0, (LPARAM)(LPCSTR)"\n");
	this->m_wndOutputRebase.SendMessage(SCI_SETREADONLY, TRUE);
}

int CRebaseDlg::GetCurrentCommitID()
{
	if(m_CommitList.m_IsOldFirst)
	{
		return this->m_CurrentRebaseIndex+1;

	}else
	{
		return m_CommitList.GetItemCount()-m_CurrentRebaseIndex;
	}
}

int CRebaseDlg::DoRebase()
{	
	CString cmd,out;
	if(m_CurrentRebaseIndex <0)
		return 0;
	if(m_CurrentRebaseIndex >= m_CommitList.GetItemCount() )
		return 0;

	GitRev *pRev = (GitRev*)m_CommitList.m_arShownList[m_CurrentRebaseIndex];
	int mode=pRev->m_Action & CTGitPath::LOGACTIONS_REBASE_MODE_MASK;
	CString nocommit;

	if( mode== CTGitPath::LOGACTIONS_REBASE_SKIP)
	{
		pRev->m_Action|= CTGitPath::LOGACTIONS_REBASE_DONE;
		return 0;
	}
	
	if( mode != CTGitPath::LOGACTIONS_REBASE_PICK )
	{
		this->m_SquashMessage+= pRev->m_Subject;
		this->m_SquashMessage+= _T("\n");
		this->m_SquashMessage+= pRev->m_Body;
	}
	else
		this->m_SquashMessage.Empty();

	if(mode == CTGitPath::LOGACTIONS_REBASE_SQUASH)
		nocommit=_T(" --no-commit ");

	CString log;
	log.Format(_T("%s %d:%s"),CTGitPath::GetActionName(mode),this->GetCurrentCommitID(),pRev->m_CommitHash.ToString());
	AddLogString(log);
	AddLogString(pRev->m_Subject);
	cmd.Format(_T("git.exe cherry-pick %s %s"),nocommit,pRev->m_CommitHash.ToString());

	if(g_Git.Run(cmd,&out,CP_UTF8))
	{
		AddLogString(out);
		CTGitPathList list;
		if(g_Git.ListConflictFile(list))
		{
			AddLogString(_T("Get conflict files fail"));
			return -1;
		}
		if(list.GetCount() == 0 )
		{
			if(mode ==  CTGitPath::LOGACTIONS_REBASE_PICK)
			{
				pRev->m_Action|= CTGitPath::LOGACTIONS_REBASE_DONE;
				return 0;
			}
			if(mode == CTGitPath::LOGACTIONS_REBASE_EDIT)
			{
				this->m_RebaseStage = REBASE_EDIT ;
				return -1; // Edit return -1 to stop rebase. 
			}
			// Squash Case
			if(CheckNextCommitIsSquash())
			{   // no squash
				// let user edit last commmit message
				this->m_RebaseStage = REBASE_SQUASH_EDIT;
				return -1;
			}
		}
		if(mode == CTGitPath::LOGACTIONS_REBASE_SQUASH)
			m_RebaseStage = REBASE_SQUASH_CONFLICT;
		else
			m_RebaseStage = REBASE_CONFLICT;
		return -1;	

	}else
	{
		AddLogString(out);
		if(mode ==  CTGitPath::LOGACTIONS_REBASE_PICK)
		{
			pRev->m_Action|= CTGitPath::LOGACTIONS_REBASE_DONE;
			return 0;
		}
		if(mode == CTGitPath::LOGACTIONS_REBASE_EDIT)
		{
			this->m_RebaseStage = REBASE_EDIT ;
			return -1; // Edit return -1 to stop rebase. 
		}

		// Squash Case
		if(CheckNextCommitIsSquash())
		{   // no squash
			// let user edit last commmit message
			this->m_RebaseStage = REBASE_SQUASH_EDIT;
			return -1;
		}
	}
	
	return 0;
}

BOOL CRebaseDlg::IsEnd()
{
	if(m_CommitList.m_IsOldFirst)
		return m_CurrentRebaseIndex>= this->m_CommitList.GetItemCount();
	else
		return m_CurrentRebaseIndex<0;
}

int CRebaseDlg::RebaseThread()
{
	int ret=0;
	while(1)
	{
		if( m_RebaseStage == REBASE_START )
		{
			if( this->StartRebase() )
			{
				InterlockedExchange(&m_bThreadRunning, FALSE);
				ret = -1;
				break;
			}
			m_RebaseStage = REBASE_CONTINUE;

		}else if( m_RebaseStage == REBASE_CONTINUE )
		{
			this->GoNext();	
			if(IsEnd())
			{
				ret = 0;
				m_RebaseStage = REBASE_FINISH;
				
			}else
			{
				ret = DoRebase();

				if( ret )
				{	
					break;
				}
			}

		}else if( m_RebaseStage == REBASE_FINISH )
		{			
			FinishRebase();
			m_RebaseStage = REBASE_DONE;
			break;
			
		}else
		{
			break;
		}
		this->PostMessage(MSG_REBASE_UPDATE_UI);
		//this->UpdateCurrentStatus();
	}

	InterlockedExchange(&m_bThreadRunning, FALSE);
	this->PostMessage(MSG_REBASE_UPDATE_UI);
	return ret;
}

void CRebaseDlg::ListConflictFile()
{
	this->m_FileListCtrl.Clear();	
	CTGitPathList list;
	CTGitPath path;
	list.AddPath(path);

	m_FileListCtrl.m_bIsRevertTheirMy = !m_IsCherryPick;

	this->m_FileListCtrl.GetStatus(&list,true);
	this->m_FileListCtrl.Show(CTGitPath::LOGACTIONS_UNMERGED|CTGitPath::LOGACTIONS_MODIFIED|CTGitPath::LOGACTIONS_ADDED|CTGitPath::LOGACTIONS_DELETED,
							   CTGitPath::LOGACTIONS_UNMERGED);
	if( this->m_FileListCtrl.GetItemCount() == 0 )
	{
		
	}
}

LRESULT CRebaseDlg::OnRebaseUpdateUI(WPARAM,LPARAM)
{
	UpdateCurrentStatus();
	if(m_CurrentRebaseIndex <0)
		return 0;
	if(m_CurrentRebaseIndex >= m_CommitList.GetItemCount() )
		return 0;
	GitRev *curRev=(GitRev*)m_CommitList.m_arShownList[m_CurrentRebaseIndex];
	
	switch(m_RebaseStage)
	{
	case REBASE_CONFLICT:
	case REBASE_SQUASH_CONFLICT:
		ListConflictFile();			
		this->m_ctrlTabCtrl.SetActiveTab(REBASE_TAB_CONFLICT);
		this->m_LogMessageCtrl.SetText(curRev->m_Subject+_T("\n")+curRev->m_Body);
		break;
	case REBASE_EDIT:
		this->m_ctrlTabCtrl.SetActiveTab(REBASE_TAB_MESSAGE);
		this->m_LogMessageCtrl.SetText(curRev->m_Subject+_T("\n")+curRev->m_Body);
		break;
	case REBASE_SQUASH_EDIT:
		this->m_ctrlTabCtrl.SetActiveTab(REBASE_TAB_MESSAGE);
		this->m_LogMessageCtrl.SetText(this->m_SquashMessage);
		break;
	default:
		this->m_ctrlTabCtrl.SetActiveTab(REBASE_TAB_LOG);
	}	
	return 0;
}
void CRebaseDlg::OnCancel()
{
	OnBnClickedAbort();
}
void CRebaseDlg::OnBnClickedAbort()
{
	CString cmd,out;
	if(m_OrigUpstreamHash.IsEmpty())
	{
		__super::OnCancel();
	}
	
	if(m_RebaseStage == CHOOSE_BRANCH || m_RebaseStage== CHOOSE_COMMIT_PICK_MODE)
	{
		return;
	}

	if(CMessageBox::Show(NULL,_T("Are you sure you want to abort the rebase process?"),_T("TortoiseGit"),MB_YESNO) != IDYES)
		return;

	if(this->m_IsFastForward)
	{
		cmd.Format(_T("git.exe reset --hard  %s"),this->m_OrigBranchHash.Left(40));
		if(g_Git.Run(cmd,&out,CP_UTF8))
		{
			AddLogString(out);
			return ;
		}
		__super::OnCancel();
		return;
	}
	cmd.Format(_T("git.exe checkout -f %s"),this->m_UpstreamCtrl.GetString());
	if(g_Git.Run(cmd,&out,CP_UTF8))
	{
		AddLogString(out);
		return ;
	}

	cmd.Format(_T("git.exe reset --hard  %s"),this->m_OrigUpstreamHash.Left(40));
	if(g_Git.Run(cmd,&out,CP_UTF8))
	{
		AddLogString(out);
		return ;
	}
	
	if(this->m_IsCherryPick) //there are not "branch" at cherry pick mode
	{
		__super::OnCancel();
		return;
	}

	cmd.Format(_T("git checkout -f %s"),this->m_BranchCtrl.GetString());
	if(g_Git.Run(cmd,&out,CP_UTF8))
	{
		AddLogString(out);
		return ;
	}
	
	cmd.Format(_T("git.exe reset --hard  %s"),this->m_OrigBranchHash.Left(40));
	if(g_Git.Run(cmd,&out,CP_UTF8))
	{
		AddLogString(out);
		return ;
	}
	__super::OnCancel();
}

void CRebaseDlg::OnBnClickedButtonBrowse()
{
	if(CBrowseRefsDlg::PickRefForCombo(&m_UpstreamCtrl, gPickRef_NoTag))
		OnCbnSelchangeUpstream();
}

void CRebaseDlg::OnBnClickedRebaseCheckForce()
{
	// TODO: Add your control notification handler code here
	this->UpdateData();
	this->FetchLogList();
	if(this->CheckRebaseCondition())
	{
		/* Disable Start Rebase */
		this->GetDlgItem(IDC_REBASE_CONTINUE)->EnableWindow(FALSE);
	}
}

void CRebaseDlg::OnStnClickedStatusStatic()
{
	// TODO: Add your control notification handler code here
}

void CRebaseDlg::OnBnClickedRebasePostButton()
{
	// TODO: Add your control notification handler code here
	this->m_Upstream=this->m_UpstreamCtrl.GetString();
	this->m_Branch=this->m_BranchCtrl.GetString();

	this->EndDialog(IDC_REBASE_POST_BUTTON+this->m_PostButton.GetCurrentEntry());
}

void CRebaseDlg::Refresh()
{
	if(this->m_IsCherryPick)
		return ;

	if(this->m_RebaseStage == CHOOSE_BRANCH )
	{
		this->UpdateData();
		this->FetchLogList();
		if(this->CheckRebaseCondition())
		{
			/* Disable Start Rebase */
			this->GetDlgItem(IDC_REBASE_CONTINUE)->EnableWindow(FALSE);
		}
	}
}