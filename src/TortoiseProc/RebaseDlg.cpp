// RebaseDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "RebaseDlg.h"
#include "AppUtils.h"

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

}


BEGIN_MESSAGE_MAP(CRebaseDlg, CResizableStandAloneDialog)
    ON_BN_CLICKED(IDC_PICK_ALL, &CRebaseDlg::OnBnClickedPickAll)
    ON_BN_CLICKED(IDC_SQUASH_ALL, &CRebaseDlg::OnBnClickedSquashAll)
    ON_BN_CLICKED(IDC_EDIT_ALL, &CRebaseDlg::OnBnClickedEditAll)
    ON_BN_CLICKED(IDC_REBASE_SPLIT, &CRebaseDlg::OnBnClickedRebaseSplit)
	ON_BN_CLICKED(IDC_REBASE_CONTINUE,OnBnClickedContinue)
	ON_WM_SIZE()
	ON_CBN_SELCHANGE(IDC_REBASE_COMBOXEX_BRANCH,   &CRebaseDlg::OnCbnSelchangeBranch)
	ON_CBN_SELCHANGE(IDC_REBASE_COMBOXEX_UPSTREAM, &CRebaseDlg::OnCbnSelchangeUpstream)
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

	rectDummy.top-=20;
	rectDummy.bottom-=20;

	rectDummy.left-=5;
	rectDummy.right-=5;
	
	if (!m_ctrlTabCtrl.Create(CMFCTabCtrl::STYLE_FLAT, rectDummy, this, IDC_REBASE_TAB))
	{
		TRACE0("Failed to create output tab window\n");
		return FALSE;      // fail to create
	}
	m_ctrlTabCtrl.SetResizeMode(CMFCTabCtrl::RESIZE_NO);
	// Create output panes:
	//const DWORD dwStyle = LBS_NOINTEGRALHEIGHT | WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL;
	DWORD dwStyle =LVS_REPORT | LVS_SHOWSELALWAYS | LVS_ALIGNLEFT | LVS_OWNERDATA | WS_BORDER | WS_TABSTOP |LVS_SINGLESEL |WS_CHILD | WS_VISIBLE;

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

	m_FileListCtrl.Init(SVNSLC_COLEXT | SVNSLC_COLSTATUS , _T("RebaseDlg"));

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

	m_CommitList.m_IsIDReplaceAction = TRUE;
//	m_CommitList.m_IsOldFirst = TRUE;
	m_CommitList.m_IsRebaseReplaceGraph = TRUE;

	m_CommitList.DeleteAllItems();
	m_CommitList.InsertGitColumn();

	FetchLogList();
	SetContinueButtonText();
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
		m_CommitList.m_logEntries[i].m_Action=action;
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
	m_CommitList.Clear();
	this->m_CommitList.FillGitLog(NULL,0,&m_UpstreamCtrl.GetString(),&m_BranchCtrl.GetString());
	if( m_CommitList.GetItemCount() == 0 )
		m_CommitList.ShowText(_T("Nothing Rebase"));

	CString hash=g_Git.GetHash(m_UpstreamCtrl.GetString());
	
	if(m_CommitList.m_logEntries[m_CommitList.m_logEntries.size()-1].m_ParentHash.size() >=0 )
	{
		if(hash ==  m_CommitList.m_logEntries[m_CommitList.m_logEntries.size()-1].m_ParentHash[0])
		{
			m_CommitList.Clear();
			m_CommitList.ShowText(_T("Nothing Rebase"));
		}
	}
	
	m_tooltips.Pop();
	AddBranchToolTips(&this->m_BranchCtrl);
	AddBranchToolTips(&this->m_UpstreamCtrl);
	
	for(int i=0;i<m_CommitList.m_logEntries.size();i++)
	{
		m_CommitList.m_logEntries[i].m_Action = CTGitPath::LOGACTIONS_REBASE_PICK;
	}
	
	m_CommitList.Invalidate();

	if(m_CommitList.m_IsOldFirst)
		this->m_CurrentRebaseIndex = -1;
	else
		this->m_CurrentRebaseIndex = m_CommitList.m_logEntries.size();
	
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
			rev.m_CommitHash,
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
	m_tooltips.RelayEvent(pMsg);
	return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}

void CRebaseDlg::OnBnClickedContinue()
{
	GitRev *prevRev,*curRev;
	prevRev=curRev=NULL;
	CRect rect;
	int prevIndex=m_CurrentRebaseIndex;
	
	UpdateCurrentStatus();

	if( m_CurrentRebaseIndex >= 0 && m_CurrentRebaseIndex< m_CommitList.m_arShownList.GetSize())
	{
		prevRev=(GitRev*)m_CommitList.m_arShownList[m_CurrentRebaseIndex];
	}
	
	if(m_CommitList.m_IsOldFirst)
		m_CurrentRebaseIndex++;
	else
		m_CurrentRebaseIndex--;

	if(m_CurrentRebaseIndex >= 0 && m_CurrentRebaseIndex<m_CommitList.m_arShownList.GetSize())
	{
		curRev=(GitRev*)m_CommitList.m_arShownList[m_CurrentRebaseIndex];

	}

	if(prevRev)
	{
		prevRev->m_Action &= ~ CTGitPath::LOGACTIONS_REBASE_CURRENT;
		prevRev->m_Action |= CTGitPath::LOGACTIONS_REBASE_DONE;
		m_CommitList.GetItemRect(prevIndex,&rect,LVIR_BOUNDS);
		m_CommitList.InvalidateRect(rect);

	}

	if(curRev)
	{
		curRev->m_Action |= CTGitPath::LOGACTIONS_REBASE_CURRENT;
		m_CommitList.GetItemRect(m_CurrentRebaseIndex,&rect,LVIR_BOUNDS);
		m_CommitList.InvalidateRect(rect);
	}


	UpdateCurrentStatus();

	m_CommitList.EnsureVisible(m_CurrentRebaseIndex,FALSE);
	
	this->SetContinueButtonText();
	this->SetControlEnable();
	UpdateProgress();
}

void CRebaseDlg::SetContinueButtonText()
{
	CString Text;
	switch(this->m_RebaseStage)
	{
	case CHOOSE_BRANCH:
	case CHOOSE_COMMIT_PICK_MODE:
		Text = _T("Start");
		break;

	case REBASE_START:
	case REBASE_CONTINUE:
		Text = _T("Continue");
		break;
	case REBASE_ABORT:
	case REBASE_FINISH:
		Text = _T("Finish");
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
		this->GetDlgItem(IDC_REBASE_COMBOXEX_BRANCH)->EnableWindow(TRUE);
		this->GetDlgItem(IDC_REBASE_COMBOXEX_UPSTREAM)->EnableWindow(TRUE);
		this->m_CommitList.m_IsEnableRebaseMenu=TRUE;
		break;

	case REBASE_START:
	case REBASE_CONTINUE:
	case REBASE_ABORT:
	case REBASE_FINISH:
		this->GetDlgItem(IDC_PICK_ALL)->EnableWindow(FALSE);
		this->GetDlgItem(IDC_EDIT_ALL)->EnableWindow(FALSE);
		this->GetDlgItem(IDC_SQUASH_ALL)->EnableWindow(FALSE);
		this->GetDlgItem(IDC_REBASE_COMBOXEX_BRANCH)->EnableWindow(FALSE);
		this->GetDlgItem(IDC_REBASE_COMBOXEX_UPSTREAM)->EnableWindow(FALSE);
		this->m_CommitList.m_IsEnableRebaseMenu=FALSE;
		break;
	}
}

void CRebaseDlg::UpdateProgress()
{
	int index;

	if(m_CommitList.m_IsOldFirst)
		index = m_CurrentRebaseIndex+1;
	else
		index = m_CommitList.GetItemCount()-m_CurrentRebaseIndex;

	m_ProgressBar.SetRange(1,m_CommitList.GetItemCount());
	m_ProgressBar.SetPos(index);

	if(m_CurrentRebaseIndex>0 && m_CurrentRebaseIndex< m_CommitList.GetItemCount())
	{
		CString text;
		text.Format(_T("Rebasing...(%d/%d)"),index,m_CommitList.GetItemCount());
		m_CtrlStatusText.SetWindowText(text);

	}
}

void CRebaseDlg::UpdateCurrentStatus()
{
	if( m_CurrentRebaseIndex < 0)
	{
		if(m_CommitList.m_IsOldFirst)
			m_RebaseStage = CRebaseDlg::REBASE_START;
		else
			m_RebaseStage = CRebaseDlg::REBASE_FINISH;
	}

	if( m_CurrentRebaseIndex == m_CommitList.m_arShownList.GetSize())
	{
		if(m_CommitList.m_IsOldFirst)
			m_RebaseStage = CRebaseDlg::REBASE_FINISH;
		else
			m_RebaseStage = CRebaseDlg::REBASE_START;
	}

	SetContinueButtonText();
	SetControlEnable();
	UpdateProgress();
}