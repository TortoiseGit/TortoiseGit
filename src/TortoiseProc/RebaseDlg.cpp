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
#include "Settings/Settings.h"
#include "MassiveGitTask.h"
#include "CommitDlg.h"
#include "StringUtils.h"
#include "Hooks.h"
#include "LogDlg.h"

// CRebaseDlg dialog

IMPLEMENT_DYNAMIC(CRebaseDlg, CResizableStandAloneDialog)

CRebaseDlg::CRebaseDlg(CWnd* pParent /*=nullptr*/)
	: CResizableStandAloneDialog(CRebaseDlg::IDD, pParent)
	, m_bAddCherryPickedFrom(FALSE)
	, m_bStatusWarning(false)
	, m_bAutoSkipFailedCommit(FALSE)
	, m_bFinishedRebase(false)
	, m_bStashed(false)
	, m_bSplitCommit(FALSE)
	, m_bPreserveMerges(FALSE)
	, m_bRebaseAutoStart(false)
	, m_bRebaseAutoEnd(false)
	, m_RebaseStage(CHOOSE_BRANCH)
	, m_CurrentRebaseIndex(-1)
	, m_bThreadRunning(FALSE)
	, m_IsCherryPick(FALSE)
	, m_bForce(BST_UNCHECKED)
	, m_IsFastForward(FALSE)
	, m_iSquashdate(CRegDWORD(L"Software\\TortoiseGit\\SquashDate", 0))
	, m_bAbort(FALSE)
	, m_CurrentCommitEmpty(false)
{
}

CRebaseDlg::~CRebaseDlg()
{
}

void CRebaseDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_REBASE_PROGRESS, m_ProgressBar);
	DDX_Control(pDX, IDC_STATUS_STATIC, m_CtrlStatusText);
	DDX_Control(pDX, IDC_REBASE_SPLIT, m_wndSplitter);
	DDX_Control(pDX,IDC_COMMIT_LIST,m_CommitList);
	DDX_Control(pDX,IDC_REBASE_COMBOXEX_BRANCH, this->m_BranchCtrl);
	DDX_Control(pDX,IDC_REBASE_COMBOXEX_UPSTREAM,   this->m_UpstreamCtrl);
	DDX_Check(pDX, IDC_REBASE_CHECK_FORCE,m_bForce);
	DDX_Check(pDX, IDC_REBASE_CHECK_PRESERVEMERGES, m_bPreserveMerges);
	DDX_Check(pDX, IDC_CHECK_CHERRYPICKED_FROM, m_bAddCherryPickedFrom);
	DDX_Control(pDX,IDC_REBASE_POST_BUTTON,m_PostButton);
	DDX_Control(pDX, IDC_SPLITALLOPTIONS, m_SplitAllOptions);
	DDX_Check(pDX, IDC_REBASE_SPLIT_COMMIT, m_bSplitCommit);
}


BEGIN_MESSAGE_MAP(CRebaseDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_REBASE_SPLIT, &CRebaseDlg::OnBnClickedRebaseSplit)
	ON_BN_CLICKED(IDC_REBASE_CONTINUE,OnBnClickedContinue)
	ON_BN_CLICKED(IDC_REBASE_ABORT,  OnBnClickedAbort)
	ON_WM_SIZE()
	ON_WM_THEMECHANGED()
	ON_WM_SYSCOLORCHANGE()
	ON_CBN_SELCHANGE(IDC_REBASE_COMBOXEX_BRANCH,   &CRebaseDlg::OnCbnSelchangeBranch)
	ON_CBN_SELCHANGE(IDC_REBASE_COMBOXEX_UPSTREAM, &CRebaseDlg::OnCbnSelchangeUpstream)
	ON_MESSAGE(MSG_REBASE_UPDATE_UI, OnRebaseUpdateUI)
	ON_REGISTERED_MESSAGE(CGitStatusListCtrl::GITSLNM_NEEDSREFRESH, OnGitStatusListCtrlNeedsRefresh)
	ON_BN_CLICKED(IDC_BUTTON_REVERSE, OnBnClickedButtonReverse)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE, &CRebaseDlg::OnBnClickedButtonBrowse)
	ON_BN_CLICKED(IDC_REBASE_CHECK_FORCE, &CRebaseDlg::OnBnClickedRebaseCheckForce)
	ON_BN_CLICKED(IDC_REBASE_CHECK_PRESERVEMERGES, &CRebaseDlg::OnBnClickedRebaseCheckForce)
	ON_BN_CLICKED(IDC_CHECK_CHERRYPICKED_FROM, &CRebaseDlg::OnBnClickedCheckCherryPickedFrom)
	ON_BN_CLICKED(IDC_REBASE_POST_BUTTON, &CRebaseDlg::OnBnClickedRebasePostButton)
	ON_BN_CLICKED(IDC_BUTTON_UP, &CRebaseDlg::OnBnClickedButtonUp)
	ON_BN_CLICKED(IDC_BUTTON_DOWN, &CRebaseDlg::OnBnClickedButtonDown)
	ON_REGISTERED_MESSAGE(TaskBarButtonCreated, OnTaskbarBtnCreated)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_COMMIT_LIST, OnLvnItemchangedLoglist)
	ON_REGISTERED_MESSAGE(CGitLogListBase::m_RebaseActionMessage, OnRebaseActionMessage)
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_SPLITALLOPTIONS, &CRebaseDlg::OnBnClickedSplitAllOptions)
	ON_BN_CLICKED(IDC_REBASE_SPLIT_COMMIT, &CRebaseDlg::OnBnClickedRebaseSplitCommit)
	ON_BN_CLICKED(IDC_BUTTON_ONTO, &CRebaseDlg::OnBnClickedButtonOnto)
	ON_BN_CLICKED(IDHELP, OnHelp)
	ON_BN_CLICKED(IDC_BUTTON_ADD, &CRebaseDlg::OnBnClickedButtonAdd)
	ON_MESSAGE(MSG_COMMITS_REORDERED, OnCommitsReordered)
END_MESSAGE_MAP()

void CRebaseDlg::CleanUpRebaseActiveFolder()
{
	if (m_IsCherryPick)
		return;
	CString adminDir;
	if (GitAdminDir::GetAdminDirPath(g_Git.m_CurrentDir, adminDir))
	{
		CString dir(adminDir + L"tgitrebase.active");
		::DeleteFile(dir + L"\\head-name");
		::DeleteFile(dir + L"\\onto");
		::RemoveDirectory(dir);
	}
}

void CRebaseDlg::AddRebaseAnchor()
{
	AdjustControlSize(IDC_CHECK_CHERRYPICKED_FROM);
	AdjustControlSize(IDC_REBASE_SPLIT_COMMIT);
	AdjustControlSize(IDC_REBASE_CHECK_FORCE);
	AdjustControlSize(IDC_REBASE_CHECK_PRESERVEMERGES);

	AddAnchor(IDC_REBASE_TAB,TOP_LEFT,BOTTOM_RIGHT);
	AddAnchor(IDC_COMMIT_LIST,TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_REBASE_SPLIT,TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_STATUS_STATIC, BOTTOM_LEFT,BOTTOM_RIGHT);
	AddAnchor(IDC_REBASE_CONTINUE,BOTTOM_RIGHT);
	AddAnchor(IDC_REBASE_ABORT, BOTTOM_RIGHT);
	AddAnchor(IDC_REBASE_PROGRESS,BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SPLITALLOPTIONS, TOP_LEFT);
	AddAnchor(IDC_BUTTON_UP, TOP_LEFT);
	AddAnchor(IDC_BUTTON_DOWN, TOP_LEFT);
	AddAnchor(IDC_BUTTON_ADD, TOP_LEFT);
	AddAnchor(IDC_REBASE_COMBOXEX_UPSTREAM, TOP_CENTER, TOP_RIGHT);
	AddAnchor(IDC_REBASE_COMBOXEX_BRANCH, TOP_LEFT, TOP_CENTER);
	AddAnchor(IDC_BUTTON_REVERSE, TOP_CENTER);
	AddAnchor(IDC_BUTTON_BROWSE, TOP_RIGHT);
	AddAnchor(IDC_BUTTON_ONTO, TOP_RIGHT);
	AddAnchor(IDC_REBASE_STATIC_UPSTREAM, TOP_CENTER);
	AddAnchor(IDC_REBASE_STATIC_BRANCH,TOP_LEFT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	AddAnchor(IDC_REBASE_CHECK_FORCE, TOP_CENTER, TOP_RIGHT);
	AddAnchor(IDC_REBASE_CHECK_PRESERVEMERGES, TOP_LEFT, TOP_CENTER);
	AddAnchor(IDC_CHECK_CHERRYPICKED_FROM, TOP_RIGHT);
	AddAnchor(IDC_REBASE_SPLIT_COMMIT, BOTTOM_RIGHT);
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

	if (!m_LogMessageCtrl.Create(L"Scintilla", L"source", 0, rectDummy, &m_ctrlTabCtrl, 0, 0))
	{
		TRACE0("Failed to create log message control");
		return FALSE;
	}
	m_ProjectProperties.ReadProps();
	m_LogMessageCtrl.Init(m_ProjectProperties);
	m_LogMessageCtrl.SetFont(CAppUtils::GetLogFontName(), CAppUtils::GetLogFontSize());
	m_LogMessageCtrl.Call(SCI_SETREADONLY, TRUE);

	dwStyle = LBS_NOINTEGRALHEIGHT | WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL;

	if (!m_wndOutputRebase.Create(L"Scintilla", L"source", 0, rectDummy, &m_ctrlTabCtrl, 0, 0))
	{
		TRACE0("Failed to create output windows\n");
		return -1;      // fail to create
	}
	m_wndOutputRebase.Init(-1);
	m_wndOutputRebase.SetFont(CAppUtils::GetLogFontName(), CAppUtils::GetLogFontSize());
	m_wndOutputRebase.Call(SCI_SETREADONLY, TRUE);

	m_tooltips.AddTool(IDC_REBASE_CHECK_FORCE,IDS_REBASE_FORCE_TT);
	m_tooltips.AddTool(IDC_REBASE_ABORT, IDS_REBASE_ABORT_TT);
	m_tooltips.AddTool(IDC_REBASE_CHECK_PRESERVEMERGES, IDS_REBASE_PRESERVEMERGES_TT);

	{
		CString temp;
		temp.LoadString(IDS_PROC_REBASE_SELECTALL_PICK);
		m_SplitAllOptions.AddEntry(temp);
		temp.LoadString(IDS_PROC_REBASE_SELECTALL_SQUASH);
		m_SplitAllOptions.AddEntry(temp);
		temp.LoadString(IDS_PROC_REBASE_SELECTALL_EDIT);
		m_SplitAllOptions.AddEntry(temp);
		temp.LoadString(IDS_PROC_REBASE_UNSELECTED_SKIP);
		m_SplitAllOptions.AddEntry(temp);
		temp.LoadString(IDS_PROC_REBASE_UNSELECTED_SQUASH);
		m_SplitAllOptions.AddEntry(temp);
		temp.LoadString(IDS_PROC_REBASE_UNSELECTED_EDIT);
		m_SplitAllOptions.AddEntry(temp);
	}

	m_FileListCtrl.Init(GITSLC_COLEXT | GITSLC_COLSTATUS | GITSLC_COLADD | GITSLC_COLDEL, L"RebaseDlg", (GITSLC_POPALL ^ (GITSLC_POPCOMMIT | GITSLC_POPRESTORE | GITSLC_POPCHANGELISTS)), false, true, GITSLC_COLEXT | GITSLC_COLSTATUS | GITSLC_COLADD | GITSLC_COLDEL);

	m_ctrlTabCtrl.AddTab(&m_FileListCtrl, CString(MAKEINTRESOURCE(IDS_PROC_REVISIONFILES)));
	m_ctrlTabCtrl.AddTab(&m_LogMessageCtrl, CString(MAKEINTRESOURCE(IDS_PROC_COMMITMESSAGE)), 1);
	AddRebaseAnchor();

	CString sWindowTitle;
	GetWindowText(sWindowTitle);
	CAppUtils::SetWindowTitle(m_hWnd, g_Git.m_CurrentDir, sWindowTitle);

	EnableSaveRestore(L"RebaseDlg");

	DWORD yPos = CRegDWORD(L"Software\\TortoiseGit\\TortoiseProc\\ResizableState\\RebaseDlgSizer");
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
			m_wndSplitter.SetWindowPos(nullptr, 0, yPos, 0, 0, SWP_NOSIZE);
			DoSize(delta);
		}
	}

	if (this->m_RebaseStage == CHOOSE_BRANCH && !m_IsCherryPick)
		this->LoadBranchInfo();
	else
	{
		this->m_BranchCtrl.EnableWindow(FALSE);
		this->m_UpstreamCtrl.EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_REVERSE)->EnableWindow(FALSE);
	}

	m_CommitList.m_ColumnRegKey = L"Rebase";
	m_CommitList.m_IsIDReplaceAction = TRUE;
//	m_CommitList.m_IsOldFirst = TRUE;
	m_CommitList.m_IsRebaseReplaceGraph = TRUE;
	m_CommitList.m_bNoHightlightHead = TRUE;
	m_CommitList.m_bIsCherryPick = !!m_IsCherryPick;

	m_CommitList.InsertGitColumn();

	this->SetControlEnable();

	if(m_IsCherryPick)
	{
		this->m_BranchCtrl.SetCurSel(-1);
		this->m_BranchCtrl.EnableWindow(FALSE);
		GetDlgItem(IDC_REBASE_CHECK_FORCE)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_REBASE_CHECK_PRESERVEMERGES)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_BUTTON_BROWSE)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_REVERSE)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_ONTO)->EnableWindow(FALSE);
		this->m_UpstreamCtrl.AddString(L"HEAD");
		this->m_UpstreamCtrl.EnableWindow(FALSE);
		CAppUtils::SetWindowTitle(m_hWnd, g_Git.m_CurrentDir, CString(MAKEINTRESOURCE(IDS_PROGS_TITLE_CHERRYPICK)));
		// fill shown list
		for (DWORD i = 0; i < m_CommitList.m_logEntries.size(); ++i)
			m_CommitList.m_arShownList.SafeAdd(&m_CommitList.m_logEntries.GetGitRevAt(i));
		m_CommitList.SetItemCountEx(static_cast<int>(m_CommitList.m_arShownList.size()));
	}
	else
	{
		static_cast<CButton*>(GetDlgItem(IDC_BUTTON_ONTO))->SetCheck(m_Onto.IsEmpty() ? BST_UNCHECKED : BST_CHECKED);
		GetDlgItem(IDC_CHECK_CHERRYPICKED_FROM)->ShowWindow(SW_HIDE);
		int iconWidth = GetSystemMetrics(SM_CXSMICON);
		int iconHeight = GetSystemMetrics(SM_CYSMICON);
		static_cast<CButton*>(GetDlgItem(IDC_BUTTON_REVERSE))->SetIcon(CCommonAppUtils::LoadIconEx(IDI_SWITCHLEFTRIGHT, iconWidth, iconHeight));
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
		this->m_CurrentRebaseIndex = static_cast<int>(m_CommitList.m_logEntries.size());

	if (GetDlgItem(IDC_REBASE_CONTINUE)->IsWindowEnabled() && m_bRebaseAutoStart)
		this->PostMessage(WM_COMMAND, MAKELONG(IDC_REBASE_CONTINUE, BN_CLICKED), reinterpret_cast<LPARAM>(GetDlgItem(IDC_REBASE_CONTINUE)->GetSafeHwnd()));

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

void CRebaseDlg::SetAllRebaseAction(int action)
{
	for (size_t i = 0; i < this->m_CommitList.m_logEntries.size(); ++i)
	{
		if (action == CGitLogListBase::LOGACTIONS_REBASE_SQUASH && (i == this->m_CommitList.m_logEntries.size() - 1 || (!m_IsCherryPick && m_CommitList.m_logEntries.GetGitRevAt(i).ParentsCount() != 1)))
			continue;
		m_CommitList.m_logEntries.GetGitRevAt(i).GetRebaseAction() = action;
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
			auto pHdr = reinterpret_cast<SPC_NMHDR*>(lParam);
			DoSize(pHdr->delta);
		}
		break;
	}

	return __super::DefWindowProc(message, wParam, lParam);
}

void CRebaseDlg::DoSize(int delta)
{
	this->RemoveAllAnchors();

	auto hdwp = BeginDeferWindowPos(9);
	hdwp = CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_COMMIT_LIST), 0, 0, 0, delta);
	hdwp = CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_REBASE_TAB), 0, delta, 0, 0);
	hdwp = CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_SPLITALLOPTIONS), 0, delta, 0, delta);
	hdwp = CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_BUTTON_UP), 0, delta, 0, delta);
	hdwp = CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_BUTTON_DOWN), 0, delta, 0, delta);
	hdwp = CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_BUTTON_ADD), 0, delta, 0, delta);
	hdwp = CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_REBASE_CHECK_FORCE), 0, delta, 0, delta);
	hdwp = CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_REBASE_CHECK_PRESERVEMERGES), 0, delta, 0, delta);
	hdwp = CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_CHECK_CHERRYPICKED_FROM), 0, delta, 0, delta);
	EndDeferWindowPos(hdwp);

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
	m_SplitAllOptions.Invalidate();
	GetDlgItem(IDC_REBASE_CHECK_FORCE)->Invalidate();
	GetDlgItem(IDC_REBASE_CHECK_PRESERVEMERGES)->Invalidate();
	GetDlgItem(IDC_CHECK_CHERRYPICKED_FROM)->Invalidate();
	GetDlgItem(IDC_BUTTON_UP)->Invalidate();
	GetDlgItem(IDC_BUTTON_DOWN)->Invalidate();
	GetDlgItem(IDC_BUTTON_ADD)->Invalidate();
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
		CRegDWORD regPos = CRegDWORD(L"Software\\TortoiseGit\\TortoiseProc\\ResizableState\\RebaseDlgSizer");
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
	int current = -1;
	g_Git.GetBranchList(list,&current,CGit::BRANCH_ALL);
	m_BranchCtrl.SetList(list);
	if (current >= 0)
		m_BranchCtrl.SetCurSel(current);
	else
		m_BranchCtrl.AddString(g_Git.GetCurrentBranch(true));
	list.clear();
	g_Git.GetBranchList(list, nullptr, CGit::BRANCH_ALL_F);
	g_Git.GetTagList(list);
	m_UpstreamCtrl.SetList(list);

	AddBranchToolTips(m_BranchCtrl);

	if(!m_Upstream.IsEmpty())
		m_UpstreamCtrl.AddString(m_Upstream);
	else
	{
		//Select pull-remote from current branch
		CString pullRemote, pullBranch;
		g_Git.GetRemoteTrackedBranchForHEAD(pullRemote, pullBranch);

		CString defaultUpstream;
		defaultUpstream.Format(L"remotes/%s/%s", static_cast<LPCTSTR>(pullRemote), static_cast<LPCTSTR>(pullBranch));
		int found = m_UpstreamCtrl.FindStringExact(0, defaultUpstream);
		if(found >= 0)
			m_UpstreamCtrl.SetCurSel(found);
		else
			m_UpstreamCtrl.SetCurSel(-1);
	}
	AddBranchToolTips(m_UpstreamCtrl);
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

	if (m_BranchCtrl.GetString().IsEmpty())
	{
		m_CommitList.ShowText(CString(MAKEINTRESOURCE(IDS_SELECTBRANCH)));
		this->GetDlgItem(IDC_REBASE_CONTINUE)->EnableWindow(false);
		return;
	}

	if (g_Git.GetHash(hash, m_BranchCtrl.GetString()))
	{
		m_CommitList.ShowText(g_Git.GetGitLastErr(L"Could not get hash of \"" + m_BranchCtrl.GetString() + L"\"."));
		this->GetDlgItem(IDC_REBASE_CONTINUE)->EnableWindow(false);
		return;
	}

	if (m_UpstreamCtrl.GetString().IsEmpty())
	{
		m_CommitList.ShowText(CString(MAKEINTRESOURCE(IDS_SELECTUPSTREAM)));
		this->GetDlgItem(IDC_REBASE_CONTINUE)->EnableWindow(false);
		return;
	}

	if (g_Git.GetHash(upstream, m_UpstreamCtrl.GetString()))
	{
		m_CommitList.ShowText(g_Git.GetGitLastErr(L"Could not get hash of \"" + m_UpstreamCtrl.GetString() + L"\"."));
		this->GetDlgItem(IDC_REBASE_CONTINUE)->EnableWindow(false);
		return;
	}

	if (hash == upstream)
	{
		m_CommitList.Clear();
		CString text;
		text.FormatMessage(IDS_REBASE_EQUAL_FMT, static_cast<LPCTSTR>(m_BranchCtrl.GetString()), static_cast<LPCTSTR>(this->m_UpstreamCtrl.GetString()));

		m_CommitList.ShowText(text);
		this->GetDlgItem(IDC_REBASE_CONTINUE)->EnableWindow(false);
		if (m_bRebaseAutoStart)
			PostMessage(WM_COMMAND, MAKELONG(IDC_REBASE_ABORT, BN_CLICKED), reinterpret_cast<LPARAM>(GetDlgItem(IDC_REBASE_ABORT)->GetSafeHwnd()));
		return;
	}

	if (g_Git.IsFastForward(m_BranchCtrl.GetString(), m_UpstreamCtrl.GetString(), &base) && m_Onto.IsEmpty())
	{
		this->m_IsFastForward=TRUE;

		m_CommitList.Clear();
		CString text;
		text.FormatMessage(IDS_REBASE_FASTFORWARD_FMT, static_cast<LPCTSTR>(m_BranchCtrl.GetString()), static_cast<LPCTSTR>(this->m_UpstreamCtrl.GetString()),
						static_cast<LPCTSTR>(m_BranchCtrl.GetString()), static_cast<LPCTSTR>(this->m_UpstreamCtrl.GetString()));

		m_CommitList.ShowText(text);
		this->GetDlgItem(IDC_REBASE_CONTINUE)->EnableWindow(true);
		SetContinueButtonText();

		return ;
	}

	if (!m_bForce && m_Onto.IsEmpty())
	{
		if (base == upstream)
		{
			m_CommitList.Clear();
			CString text;
			text.Format(IDS_REBASE_UPTODATE_FMT, static_cast<LPCTSTR>(m_BranchCtrl.GetString()));
			m_CommitList.ShowText(text);
			this->GetDlgItem(IDC_REBASE_CONTINUE)->EnableWindow(FALSE);
			SetContinueButtonText();
			if (m_bRebaseAutoStart)
				PostMessage(WM_COMMAND, MAKELONG(IDC_REBASE_ABORT, BN_CLICKED), reinterpret_cast<LPARAM>(GetDlgItem(IDC_REBASE_ABORT)->GetSafeHwnd()));
			return;
		}
	}

	m_CommitList.Clear();
	CString refFrom = g_Git.FixBranchName(m_UpstreamCtrl.GetString());
	CString refTo   = g_Git.FixBranchName(m_BranchCtrl.GetString());
	CString range;
	range.Format(L"%s..%s", static_cast<LPCTSTR>(refFrom), static_cast<LPCTSTR>(refTo));
	this->m_CommitList.FillGitLog(nullptr, &range, (m_bPreserveMerges ? 0 : CGit::LOG_INFO_NO_MERGE) | CGit::LOG_ORDER_TOPOORDER);

	if( m_CommitList.GetItemCount() == 0 )
		m_CommitList.ShowText(CString(MAKEINTRESOURCE(IDS_PROC_NOTHINGTOREBASE)));

	m_rewrittenCommitsMap.clear();
	if (m_bPreserveMerges)
	{
		CGitHash head;
		if (g_Git.GetHash(head, L"HEAD"))
		{
			AddLogString(CString(MAKEINTRESOURCE(IDS_PROC_NOHEAD)));
			return;
		}
		CGitHash upstreamHash;
		if (g_Git.GetHash(upstreamHash, m_Onto.IsEmpty() ? m_UpstreamCtrl.GetString() : m_Onto))
		{
			MessageBox(g_Git.GetGitLastErr(L"Could not get hash of \"" + (m_Onto.IsEmpty() ? m_UpstreamCtrl.GetString() : m_Onto) + L"\"."), L"TortoiseGit", MB_ICONERROR);
			return;
		}
		CString mergecmd;
		mergecmd.Format(L"git merge-base --all %s %s", static_cast<LPCTSTR>(head.ToString()), static_cast<LPCTSTR>(upstreamHash.ToString()));
		g_Git.Run(mergecmd, [&](const CStringA& line)
		{
			CGitHash hash = CGitHash::FromHexStr(line);
			if (hash.IsEmpty())
				return;
			m_rewrittenCommitsMap[hash] = upstreamHash;
		});

		std::vector<size_t> toDrop;
		for (size_t i = m_CommitList.m_arShownList.size(); i-- > 0;)
		{
			bool preserve = false;
			GitRevLoglist* pRev = m_CommitList.m_arShownList.SafeGetAt(i);
			for (const auto& parent : pRev->m_ParentHash)
			{
				const auto rewrittenParent = m_rewrittenCommitsMap.find(parent);
				if (rewrittenParent != m_rewrittenCommitsMap.cend())
				{
					preserve = true;
					break;
				}
			}
			if (preserve)
				m_rewrittenCommitsMap[pRev->m_CommitHash] = CGitHash();
			else
				toDrop.push_back(i);
		}

		// Drop already included commits
		std::vector<CGitHash> nonCherryPicked;
		CString cherryCmd;
		cherryCmd.Format(L"git rev-list \"%s...%s\" --left-right --cherry-pick", static_cast<LPCTSTR>(refFrom), static_cast<LPCTSTR>(refTo));
		g_Git.Run(cherryCmd, [&](const CStringA& line)
		{
			if (line.GetLength() < 2)
				return;
			if (line[0] != '>')
				return;
			CString hash = CUnicodeUtils::GetUnicode(line.Mid(1));
			hash.Trim();
			nonCherryPicked.emplace_back(CGitHash::FromHexStrTry(hash));
		});
		for (size_t i = m_CommitList.m_arShownList.size(); i-- > 0;)
		{
			GitRevLoglist* pRev = m_CommitList.m_arShownList.SafeGetAt(i);
			pRev->GetRebaseAction() = CGitLogListBase::LOGACTIONS_REBASE_PICK;
			if (m_rewrittenCommitsMap.find(pRev->m_CommitHash) != m_rewrittenCommitsMap.cend() && std::find(nonCherryPicked.cbegin(), nonCherryPicked.cend(), pRev->m_CommitHash) == nonCherryPicked.cend())
			{
				m_droppedCommitsMap[pRev->m_CommitHash].clear();
				m_droppedCommitsMap[pRev->m_CommitHash].push_back(pRev->m_ParentHash[0]);
				toDrop.push_back(i);
				m_rewrittenCommitsMap.erase(pRev->m_CommitHash);
			}
		}
		std::sort(toDrop.begin(), toDrop.end());
		toDrop.erase(unique(toDrop.begin(), toDrop.end()), toDrop.end());
		for (auto it = toDrop.crbegin(); it != toDrop.crend(); ++it)
		{
			m_CommitList.m_arShownList.SafeRemoveAt(*it);
			m_CommitList.m_logEntries.erase(m_CommitList.m_logEntries.begin() + *it);
		}
		m_CommitList.SetItemCountEx(static_cast<int>(m_CommitList.m_logEntries.size()));
	}

#if 0
	if(m_CommitList.m_logEntries[m_CommitList.m_logEntries.size()-1].m_ParentHash.size() >=0 )
	{
		if(upstream ==  m_CommitList.m_logEntries[m_CommitList.m_logEntries.size()-1].m_ParentHash[0])
		{
			m_CommitList.Clear();
			m_CommitList.ShowText(L"Nothing Rebase");
		}
	}
#endif

	m_tooltips.Pop();
	AddBranchToolTips(m_BranchCtrl);
	AddBranchToolTips(m_UpstreamCtrl);

	bool bHasSKip = false;
	if (!m_bPreserveMerges)
	{
		// Default all actions to 'pick'
		std::unordered_map<CGitHash, size_t> revIxMap;
		for (size_t i = 0; i < m_CommitList.m_logEntries.size(); ++i)
		{
			GitRevLoglist& rev = m_CommitList.m_logEntries.GetGitRevAt(i);
			rev.GetRebaseAction() = CGitLogListBase::LOGACTIONS_REBASE_PICK;
			revIxMap[rev.m_CommitHash] = i;
		}

		// Default to skip when already in upstream
		if (!m_Onto.IsEmpty())
			refFrom = g_Git.FixBranchName(m_Onto);
		CString cherryCmd;
		cherryCmd.Format(L"git.exe cherry \"%s\" \"%s\"", static_cast<LPCTSTR>(refFrom), static_cast<LPCTSTR>(refTo));
		g_Git.Run(cherryCmd, [&](const CStringA& line)
		{
			if (line.GetLength() < 2)
				return;
			if (line[0] != '-')
				return; // Don't skip (only skip commits starting with a '-')
			CString hash = CUnicodeUtils::GetUnicode(line.Mid(1));
			hash.Trim();
			auto itIx = revIxMap.find(CGitHash::FromHexStrTry(hash));
			if (itIx == revIxMap.end())
				return; // Not found?? Should not occur...

			// Found. Skip it.
			m_CommitList.m_logEntries.GetGitRevAt(itIx->second).GetRebaseAction() = CGitLogListBase::LOGACTIONS_REBASE_SKIP;
			bHasSKip = true;
		});
	}
	m_CommitList.Invalidate();
	if (bHasSKip)
	{
		m_CtrlStatusText.SetWindowText(CString(MAKEINTRESOURCE(IDS_REBASE_AUTOSKIPPED)));
		m_bStatusWarning = true;
	}
	else
	{
		m_CtrlStatusText.SetWindowText(m_sStatusText);
		m_bStatusWarning = false;
	}
	m_CtrlStatusText.Invalidate();

	if(m_CommitList.m_IsOldFirst)
		this->m_CurrentRebaseIndex = -1;
	else
		this->m_CurrentRebaseIndex = static_cast<int>(m_CommitList.m_logEntries.size());

	this->GetDlgItem(IDC_REBASE_CONTINUE)->EnableWindow(m_bPreserveMerges || m_CommitList.GetItemCount());
	SetContinueButtonText();
}

void CRebaseDlg::AddBranchToolTips(CHistoryCombo& pBranch)
{
	pBranch.DisableTooltip();

	CString text = pBranch.GetString();

	if (text.IsEmpty())
		return;

	GitRev rev;
	if (rev.GetCommit(text))
	{
		MessageBox(L"Failed to get commit.\n" + rev.GetLastErr(), L"TortoiseGit", MB_ICONERROR);
		return;
	}

	CString tooltip;
	tooltip.Format(L"%s: %s\n%s: %s <%s>\n%s: %s\n%s:\n%s\n%s",
					static_cast<LPCTSTR>(CString(MAKEINTRESOURCE(IDS_LOG_REVISION))),
					static_cast<LPCTSTR>(rev.m_CommitHash.ToString()),
					static_cast<LPCTSTR>(CString(MAKEINTRESOURCE(IDS_LOG_AUTHOR))),
					static_cast<LPCTSTR>(rev.GetAuthorName()),
					static_cast<LPCTSTR>(rev.GetAuthorEmail()),
					static_cast<LPCTSTR>(CString(MAKEINTRESOURCE(IDS_LOG_DATE))),
					static_cast<LPCTSTR>(CLoglistUtils::FormatDateAndTime(rev.GetAuthorDate(), DATE_LONGDATE)),
					static_cast<LPCTSTR>(CString(MAKEINTRESOURCE(IDS_LOG_MESSAGE))),
					static_cast<LPCTSTR>(rev.GetSubject()),
					static_cast<LPCTSTR>(rev.GetBody()));

	if (tooltip.GetLength() > 8000)
	{
		tooltip.Truncate(8000);
		tooltip += L"...";
	}

	m_tooltips.AddTool(pBranch.GetComboBoxCtrl(), tooltip);
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
				m_CommitList.ShiftSelectedRebaseAction();
				return TRUE;
			}
			break;
		case 'P':
			if (LogListHasFocus(pMsg->hwnd) && LogListHasMenuItem(CGitLogListBase::ID_REBASE_PICK))
			{
				m_CommitList.SetSelectedRebaseAction(CGitLogListBase::LOGACTIONS_REBASE_PICK);
				return TRUE;
			}
			break;
		case 'S':
			if (LogListHasFocus(pMsg->hwnd) && LogListHasMenuItem(CGitLogListBase::ID_REBASE_SKIP))
			{
				m_CommitList.SetSelectedRebaseAction(CGitLogListBase::LOGACTIONS_REBASE_SKIP);
				return TRUE;
			}
			break;
		case 'Q':
			if (LogListHasFocus(pMsg->hwnd) && LogListHasMenuItem(CGitLogListBase::ID_REBASE_SQUASH))
			{
				m_CommitList.SetSelectedRebaseAction(CGitLogListBase::LOGACTIONS_REBASE_SQUASH);
				return TRUE;
			}
			break;
		case 'E':
			if (LogListHasFocus(pMsg->hwnd) && LogListHasMenuItem(CGitLogListBase::ID_REBASE_EDIT))
			{
				m_CommitList.SetSelectedRebaseAction(CGitLogListBase::LOGACTIONS_REBASE_EDIT);
				return TRUE;
			}
			break;
		case 'U':
			if (LogListHasFocus(pMsg->hwnd) && GetDlgItem(IDC_BUTTON_UP)->IsWindowEnabled() == TRUE)
			{
				OnBnClickedButtonUp();
				return TRUE;
			}
			break;
		case 'D':
			if (LogListHasFocus(pMsg->hwnd) && GetDlgItem(IDC_BUTTON_DOWN)->IsWindowEnabled() == TRUE)
			{
				OnBnClickedButtonDown();
				return TRUE;
			}
			break;
		case 'A':
			if(LogListHasFocus(pMsg->hwnd) && GetAsyncKeyState(VK_CONTROL) & 0x8000)
			{
				// select all entries
				for (int i = 0; i < m_CommitList.GetItemCount(); ++i)
					m_CommitList.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
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
				if (_wcsnicmp(buff, MSFTEDIT_CLASS, 128) == 0 ||	//Unicode and MFC 2012 and later
					_wcsnicmp(buff, RICHEDIT_CLASS, 128) == 0 ||	//ANSI or MFC 2010
					_wcsnicmp(buff, L"Scintilla", 128) == 0 ||
					_wcsnicmp(buff, L"SysListView32", 128) == 0 ||
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
	return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}

bool CRebaseDlg::LogListHasFocus(HWND hwnd)
{
	TCHAR buff[128] = { 0 };
	::GetClassName(hwnd, buff, 128);

	if (_wcsnicmp(buff, L"SysListView32", 128) == 0)
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
		if ((!m_IsCherryPick && g_Git.GetConfigValueBool(L"rebase.autostash")) || CMessageBox::Show(GetSafeHwnd(), IDS_ERROR_NOCLEAN_STASH, IDS_APPNAME, 1, IDI_QUESTION, IDS_STASHBUTTON, IDS_ABORTBUTTON) == 1)
		{
			CString out;
			CString cmd = L"git.exe stash";
			this->AddLogString(cmd);
			if (g_Git.Run(cmd, &out, CP_UTF8))
			{
				MessageBox(out, L"TortoiseGit", MB_OK | MB_ICONERROR);
				return -1;
			}
			m_bStashed = true;
		}
		else
			return -1;
	}
	//Todo Check $REBASE_ROOT
	//Todo Check $DOTEST

	if (!CAppUtils::CheckUserData(GetSafeHwnd()))
		return -1;

	if (!m_IsCherryPick)
	{
		CString error;
		DWORD exitcode = 0xFFFFFFFF;
		CHooks::Instance().SetProjectProperties(g_Git.m_CurrentDir, m_ProjectProperties);
		if (CHooks::Instance().PreRebase(GetSafeHwnd(), g_Git.m_CurrentDir, m_UpstreamCtrl.GetString(), m_BranchCtrl.GetString(), exitcode, error))
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
					return -1;
			}
		}
	}

	return 0;
}

void CRebaseDlg::CheckRestoreStash()
{
	bool autoStash = !m_IsCherryPick && g_Git.GetConfigValueBool(L"rebase.autostash");
	if (m_bStashed && (autoStash || CMessageBox::Show(GetSafeHwnd(), IDS_DCOMMIT_STASH_POP, IDS_APPNAME, MB_YESNO | MB_ICONQUESTION) == IDYES))
		CAppUtils::StashPop(GetSafeHwnd(), autoStash ? 0 : 1);
	m_bStashed = false;
}

int CRebaseDlg::WriteReflog(CGitHash hash, const char* message)
{
	CAutoRepository repo(g_Git.GetGitRepository());
	CAutoReflog reflog;
	if (git_reflog_read(reflog.GetPointer(), repo, "HEAD") < 0)
	{
		MessageBox(g_Git.GetGitLastErr(L"Could not read HEAD reflog"), L"TortoiseGit", MB_ICONERROR);
		return -1;
	}
	CAutoSignature signature;
	if (git_signature_default(signature.GetPointer(), repo) < 0)
	{
		MessageBox(g_Git.GetGitLastErr(L"Could not get signature"), L"TortoiseGit", MB_ICONERROR);
		return -1;
	}
	if (git_reflog_append(reflog, hash, signature, message) < 0)
	{
		MessageBox(g_Git.GetGitLastErr(L"Could not append HEAD reflog"), L"TortoiseGit", MB_ICONERROR);
		return -1;
	}
	if (git_reflog_write(reflog) < 0)
	{
		MessageBox(g_Git.GetGitLastErr(L"Could not write HEAD reflog"), L"TortoiseGit", MB_ICONERROR);
		return -1;
	}

	return 0;
}

int CRebaseDlg::StartRebase()
{
	CString cmd,out;
	m_OrigHEADBranch = g_Git.GetCurrentBranch(true);

	m_OrigHEADHash.Empty();
	if (g_Git.GetHash(m_OrigHEADHash, L"HEAD"))
	{
		AddLogString(CString(MAKEINTRESOURCE(IDS_PROC_NOHEAD)));
		return -1;
	}
	//Todo
	//git symbolic-ref HEAD > "$DOTEST"/head-name 2> /dev/null ||
	//		echo "detached HEAD" > "$DOTEST"/head-name

	cmd.Format(L"git.exe update-ref ORIG_HEAD %s", static_cast<LPCTSTR>(m_OrigHEADHash.ToString()));
	if(g_Git.Run(cmd,&out,CP_UTF8))
	{
		AddLogString(L"update ORIG_HEAD Fail");
		return -1;
	}

	m_OrigUpstreamHash.Empty();
	if (g_Git.GetHash(m_OrigUpstreamHash, (m_IsCherryPick || m_Onto.IsEmpty()) ? m_UpstreamCtrl.GetString() : m_Onto))
	{
		MessageBox(g_Git.GetGitLastErr(L"Could not get hash of \"" + ((m_IsCherryPick || m_Onto.IsEmpty()) ? m_UpstreamCtrl.GetString() : m_Onto) + L"\"."), L"TortoiseGit", MB_ICONERROR);
		return -1;
	}

	if( !this->m_IsCherryPick )
	{
		if (g_Git.m_IsUseLibGit2)
			WriteReflog(m_OrigHEADHash, "rebase: start (" + CUnicodeUtils::GetUTF8(m_OrigHEADBranch) + " on " + CUnicodeUtils::GetUTF8(m_OrigUpstreamHash.ToString()) + ")");
		cmd.Format(L"git.exe checkout -f %s --", static_cast<LPCTSTR>(m_OrigUpstreamHash.ToString()));
		this->AddLogString(cmd);
		if (RunGitCmdRetryOrAbort(cmd))
			return -1;
	}

	CString log;
	if( !this->m_IsCherryPick )
	{
		if (g_Git.GetHash(m_OrigBranchHash, m_BranchCtrl.GetString()))
		{
			MessageBox(g_Git.GetGitLastErr(L"Could not get hash of \"" + m_BranchCtrl.GetString() + L"\"."), L"TortoiseGit", MB_ICONERROR);
			return -1;
		}
		log.Format(L"%s\r\n", static_cast<LPCTSTR>(CString(MAKEINTRESOURCE(IDS_PROC_REBASE_STARTREBASE))));
	}
	else
		log.Format(L"%s\r\n", static_cast<LPCTSTR>(CString(MAKEINTRESOURCE(IDS_PROC_REBASE_STARTCHERRYPICK))));

	this->AddLogString(log);
	return 0;
}
int CRebaseDlg::VerifyNoConflict()
{
	int hasConflicts = g_Git.HasWorkingTreeConflicts();
	if (hasConflicts < 0)
	{
		AddLogString(g_Git.GetGitLastErr(L"Checking for conflicts failed.", CGit::GIT_CMD_CHECKCONFLICTS));
		return -1;
	}
	if (hasConflicts)
	{
		CMessageBox::Show(GetSafeHwnd(), IDS_PROGRS_CONFLICTSOCCURRED, IDS_APPNAME, MB_OK | MB_ICONEXCLAMATION);
		auto locker(m_FileListCtrl.AcquireReadLock());
		auto pos = m_FileListCtrl.GetFirstSelectedItemPosition();
		while (pos)
			m_FileListCtrl.SetItemState(m_FileListCtrl.GetNextSelectedItem(pos), 0, LVIS_SELECTED);
		int nListItems = m_FileListCtrl.GetItemCount();
		for (int i = 0; i < nListItems; ++i)
		{
			auto entry = m_FileListCtrl.GetListEntry(i);
			if (entry->m_Action & CTGitPath::LOGACTIONS_UNMERGED)
			{
				m_FileListCtrl.EnsureVisible(i, FALSE);
				m_FileListCtrl.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
				m_FileListCtrl.SetFocus();
				return -1;
			}
		}
		return -1;
	}
	CleanUpRebaseActiveFolder();
	return 0;
}

int CRebaseDlg::FinishRebase()
{
	if (m_bFinishedRebase)
		return 0;

	m_bFinishedRebase = true;
	if(this->m_IsCherryPick) //cherry pick mode no "branch", working at upstream branch
	{
		m_sStatusText.LoadString(IDS_DONE);
		m_CtrlStatusText.SetWindowText(m_sStatusText);
		m_bStatusWarning = false;
		m_CtrlStatusText.Invalidate();
		return 0;
	}

	RewriteNotes();

	CGitHash head;
	if (g_Git.GetHash(head, L"HEAD"))
	{
		MessageBox(g_Git.GetGitLastErr(L"Could not get HEAD hash."), L"TortoiseGit", MB_ICONERROR);
		return -1;
	}

	m_ctrlTabCtrl.SetActiveTab(REBASE_TAB_LOG);

	if (g_Git.IsLocalBranch(m_BranchCtrl.GetString()))
	{
		CString cmd;
		cmd.Format(L"git.exe checkout -f -B %s %s --", static_cast<LPCTSTR>(m_BranchCtrl.GetString()), static_cast<LPCTSTR>(head.ToString()));
		AddLogString(cmd);
		if (RunGitCmdRetryOrAbort(cmd))
			return -1;
	}

	CString cmd;
	cmd.Format(L"git.exe reset --hard %s --", static_cast<LPCTSTR>(head.ToString()));
	AddLogString(cmd);
	if (RunGitCmdRetryOrAbort(cmd))
		return -1;

	if (g_Git.m_IsUseLibGit2)
		WriteReflog(head, "rebase: finished");

	while (m_ctrlTabCtrl.GetTabsNum() > 1)
		m_ctrlTabCtrl.RemoveTab(0);
	m_CtrlStatusText.SetWindowText(CString(MAKEINTRESOURCE(IDS_PROC_REBASEFINISHED)));
	m_sStatusText.LoadString(IDS_PROC_REBASEFINISHED);
	m_bStatusWarning = false;
	m_CtrlStatusText.Invalidate();

	m_bRebaseAutoEnd = m_bRebaseAutoStart;

	return 0;
}

void CRebaseDlg::RewriteNotes()
{
	CString rewrites;
	for (const auto& entry : m_rewrittenCommitsMap)
	{
		if (entry.second.IsEmpty())
			continue;
		rewrites += entry.first.ToString();
		rewrites += L' ';
		rewrites += entry.second.ToString();
		rewrites += L'\n';
	}
	if (rewrites.IsEmpty())
		return;
	CString tmpfile = GetTempFile();
	tmpfile.Replace(L'\\', L'/');
	if (!CStringUtils::WriteStringToTextFile(tmpfile, rewrites))
		return;
	SCOPE_EXIT{ ::DeleteFile(tmpfile); };
	CString pipefile = GetTempFile();
	pipefile.Replace(L'\\', L'/');
	CString pipecmd;
	pipecmd.Format(L"git notes copy --for-rewrite=rebase < %s", static_cast<LPCTSTR>(tmpfile));
	if (!CStringUtils::WriteStringToTextFile(pipefile, pipecmd))
		return;
	SCOPE_EXIT{ ::DeleteFile(pipefile); };
	CString out;
	g_Git.Run(L"bash.exe " + pipefile, &out, CP_UTF8);
}

void CRebaseDlg::OnBnClickedContinue()
{
	if( m_RebaseStage == REBASE_DONE)
	{
		OnOK();
		CleanUpRebaseActiveFolder();
		CheckRestoreStash();
		SaveSplitterPos();
		return;
	}

	if (m_RebaseStage == CHOOSE_BRANCH || m_RebaseStage == CHOOSE_COMMIT_PICK_MODE)
	{
		if (CAppUtils::IsTGitRebaseActive(GetSafeHwnd()))
			return;
		if (CheckRebaseCondition())
			return;
	}

	m_bAbort = FALSE;
	if( this->m_IsFastForward )
	{
		GetDlgItem(IDC_REBASE_CONTINUE)->EnableWindow(FALSE);
		CString cmd,out;
		if (g_Git.GetHash(m_OrigBranchHash, m_BranchCtrl.GetString()))
		{
			MessageBox(g_Git.GetGitLastErr(L"Could not get hash of \"" + m_BranchCtrl.GetString() + L"\"."), L"TortoiseGit", MB_ICONERROR);
			GetDlgItem(IDC_REBASE_CONTINUE)->EnableWindow(TRUE);
			return;
		}
		if (g_Git.GetHash(m_OrigUpstreamHash, m_UpstreamCtrl.GetString()))
		{
			MessageBox(g_Git.GetGitLastErr(L"Could not get hash of \"" + m_UpstreamCtrl.GetString() + L"\"."), L"TortoiseGit", MB_ICONERROR);
			GetDlgItem(IDC_REBASE_CONTINUE)->EnableWindow(TRUE);
			return;
		}

		if(!g_Git.IsFastForward(this->m_BranchCtrl.GetString(),this->m_UpstreamCtrl.GetString()))
		{
			this->m_ctrlTabCtrl.SetActiveTab(REBASE_TAB_LOG);
			AddLogString(L"No fast forward possible.\r\nMaybe repository changed");
			GetDlgItem(IDC_REBASE_CONTINUE)->EnableWindow(TRUE);
			return;
		}

		if (g_Git.IsLocalBranch(m_BranchCtrl.GetString()))
		{
			cmd.Format(L"git.exe checkout --no-track -f -B %s %s --", static_cast<LPCTSTR>(m_BranchCtrl.GetString()), static_cast<LPCTSTR>(m_UpstreamCtrl.GetString()));
			AddLogString(cmd);
			if (RunGitCmdRetryOrAbort(cmd))
			{
				GetDlgItem(IDC_REBASE_CONTINUE)->EnableWindow(TRUE);
				return;
			}
			AddLogString(out);
			out.Empty();
		}
		cmd.Format(L"git.exe reset --hard %s --", static_cast<LPCTSTR>(g_Git.FixBranchName(this->m_UpstreamCtrl.GetString())));
		CString log;
		log.Format(IDS_PROC_REBASE_FFTO, static_cast<LPCTSTR>(m_UpstreamCtrl.GetString()));
		this->AddLogString(log);

		AddLogString(cmd);
		this->m_ctrlTabCtrl.SetActiveTab(REBASE_TAB_LOG);
		if (RunGitCmdRetryOrAbort(cmd))
		{
			GetDlgItem(IDC_REBASE_CONTINUE)->EnableWindow(TRUE);
			return;
		}
		AddLogString(out);
		AddLogString(CString(MAKEINTRESOURCE(IDS_DONE)));
		m_RebaseStage = REBASE_DONE;
		UpdateCurrentStatus();

		if (m_bRebaseAutoStart)
			this->PostMessage(WM_COMMAND, MAKELONG(IDC_REBASE_CONTINUE, BN_CLICKED), reinterpret_cast<LPARAM>(GetDlgItem(IDC_REBASE_CONTINUE)->GetSafeHwnd()));

		return;
	}

	if( m_RebaseStage == CHOOSE_BRANCH|| m_RebaseStage == CHOOSE_COMMIT_PICK_MODE )
	{
		m_RebaseStage = REBASE_START;
		m_FileListCtrl.Clear();
		m_FileListCtrl.SetHasCheckboxes(false);
		m_FileListCtrl.m_CurrentVersion.Empty();
		m_ctrlTabCtrl.SetTabLabel(REBASE_TAB_CONFLICT, CString(MAKEINTRESOURCE(IDS_PROC_CONFLICTFILES)));
		m_ctrlTabCtrl.AddTab(&m_wndOutputRebase, CString(MAKEINTRESOURCE(IDS_LOG)), 2);
		m_ctrlTabCtrl.SetActiveTab(REBASE_TAB_LOG);
	}

	if( m_RebaseStage == REBASE_FINISH )
	{
		if(FinishRebase())
			return ;

		SaveSplitterPos();
		OnOK();
	}

	if( m_RebaseStage == REBASE_SQUASH_CONFLICT)
	{
		if(VerifyNoConflict())
			return;
		if (CAppUtils::MessageContainsConflictHints(GetSafeHwnd(), m_LogMessageCtrl.GetText()))
			return;
		GitRevLoglist* curRev = m_CommitList.m_arShownList.SafeGetAt(m_CurrentRebaseIndex);
		if(this->CheckNextCommitIsSquash())
		{//next commit is not squash;
			m_RebaseStage = REBASE_SQUASH_EDIT;
			this->OnRebaseUpdateUI(0,0);
			this->UpdateCurrentStatus();
			return ;
		}
		m_RebaseStage=REBASE_CONTINUE;
		curRev->GetRebaseAction() |= CGitLogListBase::LOGACTIONS_REBASE_DONE;
		m_forRewrite.push_back(curRev->m_CommitHash);
		this->UpdateCurrentStatus();
	}

	if( m_RebaseStage == REBASE_CONFLICT )
	{
		if(VerifyNoConflict())
			return;

		if (CAppUtils::MessageContainsConflictHints(GetSafeHwnd(), m_LogMessageCtrl.GetText()))
			return;

		m_ctrlTabCtrl.SetActiveTab(REBASE_TAB_LOG);

		GitRevLoglist* curRev = m_CommitList.m_arShownList.SafeGetAt(m_CurrentRebaseIndex);
		// ***************************************************
		// ATTENTION: Similar code in CommitDlg.cpp!!!
		// ***************************************************
		CMassiveGitTask mgtReAddAfterCommit(L"add --ignore-errors -f");
		CMassiveGitTask mgtReDelAfterCommit(L"rm --cached --ignore-unmatch");
		CMassiveGitTask mgtAdd(L"add -f");
		CMassiveGitTask mgtUpdateIndexForceRemove(L"update-index --force-remove");
		CMassiveGitTask mgtUpdateIndex(L"update-index");
		CMassiveGitTask mgtRm(L"rm  --ignore-unmatch");
		CMassiveGitTask mgtRmFCache(L"rm -f --cache");
		CMassiveGitTask mgtReset(L"reset", TRUE, true);
		auto locker(m_FileListCtrl.AcquireReadLock());
		for (int i = 0; i < m_FileListCtrl.GetItemCount(); i++)
		{
			auto entry = m_FileListCtrl.GetListEntry(i);
			if (entry->m_Checked)
			{
				if ((entry->m_Action & CTGitPath::LOGACTIONS_UNVER) || (entry->IsDirectory() && !(entry->m_Action & CTGitPath::LOGACTIONS_DELETED)))
					mgtAdd.AddFile(entry->GetGitPathString());
				else if (entry->m_Action & CTGitPath::LOGACTIONS_DELETED)
					mgtUpdateIndexForceRemove.AddFile(entry->GetGitPathString());
				else
					mgtUpdateIndex.AddFile(entry->GetGitPathString());

				if ((entry->m_Action & CTGitPath::LOGACTIONS_REPLACED) && !entry->GetGitOldPathString().IsEmpty())
					mgtRm.AddFile(entry->GetGitOldPathString());
			}
			else
			{
				if (entry->m_Action & CTGitPath::LOGACTIONS_ADDED || entry->m_Action & CTGitPath::LOGACTIONS_REPLACED)
				{
					mgtRmFCache.AddFile(entry->GetGitPathString());
					mgtReAddAfterCommit.AddFile(*entry);

					if (entry->m_Action & CTGitPath::LOGACTIONS_REPLACED && !entry->GetGitOldPathString().IsEmpty())
					{
						mgtReset.AddFile(entry->GetGitOldPathString());
						mgtReDelAfterCommit.AddFile(entry->GetGitOldPathString());
					}
				}
				else if(!(entry->m_Action & CTGitPath::LOGACTIONS_UNVER))
				{
					mgtReset.AddFile(entry->GetGitPathString());
					if (entry->m_Action & CTGitPath::LOGACTIONS_DELETED && !(entry->m_Action & CTGitPath::LOGACTIONS_MISSING))
						mgtReDelAfterCommit.AddFile(entry->GetGitPathString());
				}
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
			AddLogString(L"An error occurred while updating the index.");
			return;
		}

		CString allowempty;
		bool skipCurrent = false;
		if (!m_CurrentCommitEmpty)
		{
			if (g_Git.IsResultingCommitBecomeEmpty() == TRUE)
			{
				if (CheckNextCommitIsSquash() == 0)
				{
					allowempty = L"--allow-empty ";
					m_CurrentCommitEmpty = false;
				}
				else
				{
					int choose = CMessageBox::ShowCheck(GetSafeHwnd(), IDS_CHERRYPICK_EMPTY, IDS_APPNAME, 1, IDI_QUESTION, IDS_COMMIT_COMMIT, IDS_SKIPBUTTON, IDS_MSGBOX_CANCEL, nullptr, 0);
					if (choose == 2)
						skipCurrent = true;
					else if (choose == 1)
					{
						allowempty = L"--allow-empty ";
						m_CurrentCommitEmpty = true;
					}
					else
						return;
				}
			}
		}

		CString out;
		CString cmd;
		cmd.Format(L"git.exe commit %s--allow-empty-message -C %s", static_cast<LPCTSTR>(allowempty), static_cast<LPCTSTR>(curRev->m_CommitHash.ToString()));

		AddLogString(cmd);

		if (!skipCurrent && g_Git.Run(cmd, &out, CP_UTF8))
		{
			AddLogString(out);
			CMessageBox::Show(GetSafeHwnd(), out, L"TortoiseGit", MB_OK | MB_ICONERROR);
			return;
		}

		AddLogString(out);

		// update commit message if needed
		CString str = m_LogMessageCtrl.GetText().Trim();
		if (!skipCurrent && str != (curRev->GetSubject() + L'\n' + curRev->GetBody()).Trim())
		{
			if (str.IsEmpty())
			{
				CMessageBox::Show(GetSafeHwnd(), IDS_PROC_COMMITMESSAGE_EMPTY,IDS_APPNAME, MB_OK | MB_ICONERROR);
				return;
			}
			CString tempfile = ::GetTempFile();
			SCOPE_EXIT{ ::DeleteFile(tempfile); };
			if (CAppUtils::SaveCommitUnicodeFile(tempfile, str))
			{
				CMessageBox::Show(GetSafeHwnd(), L"Could not save commit message", L"TortoiseGit", MB_OK | MB_ICONERROR);
				return;
			}

			out.Empty();
			cmd.Format(L"git.exe commit --amend -F \"%s\"", static_cast<LPCTSTR>(tempfile));
			AddLogString(cmd);

			if (g_Git.Run(cmd, &out, CP_UTF8))
			{
				AddLogString(out);
				if (!g_Git.CheckCleanWorkTree())
				{
					CMessageBox::Show(GetSafeHwnd(), out, L"TortoiseGit", MB_OK | MB_ICONERROR);
					return;
				}
				CString retry;
				retry.LoadString(IDS_MSGBOX_RETRY);
				CString ignore;
				ignore.LoadString(IDS_MSGBOX_IGNORE);
				if (CMessageBox::Show(GetSafeHwnd(), out, L"TortoiseGit", 1, IDI_ERROR, retry, ignore) == 1)
					return;
			}

			AddLogString(out);
		}

		if (static_cast<DWORD>(CRegStdDWORD(L"Software\\TortoiseGit\\ReaddUnselectedAddedFilesAfterCommit", TRUE)) == TRUE)
		{
			BOOL cancel2 = FALSE;
			mgtReAddAfterCommit.Execute(cancel2);
			mgtReDelAfterCommit.Execute(cancel2);
		}

		if (curRev->GetRebaseAction() & CGitLogListBase::LOGACTIONS_REBASE_EDIT)
		{
			m_RebaseStage=REBASE_EDIT;
			this->m_ctrlTabCtrl.SetActiveTab(REBASE_TAB_MESSAGE);
			this->UpdateCurrentStatus();
			return;
		}
		else
		{
			m_RebaseStage=REBASE_CONTINUE;
			curRev->GetRebaseAction() |= CGitLogListBase::LOGACTIONS_REBASE_DONE;
			this->UpdateCurrentStatus();

			if (CheckNextCommitIsSquash() == 0) // remember commit msg after edit if next commit if squash
				ResetParentForSquash(str);
			else
			{
				m_SquashMessage.Empty();
				CGitHash head;
				if (g_Git.GetHash(head, L"HEAD"))
				{
					MessageBox(g_Git.GetGitLastErr(L"Could not get HEAD hash."), L"TortoiseGit", MB_ICONERROR);
					return;
				}
				m_rewrittenCommitsMap[curRev->m_CommitHash] = head;
			}
		}
	}

	if ((m_RebaseStage == REBASE_EDIT || m_RebaseStage == REBASE_CONTINUE || m_bSplitCommit || m_RebaseStage == REBASE_SQUASH_EDIT) && CheckNextCommitIsSquash() && (m_bSplitCommit || !g_Git.CheckCleanWorkTree(true)))
	{
		if (!m_bSplitCommit && CMessageBox::Show(GetSafeHwnd(), IDS_PROC_REBASE_CONTINUE_NOTCLEAN, IDS_APPNAME, 1, IDI_ERROR, IDS_MSGBOX_OK, IDS_ABORTBUTTON) == 2)
			return;
		BOOL isFirst = TRUE;
		do
		{
			CCommitDlg dlg;
			if (isFirst)
				dlg.m_sLogMessage = m_LogMessageCtrl.GetText();
			dlg.m_bWholeProject = true;
			dlg.m_bSelectFilesForCommit = true;
			dlg.m_bCommitAmend = isFirst && (m_RebaseStage != REBASE_SQUASH_EDIT); //  do not amend on squash_edit stage, we need a normal commit there
			if (isFirst && m_RebaseStage == REBASE_SQUASH_EDIT)
			{
				if (m_iSquashdate != 2)
					dlg.SetTime(m_SquashFirstMetaData.time);
				dlg.SetAuthor(m_SquashFirstMetaData.GetAuthor());
			}
			CTGitPathList gpl;
			gpl.AddPath(CTGitPath());
			dlg.m_pathList = gpl;
			dlg.m_bAmendDiffToLastCommit = !m_bSplitCommit;
			dlg.m_bNoPostActions = true;
			if (dlg.m_bCommitAmend)
				dlg.m_AmendStr = dlg.m_sLogMessage;
			dlg.m_bWarnDetachedHead = false;

			if (dlg.DoModal() != IDOK)
				return;

			isFirst = !m_bSplitCommit; // only select amend on second+ runs if not in split commit mode

			m_SquashMessage.Empty();
			m_CurrentCommitEmpty = dlg.m_bCommitMessageOnly;
		} while (!g_Git.CheckCleanWorkTree() || (m_bSplitCommit && CMessageBox::Show(GetSafeHwnd(), IDS_REBASE_ADDANOTHERCOMMIT, IDS_APPNAME, MB_YESNO | MB_ICONQUESTION) == IDYES));

		m_bSplitCommit = FALSE;
		UpdateData(FALSE);

		this->m_ctrlTabCtrl.SetActiveTab(REBASE_TAB_LOG);
		m_RebaseStage = REBASE_CONTINUE;
		GitRevLoglist* curRev = m_CommitList.m_arShownList.SafeGetAt(m_CurrentRebaseIndex);
		CGitHash head;
		if (g_Git.GetHash(head, L"HEAD"))
		{
			MessageBox(g_Git.GetGitLastErr(L"Could not get HEAD hash."), L"TortoiseGit", MB_ICONERROR);
			return;
		}
		m_rewrittenCommitsMap[curRev->m_CommitHash] = head;
		for (const auto& hash : m_forRewrite)
			m_rewrittenCommitsMap[hash] = head;
		m_forRewrite.clear();
		curRev->GetRebaseAction() |= CGitLogListBase::LOGACTIONS_REBASE_DONE;
		this->UpdateCurrentStatus();
	}

	if( m_RebaseStage == REBASE_EDIT ||  m_RebaseStage == REBASE_SQUASH_EDIT )
	{
		CString str;
		GitRevLoglist* curRev = m_CommitList.m_arShownList.SafeGetAt(m_CurrentRebaseIndex);

		str=this->m_LogMessageCtrl.GetText();
		if(str.Trim().IsEmpty())
		{
			CMessageBox::Show(GetSafeHwnd(), IDS_PROC_COMMITMESSAGE_EMPTY,IDS_APPNAME, MB_OK | MB_ICONERROR);
				return;
		}

		CString tempfile=::GetTempFile();
		SCOPE_EXIT{ ::DeleteFile(tempfile); };
		if (CAppUtils::SaveCommitUnicodeFile(tempfile, str))
		{
			CMessageBox::Show(GetSafeHwnd(), L"Could not save commit message", L"TortoiseGit", MB_OK | MB_ICONERROR);
			return;
		}

		CString out, cmd, options;
		bool skipCurrent = false;
		if (m_CurrentCommitEmpty)
			options = L"--allow-empty ";
		else if (g_Git.IsResultingCommitBecomeEmpty(m_RebaseStage != REBASE_SQUASH_EDIT) == TRUE)
		{
			int choose = CMessageBox::ShowCheck(GetSafeHwnd(), IDS_CHERRYPICK_EMPTY, IDS_APPNAME, 1, IDI_QUESTION, IDS_COMMIT_COMMIT, IDS_SKIPBUTTON, IDS_MSGBOX_CANCEL, nullptr, 0);
			if (choose == 2)
				skipCurrent = true;
			else if (choose == 1)
			{
				options = L"--allow-empty ";
				m_CurrentCommitEmpty = true;
			}
			else
				return;
		}

		if (m_RebaseStage == REBASE_SQUASH_EDIT)
			cmd.Format(L"git.exe commit %s%s-F \"%s\"", static_cast<LPCTSTR>(options), static_cast<LPCTSTR>(m_SquashFirstMetaData.GetAsParam(m_iSquashdate == 2)), static_cast<LPCTSTR>(tempfile));
		else
			cmd.Format(L"git.exe commit --amend %s-F \"%s\"", static_cast<LPCTSTR>(options), static_cast<LPCTSTR>(tempfile));

		if (!skipCurrent && g_Git.Run(cmd, &out, CP_UTF8))
		{
			if (!g_Git.CheckCleanWorkTree())
			{
				CMessageBox::Show(GetSafeHwnd(), out, L"TortoiseGit", MB_OK | MB_ICONERROR);
				return;
			}

			CString retry;
			retry.LoadString(IDS_MSGBOX_RETRY);
			CString ignore;
			ignore.LoadString(IDS_MSGBOX_IGNORE);
			if (CMessageBox::Show(GetSafeHwnd(), out, L"TortoiseGit", 1, IDI_ERROR, retry, ignore) == 1)
				return;
		}

		AddLogString(out);
		if (CheckNextCommitIsSquash() == 0 && m_RebaseStage != REBASE_SQUASH_EDIT) // remember commit msg after edit if next commit if squash; but don't do this if ...->squash(reset here)->pick->squash
		{
			ResetParentForSquash(str);
		}
		else
			m_SquashMessage.Empty();
		this->m_ctrlTabCtrl.SetActiveTab(REBASE_TAB_LOG);
		m_RebaseStage=REBASE_CONTINUE;
		CGitHash head;
		if (g_Git.GetHash(head, L"HEAD"))
		{
			MessageBox(g_Git.GetGitLastErr(L"Could not get HEAD hash."), L"TortoiseGit", MB_ICONERROR);
			return;
		}
		m_rewrittenCommitsMap[curRev->m_CommitHash] = head; // we had a reset to parent, so this is not the correct hash
		for (const auto& hash : m_forRewrite)
			m_rewrittenCommitsMap[hash] = head;
		m_forRewrite.clear();
		curRev->GetRebaseAction() |= CGitLogListBase::LOGACTIONS_REBASE_DONE;
		this->UpdateCurrentStatus();
	}


	InterlockedExchange(&m_bThreadRunning, TRUE);
	SetControlEnable();

	if (!AfxBeginThread(RebaseThreadEntry, this))
	{
		InterlockedExchange(&m_bThreadRunning, FALSE);
		CMessageBox::Show(GetSafeHwnd(), L"Create Rebase Thread Fail", L"TortoiseGit", MB_OK | MB_ICONERROR);
		SetControlEnable();
	}
}

void CRebaseDlg::ResetParentForSquash(const CString& commitMessage)
{
	m_SquashMessage = commitMessage;
	// reset parent so that we can do "git cherry-pick --no-commit" w/o introducing an unwanted commit
	CString cmd = L"git.exe reset --soft HEAD~1";
	m_ctrlTabCtrl.SetActiveTab(REBASE_TAB_LOG);
	if (RunGitCmdRetryOrAbort(cmd))
		return;
}

int CRebaseDlg::CheckNextCommitIsSquash()
{
	int index;
	if(m_CommitList.m_IsOldFirst)
		index=m_CurrentRebaseIndex+1;
	else
		index=m_CurrentRebaseIndex-1;

	GitRevLoglist* curRev;
	do
	{
		if(index<0)
			return -1;
		if(index>= m_CommitList.GetItemCount())
			return -1;

		curRev = m_CommitList.m_arShownList.SafeGetAt(index);

		if (curRev->GetRebaseAction() & CGitLogListBase::LOGACTIONS_REBASE_SQUASH)
			return 0;
		if (curRev->GetRebaseAction() & CGitLogListBase::LOGACTIONS_REBASE_SKIP)
		{
			if(m_CommitList.m_IsOldFirst)
				++index;
			else
				--index;
		}
		else
			return -1;

	} while(curRev->GetRebaseAction() & CGitLogListBase::LOGACTIONS_REBASE_SKIP);

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

		this->GetDlgItem(IDC_SPLITALLOPTIONS)->EnableWindow(TRUE);
		this->GetDlgItem(IDC_BUTTON_UP)->EnableWindow(TRUE);
		this->GetDlgItem(IDC_BUTTON_DOWN)->EnableWindow(TRUE);
		this->GetDlgItem(IDC_BUTTON_ADD)->EnableWindow(!m_bPreserveMerges);
		m_CommitList.EnableDragnDrop(true);

		if(!m_IsCherryPick)
		{
			this->GetDlgItem(IDC_REBASE_COMBOXEX_BRANCH)->EnableWindow(TRUE);
			this->GetDlgItem(IDC_REBASE_COMBOXEX_UPSTREAM)->EnableWindow(TRUE);
			this->GetDlgItem(IDC_BUTTON_REVERSE)->EnableWindow(TRUE);
			this->GetDlgItem(IDC_REBASE_CHECK_FORCE)->EnableWindow(TRUE);
			this->GetDlgItem(IDC_REBASE_CHECK_PRESERVEMERGES)->EnableWindow(TRUE);
		}
		this->m_CommitList.m_ContextMenuMask |= m_CommitList.GetContextMenuBit(CGitLogListBase::ID_REBASE_PICK)|
												m_CommitList.GetContextMenuBit(CGitLogListBase::ID_REBASE_SQUASH)|
												m_CommitList.GetContextMenuBit(CGitLogListBase::ID_REBASE_EDIT)|
												m_CommitList.GetContextMenuBit(CGitLogListBase::ID_REBASE_SKIP)|
												m_CommitList.GetContextMenuBit(CGitLogListBase::ID_LOG);
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
		this->GetDlgItem(IDC_SPLITALLOPTIONS)->EnableWindow(FALSE);
		this->GetDlgItem(IDC_REBASE_COMBOXEX_BRANCH)->EnableWindow(FALSE);
		this->GetDlgItem(IDC_REBASE_COMBOXEX_UPSTREAM)->EnableWindow(FALSE);
		this->GetDlgItem(IDC_BUTTON_REVERSE)->EnableWindow(FALSE);
		this->GetDlgItem(IDC_REBASE_CHECK_FORCE)->EnableWindow(FALSE);
		this->GetDlgItem(IDC_REBASE_CHECK_PRESERVEMERGES)->EnableWindow(FALSE);
		this->GetDlgItem(IDC_BUTTON_UP)->EnableWindow(FALSE);
		this->GetDlgItem(IDC_BUTTON_DOWN)->EnableWindow(FALSE);
		m_CommitList.EnableDragnDrop(false);
		this->GetDlgItem(IDC_BUTTON_ADD)->EnableWindow(FALSE);

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

	GetDlgItem(IDC_REBASE_SPLIT_COMMIT)->ShowWindow((m_RebaseStage == REBASE_EDIT || m_RebaseStage == REBASE_SQUASH_EDIT) ? SW_SHOW : SW_HIDE);

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
		text.FormatMessage(IDS_PROC_REBASING_PROGRESS, index, m_CommitList.GetItemCount());
		m_sStatusText = text;
		m_CtrlStatusText.SetWindowText(text);
		m_bStatusWarning = false;
		m_CtrlStatusText.Invalidate();
	}

	GitRevLoglist* prevRev = nullptr, *curRev = nullptr;

	if (m_CurrentRebaseIndex >= 0 && m_CurrentRebaseIndex < static_cast<int>(m_CommitList.m_arShownList.size()))
		curRev = m_CommitList.m_arShownList.SafeGetAt(m_CurrentRebaseIndex);

	for (int i = 0; i < static_cast<int>(m_CommitList.m_arShownList.size()); ++i)
	{
		prevRev = m_CommitList.m_arShownList.SafeGetAt(i);
		if (prevRev->GetRebaseAction() & CGitLogListBase::LOGACTIONS_REBASE_CURRENT)
		{
			CRect rect;
			prevRev->GetRebaseAction() &= ~CGitLogListBase::LOGACTIONS_REBASE_CURRENT;
			m_CommitList.GetItemRect(i,&rect,LVIR_BOUNDS);
			m_CommitList.InvalidateRect(rect);
		}
	}

	if(curRev)
	{
		CRect rect;
		curRev->GetRebaseAction() |= CGitLogListBase::LOGACTIONS_REBASE_CURRENT;
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
	if (m_RebaseStage == REBASE_DONE)
		GetDlgItem(IDC_REBASE_CONTINUE)->SetFocus();
}

void CRebaseDlg::AddLogString(const CString& str)
{
	this->m_wndOutputRebase.SendMessage(SCI_SETREADONLY, FALSE);
	CStringA sTextA = m_wndOutputRebase.StringForControl(str);//CUnicodeUtils::GetUTF8(str);
	this->m_wndOutputRebase.SendMessage(SCI_DOCUMENTEND);
	this->m_wndOutputRebase.SendMessage(SCI_REPLACESEL, 0, reinterpret_cast<LPARAM>(static_cast<LPCSTR>(sTextA)));
	this->m_wndOutputRebase.SendMessage(SCI_REPLACESEL, 0, reinterpret_cast<LPARAM>("\n"));
	this->m_wndOutputRebase.SendMessage(SCI_SETREADONLY, TRUE);
}

int CRebaseDlg::GetCurrentCommitID()
{
	if(m_CommitList.m_IsOldFirst)
		return this->m_CurrentRebaseIndex+1;
	else
		return m_CommitList.GetItemCount()-m_CurrentRebaseIndex;
}

int CRebaseDlg::IsCommitEmpty(const CGitHash& hash)
{
	CString cmd, tree, ptree;
	cmd.Format(L"git.exe rev-parse -q --verify %s^{tree}", static_cast<LPCTSTR>(hash.ToString()));
	if (g_Git.Run(cmd, &tree, CP_UTF8))
	{
		AddLogString(cmd);
		AddLogString(tree);
		return -1;
	}
	cmd.Format(L"git.exe rev-parse -q --verify %s^^{tree}", static_cast<LPCTSTR>(hash.ToString()));
	if (g_Git.Run(cmd, &ptree, CP_UTF8))
		ptree = L"4b825dc642cb6eb9a060e54bf8d69288fbee4904"; // empty tree
	return tree == ptree;
}

static CString GetCommitTitle(const CGitHash& parentHash)
{
	CString str;
	GitRev rev;
	if (rev.GetCommit(parentHash.ToString()) == 0)
	{
		CString commitTitle = rev.GetSubject();
		if (commitTitle.GetLength() > 20)
		{
			commitTitle.Truncate(20);
			commitTitle += L"...";
		}
		str.AppendFormat(L"\n%s (%s)", static_cast<LPCTSTR>(commitTitle), static_cast<LPCTSTR>(parentHash.ToString(g_Git.GetShortHASHLength())));
	}
	else
		str.AppendFormat(L"\n(%s)", static_cast<LPCTSTR>(parentHash.ToString(g_Git.GetShortHASHLength())));
	return str;
}

int CRebaseDlg::DoRebase()
{
	CString cmd,out;
	if(m_CurrentRebaseIndex <0)
		return 0;
	if(m_CurrentRebaseIndex >= m_CommitList.GetItemCount() )
		return 0;

	GitRevLoglist* pRev = m_CommitList.m_arShownList.SafeGetAt(m_CurrentRebaseIndex);
	int mode = pRev->GetRebaseAction() & CGitLogListBase::LOGACTIONS_REBASE_MODE_MASK;
	CString nocommit;

	if (mode == CGitLogListBase::LOGACTIONS_REBASE_SKIP)
	{
		pRev->GetRebaseAction() |= CGitLogListBase::LOGACTIONS_REBASE_DONE;
		return 0;
	}

	bool nextCommitIsSquash = (CheckNextCommitIsSquash() == 0);
	if (nextCommitIsSquash || mode != CGitLogListBase::LOGACTIONS_REBASE_PICK)
	{ // next commit is squash or not pick
		if (!this->m_SquashMessage.IsEmpty())
			this->m_SquashMessage += L"\n\n";
		this->m_SquashMessage += pRev->GetSubject();
		this->m_SquashMessage += L'\n';
		this->m_SquashMessage += pRev->GetBody().TrimRight();
		if (m_bAddCherryPickedFrom)
		{
			if (!pRev->GetBody().IsEmpty())
				m_SquashMessage += L'\n';
			m_SquashMessage += L"(cherry picked from commit ";
			m_SquashMessage += pRev->m_CommitHash.ToString();
			m_SquashMessage += L')';
		}
	}
	else
	{
		this->m_SquashMessage.Empty();
		m_SquashFirstMetaData.Empty();
	}

	if (nextCommitIsSquash && mode != CGitLogListBase::LOGACTIONS_REBASE_SQUASH)
		m_SquashFirstMetaData = SquashFirstMetaData(pRev);

	if ((nextCommitIsSquash && mode != CGitLogListBase::LOGACTIONS_REBASE_EDIT) || mode == CGitLogListBase::LOGACTIONS_REBASE_SQUASH)
	{ // next or this commit is squash (don't do this on edit->squash sequence)
		nocommit = L" --no-commit ";
		if (m_iSquashdate == 1)
			m_SquashFirstMetaData.UpdateDate(pRev);
	}

	CString log;
	log.Format(L"%s %d: %s", static_cast<LPCTSTR>(CGitLogListBase::GetRebaseActionName(mode)), GetCurrentCommitID(), static_cast<LPCTSTR>(pRev->m_CommitHash.ToString()));
	AddLogString(log);
	AddLogString(pRev->GetSubject());
	if (pRev->GetSubject().IsEmpty())
	{
		CMessageBox::Show(m_hWnd, IDS_PROC_REBASE_EMPTYCOMMITMSG, IDS_APPNAME, MB_OK | MB_ICONEXCLAMATION);
		mode = CGitLogListBase::LOGACTIONS_REBASE_EDIT;
	}

	CString cherryPickedFrom;
	if (m_bAddCherryPickedFrom)
		cherryPickedFrom = L"-x ";
	else if (!m_IsCherryPick && nocommit.IsEmpty())
		cherryPickedFrom = L"--ff "; // for issue #1833: "If the current HEAD is the same as the parent of the cherry-picked commit, then a fast forward to this commit will be performed."

	int isEmpty = IsCommitEmpty(pRev->m_CommitHash);
	if (isEmpty == 1)
	{
		cherryPickedFrom += L"--allow-empty ";
		if (mode != CGitLogListBase::LOGACTIONS_REBASE_SQUASH)
			m_CurrentCommitEmpty = true;
	}
	else if (isEmpty < 0)
		return -1;
	else
		m_CurrentCommitEmpty = false;

	if (m_IsCherryPick && pRev->m_ParentHash.size() > 1)
	{
		CString msg;
		msg.FormatMessage(IDS_CHERRYPICK_MERGECOMMIT, static_cast<LPCTSTR>(pRev->m_CommitHash.ToString()), static_cast<LPCTSTR>(pRev->GetSubject()));
		CString parent1;
		parent1.Format(IDS_PARENT, 1);
		parent1 += GetCommitTitle(pRev->m_ParentHash.at(0));
		CString parent2;
		parent2.Format(IDS_PARENT, 2);
		parent2 += GetCommitTitle(pRev->m_ParentHash.at(1));
		CString cancel;
		cancel.LoadString(IDS_MSGBOX_CANCEL);
		auto ret = CMessageBox::Show(m_hWnd, msg, L"TortoiseGit", 3, IDI_QUESTION, parent1, parent2, cancel);
		if (ret == 3)
			return - 1;

		cherryPickedFrom.AppendFormat(L"-m %d ", ret);
	}

	while (true)
	{
		cmd.Format(L"git.exe cherry-pick %s%s %s", static_cast<LPCTSTR>(cherryPickedFrom), static_cast<LPCTSTR>(nocommit), static_cast<LPCTSTR>(pRev->m_CommitHash.ToString()));
		if (m_bPreserveMerges)
		{
			bool parentRewritten = false;
			CGitHash currentHeadHash;
			if (g_Git.GetHash(currentHeadHash, L"HEAD"))
			{
				m_RebaseStage = REBASE_ERROR;
				MessageBox(g_Git.GetGitLastErr(L"Could not get HEAD hash."), L"TortoiseGit", MB_ICONERROR);
				return -1;
			}
			if (!m_currentCommits.empty())
			{
				for (const auto& commit : m_currentCommits)
					m_rewrittenCommitsMap[commit] = currentHeadHash;
				m_currentCommits.clear();
			}
			m_currentCommits.push_back(pRev->m_CommitHash);
			GIT_REV_LIST possibleParents = pRev->m_ParentHash;
			GIT_REV_LIST newParents;
			for (auto it = possibleParents.cbegin(); it != possibleParents.cend(); it = possibleParents.begin())
			{
				CGitHash parent = *it;
				possibleParents.erase(it);

				const auto rewrittenParent = m_rewrittenCommitsMap.find(parent);
				if (rewrittenParent == m_rewrittenCommitsMap.cend())
				{
					auto droppedCommitParents = m_droppedCommitsMap.find(parent);
					if (droppedCommitParents != m_droppedCommitsMap.cend())
					{
						parentRewritten = true;
						for (auto droppedIt = droppedCommitParents->second.crbegin(); droppedIt != droppedCommitParents->second.crend(); ++droppedIt)
							possibleParents.insert(possibleParents.begin(), *droppedIt);
						continue;
					}

					newParents.push_back(parent);
					continue;
				}

				if (rewrittenParent->second.IsEmpty() && parent == pRev->m_ParentHash[0] && pRev->ParentsCount() > 1)
				{
					m_RebaseStage = REBASE_ERROR;
					AddLogString(L"");
					AddLogString(L"Unrecoverable error: Merge commit parent missing.");
					return -1;
				}

				CGitHash newParent = rewrittenParent->second;
				if (newParent.IsEmpty()) // use current HEAD as fallback
					newParent = currentHeadHash;

				if (newParent != parent)
					parentRewritten = true;

				if (std::find(newParents.begin(), newParents.end(), newParent) == newParents.end())
					newParents.push_back(newParent);
			}
			if (pRev->ParentsCount() > 1)
			{
				if (mode == CGitLogListBase::LOGACTIONS_REBASE_SQUASH)
				{
					m_RebaseStage = REBASE_ERROR;
					AddLogString(L"Cannot squash merge commit on rebase.");
					return -1;
				}
				if (!parentRewritten && nocommit.IsEmpty())
					cmd.Format(L"git.exe reset --hard %s", static_cast<LPCTSTR>(pRev->m_CommitHash.ToString()));
				else
				{
					CString parentString;
					for (const auto& parent : newParents)
						parentString += L' ' + parent.ToString();
					cmd.Format(L"git.exe checkout %s", static_cast<LPCTSTR>(newParents[0].ToString()));
					if (RunGitCmdRetryOrAbort(cmd))
					{
						m_RebaseStage = REBASE_ERROR;
						return -1;
					}
					cmd.Format(L"git.exe merge --no-ff%s %s", static_cast<LPCTSTR>(nocommit), static_cast<LPCTSTR>(parentString));
					if (nocommit.IsEmpty())
					{
						if (g_Git.Run(cmd, &out, CP_UTF8))
						{
							AddLogString(cmd);
							AddLogString(out);
							int hasConflicts = g_Git.HasWorkingTreeConflicts();
							if (hasConflicts > 0)
							{
								m_RebaseStage = REBASE_CONFLICT;
								return -1;
							}
							else if (hasConflicts < 0)
								AddLogString(g_Git.GetGitLastErr(L"Checking for conflicts failed.", CGit::GIT_CMD_CHECKCONFLICTS));
							AddLogString(L"An unrecoverable error occurred.");
							m_RebaseStage = REBASE_ERROR;
							return -1;
						}
						CGitHash newHeadHash;
						if (g_Git.GetHash(newHeadHash, L"HEAD"))
						{
							m_RebaseStage = REBASE_ERROR;
							MessageBox(g_Git.GetGitLastErr(L"Could not get HEAD hash."), L"TortoiseGit", MB_ICONERROR);
							return -1;
						}
						// do nothing if already up2date
						if (currentHeadHash != newHeadHash)
							cmd.Format(L"git.exe commit --amend -C %s", static_cast<LPCTSTR>(pRev->m_CommitHash.ToString()));
					}
				}
			}
			else
			{
				if (mode != CGitLogListBase::LOGACTIONS_REBASE_SQUASH)
				{
					cmd.Format(L"git.exe checkout %s", static_cast<LPCTSTR>(newParents[0].ToString()));
					if (RunGitCmdRetryOrAbort(cmd))
					{
						m_RebaseStage = REBASE_ERROR;
						return -1;
					}
				}
				cmd.Format(L"git.exe cherry-pick %s%s %s", static_cast<LPCTSTR>(cherryPickedFrom), static_cast<LPCTSTR>(nocommit), static_cast<LPCTSTR>(pRev->m_CommitHash.ToString()));
			}
		}

		if(g_Git.Run(cmd,&out,CP_UTF8))
		{
			AddLogString(out);
			int hasConflicts = g_Git.HasWorkingTreeConflicts();
			if (hasConflicts < 0)
			{
				AddLogString(g_Git.GetGitLastErr(L"Checking for conflicts failed.", CGit::GIT_CMD_CHECKCONFLICTS));
				return -1;
			}
			if (!hasConflicts)
			{
				if (out.Find(L"commit --allow-empty") > 0)
				{
					int choose = CMessageBox::ShowCheck(GetSafeHwnd(), IDS_CHERRYPICK_EMPTY, IDS_APPNAME, 1, IDI_QUESTION, IDS_COMMIT_COMMIT, IDS_SKIPBUTTON, IDS_MSGBOX_CANCEL, nullptr, 0);
					if (choose != 1)
					{
						if (choose == 2 && !RunGitCmdRetryOrAbort(L"git.exe reset --hard"))
						{
							pRev->GetRebaseAction() |= CGitLogListBase::LOGACTIONS_REBASE_DONE;
							m_CommitList.Invalidate();
							return 0;
						}

						m_RebaseStage = REBASE_ERROR;
						AddLogString(L"An unrecoverable error occurred.");
						return -1;
					}

					cmd.Format(L"git.exe commit --allow-empty -C %s", static_cast<LPCTSTR>(pRev->m_CommitHash.ToString()));
					out.Empty();
					g_Git.Run(cmd, &out, CP_UTF8);
					m_CurrentCommitEmpty = true;
				}
				else if (mode == CGitLogListBase::LOGACTIONS_REBASE_PICK)
				{
					if (m_pTaskbarList)
						m_pTaskbarList->SetProgressState(m_hWnd, TBPF_ERROR);
					int choose = -1;
					if (!m_bAutoSkipFailedCommit)
					{
						choose = CMessageBox::ShowCheck(GetSafeHwnd(), IDS_CHERRYPICKFAILEDSKIP, IDS_APPNAME, 1, IDI_QUESTION, IDS_SKIPBUTTON, IDS_MSGBOX_RETRY, IDS_MSGBOX_CANCEL, nullptr, IDS_DO_SAME_FOR_REST, &m_bAutoSkipFailedCommit);
						if (choose == 2)
						{
							m_bAutoSkipFailedCommit = FALSE;
							continue;  // retry cherry pick
						}
					}
					if (m_bAutoSkipFailedCommit || choose == 1)
					{
						if (!RunGitCmdRetryOrAbort(L"git.exe reset --hard"))
						{
							pRev->GetRebaseAction() = CGitLogListBase::LOGACTIONS_REBASE_SKIP;
							m_CommitList.Invalidate();
							return 0;
						}
					}

					m_RebaseStage = REBASE_ERROR;
					AddLogString(L"An unrecoverable error occurred.");
					return -1;
				}
				else if (mode == CGitLogListBase::LOGACTIONS_REBASE_EDIT)
				{
					this->m_RebaseStage = REBASE_EDIT ;
					return -1; // Edit return -1 to stop rebase.
				}
				// Squash Case
				else if (CheckNextCommitIsSquash())
				{   // no squash
					// let user edit last commmit message
					this->m_RebaseStage = REBASE_SQUASH_EDIT;
					return -1;
				}
			}
			else
			{
				if (m_pTaskbarList)
					m_pTaskbarList->SetProgressState(m_hWnd, TBPF_ERROR);
				if (mode == CGitLogListBase::LOGACTIONS_REBASE_SQUASH)
					m_RebaseStage = REBASE_SQUASH_CONFLICT;
				else
					m_RebaseStage = REBASE_CONFLICT;
				return -1;
			}
		}

		AddLogString(out);
		if (mode == CGitLogListBase::LOGACTIONS_REBASE_PICK)
		{
			if (nocommit.IsEmpty())
			{
				CGitHash head;
				if (g_Git.GetHash(head, L"HEAD"))
				{
					MessageBox(g_Git.GetGitLastErr(L"Could not get HEAD hash."), L"TortoiseGit", MB_ICONERROR);
					m_RebaseStage = REBASE_ERROR;
					return -1;
				}
				m_rewrittenCommitsMap[pRev->m_CommitHash] = head;
			}
			else
				m_forRewrite.push_back(pRev->m_CommitHash);
			pRev->GetRebaseAction() |= CGitLogListBase::LOGACTIONS_REBASE_DONE;
			return 0;
		}
		if (mode == CGitLogListBase::LOGACTIONS_REBASE_EDIT)
		{
			this->m_RebaseStage = REBASE_EDIT;
			return -1; // Edit return -1 to stop rebase.
		}

		// Squash Case
		if (CheckNextCommitIsSquash())
		{ // no squash
			// let user edit last commmit message
			this->m_RebaseStage = REBASE_SQUASH_EDIT;
			return -1;
		}
		else if (mode == CGitLogListBase::LOGACTIONS_REBASE_SQUASH)
		{
			pRev->GetRebaseAction() |= CGitLogListBase::LOGACTIONS_REBASE_DONE;
			m_forRewrite.push_back(pRev->m_CommitHash);
		}

		return 0;
	}
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
	while (!m_bAbort)
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
					break;
			}
		}
		else if( m_RebaseStage == REBASE_FINISH )
		{
			SendMessage(MSG_REBASE_UPDATE_UI);
			m_RebaseStage = REBASE_DONE;
			break;
		}
		else
			break;
		this->PostMessage(MSG_REBASE_UPDATE_UI);
	}

	InterlockedExchange(&m_bThreadRunning, FALSE);
	this->PostMessage(MSG_REBASE_UPDATE_UI);
	if (m_bAbort)
		PostMessage(WM_COMMAND, MAKELONG(IDC_REBASE_ABORT, BN_CLICKED), reinterpret_cast<LPARAM>(GetDlgItem(IDC_REBASE_ABORT)->GetSafeHwnd()));
	return ret;
}

void CRebaseDlg::ListConflictFile(bool noStoreScrollPosition)
{
	if (!noStoreScrollPosition)
		m_FileListCtrl.StoreScrollPos();
	this->m_FileListCtrl.Clear();
	m_FileListCtrl.SetHasCheckboxes(true);

	if (!m_IsCherryPick)
	{
		CString adminDir;
		if (GitAdminDir::GetAdminDirPath(g_Git.m_CurrentDir, adminDir))
		{
			CString dir(adminDir + L"tgitrebase.active");
			::CreateDirectory(dir, nullptr);
			CStringUtils::WriteStringToTextFile(dir + L"\\head-name", m_BranchCtrl.GetString());
			CStringUtils::WriteStringToTextFile(dir + L"\\onto", m_Onto.IsEmpty() ? m_UpstreamCtrl.GetString() : m_Onto);
		}
	}

	this->m_FileListCtrl.GetStatus(nullptr, true);
	m_FileListCtrl.Show(CTGitPath::LOGACTIONS_UNMERGED | CTGitPath::LOGACTIONS_MODIFIED | CTGitPath::LOGACTIONS_ADDED | CTGitPath::LOGACTIONS_DELETED | CTGitPath::LOGACTIONS_REPLACED, CTGitPath::LOGACTIONS_UNMERGED);

	m_FileListCtrl.Check(GITSLC_SHOWFILES);
	bool hasSubmoduleChange = false;
	auto locker(m_FileListCtrl.AcquireReadLock());
	for (int i = 0; i < m_FileListCtrl.GetItemCount(); i++)
	{
		auto entry = m_FileListCtrl.GetListEntry(i);
		if (entry->IsDirectory())
		{
			hasSubmoduleChange = true;
			break;
		}
	}

	if (hasSubmoduleChange)
	{
		m_CtrlStatusText.SetWindowText(m_sStatusText + L", " + CString(MAKEINTRESOURCE(IDS_CARE_SUBMODULE_CHANGES)));
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

	if (m_RebaseStage == REBASE_DONE && m_bRebaseAutoEnd)
	{
		m_bRebaseAutoEnd = false;
		this->PostMessage(WM_COMMAND, MAKELONG(IDC_REBASE_CONTINUE, BN_CLICKED), reinterpret_cast<LPARAM>(GetDlgItem(IDC_REBASE_CONTINUE)->GetSafeHwnd()));
	}

	if (m_RebaseStage == REBASE_DONE && m_pTaskbarList)
		m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NOPROGRESS); // do not show progress on taskbar any more to show we finished
	if(m_CurrentRebaseIndex <0)
		return 0;
	if(m_CurrentRebaseIndex >= m_CommitList.GetItemCount() )
		return 0;
	GitRev* curRev = m_CommitList.m_arShownList.SafeGetAt(m_CurrentRebaseIndex);

	switch(m_RebaseStage)
	{
	case REBASE_CONFLICT:
	case REBASE_SQUASH_CONFLICT:
		{
		ListConflictFile(true);
		this->m_ctrlTabCtrl.SetActiveTab(REBASE_TAB_CONFLICT);
		if (m_pTaskbarList)
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_ERROR);
		this->m_LogMessageCtrl.Call(SCI_SETREADONLY, FALSE);
		CString logMessage;
		if (m_IsCherryPick)
		{
			CString dotGitPath;
			GitAdminDir::GetWorktreeAdminDirPath(g_Git.m_CurrentDir, dotGitPath);
			// vanilla git also re-uses MERGE_MSG on conflict (listing all conflicted files)
			// and it's also needed for cherry-pick in order to get cherry-picked-from included on conflicts
			CGit::LoadTextFile(dotGitPath + L"MERGE_MSG", logMessage);
		}
		if (logMessage.IsEmpty())
			logMessage = curRev->GetSubject() + L'\n' + curRev->GetBody();
		this->m_LogMessageCtrl.SetText(logMessage);
		break;
		}
	case REBASE_EDIT:
		this->m_ctrlTabCtrl.SetActiveTab(REBASE_TAB_MESSAGE);
		if (m_pTaskbarList)
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_PAUSED);
		this->m_LogMessageCtrl.Call(SCI_SETREADONLY, FALSE);
		if (m_bAddCherryPickedFrom)
		{
			// Since the new commit is done and the HEAD points to it,
			// just using the new body modified by git self.
			GitRev headRevision;
			if (headRevision.GetCommit(L"HEAD"))
				MessageBox(headRevision.GetLastErr(), L"TortoiseGit", MB_ICONERROR);

			m_LogMessageCtrl.SetText(headRevision.GetSubject() + L'\n' + headRevision.GetBody());
		}
		else
			m_LogMessageCtrl.SetText(curRev->GetSubject() + L'\n' + curRev->GetBody());
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
	if (m_bThreadRunning)
	{
		if (CMessageBox::Show(GetSafeHwnd(), IDS_PROC_REBASE_ABORT, IDS_APPNAME, MB_YESNO | MB_ICONQUESTION) != IDYES)
			return;
		m_bAbort = TRUE;
		return;
	}

	if (m_pTaskbarList)
		m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NOPROGRESS);

	m_tooltips.Pop();

	SaveSplitterPos();

	if (m_RebaseStage == CHOOSE_BRANCH || m_RebaseStage== CHOOSE_COMMIT_PICK_MODE)
	{
		__super::OnCancel();
		goto end;
	}

	if (m_OrigUpstreamHash.IsEmpty() || m_OrigHEADHash.IsEmpty())
	{
		__super::OnCancel();
		goto end;
	}

	if (!m_bAbort && CMessageBox::Show(GetSafeHwnd(), IDS_PROC_REBASE_ABORT, IDS_APPNAME, MB_YESNO | MB_ICONQUESTION) != IDYES)
		return;

	m_ctrlTabCtrl.SetActiveTab(REBASE_TAB_LOG);

	if (g_Git.m_IsUseLibGit2 && !m_IsCherryPick)
	{
		CGitHash head;
		if (!g_Git.GetHash(head, L"HEAD"))
			WriteReflog(head, "rebase: begin aborting...");
	}

	if(this->m_IsFastForward)
	{
		CString cmd;
		cmd.Format(L"git.exe reset --hard %s --", static_cast<LPCTSTR>(this->m_OrigBranchHash.ToString()));
		RunGitCmdRetryOrAbort(cmd);
		__super::OnCancel();
		goto end;
	}

	if (m_IsCherryPick) // there are not "branch" at cherry pick mode
	{
		CString cmd;
		cmd.Format(L"git.exe reset --hard %s --", static_cast<LPCTSTR>(m_OrigUpstreamHash.ToString()));
		RunGitCmdRetryOrAbort(cmd);
		__super::OnCancel();
		goto end;
	}

	if (m_OrigHEADBranch == m_BranchCtrl.GetString())
	{
		CString cmd, out;
		if (g_Git.IsLocalBranch(m_OrigHEADBranch))
			cmd.Format(L"git.exe checkout -f -B %s %s --", static_cast<LPCTSTR>(m_BranchCtrl.GetString()), static_cast<LPCTSTR>(m_OrigBranchHash.ToString()));
		else
			cmd.Format(L"git.exe checkout -f %s --", static_cast<LPCTSTR>(m_OrigBranchHash.ToString()));
		if (g_Git.Run(cmd, &out, CP_UTF8))
		{
			AddLogString(out);
			::MessageBox(m_hWnd, L"Unrecoverable error on cleanup:\n" + out, L"TortoiseGit", MB_ICONERROR);
			__super::OnCancel();
			goto end;
		}

		cmd.Format(L"git.exe reset --hard %s --", static_cast<LPCTSTR>(m_OrigBranchHash.ToString()));
		RunGitCmdRetryOrAbort(cmd);
	}
	else
	{
		CString cmd, out;
		if (m_OrigHEADBranch != g_Git.GetCurrentBranch(true))
		{
			if (g_Git.IsLocalBranch(m_OrigHEADBranch))
				cmd.Format(L"git.exe checkout -f -B %s %s --", static_cast<LPCTSTR>(m_OrigHEADBranch), static_cast<LPCTSTR>(m_OrigHEADHash.ToString()));
			else
				cmd.Format(L"git.exe checkout -f %s --", static_cast<LPCTSTR>(m_OrigHEADHash.ToString()));
			if (g_Git.Run(cmd, &out, CP_UTF8))
			{
				AddLogString(out);
				::MessageBox(m_hWnd, L"Unrecoverable error on cleanup:\n" + out, L"TortoiseGit", MB_ICONERROR);
				// continue to restore moved branch
			}
		}

		cmd.Format(L"git.exe reset --hard %s --", static_cast<LPCTSTR>(m_OrigHEADHash.ToString()));
		RunGitCmdRetryOrAbort(cmd);

		// restore moved branch
		if (g_Git.IsLocalBranch(m_BranchCtrl.GetString()))
		{
			cmd.Format(L"git.exe branch -f %s %s --", static_cast<LPCTSTR>(m_BranchCtrl.GetString()), static_cast<LPCTSTR>(m_OrigBranchHash.ToString()));
			if (g_Git.Run(cmd, &out, CP_UTF8))
			{
				AddLogString(out);
				::MessageBox(m_hWnd, L"Unrecoverable error on cleanup:\n" + out, L"TortoiseGit", MB_ICONERROR);
				__super::OnCancel();
				goto end;
			}
		}
	}
	if (g_Git.m_IsUseLibGit2)
		WriteReflog(m_OrigHEADHash, "rebase: aborted");
	__super::OnCancel();
end:
	CleanUpRebaseActiveFolder();
	CheckRestoreStash();
}

void CRebaseDlg::OnBnClickedButtonReverse()
{
	CString temp = m_BranchCtrl.GetString();
	m_BranchCtrl.AddString(m_UpstreamCtrl.GetString());
	m_UpstreamCtrl.AddString(temp);
	OnCbnSelchangeUpstream();
}

void CRebaseDlg::OnBnClickedButtonBrowse()
{
	if (CBrowseRefsDlg::PickRefForCombo(m_UpstreamCtrl))
		OnCbnSelchangeUpstream();
}

void CRebaseDlg::OnBnClickedRebaseCheckForce()
{
	this->UpdateData();
	GetDlgItem(IDC_BUTTON_ADD)->EnableWindow(!m_bPreserveMerges);
	this->FetchLogList();
}

void CRebaseDlg::OnBnClickedRebasePostButton()
{
	this->m_Upstream=this->m_UpstreamCtrl.GetString();
	this->m_Branch=this->m_BranchCtrl.GetString();

	this->EndDialog(static_cast<int>(IDC_REBASE_POST_BUTTON + this->m_PostButton.GetCurrentEntry()));
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
		ListConflictFile(false);
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

void CRebaseDlg::OnBnClickedButtonUp()
{
	POSITION pos;
	pos = m_CommitList.GetFirstSelectedItemPosition();

	bool moveToTop = !!(GetAsyncKeyState(VK_SHIFT) & 0x8000);
	// do nothing if the first selected item is the first item in the list
	if (!moveToTop && m_CommitList.GetNextSelectedItem(pos) == 0)
		return;

	pos = m_CommitList.GetFirstSelectedItemPosition();

	int count = 0;
	bool changed = false;
	while(pos)
	{
		int index=m_CommitList.GetNextSelectedItem(pos);
		count = moveToTop ? count : (index - 1);
		while (index > count)
		{
			std::swap(m_CommitList.m_logEntries[index], m_CommitList.m_logEntries[index - 1]);
			std::swap(m_CommitList.m_arShownList[index], m_CommitList.m_arShownList[index - 1]);
			m_CommitList.SetItemState(index - 1, LVIS_SELECTED, LVIS_SELECTED);
			m_CommitList.SetItemState(index, 0, LVIS_SELECTED);
			changed = true;
			index--;
		}
		count++;
	}
	if (changed)
	{
		pos = m_CommitList.GetFirstSelectedItemPosition();
		m_CommitList.EnsureVisible(m_CommitList.GetNextSelectedItem(pos), false);
		m_CommitList.Invalidate();
		m_CommitList.SetFocus();
	}
}

void CRebaseDlg::OnBnClickedButtonDown()
{
	if (m_CommitList.GetSelectedCount() == 0)
		return;

	bool moveToBottom = !!(GetAsyncKeyState(VK_SHIFT) & 0x8000);
	POSITION pos;
	pos = m_CommitList.GetFirstSelectedItemPosition();
	bool changed = false;
	// use an array to store all selected item indexes; the user won't select too much items
	auto indexes = std::make_unique<int[]>(m_CommitList.GetSelectedCount());
	int i = 0;
	while(pos)
		indexes[i++] = m_CommitList.GetNextSelectedItem(pos);
	// don't move any item if the last selected item is the last item in the m_CommitList
	// (that would change the order of the selected items)
	if (!moveToBottom && indexes[m_CommitList.GetSelectedCount() - 1] >= m_CommitList.GetItemCount() - 1)
		return;
	int count = m_CommitList.GetItemCount() - 1;
	// iterate over the indexes backwards in order to correctly move multiselected items
	for (i = m_CommitList.GetSelectedCount() - 1; i >= 0; i--)
	{
		int index = indexes[i];
		count = moveToBottom ? count : (index + 1);
		while (index < count)
		{
			std::swap(m_CommitList.m_logEntries[index], m_CommitList.m_logEntries[index + 1]);
			std::swap(m_CommitList.m_arShownList[index], m_CommitList.m_arShownList[index + 1]);
			m_CommitList.SetItemState(index, 0, LVIS_SELECTED);
			m_CommitList.SetItemState(index + 1, LVIS_SELECTED, LVIS_SELECTED);
			changed = true;
			index++;
		}
		count--;
	}
	m_CommitList.EnsureVisible(indexes[m_CommitList.GetSelectedCount() - 1] + 1, false);
	if (changed)
	{
		m_CommitList.Invalidate();
		m_CommitList.SetFocus();
	}
}

LRESULT CRebaseDlg::OnCommitsReordered(WPARAM wParam, LPARAM /*lParam*/)
{
	POSITION pos = m_CommitList.GetFirstSelectedItemPosition();
	int first = m_CommitList.GetNextSelectedItem(pos);
	int last = first;
	while (pos)
		last = m_CommitList.GetNextSelectedItem(pos);
	++last;

	for (int i = first; i < last; ++i)
		m_CommitList.SetItemState(i, 0, LVIS_SELECTED);

	int dest = static_cast<int>(wParam);
	if (dest > first)
	{
		std::rotate(m_CommitList.m_logEntries.begin() + first, m_CommitList.m_logEntries.begin() + last, m_CommitList.m_logEntries.begin() + dest);
		std::rotate(m_CommitList.m_arShownList.begin() + first, m_CommitList.m_arShownList.begin() + last, m_CommitList.m_arShownList.begin() + dest);
		for (int i = first + dest - last; i < dest; ++i)
			m_CommitList.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
	}
	else
	{
		std::rotate(m_CommitList.m_logEntries.begin() + dest, m_CommitList.m_logEntries.begin() + first, m_CommitList.m_logEntries.begin() + last);
		std::rotate(m_CommitList.m_arShownList.begin() + dest, m_CommitList.m_arShownList.begin() + first, m_CommitList.m_arShownList.begin() + last);
		for (int i = dest; i < dest + (last - first); ++i)
			m_CommitList.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
	}

	m_CommitList.Invalidate();

	return 0;
}

LRESULT CRebaseDlg::OnTaskbarBtnCreated(WPARAM wParam, LPARAM lParam)
{
	m_pTaskbarList.Release();
	m_pTaskbarList.CoCreateInstance(CLSID_TaskbarList);
	return __super::OnTaskbarButtonCreated(wParam, lParam);
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
		if (pNMLV->iItem == static_cast<int>(m_CommitList.m_arShownList.size()))
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
			FillLogMessageCtrl();
	}
	else
		FillLogMessageCtrl();
}

void CRebaseDlg::FillLogMessageCtrl()
{
	int selCount = m_CommitList.GetSelectedCount();
	if (selCount == 1 && (m_RebaseStage == CHOOSE_BRANCH || m_RebaseStage == CHOOSE_COMMIT_PICK_MODE))
	{
		POSITION pos = m_CommitList.GetFirstSelectedItemPosition();
		int selIndex = m_CommitList.GetNextSelectedItem(pos);
		GitRevLoglist* pLogEntry = m_CommitList.m_arShownList.SafeGetAt(selIndex);
		m_FileListCtrl.UpdateWithGitPathList(pLogEntry->GetFiles(&m_CommitList));
		m_FileListCtrl.m_CurrentVersion = pLogEntry->m_CommitHash;
		m_FileListCtrl.Show(GITSLC_SHOWVERSIONED);
		m_LogMessageCtrl.Call(SCI_SETREADONLY, FALSE);
		m_LogMessageCtrl.SetText(pLogEntry->GetSubject() + L'\n' + pLogEntry->GetBody());
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
		GitRevLoglist* pRev = m_CommitList.m_arShownList.SafeGetAt(m_CurrentRebaseIndex);
		int mode = pRev->GetRebaseAction() & CGitLogListBase::LOGACTIONS_REBASE_MODE_MASK;
		if (mode == CGitLogListBase::LOGACTIONS_REBASE_SKIP)
		{
			if (!RunGitCmdRetryOrAbort(L"git.exe reset --hard"))
			{
				m_FileListCtrl.Clear();
				m_RebaseStage = REBASE_CONTINUE;
				UpdateCurrentStatus();
			}
		}
	}
	return 0;
}


void CRebaseDlg::OnBnClickedSplitAllOptions()
{
	switch (m_SplitAllOptions.GetCurrentEntry())
	{
	case 0:
		SetAllRebaseAction(CGitLogListBase::LOGACTIONS_REBASE_PICK);
		break;
	case 1:
		SetAllRebaseAction(CGitLogListBase::LOGACTIONS_REBASE_SQUASH);
		break;
	case 2:
		SetAllRebaseAction(CGitLogListBase::LOGACTIONS_REBASE_EDIT);
		break;
	case 3:
		m_CommitList.SetUnselectedRebaseAction(CGitLogListBase::LOGACTIONS_REBASE_SKIP);
		break;
	case 4:
		m_CommitList.SetUnselectedRebaseAction(CGitLogListBase::LOGACTIONS_REBASE_SQUASH);
		break;
	case 5:
		m_CommitList.SetUnselectedRebaseAction(CGitLogListBase::LOGACTIONS_REBASE_EDIT);
		break;
	default:
		ATLASSERT(false);
	}
}

void CRebaseDlg::OnBnClickedRebaseSplitCommit()
{
	UpdateData();
}

static bool GetCompareHash(const CString& ref, const CGitHash& hash)
{
	CGitHash refHash;
	if (g_Git.GetHash(refHash, ref))
		MessageBox(nullptr, g_Git.GetGitLastErr(L"Could not get hash of \"" + ref + L"\"."), L"TortoiseGit", MB_ICONERROR);
	return refHash.IsEmpty() || (hash == refHash);
}

void CRebaseDlg::OnBnClickedButtonOnto()
{
	m_Onto = CBrowseRefsDlg::PickRef(false, m_Onto);
	if (!m_Onto.IsEmpty())
	{
		// make sure that the user did not select upstream, selected branch or HEAD
		CGitHash hash;
		if (g_Git.GetHash(hash, m_Onto))
		{
			MessageBox(g_Git.GetGitLastErr(L"Could not get hash of \"" + m_BranchCtrl.GetString() + L"\"."), L"TortoiseGit", MB_ICONERROR);
			m_Onto.Empty();
			static_cast<CButton*>(GetDlgItem(IDC_BUTTON_ONTO))->SetCheck(m_Onto.IsEmpty() ? BST_UNCHECKED : BST_CHECKED);
			return;
		}
		if (GetCompareHash(L"HEAD", hash) || GetCompareHash(m_UpstreamCtrl.GetString(), hash) || GetCompareHash(m_BranchCtrl.GetString(), hash))
			m_Onto.Empty();
	}
	if (m_Onto.IsEmpty())
		m_tooltips.DelTool(IDC_BUTTON_ONTO);
	else
		m_tooltips.AddTool(IDC_BUTTON_ONTO, m_Onto);
	static_cast<CButton*>(GetDlgItem(IDC_BUTTON_ONTO))->SetCheck(m_Onto.IsEmpty() ? BST_UNCHECKED : BST_CHECKED);
	FetchLogList();
}

void CRebaseDlg::OnHelp()
{
	HtmlHelp(0x20000 + (m_IsCherryPick ? IDD_REBASECHERRYPICK : IDD_REBASE));
}

int	CRebaseDlg::RunGitCmdRetryOrAbort(const CString& cmd)
{
	while (true)
	{
		CString out;
		if (g_Git.Run(cmd, &out, CP_UTF8))
		{
			AddLogString(cmd);
			AddLogString(CString(MAKEINTRESOURCE(IDS_FAIL)));
			AddLogString(out);
			CString msg;
			msg.Format(L"\"%s\" failed.\n%s", static_cast<LPCTSTR>(cmd), static_cast<LPCTSTR>(out));
			if (CMessageBox::Show(GetSafeHwnd(), msg, L"TortoiseGit", 1, IDI_ERROR, CString(MAKEINTRESOURCE(IDS_MSGBOX_RETRY)), CString(MAKEINTRESOURCE(IDS_MSGBOX_ABORT))) != 1)
				return -1;
		}
		else
			return 0;
	}
}

LRESULT CRebaseDlg::OnThemeChanged()
{
	CMFCVisualManager::GetInstance()->DestroyInstance();
	return 0;
}

void CRebaseDlg::OnSysColorChange()
{
	__super::OnSysColorChange();
	m_LogMessageCtrl.SetColors(true);
	m_LogMessageCtrl.SetFont(CAppUtils::GetLogFontName(), CAppUtils::GetLogFontSize());
	m_wndOutputRebase.SetColors(true);
	m_wndOutputRebase.SetFont(CAppUtils::GetLogFontName(), CAppUtils::GetLogFontSize());
}

void CRebaseDlg::OnBnClickedButtonAdd()
{
	CLogDlg dlg;
	// tell the dialog to use mode for selecting revisions
	dlg.SetSelect(true);
	// allow multi-select
	dlg.SingleSelection(false);
	if (dlg.DoModal() != IDOK || dlg.GetSelectedHash().empty())
		return;

	auto selectedHashes = dlg.GetSelectedHash();
	for (auto it = selectedHashes.crbegin(); it != selectedHashes.crend(); ++it)
	{
		GitRevLoglist* pRev = m_CommitList.m_logEntries.m_pLogCache->GetCacheData(*it);
		if (pRev->GetCommit(it->ToString()))
			return;
		if (pRev->GetParentFromHash(pRev->m_CommitHash))
			return;
		pRev->GetRebaseAction() = CGitLogListBase::LOGACTIONS_REBASE_PICK;
		if (m_CommitList.m_IsOldFirst)
		{
			m_CommitList.m_logEntries.push_back(pRev->m_CommitHash);
			m_CommitList.m_arShownList.SafeAdd(pRev);
		}
		else
		{
			m_CommitList.m_logEntries.insert(m_CommitList.m_logEntries.cbegin(), pRev->m_CommitHash);
			m_CommitList.m_arShownList.SafeAddFront(pRev);
		}
	}
	m_CommitList.SetItemCountEx(static_cast<int>(m_CommitList.m_logEntries.size()));
	m_CommitList.Invalidate();

	if (m_CommitList.m_IsOldFirst)
		m_CurrentRebaseIndex = -1;
	else
		m_CurrentRebaseIndex = static_cast<int>(m_CommitList.m_logEntries.size());
}
