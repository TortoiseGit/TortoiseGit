﻿// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2014 - TortoiseSVN
// Copyright (C) 2008-2025 - TortoiseGit

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

#include "stdafx.h"
#include "TortoiseProc.h"
#include "CommitDlg.h"
#include "MessageBox.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "Git.h"
#include "HistoryDlg.h"
#include "Hooks.h"
#include "UnicodeUtils.h"
#include "../TGitCache/CacheInterface.h"
#include "ProgressDlg.h"
#include "ShellUpdater.h"
#include "COMError.h"
#include "Globals.h"
#include "SysProgressDlg.h"
#include "MassiveGitTask.h"
#include "LogDlg.h"
#include "BstrSafeVector.h"
#include "StringUtils.h"
#include "FileTextLines.h"
#include "DPIAware.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

UINT CCommitDlg::WM_AUTOLISTREADY = RegisterWindowMessage(L"TORTOISEGIT_AUTOLISTREADY_MSG");
UINT CCommitDlg::WM_UPDATEOKBUTTON = RegisterWindowMessage(L"TORTOISEGIT_COMMIT_UPDATEOKBUTTON");
UINT CCommitDlg::WM_UPDATEDATAFALSE = RegisterWindowMessage(L"TORTOISEGIT_COMMIT_UPDATEDATAFALSE");
UINT CCommitDlg::WM_PARTIALSTAGINGREFRESHPATCHVIEW = RegisterWindowMessage(L"TORTOISEGIT_COMMIT_PARTIALSTAGINGREFRESHPATCHVIEW"); // same string in PatchViewDlg.cpp!!!

IMPLEMENT_DYNAMIC(CCommitDlg, CResizableStandAloneDialog)
CCommitDlg::CCommitDlg(CWnd* pParent /*=nullptr*/)
	: CResizableStandAloneDialog(CCommitDlg::IDD, pParent)
	, m_bShowUnversioned(FALSE)
	, m_bWholeProject(FALSE)
	, m_bWholeProject2(FALSE)
	, m_bDoNotAutoselectSubmodules(FALSE)
	, m_bSetCommitDateTime(FALSE)
	, m_bCreateNewBranch(FALSE)
	, m_bCommitMessageOnly(FALSE)
	, m_bSetAuthor(FALSE)
	, m_bStagingSupport(FALSE)
	, m_bAmendDiffToLastCommit(FALSE)
	, m_bCommitAmend(FALSE)
{
}

CCommitDlg::~CCommitDlg()
{
	if (m_hAccelOkButton)
		DestroyAcceleratorTable(m_hAccelOkButton);
	delete m_pThread;
}

void CCommitDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FILELIST, m_ListCtrl);
	DDX_Control(pDX, IDC_LOGMESSAGE, m_cLogMessage);
	DDX_Check(pDX, IDC_SHOWUNVERSIONED, m_bShowUnversioned);
	DDX_Check(pDX, IDC_COMMIT_SETDATETIME, m_bSetCommitDateTime);
	DDX_Check(pDX, IDC_CHECK_NEWBRANCH, m_bCreateNewBranch);
	DDX_Text(pDX, IDC_COMMIT_AUTHORDATA, m_sAuthor);
	DDX_Check(pDX, IDC_WHOLE_PROJECT, m_bWholeProject);
	DDX_Control(pDX, IDC_SPLITTER, m_wndSplitter);
	DDX_Check(pDX, IDC_NOAUTOSELECTSUBMODULES, m_bDoNotAutoselectSubmodules);
	DDX_Check(pDX,IDC_COMMIT_AMEND,m_bCommitAmend);
	DDX_Check(pDX, IDC_COMMIT_MESSAGEONLY, m_bCommitMessageOnly);
	DDX_Check(pDX,IDC_COMMIT_AMENDDIFF,m_bAmendDiffToLastCommit);
	DDX_Check(pDX, IDC_COMMIT_SETAUTHOR, m_bSetAuthor);
	DDX_Check(pDX, IDC_STAGINGSUPPORT, m_bStagingSupport);
	DDX_Control(pDX,IDC_VIEW_PATCH,m_ctrlShowPatch);
	DDX_Control(pDX, IDC_PARTIAL_STAGING, m_ctrlPartialStaging);
	DDX_Control(pDX, IDC_PARTIAL_UNSTAGING, m_ctrlPartialUnstaging);
	DDX_Control(pDX, IDC_COMMIT_DATEPICKER, m_CommitDate);
	DDX_Control(pDX, IDC_COMMIT_TIMEPICKER, m_CommitTime);
	DDX_Control(pDX, IDC_COMMIT_AS_COMMIT_DATE, m_AsCommitDateCtrl);
	DDX_Control(pDX, IDC_CHECKALL, m_CheckAll);
	DDX_Control(pDX, IDC_CHECKNONE, m_CheckNone);
	DDX_Control(pDX, IDC_CHECKUNVERSIONED, m_CheckUnversioned);
	DDX_Control(pDX, IDC_CHECKVERSIONED, m_CheckVersioned);
	DDX_Control(pDX, IDC_CHECKADDED, m_CheckAdded);
	DDX_Control(pDX, IDC_CHECKDELETED, m_CheckDeleted);
	DDX_Control(pDX, IDC_CHECKMODIFIED, m_CheckModified);
	DDX_Control(pDX, IDC_CHECKFILES, m_CheckFiles);
	DDX_Control(pDX, IDC_CHECKSUBMODULES, m_CheckSubmodules);
	DDX_Control(pDX, IDOK, m_ctrlOkButton);
}

BEGIN_MESSAGE_MAP(CCommitDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_SHOWUNVERSIONED, OnBnClickedShowunversioned)
	ON_NOTIFY(SCN_UPDATEUI, IDC_LOGMESSAGE, OnScnUpdateUI)
//	ON_BN_CLICKED(IDC_HISTORY, OnBnClickedHistory)
	ON_BN_CLICKED(IDC_BUGTRAQBUTTON, OnBnClickedBugtraqbutton)
	ON_EN_CHANGE(IDC_LOGMESSAGE, OnEnChangeLogmessage)
	ON_REGISTERED_MESSAGE(CGitStatusListCtrl::GITSLNM_ITEMCOUNTCHANGED, OnGitStatusListCtrlItemCountChanged)
	ON_REGISTERED_MESSAGE(CGitStatusListCtrl::GITSLNM_NEEDSREFRESH, OnGitStatusListCtrlNeedsRefresh)
	ON_REGISTERED_MESSAGE(CGitStatusListCtrl::GITSLNM_ADDFILE, OnFileDropped)
	ON_REGISTERED_MESSAGE(CGitStatusListCtrl::GITSLNM_CHECKCHANGED, &CCommitDlg::OnGitStatusListCtrlCheckChanged)
	ON_REGISTERED_MESSAGE(CGitStatusListCtrl::GITSLNM_ITEMCHANGED, &CCommitDlg::OnGitStatusListCtrlItemChanged)

	ON_REGISTERED_MESSAGE(CLinkControl::LK_LINKITEMCLICKED, &CCommitDlg::OnCheck)
	ON_REGISTERED_MESSAGE(WM_AUTOLISTREADY, OnAutoListReady)
	ON_REGISTERED_MESSAGE(WM_UPDATEOKBUTTON, OnUpdateOKButton)
	ON_REGISTERED_MESSAGE(WM_UPDATEDATAFALSE, OnUpdateDataFalse)
	ON_REGISTERED_MESSAGE(WM_PARTIALSTAGINGREFRESHPATCHVIEW, OnPartialStagingRefreshPatchView)
	ON_WM_TIMER()
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_SIGNOFF, &CCommitDlg::OnBnClickedSignOff)
	ON_BN_CLICKED(IDC_COMMIT_AMEND, &CCommitDlg::OnBnClickedCommitAmend)
	ON_BN_CLICKED(IDC_COMMIT_MESSAGEONLY, &CCommitDlg::OnBnClickedCommitMessageOnly)
	ON_BN_CLICKED(IDC_WHOLE_PROJECT, &CCommitDlg::OnBnClickedWholeProject)
	ON_COMMAND(ID_FOCUS_MESSAGE,&CCommitDlg::OnFocusMessage)
	ON_COMMAND(ID_FOCUS_FILELIST, OnFocusFileList)
	ON_STN_CLICKED(IDC_VIEW_PATCH, &CCommitDlg::OnStnClickedViewPatch)
	ON_STN_CLICKED(IDC_PARTIAL_STAGING, &CCommitDlg::OnStnClickedPartialStaging)
	ON_STN_CLICKED(IDC_PARTIAL_UNSTAGING, &CCommitDlg::OnStnClickedPartialUnstaging)
	ON_WM_MOVE()
	ON_WM_MOVING()
	ON_WM_SIZING()
	ON_NOTIFY(HDN_ITEMCHANGED, 0, &CCommitDlg::OnHdnItemchangedFilelist)
	ON_BN_CLICKED(IDC_COMMIT_AMENDDIFF, &CCommitDlg::OnBnClickedCommitAmenddiff)
	ON_BN_CLICKED(IDC_NOAUTOSELECTSUBMODULES, &CCommitDlg::OnBnClickedNoautoselectsubmodules)
	ON_BN_CLICKED(IDC_COMMIT_SETDATETIME, &CCommitDlg::OnBnClickedCommitSetDateTime)
	ON_BN_CLICKED(IDC_COMMIT_AS_COMMIT_DATE, &CCommitDlg::OnBnClickedCommitAsCommitDate)
	ON_BN_CLICKED(IDC_CHECK_NEWBRANCH, &CCommitDlg::OnBnClickedCheckNewBranch)
	ON_BN_CLICKED(IDC_COMMIT_SETAUTHOR, &CCommitDlg::OnBnClickedCommitSetauthor)
	ON_BN_CLICKED(IDC_STAGINGSUPPORT, &CCommitDlg::OnBnClickedStagingSupport)
END_MESSAGE_MAP()

static int GetCommitTemplate(CString &msg)
{
	msg.Empty();
	CString tplFilename = g_Git.GetConfigValue(L"commit.template");
	if (tplFilename.IsEmpty())
		return -1;

	if (tplFilename[0] == L'/')
	{
		if (tplFilename.GetLength() >= 3)
		{
			// handle "/d/TortoiseGit/tpl.txt" -> "d:/TortoiseGit/tpl.txt"
			if (tplFilename[2] == L'/')
			{
				LPWSTR buf = tplFilename.GetBuffer();
				buf[0] = buf[1];
				buf[1] = L':';
				tplFilename.ReleaseBuffer();
			}
		}
	}
	else if (CStringUtils::StartsWith(tplFilename, L"~/"))
		tplFilename = g_Git.GetHomeDirectory() + tplFilename.Mid(static_cast<int>(wcslen(L"~")));

	tplFilename.Replace(L'/', L'\\');

	if (!CGit::LoadTextFile(tplFilename, msg))
	{
		MessageBox(nullptr, L"Could not open and load commit.template file: " + tplFilename, L"TortoiseGit", MB_ICONERROR);
		return -1;
	}
	return 0;
}

BOOL CCommitDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	GetCommitTemplate(m_sLogTemplate);
	if (m_sLogMessage.IsEmpty())
	{
		if (!m_bForceCommitAmend)
			m_sLogMessage = m_sLogTemplate;

		CString dotGitPath;
		GitAdminDir::GetWorktreeAdminDirPath(g_Git.m_CurrentDir, dotGitPath);
		CGit::LoadTextFile(dotGitPath + L"SQUASH_MSG", m_sLogMessage);
		CGit::LoadTextFile(dotGitPath + L"MERGE_MSG", m_sLogMessage);
	}

	m_ProjectProperties.ReadProps();

	if (!RunStartCommitHook())
	{
		EndDialog(IDCANCEL);
		return FALSE;
	}

	m_regAddBeforeCommit = CRegDWORD(L"Software\\TortoiseGit\\AddBeforeCommit", TRUE);
	m_bShowUnversioned = m_regAddBeforeCommit;

	CString regPath(g_Git.m_CurrentDir);
	regPath.Replace(L':', L'_');
	m_regShowWholeProject = CRegDWORD(L"Software\\TortoiseGit\\TortoiseProc\\ShowWholeProject\\" + regPath, FALSE);
	m_bWholeProject = m_regShowWholeProject;

	m_History.SetMaxHistoryItems(CRegDWORD(L"Software\\TortoiseGit\\MaxHistoryItems", 25));

	m_regDoNotAutoselectSubmodules = CRegDWORD(L"Software\\TortoiseGit\\DoNotAutoselectSubmodules", FALSE);
	m_bDoNotAutoselectSubmodules = m_regDoNotAutoselectSubmodules;

	m_hAccel = LoadAccelerators(AfxGetResourceHandle(),MAKEINTRESOURCE(IDR_ACC_COMMITDLG));

	if (m_pathList.IsEmpty())
		m_bWholeProject2 = true;

	if(this->m_pathList.GetCount() == 1 && m_pathList[0].IsEmpty())
		m_bWholeProject2 = true;

	SetDlgTitle();

	// git commit accepts only 1970-01-01 to 2099-12-31 regardless timezone
	COleDateTime minDate(1970, 1, 1, 0, 0, 0), maxDate(2099, 12, 31, 0, 0, 0);
	m_CommitDate.SetRange(&minDate, &maxDate);
	if (m_bSetCommitDateTime)
	{
		m_CommitDate.SetTime(&m_wantCommitTime);
		m_CommitTime.SetTime(&m_wantCommitTime);
		GetDlgItem(IDC_COMMIT_DATEPICKER)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_COMMIT_TIMEPICKER)->ShowWindow(SW_SHOW);
	}

	UpdateData(FALSE);

	m_ListCtrl.Init(GITSLC_COLEXT | GITSLC_COLSTATUS | GITSLC_COLADD | GITSLC_COLDEL, L"CommitDlg", (GITSLC_POPALL ^ (GITSLC_POPCOMMIT | GITSLC_POPSAVEAS | GITSLC_POPPREPAREDIFF)), true, true);
	m_ListCtrl.SetStatLabel(GetDlgItem(IDC_STATISTICS));
	m_ListCtrl.SetCancelBool(&m_bCancelled);
	m_ListCtrl.SetEmptyString(IDS_COMMITDLG_NOTHINGTOCOMMIT);
	m_ListCtrl.EnableFileDrop();
	m_ListCtrl.SetBackgroundImage(IDI_COMMIT_BKG);

	//this->DialogEnableWindow(IDC_COMMIT_AMEND,FALSE);

	m_cLogMessage.Init(m_ProjectProperties);
	m_cLogMessage.SetFont(CAppUtils::GetLogFontName(), CAppUtils::GetLogFontSize());
	m_cLogMessage.RegisterContextMenuHandler(this);
	std::map<int, UINT> icons;
	icons[AUTOCOMPLETE_SPELLING] = IDI_SPELL;
	icons[AUTOCOMPLETE_FILENAME] = IDI_FILE;
	icons[AUTOCOMPLETE_PROGRAMCODE] = IDI_CODE;
	icons[AUTOCOMPLETE_SNIPPET] = IDI_SNIPPET;
	m_cLogMessage.SetIcon(icons);

	OnEnChangeLogmessage();

	m_tooltips.AddTool(IDC_COMMIT_AMEND,IDS_COMMIT_AMEND_TT);
	m_tooltips.AddTool(IDC_MERGEACTIVE, IDC_MERGEACTIVE_TT);
	m_tooltips.AddTool(IDC_COMMIT_MESSAGEONLY, IDS_COMMIT_MESSAGEONLY_TT);
	m_tooltips.AddTool(IDC_COMMIT_AS_COMMIT_DATE, IDS_COMMIT_AS_COMMIT_DATE_TT);

	CBugTraqAssociations bugtraq_associations;
	bugtraq_associations.Load(m_ProjectProperties.GetProviderUUID(), m_ProjectProperties.sProviderParams);

	if (bugtraq_associations.FindProvider(g_Git.m_CurrentDir, &m_bugtraq_association))
	{
		CComPtr<IBugTraqProvider> pProvider;
		HRESULT hr = pProvider.CoCreateInstance(m_bugtraq_association.GetProviderClass());
		if (SUCCEEDED(hr))
		{
			m_BugTraqProvider = pProvider;
			ATL::CComBSTR temp;
			ATL::CComBSTR parameters(m_bugtraq_association.GetParameters());
			if (SUCCEEDED(hr = pProvider->GetLinkText(GetSafeHwnd(), parameters, &temp)))
			{
				SetDlgItemText(IDC_BUGTRAQBUTTON, temp);
				GetDlgItem(IDC_BUGTRAQBUTTON)->EnableWindow(TRUE);
				GetDlgItem(IDC_BUGTRAQBUTTON)->ShowWindow(SW_SHOW);
			}
		}
	}
	else
	{
		GetDlgItem(IDC_BUGTRAQBUTTON)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_BUGTRAQBUTTON)->EnableWindow(FALSE);
	}
	if (!m_ProjectProperties.sMessage.IsEmpty())
	{
		GetDlgItem(IDC_BUGID)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_BUGIDLABEL)->ShowWindow(SW_SHOW);
		if (!m_ProjectProperties.sLabel.IsEmpty())
			SetDlgItemText(IDC_BUGIDLABEL, m_ProjectProperties.sLabel);
		GetDlgItem(IDC_BUGID)->SetFocus();
		CString sBugID = m_ProjectProperties.GetBugIDFromLog(m_sLogMessage);
		if (!sBugID.IsEmpty())
		{
			SetDlgItemText(IDC_BUGID, sBugID);
		}
	}
	else
	{
		GetDlgItem(IDC_BUGID)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_BUGIDLABEL)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_LOGMESSAGE)->SetFocus();
	}

	if (!m_sLogMessage.IsEmpty())
	{
		m_cLogMessage.SetText(m_sLogMessage);
		m_cLogMessage.Call(SCI_SETCURRENTPOS, 0);
		m_cLogMessage.Call(SCI_SETSEL, 0, 0);
	}

	GetWindowText(m_sWindowTitle);

	AdjustControlSize(IDC_SHOWUNVERSIONED);
	AdjustControlSize(IDC_WHOLE_PROJECT);
	AdjustControlSize(IDC_CHECK_NEWBRANCH);
	AdjustControlSize(IDC_COMMIT_AMEND);
	AdjustControlSize(IDC_COMMIT_MESSAGEONLY);
	AdjustControlSize(IDC_COMMIT_AMENDDIFF);
	AdjustControlSize(IDC_COMMIT_SETDATETIME);
	AdjustControlSize(IDC_COMMIT_SETAUTHOR);
	AdjustControlSize(IDC_NOAUTOSELECTSUBMODULES);
	AdjustControlSize(IDC_COMMIT_AS_COMMIT_DATE);
	AdjustControlSize(IDC_STAGINGSUPPORT);

	// line up all controls and adjust their sizes.
#define LINKSPACING 9
	RECT rc = AdjustControlSize(IDC_SELECTLABEL);
	rc.right -= 15;	// AdjustControlSize() adds 20 pixels for the checkbox/radio button bitmap, but this is a label...
	rc = AdjustStaticSize(IDC_CHECKALL, rc, LINKSPACING);
	rc = AdjustStaticSize(IDC_CHECKNONE, rc, LINKSPACING);
	rc = AdjustStaticSize(IDC_CHECKUNVERSIONED, rc, LINKSPACING);
	rc = AdjustStaticSize(IDC_CHECKVERSIONED, rc, LINKSPACING);
	rc = AdjustStaticSize(IDC_CHECKADDED, rc, LINKSPACING);
	rc = AdjustStaticSize(IDC_CHECKDELETED, rc, LINKSPACING);
	rc = AdjustStaticSize(IDC_CHECKMODIFIED, rc, LINKSPACING);
	rc = AdjustStaticSize(IDC_CHECKFILES, rc, LINKSPACING);
	rc = AdjustStaticSize(IDC_CHECKSUBMODULES, rc, LINKSPACING);

	GetClientRect(m_DlgOrigRect);
	m_cLogMessage.GetClientRect(m_LogMsgOrigRect);

	AddAnchor(IDC_COMMITLABEL, TOP_LEFT, ANCHOR(94, 0));
	AddAnchor(IDC_BUGIDLABEL, TOP_RIGHT);
	AddAnchor(IDC_BUGID, TOP_RIGHT);
	AddAnchor(IDC_BUGTRAQBUTTON, TOP_RIGHT);
	AddAnchor(IDC_COMMIT_TO, ANCHOR(6, 0), TOP_RIGHT);
	AddAnchor(IDC_CHECK_NEWBRANCH, TOP_RIGHT);
	AddAnchor(IDC_NEWBRANCH, ANCHOR(6,0), TOP_RIGHT);
	AddAnchor(IDC_MESSAGEGROUP, TOP_LEFT, TOP_RIGHT);
//	AddAnchor(IDC_HISTORY, TOP_LEFT);
	AddAnchor(IDC_LOGMESSAGE, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_SIGNOFF, TOP_RIGHT);
	AddAnchor(IDC_VIEW_PATCH, BOTTOM_RIGHT);
	AddAnchor(IDC_PARTIAL_STAGING, BOTTOM_RIGHT);
	AddAnchor(IDC_PARTIAL_UNSTAGING, BOTTOM_RIGHT);
	AddAnchor(IDC_LISTGROUP, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SPLITTER, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_FILELIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SHOWUNVERSIONED, BOTTOM_LEFT);
	AddAnchor(IDC_STATISTICS, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_TEXT_INFO, TOP_RIGHT);
	AddAnchor(IDC_WHOLE_PROJECT, BOTTOM_LEFT);
	AddAnchor(IDC_NOAUTOSELECTSUBMODULES, BOTTOM_LEFT);
	AddAnchor(IDC_STAGINGSUPPORT, BOTTOM_LEFT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	AddAnchor(IDC_MERGEACTIVE, BOTTOM_RIGHT);
	AddAnchor(IDC_COMMIT_AMEND,TOP_LEFT);
	AddAnchor(IDC_COMMIT_MESSAGEONLY, BOTTOM_LEFT);
	AddAnchor(IDC_COMMIT_AMENDDIFF,TOP_LEFT);
	AddAnchor(IDC_COMMIT_SETDATETIME,TOP_LEFT);
	AddAnchor(IDC_COMMIT_DATEPICKER,TOP_LEFT);
	AddAnchor(IDC_COMMIT_TIMEPICKER,TOP_LEFT);
	AddAnchor(IDC_COMMIT_AS_COMMIT_DATE, TOP_LEFT);
	AddAnchor(IDC_COMMIT_SETAUTHOR, TOP_LEFT);
	AddAnchor(IDC_COMMIT_AUTHORDATA, TOP_LEFT, TOP_RIGHT);

	AddAnchor(IDC_SELECTLABEL, TOP_LEFT);
	AddAnchor(IDC_CHECKALL, TOP_LEFT);
	AddAnchor(IDC_CHECKNONE, TOP_LEFT);
	AddAnchor(IDC_CHECKUNVERSIONED, TOP_LEFT);
	AddAnchor(IDC_CHECKVERSIONED, TOP_LEFT);
	AddAnchor(IDC_CHECKADDED, TOP_LEFT);
	AddAnchor(IDC_CHECKDELETED, TOP_LEFT);
	AddAnchor(IDC_CHECKMODIFIED, TOP_LEFT);
	AddAnchor(IDC_CHECKFILES, TOP_LEFT);
	AddAnchor(IDC_CHECKSUBMODULES, TOP_LEFT);

	if (GetExplorerHWND())
		CenterWindow(CWnd::FromHandle(GetExplorerHWND()));
	EnableSaveRestore(L"CommitDlg");
	DWORD yPos = CDPIAware::Instance().ScaleY(GetSafeHwnd(), CRegDWORD(L"Software\\TortoiseGit\\TortoiseProc\\ResizableState\\CommitDlgSizer"));
	RECT rcDlg, rcLogMsg, rcFileList;
	GetClientRect(&rcDlg);
	m_cLogMessage.GetWindowRect(&rcLogMsg);
	ScreenToClient(&rcLogMsg);
	m_ListCtrl.GetWindowRect(&rcFileList);
	ScreenToClient(&rcFileList);
	if (yPos)
	{
		RECT rectSplitter;
		m_wndSplitter.GetWindowRect(&rectSplitter);
		ScreenToClient(&rectSplitter);
		const int delta = yPos - rectSplitter.top;
		if ((rcLogMsg.bottom + delta > rcLogMsg.top) && (rcLogMsg.bottom + delta < rcFileList.bottom - CDPIAware::Instance().ScaleY(GetSafeHwnd(), 30)))
		{
			m_wndSplitter.SetWindowPos(nullptr, rectSplitter.left, yPos, 0, 0, SWP_NOSIZE);
			DoSize(delta);
			Invalidate();
		}
	}

	SetSplitterRange();

	if (m_bForceCommitAmend || m_bCommitAmend)
	{
		DialogEnableWindow(IDC_COMMIT_AMENDDIFF, TRUE);
		GetDlgItem(IDC_COMMIT_AMENDDIFF)->ShowWindow(SW_SHOW);
	}

	// add all directories to the watcher
	/*
	for (int i=0; i<m_pathList.GetCount(); ++i)
	{
		if (m_pathList[i].IsDirectory())
			m_pathwatcher.AddPath(m_pathList[i]);
	}*/

	this->m_ctrlShowPatch.SetURL(CString());
	this->m_ctrlPartialStaging.SetURL(CString());
	this->m_ctrlPartialUnstaging.SetURL(CString());
	if (g_Git.GetConfigValueBool(L"tgit.commitstagingsupport"))
	{
		m_bStagingSupport = true;
		UpdateData(false);
		PrepareStagingSupport();
	}
	if (g_Git.GetConfigValueBool(L"tgit.commitshowpatch"))
	{
		if (m_bStagingSupport)
			OnStnClickedPartialStaging();
		else
			OnStnClickedViewPatch();
	}

	StartStatusThread();
	CRegDWORD err = CRegDWORD(L"Software\\TortoiseGit\\ErrorOccurred", FALSE);
	CRegDWORD historyhint = CRegDWORD(L"Software\\TortoiseGit\\HistoryHintShown", FALSE);
	if (static_cast<DWORD>(err) != FALSE && static_cast<DWORD>(historyhint) == FALSE)
	{
		historyhint = TRUE;
//		ShowBalloon(IDC_HISTORY, IDS_COMMITDLG_HISTORYHINT_TT, IDI_INFORMATION);
	}
	err = FALSE;

	if (CTGitPath(g_Git.m_CurrentDir).IsMergeActive())
	{
		DialogEnableWindow(IDC_CHECK_NEWBRANCH, FALSE);
		m_bCreateNewBranch = FALSE;
		GetDlgItem(IDC_MERGEACTIVE)->ShowWindow(SW_SHOW);
		CMessageBox::ShowCheck(GetSafeHwnd(), IDS_COMMIT_MERGE_HINT, IDS_APPNAME, MB_ICONINFORMATION, L"CommitMergeHint", IDS_MSGBOX_DONOTSHOWAGAIN);
	}

	PrepareOkButton();

	SetTheme(CTheme::Instance().IsDarkTheme());

	return FALSE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CCommitDlg::PrepareOkButton()
{
	if (m_bNoPostActions)
		return;
	m_regLastAction = CRegDWORD(L"Software\\TortoiseGit\\CommitLastAction", 0);
	int i = 0;
	for (auto labelId : { IDS_COMMIT_COMMIT, IDS_COMMIT_RECOMMIT, IDS_COMMIT_COMMITPUSH })
	{
		++i;
		CString label;
		label.LoadString(labelId);
		m_ctrlOkButton.AddEntry(label);
		wchar_t accellerator = CStringUtils::GetAccellerator(label);
		if (accellerator == L'\0')
			continue;
		++m_accellerators[accellerator].cnt;
		if (m_accellerators[accellerator].cnt > 1)
			m_accellerators[accellerator].id = -1;
		else
			m_accellerators[accellerator].id = i - 1;
	}
	m_ctrlOkButton.SetCurrentEntry(m_regLastAction);
	if (m_accellerators.size())
	{
		auto lpaccelNew = static_cast<LPACCEL>(LocalAlloc(LPTR, m_accellerators.size() * sizeof(ACCEL)));
		if (!lpaccelNew)
			return;
		SCOPE_EXIT{ LocalFree(lpaccelNew); };
		i = 0;
		for (auto& entry : m_accellerators)
		{
			lpaccelNew[i].cmd = static_cast<WORD>(WM_USER + 1 + entry.second.id);
			lpaccelNew[i].fVirt = FVIRTKEY | FALT;
			lpaccelNew[i].key = entry.first;
			entry.second.wmid = lpaccelNew[i].cmd;
			++i;
		}
		m_hAccelOkButton = CreateAcceleratorTable(lpaccelNew, static_cast<int>(m_accellerators.size()));
	}
}

static bool UpdateIndex(CMassiveGitTask &mgt, CSysProgressDlg &sysProgressDlg, int progress, int maxProgress)
{
	if (mgt.IsListEmpty())
		return true;

	if (sysProgressDlg.HasUserCancelled())
		return false;

	if (sysProgressDlg.IsVisible())
	{
		sysProgressDlg.SetTitle(IDS_APPNAME);
		sysProgressDlg.SetLine(1, CString(MAKEINTRESOURCE(IDS_PROC_COMMIT_PREPARECOMMIT)));
		sysProgressDlg.SetLine(2, CString(MAKEINTRESOURCE(IDS_PROC_COMMIT_UPDATEINDEX)));
		sysProgressDlg.SetProgress(progress, maxProgress);
		AfxGetThread()->PumpMessage(); // process messages, in order to avoid freezing
	}

	BOOL cancel = FALSE;
	return mgt.Execute(cancel);
}

static void DoPush(HWND hWnd, bool usePushDlg)
{
	CString head;
	if (g_Git.GetCurrentBranchFromFile(g_Git.m_CurrentDir, head))
		return;
	CString remote, remotebranch;
	g_Git.GetRemotePushBranch(head, remote, remotebranch);
	if (usePushDlg || remote.IsEmpty() || remotebranch.IsEmpty())
	{
		CAppUtils::Push(hWnd);
		return;
	}

	CAppUtils::DoPush(hWnd, CAppUtils::IsSSHPutty(), false, false, false, false, false, head, remote, remotebranch, false, 0, L"");
}

void CCommitDlg::OnOK()
{
	if (m_bBlock)
		return;
	if (m_bThreadRunning)
	{
		m_bCancelled = true;
		StopStatusThread();
	}
	this->UpdateData();

	auto locker(m_ListCtrl.AcquireReadLock());

	if (m_ListCtrl.GetConflictedCount() != 0 && CMessageBox::ShowCheck(GetSafeHwnd(), IDS_PROGRS_CONFLICTSOCCURRED, IDS_APPNAME, 1, IDI_EXCLAMATION, IDS_OKBUTTON, IDS_IGNOREBUTTON, 0, L"CommitWarnOnUnresolved") == 1)
	{
		auto pos = m_ListCtrl.GetFirstSelectedItemPosition();
		while (pos)
			m_ListCtrl.SetItemState(m_ListCtrl.GetNextSelectedItem(pos), 0, LVIS_SELECTED);
		const int nListItems = m_ListCtrl.GetItemCount();
		for (int i = 0; i < nListItems; ++i)
		{
			auto entry = m_ListCtrl.GetListEntry(i);
			if (entry->m_Action & CTGitPath::LOGACTIONS_UNMERGED)
			{
				m_ListCtrl.EnsureVisible(i, FALSE);
				m_ListCtrl.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
				m_ListCtrl.SetFocus();
				return;
			}
		}
		return;
	}

	CString newBranch;
	if (m_bCreateNewBranch)
	{
		GetDlgItemText(IDC_NEWBRANCH, newBranch);
		if (!g_Git.IsBranchNameValid(newBranch))
		{
			ShowEditBalloon(IDC_NEWBRANCH, IDS_B_T_NOTEMPTY, IDS_ERR_ERROR, TTI_ERROR);
			return;
		}
		if (g_Git.BranchTagExists(newBranch))
		{
			// branch already exists
			CString msg;
			msg.LoadString(IDS_B_EXISTS);
			msg += L' ' + CString(MAKEINTRESOURCE(IDS_B_DELETEORDIFFERENTNAME));
			ShowEditBalloon(IDC_NEWBRANCH, msg, CString(MAKEINTRESOURCE(IDS_WARN_WARNING)));
			return;
		}
		if (g_Git.BranchTagExists(newBranch, false))
		{
			// tag with the same name exists -> shortref is ambiguous
			if (CMessageBox::Show(m_hWnd, IDS_B_SAMETAGNAMEEXISTS, IDS_APPNAME, 2, IDI_EXCLAMATION, IDS_CONTINUEBUTTON, IDS_ABORTBUTTON) == 2)
				return;
		}
	}

	CString id;
	GetDlgItemText(IDC_BUGID, id);
	if (!m_ProjectProperties.CheckBugID(id))
	{
		ShowEditBalloon(IDC_BUGID, IDS_COMMITDLG_ONLYNUMBERS, IDS_ERR_ERROR, TTI_ERROR);
		return;
	}
	m_sLogMessage = m_cLogMessage.GetText();
	if ( m_sLogMessage.IsEmpty() )
	{
		// no message entered, go round again
		CMessageBox::Show(this->m_hWnd, IDS_COMMITDLG_NOMESSAGE, IDS_APPNAME, MB_OK | MB_ICONERROR);
		return;
	}
	if ((m_ProjectProperties.bWarnIfNoIssue) && (id.IsEmpty() && !m_ProjectProperties.HasBugID(m_sLogMessage)))
	{
		if (CMessageBox::Show(this->m_hWnd, IDS_COMMITDLG_NOISSUEWARNING, IDS_APPNAME, MB_YESNO | MB_ICONWARNING)!=IDYES)
			return;
	}
	if (!m_sLogTemplate.IsEmpty() && m_sLogTemplate == m_sLogMessage)
	{
		if (CMessageBox::ShowCheck(GetSafeHwnd(), IDS_COMMITDLG_NOTEDITEDTEMPLATE, IDS_APPNAME, 2, IDI_WARNING, IDS_PROCEEDBUTTON, IDS_MSGBOX_NO, 0, L"CommitMessageTemplateNotEdited", IDS_MSGBOX_DONOTSHOWAGAIN) != 1)
		{
			CMessageBox::RemoveRegistryKey(L"CommitMessageTemplateNotEdited"); // only remember "proceed anyway"
			return;
		}
	}

	if (m_ProjectProperties.bWarnNoSignedOffBy == TRUE && m_cLogMessage.GetText().Find(GetSignedOffByLine()) == -1)
	{
		const UINT retval = CMessageBox::Show(this->m_hWnd, IDS_PROC_COMMIT_NOSIGNOFFLINE, IDS_APPNAME, 1, IDI_WARNING, IDS_PROC_COMMIT_ADDSIGNOFFBUTTON, IDS_PROC_COMMIT_NOADDSIGNOFFBUTTON, IDS_ABORTBUTTON);
		if (retval == 1)
		{
			OnBnClickedSignOff();
			m_sLogMessage = m_cLogMessage.GetText();
		}
		else if (retval == 3)
			return;
	}

	if (CAppUtils::MessageContainsConflictHints(GetSafeHwnd(), m_sLogMessage))
		return;

	CHooks::Instance().SetProjectProperties(g_Git.m_CurrentDir, m_ProjectProperties);
	if (CHooks::Instance().IsHookPresent(HookType::pre_commit_hook, g_Git.m_CurrentDir))
	{
		DWORD exitcode = 0xFFFFFFFF;
		CString error;
		CTGitPathList list;
		m_ListCtrl.WriteCheckedNamesToPathList(list);
		if (CHooks::Instance().PreCommit(GetSafeHwnd(), g_Git.m_CurrentDir, list, m_sLogMessage, exitcode, error))
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
	}

	const int nListItems = m_ListCtrl.GetItemCount();
	for (int i = 0; i < nListItems && !m_bCommitMessageOnly; ++i)
	{
		auto entry = m_ListCtrl.GetListEntry(i);
		if (!entry->m_Checked || !entry->IsDirectory())
			continue;

		bool dirty = false;
		if (entry->m_Action & CTGitPath::LOGACTIONS_UNVER)
		{
			CGit subgit;
			subgit.m_IsUseGitDLL = false;
			subgit.m_CurrentDir = g_Git.CombinePath(entry);
			CString subcmdout;
			subgit.Run(L"git.exe status --porcelain", &subcmdout, CP_UTF8);
			dirty = !subcmdout.IsEmpty();
		}
		else
		{
			CString cmd, cmdout;
			cmd.Format(L"git.exe diff -- \"%s\"", entry->GetWinPath());
			g_Git.Run(cmd, &cmdout, CP_UTF8);
			dirty = CStringUtils::EndsWith(cmdout, L"-dirty\n");
		}

		if (dirty)
		{
			CString message;
			message.Format(IDS_COMMITDLG_SUBMODULEDIRTY, static_cast<LPCWSTR>(entry->GetGitPathString()));
			const auto result = CMessageBox::Show(m_hWnd, message, IDS_APPNAME, 1, IDI_QUESTION, IDS_PROGRS_CMD_COMMIT, IDS_MSGBOX_IGNORE, IDS_MSGBOX_CANCEL);
			if (result == 1)
			{
				CString cmdCommit;
				cmdCommit.Format(L"/command:commit /path:\"%s\\%s\"", static_cast<LPCWSTR>(g_Git.m_CurrentDir), entry->GetWinPath());
				CAppUtils::RunTortoiseGitProc(cmdCommit);
				return;
			}
			else if (result == 2)
				continue;
			else
				return;
		}
	}

	if (!m_bCommitMessageOnly)
		m_ListCtrl.WriteCheckedNamesToPathList(m_selectedPathList);
	m_pathwatcher.Stop();
	InterlockedExchange(&m_bBlock, TRUE);

	int nchecked = 0;

	// these two commands will not be executed if staging support is enabled and neither will everything else inside PrepareIndexForCommitWithoutStagingSupport()
	CMassiveGitTask mgtReAddAfterCommit(L"add --ignore-errors -f");
	CMassiveGitTask mgtReDelAfterCommit(L"rm --cached --ignore-unmatch");

	CString cmd;
	CString out;

	bool bAddSuccess=true;
	bool bCloseCommitDlg=false;

	CBlockCacheForPath cacheBlock(g_Git.m_CurrentDir);

	if (!m_bStagingSupport)
	{
		PrepareIndexForCommitWithoutStagingSupport(nListItems, bAddSuccess, nchecked, mgtReAddAfterCommit, mgtReDelAfterCommit);
	}
	else // Staging support is enabled. Simply assume the index is up-to-date and skip all the code dealing with it (inside PrepareIndexForCommitWithoutStagingSupport)
	{
		// The code below that deals with the shell icons is also done when staging support is disabled (i.e. inside PrepareIndexForCommitWithoutStagingSupport)
		for (int j = 0; bAddSuccess && j < nListItems; ++j)
			CShellUpdater::Instance().AddPathForUpdate(*m_ListCtrl.GetListEntry(j));
	}

	if (bAddSuccess && m_bCreateNewBranch)
	{
		if (g_Git.Run(L"git.exe branch " + newBranch, &out, CP_UTF8))
		{
			MessageBox(L"Creating new branch failed:\n" + out, L"TortoiseGit", MB_OK | MB_ICONERROR);
			bAddSuccess = false;
		}
		if (g_Git.Run(L"git.exe checkout " + newBranch + L" --", &out, CP_UTF8))
		{
			MessageBox(L"Switching to new branch failed:\n" + out, L"TortoiseGit", MB_OK | MB_ICONERROR);
			bAddSuccess = false;
		}
	}

	if (bAddSuccess && m_bWarnDetachedHead && CheckHeadDetach())
		bAddSuccess = false;

	UpdateLogMsgByBugId(true);

	// now let the bugtraq plugin check the commit message
	CComPtr<IBugTraqProvider2> pProvider2;
	if (m_BugTraqProvider)
	{
		HRESULT hr = m_BugTraqProvider.QueryInterface(&pProvider2);
		if (SUCCEEDED(hr))
		{
			ATL::CComBSTR temp;
			ATL::CComBSTR repositoryRoot(g_Git.m_CurrentDir);
			ATL::CComBSTR parameters(m_bugtraq_association.GetParameters());
			ATL::CComBSTR commonRoot(m_pathList.GetCommonRoot().GetDirectory().GetWinPath());
			ATL::CComBSTR commitMessage(m_sLogMessage);
			CBstrSafeVector pathList(m_selectedPathList.GetCount());

			for (LONG index = 0; index < m_selectedPathList.GetCount(); ++index)
				pathList.PutElement(index, m_selectedPathList[index].GetGitPathString());

			if (FAILED(hr = pProvider2->CheckCommit(GetSafeHwnd(), parameters, repositoryRoot, commonRoot, pathList, commitMessage, &temp)))
			{
				COMError ce(hr);
				CString sErr;
				sErr.FormatMessage(IDS_ERR_FAILEDISSUETRACKERCOM, static_cast<LPCWSTR>(m_bugtraq_association.GetProviderName()), ce.GetMessageAndDescription().c_str());
				CMessageBox::Show(GetSafeHwnd(), sErr, L"TortoiseGit", MB_ICONERROR);
			}
			else
			{
				CString sError { static_cast<LPCWSTR>(temp) };
				if (!sError.IsEmpty())
				{
					CMessageBox::Show(GetSafeHwnd(), sError, L"TortoiseGit", MB_ICONERROR);
					InterlockedExchange(&m_bBlock, FALSE);
					return;
				}
			}
		}
	}

	if (m_bCommitMessageOnly || bAddSuccess && (nchecked || m_bStagingSupport || m_bCommitAmend || CTGitPath(g_Git.m_CurrentDir).IsMergeActive()))
	{
		bCloseCommitDlg = true;

		CString tempfile=::GetTempFile();
		if (tempfile.IsEmpty() || CAppUtils::SaveCommitUnicodeFile(tempfile, m_sLogMessage))
		{
			CMessageBox::Show(GetSafeHwnd(), L"Could not save commit message", L"TortoiseGit", MB_OK | MB_ICONERROR);
			InterlockedExchange(&m_bBlock, FALSE);
			return;
		}

		CTGitPath path=g_Git.m_CurrentDir;

		const BOOL IsGitSVN = path.GetAdminDirMask() & ITEMIS_GITSVN;

		out.Empty();
		CString amend;
		if(this->m_bCommitAmend)
			amend = L"--amend";

		CString dateTime;
		if (m_bSetCommitDateTime)
		{
			COleDateTime date, time;
			m_CommitDate.GetTime(date);
			m_CommitTime.GetTime(time);
			COleDateTime dateWithTime(date.GetYear(), date.GetMonth(), date.GetDay(), time.GetHour(), time.GetMinute(), time.GetSecond());
			if (dateWithTime < COleDateTime((time_t)0))
			{
				CMessageBox::Show(GetSafeHwnd(), L"Invalid time", L"TortoiseGit", MB_OK | MB_ICONERROR);
				InterlockedExchange(&m_bBlock, FALSE);
				return;
			}
			if (m_bCommitAmend && m_AsCommitDateCtrl.GetCheck())
				dateTime = L"--date=\"now\"";
			else
				dateTime.Format(L"--date=%sT%s", static_cast<LPCWSTR>(date.Format(L"%Y-%m-%d")), static_cast<LPCWSTR>(time.Format(L"%H:%M:%S")));
		}
		CString author;
		if (m_bSetAuthor)
			author.Format(L"--author=\"%s\"", static_cast<LPCWSTR>(m_sAuthor));
		CString allowEmpty = m_bCommitMessageOnly ? L"--allow-empty" : L"";
		// TODO: make sure notes.amend.rewrite does still work when switching to libgit2
		cmd.Format(L"git.exe commit %s %s %s %s -F \"%s\"", static_cast<LPCWSTR>(author), static_cast<LPCWSTR>(dateTime), static_cast<LPCWSTR>(amend), static_cast<LPCWSTR>(allowEmpty), static_cast<LPCWSTR>(tempfile));

		CCommitProgressDlg progress;
		progress.m_bBufferAll=true; // improve show speed when there are many file added.
		progress.m_GitCmd=cmd;
		progress.m_bShowCommand = FALSE;	// don't show the commit command
		progress.m_PreText = out;			// show any output already generated in log window
		if (m_ctrlOkButton.GetCurrentEntry() > 0)
			progress.m_AutoClose = GitProgressAutoClose::AUTOCLOSE_IF_NO_ERRORS;

		progress.m_PostCmdCallback = [&](DWORD status, PostCmdList& postCmdList)
		{
			if (status || m_bNoPostActions || m_bAutoClose)
				return;

			if (IsGitSVN)
				postCmdList.emplace_back(IDI_COMMIT, IDS_MENUSVNDCOMMIT, [&] { m_PostCmd = Git_PostCommit_Cmd::DCommit; });

			postCmdList.emplace_back(IDI_PUSH, IDS_MENUPUSH, [&] { m_PostCmd = Git_PostCommit_Cmd::Push; });
			postCmdList.emplace_back(IDI_PULL, IDS_MENUPULL, [&] { m_PostCmd = Git_PostCommit_Cmd::Pull; });
			postCmdList.emplace_back(IDI_COMMIT, IDS_PROC_COMMIT_RECOMMIT, [&] { m_PostCmd = Git_PostCommit_Cmd::ReCommit; });
			postCmdList.emplace_back(IDI_TAG, IDS_MENUTAG, [&] { m_PostCmd = Git_PostCommit_Cmd::CreateTag; });
		};

		m_PostCmd = Git_PostCommit_Cmd::Nothing;
		progress.DoModal();

		if (!m_bNoPostActions)
			m_regLastAction = static_cast<int>(m_ctrlOkButton.GetCurrentEntry());
		if (m_ctrlOkButton.GetCurrentEntry() == 1)
			m_PostCmd = Git_PostCommit_Cmd::ReCommit;

		::DeleteFile(tempfile);

		if (m_BugTraqProvider && progress.m_GitStatus == 0)
		{
			CComPtr<IBugTraqProvider2> pProvider;
			HRESULT hr = m_BugTraqProvider.QueryInterface(&pProvider);
			if (SUCCEEDED(hr))
			{
				ATL::CComBSTR commonRoot(g_Git.m_CurrentDir);
				CBstrSafeVector pathList(m_selectedPathList.GetCount());

				for (LONG index = 0; index < m_selectedPathList.GetCount(); ++index)
					pathList.PutElement(index, m_selectedPathList[index].GetGitPathString());

				ATL::CComBSTR logMessage(m_sLogMessage);

				CGitHash hash;
				if (g_Git.GetHash(hash, L"HEAD"))
					MessageBox(g_Git.GetGitLastErr(L"Could not get HEAD hash after committing."), L"TortoiseGit", MB_ICONERROR);
				LONG version = g_Git.Hash2int(hash);

				ATL::CComBSTR temp;
				if (FAILED(hr = pProvider->OnCommitFinished(GetSafeHwnd(),
					commonRoot,
					pathList,
					logMessage,
					version,
					&temp)))
				{
					CString sErr { static_cast<LPCWSTR>(temp) };
					if (!sErr.IsEmpty())
						CMessageBox::Show(GetSafeHwnd(), sErr, L"TortoiseGit", MB_OK | MB_ICONERROR);
					else
					{
						COMError ce(hr);
						sErr.FormatMessage(IDS_ERR_FAILEDISSUETRACKERCOM, ce.GetSource().c_str(), ce.GetMessageAndDescription().c_str());
						CMessageBox::Show(GetSafeHwnd(), sErr, L"TortoiseGit", MB_OK | MB_ICONERROR);
					}
				}
			}
		}
		RestoreFiles(progress.m_GitStatus == 0, false);
		if (!m_bStagingSupport && static_cast<DWORD>(CRegStdDWORD(L"Software\\TortoiseGit\\ReaddUnselectedAddedFilesAfterCommit", TRUE)) == TRUE)
		{
			BOOL cancel = FALSE;
			mgtReAddAfterCommit.Execute(cancel);
			mgtReDelAfterCommit.Execute(cancel);
		}

		if (!progress.m_GitStatus)
		{
			DWORD exitcode = 0xFFFFFFFF;
			CString error;
			CHooks::Instance().SetProjectProperties(g_Git.m_CurrentDir, m_ProjectProperties);
			if (CHooks::Instance().PostCommit(GetSafeHwnd(), g_Git.m_CurrentDir, amend.IsEmpty(), exitcode, error))
			{
				if (exitcode)
				{
					CString temp;
					temp.Format(IDS_ERR_HOOKFAILED, static_cast<LPCWSTR>(error));
					MessageBox(temp, L"TortoiseGit", MB_ICONERROR);
					bCloseCommitDlg = false;
				}
			}
			CTGitPathList* pList;
			if (m_bWholeProject || m_bWholeProject2)
				pList = nullptr;
			else
				pList = &m_pathList;
			if (!m_ListCtrl.KeepChangeList())
			{
				m_ListCtrl.PruneChangelists(pList);
				m_ListCtrl.SaveChangelists();
			}
		}

		if (progress.m_GitStatus || m_PostCmd == Git_PostCommit_Cmd::ReCommit)
		{
			bCloseCommitDlg = false;
			if (m_bCreateNewBranch)
			{
				SetDlgItemText(IDC_COMMIT_TO, g_Git.GetCurrentBranch()); // issue #3625
				GetDlgItem(IDC_NEWBRANCH)->ShowWindow(SW_HIDE);
				GetDlgItem(IDC_COMMIT_TO)->ShowWindow(SW_SHOW);
			}
			m_bCreateNewBranch = FALSE;
			if (!progress.m_GitStatus && m_PostCmd == Git_PostCommit_Cmd::ReCommit)
			{
				if (!m_sLogMessage.IsEmpty())
				{
					ReloadHistoryEntries();
					m_History.AddEntry(m_sLogMessage);
					m_History.Save();
				}
				if (m_bCommitAmend && !m_NoAmendStr.IsEmpty() && (m_sLogTemplate.Compare(m_NoAmendStr) != 0))
				{
					ReloadHistoryEntries();
					m_History.AddEntry(m_NoAmendStr);
					m_History.Save();
				}

				GetCommitTemplate(m_sLogTemplate);
				m_sLogMessage = m_sLogTemplate;
				m_cLogMessage.SetText(m_sLogMessage);
				m_cLogMessage.ClearUndoBuffer();
				m_bCommitMessageOnly = FALSE;
				m_ListCtrl.EnableWindow(TRUE);
				m_ListCtrl.Clear();
				if (!RunStartCommitHook())
					bCloseCommitDlg = true;
			}

			if (!progress.m_GitStatus)
			{
				m_AmendStr.Empty();
				m_bCommitAmend = FALSE;
				GetDlgItem(IDC_COMMIT_AMENDDIFF)->ShowWindow(SW_HIDE);
				DialogEnableWindow(IDC_COMMIT_AMENDDIFF, FALSE);
				m_bSetCommitDateTime = FALSE;
				m_AsCommitDateCtrl.ShowWindow(SW_HIDE);
				m_AsCommitDateCtrl.SetCheck(FALSE);
				m_AsCommitDateCtrl.EnableWindow(FALSE);
				GetDlgItem(IDC_COMMIT_DATEPICKER)->ShowWindow(SW_HIDE);
				GetDlgItem(IDC_COMMIT_TIMEPICKER)->ShowWindow(SW_HIDE);
				m_bSetAuthor = FALSE;
				m_sAuthor.Format(L"%s <%s>", static_cast<LPCWSTR>(g_Git.GetUserName()), static_cast<LPCWSTR>(g_Git.GetUserEmail()));
				GetDlgItem(IDC_COMMIT_AUTHORDATA)->SendMessage(EM_SETREADONLY, TRUE);
			}

			UpdateData(FALSE);
		}
	}
	else if(bAddSuccess)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERROR_NOTHING_COMMIT, IDS_COMMIT_FINISH, MB_OK | MB_ICONINFORMATION);
		bCloseCommitDlg=false;
	}

	UpdateData();
	m_regAddBeforeCommit = m_bShowUnversioned;
	m_regDoNotAutoselectSubmodules = m_bDoNotAutoselectSubmodules;
	InterlockedExchange(&m_bBlock, FALSE);

	if (!m_sLogMessage.IsEmpty())
	{
		ReloadHistoryEntries();
		m_History.AddEntry(m_sLogMessage);
		m_History.Save();
	}
	if (m_bCommitAmend && !m_NoAmendStr.IsEmpty() && (m_sLogTemplate.Compare(m_NoAmendStr) != 0))
	{
		ReloadHistoryEntries();
		m_History.AddEntry(m_NoAmendStr);
		m_History.Save();
	}

	SaveSplitterPos();

	if (bCloseCommitDlg)
	{
		if (m_ctrlOkButton.GetCurrentEntry() == 2)
			DoPush(GetSafeHwnd(), !!m_bCommitAmend);
		CResizableStandAloneDialog::OnOK();
	}
	else if (m_PostCmd == Git_PostCommit_Cmd::ReCommit)
	{
		m_bDoNotStoreLastSelectedLine = true;
		this->Refresh();
		this->BringWindowToTop();
	}

	CShellUpdater::Instance().Flush();
}

void CCommitDlg::PrepareIndexForCommitWithoutStagingSupport(int nListItems, bool& bAddSuccess, int& nchecked, CMassiveGitTask& mgtReAddAfterCommit, CMassiveGitTask& mgtReDelAfterCommit)
{
	//first add all the unversioned files the user selected
	//and check if all versioned files are selected

	CSysProgressDlg sysProgressDlg;
	if (nListItems >= 25)
	{
		sysProgressDlg.SetTitle(CString(MAKEINTRESOURCE(IDS_PROC_COMMIT_PREPARECOMMIT)));
		sysProgressDlg.SetLine(1, CString(MAKEINTRESOURCE(IDS_PROC_COMMIT_UPDATEINDEX)));
		sysProgressDlg.SetTime(true);
		sysProgressDlg.SetShowProgressBar(true);
		sysProgressDlg.ShowModal(this, true);
	}

	ULONGLONG currentTicks = GetTickCount64();

	if (g_Git.UsingLibGit2(CGit::GIT_CMD_COMMIT_UPDATE_INDEX))
	{
		bAddSuccess = false;
		do
		{
			CAutoRepository repository(g_Git.GetGitRepository());
			if (!repository)
			{
				CMessageBox::Show(GetSafeHwnd(), CGit::GetLibGit2LastErr(L"Could not open repository."), L"TortoiseGit", MB_OK | MB_ICONERROR);
				break;
			}

			CGitHash revHash;
			CString revRef = L"HEAD";
			if (m_bCommitAmend && !m_bAmendDiffToLastCommit)
				revRef = L"HEAD~1";
			if (CGit::GetHash(repository, revHash, revRef))
			{
				MessageBox(g_Git.GetLibGit2LastErr(L"Could not get HEAD hash after committing."), L"TortoiseGit", MB_ICONERROR);
				break;
			}

			CAutoCommit commit;
			if (!revHash.IsEmpty() && git_commit_lookup(commit.GetPointer(), repository, revHash))
			{
				CMessageBox::Show(GetSafeHwnd(), CGit::GetLibGit2LastErr(L"Could not get last commit."), L"TortoiseGit", MB_OK | MB_ICONERROR);
				break;
			}

			CAutoTree tree;
			if (!revHash.IsEmpty() && git_commit_tree(tree.GetPointer(), commit))
			{
				CMessageBox::Show(GetSafeHwnd(), CGit::GetLibGit2LastErr(L"Could not read tree of commit."), L"TortoiseGit", MB_OK | MB_ICONERROR);
				break;
			}

			CAutoIndex index;
			if (git_repository_index(index.GetPointer(), repository))
			{
				CMessageBox::Show(GetSafeHwnd(), CGit::GetLibGit2LastErr(L"Could not get the repository index."), L"TortoiseGit", MB_OK | MB_ICONERROR);
				break;
			}

			CAutoIndex indexOld;
			git_index_options opts = GIT_INDEX_OPTIONS_INIT;
			opts.oid_type = git_repository_oid_type(repository);
			if (!revHash.IsEmpty() && (git_index_new(indexOld.GetPointer(), &opts) || git_index_read_tree(indexOld, tree)))
			{
				CMessageBox::Show(GetSafeHwnd(), CGit::GetLibGit2LastErr(L"Could not read the tree into the index."), L"TortoiseGit", MB_OK | MB_ICONERROR);
				break;
			}

			bAddSuccess = true;

			for (int j = 0; j < nListItems; ++j)
			{
				auto entry = m_ListCtrl.GetListEntry(j);

				if (sysProgressDlg.IsVisible())
				{
					if (GetTickCount64() - currentTicks > 1000UL || j == nListItems - 1 || j == 0)
					{
						sysProgressDlg.SetLine(2, entry->GetGitPathString(), true);
						sysProgressDlg.SetProgress(j, nListItems);
						AfxGetThread()->PumpMessage(); // process messages, in order to avoid freezing; do not call this too often: this takes time!
						currentTicks = GetTickCount64();
					}
				}

				CStringA filePathA = CUnicodeUtils::GetUTF8(entry->GetGitPathString()).TrimRight(L'/');

				if (entry->m_Checked && !m_bCommitMessageOnly)
				{
					if (entry->m_Action & CTGitPath::LOGACTIONS_DELETED)
						git_index_remove_bypath(index, filePathA); // ignore error
					else
					{
						if (git_index_add_bypath(index, filePathA))
						{
							bAddSuccess = false;
							CMessageBox::Show(GetSafeHwnd(), CGit::GetLibGit2LastErr(L"Could not add \"" + entry->GetGitPathString() + L"\" to index."), L"TortoiseGit", MB_OK | MB_ICONERROR);
							break;
						}
					}

					if ((entry->m_Action & CTGitPath::LOGACTIONS_REPLACED) && !entry->GetGitOldPathString().IsEmpty())
						git_index_remove_bypath(index, CUnicodeUtils::GetUTF8(entry->GetGitOldPathString())); // ignore error

					++nchecked;
				}
				else
				{
					if (entry->m_Action & CTGitPath::LOGACTIONS_ADDED || entry->m_Action & CTGitPath::LOGACTIONS_REPLACED)
					{
						git_index_remove_bypath(index, filePathA); // ignore error
						mgtReAddAfterCommit.AddFile(*entry);

						if (entry->m_Action & CTGitPath::LOGACTIONS_REPLACED && !entry->GetGitOldPathString().IsEmpty())
						{
							const git_index_entry* oldIndexEntry = nullptr;
							if ((oldIndexEntry = git_index_get_bypath(indexOld, CUnicodeUtils::GetUTF8(entry->GetGitOldPathString()), 0)) == nullptr || git_index_add(index, oldIndexEntry))
							{
								bAddSuccess = false;
								CMessageBox::Show(GetSafeHwnd(), CGit::GetLibGit2LastErr(L"Could not reset \"" + entry->GetGitOldPathString() + L"\" to old index entry."), L"TortoiseGit", MB_OK | MB_ICONERROR);
								break;
							}
							mgtReDelAfterCommit.AddFile(entry->GetGitOldPathString());
						}
					}
					else if (!(entry->m_Action & CTGitPath::LOGACTIONS_UNVER))
					{
						const git_index_entry* oldIndexEntry = nullptr;
						if ((oldIndexEntry = git_index_get_bypath(indexOld, filePathA, 0)) == nullptr || git_index_add(index, oldIndexEntry))
						{
							bAddSuccess = false;
							CMessageBox::Show(GetSafeHwnd(), CGit::GetLibGit2LastErr(L"Could not reset \"" + entry->GetGitPathString() + L"\" to old index entry."), L"TortoiseGit", MB_OK | MB_ICONERROR);
							break;
						}
						if (entry->m_Action & CTGitPath::LOGACTIONS_DELETED && !(entry->m_Action & CTGitPath::LOGACTIONS_MISSING))
							mgtReDelAfterCommit.AddFile(entry->GetGitPathString());
					}
				}

				if (sysProgressDlg.HasUserCancelled())
				{
					bAddSuccess = false;
					break;
				}
			}
			if (bAddSuccess && git_index_write(index))
				bAddSuccess = false;

			for (int j = 0; bAddSuccess && j < nListItems; ++j)
				CShellUpdater::Instance().AddPathForUpdate(*m_ListCtrl.GetListEntry(j));
		} while (0);
	}
	else
	{
		// ***************************************************
		// ATTENTION: Similar code in RebaseDlg.cpp!!!
		// ***************************************************
		CMassiveGitTask mgtAdd(L"add -f");
		CMassiveGitTask mgtUpdateIndexForceRemove(L"update-index --force-remove");
		CMassiveGitTask mgtUpdateIndex(L"update-index");
		CMassiveGitTask mgtRm(L"rm --ignore-unmatch");
		CMassiveGitTask mgtRmFCache(L"rm -f --cache");
		CString resetCmd = L"reset";
		if (m_bCommitAmend && !m_bAmendDiffToLastCommit)
			resetCmd += L" HEAD~1";
		CMassiveGitTask mgtReset(resetCmd, TRUE, true);
		for (int j = 0; j < nListItems; ++j)
		{
			auto entry = m_ListCtrl.GetListEntry(j);

			if (entry->m_Checked && !m_bCommitMessageOnly)
			{
				if ((entry->m_Action & CTGitPath::LOGACTIONS_UNVER) || (entry->IsDirectory() && !(entry->m_Action & CTGitPath::LOGACTIONS_DELETED)))
					mgtAdd.AddFile(entry->GetGitPathString());
				else if (entry->m_Action & CTGitPath::LOGACTIONS_DELETED)
					mgtUpdateIndexForceRemove.AddFile(entry->GetGitPathString());
				else
					mgtUpdateIndex.AddFile(entry->GetGitPathString());

				if ((entry->m_Action & CTGitPath::LOGACTIONS_REPLACED) && !entry->GetGitOldPathString().IsEmpty())
					mgtRm.AddFile(entry->GetGitOldPathString());

				++nchecked;
			}
			else
			{
				if (entry->m_Action & CTGitPath::LOGACTIONS_ADDED || entry->m_Action & CTGitPath::LOGACTIONS_REPLACED)
				{ //To init git repository, there are not HEAD, so we can use git reset command
					mgtRmFCache.AddFile(entry->GetGitPathString());
					mgtReAddAfterCommit.AddFile(*entry);

					if (entry->m_Action & CTGitPath::LOGACTIONS_REPLACED && !entry->GetGitOldPathString().IsEmpty())
					{
						mgtReset.AddFile(entry->GetGitOldPathString());
						mgtReDelAfterCommit.AddFile(entry->GetGitOldPathString());
					}
				}
				else if (!(entry->m_Action & CTGitPath::LOGACTIONS_UNVER))
				{
					mgtReset.AddFile(entry->GetGitPathString());
					if (entry->m_Action & CTGitPath::LOGACTIONS_DELETED && !(entry->m_Action & CTGitPath::LOGACTIONS_MISSING))
						mgtReDelAfterCommit.AddFile(entry->GetGitPathString());
				}
			}

			if (sysProgressDlg.HasUserCancelled())
			{
				bAddSuccess = false;
				break;
			}
		}

		CMassiveGitTask tasks[] = { mgtAdd, mgtUpdateIndexForceRemove, mgtUpdateIndex, mgtRm, mgtRmFCache, mgtReset };
		int progress = 0, maxProgress = 0;
		for (int j = 0; j < _countof(tasks); ++j)
			maxProgress += tasks[j].GetListCount();
		for (int j = 0; j < _countof(tasks); ++j)
			bAddSuccess = bAddSuccess && UpdateIndex(tasks[j], sysProgressDlg, progress += tasks[j].GetListCount(), maxProgress);

		if (sysProgressDlg.HasUserCancelled())
			bAddSuccess = false;

		for (int j = 0; bAddSuccess && j < nListItems; ++j)
			CShellUpdater::Instance().AddPathForUpdate(*m_ListCtrl.GetListEntry(j));
	}

	if (sysProgressDlg.HasUserCancelled())
		bAddSuccess = false;

	sysProgressDlg.Stop();
}

void CCommitDlg::SaveSplitterPos()
{
	if (!IsIconic())
	{
		CRegDWORD regPos(L"Software\\TortoiseGit\\TortoiseProc\\ResizableState\\CommitDlgSizer");
		RECT rectSplitter;
		m_wndSplitter.GetWindowRect(&rectSplitter);
		ScreenToClient(&rectSplitter);
		regPos = CDPIAware::Instance().UnscaleY(GetSafeHwnd(), rectSplitter.top);
	}
}

UINT CCommitDlg::StatusThreadEntry(LPVOID pVoid)
{
	return static_cast<CCommitDlg*>(pVoid)->StatusThread();
}

void CCommitDlg::ReloadHistoryEntries()
{
	CString reg;
	reg.Format(L"Software\\TortoiseGit\\History\\commit%s", static_cast<LPCWSTR>(m_ListCtrl.m_sUUID));
	reg.Replace(L':', L'_');
	m_History.Load(reg, L"logmsgs");
}

UINT CCommitDlg::StatusThread()
{
	//get the status of all selected file/folders recursively
	//and show the ones which have to be committed to the user
	//in a list control.
	m_pathwatcher.Stop();

	m_bCancelled = false;

	DialogEnableWindow(IDOK, false);
	DialogEnableWindow(IDC_SHOWUNVERSIONED, false);
	DialogEnableWindow(IDC_WHOLE_PROJECT, false);
	DialogEnableWindow(IDC_NOAUTOSELECTSUBMODULES, false);
	DialogEnableWindow(IDC_COMMIT_AMEND, FALSE);
	DialogEnableWindow(IDC_COMMIT_AMENDDIFF, FALSE);
	DialogEnableWindow(IDC_STAGINGSUPPORT, false);
	GetDlgItem(IDC_COMMIT_AUTHORDATA)->SendMessage(EM_SETREADONLY, TRUE);
	// read the list of recent log entries before querying the WC for status
	// -> the user may select one and modify / update it while we are crawling the WC

	DialogEnableWindow(IDC_CHECKALL, false);
	DialogEnableWindow(IDC_CHECKNONE, false);
	DialogEnableWindow(IDC_CHECKUNVERSIONED, false);
	DialogEnableWindow(IDC_CHECKVERSIONED, false);
	DialogEnableWindow(IDC_CHECKADDED, false);
	DialogEnableWindow(IDC_CHECKDELETED, false);
	DialogEnableWindow(IDC_CHECKMODIFIED, false);
	DialogEnableWindow(IDC_CHECKFILES, false);
	DialogEnableWindow(IDC_CHECKSUBMODULES, false);

	g_Git.RefreshGitIndex();

	CTGitPath repoRoot { g_Git.m_CurrentDir };
	if (repoRoot.IsCherryPickActive())
	{
		GetDlgItem(IDC_COMMIT_AMENDDIFF)->ShowWindow(SW_HIDE);
		DialogEnableWindow(IDC_COMMIT_AMENDDIFF, FALSE);
		m_bCommitAmend = FALSE;
		SendMessage(WM_UPDATEDATAFALSE);
	}

	if (!m_bDoNotStoreLastSelectedLine)
		m_ListCtrl.StoreScrollPos();
	// Initialise the list control with the status of the files/folders below us
	m_ListCtrl.Clear();
	BOOL success;
	CTGitPathList *pList;
	m_ListCtrl.m_amend = (m_bCommitAmend==TRUE || m_bForceCommitAmend) && (m_bAmendDiffToLastCommit==FALSE);
	m_ListCtrl.m_bIncludedStaged = true;
	m_ListCtrl.m_bDoNotAutoselectSubmodules = (m_bDoNotAutoselectSubmodules == TRUE);

	if(m_bWholeProject || m_bWholeProject2)
		pList = nullptr;
	else
		pList = &m_pathList;

	success = m_ListCtrl.GetStatus(pList, false, false, false, false, false, m_bStagingSupport);

	//m_ListCtrl.UpdateFileList(git_revnum_t(GIT_REV_ZERO));
	if(this->m_bShowUnversioned)
		m_ListCtrl.UpdateFileList(CGitStatusListCtrl::FILELIST_UNVER, true, pList, m_bStagingSupport);

	m_ListCtrl.CheckIfChangelistsArePresent(false);

	DWORD dwShow = GITSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALSFROMDIFFERENTREPOS | GITSLC_SHOWINCHANGELIST | GITSLC_SHOWDIRECTFILES;
	dwShow |= DWORD(m_regAddBeforeCommit) ? GITSLC_SHOWUNVERSIONED : 0;
	if (success)
	{
		/*if (!m_checkedPathList.IsEmpty())
			m_ListCtrl.Show(dwShow, m_checkedPathList);
		else*/
		{
			DWORD dwCheck = m_bSelectFilesForCommit ? dwShow : 0;
			dwCheck &=~(CTGitPath::LOGACTIONS_UNVER); //don't check unversion file default.
			m_ListCtrl.Show(dwShow, dwCheck);
		}

		SetDlgItemText(IDC_COMMIT_TO, g_Git.GetCurrentBranch());
		m_tooltips.AddTool(GetDlgItem(IDC_STATISTICS), m_ListCtrl.GetStatisticsString());
		if (m_ListCtrl.GetItemCount() != 0 && m_ListCtrl.GetTopIndex() == 0 && (m_bDoNotStoreLastSelectedLine || CRegDWORD(L"Software\\TortoiseGit\\RememberFileListPosition", TRUE) != TRUE))
			m_ListCtrl.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED);
	}
	if (!success)
	{
		if (!m_ListCtrl.GetLastErrorMessage().IsEmpty())
			m_ListCtrl.SetEmptyString(m_ListCtrl.GetLastErrorMessage());
		m_ListCtrl.Show(dwShow);
	}
	m_bDoNotStoreLastSelectedLine = false;

	if (m_ListCtrl.GetItemCount() == 0 && m_ListCtrl.HasUnversionedItems() && !repoRoot.IsMergeActive())
	{
		CString temp;
		temp.LoadString(IDS_COMMITDLG_NOTHINGTOCOMMITUNVERSIONED);
		if (CMessageBox::ShowCheck(GetSafeHwnd(), temp, L"TortoiseGit", MB_ICONINFORMATION | MB_YESNO, L"NothingToCommitShowUnversioned", nullptr) == IDYES)
		{
			m_bShowUnversioned = TRUE;
			GetDlgItem(IDC_SHOWUNVERSIONED)->SendMessage(BM_SETCHECK, BST_CHECKED);
			dwShow = static_cast<DWORD>(GITSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALSFROMDIFFERENTREPOS | GITSLC_SHOWUNVERSIONED);
			m_ListCtrl.UpdateFileList(CGitStatusListCtrl::FILELIST_UNVER);
			m_ListCtrl.Show(dwShow,dwShow&(~CTGitPath::LOGACTIONS_UNVER));
		}
	}

	GetDlgItem(IDC_MERGEACTIVE)->ShowWindow(repoRoot.IsMergeActive() ? SW_SHOW : SW_HIDE);

	SetDlgTitle();

	// we don't have to block the commit dialog while we fetch the
	// auto completion list.
	m_pathwatcher.ClearChangedPaths();
	InterlockedExchange(&m_bBlock, FALSE);
	std::map<CString, int> autolist;
	if (static_cast<DWORD>(CRegDWORD(L"Software\\TortoiseGit\\Autocompletion", TRUE)) == TRUE)
	{
		m_ListCtrl.BusyCursor(true);
		GetAutocompletionList(autolist);
		m_ListCtrl.BusyCursor(false);
	}
	SendMessage(WM_UPDATEOKBUTTON);
	m_ListCtrl.Invalidate();
	if (m_bRunThread)
	{
		DialogEnableWindow(IDC_SHOWUNVERSIONED, true);
		DialogEnableWindow(IDC_WHOLE_PROJECT, !m_bWholeProject2);
		DialogEnableWindow(IDC_NOAUTOSELECTSUBMODULES, true);
		DialogEnableWindow(IDC_STAGINGSUPPORT, true);

		// activate amend checkbox (if necessary)
		if (g_Git.IsInitRepos())
		{
			m_bCommitAmend = FALSE;
			SendMessage(WM_UPDATEDATAFALSE);
		}
		else
		{
			if (m_bForceCommitAmend)
			{
				m_bCommitAmend = TRUE;
				SendMessage(WM_UPDATEDATAFALSE);
			}
			else
				GetDlgItem(IDC_COMMIT_AMEND)->EnableWindow(!repoRoot.IsCherryPickActive());

			CGitHash hash;
			if (g_Git.GetHash(hash, L"HEAD"))
			{
				MessageBox(g_Git.GetGitLastErr(L"Could not get HEAD hash."), L"TortoiseGit", MB_ICONERROR);
			}
			if (!hash.IsEmpty())
			{
				GitRev headRevision;
				if (headRevision.GetParentFromHash(hash))
					MessageBox(headRevision.GetLastErr(), L"TortoiseGit", MB_ICONERROR);
				// do not allow to show diff to "last" revision if it has more that one parent
				if (headRevision.ParentsCount() < 1)
				{
					m_bAmendDiffToLastCommit = TRUE;
					SendMessage(WM_UPDATEDATAFALSE);
				}
				else
					GetDlgItem(IDC_COMMIT_AMENDDIFF)->EnableWindow(TRUE);
			}
		}

		if (m_bSetAuthor)
			GetDlgItem(IDC_COMMIT_AUTHORDATA)->SendMessage(EM_SETREADONLY, FALSE);
		else
		{
			m_sAuthor.Format(L"%s <%s>", static_cast<LPCWSTR>(g_Git.GetUserName()), static_cast<LPCWSTR>(g_Git.GetUserEmail()));
			if (m_bCommitAmend)
			{
				GitRev headRevision;
				if (headRevision.GetCommit(L"HEAD"))
					MessageBox(headRevision.GetLastErr(), L"TortoiseGit", MB_ICONERROR);
				else
					m_sAuthor.Format(L"%s <%s>", static_cast<LPCWSTR>(headRevision.GetAuthorName()), static_cast<LPCWSTR>(headRevision.GetAuthorEmail()));
			}
			SendMessage(WM_UPDATEDATAFALSE);
		}

		UpdateCheckLinks();

		// we have the list, now signal the main thread about it
		SendMessage(WM_AUTOLISTREADY, 0, reinterpret_cast<LPARAM>(&autolist));	// only send the message if the thread wasn't told to quit!
	}

	InterlockedExchange(&m_bRunThread, FALSE);
	InterlockedExchange(&m_bThreadRunning, FALSE);
	// force the cursor to normal
	RefreshCursor();

	return 0;
}

void CCommitDlg::SetDlgTitle()
{
	if (m_sTitle.IsEmpty())
		GetWindowText(m_sTitle);

	if (m_bWholeProject || m_bWholeProject2)
		CAppUtils::SetWindowTitle(m_hWnd, g_Git.m_CurrentDir, m_sTitle);
	else
	{
		if (m_pathList.GetCount() == 1)
			CAppUtils::SetWindowTitle(m_hWnd, g_Git.CombinePath(m_pathList[0].GetUIPathString()), m_sTitle);
		else
			CAppUtils::SetWindowTitle(m_hWnd, g_Git.CombinePath(m_ListCtrl.GetCommonDirectory(false)), m_sTitle);
	}
}

void CCommitDlg::OnCancel()
{
	UpdateData();
	m_sLogMessage = m_cLogMessage.GetText();
	UpdateLogMsgByBugId(false);

	bool hasChangedMessage;
	if (m_bCommitAmend)
		hasChangedMessage = (m_AmendStr.Compare(m_sLogMessage) != 0) && !m_sLogMessage.IsEmpty();
	else
		hasChangedMessage = (m_sLogTemplate.Compare(m_sLogMessage) != 0) && !m_sLogMessage.IsEmpty();

	CString tmp;
	tmp.LoadString(IDS_REALLYCANCEL);
	tmp.AppendChar(L'\n');
	tmp.AppendChar(L'\n');
	tmp.Append(CString(MAKEINTRESOURCE(IDS_HINTLASTMESSAGES)));
	CString dontaskagain;
	dontaskagain.LoadString(IDS_MSGBOX_DONOTSHOWAGAIN);
	BOOL dontaskagainchecked = FALSE;
	if ((hasChangedMessage || m_ListCtrl.GetItemCount() > 0) && CMessageBox::ShowCheck(GetSafeHwnd(), tmp, L"TortoiseGit", MB_ICONQUESTION | MB_DEFBUTTON2 | MB_YESNO, L"CommitAskBeforeCancel", dontaskagain, &dontaskagainchecked) == IDNO)
	{
		if (dontaskagainchecked)
			CMessageBox::SetRegistryValue(L"CommitAskBeforeCancel", IDYES);
		return;
	}

	m_bCancelled = true;
	m_pathwatcher.Stop();

	if (m_bThreadRunning)
		StopStatusThread();
	if (!RestoreFiles())
		return;
	if (hasChangedMessage)
	{
		ReloadHistoryEntries();
		m_History.AddEntry(m_sLogMessage);
		m_History.Save();
	}
	if (m_bCommitAmend && !m_NoAmendStr.IsEmpty() && (m_sLogTemplate.Compare(m_NoAmendStr) != 0))
	{
		ReloadHistoryEntries();
		m_History.AddEntry(m_NoAmendStr);
		m_History.Save();
	}
	SaveSplitterPos();
	CResizableStandAloneDialog::OnCancel();
}

void CCommitDlg::UpdateLogMsgByBugId(bool compareExistingBugID)
{
	GetDlgItemText(IDC_BUGID, m_sBugID);
	m_sBugID.Trim();
	if (m_sBugID.IsEmpty())
		return;
	if (compareExistingBugID)
	{
		CString sExistingBugID = m_ProjectProperties.FindBugID(m_sLogMessage);
		sExistingBugID.Trim();
		if (!m_sBugID.Compare(sExistingBugID)) // not match
			return;
	}

	m_sBugID.Replace(L", ", L",");
	m_sBugID.Replace(L" ,", L",");
	CString sBugID = m_ProjectProperties.sMessage;
	sBugID.Replace(L"%BUGID%", m_sBugID);
	if (m_ProjectProperties.bAppend)
		m_sLogMessage += L'\n' + sBugID + L'\n';
	else
		m_sLogMessage = sBugID + L'\n' + m_sLogMessage;
}

BOOL CCommitDlg::PreTranslateMessage(MSG* pMsg)
{
	if (m_hAccel)
	{
		const int ret = TranslateAccelerator(m_hWnd, m_hAccel, pMsg);
		if (ret)
			return TRUE;
	}
	if (m_hAccelOkButton && GetDlgItem(IDOK)->IsWindowEnabled() && TranslateAccelerator(m_hWnd, m_hAccelOkButton, pMsg))
		return TRUE;

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
		case VK_RETURN:
			{
				if (GetAsyncKeyState(VK_CONTROL)&0x8000)
				{
					if ( GetDlgItem(IDOK)->IsWindowEnabled() )
						PostMessage(WM_COMMAND, IDOK);
					return TRUE;
				}
				if ( GetFocus()==GetDlgItem(IDC_BUGID) )
				{
					// Pressing RETURN in the bug id control
					// moves the focus to the message
					GetDlgItem(IDC_LOGMESSAGE)->SetFocus();
					return TRUE;
				}
			}
			break;
		}
	}

	return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}

void CCommitDlg::Refresh()
{
	if (m_bThreadRunning)
		return;

	StartStatusThread();
}

void CCommitDlg::StartStatusThread()
{
	if (InterlockedExchange(&m_bBlock, TRUE) != FALSE)
		return;

	delete m_pThread;

	m_pThread = AfxBeginThread(StatusThreadEntry, this, THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
	if (!m_pThread)
	{
		CMessageBox::Show(GetSafeHwnd(), IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
		InterlockedExchange(&m_bBlock, FALSE);
		return;
	}
	InterlockedExchange(&m_bThreadRunning, TRUE);// so the main thread knows that this thread is still running
	InterlockedExchange(&m_bRunThread, TRUE);	// if this is set to FALSE, the thread should stop
	m_pThread->m_bAutoDelete = FALSE;
	m_pThread->ResumeThread();
}

void CCommitDlg::StopStatusThread()
{
	InterlockedExchange(&m_bRunThread, FALSE);
	WaitForSingleObject(m_pThread->m_hThread, 5000);
	if (!m_bThreadRunning)
		return;

	// we gave the thread a chance to quit. Since the thread didn't
	// listen to us we have to kill it.
	g_Git.KillRelatedThreads(m_pThread);
	InterlockedExchange(&m_bThreadRunning, FALSE);
}

void CCommitDlg::OnBnClickedShowunversioned()
{
	m_tooltips.Pop();	// hide the tooltips
	UpdateData();
	m_regAddBeforeCommit = m_bShowUnversioned;
	if (!m_bBlock)
	{
		DWORD dwShow = m_ListCtrl.GetShowFlags();
		if (DWORD(m_regAddBeforeCommit))
			dwShow |= GITSLC_SHOWUNVERSIONED;
		else
			dwShow &= ~GITSLC_SHOWUNVERSIONED;
		if(dwShow & GITSLC_SHOWUNVERSIONED)
		{
			if (m_bWholeProject || m_bWholeProject2)
				m_ListCtrl.GetStatus(nullptr, false, false, true);
			else
				m_ListCtrl.GetStatus(&this->m_pathList,false,false,true);
		}
		m_ListCtrl.StoreScrollPos();
		m_ListCtrl.Show(dwShow, 0, true, dwShow & ~(CTGitPath::LOGACTIONS_UNVER), true);
		UpdateCheckLinks();
	}
}

void CCommitDlg::OnEnChangeLogmessage()
{
	SendMessage(WM_UPDATEOKBUTTON);
}

LRESULT CCommitDlg::OnGitStatusListCtrlItemCountChanged(WPARAM, LPARAM)
{
#if 0
	if ((m_ListCtrl.GetItemCount() == 0)&&(m_ListCtrl.HasUnversionedItems())&&(!m_bShowUnversioned))
	{
		if (CMessageBox::Show(*this, IDS_COMMITDLG_NOTHINGTOCOMMITUNVERSIONED, IDS_APPNAME, MB_ICONINFORMATION | MB_YESNO)==IDYES)
		{
			m_bShowUnversioned = TRUE;
			DWORD dwShow = GitSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALSFROMDIFFERENTREPOS | GitSLC_SHOWUNVERSIONED;
			m_ListCtrl.Show(dwShow);
			UpdateData(FALSE);
		}
	}
#endif
	return 0;
}

LRESULT CCommitDlg::OnGitStatusListCtrlNeedsRefresh(WPARAM, LPARAM)
{
	Refresh();
	return 0;
}

LRESULT CCommitDlg::OnFileDropped(WPARAM, LPARAM lParam)
{
	BringWindowToTop();
	SetForegroundWindow();
	SetActiveWindow();
	// if multiple files/folders are dropped
	// this handler is called for every single item
	// separately.
	// To avoid creating multiple refresh threads and
	// causing crashes, we only add the items to the
	// list control and start a timer.
	// When the timer expires, we start the refresh thread,
	// but only if it isn't already running - otherwise we
	// restart the timer.
	CTGitPath path;
	path.SetFromWin(reinterpret_cast<LPCWSTR>(lParam));

	// check whether the dropped file belongs to the very same repository
	CString projectDir;
	if (!path.HasAdminDir(&projectDir) || !CPathUtils::ArePathStringsEqual(g_Git.m_CurrentDir, projectDir))
		return 0;

	// just add all the items we get here.
	// if the item is versioned, the add will fail but nothing
	// more will happen.
	CString cmd;
	cmd.Format(L"git.exe add -- \"%s\"", static_cast<LPCWSTR>(path.GetWinPathString()));
	g_Git.Run(cmd, nullptr, CP_UTF8);

	if (!m_ListCtrl.HasPath(path))
	{
		if (m_pathList.AreAllPathsFiles())
		{
			m_pathList.AddPath(path);
			m_pathList.RemoveDuplicates();
		}
		else
		{
			// if the path list contains folders, we have to check whether
			// our just (maybe) added path is a child of one of those. If it is
			// a child of a folder already in the list, we must not add it. Otherwise
			// that path could show up twice in the list.
			if (!m_pathList.IsAnyAncestorOf(path))
			{
				m_pathList.AddPath(path);
				m_pathList.RemoveDuplicates();
			}
		}
	}
	m_ListCtrl.ResetChecked(path);

	// Always start the timer, since the status of an existing item might have changed
	SetTimer(REFRESHTIMER, 200, nullptr);
	CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Item %s dropped, timer started\n", path.GetWinPath());
	return 0;
}

LRESULT CCommitDlg::OnAutoListReady(WPARAM, LPARAM lparam)
{
	m_cLogMessage.SetAutoCompletionList(std::move(*reinterpret_cast<std::map<CString, int>*>(lparam)), '*');
	return 0;
}

//////////////////////////////////////////////////////////////////////////
// functions which run in the status thread
//////////////////////////////////////////////////////////////////////////

void CCommitDlg::ParseRegexFile(const CString& sFile, std::map<CString, CString>& mapRegex)
{
	CString strLine;
	try
	{
		CStdioFile file(sFile, CFile::typeText | CFile::modeRead | CFile::shareDenyWrite);
		while (m_bRunThread && file.ReadString(strLine))
		{
			if (strLine.IsEmpty())
				continue;
			if (strLine[0] == L'#')
				continue;
			const int eqpos = strLine.Find('=');
			CString rgx;
			rgx = strLine.Mid(eqpos+1).Trim();

			int pos = -1;
			while (((pos = strLine.Find(','))>=0)&&(pos < eqpos))
			{
				mapRegex[strLine.Left(pos)] = rgx;
				strLine = strLine.Mid(pos+1).Trim();
			}
			mapRegex[strLine.Left(strLine.Find('=')).Trim()] = rgx;
		}
		file.Close();
	}
	catch (CFileException* pE)
	{
		CTraceToOutputDebugString::Instance()(__FUNCTION__ ": CFileException loading auto list regex file\n");
		pE->Delete();
	}
}

void CCommitDlg::ParseSnippetFile(const CString& sFile, std::map<CString, CString>& mapSnippet)
{
	CString strLine;
	try
	{
		CStdioFile file(sFile, CFile::typeText | CFile::modeRead | CFile::shareDenyWrite);
		while (m_bRunThread && file.ReadString(strLine))
		{
			if (strLine.IsEmpty())
				continue;
			if (strLine[0] == L'#') // comment char
				continue;
			const int eqpos = strLine.Find(L'=');
			if (eqpos <= 0)
				continue;
			CString key = strLine.Left(eqpos);
			auto rawvalue = strLine.GetBuffer() + eqpos + 1;
			CString value;
			bool isEscaped = false;
			for (int i = 0, len = strLine.GetLength() - eqpos - 1; i < len; ++i)
			{
				wchar_t c = rawvalue[i];
				if (isEscaped)
				{
					switch (c)
					{
					case L't': c = L'\t'; break;
					case L'n': c = L'\n'; break;
					case L'r': c = L'\r'; break;
					case L'\\': c = L'\\'; break;
					default: value += rawvalue[i - 1]; break; // i is known to be > 0
					}
					value += c;
					isEscaped = false;
				}
				else if (c == L'\\')
					isEscaped = true;
				else
					value += c;
			}
			mapSnippet[key] = value;
		}
		file.Close();
	}
	catch (CFileException* pE)
	{
		CTraceToOutputDebugString::Instance()(__FUNCTION__ ": CFileException loading auto list regex file\n");
		pE->Delete();
	}
}

void CCommitDlg::GetAutocompletionList(std::map<CString, int>& autolist)
{
	// the auto completion list is made of strings from each selected files.
	// the strings used are extracted from the files with regexes found
	// in the file "autolist.txt".
	// the format of that file is:
	// file extensions separated with commas '=' regular expression to use
	// example:
	// .h, .hpp = (?<=class[\s])\b\w+\b|(\b\w+(?=[\s ]?\(\);))
	// .cpp = (?<=[^\s]::)\b\w+\b

	std::map<CString, CString> mapRegex;
	CString sRegexFile = CPathUtils::GetAppDirectory();
	CRegDWORD regtimeout = CRegDWORD(L"Software\\TortoiseGit\\AutocompleteParseTimeout", 5);
	ULONGLONG timeoutvalue = ULONGLONG(DWORD(regtimeout)) * 1000UL;;
	sRegexFile += L"autolist.txt";
	if (!m_bRunThread)
		return;
	ParseRegexFile(sRegexFile, mapRegex);
	sRegexFile = CPathUtils::GetAppDataDirectory();
	sRegexFile += L"autolist.txt";
	if (PathFileExists(sRegexFile))
		ParseRegexFile(sRegexFile, mapRegex);

	m_snippet.clear();
	CString sSnippetFile = CPathUtils::GetAppDirectory();
	sSnippetFile += L"snippet.txt";
	ParseSnippetFile(sSnippetFile, m_snippet);
	sSnippetFile = CPathUtils::GetAppDataDirectory();
	sSnippetFile += L"snippet.txt";
	if (PathFileExists(sSnippetFile))
		ParseSnippetFile(sSnippetFile, m_snippet);
	for (const auto& snip : m_snippet)
		autolist.emplace(snip.first, AUTOCOMPLETE_SNIPPET);

	ULONGLONG starttime = GetTickCount64();

	// now we have two arrays of strings, where the first array contains all
	// file extensions we can use and the second the corresponding regex strings
	// to apply to those files.

	// the next step is to go over all files shown in the commit dialog
	// and scan them for strings we can use
	const int nListItems = m_ListCtrl.GetItemCount();

	for (int i=0; i<nListItems && m_bRunThread; ++i)
	{
		// stop parsing after timeout
		if ((!m_bRunThread) || (GetTickCount64() - starttime > timeoutvalue))
			return;

		CString sWinPath;
		CString sPartPath;
		CString sExt;
		int action;
		{
			auto locker(m_ListCtrl.AcquireReadLock());
			auto path = m_ListCtrl.GetListEntry(i);

			if (!path)
				continue;

			sWinPath = path->GetWinPathString();
			sPartPath = path->GetGitPathString();
			sExt = path->GetFileExtension();
			action = path->m_Action;
		}
		autolist.emplace(sPartPath, AUTOCOMPLETE_FILENAME);

		int pos = 0;
		int lastPos = 0;
		while ((pos = sPartPath.Find('/', pos)) >= 0)
		{
			++pos;
			lastPos = pos;
			autolist.emplace(sPartPath.Mid(pos), AUTOCOMPLETE_FILENAME);
		}

		// Last inserted entry is a file name.
		// Some users prefer to also list file name without extension.
		if (CRegDWORD(L"Software\\TortoiseGit\\AutocompleteRemovesExtensions", FALSE))
		{
			const int dotPos = sPartPath.ReverseFind('.');
			if ((dotPos >= 0) && (dotPos > lastPos))
				autolist.emplace(sPartPath.Mid(lastPos, dotPos - lastPos), AUTOCOMPLETE_FILENAME);
		}

		if (action == CTGitPath::LOGACTIONS_UNVER && !CRegDWORD(L"Software\\TortoiseGit\\AutocompleteParseUnversioned", FALSE))
			continue;
		if (action == CTGitPath::LOGACTIONS_IGNORE)
			continue;

		sExt.MakeLower();
		// find the regex string which corresponds to the file extension
		CString rdata = mapRegex[sExt];
		if (rdata.IsEmpty())
			continue;

		ScanFile(autolist, sWinPath, rdata, sExt);
	}
	CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Auto completion list loaded in %I64u msec\n", GetTickCount64() - starttime);
}

void CCommitDlg::ScanFile(std::map<CString, int>& autolist, const CString& sFilePath, const CString& sRegex, const CString& sExt)
{
	static std::map<CString, std::wregex> regexmap;

	CAutoFile hFile = CreateFile(sFilePath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (!hFile)
		return;

	LARGE_INTEGER fileSize;
	if (GetFileSizeEx(hFile, &fileSize); fileSize.QuadPart == 0 || fileSize.QuadPart >= INT_MAX || fileSize.QuadPart > CRegDWORD(L"Software\\TortoiseGit\\AutocompleteParseMaxSize", 300000L))
	{
		// no empty files or files bigger than a configurable maximum (default: 300k)
		return;
	}
	// allocate memory to hold file contents
	std::unique_ptr<BYTE[]> fileBuffer;
	try
	{
		fileBuffer = std::unique_ptr<BYTE[]>(new BYTE[fileSize.LowPart]); // prevent default initialization
	}
	catch (CMemoryException*)
	{
		return;
	}
	DWORD readbytes;
	if (!ReadFile(hFile, fileBuffer.get(), fileSize.LowPart, &readbytes, nullptr))
		return;
	CFileTextLines filetextlines;
	CFileTextLines::UnicodeType type = filetextlines.CheckUnicodeType(fileBuffer.get(), readbytes);
	std::unique_ptr<CDecodeFilter> pFilter;
	try
	{
		switch (type)
		{
		case CFileTextLines::UnicodeType::BINARY:
			return;
		case CFileTextLines::UnicodeType::UTF8:
		case CFileTextLines::UnicodeType::UTF8BOM:
			pFilter = std::make_unique<CUtf8Filter>(nullptr);
			break;
		default:
		case CFileTextLines::UnicodeType::ASCII:
			pFilter = std::make_unique<CAsciiFilter>(nullptr);
			break;
		case CFileTextLines::UnicodeType::UTF16_BE:
		case CFileTextLines::UnicodeType::UTF16_BEBOM:
			pFilter = std::make_unique<CUtf16beFilter>(nullptr);
			break;
		case CFileTextLines::UnicodeType::UTF16_LE:
		case CFileTextLines::UnicodeType::UTF16_LEBOM:
			pFilter = std::make_unique<CUtf16leFilter>(nullptr);
			break;
		case CFileTextLines::UnicodeType::UTF32_BE:
			pFilter = std::make_unique<CUtf32beFilter>(nullptr);
			break;
		case CFileTextLines::UnicodeType::UTF32_LE:
			pFilter = std::make_unique<CUtf32leFilter>(nullptr);
			break;
		}
		if (!pFilter->Decode(std::move(fileBuffer), readbytes))
			return;
	}
	catch (CMemoryException*)
	{
		return;
	}

	std::wstring_view sFileContent = pFilter->GetStringView();
	if (sFileContent.empty() || !m_bRunThread)
		return;

	try
	{
		std::wregex regCheck;
		std::map<CString, std::wregex>::const_iterator regIt;
		if ((regIt = regexmap.find(sExt)) != regexmap.end())
			regCheck = regIt->second;
		else
		{
			regCheck = std::wregex(sRegex, std::regex_constants::icase | std::regex_constants::ECMAScript);
			regexmap[sExt] = regCheck;
		}
		const std::wcregex_iterator end;
		for (std::wcregex_iterator it(sFileContent.data(), sFileContent.data() + sFileContent.size(), regCheck); it != end; ++it)
		{
			const std::wcmatch match = *it;
			for (size_t i = 1; i < match.size(); ++i)
			{
				if (match[i].second-match[i].first)
					autolist.emplace(std::wstring(match[i]).c_str(), AUTOCOMPLETE_PROGRAMCODE);
			}
		}
	}
	catch (std::exception&) {}
}

// CSciEditContextMenuInterface
void CCommitDlg::InsertMenuItems(CMenu& mPopup, int& nCmd)
{
	CString sMenuItemText;
	sMenuItemText.LoadString(IDS_COMMITDLG_POPUP_PICKCOMMITHASH);
	m_nPopupPickCommitHash = nCmd++;
	mPopup.AppendMenu(MF_STRING | MF_ENABLED, m_nPopupPickCommitHash, sMenuItemText);

	sMenuItemText.LoadString(IDS_COMMITDLG_POPUP_PICKCOMMITMESSAGE);
	m_nPopupPickCommitMessage = nCmd++;
	mPopup.AppendMenu(MF_STRING | MF_ENABLED, m_nPopupPickCommitMessage, sMenuItemText);

	sMenuItemText.LoadString(IDS_COMMITDLG_POPUP_PASTEFILELIST);
	m_nPopupPasteListCmd = nCmd++;
	mPopup.AppendMenu(MF_STRING | MF_ENABLED, m_nPopupPasteListCmd, sMenuItemText);

	ReloadHistoryEntries();
	if (!m_History.IsEmpty())
	{
		sMenuItemText.LoadString(IDS_COMMITDLG_POPUP_PASTELASTMESSAGE);
		m_nPopupPasteLastMessage = nCmd++;
		mPopup.AppendMenu(MF_STRING | MF_ENABLED, m_nPopupPasteLastMessage, sMenuItemText);

		sMenuItemText.LoadString(IDS_COMMITDLG_POPUP_LOGHISTORY);
		m_nPopupRecentMessage = nCmd++;
		mPopup.AppendMenu(MF_STRING | MF_ENABLED, m_nPopupRecentMessage, sMenuItemText);
	}
}

bool CCommitDlg::HandleMenuItemClick(int cmd, CSciEdit * pSciEdit)
{
	if (cmd == m_nPopupPickCommitHash)
	{
		// use the git log to allow selection of a version
		CLogDlg dlg;
		if (dlg.IsThreadRunning())
		{
			CMessageBox::Show(GetSafeHwnd(), IDS_PROC_LOG_ONLYONCE, IDS_APPNAME, MB_ICONEXCLAMATION);
			return true;
		}
		// tell the dialog to use mode for selecting revisions
		dlg.SetSelect(true);
		// only one revision must be selected however
		dlg.SingleSelection(true);
		dlg.ShowWorkingTreeChanges(false);
		if (dlg.DoModal() == IDOK && !dlg.GetSelectedHash().empty())
			pSciEdit->InsertText(dlg.GetSelectedHash().at(0).ToString());
		BringWindowToTop(); /* cf. issue #3493 */
		return true;
	}

	if (cmd == m_nPopupPickCommitMessage)
	{
		// use the git log to allow selection of a version
		CLogDlg dlg;
		if (dlg.IsThreadRunning())
		{
			CMessageBox::Show(GetSafeHwnd(), IDS_PROC_LOG_ONLYONCE, IDS_APPNAME, MB_ICONEXCLAMATION);
			return true;
		}
		// tell the dialog to use mode for selecting revisions
		dlg.SetSelect(true);
		// only one revision must be selected however
		dlg.SingleSelection(true);
		dlg.ShowWorkingTreeChanges(false);
		if (dlg.DoModal() == IDOK && !dlg.GetSelectedHash().empty())
		{
			GitRev rev;
			if (rev.GetCommit(dlg.GetSelectedHash().at(0).ToString()))
			{
				MessageBox(rev.GetLastErr(), L"TortoiseGit", MB_ICONERROR);
				return false;
			}
			CString message = rev.GetSubject() + L"\r\n" + rev.GetBody();
			pSciEdit->InsertText(message);
		}
		BringWindowToTop(); /* cf. issue #3493 */
		return true;
	}

	if (cmd == m_nPopupPasteListCmd)
	{
		CString logmsg;
		auto locker(m_ListCtrl.AcquireReadLock());
		const int nListItems = m_ListCtrl.GetItemCount();
		for (int i=0; i<nListItems; ++i)
		{
			auto entry = m_ListCtrl.GetListEntry(i);
			if (entry&&entry->m_Checked)
			{
				CString status = entry->GetActionName();
				if(entry->m_Action & CTGitPath::LOGACTIONS_UNVER)
					status = CTGitPath::GetActionName(CTGitPath::LOGACTIONS_ADDED);

				//git_wc_status_kind status = entry->status;
				WORD langID = static_cast<WORD>(CRegStdDWORD(L"Software\\TortoiseGit\\LanguageID", GetUserDefaultLangID()));
				if (m_ProjectProperties.bFileListInEnglish)
					langID = 1033;

				logmsg.AppendFormat(L"%-10s %s\r\n", static_cast<LPCWSTR>(status), static_cast<LPCWSTR>(m_ListCtrl.GetItemText(i, 0)));
			}
		}
		pSciEdit->InsertText(logmsg);
		return true;
	}

	if(cmd == m_nPopupPasteLastMessage)
	{
		if (m_History.IsEmpty())
			return false;

		CString logmsg;
		logmsg +=m_History.GetEntry(0);
		pSciEdit->InsertText(logmsg);
		return true;
	}

	if(cmd == m_nPopupRecentMessage )
	{
		OnBnClickedHistory();
		return true;
	}
	return false;
}

void CCommitDlg::HandleSnippet(int type, const CString &text, CSciEdit *pSciEdit)
{
	if (type == AUTOCOMPLETE_SNIPPET)
	{
		CString target = m_snippet[text];
		pSciEdit->GetWordUnderCursor(true);
		pSciEdit->InsertText(target, false);
	}
}

void CCommitDlg::OnTimer(UINT_PTR nIDEvent)
{
	switch (nIDEvent)
	{
	case ENDDIALOGTIMER:
		KillTimer(ENDDIALOGTIMER);
		EndDialog(0);
		break;
	case REFRESHTIMER:
		if (m_bThreadRunning)
		{
			SetTimer(REFRESHTIMER, 200, nullptr);
			CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Wait some more before refreshing\n");
		}
		else
		{
			KillTimer(REFRESHTIMER);
			CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Refreshing after items dropped\n");
			Refresh();
		}
		break;
	case FILLPATCHVTIMER:
		FillPatchView();
		break;
	}
	__super::OnTimer(nIDEvent);
}

void CCommitDlg::OnBnClickedHistory()
{
	m_tooltips.Pop();	// hide the tooltips
	if (m_pathList.IsEmpty())
		return;

	CHistoryDlg historyDlg;
	ReloadHistoryEntries();
	historyDlg.SetHistory(m_History);
	if (historyDlg.DoModal() != IDOK)
		return;

	CString sMsg = historyDlg.GetSelectedText();
	if (sMsg != m_cLogMessage.GetText().Left(sMsg.GetLength()))
	{
		CString sBugID = m_ProjectProperties.FindBugID(sMsg);
		if ((!sBugID.IsEmpty()) && ((GetDlgItem(IDC_BUGID)->IsWindowVisible())))
			SetDlgItemText(IDC_BUGID, sBugID);
		if (m_sLogTemplate.Compare(m_cLogMessage.GetText()) != 0)
			m_cLogMessage.InsertText(sMsg, !m_cLogMessage.GetText().IsEmpty());
		else
			m_cLogMessage.SetText(sMsg);
	}

	SendMessage(WM_UPDATEOKBUTTON);
	GetDlgItem(IDC_LOGMESSAGE)->SetFocus();
}

void CCommitDlg::OnBnClickedBugtraqbutton()
{
	m_tooltips.Pop();	// hide the tooltips
	CString sMsg = m_cLogMessage.GetText();

	if (!m_BugTraqProvider)
		return;

	ATL::CComBSTR parameters(m_bugtraq_association.GetParameters());
	ATL::CComBSTR commonRoot(m_pathList.GetCommonRoot().GetDirectory().GetWinPath());
	CBstrSafeVector pathList(m_pathList.GetCount());

	for (LONG index = 0; index < m_pathList.GetCount(); ++index)
		pathList.PutElement(index, m_pathList[index].GetGitPathString());

	ATL::CComBSTR originalMessage(sMsg);
	ATL::CComBSTR temp;

	// first try the IBugTraqProvider2 interface
	CComPtr<IBugTraqProvider2> pProvider2;
	HRESULT hr = m_BugTraqProvider.QueryInterface(&pProvider2);
	bool bugIdOutSet = false;
	if (SUCCEEDED(hr))
	{
		ATL::CComBSTR repositoryRoot(g_Git.m_CurrentDir);
		ATL::CComBSTR bugIDOut;
		GetDlgItemText(IDC_BUGID, m_sBugID);
		ATL::CComBSTR bugID(m_sBugID);
		CBstrSafeVector revPropNames;
		CBstrSafeVector revPropValues;
		if (FAILED(hr = pProvider2->GetCommitMessage2(GetSafeHwnd(), parameters, repositoryRoot, commonRoot, pathList, originalMessage, bugID, &bugIDOut, &revPropNames, &revPropValues, &temp)))
		{
			CString sErr;
			sErr.FormatMessage(IDS_ERR_FAILEDISSUETRACKERCOM, static_cast<LPCWSTR>(m_bugtraq_association.GetProviderName()), _com_error(hr).ErrorMessage());
			CMessageBox::Show(GetSafeHwnd(), sErr, L"TortoiseGit", MB_ICONERROR);
		}
		else
		{
			if (bugIDOut)
			{
				bugIdOutSet = true;
				m_sBugID = bugIDOut;
				SetDlgItemText(IDC_BUGID, m_sBugID);
			}
			m_cLogMessage.SetText(static_cast<LPCWSTR>(temp));
		}
	}
	else
	{
		// if IBugTraqProvider2 failed, try IBugTraqProvider
		CComPtr<IBugTraqProvider> pProvider;
		hr = m_BugTraqProvider.QueryInterface(&pProvider);
		if (FAILED(hr))
		{
			CString sErr;
			sErr.FormatMessage(IDS_ERR_FAILEDISSUETRACKERCOM, static_cast<LPCWSTR>(m_bugtraq_association.GetProviderName()), _com_error(hr).ErrorMessage());
			CMessageBox::Show(GetSafeHwnd(), sErr, L"TortoiseGit", MB_ICONERROR);
			return;
		}

		if (FAILED(hr = pProvider->GetCommitMessage(GetSafeHwnd(), parameters, commonRoot, pathList, originalMessage, &temp)))
		{
			CString sErr;
			sErr.FormatMessage(IDS_ERR_FAILEDISSUETRACKERCOM, static_cast<LPCWSTR>(m_bugtraq_association.GetProviderName()), _com_error(hr).ErrorMessage());
			CMessageBox::Show(GetSafeHwnd(), sErr, L"TortoiseGit", MB_ICONERROR);
		}
		else
			m_cLogMessage.SetText(static_cast<LPCWSTR>(temp));
	}
	m_sLogMessage = m_cLogMessage.GetText();
	if (!m_ProjectProperties.sMessage.IsEmpty())
	{
		CString sBugID = m_ProjectProperties.FindBugID(m_sLogMessage);
		if (!sBugID.IsEmpty() && !bugIdOutSet)
			SetDlgItemText(IDC_BUGID, sBugID);
	}

	m_cLogMessage.SetFocus();
}

void CCommitDlg::FillPatchView(bool onlySetTimer)
{
	if(::IsWindow(this->m_patchViewdlg.m_hWnd))
	{
		KillTimer(FILLPATCHVTIMER);
		if (onlySetTimer)
		{
			SetTimer(FILLPATCHVTIMER, 100, nullptr);
			return;
		}

		auto locker(m_ListCtrl.AcquireReadWeakLock(50));
		if (!locker.IsAcquired())
		{
			SetTimer(FILLPATCHVTIMER, 100, nullptr);
			return;
		}
		POSITION pos=m_ListCtrl.GetFirstSelectedItemPosition();
		CString cmd,out;

		CString head = L"HEAD";
		if (m_bCommitAmend == TRUE && m_bAmendDiffToLastCommit == FALSE)
			head = L"HEAD~1";
		while(pos)
		{
			const int nSelect = m_ListCtrl.GetNextSelectedItem(pos);
			auto p = m_ListCtrl.GetListEntry(nSelect);
			if(p && !(p->m_Action&CTGitPath::LOGACTIONS_UNVER) )
			{
				if (m_bStagingSupport)
				{
					// This will only work if called after ShowPartialStagingTextAndUpdateDisplayStatus (or Unstaging)

					if (!(p->m_Action & CTGitPath::LOGACTIONS_ADDED) && !(p->m_Action & CTGitPath::LOGACTIONS_DELETED) && !(p->m_Action & CTGitPath::LOGACTIONS_MISSING) && !(p->m_Action & CTGitPath::LOGACTIONS_UNMERGED) && !(p->IsDirectory()))
					{
						if (!(m_stagingDisplayState & SHOW_STAGING))        // link does not currently display show staging, then it's "hide staging", meaning the staging window is open
							m_patchViewdlg.EnableStaging(EnableStagingTypes::Staging);
						else if (!(m_stagingDisplayState & SHOW_UNSTAGING)) // link does not currently display show unstaging, then it's "hide unstaging", meaning the unstaging window is open
							m_patchViewdlg.EnableStaging(EnableStagingTypes::Unstaging);
					}
					else
						m_patchViewdlg.EnableStaging(EnableStagingTypes::None);

					bool useCachedParameter = false;
					if (!(m_stagingDisplayState & SHOW_UNSTAGING)) // link does not currently display show unstaging, then it's "hide unstaging", meaning the unstaging window is open
						useCachedParameter = true;
					else
						head.Empty();

					if (!p->GetGitOldPathString().IsEmpty())
						cmd.Format(L"git.exe diff %s%s -- \"%s\" \"%s\"", static_cast<LPCWSTR>(head), useCachedParameter ? L" --cached" : L"", static_cast<LPCWSTR>(p->GetGitOldPathString()), static_cast<LPCWSTR>(p->GetGitPathString()));
					else
						cmd.Format(L"git.exe diff %s%s -- \"%s\"", static_cast<LPCWSTR>(head), useCachedParameter ? L" --cached" : L"", static_cast<LPCWSTR>(p->GetGitPathString()));
				}
				else
				{
					if (!p->GetGitOldPathString().IsEmpty())
						cmd.Format(L"git.exe diff %s -- \"%s\" \"%s\"", static_cast<LPCWSTR>(head), static_cast<LPCWSTR>(p->GetGitOldPathString()), static_cast<LPCWSTR>(p->GetGitPathString()));
					else
						cmd.Format(L"git.exe diff %s -- \"%s\"", static_cast<LPCWSTR>(head), static_cast<LPCWSTR>(p->GetGitPathString()));
				}
				g_Git.Run(cmd, &out, CP_UTF8);
			}
			else
				m_patchViewdlg.EnableStaging(EnableStagingTypes::None);
			if (m_bStagingSupport)
				break; // only one file at a time (the hunk staging code supports many files at once, so it's possible to relax this restriction for that case)
		}

		m_patchViewdlg.SetText(out);
	}
}
LRESULT CCommitDlg::OnGitStatusListCtrlItemChanged(WPARAM /*wparam*/, LPARAM /*lparam*/)
{
	// This handler is blocked during a partial staging, because OnPartialStagingRefreshPatchView itself calls FillPatchView
	if (!m_nBlockItemChangeHandler)
		this->FillPatchView(true);
	return 0;
}


LRESULT CCommitDlg::OnGitStatusListCtrlCheckChanged(WPARAM, LPARAM)
{
	SendMessage(WM_UPDATEOKBUTTON);
	return 0;
}

LRESULT CCommitDlg::OnCheck(WPARAM wnd, LPARAM)
{
	HWND hwnd = reinterpret_cast<HWND>(wnd);
	const bool check = !(GetAsyncKeyState(VK_SHIFT) & 0x8000);
	if (hwnd == GetDlgItem(IDC_CHECKALL)->GetSafeHwnd())
		m_ListCtrl.Check(GITSLC_SHOWEVERYTHING, check);
	else if (hwnd == GetDlgItem(IDC_CHECKNONE)->GetSafeHwnd())
		m_ListCtrl.Check(GITSLC_SHOWEVERYTHING, !check);
	else if (hwnd == GetDlgItem(IDC_CHECKUNVERSIONED)->GetSafeHwnd())
		m_ListCtrl.Check(GITSLC_SHOWUNVERSIONED, check);
	else if (hwnd == GetDlgItem(IDC_CHECKVERSIONED)->GetSafeHwnd())
		m_ListCtrl.Check(GITSLC_SHOWVERSIONED, check);
	else if (hwnd == GetDlgItem(IDC_CHECKADDED)->GetSafeHwnd())
		m_ListCtrl.Check(GITSLC_SHOWADDED, check);
	else if (hwnd == GetDlgItem(IDC_CHECKDELETED)->GetSafeHwnd())
		m_ListCtrl.Check(GITSLC_SHOWREMOVED, check);
	else if (hwnd == GetDlgItem(IDC_CHECKMODIFIED)->GetSafeHwnd())
		m_ListCtrl.Check(GITSLC_SHOWMODIFIED, check);
	else if (hwnd == GetDlgItem(IDC_CHECKFILES)->GetSafeHwnd())
		m_ListCtrl.Check(GITSLC_SHOWFILES, check);
	else if (hwnd == GetDlgItem(IDC_CHECKSUBMODULES)->GetSafeHwnd())
		m_ListCtrl.Check(GITSLC_SHOWSUBMODULES, check);

	return 0;
}

LRESULT CCommitDlg::OnUpdateOKButton(WPARAM, LPARAM)
{
	if (m_bBlock)
		return 0;

	CString text = m_cLogMessage.GetText().Trim();
	const bool bValidLogSize = !text.IsEmpty() && text.GetLength() >= m_ProjectProperties.nMinLogSize;
	const bool bAmendOrSelectFilesOrMerge = m_ListCtrl.GetSelected() > 0 || (m_bCommitAmend && m_bAmendDiffToLastCommit) || CTGitPath(g_Git.m_CurrentDir).IsMergeActive();

	DialogEnableWindow(IDOK, bValidLogSize && (m_bCommitMessageOnly || bAmendOrSelectFilesOrMerge));

	return 0;
}

LRESULT CCommitDlg::OnUpdateDataFalse(WPARAM, LPARAM)
{
	UpdateData(FALSE);
	return 0;
}

LRESULT CCommitDlg::OnPartialStagingRefreshPatchView(WPARAM wParam, LPARAM)
{
	m_patchViewdlg.ClearView();

	{
		// Block the item change handler to make sure FillPatchView is called only once (below)
		ScopedInDecrement blocker(m_nBlockItemChangeHandler);
		CTGitPath::StagingStatus newStatus = static_cast<CTGitPath::StagingStatus>(wParam);
		m_ListCtrl.UpdateSelectedFileStagingStatus(newStatus);
	}

	FillPatchView();
	SendMessage(WM_UPDATEOKBUTTON);
	return 0;
}

LRESULT CCommitDlg::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_NOTIFY:
		if (wParam == IDC_SPLITTER)
		{
			auto pHdr = reinterpret_cast<SPC_NMHDR*>(lParam);
			DoSize(pHdr->delta);
		}
		break;
	case WM_COMMAND:
		if (m_hAccelOkButton && LOWORD(wParam) >= WM_USER && LOWORD(wParam) <= WM_USER + m_accellerators.size())
		{
			for (const auto& entry : m_accellerators)
			{
				if (entry.second.wmid != LOWORD(wParam))
					continue;
				if (entry.second.id == -1)
					m_ctrlOkButton.PostMessage(WM_KEYDOWN, VK_F4, NULL);
				else
				{
					m_ctrlOkButton.SetCurrentEntry(entry.second.id);
					OnOK();
				}
				return 0;
			}
		}
		break;
	}

	return __super::DefWindowProc(message, wParam, lParam);
}

void CCommitDlg::SetSplitterRange()
{
	if ((m_ListCtrl)&&(m_cLogMessage))
	{
		CRect rcTop;
		m_cLogMessage.GetWindowRect(rcTop);
		ScreenToClient(rcTop);
		CRect rcMiddle;
		m_ListCtrl.GetWindowRect(rcMiddle);
		ScreenToClient(rcMiddle);
		if (rcMiddle.Height() && rcMiddle.Width())
			m_wndSplitter.SetRange(rcTop.top + CDPIAware::Instance().ScaleY(GetSafeHwnd(), 120), rcMiddle.bottom - CDPIAware::Instance().ScaleY(GetSafeHwnd(), 80));
	}
}

void CCommitDlg::DoSize(int delta)
{
	RemoveAnchor(IDC_MESSAGEGROUP);
	RemoveAnchor(IDC_LOGMESSAGE);
	RemoveAnchor(IDC_SPLITTER);
	RemoveAnchor(IDC_SIGNOFF);
	RemoveAnchor(IDC_COMMIT_AMEND);
	RemoveAnchor(IDC_COMMIT_AMENDDIFF);
	RemoveAnchor(IDC_COMMIT_SETDATETIME);
	RemoveAnchor(IDC_COMMIT_DATEPICKER);
	RemoveAnchor(IDC_COMMIT_TIMEPICKER);
	RemoveAnchor(IDC_COMMIT_AS_COMMIT_DATE);
	RemoveAnchor(IDC_COMMIT_SETAUTHOR);
	RemoveAnchor(IDC_COMMIT_AUTHORDATA);
	RemoveAnchor(IDC_LISTGROUP);
	RemoveAnchor(IDC_FILELIST);
	RemoveAnchor(IDC_TEXT_INFO);
	RemoveAnchor(IDC_SELECTLABEL);
	RemoveAnchor(IDC_CHECKALL);
	RemoveAnchor(IDC_CHECKNONE);
	RemoveAnchor(IDC_CHECKUNVERSIONED);
	RemoveAnchor(IDC_CHECKVERSIONED);
	RemoveAnchor(IDC_CHECKADDED);
	RemoveAnchor(IDC_CHECKDELETED);
	RemoveAnchor(IDC_CHECKMODIFIED);
	RemoveAnchor(IDC_CHECKFILES);
	RemoveAnchor(IDC_CHECKSUBMODULES);

	auto hdwp = BeginDeferWindowPos(24);
	CSplitterControl::ChangeRect(hdwp, &m_cLogMessage, 0, 0, 0, delta);
	CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_MESSAGEGROUP), 0, 0, 0, delta);
	CSplitterControl::ChangeRect(hdwp, &m_ListCtrl, 0, delta, 0, 0);
	CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_LISTGROUP), 0, delta, 0, 0);
	CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_SIGNOFF), 0, delta, 0, delta);
	CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_COMMIT_AMEND), 0, delta, 0, delta);
	CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_COMMIT_AMENDDIFF), 0, delta, 0, delta);
	CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_COMMIT_SETDATETIME), 0, delta, 0, delta);
	CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_COMMIT_DATEPICKER), 0, delta, 0, delta);
	CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_COMMIT_TIMEPICKER), 0, delta, 0, delta);
	CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_COMMIT_AS_COMMIT_DATE), 0, delta, 0, delta);
	CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_COMMIT_SETAUTHOR), 0, delta, 0, delta);
	CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_COMMIT_AUTHORDATA), 0, delta, 0, delta);
	CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_TEXT_INFO), 0, delta, 0, delta);
	CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_SELECTLABEL), 0, delta, 0, delta);
	CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_CHECKALL), 0, delta, 0, delta);
	CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_CHECKNONE), 0, delta, 0, delta);
	CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_CHECKUNVERSIONED), 0, delta, 0, delta);
	CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_CHECKVERSIONED), 0, delta, 0, delta);
	CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_CHECKADDED), 0, delta, 0, delta);
	CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_CHECKDELETED), 0, delta, 0, delta);
	CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_CHECKMODIFIED), 0, delta, 0, delta);
	CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_CHECKFILES), 0, delta, 0, delta);
	CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_CHECKSUBMODULES), 0, delta, 0, delta);
	EndDeferWindowPos(hdwp);

	AddAnchor(IDC_MESSAGEGROUP, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_LOGMESSAGE, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_SPLITTER, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_LISTGROUP, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_FILELIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SIGNOFF,TOP_RIGHT);
	AddAnchor(IDC_COMMIT_AMEND,TOP_LEFT);
	AddAnchor(IDC_COMMIT_AMENDDIFF,TOP_LEFT);
	AddAnchor(IDC_COMMIT_SETDATETIME,TOP_LEFT);
	AddAnchor(IDC_COMMIT_DATEPICKER,TOP_LEFT);
	AddAnchor(IDC_COMMIT_TIMEPICKER,TOP_LEFT);
	AddAnchor(IDC_COMMIT_AS_COMMIT_DATE, TOP_LEFT);
	AddAnchor(IDC_COMMIT_SETAUTHOR, TOP_LEFT);
	AddAnchor(IDC_COMMIT_AUTHORDATA, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_TEXT_INFO,TOP_RIGHT);
	AddAnchor(IDC_SELECTLABEL, TOP_LEFT);
	AddAnchor(IDC_CHECKALL, TOP_LEFT);
	AddAnchor(IDC_CHECKNONE, TOP_LEFT);
	AddAnchor(IDC_CHECKUNVERSIONED, TOP_LEFT);
	AddAnchor(IDC_CHECKVERSIONED, TOP_LEFT);
	AddAnchor(IDC_CHECKADDED, TOP_LEFT);
	AddAnchor(IDC_CHECKDELETED, TOP_LEFT);
	AddAnchor(IDC_CHECKMODIFIED, TOP_LEFT);
	AddAnchor(IDC_CHECKFILES, TOP_LEFT);
	AddAnchor(IDC_CHECKSUBMODULES, TOP_LEFT);
	ArrangeLayout();
	// adjust the minimum size of the dialog to prevent the resizing from
	// moving the list control too far down.
	CRect rcLogMsg;
	m_cLogMessage.GetClientRect(rcLogMsg);
	SetMinTrackSize(CSize(m_DlgOrigRect.Width(), m_DlgOrigRect.Height()-m_LogMsgOrigRect.Height()+rcLogMsg.Height()));

	SetSplitterRange();
	m_cLogMessage.Invalidate();
	GetDlgItem(IDC_LOGMESSAGE)->Invalidate();
}

void CCommitDlg::OnSize(UINT nType, int cx, int cy)
{
	// first, let the resizing take place
	__super::OnSize(nType, cx, cy);

	//set range
	SetSplitterRange();
}

CString CCommitDlg::GetSignedOffByLine()
{
	CString str;

	CString username = g_Git.GetUserName();
	CString email = g_Git.GetUserEmail();
	username.Remove(L'\n');
	email.Remove(L'\n');

	str.Format(L"Signed-off-by: %s <%s>", static_cast<LPCWSTR>(username), static_cast<LPCWSTR>(email));

	return str;
}

void CCommitDlg::OnBnClickedSignOff()
{
	CString str = GetSignedOffByLine();

	if (m_cLogMessage.GetText().Find(str) == -1) {
		m_cLogMessage.SetText(m_cLogMessage.GetText().TrimRight());
		const int lastNewline = m_cLogMessage.GetText().ReverseFind(L'\n');
		int foundByLine = -1;
		if (lastNewline > 0)
			foundByLine = m_cLogMessage.GetText().Find(L"-by: ", lastNewline);

		if (foundByLine == -1 || foundByLine < lastNewline)
			str = L"\r\n" + str;

		m_cLogMessage.SetText(m_cLogMessage.GetText() + L"\r\n" + str + L"\r\n");
	}
}

void CCommitDlg::OnBnClickedCommitAmend()
{
	this->UpdateData();
	if(this->m_bCommitAmend && this->m_AmendStr.IsEmpty())
	{
		GitRev rev;
		if (rev.GetCommit(L"HEAD"))
			MessageBox(rev.GetLastErr(), L"TortoiseGit", MB_ICONERROR);
		m_AmendStr = rev.GetSubject() + L'\n' + rev.GetBody();
	}

	if(this->m_bCommitAmend)
	{
		this->m_NoAmendStr=this->m_cLogMessage.GetText();
		m_cLogMessage.SetText(m_AmendStr);
		DialogEnableWindow(IDC_COMMIT_AMENDDIFF, m_bStagingSupport);
		GetDlgItem(IDC_COMMIT_AMENDDIFF)->ShowWindow(SW_SHOW);
		if (m_bStagingSupport)
			UpdateData(FALSE);
		if (m_bSetCommitDateTime)
		{
			m_AsCommitDateCtrl.EnableWindow(TRUE);
			m_AsCommitDateCtrl.ShowWindow(SW_SHOW);
		}
	}
	else
	{
		this->m_AmendStr=this->m_cLogMessage.GetText();
		m_cLogMessage.SetText(m_NoAmendStr);
		GetDlgItem(IDC_COMMIT_AMENDDIFF)->ShowWindow(SW_HIDE);
		DialogEnableWindow(IDC_COMMIT_AMENDDIFF, FALSE);
		m_AsCommitDateCtrl.ShowWindow(SW_HIDE);
		m_AsCommitDateCtrl.SetCheck(FALSE);
		m_AsCommitDateCtrl.EnableWindow(FALSE);
		OnBnClickedCommitAsCommitDate();
	}

	OnBnClickedCommitSetDateTime(); // to update the commit date and time
	OnBnClickedCommitSetauthor(); // to update the commit author

	GetDlgItem(IDC_LOGMESSAGE)->SetFocus();
	Refresh();
}

void CCommitDlg::OnBnClickedCommitMessageOnly()
{
	this->UpdateData();
	this->m_ListCtrl.EnableWindow(m_bCommitMessageOnly ? FALSE : TRUE);
	SendMessage(WM_UPDATEOKBUTTON);
}

void CCommitDlg::OnBnClickedWholeProject()
{
	m_tooltips.Pop();	// hide the tooltips
	UpdateData();
	m_ListCtrl.Clear();
	if (!m_bBlock)
	{
		if (m_bWholeProject || m_bWholeProject2)
			m_ListCtrl.GetStatus(nullptr, true, false, true, false, false, m_bStagingSupport);
		else
			m_ListCtrl.GetStatus(&this->m_pathList, true, false, true, false, false, m_bStagingSupport);

		m_regShowWholeProject = m_bWholeProject;

		DWORD dwShow = m_ListCtrl.GetShowFlags();
		if (DWORD(m_regAddBeforeCommit))
			dwShow |= GITSLC_SHOWUNVERSIONED;
		else
			dwShow &= ~GITSLC_SHOWUNVERSIONED;

		m_ListCtrl.StoreScrollPos();
		m_ListCtrl.Show(dwShow, dwShow & ~(CTGitPath::LOGACTIONS_UNVER), true);
		UpdateCheckLinks();
	}

	SetDlgTitle();
}

void CCommitDlg::OnFocusMessage()
{
	m_cLogMessage.SetFocus();
}

void CCommitDlg::OnFocusFileList()
{
	m_ListCtrl.SetFocus();
}

void CCommitDlg::OnScnUpdateUI(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	auto pos = static_cast<Sci_Position>(m_cLogMessage.Call(SCI_GETCURRENTPOS));
	auto line = static_cast<int>(m_cLogMessage.Call(SCI_LINEFROMPOSITION, pos));
	auto column = static_cast<int>(m_cLogMessage.Call(SCI_GETCOLUMN, pos));

	CString str;
	str.Format(L"%d/%d", line + 1, column + 1);
	this->GetDlgItem(IDC_TEXT_INFO)->SetWindowText(str);

	if(*pResult)
		*pResult=0;
}

void CCommitDlg::TogglePatchView()
{
	DestroyPatchViewDlgIfOpen();
	if (m_bStagingSupport)
	{
		ShowPartialStagingTextAndUpdateDisplayState(true);
		ShowPartialUnstagingTextAndUpdateDisplayState(true);
	}
	else
		ShowViewPatchText(true);
}

void CCommitDlg::OnStnClickedViewPatch()
{
	if (IsWindow(this->m_patchViewdlg.m_hWnd))
	{
		DestroyPatchViewDlgIfOpen();
		ShowViewPatchText(true);
	}
	else
	{
		CreatePatchViewDlg();
		ShowViewPatchText(false);
		FillPatchView();
	}
}

void CCommitDlg::OnStnClickedPartialStaging()
{
	DestroyPatchViewDlgIfOpen();
	if (m_stagingDisplayState & SHOW_STAGING) // clicked Partial Staging, either with the patch window closed or open in Unstaging mode
	{
		CreatePatchViewDlg();
		m_patchViewdlg.SetWindowText(CString(MAKEINTRESOURCE(IDS_VIEWPATCH_INDEX_WORKTREE)));
		ShowPartialStagingTextAndUpdateDisplayState(false); // change "Partial Staging" to "Hide Staging"
		ShowPartialUnstagingTextAndUpdateDisplayState(true); // show "Partial Unstaging"
		FillPatchView(); // this needs to be called after the two calls to ShowPartial..... above
	}
	else // clicked Hide Staging
	{
		ShowPartialStagingTextAndUpdateDisplayState(true); // change "Hide Staging" to "Partial Staging"
		ShowPartialUnstagingTextAndUpdateDisplayState(true); // show "Partial Unstaging"
	}
}

void CCommitDlg::OnStnClickedPartialUnstaging()
{
	DestroyPatchViewDlgIfOpen();
	if (m_stagingDisplayState & SHOW_UNSTAGING) // clicked Partial Unstaging, either with the patch window closed or open in Staging mode
	{
		CreatePatchViewDlg();
		m_patchViewdlg.SetWindowText(CString(MAKEINTRESOURCE(IDS_VIEWPATCH_HEAD_INDEX)));
		ShowPartialUnstagingTextAndUpdateDisplayState(false); // change "Partial Unstaging" to "Hide Unstaging"
		ShowPartialStagingTextAndUpdateDisplayState(true); // show "Partial Staging"
		FillPatchView(); // this needs to be called after the two calls to ShowPartial..... above
	}
	else // clicked Hide Unstaging
	{
		ShowPartialUnstagingTextAndUpdateDisplayState(true); // change "Hide Unstaging" to "Partial Unstaging"
		ShowPartialStagingTextAndUpdateDisplayState(true); // show "Partial Staging"
	}
}

void CCommitDlg::CreatePatchViewDlg()
{
	m_patchViewdlg.m_ParentDlg = this;
	BOOL viewPatchEnabled = FALSE;
	viewPatchEnabled = g_Git.GetConfigValueBool(L"tgit.commitshowpatch");
	if (viewPatchEnabled == FALSE)
		g_Git.SetConfigValue(L"tgit.commitshowpatch", L"true");
	m_patchViewdlg.Create(IDD_PATCH_VIEW, this);
	m_patchViewdlg.ShowAndAlignToParent();

	GetDlgItem(IDC_LOGMESSAGE)->SetFocus();
}

void CCommitDlg::DestroyPatchViewDlgIfOpen()
{
	if (IsWindow(this->m_patchViewdlg.m_hWnd))
	{
		g_Git.SetConfigValue(L"tgit.commitshowpatch", L"false");
		m_patchViewdlg.ShowWindow(SW_HIDE);
		m_patchViewdlg.EnableWindow(FALSE);
		m_patchViewdlg.DestroyWindow();
	}
}

void CCommitDlg::OnMoving(UINT fwSide, LPRECT pRect)
{
	__super::OnMoving(fwSide, pRect);

	m_patchViewdlg.ParentOnMoving(m_hWnd, pRect);
}

void CCommitDlg::OnSizing(UINT fwSide, LPRECT pRect)
{
	__super::OnSizing(fwSide, pRect);

	m_patchViewdlg.ParentOnSizing(m_hWnd, pRect);
}

void CCommitDlg::OnHdnItemchangedFilelist(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	*pResult = 0;
	TRACE("Item Changed\r\n");
}

int CCommitDlg::CheckHeadDetach()
{
	CString output;
	if (CGit::GetCurrentBranchFromFile(g_Git.m_CurrentDir, output))
	{
		const int retval = CMessageBox::Show(GetSafeHwnd(), IDS_PROC_COMMIT_DETACHEDWARNING, IDS_APPNAME, MB_YESNOCANCEL | MB_ICONWARNING);
		if(retval == IDYES)
		{
			if (CAppUtils::CreateBranchTag(GetSafeHwnd(), FALSE, nullptr, true) == FALSE)
				return 1;
		}
		else if (retval == IDCANCEL)
			return 1;
	}
	return 0;
}

void CCommitDlg::OnBnClickedCommitAmenddiff()
{
	UpdateData();
	Refresh();
}

void CCommitDlg::OnBnClickedNoautoselectsubmodules()
{
	UpdateData();
	Refresh();
}

void CCommitDlg::OnBnClickedCommitSetDateTime()
{
	UpdateData();

	if (m_bSetCommitDateTime)
	{
		CTime authordate = CTime::GetCurrentTime();
		if (m_bCommitAmend)
		{
			GitRev headRevision;
			if (headRevision.GetCommit(L"HEAD"))
				MessageBox(headRevision.GetLastErr(), L"TortoiseGit", MB_ICONERROR);
			authordate = headRevision.GetAuthorDate();
			m_AsCommitDateCtrl.EnableWindow(TRUE);
			m_AsCommitDateCtrl.ShowWindow(SW_SHOW);
		}

		m_CommitDate.SetTime(&authordate);
		m_CommitTime.SetTime(&authordate);

		GetDlgItem(IDC_COMMIT_DATEPICKER)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_COMMIT_TIMEPICKER)->ShowWindow(SW_SHOW);
	}
	else
	{
		GetDlgItem(IDC_COMMIT_DATEPICKER)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_COMMIT_TIMEPICKER)->ShowWindow(SW_HIDE);
		m_AsCommitDateCtrl.ShowWindow(SW_HIDE);
		m_AsCommitDateCtrl.EnableWindow(FALSE);
		m_AsCommitDateCtrl.SetCheck(FALSE);
		OnBnClickedCommitAsCommitDate();
	}
}

void CCommitDlg::OnBnClickedCommitAsCommitDate()
{
	GetDlgItem(IDC_COMMIT_DATEPICKER)->EnableWindow(!m_AsCommitDateCtrl.GetCheck());
	GetDlgItem(IDC_COMMIT_TIMEPICKER)->EnableWindow(!m_AsCommitDateCtrl.GetCheck());
}

void CCommitDlg::OnBnClickedCheckNewBranch()
{
	UpdateData();
	if (m_bCreateNewBranch)
	{
		GetDlgItem(IDC_COMMIT_TO)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_NEWBRANCH)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_NEWBRANCH)->SetFocus();
		GetDlgItem(IDC_NEWBRANCH)->SendMessage(EM_SETSEL, 0, -1);
	}
	else
	{
		GetDlgItem(IDC_NEWBRANCH)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_COMMIT_TO)->ShowWindow(SW_SHOW);
	}
}

bool CCommitDlg::RestoreFiles(bool doNotAsk, bool allowCancel)
{
	if (m_ListCtrl.m_restorepaths.empty())
		return true;

	if (!doNotAsk)
	{
		auto ret = CMessageBox::Show(m_hWnd, IDS_PROC_COMMIT_RESTOREFILES, IDS_APPNAME, 2, IDI_QUESTION, IDS_PROC_COMMIT_RESTOREFILES_RESTORE, IDS_PROC_COMMIT_RESTOREFILES_KEEP, allowCancel ? IDS_MSGBOX_CANCEL : 0);
		if (ret == 3)
			return false;
		if (ret == 2)
			return true;
	}

	for (const auto& item : m_ListCtrl.m_restorepaths)
	{
		CString dest = g_Git.CombinePath(item.first);
		CopyFile(item.second, dest, FALSE);
		CPathUtils::Touch(dest);
	}
	m_ListCtrl.m_restorepaths.clear();

	return true;
}

void CCommitDlg::UpdateCheckLinks()
{
	DialogEnableWindow(IDC_CHECKALL, true);
	DialogEnableWindow(IDC_CHECKNONE, true);
	DialogEnableWindow(IDC_CHECKUNVERSIONED, m_ListCtrl.GetUnversionedCount() > 0);
	DialogEnableWindow(IDC_CHECKVERSIONED, m_ListCtrl.GetItemCount() > m_ListCtrl.GetUnversionedCount());
	DialogEnableWindow(IDC_CHECKADDED, m_ListCtrl.GetAddedCount() > 0);
	DialogEnableWindow(IDC_CHECKDELETED, m_ListCtrl.GetDeletedCount() > 0);
	DialogEnableWindow(IDC_CHECKMODIFIED, m_ListCtrl.GetModifiedCount() > 0);
	DialogEnableWindow(IDC_CHECKFILES, m_ListCtrl.GetFileCount() > 0);
	DialogEnableWindow(IDC_CHECKSUBMODULES, m_ListCtrl.GetSubmoduleCount() > 0);
}

void CCommitDlg::OnBnClickedCommitSetauthor()
{
	UpdateData();

	GetDlgItem(IDC_COMMIT_AUTHORDATA)->SendMessage(EM_SETREADONLY, m_bSetAuthor ? FALSE: TRUE);

	m_sAuthor.Format(L"%s <%s>", static_cast<LPCWSTR>(g_Git.GetUserName()), static_cast<LPCWSTR>(g_Git.GetUserEmail()));
	if (m_bCommitAmend)
	{
		GitRev headRevision;
		if (headRevision.GetCommit(L"HEAD"))
			MessageBox(headRevision.GetLastErr(), L"TortoiseGit", MB_ICONERROR);
		else
			m_sAuthor.Format(L"%s <%s>", static_cast<LPCWSTR>(headRevision.GetAuthorName()), static_cast<LPCWSTR>(headRevision.GetAuthorEmail()));
	}
	UpdateData(FALSE);
}

void CCommitDlg::PrepareStagingSupport()
{
	DestroyPatchViewDlgIfOpen();
	g_Git.SetConfigValue(L"tgit.commitstagingsupport", m_bStagingSupport ? L"true" : L"false");
	m_ListCtrl.EnableThreeStateCheckboxes(m_bStagingSupport);
	if (m_bStagingSupport)
	{
		if (m_bCommitAmend)
		{
			UpdateData(false);
			DialogEnableWindow(IDC_COMMIT_AMENDDIFF, false);
		}
		CMessageBox::ShowCheck(GetSafeHwnd(), IDS_TIPSTAGINGMODE, IDS_APPNAME, MB_ICONINFORMATION | MB_OK, L"HintStagingMode", IDS_MSGBOX_DONOTSHOWAGAIN);
		GetDlgItem(IDC_VIEW_PATCH)->ShowWindow(SW_HIDE);
		DialogEnableWindow(IDC_VIEW_PATCH, FALSE);
		ShowPartialStagingTextAndUpdateDisplayState(true);
		DialogEnableWindow(IDC_PARTIAL_STAGING, TRUE);
		GetDlgItem(IDC_PARTIAL_STAGING)->ShowWindow(SW_SHOW);
		ShowPartialUnstagingTextAndUpdateDisplayState(true);
		DialogEnableWindow(IDC_PARTIAL_UNSTAGING, TRUE);
		GetDlgItem(IDC_PARTIAL_UNSTAGING)->ShowWindow(SW_SHOW);
	}
	else
	{
		ShowViewPatchText(true);
		DialogEnableWindow(IDC_VIEW_PATCH, TRUE);
		GetDlgItem(IDC_VIEW_PATCH)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_PARTIAL_STAGING)->ShowWindow(SW_HIDE);
		DialogEnableWindow(IDC_PARTIAL_STAGING, FALSE);
		GetDlgItem(IDC_PARTIAL_UNSTAGING)->ShowWindow(SW_HIDE);
		DialogEnableWindow(IDC_PARTIAL_UNSTAGING, FALSE);
	}
}

void CCommitDlg::OnBnClickedStagingSupport()
{
	UpdateData();
	PrepareStagingSupport();
	Refresh();
}

bool CCommitDlg::RunStartCommitHook()
{
	DWORD exitcode = 0xFFFFFFFF;
	CString error;
	CHooks::Instance().SetProjectProperties(g_Git.m_CurrentDir, m_ProjectProperties);
	if (CHooks::Instance().StartCommit(GetSafeHwnd(), g_Git.m_CurrentDir, m_pathList, m_sLogMessage, exitcode, error))
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
				return false;
		}
	}
	return true;
}
