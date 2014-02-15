// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2014 - TortoiseGit
// Copyright (C) 2011-2013 - Sven Strickroth <email@cs-ware.de>

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

// RebaseDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "RebaseDlg.h"
#include "AppUtils.h"
#include "LoglistUtils.h"
#include "MessageBox.h"
#include "UnicodeUtils.h"
#include "BrowseRefsDlg.h"
#include "ProgressDlg.h"
#include "SmartHandle.h"
#include "../TGitCache/CacheInterface.h"
#include "Settings\Settings.h"
#include "MassiveGitTask.h"

// CRebaseDlg dialog

IMPLEMENT_DYNAMIC(CRebaseDlg, CResizableStandAloneDialog)

CRebaseDlg::CRebaseDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CRebaseDlg::IDD, pParent)
	, m_bPickAll(FALSE)
	, m_bSquashAll(FALSE)
	, m_bEditAll(FALSE)
	, m_bAddCherryPickedFrom(FALSE)
	, m_bStatusWarning(false)
	, m_bAutoSkipFailedCommit(FALSE)
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
	DDX_Check(pDX, IDC_CHECK_CHERRYPICKED_FROM, m_bAddCherryPickedFrom);
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
	ON_REGISTERED_MESSAGE(CGitStatusListCtrl::GITSLNM_NEEDSREFRESH, OnGitStatusListCtrlNeedsRefresh)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE, &CRebaseDlg::OnBnClickedButtonBrowse)
	ON_BN_CLICKED(IDC_REBASE_CHECK_FORCE, &CRebaseDlg::OnBnClickedRebaseCheckForce)
	ON_BN_CLICKED(IDC_CHECK_CHERRYPICKED_FROM, &CRebaseDlg::OnBnClickedCheckCherryPickedFrom)
	ON_BN_CLICKED(IDC_REBASE_POST_BUTTON, &CRebaseDlg::OnBnClickedRebasePostButton)
	ON_BN_CLICKED(IDC_BUTTON_UP2, &CRebaseDlg::OnBnClickedButtonUp2)
	ON_BN_CLICKED(IDC_BUTTON_DOWN2, &CRebaseDlg::OnBnClickedButtonDown2)
	ON_REGISTERED_MESSAGE(WM_TASKBARBTNCREATED, OnTaskbarBtnCreated)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_COMMIT_LIST, OnLvnItemchangedLoglist)
	ON_REGISTERED_MESSAGE(CGitLogListBase::m_RebaseActionMessage, OnRebaseActionMessage)
	ON_WM_CTLCOLOR()
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
	AddAnchor(IDC_BUTTON_UP2,TOP_LEFT);
	AddAnchor(IDC_BUTTON_DOWN2,TOP_LEFT);
	AddAnchor(IDC_REBASE_COMBOXEX_UPSTREAM,TOP_LEFT);
	AddAnchor(IDC_REBASE_COMBOXEX_BRANCH,TOP_LEFT);
	AddAnchor(IDC_REBASE_STATIC_UPSTREAM,TOP_LEFT);
	AddAnchor(IDC_REBASE_STATIC_BRANCH,TOP_LEFT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	AddAnchor(IDC_REBASE_CHECK_FORCE,TOP_RIGHT);
	AddAnchor(IDC_CHECK_CHERRYPICKED_FROM, TOP_RIGHT);
	AddAnchor(IDC_REBASE_POST_BUTTON,BOTTOM_LEFT);

	this->AddOthersToAnchor();
}

BOOL CRebaseDlg::OnInitDialog()
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
	DWORD dwStyle =LVS_REPORT | LVS_SHOWSELALWAYS | LVS_ALIGNLEFT | WS_BORDER | WS_TABSTOP | WS_CHILD | WS_VISIBLE;

	if (! this->m_FileListCtrl.Create(dwStyle,rectDummy,&this->m_ctrlTabCtrl,0) )
	{
		TRACE0("Failed to create output windows\n");
		return FALSE;      // fail to create
	}
	m_FileListCtrl.m_hwndLogicalParent = this;

	if( ! this->m_LogMessageCtrl.Create(_T("Scintilla"),_T("source"),0,rectDummy,&m_ctrlTabCtrl,0,0) )
	{
		TRACE0("Failed to create log message control");
		return FALSE;
	}
	m_LogMessageCtrl.Init(0);
	m_LogMessageCtrl.Call(SCI_SETREADONLY, TRUE);

	dwStyle = LBS_NOINTEGRALHEIGHT | WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL;

	if (!m_wndOutputRebase.Create(_T("Scintilla"),_T("source"),0,rectDummy, &m_ctrlTabCtrl, 0,0) )
	{
		TRACE0("Failed to create output windows\n");
		return -1;      // fail to create
	}
	m_wndOutputRebase.Init(0, FALSE);
	m_wndOutputRebase.Call(SCI_SETREADONLY, TRUE);

	m_tooltips.Create(this);

	m_tooltips.AddTool(IDC_REBASE_CHECK_FORCE,IDS_REBASE_FORCE_TT);
	m_tooltips.AddTool(IDC_REBASE_ABORT,IDS_REBASE_ABORT_TT);



	m_FileListCtrl.Init(GITSLC_COLEXT | GITSLC_COLSTATUS |GITSLC_COLADD|GITSLC_COLDEL , _T("RebaseDlg"),(GITSLC_POPALL ^ (GITSLC_POPCOMMIT|GITSLC_POPRESTORE)), false, true, GITSLC_COLEXT | GITSLC_COLSTATUS | GITSLC_COLADD| GITSLC_COLDEL);

	m_ctrlTabCtrl.AddTab(&m_FileListCtrl, CString(MAKEINTRESOURCE(IDS_PROC_REVISIONFILES)));
	m_ctrlTabCtrl.AddTab(&m_LogMessageCtrl, CString(MAKEINTRESOURCE(IDS_PROC_COMMITMESSAGE)), 1);
	AddRebaseAnchor();

	AdjustControlSize(IDC_PICK_ALL);
	AdjustControlSize(IDC_SQUASH_ALL);
	AdjustControlSize(IDC_EDIT_ALL);
	AdjustControlSize(IDC_CHECK_CHERRYPICKED_FROM);

	CString sWindowTitle;
	GetWindowText(sWindowTitle);
	CAppUtils::SetWindowTitle(m_hWnd, g_Git.m_CurrentDir, sWindowTitle);

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

	}
	else
	{
		this->m_BranchCtrl.EnableWindow(FALSE);
		this->m_UpstreamCtrl.EnableWindow(FALSE);
	}

	m_CommitList.m_ColumnRegKey = _T("Rebase");
	m_CommitList.m_IsIDReplaceAction = TRUE;
//	m_CommitList.m_IsOldFirst = TRUE;
	m_CommitList.m_IsRebaseReplaceGraph = TRUE;
	m_CommitList.m_bNoHightlightHead = TRUE;

	m_CommitList.InsertGitColumn();

	this->SetControlEnable();

	if(m_IsCherryPick)
	{
		this->m_BranchCtrl.SetCurSel(-1);
		this->m_BranchCtrl.EnableWindow(FALSE);
		GetDlgItem(IDC_REBASE_CHECK_FORCE)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_BUTTON_BROWSE)->EnableWindow(FALSE);
		this->m_UpstreamCtrl.AddString(_T("HEAD"));
		this->m_UpstreamCtrl.EnableWindow(FALSE);
		CAppUtils::SetWindowTitle(m_hWnd, g_Git.m_CurrentDir, CString(MAKEINTRESOURCE(IDS_PROGS_TITLE_CHERRYPICK)));
		this->m_CommitList.StartFilter();
	}
	else
	{
		GetDlgItem(IDC_CHECK_CHERRYPICKED_FROM)->ShowWindow(SW_HIDE);
		SetContinueButtonText();
		m_CommitList.DeleteAllItems();
		FetchLogList();
	}

	m_CommitList.m_ContextMenuMask &= ~(m_CommitList.GetContextMenuBit(CGitLogListBase::ID_CHERRY_PICK)|
										m_CommitList.GetContextMenuBit(CGitLogListBase::ID_SWITCHTOREV)|
										m_CommitList.GetContextMenuBit(CGitLogListBase::ID_RESET)|
										m_CommitList.GetContextMenuBit(CGitLogListBase::ID_REVERTREV)|
										m_CommitList.GetContextMenuBit(CGitLogListBase::ID_MERGEREV) |
										m_CommitList.GetContextMenuBit(CGitLogListBase::ID_REBASE_TO_VERSION)|
										m_CommitList.GetContextMenuBit(CGitLogListBase::ID_REVERTTOREV)|
										m_CommitList.GetContextMenuBit(CGitLogListBase::ID_COMBINE_COMMIT));

	if(m_CommitList.m_IsOldFirst)
		this->m_CurrentRebaseIndex = -1;
	else
		this->m_CurrentRebaseIndex = (int)m_CommitList.m_logEntries.size();

	return TRUE;
}
// CRebaseDlg message handlers

HBRUSH CRebaseDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	if (pWnd->GetDlgCtrlID() == IDC_STATUS_STATIC && nCtlColor == CTLCOLOR_STATIC && m_bStatusWarning)
	{
		pDC->SetBkColor(RGB(255, 0, 0));
		pDC->SetTextColor(RGB(255, 255, 255));
		return CreateSolidBrush(RGB(255, 0, 0));
	}

	return CResizableStandAloneDialog::OnCtlColor(pDC, pWnd, nCtlColor);
}

void CRebaseDlg::OnBnClickedPickAll()
{
	this->UpdateData();
	if(this->m_bPickAll)
		this->SetAllRebaseAction(CTGitPath::LOGACTIONS_REBASE_PICK);

	this->m_bEditAll=FALSE;
	this->m_bSquashAll=FALSE;
	this->UpdateData(FALSE);
}

void CRebaseDlg::OnBnClickedSquashAll()
{
	this->UpdateData();
	if(this->m_bSquashAll)
		this->SetAllRebaseAction(CTGitPath::LOGACTIONS_REBASE_SQUASH);

	this->m_bEditAll=FALSE;
	this->m_bPickAll=FALSE;
	this->UpdateData(FALSE);
}

void CRebaseDlg::OnBnClickedEditAll()
{
	this->UpdateData();
	if( this->m_bEditAll )
		this->SetAllRebaseAction(CTGitPath::LOGACTIONS_REBASE_EDIT);

	this->m_bPickAll=FALSE;
	this->m_bSquashAll=FALSE;
	this->UpdateData(FALSE);
}

void CRebaseDlg::SetAllRebaseAction(int action)
{
	for (size_t i = 0; i < this->m_CommitList.m_logEntries.size(); ++i)
	{
		m_CommitList.m_logEntries.GetGitRevAt(i).GetAction(&m_CommitList)=action;
	}
	m_CommitList.Invalidate();
}

void CRebaseDlg::OnBnClickedRebaseSplit()
{
	this->UpdateData();
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
	CSplitterControl::ChangePos(GetDlgItem(IDC_BUTTON_UP2),0,delta);
	CSplitterControl::ChangePos(GetDlgItem(IDC_BUTTON_DOWN2),0,delta);
	CSplitterControl::ChangePos(GetDlgItem(IDC_REBASE_CHECK_FORCE),0,delta);
	CSplitterControl::ChangePos(GetDlgItem(IDC_CHECK_CHERRYPICKED_FROM), 0, delta);

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
	m_BranchCtrl.SetCurSel(current);
	list.clear();
	g_Git.GetBranchList(list, NULL, CGit::BRANCH_ALL_F);
	m_UpstreamCtrl.AddString(list);

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
		else
			m_UpstreamCtrl.SetCurSel(-1);
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
	CGitHash base,hash,upstream;
	m_IsFastForward=FALSE;

	if (g_Git.GetHash(hash, m_BranchCtrl.GetString()))
	{
		m_CommitList.ShowText(g_Git.GetGitLastErr(_T("Could not get hash of \"") + m_BranchCtrl.GetString() + _T("\".")));
		this->GetDlgItem(IDC_REBASE_CONTINUE)->EnableWindow(false);
		return;
	}
	if (g_Git.GetHash(upstream, m_UpstreamCtrl.GetString()))
	{
		m_CommitList.ShowText(g_Git.GetGitLastErr(_T("Could not get hash of \"") + m_UpstreamCtrl.GetString() + _T("\".")));
		this->GetDlgItem(IDC_REBASE_CONTINUE)->EnableWindow(false);
		return;
	}

	if (hash == upstream)
	{
		m_CommitList.Clear();
		CString text,fmt;
		fmt.LoadString(IDS_REBASE_EQUAL_FMT);
		text.Format(fmt,m_BranchCtrl.GetString(),this->m_UpstreamCtrl.GetString());

		m_CommitList.ShowText(text);
		this->GetDlgItem(IDC_REBASE_CONTINUE)->EnableWindow(false);
		return;
	}

	if (g_Git.IsFastForward(m_BranchCtrl.GetString(), m_UpstreamCtrl.GetString(), &base))
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

	if(!this->m_bForce)
	{
		if (base == upstream)
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
	CString refFrom = g_Git.FixBranchName(m_UpstreamCtrl.GetString());
	CString refTo   = g_Git.FixBranchName(m_BranchCtrl.GetString());
	CString range;
	range.Format(_T("%s..%s"), refFrom, refTo);
	this->m_CommitList.FillGitLog(nullptr, &range, 0);

	if( m_CommitList.GetItemCount() == 0 )
		m_CommitList.ShowText(CString(MAKEINTRESOURCE(IDS_PROC_NOTHINGTOREBASE)));

#if 0
	if(m_CommitList.m_logEntries[m_CommitList.m_logEntries.size()-1].m_ParentHash.size() >=0 )
	{
		if(upstream ==  m_CommitList.m_logEntries[m_CommitList.m_logEntries.size()-1].m_ParentHash[0])
		{
			m_CommitList.Clear();
			m_CommitList.ShowText(_T("Nothing Rebase"));
		}
	}
#endif

	m_tooltips.Pop();
	AddBranchToolTips(&this->m_BranchCtrl);
	AddBranchToolTips(&this->m_UpstreamCtrl);

	//Default all actions to 'pick'
	std::map<CGitHash, size_t> revIxMap;
	for (size_t i = 0; i < m_CommitList.m_logEntries.size(); ++i)
	{
		GitRev& rev = m_CommitList.m_logEntries.GetGitRevAt(i);
		rev.GetAction(&m_CommitList) = CTGitPath::LOGACTIONS_REBASE_PICK;
		revIxMap[rev.m_CommitHash] = i;
	}

	//Default to skip when already in upstream
	CString cherryCmd;
	cherryCmd.Format(L"git.exe cherry \"%s\" \"%s\"", refFrom, refTo);
	g_Git.Run(cherryCmd, [&](const CStringA& line)
	{
		if (line.GetLength() < 2)
			return;
		if (line[0] != '-')
			return; //Dont skip (only skip commits starting with a '-')
		CString hash = CUnicodeUtils::GetUnicode(line.Mid(1));
		hash.Trim();
		auto itIx = revIxMap.find(CGitHash(hash));
		if (itIx == revIxMap.end())
			return; //Not found?? Should not occur...

		//Found. Skip it.
		m_CommitList.m_logEntries.GetGitRevAt(itIx->second).GetAction(&m_CommitList) = CTGitPath::LOGACTIONS_REBASE_SKIP;
	});

	m_CommitList.Invalidate();

	if(m_CommitList.m_IsOldFirst)
		this->m_CurrentRebaseIndex = -1;
	else
		this->m_CurrentRebaseIndex = (int)m_CommitList.m_logEntries.size();

	this->GetDlgItem(IDC_REBASE_CONTINUE)->EnableWindow(m_CommitList.GetItemCount());
	SetContinueButtonText();
}

void CRebaseDlg::AddBranchToolTips(CHistoryCombo *pBranch)
{
	if(pBranch)
	{
		CString text=pBranch->GetString();
		CString tooltip;

		GitRev rev;
		try
		{
			rev.GetCommit(text);
		}
		catch (const char *msg)
		{
			CMessageBox::Show(m_hWnd, _T("Could not get commit ") + text + _T("\nlibgit reports:\n") + CString(msg), _T("TortoiseGit"), MB_ICONERROR);
		}

		tooltip.Format(_T("%s: %s\n%s: %s <%s>\n%s: %s\n%s:\n%s\n%s"),
			CString(MAKEINTRESOURCE(IDS_LOG_REVISION)),
			rev.m_CommitHash.ToString(),
			CString(MAKEINTRESOURCE(IDS_LOG_AUTHOR)),
			rev.GetAuthorName(),
			rev.GetAuthorEmail(),
			CString(MAKEINTRESOURCE(IDS_LOG_DATE)),
			CLoglistUtils::FormatDateAndTime(rev.GetAuthorDate(), DATE_LONGDATE),
			CString(MAKEINTRESOURCE(IDS_LOG_MESSAGE)),
			rev.GetSubject(),
			rev.GetBody());

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
		case ' ':
			if (LogListHasFocus(pMsg->hwnd)
				&& LogListHasMenuItem(CGitLogListBase::ID_REBASE_PICK)
				&& LogListHasMenuItem(CGitLogListBase::ID_REBASE_SQUASH)
				&& LogListHasMenuItem(CGitLogListBase::ID_REBASE_EDIT)
				&& LogListHasMenuItem(CGitLogListBase::ID_REBASE_SKIP))
			{
				m_CommitList.ShiftSelectedAction();
				return TRUE;
			}
			break;
		case 'P':
			if (LogListHasFocus(pMsg->hwnd) && LogListHasMenuItem(CGitLogListBase::ID_REBASE_PICK))
			{
				m_CommitList.SetSelectedAction(CTGitPath::LOGACTIONS_REBASE_PICK);
				return TRUE;
			}
			break;
		case 'S':
			if (LogListHasFocus(pMsg->hwnd) && LogListHasMenuItem(CGitLogListBase::ID_REBASE_SKIP))
			{
				m_CommitList.SetSelectedAction(CTGitPath::LOGACTIONS_REBASE_SKIP);
				return TRUE;
			}
			break;
		case 'Q':
			if (LogListHasFocus(pMsg->hwnd) && LogListHasMenuItem(CGitLogListBase::ID_REBASE_SQUASH))
			{
				m_CommitList.SetSelectedAction(CTGitPath::LOGACTIONS_REBASE_SQUASH);
				return TRUE;
			}
			break;
		case 'E':
			if (LogListHasFocus(pMsg->hwnd) && LogListHasMenuItem(CGitLogListBase::ID_REBASE_EDIT))
			{
				m_CommitList.SetSelectedAction(CTGitPath::LOGACTIONS_REBASE_EDIT);
				return TRUE;
			}
			break;
		case 'A':
			if(LogListHasFocus(pMsg->hwnd) && GetAsyncKeyState(VK_CONTROL) & 0x8000)
			{
				// select all entries
				for (int i = 0; i < m_CommitList.GetItemCount(); ++i)
				{
					m_CommitList.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
				}
				return TRUE;
			}
			break;
		case VK_F5:
			{
				Refresh();
				return TRUE;
			}
			break;
		case VK_RETURN:
			{
				if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
				{
					if (GetDlgItem(IDC_REBASE_CONTINUE)->IsWindowEnabled())
						GetDlgItem(IDC_REBASE_CONTINUE)->SetFocus();
					else if (GetDlgItem(IDC_REBASE_ABORT)->IsWindowEnabled())
						GetDlgItem(IDC_REBASE_ABORT)->SetFocus();
					else
						GetDlgItem(IDHELP)->SetFocus();
					return TRUE;
				}
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
	else if (pMsg->message == WM_NEXTDLGCTL)
	{
		HWND hwnd = GetFocus()->GetSafeHwnd();
		if (hwnd == m_LogMessageCtrl.GetSafeHwnd() || hwnd == m_wndOutputRebase.GetSafeHwnd())
		{
			if (GetDlgItem(IDC_REBASE_CONTINUE)->IsWindowEnabled())
				GetDlgItem(IDC_REBASE_CONTINUE)->SetFocus();
			else if (GetDlgItem(IDC_REBASE_ABORT)->IsWindowEnabled())
				GetDlgItem(IDC_REBASE_ABORT)->SetFocus();
			else
				GetDlgItem(IDHELP)->SetFocus();
			return TRUE;
		}
	}
	m_tooltips.RelayEvent(pMsg);
	return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}

bool CRebaseDlg::LogListHasFocus(HWND hwnd)
{
	TCHAR buff[128] = { 0 };
	::GetClassName(hwnd, buff, 128);

	if(_tcsnicmp(buff, _T("SysListView32"), 128) == 0)
		return true;
	return false;
}

bool CRebaseDlg::LogListHasMenuItem(int i)
{
	return (m_CommitList.m_ContextMenuMask & m_CommitList.GetContextMenuBit(i)) != 0;
}

int CRebaseDlg::CheckRebaseCondition()
{
	this->m_ctrlTabCtrl.SetActiveTab(REBASE_TAB_LOG);

	if( !g_Git.CheckCleanWorkTree()  )
	{
		if (CMessageBox::Show(NULL, IDS_ERROR_NOCLEAN_STASH, IDS_APPNAME, 1, IDI_QUESTION, IDS_STASHBUTTON, IDS_ABORTBUTTON) == 1)
		{
			CString cmd,out;
			cmd=_T("git.exe stash");
			this->AddLogString(cmd);
			if (g_Git.Run(cmd, &out, CP_UTF8))
			{
				CMessageBox::Show(NULL,out,_T("TortoiseGit"),MB_OK);
				return -1;
			}

		}
		else
			return -1;
	}
	//Todo Check $REBASE_ROOT
	//Todo Check $DOTEST

	if (!CAppUtils::CheckUserData())
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
		cmd.Format(_T("git.exe checkout %s --"), this->m_BranchCtrl.GetString());
		this->AddLogString(cmd);
		while (true)
		{
			out.Empty();
			if (g_Git.Run(cmd, &out, CP_UTF8))
			{
				this->AddLogString(out);
				if (CMessageBox::Show(m_hWnd, out + _T("\nRetry?"), _T("TortoiseGit"), MB_YESNO | MB_ICONERROR) != IDYES)
					return -1;
			}
			else
				break;
		}

		this->AddLogString(out);
	}

	cmd=_T("git.exe rev-parse --verify HEAD");
	if(g_Git.Run(cmd,&out,CP_UTF8))
	{
		AddLogString(CString(MAKEINTRESOURCE(IDS_PROC_NOHEAD)));
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
	if (g_Git.GetHash(m_OrigUpstreamHash, m_UpstreamCtrl.GetString()))
	{
		MessageBox(g_Git.GetGitLastErr(_T("Could not get hash of \"") + m_UpstreamCtrl.GetString() + _T("\".")), _T("TortoiseGit"), MB_ICONERROR);
		return -1;
	}

	if( !this->m_IsCherryPick )
	{
		cmd.Format(_T("git.exe checkout -f %s --"), m_OrigUpstreamHash.ToString());
		this->AddLogString(cmd);
		while (true)
		{
			out.Empty();
			if (g_Git.Run(cmd, &out, CP_UTF8))
			{
				this->AddLogString(out);
				if (CMessageBox::Show(m_hWnd, out + _T("\nRetry?"), _T("TortoiseGit"), MB_YESNO | MB_ICONERROR) != IDYES)
					return -1;
			}
			else
				break;
		}
	}

	CString log;
	if( !this->m_IsCherryPick )
	{
		if (g_Git.GetHash(m_OrigBranchHash, m_BranchCtrl.GetString()))
		{
			MessageBox(g_Git.GetGitLastErr(_T("Could not get hash of \"") + m_BranchCtrl.GetString() + _T("\".")), _T("TortoiseGit"), MB_ICONERROR);
			return -1;
		}
		log.Format(_T("%s\r\n"), CString(MAKEINTRESOURCE(IDS_PROC_REBASE_STARTREBASE)));
	}
	else
		log.Format(_T("%s\r\n"), CString(MAKEINTRESOURCE(IDS_PROC_REBASE_STARTCHERRYPICK)));

	this->AddLogString(log);
	return 0;
}
int CRebaseDlg::VerifyNoConflict()
{
	CTGitPathList list;
	if(g_Git.ListConflictFile(list))
	{
		AddLogString(_T("Get conflict files fail"));
		return -1;
	}
	if (!list.IsEmpty())
	{
		CMessageBox::Show(NULL, IDS_PROGRS_CONFLICTSOCCURED, IDS_APPNAME, MB_OK);
		return -1;
	}
	return 0;

}
int CRebaseDlg::FinishRebase()
{
	if(this->m_IsCherryPick) //cherry pick mode no "branch", working at upstream branch
	{
		m_sStatusText.LoadString(IDS_DONE);
		m_CtrlStatusText.SetWindowText(m_sStatusText);
		m_bStatusWarning = false;
		m_CtrlStatusText.Invalidate();
		return 0;
	}

	CGitHash head;
	if (g_Git.GetHash(head, _T("HEAD")))
	{
		MessageBox(g_Git.GetGitLastErr(_T("Could not get HEAD hash.")), _T("TortoiseGit"), MB_ICONERROR);
		return -1;
	}
	CString out,cmd;

	cmd.Format(_T("git.exe checkout -f %s --"), this->m_BranchCtrl.GetString());
	AddLogString(cmd);
	while (true)
	{
		out.Empty();
		if (g_Git.Run(cmd, &out, CP_UTF8))
		{
			AddLogString(out);
			if (CMessageBox::Show(m_hWnd, out + _T("\nRetry?"), _T("TortoiseGit"), MB_YESNO | MB_ICONERROR) != IDYES)
				return -1;
		}
		else
			break;
	}
	AddLogString(out);

	cmd.Format(_T("git.exe reset --hard %s --"), head.ToString());
	AddLogString(cmd);
	while (true)
	{
		out.Empty();
		if (g_Git.Run(cmd, &out, CP_UTF8))
		{
			AddLogString(out);
			if (CMessageBox::Show(m_hWnd, out + _T("\nRetry?"), _T("TortoiseGit"), MB_YESNO | MB_ICONERROR) != IDYES)
				return -1;
		}
		else
			break;
	}
	AddLogString(out);

	while (m_ctrlTabCtrl.GetTabsNum() > 1)
		m_ctrlTabCtrl.RemoveTab(0);
	m_CtrlStatusText.SetWindowText(CString(MAKEINTRESOURCE(IDS_PROC_REBASEFINISHED)));
	m_sStatusText = CString(MAKEINTRESOURCE(IDS_PROC_REBASEFINISHED));
	m_bStatusWarning = false;
	m_CtrlStatusText.Invalidate();

	return 0;
}
void CRebaseDlg::OnBnClickedContinue()
{
	if( m_RebaseStage == REBASE_DONE)
	{
		OnOK();
		return;
	}

	if (m_RebaseStage == CHOOSE_BRANCH || m_RebaseStage == CHOOSE_COMMIT_PICK_MODE)
	{
		if (CheckRebaseCondition())
			return;
	}

	if( this->m_IsFastForward )
	{
		CString cmd,out;
		CString oldbranch = g_Git.GetCurrentBranch();
		if( oldbranch != m_BranchCtrl.GetString() )
		{
			cmd.Format(_T("git.exe checkout %s --"), m_BranchCtrl.GetString());
			AddLogString(cmd);
			while (true)
			{
				out.Empty();
				if (g_Git.Run(cmd, &out, CP_UTF8))
				{
					this->m_ctrlTabCtrl.SetActiveTab(REBASE_TAB_LOG);
					AddLogString(out);
					if (CMessageBox::Show(m_hWnd, out + _T("\nRetry?"), _T("TortoiseGit"), MB_YESNO | MB_ICONERROR) != IDYES)
						return;
				}
				else
					break;
			}
		}
		AddLogString(out);
		out.Empty();
		if (g_Git.GetHash(m_OrigBranchHash, m_BranchCtrl.GetString()))
		{
			MessageBox(g_Git.GetGitLastErr(_T("Could not get hash of \"") + m_BranchCtrl.GetString() + _T("\".")), _T("TortoiseGit"), MB_ICONERROR);
			return;
		}
		if (g_Git.GetHash(m_OrigUpstreamHash, m_UpstreamCtrl.GetString()))
		{
			MessageBox(g_Git.GetGitLastErr(_T("Could not get hash of \"") + m_UpstreamCtrl.GetString() + _T("\".")), _T("TortoiseGit"), MB_ICONERROR);
			return;
		}

		if(!g_Git.IsFastForward(this->m_BranchCtrl.GetString(),this->m_UpstreamCtrl.GetString()))
		{
			this->m_ctrlTabCtrl.SetActiveTab(REBASE_TAB_LOG);
			AddLogString(_T("No fast forward possible.\r\nMaybe repository changed"));
			return;
		}

		cmd.Format(_T("git.exe reset --hard %s --"), g_Git.FixBranchName(this->m_UpstreamCtrl.GetString()));
		CString log;
		log.Format(IDS_PROC_REBASE_FFTO, m_UpstreamCtrl.GetString());
		this->AddLogString(log);

		AddLogString(cmd);
		this->m_ctrlTabCtrl.SetActiveTab(REBASE_TAB_LOG);
		while (true)
		{
			out.Empty();
			if (g_Git.Run(cmd, &out, CP_UTF8))
			{
				AddLogString(CString(MAKEINTRESOURCE(IDS_FAIL)));
				AddLogString(out);
				if (CMessageBox::Show(m_hWnd, out + _T("\nRetry?"), _T("TortoiseGit"), MB_YESNO | MB_ICONERROR) != IDYES)
					return;
			}
			else
				break;
		}
		AddLogString(out);
		AddLogString(CString(MAKEINTRESOURCE(IDS_DONE)));
		m_RebaseStage = REBASE_DONE;
		UpdateCurrentStatus();
		return;

	}
	if( m_RebaseStage == CHOOSE_BRANCH|| m_RebaseStage == CHOOSE_COMMIT_PICK_MODE )
	{
		if(CheckRebaseCondition())
			return ;
		m_RebaseStage = REBASE_START;
		m_FileListCtrl.Clear();
		m_FileListCtrl.SetHasCheckboxes(false);
		m_FileListCtrl.m_CurrentVersion = L"";
		m_ctrlTabCtrl.SetTabLabel(REBASE_TAB_CONFLICT, CString(MAKEINTRESOURCE(IDS_PROC_CONFLICTFILES)));
		m_ctrlTabCtrl.AddTab(&m_wndOutputRebase, CString(MAKEINTRESOURCE(IDS_LOG)), 2);
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
		curRev->GetAction(&m_CommitList)|=CTGitPath::LOGACTIONS_REBASE_DONE;
		this->UpdateCurrentStatus();

	}

	if( m_RebaseStage == REBASE_CONFLICT )
	{
		if(VerifyNoConflict())
			return;

		GitRev *curRev=(GitRev*)m_CommitList.m_arShownList[m_CurrentRebaseIndex];
		// ***************************************************
		// ATTENTION: Similar code in CommitDlg.cpp!!!
		// ***************************************************
		CMassiveGitTask mgtReAddAfterCommit(_T("add --ignore-errors -f"));
		CMassiveGitTask mgtAdd(_T("add -f"));
		CMassiveGitTask mgtUpdateIndexForceRemove(_T("update-index --force-remove"));
		CMassiveGitTask mgtUpdateIndex(_T("update-index"));
		CMassiveGitTask mgtRm(_T("rm  --ignore-unmatch"));
		CMassiveGitTask mgtRmFCache(_T("rm -f --cache"));
		CMassiveGitTask mgtReset(_T("reset"), TRUE, true);
		for (int i = 0; i < m_FileListCtrl.GetItemCount(); i++)
		{
			CTGitPath *entry = (CTGitPath *)m_FileListCtrl.GetItemData(i);
			if (entry->m_Checked)
			{
				if (entry->m_Action & CTGitPath::LOGACTIONS_UNVER)
					mgtAdd.AddFile(entry->GetGitPathString());
				else if (entry->m_Action & CTGitPath::LOGACTIONS_DELETED)
					mgtUpdateIndexForceRemove.AddFile(entry->GetGitPathString());
				else
					mgtUpdateIndex.AddFile(entry->GetGitPathString());

				if (entry->m_Action & CTGitPath::LOGACTIONS_REPLACED)
					mgtRm.AddFile(entry->GetGitOldPathString());
			}
			else
			{
				if (entry->m_Action & CTGitPath::LOGACTIONS_ADDED || entry->m_Action & CTGitPath::LOGACTIONS_REPLACED)
				{
					mgtRmFCache.AddFile(entry->GetGitPathString());
					mgtReAddAfterCommit.AddFile(*entry);

					if (entry->m_Action & CTGitPath::LOGACTIONS_REPLACED && !entry->GetGitOldPathString().IsEmpty())
						mgtReset.AddFile(entry->GetGitOldPathString());
				}
				else if(!(entry->m_Action & CTGitPath::LOGACTIONS_UNVER))
					mgtReset.AddFile(entry->GetGitPathString());
			}
		}

		BOOL cancel = FALSE;
		bool successful = true;
		successful = successful && mgtAdd.Execute(cancel);
		successful = successful && mgtUpdateIndexForceRemove.Execute(cancel);
		successful = successful && mgtUpdateIndex.Execute(cancel);
		successful = successful && mgtRm.Execute(cancel);
		successful = successful && mgtRmFCache.Execute(cancel);
		successful = successful && mgtReset.Execute(cancel);

		if (!successful)
		{
			AddLogString(_T("An error occurred while updating the index."));
			return;
		}

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

		// update commit message if needed
		CString str = m_LogMessageCtrl.GetText().Trim();
		if (str != (curRev->GetSubject() + _T("\n") + curRev->GetBody()).Trim())
		{
			if (str.Trim().IsEmpty())
			{
				CMessageBox::Show(NULL, IDS_PROC_COMMITMESSAGE_EMPTY,IDS_APPNAME, MB_OK | MB_ICONERROR);
				return;
			}
			CString tempfile = ::GetTempFile();
			CAppUtils::SaveCommitUnicodeFile(tempfile, str);

			out.Empty();
			cmd.Format(_T("git.exe commit --amend -F \"%s\""), tempfile);
			AddLogString(cmd);

			if (g_Git.Run(cmd, &out, CP_UTF8))
			{
				AddLogString(out);
				if (!g_Git.CheckCleanWorkTree())
				{
					CMessageBox::Show(NULL, out, _T("TortoiseGit"), MB_OK | MB_ICONERROR);
					return;
				}
			}

			AddLogString(out);
		}

		if (((DWORD)CRegStdDWORD(_T("Software\\TortoiseGit\\ReaddUnselectedAddedFilesAfterCommit"), TRUE)) == TRUE)
		{
			BOOL cancel = FALSE;
			mgtReAddAfterCommit.Execute(cancel);
		}

		this->m_ctrlTabCtrl.SetActiveTab(REBASE_TAB_LOG);
		if( curRev->GetAction(&m_CommitList) & CTGitPath::LOGACTIONS_REBASE_EDIT)
		{
			m_RebaseStage=REBASE_EDIT;
			this->m_ctrlTabCtrl.SetActiveTab(REBASE_TAB_MESSAGE);
			this->UpdateCurrentStatus();
			return;
		}
		else
		{
			m_RebaseStage=REBASE_CONTINUE;
			curRev->GetAction(&m_CommitList)|=CTGitPath::LOGACTIONS_REBASE_DONE;
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
			CMessageBox::Show(NULL, IDS_PROC_COMMITMESSAGE_EMPTY,IDS_APPNAME, MB_OK | MB_ICONERROR);
				return;
		}

		CString tempfile=::GetTempFile();
		CAppUtils::SaveCommitUnicodeFile(tempfile, str);

		CString out,cmd;

		if(  m_RebaseStage == REBASE_SQUASH_EDIT )
			cmd.Format(_T("git.exe commit %s-F \"%s\""), m_SquashFirstMetaData, tempfile);
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

		::DeleteFile(tempfile);
		AddLogString(out);
		m_SquashMessage.Empty();
		this->m_ctrlTabCtrl.SetActiveTab(REBASE_TAB_LOG);
		m_RebaseStage=REBASE_CONTINUE;
		curRev->GetAction(&m_CommitList)|=CTGitPath::LOGACTIONS_REBASE_DONE;
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

		if( curRev->GetAction(&m_CommitList)&CTGitPath::LOGACTIONS_REBASE_SQUASH )
			return 0;
		if( curRev->GetAction(&m_CommitList)&CTGitPath::LOGACTIONS_REBASE_SKIP)
		{
			if(m_CommitList.m_IsOldFirst)
				++index;
			else
				--index;
		}
		else
			return -1;

	}while(curRev->GetAction(&m_CommitList)&CTGitPath::LOGACTIONS_REBASE_SKIP);

	return -1;

}
int CRebaseDlg::GoNext()
{
	if(m_CommitList.m_IsOldFirst)
		++m_CurrentRebaseIndex;
	else
		--m_CurrentRebaseIndex;
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
			Text.LoadString(IDS_PROC_STARTREBASEFFBUTTON);
		else
			Text.LoadString(IDS_PROC_STARTREBASEBUTTON);
		break;

	case REBASE_START:
	case REBASE_ERROR:
	case REBASE_CONTINUE:
	case REBASE_SQUASH_CONFLICT:
		Text.LoadString(IDS_CONTINUEBUTTON);
		break;

	case REBASE_CONFLICT:
		Text.LoadString(IDS_COMMITBUTTON);
		break;
	case REBASE_EDIT:
		Text.LoadString(IDS_AMENDBUTTON);
		break;

	case REBASE_SQUASH_EDIT:
		Text.LoadString(IDS_COMMITBUTTON);
		break;

	case REBASE_ABORT:
	case REBASE_FINISH:
		Text.LoadString(IDS_FINISHBUTTON);
		break;

	case REBASE_DONE:
		Text.LoadString(IDS_DONE);
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
		this->GetDlgItem(IDC_BUTTON_UP2)->EnableWindow(TRUE);
		this->GetDlgItem(IDC_BUTTON_DOWN2)->EnableWindow(TRUE);

		if(!m_IsCherryPick)
		{
			this->GetDlgItem(IDC_REBASE_COMBOXEX_BRANCH)->EnableWindow(TRUE);
			this->GetDlgItem(IDC_REBASE_COMBOXEX_UPSTREAM)->EnableWindow(TRUE);
			this->GetDlgItem(IDC_REBASE_CHECK_FORCE)->EnableWindow(TRUE);
		}
		this->m_CommitList.m_ContextMenuMask |= m_CommitList.GetContextMenuBit(CGitLogListBase::ID_REBASE_PICK)|
												m_CommitList.GetContextMenuBit(CGitLogListBase::ID_REBASE_SQUASH)|
												m_CommitList.GetContextMenuBit(CGitLogListBase::ID_REBASE_EDIT)|
												m_CommitList.GetContextMenuBit(CGitLogListBase::ID_REBASE_SKIP);
		break;

	case REBASE_START:
	case REBASE_CONTINUE:
	case REBASE_ABORT:
	case REBASE_ERROR:
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
		this->GetDlgItem(IDC_BUTTON_UP2)->EnableWindow(FALSE);
		this->GetDlgItem(IDC_BUTTON_DOWN2)->EnableWindow(FALSE);

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

	}
	else if (m_RebaseStage != REBASE_ERROR)
	{
		this->GetDlgItem(IDC_REBASE_CONTINUE)->EnableWindow(TRUE);
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

	int finishedCommits = index - 1; // introduced an variable which shows the number handled revisions for the progress bars
	if (m_RebaseStage == REBASE_FINISH || finishedCommits == -1)
		finishedCommits = index;

	m_ProgressBar.SetRange32(0, m_CommitList.GetItemCount());
	m_ProgressBar.SetPos(finishedCommits);
	if (m_pTaskbarList)
	{
		m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NORMAL);
		m_pTaskbarList->SetProgressValue(m_hWnd, finishedCommits, m_CommitList.GetItemCount());
	}

	if(m_CurrentRebaseIndex>=0 && m_CurrentRebaseIndex< m_CommitList.GetItemCount())
	{
		CString text;
		text.Format(IDS_PROC_REBASING_PROGRESS, index, m_CommitList.GetItemCount());
		m_sStatusText = text;
		m_CtrlStatusText.SetWindowText(text);
		m_bStatusWarning = false;
		m_CtrlStatusText.Invalidate();
	}

	GitRev *prevRev=NULL, *curRev=NULL;

	if( m_CurrentRebaseIndex >= 0 && m_CurrentRebaseIndex< m_CommitList.m_arShownList.GetSize())
	{
		curRev=(GitRev*)m_CommitList.m_arShownList[m_CurrentRebaseIndex];
	}

	for (int i = 0; i < m_CommitList.m_arShownList.GetSize(); ++i)
	{
		prevRev=(GitRev*)m_CommitList.m_arShownList[i];
		if(prevRev->GetAction(&m_CommitList) & CTGitPath::LOGACTIONS_REBASE_CURRENT)
		{
			prevRev->GetAction(&m_CommitList) &= ~ CTGitPath::LOGACTIONS_REBASE_CURRENT;
			m_CommitList.GetItemRect(i,&rect,LVIR_BOUNDS);
			m_CommitList.InvalidateRect(rect);
		}
	}

	if(curRev)
	{
		curRev->GetAction(&m_CommitList) |= CTGitPath::LOGACTIONS_REBASE_CURRENT;
		m_CommitList.GetItemRect(m_CurrentRebaseIndex,&rect,LVIR_BOUNDS);
		m_CommitList.InvalidateRect(rect);
	}
	m_CommitList.EnsureVisible(m_CurrentRebaseIndex,FALSE);
}

void CRebaseDlg::UpdateCurrentStatus()
{
	SetContinueButtonText();
	SetControlEnable();
	UpdateProgress();
}

void CRebaseDlg::AddLogString(CString str)
{
	this->m_wndOutputRebase.SendMessage(SCI_SETREADONLY, FALSE);
	CStringA sTextA = m_wndOutputRebase.StringForControl(str);//CUnicodeUtils::GetUTF8(str);
	this->m_wndOutputRebase.SendMessage(SCI_DOCUMENTEND);
	this->m_wndOutputRebase.SendMessage(SCI_REPLACESEL, 0, (LPARAM)(LPCSTR)sTextA);
	this->m_wndOutputRebase.SendMessage(SCI_REPLACESEL, 0, (LPARAM)(LPCSTR)"\n");
	this->m_wndOutputRebase.SendMessage(SCI_SETREADONLY, TRUE);
}

int CRebaseDlg::GetCurrentCommitID()
{
	if(m_CommitList.m_IsOldFirst)
	{
		return this->m_CurrentRebaseIndex+1;

	}
	else
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
	int mode=pRev->GetAction(&m_CommitList) & CTGitPath::LOGACTIONS_REBASE_MODE_MASK;
	CString nocommit;

	if( mode== CTGitPath::LOGACTIONS_REBASE_SKIP)
	{
		pRev->GetAction(&m_CommitList)|= CTGitPath::LOGACTIONS_REBASE_DONE;
		return 0;
	}

	bool nextCommitIsSquash = (CheckNextCommitIsSquash() == 0);
	if (nextCommitIsSquash || mode != CTGitPath::LOGACTIONS_REBASE_PICK)
	{ // next commit is squash or not pick
		if (!this->m_SquashMessage.IsEmpty())
			this->m_SquashMessage += _T("\n\n");
		this->m_SquashMessage += pRev->GetSubject();
		this->m_SquashMessage += _T("\n");
		this->m_SquashMessage += pRev->GetBody().TrimRight();
	}
	else
	{
		this->m_SquashMessage.Empty();
		m_SquashFirstMetaData.Empty();
	}

	if (nextCommitIsSquash || mode == CTGitPath::LOGACTIONS_REBASE_SQUASH)
	{ // next or this commit is squash
		nocommit=_T(" --no-commit ");
	}

	if (nextCommitIsSquash && mode != CTGitPath::LOGACTIONS_REBASE_SQUASH)
		m_SquashFirstMetaData.Format(_T("--date=%s --author=\"%s <%s>\" "), pRev->GetAuthorDate().Format(_T("%Y-%m-%dT%H:%M:%S")), pRev->GetAuthorName(), pRev->GetAuthorEmail());

	CString log;
	log.Format(_T("%s %d: %s"),CTGitPath::GetActionName(mode),this->GetCurrentCommitID(),pRev->m_CommitHash.ToString());
	AddLogString(log);
	AddLogString(pRev->GetSubject());
	if (pRev->GetSubject().IsEmpty())
	{
		CMessageBox::Show(m_hWnd, IDS_PROC_REBASE_EMPTYCOMMITMSG, IDS_APPNAME, MB_OK | MB_ICONEXCLAMATION);
		mode = CTGitPath::LOGACTIONS_REBASE_EDIT;
	}

	CString cherryPickedFrom;
	if (m_bAddCherryPickedFrom)
		cherryPickedFrom = _T("-x ");
	else if (!m_IsCherryPick && nocommit.IsEmpty())
		cherryPickedFrom = _T("--ff "); // for issue #1833: "If the current HEAD is the same as the parent of the cherry-pick’ed commit, then a fast forward to this commit will be performed."

	cmd.Format(_T("git.exe cherry-pick %s%s %s"), cherryPickedFrom, nocommit, pRev->m_CommitHash.ToString());

	if(g_Git.Run(cmd,&out,CP_UTF8))
	{
		AddLogString(out);
		CTGitPathList list;
		if(g_Git.ListConflictFile(list))
		{
			AddLogString(_T("Get conflict files fail"));
			return -1;
		}
		if (list.IsEmpty())
		{
			if(mode ==  CTGitPath::LOGACTIONS_REBASE_PICK)
			{
				if (m_pTaskbarList)
					m_pTaskbarList->SetProgressState(m_hWnd, TBPF_ERROR);
				if (m_bAutoSkipFailedCommit || CMessageBox::ShowCheck(m_hWnd, IDS_CHERRYPICKFAILEDSKIP, IDS_APPNAME, 1, IDI_QUESTION, IDS_SKIPBUTTON, IDS_MSGBOX_CANCEL, 0, NULL, IDS_DO_SAME_FOR_REST, &m_bAutoSkipFailedCommit) == 1)
				{
					bool resetOK = false;
					while (!resetOK)
					{
						out.Empty();
						if (g_Git.Run(_T("git.exe reset --hard"), &out, CP_UTF8))
						{
							AddLogString(out);
							if (CMessageBox::Show(m_hWnd, _T("Retry?\nUnrecoverable error on cleanup:\n") + out, _T("TortoiseGit"), MB_YESNO | MB_ICONERROR) != IDYES)
								break;
						}
						else
							resetOK = true;
					}

					if (resetOK)
					{
						pRev->GetAction(&m_CommitList) = CTGitPath::LOGACTIONS_REBASE_SKIP;
						m_CommitList.Invalidate();
						return 0;
					}
				}

				m_RebaseStage = REBASE_ERROR;
				AddLogString(_T("An unrecoverable error occurred."));
				return -1;
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

		if (m_pTaskbarList)
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_ERROR);
		if(mode == CTGitPath::LOGACTIONS_REBASE_SQUASH)
			m_RebaseStage = REBASE_SQUASH_CONFLICT;
		else
			m_RebaseStage = REBASE_CONFLICT;
		return -1;

	}
	else
	{
		AddLogString(out);
		if(mode ==  CTGitPath::LOGACTIONS_REBASE_PICK)
		{
			pRev->GetAction(&m_CommitList)|= CTGitPath::LOGACTIONS_REBASE_DONE;
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
		else if(mode == CTGitPath::LOGACTIONS_REBASE_SQUASH)
			pRev->GetAction(&m_CommitList)|= CTGitPath::LOGACTIONS_REBASE_DONE;
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
	CBlockCacheForPath cacheBlock(g_Git.m_CurrentDir);

	int ret=0;
	while(1)
	{
		if( m_RebaseStage == REBASE_START )
		{
			if( this->StartRebase() )
			{
				ret = -1;
				break;
			}
			m_RebaseStage = REBASE_CONTINUE;

		}
		else if( m_RebaseStage == REBASE_CONTINUE )
		{
			this->GoNext();
			SendMessage(MSG_REBASE_UPDATE_UI);
			if(IsEnd())
			{
				ret = 0;
				m_RebaseStage = REBASE_FINISH;

			}
			else
			{
				ret = DoRebase();

				if( ret )
				{
					break;
				}
			}

		}
		else if( m_RebaseStage == REBASE_FINISH )
		{
			SendMessage(MSG_REBASE_UPDATE_UI);
			m_RebaseStage = REBASE_DONE;
			break;

		}
		else
		{
			break;
		}
		this->PostMessage(MSG_REBASE_UPDATE_UI);
	}

	InterlockedExchange(&m_bThreadRunning, FALSE);
	this->PostMessage(MSG_REBASE_UPDATE_UI);
	return ret;
}

void CRebaseDlg::ListConflictFile()
{
	this->m_FileListCtrl.Clear();
	m_FileListCtrl.SetHasCheckboxes(true);
	CTGitPathList list;
	CTGitPath path;
	list.AddPath(path);

	m_FileListCtrl.m_bIsRevertTheirMy = !m_IsCherryPick;

	this->m_FileListCtrl.GetStatus(&list,true);
	this->m_FileListCtrl.Show(CTGitPath::LOGACTIONS_UNMERGED|CTGitPath::LOGACTIONS_MODIFIED|CTGitPath::LOGACTIONS_ADDED|CTGitPath::LOGACTIONS_DELETED,
							   CTGitPath::LOGACTIONS_UNMERGED);

	m_FileListCtrl.Check(GITSLC_SHOWFILES);
	bool hasSubmoduleChange = false;
	for (int i = 0; i < m_FileListCtrl.GetItemCount(); i++)
	{
		CTGitPath *entry = (CTGitPath *)m_FileListCtrl.GetItemData(i);
		if (entry->IsDirectory())
		{
			hasSubmoduleChange = true;
			break;
		}
	}

	if (hasSubmoduleChange)
	{
		m_CtrlStatusText.SetWindowText(m_sStatusText + _T(", ") + CString(MAKEINTRESOURCE(IDS_CARE_SUBMODULE_CHANGES)));
		m_bStatusWarning = true;
		m_CtrlStatusText.Invalidate();
	}
	else
	{
		m_CtrlStatusText.SetWindowText(m_sStatusText);
		m_bStatusWarning = false;
		m_CtrlStatusText.Invalidate();
	}
}

LRESULT CRebaseDlg::OnRebaseUpdateUI(WPARAM,LPARAM)
{
	if (m_RebaseStage == REBASE_FINISH)
	{
		FinishRebase();
		return 0;
	}
	UpdateCurrentStatus();
	if (m_RebaseStage == REBASE_DONE && m_pTaskbarList)
		m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NOPROGRESS); // do not show progress on taskbar any more to show we finished
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
		if (m_pTaskbarList)
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_ERROR);
		this->m_LogMessageCtrl.Call(SCI_SETREADONLY, FALSE);
		this->m_LogMessageCtrl.SetText(curRev->GetSubject()+_T("\n")+curRev->GetBody());
		break;
	case REBASE_EDIT:
		this->m_ctrlTabCtrl.SetActiveTab(REBASE_TAB_MESSAGE);
		if (m_pTaskbarList)
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_PAUSED);
		this->m_LogMessageCtrl.Call(SCI_SETREADONLY, FALSE);
		this->m_LogMessageCtrl.SetText(curRev->GetSubject()+_T("\n")+curRev->GetBody());
		break;
	case REBASE_SQUASH_EDIT:
		this->m_ctrlTabCtrl.SetActiveTab(REBASE_TAB_MESSAGE);
		this->m_LogMessageCtrl.Call(SCI_SETREADONLY, FALSE);
		this->m_LogMessageCtrl.SetText(this->m_SquashMessage);
		if (m_pTaskbarList)
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_PAUSED);
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
	if (m_pTaskbarList)
		m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NOPROGRESS);

	m_tooltips.Pop();

	CString cmd,out;
	CString pron = m_OrigUpstreamHash.ToString();
	if(m_OrigUpstreamHash.IsEmpty())
	{
		__super::OnCancel();
	}

	if(m_RebaseStage == CHOOSE_BRANCH || m_RebaseStage== CHOOSE_COMMIT_PICK_MODE)
	{
		return;
	}

	if(CMessageBox::Show(NULL, IDS_PROC_REBASE_ABORT, IDS_APPNAME, MB_YESNO) != IDYES)
		return;

	if(this->m_IsFastForward)
	{
		cmd.Format(_T("git.exe reset --hard %s --"),this->m_OrigBranchHash.ToString());
		while (true)
		{
			out.Empty();
			if (g_Git.Run(cmd, &out, CP_UTF8))
			{
				AddLogString(out);
				if (CMessageBox::Show(m_hWnd, _T("Retry?\nUnrecoverable error on cleanup:\n") + out, _T("TortoiseGit"), MB_YESNO | MB_ICONERROR) != IDYES)
					break;
			}
			else
				break;
		}
		__super::OnCancel();
		return;
	}
	cmd.Format(_T("git.exe checkout -f %s --"), g_Git.FixBranchName(this->m_UpstreamCtrl.GetString()));
	while (true)
	{
		out.Empty();
		if (g_Git.Run(cmd, &out, CP_UTF8))
		{
			AddLogString(out);
			if (CMessageBox::Show(this->m_hWnd, _T("Retry?\nUnrecoverable error on cleanup:\n") + out, _T("TortoiseGit"), MB_YESNO | MB_ICONERROR) != IDYES)
			{
				__super::OnCancel();
				return;
			}
		}
		else
			break;
	}

	cmd.Format(_T("git.exe reset --hard %s --"), this->m_OrigUpstreamHash.ToString());
	while (true)
	{
		out.Empty();
		if (g_Git.Run(cmd, &out, CP_UTF8))
		{
			AddLogString(out);
			if (CMessageBox::Show(m_hWnd, _T("Retry?\nUnrecoverable error on cleanup:\n") + out, _T("TortoiseGit"), MB_YESNO | MB_ICONERROR) != IDYES)
			{
				__super::OnCancel();
				return;
			}
		}
		else
			break;
	}

	if(this->m_IsCherryPick) //there are not "branch" at cherry pick mode
	{
		__super::OnCancel();
		return;
	}

	cmd.Format(_T("git.exe checkout -f %s --"), this->m_BranchCtrl.GetString());
	if(g_Git.Run(cmd,&out,CP_UTF8))
	{
		AddLogString(out);
		::MessageBox(this->m_hWnd, _T("Unrecoverable error on cleanup:\n") + out, _T("TortoiseGit"), MB_ICONERROR);
		__super::OnCancel();
		return;
	}

	cmd.Format(_T("git.exe reset --hard %s --"), this->m_OrigBranchHash.ToString());
	while (true)
	{
		out.Empty();
		if (g_Git.Run(cmd, &out, CP_UTF8))
		{
			AddLogString(out);
			if (CMessageBox::Show(m_hWnd, _T("Retry?\nUnrecoverable error on cleanup:\n") + out, _T("TortoiseGit"), MB_YESNO | MB_ICONERROR) != IDYES)
				break;
		}
		else
			break;
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
	this->UpdateData();
	this->FetchLogList();
}

void CRebaseDlg::OnBnClickedRebasePostButton()
{
	this->m_Upstream=this->m_UpstreamCtrl.GetString();
	this->m_Branch=this->m_BranchCtrl.GetString();

	this->EndDialog((int)(IDC_REBASE_POST_BUTTON+this->m_PostButton.GetCurrentEntry()));
}

LRESULT CRebaseDlg::OnGitStatusListCtrlNeedsRefresh(WPARAM, LPARAM)
{
	Refresh();
	return 0;
}

void CRebaseDlg::Refresh()
{
	if (m_RebaseStage == REBASE_CONFLICT || m_RebaseStage == REBASE_SQUASH_CONFLICT)
	{
		ListConflictFile();
		return;
	}

	if(this->m_IsCherryPick)
		return ;

	if(this->m_RebaseStage == CHOOSE_BRANCH )
	{
		this->UpdateData();
		this->FetchLogList();
	}
}

void CRebaseDlg::OnBnClickedButtonUp2()
{
	POSITION pos;
	pos = m_CommitList.GetFirstSelectedItemPosition();

	// do nothing if the first selected item is the first item in the list
	if (m_CommitList.GetNextSelectedItem(pos) == 0)
		return;

	pos = m_CommitList.GetFirstSelectedItemPosition();

	bool changed = false;
	while(pos)
	{
		int index=m_CommitList.GetNextSelectedItem(pos);
		if(index>=1)
		{
			CGitHash old = m_CommitList.m_logEntries[index-1];
			m_CommitList.m_logEntries[index-1] = m_CommitList.m_logEntries[index];
			m_CommitList.m_logEntries[index] = old;
			m_CommitList.RecalculateShownList(&m_CommitList.m_arShownList);
			m_CommitList.SetItemState(index-1, LVIS_SELECTED, LVIS_SELECTED);
			m_CommitList.SetItemState(index, 0, LVIS_SELECTED);
			changed = true;
		}
	}
	if (changed)
	{
		pos = m_CommitList.GetFirstSelectedItemPosition();
		m_CommitList.EnsureVisible(m_CommitList.GetNextSelectedItem(pos), false);
		m_CommitList.Invalidate();
		m_CommitList.SetFocus();
	}
}

void CRebaseDlg::OnBnClickedButtonDown2()
{
	if (m_CommitList.GetSelectedCount() == 0)
		return;

	POSITION pos;
	pos = m_CommitList.GetFirstSelectedItemPosition();
	bool changed = false;
	// use an array to store all selected item indexes; the user won't select too much items
	int* indexes = NULL;
	indexes = new int[m_CommitList.GetSelectedCount()];
	int i = 0;
	while(pos)
	{
		indexes[i++] = m_CommitList.GetNextSelectedItem(pos);
	}
	// don't move any item if the last selected item is the last item in the m_CommitList
	// (that would change the order of the selected items)
	if(indexes[m_CommitList.GetSelectedCount() - 1] < m_CommitList.GetItemCount() - 1)
	{
		// iterate over the indexes backwards in order to correctly move multiselected items
		for (i = m_CommitList.GetSelectedCount() - 1; i >= 0; i--)
		{
			int index = indexes[i];
			CGitHash old = m_CommitList.m_logEntries[index+1];
			m_CommitList.m_logEntries[index+1] = m_CommitList.m_logEntries[index];
			m_CommitList.m_logEntries[index] = old;
			m_CommitList.RecalculateShownList(&m_CommitList.m_arShownList);
			m_CommitList.SetItemState(index, 0, LVIS_SELECTED);
			m_CommitList.SetItemState(index+1, LVIS_SELECTED, LVIS_SELECTED);
			changed = true;
		}
	}
	m_CommitList.EnsureVisible(indexes[m_CommitList.GetSelectedCount() - 1] + 1, false);
	delete [] indexes;
	indexes = NULL;
	if (changed)
	{
		m_CommitList.Invalidate();
		m_CommitList.SetFocus();
	}
}

LRESULT CRebaseDlg::OnTaskbarBtnCreated(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	m_pTaskbarList.Release();
	m_pTaskbarList.CoCreateInstance(CLSID_TaskbarList);
	SetUUIDOverlayIcon(m_hWnd);
	return 0;
}

void CRebaseDlg::OnLvnItemchangedLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;
	if(m_CommitList.m_bNoDispUpdates)
		return;
	if (pNMLV->iItem >= 0)
	{
		this->m_CommitList.m_nSearchIndex = pNMLV->iItem;
		if (pNMLV->iSubItem != 0)
			return;
		if ((pNMLV->iItem == m_CommitList.m_arShownList.GetCount()))
		{
			// remove the selected state
			if (pNMLV->uChanged & LVIF_STATE)
			{
				m_CommitList.SetItemState(pNMLV->iItem, 0, LVIS_SELECTED);
				FillLogMessageCtrl();
			}
			return;
		}
		if (pNMLV->uChanged & LVIF_STATE)
		{
			FillLogMessageCtrl();
		}
	}
	else
	{
		FillLogMessageCtrl();
	}
}

void CRebaseDlg::FillLogMessageCtrl()
{
	int selCount = m_CommitList.GetSelectedCount();
	if (selCount == 1 && (m_RebaseStage == CHOOSE_BRANCH || m_RebaseStage == CHOOSE_COMMIT_PICK_MODE))
	{
		POSITION pos = m_CommitList.GetFirstSelectedItemPosition();
		int selIndex = m_CommitList.GetNextSelectedItem(pos);
		GitRev* pLogEntry = reinterpret_cast<GitRev *>(m_CommitList.m_arShownList.SafeGetAt(selIndex));
		m_FileListCtrl.UpdateWithGitPathList(pLogEntry->GetFiles(&m_CommitList));
		m_FileListCtrl.m_CurrentVersion = pLogEntry->m_CommitHash;
		m_FileListCtrl.Show(GITSLC_SHOWVERSIONED);
		m_LogMessageCtrl.Call(SCI_SETREADONLY, FALSE);
		m_LogMessageCtrl.SetText(pLogEntry->GetSubject() + _T("\n") + pLogEntry->GetBody());
		m_LogMessageCtrl.Call(SCI_SETREADONLY, TRUE);
	}
}
void CRebaseDlg::OnBnClickedCheckCherryPickedFrom()
{
	UpdateData();
}

LRESULT CRebaseDlg::OnRebaseActionMessage(WPARAM, LPARAM)
{
	if (m_RebaseStage == REBASE_ERROR || m_RebaseStage == REBASE_CONFLICT)
	{
		GitRev *pRev = (GitRev*)m_CommitList.m_arShownList[m_CurrentRebaseIndex];
		int mode = pRev->GetAction(&m_CommitList) & CTGitPath::LOGACTIONS_REBASE_MODE_MASK;
		if (mode == CTGitPath::LOGACTIONS_REBASE_SKIP)
		{
			CString out;
			bool resetOK = false;
			while (!resetOK)
			{
				out.Empty();
				if (g_Git.Run(_T("git.exe reset --hard"), &out, CP_UTF8))
				{
					AddLogString(out);
					if (CMessageBox::Show(m_hWnd, _T("Retry?\nUnrecoverable error on cleanup:\n") + out, _T("TortoiseGit"), MB_YESNO | MB_ICONERROR) != IDYES)
						break;
				}
				else
					resetOK = true;
			}

			if (resetOK)
			{
				m_FileListCtrl.Clear();
				m_RebaseStage = REBASE_CONTINUE;
				UpdateCurrentStatus();
			}
		}
	}
	return 0;
}
