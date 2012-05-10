// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN
// Copyright (C) 2008-2012 - TortoiseGit

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
#include "DirFileEnum.h"
//#include "GitConfig.h"
#include "ProjectProperties.h"
#include "MessageBox.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "Git.h"
#include "Registry.h"
#include "GitStatus.h"
#include "HistoryDlg.h"
#include "Hooks.h"
#include "UnicodeUtils.h"
#include "../TGitCache/CacheInterface.h"
#include "ProgressDlg.h"
#include "ShellUpdater.h"
#include "Commands/PushCommand.h"
#include "PatchViewDlg.h"
#include "COMError.h"
#include "Globals.h"
#include "SysProgressDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

UINT CCommitDlg::WM_AUTOLISTREADY = RegisterWindowMessage(_T("TORTOISEGIT_AUTOLISTREADY_MSG"));

IMPLEMENT_DYNAMIC(CCommitDlg, CResizableStandAloneDialog)
CCommitDlg::CCommitDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CCommitDlg::IDD, pParent)
	, m_bRecursive(FALSE)
	, m_bShowUnversioned(FALSE)
	, m_bBlock(FALSE)
	, m_bThreadRunning(FALSE)
	, m_bRunThread(FALSE)
	, m_pThread(NULL)
	, m_bWholeProject(FALSE)
	, m_bKeepChangeList(TRUE)
	, m_bDoNotAutoselectSubmodules(FALSE)
	, m_itemsCount(0)
	, m_bSelectFilesForCommit(TRUE)
	, m_bNoPostActions(FALSE)
	, m_bAutoClose(false)
	, m_bSetCommitDateTime(FALSE)
	, m_bCreateNewBranch(FALSE)
	, m_bCreateTagAfterCommit(FALSE)
{
	this->m_bCommitAmend=FALSE;
	m_bPushAfterCommit = FALSE;
}

CCommitDlg::~CCommitDlg()
{
	if(m_pThread != NULL)
	{
		delete m_pThread;
	}
}

void CCommitDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FILELIST, m_ListCtrl);
	DDX_Control(pDX, IDC_LOGMESSAGE, m_cLogMessage);
	DDX_Check(pDX, IDC_SHOWUNVERSIONED, m_bShowUnversioned);
	DDX_Check(pDX, IDC_COMMIT_SETDATETIME, m_bSetCommitDateTime);
	DDX_Check(pDX, IDC_CHECK_NEWBRANCH, m_bCreateNewBranch);
	DDX_Text(pDX, IDC_NEWBRANCH, m_sCreateNewBranch);
	DDX_Control(pDX, IDC_SELECTALL, m_SelectAll);
	DDX_Text(pDX, IDC_BUGID, m_sBugID);
	DDX_Check(pDX, IDC_WHOLE_PROJECT, m_bWholeProject);
	DDX_Control(pDX, IDC_SPLITTER, m_wndSplitter);
	DDX_Check(pDX, IDC_KEEPLISTS, m_bKeepChangeList);
	DDX_Check(pDX, IDC_NOAUTOSELECTSUBMODULES, m_bDoNotAutoselectSubmodules);
	DDX_Check(pDX,IDC_COMMIT_AMEND,m_bCommitAmend);
	DDX_Check(pDX,IDC_COMMIT_AMENDDIFF,m_bAmendDiffToLastCommit);
	DDX_Control(pDX,IDC_VIEW_PATCH,m_ctrlShowPatch);
	DDX_Control(pDX, IDC_COMMIT_DATEPICKER, m_CommitDate);
	DDX_Control(pDX, IDC_COMMIT_TIMEPICKER, m_CommitTime);
}

BEGIN_MESSAGE_MAP(CCommitDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_SELECTALL, OnBnClickedSelectall)
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
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

	ON_REGISTERED_MESSAGE(WM_AUTOLISTREADY, OnAutoListReady)
	ON_WM_TIMER()
	ON_WM_SIZE()
	ON_STN_CLICKED(IDC_EXTERNALWARNING, &CCommitDlg::OnStnClickedExternalwarning)
	ON_BN_CLICKED(IDC_SIGNOFF, &CCommitDlg::OnBnClickedSignOff)
	ON_BN_CLICKED(IDC_COMMIT_AMEND, &CCommitDlg::OnBnClickedCommitAmend)
	ON_BN_CLICKED(IDC_WHOLE_PROJECT, &CCommitDlg::OnBnClickedWholeProject)
	ON_COMMAND(ID_FOCUS_MESSAGE,&CCommitDlg::OnFocusMessage)
	ON_STN_CLICKED(IDC_VIEW_PATCH, &CCommitDlg::OnStnClickedViewPatch)
	ON_WM_MOVE()
	ON_WM_MOVING()
	ON_WM_SIZING()
	ON_NOTIFY(HDN_ITEMCHANGED, 0, &CCommitDlg::OnHdnItemchangedFilelist)
	ON_BN_CLICKED(IDC_COMMIT_AMENDDIFF, &CCommitDlg::OnBnClickedCommitAmenddiff)
	ON_BN_CLICKED(IDC_NOAUTOSELECTSUBMODULES, &CCommitDlg::OnBnClickedNoautoselectsubmodules)
	ON_BN_CLICKED(IDC_COMMIT_SETDATETIME, &CCommitDlg::OnBnClickedCommitSetDateTime)
	ON_BN_CLICKED(IDC_CHECK_NEWBRANCH, &CCommitDlg::OnBnClickedCheckNewBranch)
END_MESSAGE_MAP()

BOOL CCommitDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	CAppUtils::GetCommitTemplate(this->m_sLogMessage);

	CString dotGitPath;
	g_GitAdminDir.GetAdminDirPath(g_Git.m_CurrentDir, dotGitPath);
	if(PathFileExists(dotGitPath + _T("MERGE_MSG")))
	{
		CStdioFile file;
		if(file.Open(dotGitPath + _T("MERGE_MSG"), CFile::modeRead))
		{
			CString str;
			while(file.ReadString(str))
			{
				m_sLogMessage += str;
				str.Empty();
				m_sLogMessage += _T("\n");
			}
		}
	}

	if (CTGitPath(g_Git.m_CurrentDir).IsMergeActive())
		DialogEnableWindow(IDC_CHECK_NEWBRANCH, FALSE);

	m_regAddBeforeCommit = CRegDWORD(_T("Software\\TortoiseGit\\AddBeforeCommit"), TRUE);
	m_bShowUnversioned = m_regAddBeforeCommit;

	m_History.SetMaxHistoryItems((LONG)CRegDWORD(_T("Software\\TortoiseGit\\MaxHistoryItems"), 25));

	m_regKeepChangelists = CRegDWORD(_T("Software\\TortoiseGit\\KeepChangeLists"), FALSE);
	m_bKeepChangeList = m_regKeepChangelists;

	m_regDoNotAutoselectSubmodules = CRegDWORD(_T("Software\\TortoiseGit\\DoNotAutoselectSubmodules"), FALSE);
	m_bDoNotAutoselectSubmodules = m_regDoNotAutoselectSubmodules;

	m_hAccel = LoadAccelerators(AfxGetResourceHandle(),MAKEINTRESOURCE(IDR_ACC_COMMITDLG));

//	GitConfig config;
//	m_bWholeProject = config.KeepLocks();

	if(this->m_pathList.GetCount() == 0)
		m_bWholeProject =true;

	if(this->m_pathList.GetCount() == 1 && m_pathList[0].IsEmpty())
		m_bWholeProject =true;

	SetDlgTitle();

	UpdateData(FALSE);

	m_ListCtrl.Init(GITSLC_COLEXT | GITSLC_COLSTATUS | GITSLC_COLADD |GITSLC_COLDEL, _T("CommitDlg"),(GITSLC_POPALL ^ (GITSLC_POPCOMMIT | GITSLC_POPSAVEAS)));
	m_ListCtrl.SetSelectButton(&m_SelectAll);
	m_ListCtrl.SetStatLabel(GetDlgItem(IDC_STATISTICS));
	m_ListCtrl.SetCancelBool(&m_bCancelled);
	m_ListCtrl.SetEmptyString(IDS_COMMITDLG_NOTHINGTOCOMMIT);
	m_ListCtrl.EnableFileDrop();
	m_ListCtrl.SetBackgroundImage(IDI_COMMIT_BKG);

	//this->DialogEnableWindow(IDC_COMMIT_AMEND,FALSE);
	m_ProjectProperties.ReadPropsPathList(m_pathList);

	m_cLogMessage.Init(m_ProjectProperties);
	m_cLogMessage.SetFont((CString)CRegString(_T("Software\\TortoiseGit\\LogFontName"), _T("Courier New")), (DWORD)CRegDWORD(_T("Software\\TortoiseGit\\LogFontSize"), 8));
	m_cLogMessage.RegisterContextMenuHandler(this);

	OnEnChangeLogmessage();

	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_EXTERNALWARNING, IDS_COMMITDLG_EXTERNALS);
	m_tooltips.AddTool(IDC_COMMIT_AMEND,IDS_COMMIT_AMEND_TT);
//	m_tooltips.AddTool(IDC_HISTORY, IDS_COMMITDLG_HISTORY_TT);

	m_SelectAll.SetCheck(BST_INDETERMINATE);

	CBugTraqAssociations bugtraq_associations;
	bugtraq_associations.Load();

	if (bugtraq_associations.FindProvider(g_Git.m_CurrentDir, &m_bugtraq_association))
	{
		GetDlgItem(IDC_BUGID)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_BUGIDLABEL)->ShowWindow(SW_HIDE);

		CComPtr<IBugTraqProvider> pProvider;
		HRESULT hr = pProvider.CoCreateInstance(m_bugtraq_association.GetProviderClass());
		if (SUCCEEDED(hr))
		{
			m_BugTraqProvider = pProvider;
			BSTR temp = NULL;
			if (SUCCEEDED(hr = pProvider->GetLinkText(GetSafeHwnd(), m_bugtraq_association.GetParameters().AllocSysString(), &temp)))
			{
				SetDlgItemText(IDC_BUGTRAQBUTTON, temp);
				GetDlgItem(IDC_BUGTRAQBUTTON)->EnableWindow(TRUE);
				GetDlgItem(IDC_BUGTRAQBUTTON)->ShowWindow(SW_SHOW);
			}

			SysFreeString(temp);
		}

		GetDlgItem(IDC_LOGMESSAGE)->SetFocus();
	}
	else if (!m_ProjectProperties.sMessage.IsEmpty())
	{
		GetDlgItem(IDC_BUGID)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_BUGIDLABEL)->ShowWindow(SW_SHOW);
		if (!m_ProjectProperties.sLabel.IsEmpty())
			SetDlgItemText(IDC_BUGIDLABEL, m_ProjectProperties.sLabel);
		GetDlgItem(IDC_BUGTRAQBUTTON)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_BUGTRAQBUTTON)->EnableWindow(FALSE);
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
		GetDlgItem(IDC_BUGTRAQBUTTON)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_BUGTRAQBUTTON)->EnableWindow(FALSE);
		GetDlgItem(IDC_LOGMESSAGE)->SetFocus();
	}

	if (!m_sLogMessage.IsEmpty())
		m_cLogMessage.SetText(m_sLogMessage);

	GetWindowText(m_sWindowTitle);

	AdjustControlSize(IDC_SHOWUNVERSIONED);
	AdjustControlSize(IDC_SELECTALL);
	AdjustControlSize(IDC_WHOLE_PROJECT);

	GetClientRect(m_DlgOrigRect);
	m_cLogMessage.GetClientRect(m_LogMsgOrigRect);

	AddAnchor(IDC_COMMITLABEL, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_BUGIDLABEL, TOP_RIGHT);
	AddAnchor(IDC_BUGID, TOP_RIGHT);
	AddAnchor(IDC_BUGTRAQBUTTON, TOP_RIGHT);
	AddAnchor(IDC_COMMIT_TO, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_CHECK_NEWBRANCH, TOP_RIGHT);
	AddAnchor(IDC_NEWBRANCH, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_MESSAGEGROUP, TOP_LEFT, TOP_RIGHT);
//	AddAnchor(IDC_HISTORY, TOP_LEFT);
	AddAnchor(IDC_LOGMESSAGE, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_SIGNOFF, TOP_RIGHT);
	AddAnchor(IDC_VIEW_PATCH, BOTTOM_RIGHT);
	AddAnchor(IDC_LISTGROUP, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SPLITTER, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_FILELIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SHOWUNVERSIONED, BOTTOM_LEFT);
	AddAnchor(IDC_SELECTALL, BOTTOM_LEFT);
	AddAnchor(IDC_EXTERNALWARNING, BOTTOM_RIGHT);
	AddAnchor(IDC_STATISTICS, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_TEXT_INFO, TOP_RIGHT);
	AddAnchor(IDC_WHOLE_PROJECT, BOTTOM_LEFT);
	AddAnchor(IDC_KEEPLISTS, BOTTOM_LEFT);
	AddAnchor(IDC_NOAUTOSELECTSUBMODULES, BOTTOM_LEFT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	AddAnchor(IDC_COMMIT_AMEND,TOP_LEFT);
	AddAnchor(IDC_COMMIT_AMENDDIFF,TOP_LEFT);
	AddAnchor(IDC_COMMIT_SETDATETIME,TOP_LEFT);
	AddAnchor(IDC_COMMIT_DATEPICKER,TOP_LEFT);
	AddAnchor(IDC_COMMIT_TIMEPICKER,TOP_LEFT);

	if (hWndExplorer)
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	EnableSaveRestore(_T("CommitDlg"));
	DWORD yPos = CRegDWORD(_T("Software\\TortoiseGit\\TortoiseProc\\ResizableState\\CommitDlgSizer"));
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
		int delta = yPos - rectSplitter.top;
		if ((rcLogMsg.bottom + delta > rcLogMsg.top)&&(rcLogMsg.bottom + delta < rcFileList.bottom - 30))
		{
			m_wndSplitter.SetWindowPos(NULL, 0, yPos, 0, 0, SWP_NOSIZE);
			DoSize(delta);
		}
	}

	// add all directories to the watcher
	/*
	for (int i=0; i<m_pathList.GetCount(); ++i)
	{
		if (m_pathList[i].IsDirectory())
			m_pathwatcher.AddPath(m_pathList[i]);
	}*/

	m_updatedPathList = m_pathList;

	//first start a thread to obtain the file list with the status without
	//blocking the dialog
	InterlockedExchange(&m_bBlock, TRUE);
	m_pThread = AfxBeginThread(StatusThreadEntry, this, THREAD_PRIORITY_NORMAL,0,CREATE_SUSPENDED);
	if (m_pThread==NULL)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
		InterlockedExchange(&m_bBlock, FALSE);
	}
	else
	{
		m_pThread->m_bAutoDelete = FALSE;
		m_pThread->ResumeThread();
	}
	CRegDWORD err = CRegDWORD(_T("Software\\TortoiseGit\\ErrorOccurred"), FALSE);
	CRegDWORD historyhint = CRegDWORD(_T("Software\\TortoiseGit\\HistoryHintShown"), FALSE);
	if ((((DWORD)err)!=FALSE)&&((((DWORD)historyhint)==FALSE)))
	{
		historyhint = TRUE;
//		ShowBalloon(IDC_HISTORY, IDS_COMMITDLG_HISTORYHINT_TT, IDI_INFORMATION);
	}
	err = FALSE;

	if (g_Git.IsInitRepos())
	{
		m_bCommitAmend = FALSE;
		GetDlgItem(IDC_COMMIT_AMEND)->EnableWindow(FALSE);
		UpdateData(FALSE);
	}
	else
	{
		if(m_bCommitAmend)
		{
			GetDlgItem(IDC_COMMIT_AMEND)->EnableWindow(FALSE);
			GetDlgItem(IDC_COMMIT_AMENDDIFF)->ShowWindow(SW_SHOW);
		}

		CGitHash hash = g_Git.GetHash(_T("HEAD"));
		GitRev headRevision;
		headRevision.GetParentFromHash(hash);
		// do not allow to show diff to "last" revision if it has more that one parent
		if (headRevision.ParentsCount() != 1)
		{
			m_bAmendDiffToLastCommit = true;
			UpdateData(FALSE);
			GetDlgItem(IDC_COMMIT_AMENDDIFF)->EnableWindow(FALSE);
		}
	}

	this->m_ctrlShowPatch.SetURL(CString());

	BOOL viewPatchEnabled = FALSE;
	m_ProjectProperties.GetBOOLProps(viewPatchEnabled, _T("tgit.commitshowpatch"));
	if (viewPatchEnabled)
		OnStnClickedViewPatch();

	return FALSE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CCommitDlg::OnOK()
{
	if (m_bBlock)
		return;
	if (m_bThreadRunning)
	{
		m_bCancelled = true;
		InterlockedExchange(&m_bRunThread, FALSE);
		WaitForSingleObject(m_pThread->m_hThread, 1000);
		if (m_bThreadRunning)
		{
			// we gave the thread a chance to quit. Since the thread didn't
			// listen to us we have to kill it.
			TerminateThread(m_pThread->m_hThread, (DWORD)-1);
			InterlockedExchange(&m_bThreadRunning, FALSE);
		}
	}
	this->UpdateData();

	if (m_bCreateNewBranch && !g_Git.IsBranchNameValid(m_sCreateNewBranch))
	{
		ShowEditBalloon(IDC_NEWBRANCH, IDS_B_T_NOTEMPTY, TTI_ERROR);
		return;
	}

	CString id;
	GetDlgItemText(IDC_BUGID, id);
	if (!m_ProjectProperties.CheckBugID(id))
	{
		ShowEditBalloon(IDC_BUGID, IDS_COMMITDLG_ONLYNUMBERS, TTI_ERROR);
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

	BOOL bWarnNoSignedOffBy = FALSE;
	ProjectProperties::GetBOOLProps(bWarnNoSignedOffBy, _T("tgit.warnnosignedoffby"));
	if (bWarnNoSignedOffBy == TRUE && m_cLogMessage.GetText().Find(GetSignedOffByLine()) == -1)
	{
		UINT retval = CMessageBox::Show(this->m_hWnd, IDS_PROC_COMMIT_NOSIGNOFFLINE, IDS_APPNAME, 1, IDI_WARNING, IDS_PROC_COMMIT_ADDSIGNOFFBUTTON, IDS_PROC_COMMIT_NOADDSIGNOFFBUTTON, IDS_ABORTBUTTON);
		if (retval == 1)
		{
			OnBnClickedSignOff();
			m_sLogMessage = m_cLogMessage.GetText();
		}
		else if (retval == 3)
			return;
	}

	m_ListCtrl.WriteCheckedNamesToPathList(m_selectedPathList);
	m_pathwatcher.Stop();
	InterlockedExchange(&m_bBlock, TRUE);
	CDWordArray arDeleted;
	//first add all the unversioned files the user selected
	//and check if all versioned files are selected
	int nchecked = 0;
	m_bRecursive = true;
	int nListItems = m_ListCtrl.GetItemCount();

	CTGitPathList itemsToAdd;
	CTGitPathList itemsToRemove;
	//std::set<CString> checkedLists;
	//std::set<CString> uncheckedLists;

		// now let the bugtraq plugin check the commit message
	CComPtr<IBugTraqProvider2> pProvider2 = NULL;
	if (m_BugTraqProvider)
	{
		HRESULT hr = m_BugTraqProvider.QueryInterface(&pProvider2);
		if (SUCCEEDED(hr))
		{
			BSTR temp = NULL;
			CString common = g_Git.m_CurrentDir;
			BSTR repositoryRoot = common.AllocSysString();
			BSTR parameters = m_bugtraq_association.GetParameters().AllocSysString();
			BSTR commonRoot = SysAllocString(m_pathList.GetCommonRoot().GetDirectory().GetWinPath());
			BSTR commitMessage = m_sLogMessage.AllocSysString();
			SAFEARRAY *pathList = SafeArrayCreateVector(VT_BSTR, 0, m_selectedPathList.GetCount());

			for (LONG index = 0; index < m_selectedPathList.GetCount(); ++index)
				SafeArrayPutElement(pathList, &index, m_selectedPathList[index].GetGitPathString().AllocSysString());

			if (FAILED(hr = pProvider2->CheckCommit(GetSafeHwnd(), parameters, repositoryRoot, commonRoot, pathList, commitMessage, &temp)))
			{
				COMError ce(hr);
				CString sErr;
				sErr.Format(IDS_ERR_FAILEDISSUETRACKERCOM, m_bugtraq_association.GetProviderName(), ce.GetMessageAndDescription().c_str());
				CMessageBox::Show(m_hWnd, sErr, _T("TortoiseGit"), MB_ICONERROR);
			}
			else
			{
				CString sError = temp;
				if (!sError.IsEmpty())
				{
					CMessageBox::Show(m_hWnd, sError, _T("TortoiseGit"), MB_ICONERROR);
					return;
				}
				SysFreeString(temp);
			}
		}
	}

	//CString checkedfiles;
	//CString uncheckedfiles;

	CString cmd;
	CString out;

	bool bAddSuccess=true;
	bool bCloseCommitDlg=false;

	CSysProgressDlg sysProgressDlg;
	if (nListItems >= 25 && sysProgressDlg.IsValid())
	{
		sysProgressDlg.SetTitle(CString(MAKEINTRESOURCE(IDS_PROC_COMMIT_PREPARECOMMIT)));
		sysProgressDlg.SetLine(1, CString(MAKEINTRESOURCE(IDS_PROC_COMMIT_UPDATEINDEX)));
		sysProgressDlg.SetTime(true);
		sysProgressDlg.SetShowProgressBar(true);
		sysProgressDlg.ShowModal(this, true);
	}

	CBlockCacheForPath cacheBlock(g_Git.m_CurrentDir);
	DWORD currentTicks = GetTickCount();

	for (int j=0; j<nListItems; j++)
	{
		CTGitPath *entry = (CTGitPath*)m_ListCtrl.GetItemData(j);
		if (sysProgressDlg.IsValid())
		{
			if (GetTickCount() - currentTicks > 1000 || j == nListItems - 1 || j == 0)
			{
				sysProgressDlg.SetLine(2, entry->GetGitPathString(), true);
				sysProgressDlg.SetProgress(j, nListItems);
				AfxGetThread()->PumpMessage(); // process messages, in order to avoid freezing; do not call this too: this takes time!
				currentTicks = GetTickCount();
			}
		}
		//const CGitStatusListCtrl::FileEntry * entry = m_ListCtrl.GetListEntry(j);
		if (entry->m_Checked)
		{
#if 0
			if (entry->status == Git_wc_status_unversioned)
			{
				itemsToAdd.AddPath(entry->GetPath());
			}
			if (entry->status == Git_wc_status_conflicted)
			{
				bHasConflicted = true;
			}
			if (entry->status == Git_wc_status_missing)
			{
				itemsToRemove.AddPath(entry->GetPath());
			}
			if (entry->status == Git_wc_status_deleted)
			{
				arDeleted.Add(j);
			}
			if (entry->IsInExternal())
			{
				bCheckedInExternal = true;
			}
#endif
			if( entry->m_Action & CTGitPath::LOGACTIONS_UNVER)
				cmd.Format(_T("git.exe add -f -- \"%s\""),entry->GetGitPathString());
			else if ( entry->m_Action & CTGitPath::LOGACTIONS_DELETED)
				cmd.Format(_T("git.exe update-index --force-remove -- \"%s\""),entry->GetGitPathString());
			else
				cmd.Format(_T("git.exe update-index  -- \"%s\""),entry->GetGitPathString());

			if (g_Git.Run(cmd, &out, CP_UTF8))
			{
				CMessageBox::Show(NULL,out,_T("TortoiseGit"),MB_OK|MB_ICONERROR);
				bAddSuccess = false ;
				break;
			}

			if( entry->m_Action & CTGitPath::LOGACTIONS_REPLACED)
				cmd.Format(_T("git.exe rm -- \"%s\""), entry->GetGitOldPathString());

			g_Git.Run(cmd, &out, CP_UTF8);

			nchecked++;

			//checkedLists.insert(entry->GetGitPathString());
//			checkedfiles += _T("\"")+entry->GetGitPathString()+_T("\" ");
		}
		else
		{
			//uncheckedLists.insert(entry->GetGitPathString());
			if(entry->m_Action & CTGitPath::LOGACTIONS_ADDED)
			{	//To init git repository, there are not HEAD, so we can use git reset command
				cmd.Format(_T("git.exe rm -f --cache -- \"%s\""),entry->GetGitPathString());
				if (g_Git.Run(cmd, &out, CP_UTF8))
				{
					CMessageBox::Show(NULL,out,_T("TortoiseGit"),MB_OK|MB_ICONERROR);
					bAddSuccess = false ;
					bCloseCommitDlg=false;
					break;
				}

			}
			else if(!( entry->m_Action & CTGitPath::LOGACTIONS_UNVER ) )
			{
				if (m_bCommitAmend && !m_bAmendDiffToLastCommit)
				{
					cmd.Format(_T("git.exe reset HEAD~2 -- \"%s\""), entry->GetGitPathString());
				}
				else
				{
					cmd.Format(_T("git.exe reset -- \"%s\""), entry->GetGitPathString());
				}
				if (g_Git.Run(cmd, &out, CP_UTF8))
				{
					/* when reset a unstage file will report error.
					CMessageBox::Show(NULL,out,_T("TortoiseGit"),MB_OK|MB_ICONERROR);
					bAddSuccess = false ;
					bCloseCommitDlg=false;
					break;
					*/
				}
				// && !entry->IsDirectory()
				if (m_bCommitAmend && !m_bAmendDiffToLastCommit)
					continue;
			}

		//	uncheckedfiles += _T("\"")+entry->GetGitPathString()+_T("\" ");
#if 0
			if ((entry->status != Git_wc_status_unversioned)	&&
				(entry->status != Git_wc_status_ignored))
			{
				nUnchecked++;
				uncheckedLists.insert(entry->GetChangeList());
				if (m_bRecursive)
				{
					// This algorithm is for the sake of simplicity of the complexity O(N?
					for (int k=0; k<nListItems; k++)
					{
						const CGitStatusListCtrl::FileEntry * entryK = m_ListCtrl.GetListEntry(k);
						if (entryK->IsChecked() && entryK->GetPath().IsAncestorOf(entry->GetPath())  )
						{
							// Fall back to a non-recursive commit to prevent items being
							// committed which aren't checked although its parent is checked
							// (property change, directory deletion, ... )
							m_bRecursive = false;
							break;
						}
					}
				}
			}
#endif
		}

		if (sysProgressDlg.IsValid() && sysProgressDlg.HasUserCancelled())
		{
			bAddSuccess = false;
			break;
		}

		CShellUpdater::Instance().AddPathForUpdate(*entry);
	}

	if (sysProgressDlg.IsValid())
		sysProgressDlg.Stop();

	if (bAddSuccess && m_bCreateNewBranch)
	{
		if (g_Git.Run(_T("git branch ") + m_sCreateNewBranch, &out, CP_UTF8))
		{
			MessageBox(_T("Creating new branch failed:\n") + out, _T("TortoiseGit"), MB_OK | MB_ICONERROR);
			bAddSuccess = false;
		}
		if (g_Git.Run(_T("git checkout ") + m_sCreateNewBranch, &out, CP_UTF8))
		{
			MessageBox(_T("Switching to new branch failed:\n") + out, _T("TortoiseGit"), MB_OK | MB_ICONERROR);
			bAddSuccess = false;
		}
	}

	if (bAddSuccess && CheckHeadDetach())
		bAddSuccess = false;

	//if(uncheckedfiles.GetLength()>0)
	//{
	//	cmd.Format(_T("git.exe reset -- %s"),uncheckedfiles);
	//	g_Git.Run(cmd,&out);
	//}

	m_sBugID.Trim();
	if (!m_sBugID.IsEmpty())
	{
		m_sBugID.Replace(_T(", "), _T(","));
		m_sBugID.Replace(_T(" ,"), _T(","));
		CString sBugID = m_ProjectProperties.sMessage;
		sBugID.Replace(_T("%BUGID%"), m_sBugID);
		if (m_ProjectProperties.bAppend)
			m_sLogMessage += _T("\n") + sBugID + _T("\n");
		else
			m_sLogMessage = sBugID + _T("\n") + m_sLogMessage;
	}

	//if(checkedfiles.GetLength()>0)
	if (bAddSuccess && (nchecked || m_bCommitAmend ||  CTGitPath(g_Git.m_CurrentDir).IsMergeActive()))
	{
	//	cmd.Format(_T("git.exe update-index -- %s"),checkedfiles);
	//	g_Git.Run(cmd,&out);

		bCloseCommitDlg = true;

		CString tempfile=::GetTempFile();

		CAppUtils::SaveCommitUnicodeFile(tempfile,m_sLogMessage);
		//file.WriteString(m_sLogMessage);

		CTGitPath path=g_Git.m_CurrentDir;

		BOOL IsGitSVN = path.GetAdminDirMask() & ITEMIS_GITSVN;

		out =_T("");
		CString amend;
		if(this->m_bCommitAmend)
		{
			amend=_T("--amend");
		}
		CString dateTime;
		if (m_bSetCommitDateTime)
		{
			CTime date, time;
			m_CommitDate.GetTime(date);
			m_CommitTime.GetTime(time);
			dateTime.Format(_T("--date=%sT%s"), date.Format(_T("%Y-%m-%d")), time.Format(_T("%H:%M:%S")));
		}
		cmd.Format(_T("git.exe commit %s %s -F \"%s\""), dateTime, amend, tempfile);

		CCommitProgressDlg progress;
		progress.m_bBufferAll=true; // improve show speed when there are many file added.
		progress.m_GitCmd=cmd;
		progress.m_bShowCommand = FALSE;	// don't show the commit command
		progress.m_PreText = out;			// show any output already generated in log window
		progress.m_bAutoCloseOnSuccess = m_bAutoClose;

		if (!m_bNoPostActions && !m_bAutoClose)
		{
			progress.m_PostCmdList.Add( IsGitSVN? CString(MAKEINTRESOURCE(IDS_MENUSVNDCOMMIT)): CString(MAKEINTRESOURCE(IDS_MENUPUSH)));
			progress.m_PostCmdList.Add(CString(MAKEINTRESOURCE(IDS_PROC_COMMIT_RECOMMIT)));
			progress.m_PostCmdList.Add(CString(MAKEINTRESOURCE(IDS_MENUTAG)));
		}

		m_PostCmd = IsGitSVN? GIT_POST_CMD_DCOMMIT:GIT_POST_CMD_PUSH;

		DWORD userResponse = progress.DoModal();

		if(progress.m_GitStatus || userResponse == (IDC_PROGRESS_BUTTON1+1))
		{
			bCloseCommitDlg = false;
			if( userResponse == (IDC_PROGRESS_BUTTON1+1 ))
			{
				this->m_sLogMessage.Empty();
				m_cLogMessage.SetText(m_sLogMessage);
			}

			this->Refresh();
		}
		else if(userResponse == IDC_PROGRESS_BUTTON1 + 2)
		{
			m_bCreateTagAfterCommit=true;
		}
		else if(userResponse == IDC_PROGRESS_BUTTON1)
		{
			//User pressed 'Push' button after successful commit.
			m_bPushAfterCommit=true;
		}

		CFile::Remove(tempfile);

		if (m_BugTraqProvider && progress.m_GitStatus == 0)
		{
			CComPtr<IBugTraqProvider2> pProvider = NULL;
			HRESULT hr = m_BugTraqProvider.QueryInterface(&pProvider);
			if (SUCCEEDED(hr))
			{
				BSTR commonRoot = SysAllocString(g_Git.m_CurrentDir);
				SAFEARRAY *pathList = SafeArrayCreateVector(VT_BSTR, 0,this->m_selectedPathList.GetCount());

				for (LONG index = 0; index < m_selectedPathList.GetCount(); ++index)
					SafeArrayPutElement(pathList, &index, m_selectedPathList[index].GetGitPathString().AllocSysString());

				BSTR logMessage = m_sLogMessage.AllocSysString();

				CGitHash hash=g_Git.GetHash(_T("HEAD"));
				LONG version = g_Git.Hash2int(hash);

				BSTR temp = NULL;
				if (FAILED(hr = pProvider->OnCommitFinished(GetSafeHwnd(),
					commonRoot,
					pathList,
					logMessage,
					(LONG)version,
					&temp)))
				{
					CString sErr = temp;
					if (!sErr.IsEmpty())
						CMessageBox::Show(NULL,(sErr),_T("TortoiseGit"),MB_OK|MB_ICONERROR);
					else
					{
						COMError ce(hr);
						sErr.Format(IDS_ERR_FAILEDISSUETRACKERCOM, ce.GetSource().c_str(), ce.GetMessageAndDescription().c_str());
						CMessageBox::Show(NULL,(sErr),_T("TortoiseGit"),MB_OK|MB_ICONERROR);
					}
				}

				SysFreeString(temp);
			}
		}
		RestoreFiles(progress.m_GitStatus == 0);
	}
	else if(bAddSuccess)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERROR_NOTHING_COMMIT, IDS_COMMIT_FINISH, MB_OK | MB_ICONINFORMATION);
		bCloseCommitDlg=false;
	}
#if 0
	if (m_pathwatcher.GetNumberOfChangedPaths() && m_bRecursive)
	{
		// There are paths which got changed (touched at least).
		// We have to find out if this affects the selection in the commit dialog
		// If it could affect the selection, revert back to a non-recursive commit
		CTGitPathList changedList = m_pathwatcher.GetChangedPaths();
		changedList.RemoveDuplicates();
		for (int i=0; i<changedList.GetCount(); ++i)
		{
			if (changedList[i].IsAdminDir())
			{
				// something inside an admin dir was changed.
				// if it's the entries file, then we have to fully refresh because
				// files may have been added/removed from version control
				if ((changedList[i].GetWinPathString().Right(7).CompareNoCase(_T("entries")) == 0) &&
					(changedList[i].GetWinPathString().Find(_T("\\tmp\\"))<0))
				{
					m_bRecursive = false;
					break;
				}
			}
			else if (!m_ListCtrl.IsPathShown(changedList[i]))
			{
				// a path which is not shown in the list has changed
				CGitStatusListCtrl::FileEntry * entry = m_ListCtrl.GetListEntry(changedList[i]);
				if (entry)
				{
					// check if the changed path would get committed by a recursive commit
					if ((!entry->IsFromDifferentRepository()) &&
						(!entry->IsInExternal()) &&
						(!entry->IsNested()) &&
						(!entry->IsChecked()))
					{
						m_bRecursive = false;
						break;
					}
				}
			}
		}
	}


	// Now, do all the adds - make sure that the list is sorted so that parents
	// are added before their children
	itemsToAdd.SortByPathname();
	Git Git;
	if (!Git.Add(itemsToAdd, &m_ProjectProperties, Git_depth_empty, FALSE, FALSE, TRUE))
	{
		CMessageBox::Show(m_hWnd, Git.GetLastErrorMessage(), _T("TortoiseGit"), MB_ICONERROR);
		InterlockedExchange(&m_bBlock, FALSE);
		Refresh();
		return;
	}

	// Remove any missing items
	// Not sure that this sort is really necessary - indeed, it might be better to do a reverse sort at this point
	itemsToRemove.SortByPathname();
	Git.Remove(itemsToRemove, TRUE);

	//the next step: find all deleted files and check if they're
	//inside a deleted folder. If that's the case, then remove those
	//files from the list since they'll get deleted by the parent
	//folder automatically.
	m_ListCtrl.Block(TRUE, FALSE);
	INT_PTR nDeleted = arDeleted.GetCount();
	for (INT_PTR i=0; i<arDeleted.GetCount(); i++)
	{
		if (m_ListCtrl.GetCheck(arDeleted.GetAt(i)))
		{
			const CTGitPath& path = m_ListCtrl.GetListEntry(arDeleted.GetAt(i))->GetPath();
			if (path.IsDirectory())
			{
				//now find all children of this directory
				for (int j=0; j<arDeleted.GetCount(); j++)
				{
					if (i!=j)
					{
						CGitStatusListCtrl::FileEntry* childEntry = m_ListCtrl.GetListEntry(arDeleted.GetAt(j));
						if (childEntry->IsChecked())
						{
							if (path.IsAncestorOf(childEntry->GetPath()))
							{
								m_ListCtrl.SetEntryCheck(childEntry, arDeleted.GetAt(j), false);
								nDeleted--;
							}
						}
					}
				}
			}
		}
	}
	m_ListCtrl.Block(FALSE, FALSE);

	if ((nUnchecked != 0)||(bCheckedInExternal)||(bHasConflicted)||(!m_bRecursive))
	{
		//save only the files the user has checked into the temporary file
		m_ListCtrl.WriteCheckedNamesToPathList(m_pathList);
	}

	// the item count is used in the progress dialog to show the overall commit
	// progress.
	// deleted items only send one notification event, all others send two
	m_itemsCount = ((m_selectedPathList.GetCount() - nDeleted - itemsToRemove.GetCount()) * 2) + nDeleted + itemsToRemove.GetCount();

	if ((m_bRecursive)&&(checkedLists.size() == 1))
	{
		// all checked items belong to the same changelist
		// find out if there are any unchecked items which belong to that changelist
		if (uncheckedLists.find(*checkedLists.begin()) == uncheckedLists.end())
			m_sChangeList = *checkedLists.begin();
	}
#endif
	UpdateData();
	m_regAddBeforeCommit = m_bShowUnversioned;
	if (!GetDlgItem(IDC_WHOLE_PROJECT)->IsWindowEnabled())
		m_bWholeProject = FALSE;
	m_regKeepChangelists = m_bKeepChangeList;
	m_regDoNotAutoselectSubmodules = m_bDoNotAutoselectSubmodules;
	if (!GetDlgItem(IDC_KEEPLISTS)->IsWindowEnabled())
		m_bKeepChangeList = FALSE;
	InterlockedExchange(&m_bBlock, FALSE);

	m_History.AddEntry(m_sLogMessage);
	m_History.Save();

	SaveSplitterPos();

	if( bCloseCommitDlg )
		CResizableStandAloneDialog::OnOK();

	CShellUpdater::Instance().Flush();
}

void CCommitDlg::SaveSplitterPos()
{
	if (!IsIconic())
	{
		CRegDWORD regPos = CRegDWORD(_T("Software\\TortoiseGit\\TortoiseProc\\ResizableState\\CommitDlgSizer"));
		RECT rectSplitter;
		m_wndSplitter.GetWindowRect(&rectSplitter);
		ScreenToClient(&rectSplitter);
		regPos = rectSplitter.top;
	}
}

UINT CCommitDlg::StatusThreadEntry(LPVOID pVoid)
{
	return ((CCommitDlg*)pVoid)->StatusThread();
}

UINT CCommitDlg::StatusThread()
{
	//get the status of all selected file/folders recursively
	//and show the ones which have to be committed to the user
	//in a list control.
	InterlockedExchange(&m_bBlock, TRUE);
	InterlockedExchange(&m_bThreadRunning, TRUE);// so the main thread knows that this thread is still running
	InterlockedExchange(&m_bRunThread, TRUE);	// if this is set to FALSE, the thread should stop

	m_pathwatcher.Stop();

	g_Git.RefreshGitIndex();

	m_bCancelled = false;

	DialogEnableWindow(IDOK, false);
	DialogEnableWindow(IDC_SHOWUNVERSIONED, false);
	DialogEnableWindow(IDC_WHOLE_PROJECT, false);
	DialogEnableWindow(IDC_SELECTALL, false);
	DialogEnableWindow(IDC_NOAUTOSELECTSUBMODULES, false);
	GetDlgItem(IDC_EXTERNALWARNING)->ShowWindow(SW_HIDE);
	DialogEnableWindow(IDC_EXTERNALWARNING, false);
	// read the list of recent log entries before querying the WC for status
	// -> the user may select one and modify / update it while we are crawling the WC

	if (m_History.GetCount()==0)
	{
		CString reg;
		reg.Format(_T("Software\\TortoiseGit\\History\\commit%s"), (LPCTSTR)m_ListCtrl.m_sUUID);
		reg.Replace(_T(':'),_T('_'));
		m_History.Load(reg, _T("logmsgs"));
	}

	// Initialise the list control with the status of the files/folders below us
	m_ListCtrl.Clear();
	BOOL success;
	CTGitPathList *pList;
	m_ListCtrl.m_amend = (m_bCommitAmend==TRUE) && (m_bAmendDiffToLastCommit==FALSE);
	m_ListCtrl.m_bDoNotAutoselectSubmodules = (m_bDoNotAutoselectSubmodules == TRUE);

	if(m_bWholeProject)
		pList=NULL;
	else
		pList = &m_pathList;

	success=m_ListCtrl.GetStatus(pList);

	//m_ListCtrl.UpdateFileList(git_revnum_t(GIT_REV_ZERO));
	if(this->m_bShowUnversioned)
		m_ListCtrl.UpdateFileList(CGitStatusListCtrl::FILELIST_UNVER,true,pList);

	m_ListCtrl.CheckIfChangelistsArePresent(false);

	DWORD dwShow = GITSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALSFROMDIFFERENTREPOS | GITSLC_SHOWLOCKS | GITSLC_SHOWINCHANGELIST;
	dwShow |= DWORD(m_regAddBeforeCommit) ? GITSLC_SHOWUNVERSIONED : 0;
	if (success)
	{
		if (m_checkedPathList.GetCount())
			m_ListCtrl.Show(dwShow, m_checkedPathList);
		else
		{
			DWORD dwCheck = m_bSelectFilesForCommit ? dwShow : 0;
			dwCheck &=~(CTGitPath::LOGACTIONS_UNVER); //don't check unversion file default.
			m_ListCtrl.Show(dwShow, dwCheck);
			m_bSelectFilesForCommit = true;
		}

		if (m_ListCtrl.HasExternalsFromDifferentRepos())
		{
			GetDlgItem(IDC_EXTERNALWARNING)->ShowWindow(SW_SHOW);
			DialogEnableWindow(IDC_EXTERNALWARNING, TRUE);
		}

		SetDlgItemText(IDC_COMMIT_TO, g_Git.GetCurrentBranch());
		m_tooltips.AddTool(GetDlgItem(IDC_STATISTICS), m_ListCtrl.GetStatisticsString());
	}
	if (!success)
	{
		if (!m_ListCtrl.GetLastErrorMessage().IsEmpty())
			m_ListCtrl.SetEmptyString(m_ListCtrl.GetLastErrorMessage());
		m_ListCtrl.Show(dwShow);
	}
	//
	if ((m_ListCtrl.GetItemCount()==0)&&(m_ListCtrl.HasUnversionedItems())
		 && !PathFileExists(g_Git.m_CurrentDir+_T("\\.git\\MERGE_HEAD")))
	{
		CString temp;
		temp.LoadString(IDS_COMMITDLG_NOTHINGTOCOMMITUNVERSIONED);
		if (CMessageBox::ShowCheck(m_hWnd, temp, _T("TortoiseGit"), MB_ICONINFORMATION | MB_YESNO, _T("NothingToCommitShowUnversioned"), NULL)==IDYES)
		{
			m_bShowUnversioned = TRUE;
			GetDlgItem(IDC_SHOWUNVERSIONED)->SendMessage(BM_SETCHECK, BST_CHECKED);
			DWORD dwShow = (DWORD)(GITSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALSFROMDIFFERENTREPOS | GITSLC_SHOWUNVERSIONED | GITSLC_SHOWLOCKS);
			m_ListCtrl.UpdateFileList(CGitStatusListCtrl::FILELIST_UNVER);
			m_ListCtrl.Show(dwShow,dwShow&(~CTGitPath::LOGACTIONS_UNVER));
		}
	}

	SetDlgTitle();

	m_autolist.clear();
	// we don't have to block the commit dialog while we fetch the
	// auto completion list.
	m_pathwatcher.ClearChangedPaths();
	InterlockedExchange(&m_bBlock, FALSE);
	if ((DWORD)CRegDWORD(_T("Software\\TortoiseGit\\Autocompletion"), TRUE)==TRUE)
	{
		m_ListCtrl.Block(TRUE, TRUE);
		GetAutocompletionList();
		m_ListCtrl.Block(FALSE, FALSE);
	}
	UpdateOKButton();
	if (m_bRunThread)
	{
		DialogEnableWindow(IDC_SHOWUNVERSIONED, true);
		DialogEnableWindow(IDC_WHOLE_PROJECT, true);
		DialogEnableWindow(IDC_SELECTALL, true);
		DialogEnableWindow(IDC_NOAUTOSELECTSUBMODULES, true);
		if (m_ListCtrl.HasChangeLists())
			DialogEnableWindow(IDC_KEEPLISTS, true);
		if (m_ListCtrl.HasLocks())
			DialogEnableWindow(IDC_WHOLE_PROJECT, true);
		// we have the list, now signal the main thread about it
		SendMessage(WM_AUTOLISTREADY);	// only send the message if the thread wasn't told to quit!
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

	if (m_bWholeProject)
		CAppUtils::SetWindowTitle(m_hWnd, g_Git.m_CurrentDir, m_sTitle);
	else
	{
		if (m_pathList.GetCount() == 1)
			CAppUtils::SetWindowTitle(m_hWnd, (g_Git.m_CurrentDir + _T("\\") + m_pathList[0].GetUIPathString()).TrimRight('\\'), m_sTitle);
		else
			CAppUtils::SetWindowTitle(m_hWnd, g_Git.m_CurrentDir + _T("\\") + m_ListCtrl.GetCommonDirectory(false), m_sTitle);
	}
}

void CCommitDlg::OnCancel()
{
	m_bCancelled = true;
	m_pathwatcher.Stop();

	if (m_bThreadRunning)
	{
		InterlockedExchange(&m_bRunThread, FALSE);
		WaitForSingleObject(m_pThread->m_hThread, 1000);
		if (m_bThreadRunning)
		{
			// we gave the thread a chance to quit. Since the thread didn't
			// listen to us we have to kill it.
			TerminateThread(m_pThread->m_hThread, (DWORD)-1);
			InterlockedExchange(&m_bThreadRunning, FALSE);
		}
	}
	UpdateData();
	m_sBugID.Trim();
	m_sLogMessage = m_cLogMessage.GetText();
	if (!m_sBugID.IsEmpty())
	{
		m_sBugID.Replace(_T(", "), _T(","));
		m_sBugID.Replace(_T(" ,"), _T(","));
		CString sBugID = m_ProjectProperties.sMessage;
		sBugID.Replace(_T("%BUGID%"), m_sBugID);
		if (m_ProjectProperties.bAppend)
			m_sLogMessage += _T("\n") + sBugID + _T("\n");
		else
			m_sLogMessage = sBugID + _T("\n") + m_sLogMessage;
	}
	if (m_ProjectProperties.sLogTemplate.Compare(m_sLogMessage) != 0)
		m_History.AddEntry(m_sLogMessage);
	m_History.Save();
	RestoreFiles();
	SaveSplitterPos();
	CResizableStandAloneDialog::OnCancel();
}

void CCommitDlg::OnBnClickedSelectall()
{
	m_tooltips.Pop();	// hide the tooltips
	UINT state = (m_SelectAll.GetState() & 0x0003);
	if (state == BST_INDETERMINATE)
	{
		// It is not at all useful to manually place the checkbox into the indeterminate state...
		// We will force this on to the unchecked state
		state = BST_UNCHECKED;
		m_SelectAll.SetCheck(state);
	}
	m_ListCtrl.SelectAll(state == BST_CHECKED);
}

BOOL CCommitDlg::PreTranslateMessage(MSG* pMsg)
{
	if (!m_bBlock)
		m_tooltips.RelayEvent(pMsg);

	if (m_hAccel)
	{
		int ret = TranslateAccelerator(m_hWnd, m_hAccel, pMsg);
		if (ret)
			return TRUE;
	}

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
					{
						PostMessage(WM_COMMAND, IDOK);
					}
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

	InterlockedExchange(&m_bBlock, TRUE);
	m_pThread = AfxBeginThread(StatusThreadEntry, this, THREAD_PRIORITY_NORMAL,0,CREATE_SUSPENDED);
	if (m_pThread==NULL)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
		InterlockedExchange(&m_bBlock, FALSE);
	}
	else
	{
		m_pThread->m_bAutoDelete = FALSE;
		m_pThread->ResumeThread();
	}
}

void CCommitDlg::OnBnClickedHelp()
{
	OnHelp();
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
			if(m_bWholeProject)
				m_ListCtrl.GetStatus(NULL,false,false,true);
			else
				m_ListCtrl.GetStatus(&this->m_pathList,false,false,true);
		}
		m_ListCtrl.Show(dwShow, 0, true, dwShow & ~(CTGitPath::LOGACTIONS_UNVER), true);
	}
}

void CCommitDlg::OnStnClickedExternalwarning()
{
	m_tooltips.Popup();
}

void CCommitDlg::OnEnChangeLogmessage()
{
	UpdateOKButton();
}

LRESULT CCommitDlg::OnGitStatusListCtrlItemCountChanged(WPARAM, LPARAM)
{
#if 0
	if ((m_ListCtrl.GetItemCount() == 0)&&(m_ListCtrl.HasUnversionedItems())&&(!m_bShowUnversioned))
	{
		if (CMessageBox::Show(*this, IDS_COMMITDLG_NOTHINGTOCOMMITUNVERSIONED, IDS_APPNAME, MB_ICONINFORMATION | MB_YESNO)==IDYES)
		{
			m_bShowUnversioned = TRUE;
			DWORD dwShow = GitSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALSFROMDIFFERENTREPOS | GitSLC_SHOWUNVERSIONED | GitSLC_SHOWLOCKS;
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

LRESULT CCommitDlg::OnFileDropped(WPARAM, LPARAM /*lParam*/)
{
#if 0
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
	path.SetFromWin((LPCTSTR)lParam);

	// just add all the items we get here.
	// if the item is versioned, the add will fail but nothing
	// more will happen.
	Git Git;
	Git.Add(CTGitPathList(path), &m_ProjectProperties, Git_depth_empty, false, true, true);

	if (!m_ListCtrl.HasPath(path))
	{
		if (m_pathList.AreAllPathsFiles())
		{
			m_pathList.AddPath(path);
			m_pathList.RemoveDuplicates();
			m_updatedPathList.AddPath(path);
			m_updatedPathList.RemoveDuplicates();
		}
		else
		{
			// if the path list contains folders, we have to check whether
			// our just (maybe) added path is a child of one of those. If it is
			// a child of a folder already in the list, we must not add it. Otherwise
			// that path could show up twice in the list.
			bool bHasParentInList = false;
			for (int i=0; i<m_pathList.GetCount(); ++i)
			{
				if (m_pathList[i].IsAncestorOf(path))
				{
					bHasParentInList = true;
					break;
				}
			}
			if (!bHasParentInList)
			{
				m_pathList.AddPath(path);
				m_pathList.RemoveDuplicates();
				m_updatedPathList.AddPath(path);
				m_updatedPathList.RemoveDuplicates();
			}
		}
	}

	// Always start the timer, since the status of an existing item might have changed
	SetTimer(REFRESHTIMER, 200, NULL);
	ATLTRACE(_T("Item %s dropped, timer started\n"), path.GetWinPath());
#endif
	return 0;
}

LRESULT CCommitDlg::OnAutoListReady(WPARAM, LPARAM)
{
	m_cLogMessage.SetAutoCompletionList(m_autolist, '*');
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
			int eqpos = strLine.Find('=');
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
		TRACE("CFileException loading auto list regex file\n");
		pE->Delete();
		return;
	}
}
void CCommitDlg::GetAutocompletionList()
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
	CRegDWORD regtimeout = CRegDWORD(_T("Software\\TortoiseGit\\AutocompleteParseTimeout"), 5);
	DWORD timeoutvalue = regtimeout*1000;
	sRegexFile += _T("autolist.txt");
	if (!m_bRunThread)
		return;
	ParseRegexFile(sRegexFile, mapRegex);
	SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, sRegexFile.GetBuffer(MAX_PATH+1));
	sRegexFile.ReleaseBuffer();
	sRegexFile += _T("\\TortoiseGit\\autolist.txt");
	if (PathFileExists(sRegexFile))
	{
		ParseRegexFile(sRegexFile, mapRegex);
	}
	DWORD starttime = GetTickCount();

	// now we have two arrays of strings, where the first array contains all
	// file extensions we can use and the second the corresponding regex strings
	// to apply to those files.

	// the next step is to go over all files shown in the commit dialog
	// and scan them for strings we can use
	int nListItems = m_ListCtrl.GetItemCount();

	for (int i=0; i<nListItems && m_bRunThread; ++i)
	{
		// stop parsing after timeout
		if ((!m_bRunThread) || (GetTickCount() - starttime > timeoutvalue))
			return;

		CTGitPath *path = (CTGitPath*)m_ListCtrl.GetItemData(i);

		if(path == NULL)
			continue;

		CString sPartPath =path->GetGitPathString();
		m_autolist.insert(sPartPath);

//		const CGitStatusListCtrl::FileEntry * entry = m_ListCtrl.GetListEntry(i);
//		if (!entry)
//			continue;

		// add the path parts to the auto completion list too
//		CString sPartPath = entry->GetRelativeGitPath();
//		m_autolist.insert(sPartPath);


		int pos = 0;
		int lastPos = 0;
		while ((pos = sPartPath.Find('/', pos)) >= 0)
		{
			pos++;
			lastPos = pos;
			m_autolist.insert(sPartPath.Mid(pos));
		}

		// Last inserted entry is a file name.
		// Some users prefer to also list file name without extension.
		if (CRegDWORD(_T("Software\\TortoiseGit\\AutocompleteRemovesExtensions"), FALSE))
		{
			int dotPos = sPartPath.ReverseFind('.');
			if ((dotPos >= 0) && (dotPos > lastPos))
				m_autolist.insert(sPartPath.Mid(lastPos, dotPos - lastPos));
		}
#if 0
		if ((entry->status <= Git_wc_status_normal)||(entry->status == Git_wc_status_ignored))
			continue;

		CString sExt = entry->GetPath().GetFileExtension();
		sExt.MakeLower();
		// find the regex string which corresponds to the file extension
		CString rdata = mapRegex[sExt];
		if (rdata.IsEmpty())
			continue;

		ScanFile(entry->GetPath().GetWinPathString(), rdata);
		if ((entry->textstatus != Git_wc_status_unversioned) &&
			(entry->textstatus != Git_wc_status_none) &&
			(entry->textstatus != Git_wc_status_ignored) &&
			(entry->textstatus != Git_wc_status_added) &&
			(entry->textstatus != Git_wc_status_normal))
		{
			CTGitPath basePath = Git::GetPristinePath(entry->GetPath());
			if (!basePath.IsEmpty())
				ScanFile(basePath.GetWinPathString(), rdata);
		}
#endif
	}
	ATLTRACE(_T("Auto completion list loaded in %d msec\n"), GetTickCount() - starttime);
}

void CCommitDlg::ScanFile(const CString& sFilePath, const CString& sRegex)
{
	wstring sFileContent;
	HANDLE hFile = CreateFile(sFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		DWORD size = GetFileSize(hFile, NULL);
		if (size > 1000000L)
		{
			// no files bigger than 1 Meg
			CloseHandle(hFile);
			return;
		}
		// allocate memory to hold file contents
		char * buffer = new char[size];
		DWORD readbytes;
		ReadFile(hFile, buffer, size, &readbytes, NULL);
		CloseHandle(hFile);
		int opts = 0;
		IsTextUnicode(buffer, readbytes, &opts);
		if (opts & IS_TEXT_UNICODE_NULL_BYTES)
		{
			delete [] buffer;
			return;
		}
		if (opts & IS_TEXT_UNICODE_UNICODE_MASK)
		{
			sFileContent = wstring((wchar_t*)buffer, readbytes/sizeof(WCHAR));
		}
		if ((opts & IS_TEXT_UNICODE_NOT_UNICODE_MASK)||(opts == 0))
		{
			int ret = MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, (LPCSTR)buffer, readbytes, NULL, 0);
			wchar_t * pWideBuf = new wchar_t[ret];
			int ret2 = MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, (LPCSTR)buffer, readbytes, pWideBuf, ret);
			if (ret2 == ret)
				sFileContent = wstring(pWideBuf, ret);
			delete [] pWideBuf;
		}
		delete [] buffer;
	}
	if (sFileContent.empty()|| !m_bRunThread)
	{
		return;
	}

	try
	{
		const tr1::wregex regCheck(sRegex, tr1::regex_constants::icase | tr1::regex_constants::ECMAScript);
		const tr1::wsregex_iterator end;
		wstring s = sFileContent;
		for (tr1::wsregex_iterator it(s.begin(), s.end(), regCheck); it != end; ++it)
		{
			const tr1::wsmatch match = *it;
			for (size_t i=1; i<match.size(); ++i)
			{
				if (match[i].second-match[i].first)
				{
					ATLTRACE(_T("matched keyword : %s\n"), wstring(match[i]).c_str());
					m_autolist.insert(wstring(match[i]).c_str());
				}
			}
		}
	}
	catch (exception) {}
}

// CSciEditContextMenuInterface
void CCommitDlg::InsertMenuItems(CMenu& mPopup, int& nCmd)
{
	CString sMenuItemText(MAKEINTRESOURCE(IDS_COMMITDLG_POPUP_PASTEFILELIST));
	m_nPopupPasteListCmd = nCmd++;
	mPopup.AppendMenu(MF_STRING | MF_ENABLED, m_nPopupPasteListCmd, sMenuItemText);

	//CString sMenuItemText(MAKEINTRESOURCE(IDS_COMMITDLG_POPUP_PASTEFILELIST));
	if(m_History.GetCount() > 0)
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

	if (m_bBlock)
		return false;
	if (cmd == m_nPopupPasteListCmd)
	{
		CString logmsg;
		int nListItems = m_ListCtrl.GetItemCount();
		for (int i=0; i<nListItems; ++i)
		{
			CTGitPath * entry = (CTGitPath*)m_ListCtrl.GetItemData(i);
			if (entry&&entry->m_Checked)
			{
				CString line;
				CString status = entry->GetActionName();
				if(entry->m_Action & CTGitPath::LOGACTIONS_UNVER)
					status = _T("Add"); // I18N TODO

				//git_wc_status_kind status = entry->status;
				WORD langID = (WORD)CRegStdDWORD(_T("Software\\TortoiseGit\\LanguageID"), GetUserDefaultLangID());
				if (m_ProjectProperties.bFileListInEnglish)
					langID = 1033;

				line.Format(_T("%-10s %s\r\n"),status , (LPCTSTR)m_ListCtrl.GetItemText(i,0));
				logmsg += line;
			}
		}
		pSciEdit->InsertText(logmsg);
		return true;
	}

	if(cmd == m_nPopupPasteLastMessage)
	{
		if(m_History.GetCount() ==0 )
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
			SetTimer(REFRESHTIMER, 200, NULL);
			ATLTRACE("Wait some more before refreshing\n");
		}
		else
		{
			KillTimer(REFRESHTIMER);
			ATLTRACE("Refreshing after items dropped\n");
			Refresh();
		}
		break;
	}
	__super::OnTimer(nIDEvent);
}

void CCommitDlg::OnBnClickedHistory()
{
	m_tooltips.Pop();	// hide the tooltips
	if (m_pathList.GetCount() == 0)
		return;

	CHistoryDlg historyDlg;
	historyDlg.SetHistory(m_History);
	if (historyDlg.DoModal() != IDOK)
		return;

	CString sMsg = historyDlg.GetSelectedText();
	if (sMsg != m_cLogMessage.GetText().Left(sMsg.GetLength()))
	{
		CString sBugID = m_ProjectProperties.GetBugIDFromLog(sMsg);
		if (!sBugID.IsEmpty())
		{
			SetDlgItemText(IDC_BUGID, sBugID);
		}
		if (m_ProjectProperties.sLogTemplate.Compare(m_cLogMessage.GetText())!=0)
			m_cLogMessage.InsertText(sMsg, !m_cLogMessage.GetText().IsEmpty());
		else
			m_cLogMessage.SetText(sMsg);
	}

	UpdateOKButton();
	GetDlgItem(IDC_LOGMESSAGE)->SetFocus();

}

void CCommitDlg::OnBnClickedBugtraqbutton()
{
	m_tooltips.Pop();	// hide the tooltips
	CString sMsg = m_cLogMessage.GetText();

	if (m_BugTraqProvider == NULL)
		return;

	BSTR parameters = m_bugtraq_association.GetParameters().AllocSysString();
	BSTR commonRoot = SysAllocString(g_Git.m_CurrentDir);
	SAFEARRAY *pathList = SafeArrayCreateVector(VT_BSTR, 0, m_pathList.GetCount());

	for (LONG index = 0; index < m_pathList.GetCount(); ++index)
		SafeArrayPutElement(pathList, &index, m_pathList[index].GetGitPathString().AllocSysString());

	BSTR originalMessage = sMsg.AllocSysString();
	BSTR temp = NULL;
//	m_revProps.clear();

	// first try the IBugTraqProvider2 interface
	CComPtr<IBugTraqProvider2> pProvider2 = NULL;
	HRESULT hr = m_BugTraqProvider.QueryInterface(&pProvider2);
	if (SUCCEEDED(hr))
	{
		//CString common = m_ListCtrl.GetCommonURL(false).GetGitPathString();
		BSTR repositoryRoot = g_Git.m_CurrentDir.AllocSysString();
		BSTR bugIDOut = NULL;
		GetDlgItemText(IDC_BUGID, m_sBugID);
		BSTR bugID = m_sBugID.AllocSysString();
		SAFEARRAY * revPropNames = NULL;
		SAFEARRAY * revPropValues = NULL;
		if (FAILED(hr = pProvider2->GetCommitMessage2(GetSafeHwnd(), parameters, repositoryRoot, commonRoot, pathList, originalMessage, bugID, &bugIDOut, &revPropNames, &revPropValues, &temp)))
		{
			CString sErr;
			sErr.Format(IDS_ERR_FAILEDISSUETRACKERCOM, m_bugtraq_association.GetProviderName(), _com_error(hr).ErrorMessage());
			CMessageBox::Show(m_hWnd, sErr, _T("TortoiseGit"), MB_ICONERROR);
		}
		else
		{
			if (bugIDOut)
			{
				m_sBugID = bugIDOut;
				SysFreeString(bugIDOut);
				SetDlgItemText(IDC_BUGID, m_sBugID);
			}
			SysFreeString(bugID);
			SysFreeString(repositoryRoot);
			m_cLogMessage.SetText(temp);
			BSTR HUGEP *pbRevNames;
			BSTR HUGEP *pbRevValues;

			HRESULT hr1 = SafeArrayAccessData(revPropNames, (void HUGEP**)&pbRevNames);
			if (SUCCEEDED(hr1))
			{
				HRESULT hr2 = SafeArrayAccessData(revPropValues, (void HUGEP**)&pbRevValues);
				if (SUCCEEDED(hr2))
				{
					if (revPropNames->rgsabound->cElements == revPropValues->rgsabound->cElements)
					{
						for (ULONG i = 0; i < revPropNames->rgsabound->cElements; i++)
						{
//							m_revProps[pbRevNames[i]] = pbRevValues[i];
						}
					}
					SafeArrayUnaccessData(revPropValues);
				}
				SafeArrayUnaccessData(revPropNames);
			}
			if (revPropNames)
				SafeArrayDestroy(revPropNames);
			if (revPropValues)
				SafeArrayDestroy(revPropValues);
		}
	}
	else
	{
		// if IBugTraqProvider2 failed, try IBugTraqProvider
		CComPtr<IBugTraqProvider> pProvider = NULL;
		hr = m_BugTraqProvider.QueryInterface(&pProvider);
		if (FAILED(hr))
		{
			CString sErr;
			sErr.Format(IDS_ERR_FAILEDISSUETRACKERCOM, (LPCTSTR)m_bugtraq_association.GetProviderName(), _com_error(hr).ErrorMessage());
			CMessageBox::Show(m_hWnd, sErr, _T("TortoiseGit"), MB_ICONERROR);
			return;
		}

		if (FAILED(hr = pProvider->GetCommitMessage(GetSafeHwnd(), parameters, commonRoot, pathList, originalMessage, &temp)))
		{
			CString sErr;
			sErr.Format(IDS_ERR_FAILEDISSUETRACKERCOM, m_bugtraq_association.GetProviderName(), _com_error(hr).ErrorMessage());
			CMessageBox::Show(m_hWnd, sErr, _T("TortoiseGit"), MB_ICONERROR);
		}
		else
			m_cLogMessage.SetText(temp);
	}
	m_sLogMessage = m_cLogMessage.GetText();
	if (!m_ProjectProperties.sMessage.IsEmpty())
	{
		CString sBugID = m_ProjectProperties.FindBugID(m_sLogMessage);
		if (!sBugID.IsEmpty())
		{
			SetDlgItemText(IDC_BUGID, sBugID);
		}
	}

	m_cLogMessage.SetFocus();

	SysFreeString(parameters);
	SysFreeString(commonRoot);
	SafeArrayDestroy(pathList);
	SysFreeString(originalMessage);
	SysFreeString(temp);

}

void CCommitDlg::FillPatchView()
{
	if(::IsWindow(this->m_patchViewdlg.m_hWnd))
	{
		m_patchViewdlg.m_ctrlPatchView.SetText(CString());

		POSITION pos=m_ListCtrl.GetFirstSelectedItemPosition();
		m_patchViewdlg.m_ctrlPatchView.Call(SCI_SETREADONLY, FALSE);
		CString cmd,out;

		while(pos)
		{
			int nSelect = m_ListCtrl.GetNextSelectedItem(pos);
			CTGitPath * p=(CTGitPath*)m_ListCtrl.GetItemData(nSelect);
			if(p && !(p->m_Action&CTGitPath::LOGACTIONS_UNVER) )
			{
				CString head = _T("HEAD");
				if(m_bCommitAmend==TRUE && m_bAmendDiffToLastCommit==FALSE)
					head = _T("HEAD~1");
				cmd.Format(_T("git.exe diff %s -- \"%s\""), head, p->GetGitPathString());
				g_Git.Run(cmd, &out, CP_UTF8);
			}
		}

		m_patchViewdlg.m_ctrlPatchView.SetText(out);
		m_patchViewdlg.m_ctrlPatchView.Call(SCI_SETREADONLY, TRUE);
		m_patchViewdlg.m_ctrlPatchView.Call(SCI_GOTOPOS, 0);
		CRect rect;
		m_patchViewdlg.m_ctrlPatchView.GetClientRect(rect);
		m_patchViewdlg.m_ctrlPatchView.Call(SCI_SETSCROLLWIDTH, rect.Width() - 4);
	}
}
LRESULT CCommitDlg::OnGitStatusListCtrlItemChanged(WPARAM /*wparam*/, LPARAM /*lparam*/)
{
	this->FillPatchView();
	return 0;
}


LRESULT CCommitDlg::OnGitStatusListCtrlCheckChanged(WPARAM, LPARAM)
{
	UpdateOKButton();
	return 0;
}

void CCommitDlg::UpdateOKButton()
{
	if (m_bBlock)
		return;

	bool bValidLogSize = m_cLogMessage.GetText().GetLength() >= m_ProjectProperties.nMinLogSize && m_cLogMessage.GetText().GetLength() > 0;
	bool bAmendOrSelectFilesOrMerge = m_ListCtrl.GetSelected() > 0 || (m_bCommitAmend && m_bAmendDiffToLastCommit) || CTGitPath(g_Git.m_CurrentDir).IsMergeActive();

	DialogEnableWindow(IDOK, bValidLogSize && bAmendOrSelectFilesOrMerge);
}

LRESULT CCommitDlg::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_NOTIFY:
		if (wParam == IDC_SPLITTER)
		{
			SPC_NMHDR* pHdr = (SPC_NMHDR*) lParam;
			DoSize(pHdr->delta);
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
			m_wndSplitter.SetRange(rcTop.top+60, rcMiddle.bottom-80);
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
	RemoveAnchor(IDC_LISTGROUP);
	RemoveAnchor(IDC_FILELIST);
	RemoveAnchor(IDC_TEXT_INFO);

	CSplitterControl::ChangeHeight(&m_cLogMessage, delta, CW_TOPALIGN);
	CSplitterControl::ChangeHeight(GetDlgItem(IDC_MESSAGEGROUP), delta, CW_TOPALIGN);
	CSplitterControl::ChangeHeight(&m_ListCtrl, -delta, CW_BOTTOMALIGN);
	CSplitterControl::ChangeHeight(GetDlgItem(IDC_LISTGROUP), -delta, CW_BOTTOMALIGN);
	CSplitterControl::ChangePos(GetDlgItem(IDC_SIGNOFF),0,delta);
	CSplitterControl::ChangePos(GetDlgItem(IDC_COMMIT_AMEND),0,delta);
	CSplitterControl::ChangePos(GetDlgItem(IDC_COMMIT_AMENDDIFF),0,delta);
	CSplitterControl::ChangePos(GetDlgItem(IDC_COMMIT_SETDATETIME),0,delta);
	CSplitterControl::ChangePos(GetDlgItem(IDC_COMMIT_DATEPICKER),0,delta);
	CSplitterControl::ChangePos(GetDlgItem(IDC_COMMIT_TIMEPICKER),0,delta);
	CSplitterControl::ChangePos(GetDlgItem(IDC_TEXT_INFO),0,delta);

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
	AddAnchor(IDC_TEXT_INFO,TOP_RIGHT);
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
	username.Remove(_T('\n'));
	email.Remove(_T('\n'));

	str.Format(_T("Signed-off-by: %s <%s>"), username, email);

	return str;
}

void CCommitDlg::OnBnClickedSignOff()
{
	CString str = GetSignedOffByLine();

	if (m_cLogMessage.GetText().Find(str) == -1) {
		m_cLogMessage.SetText(m_cLogMessage.GetText().TrimRight());
		int lastNewline = m_cLogMessage.GetText().ReverseFind(_T('\n'));
		int foundByLine = -1;
		if (lastNewline > 0)
			foundByLine = m_cLogMessage.GetText().Find(_T("-by: "), lastNewline);

		if (foundByLine == -1 || foundByLine < lastNewline)
			str = _T("\r\n") + str;

		m_cLogMessage.SetText(m_cLogMessage.GetText()+_T("\r\n")+str+_T("\r\n"));
	}
}

void CCommitDlg::OnBnClickedCommitAmend()
{
	this->UpdateData();
	if(this->m_bCommitAmend && this->m_AmendStr.IsEmpty())
	{
		GitRev rev;
		rev.GetCommit(CString(_T("HEAD")));
		m_AmendStr=rev.GetSubject()+_T("\n")+rev.GetBody();
	}

	if(this->m_bCommitAmend)
	{
		this->m_NoAmendStr=this->m_cLogMessage.GetText();
		m_cLogMessage.SetText(m_AmendStr);
		GetDlgItem(IDC_COMMIT_AMENDDIFF)->ShowWindow(SW_SHOW);
	}
	else
	{
		this->m_AmendStr=this->m_cLogMessage.GetText();
		m_cLogMessage.SetText(m_NoAmendStr);
		GetDlgItem(IDC_COMMIT_AMENDDIFF)->ShowWindow(SW_HIDE);
	}

	OnBnClickedCommitSetDateTime(); // to update the commit date and time

	Refresh();
}

void CCommitDlg::OnBnClickedWholeProject()
{
	m_tooltips.Pop();	// hide the tooltips
	UpdateData();
	m_ListCtrl.Clear();
	if (!m_bBlock)
	{
		if(m_bWholeProject)
			m_ListCtrl.GetStatus(NULL,true,false,true);
		else
			m_ListCtrl.GetStatus(&this->m_pathList,true,false,true);

		DWORD dwShow = m_ListCtrl.GetShowFlags();
		if (DWORD(m_regAddBeforeCommit))
			dwShow |= GITSLC_SHOWUNVERSIONED;
		else
			dwShow &= ~GITSLC_SHOWUNVERSIONED;

		m_ListCtrl.Show(dwShow, dwShow & ~(CTGitPath::LOGACTIONS_UNVER), true);
	}

	SetDlgTitle();
}

void CCommitDlg::OnFocusMessage()
{
	m_cLogMessage.SetFocus();
}

void CCommitDlg::OnScnUpdateUI(NMHDR *pNMHDR, LRESULT *pResult)
{
	UNREFERENCED_PARAMETER(pNMHDR);
	int pos=this->m_cLogMessage.Call(SCI_GETCURRENTPOS);
	int line=this->m_cLogMessage.Call(SCI_LINEFROMPOSITION,pos);
	int column=this->m_cLogMessage.Call(SCI_GETCOLUMN,pos);

	CString str;
	str.Format(_T("%d/%d"),line+1,column+1);
	this->GetDlgItem(IDC_TEXT_INFO)->SetWindowText(str);

	if(*pResult)
		*pResult=0;
}
void CCommitDlg::OnStnClickedViewPatch()
{
	m_patchViewdlg.m_pProjectProperties = &this->m_ProjectProperties;
	m_patchViewdlg.m_ParentCommitDlg = this;
	if(!IsWindow(this->m_patchViewdlg.m_hWnd))
	{
		BOOL viewPatchEnabled = FALSE;
		m_ProjectProperties.GetBOOLProps(viewPatchEnabled, _T("tgit.commitshowpatch"));
		if (viewPatchEnabled == FALSE)
			g_Git.SetConfigValue(_T("tgit.commitshowpatch"), _T("true"));
		m_patchViewdlg.Create(IDD_PATCH_VIEW,this);
		m_patchViewdlg.m_ctrlPatchView.Call(SCI_SETSCROLLWIDTHTRACKING, TRUE);
		CRect rect;
		this->GetWindowRect(&rect);

		m_patchViewdlg.ShowWindow(SW_SHOW);

		m_patchViewdlg.SetWindowPos(NULL,rect.right,rect.top,rect.Width(),rect.Height(),
				SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);

		ShowViewPatchText(false);
		FillPatchView();
	}
	else
	{
		g_Git.SetConfigValue(_T("tgit.commitshowpatch"), _T("false"));
		m_patchViewdlg.ShowWindow(SW_HIDE);
		m_patchViewdlg.DestroyWindow();
		ShowViewPatchText(true);
	}
	this->m_ctrlShowPatch.Invalidate();
}

void CCommitDlg::OnMoving(UINT fwSide, LPRECT pRect)
{
	__super::OnMoving(fwSide, pRect);

	if (::IsWindow(m_patchViewdlg.m_hWnd))
	{
		RECT patchrect;
		m_patchViewdlg.GetWindowRect(&patchrect);
		if (::IsWindow(m_hWnd))
		{
			RECT thisrect;
			GetWindowRect(&thisrect);
			if (patchrect.left == thisrect.right)
			{
				m_patchViewdlg.SetWindowPos(NULL, patchrect.left - (thisrect.left - pRect->left), patchrect.top - (thisrect.top - pRect->top),
					0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
			}
		}
	}

}

void CCommitDlg::OnSizing(UINT fwSide, LPRECT pRect)
{
	__super::OnSizing(fwSide, pRect);

	if(::IsWindow(this->m_patchViewdlg.m_hWnd))
	{
		CRect thisrect, patchrect;
		this->GetWindowRect(thisrect);
		this->m_patchViewdlg.GetWindowRect(patchrect);
		if(thisrect.right==patchrect.left)
		{
			patchrect.left -= (thisrect.right - pRect->right);
			patchrect.right-= (thisrect.right - pRect->right);

			if(	patchrect.bottom == thisrect.bottom)
			{
				patchrect.bottom -= (thisrect.bottom - pRect->bottom);
			}
			if(	patchrect.top == thisrect.top)
			{
				patchrect.top -=  thisrect.top-pRect->top;
			}
			m_patchViewdlg.MoveWindow(patchrect);
		}
	}
}

void CCommitDlg::OnHdnItemchangedFilelist(NMHDR *pNMHDR, LRESULT *pResult)
{
	UNREFERENCED_PARAMETER(pNMHDR);
	*pResult = 0;
	TRACE("Item Changed\r\n");
}

int CCommitDlg::CheckHeadDetach()
{
	CString output;
	if(g_Git.GetCurrentBranchFromFile(g_Git.m_CurrentDir,output))
	{
		int retval = CMessageBox::Show(NULL, IDS_PROC_COMMIT_DETACHEDWARNING, IDS_APPNAME, MB_YESNOCANCEL | MB_ICONWARNING);
		if(retval == IDYES)
		{
			if (CAppUtils::CreateBranchTag(FALSE, NULL, true) == FALSE)
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
			headRevision.GetCommit(_T("HEAD"));
			authordate = headRevision.GetAuthorDate();
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
	}
}

void CCommitDlg::OnBnClickedCheckNewBranch()
{
	UpdateData();
	if (m_bCreateNewBranch)
	{
		GetDlgItem(IDC_COMMIT_TO)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_NEWBRANCH)->ShowWindow(SW_SHOW);
	}
	else
	{
		GetDlgItem(IDC_NEWBRANCH)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_COMMIT_TO)->ShowWindow(SW_SHOW);
	}
}

void CCommitDlg::RestoreFiles(bool doNotAsk)
{
	if (m_ListCtrl.m_restorepaths.size() && (doNotAsk || CMessageBox::Show(m_hWnd, IDS_PROC_COMMIT_RESTOREFILES, IDS_APPNAME, 2, IDI_QUESTION, IDS_PROC_COMMIT_RESTOREFILES_RESTORE, IDS_PROC_COMMIT_RESTOREFILES_KEEP) == 1))
	{
		for (std::map<CString, CString>::iterator it = m_ListCtrl.m_restorepaths.begin(); it != m_ListCtrl.m_restorepaths.end(); ++it)
			CopyFile(it->second, g_Git.m_CurrentDir + _T("\\") + it->first, FALSE);
		m_ListCtrl.m_restorepaths.clear();
	}
}
