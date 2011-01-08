// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseGit

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
#include "CommonResource.h"
#include "UnicodeUtils.h"
#include "ProgressDlg.h"
#include "ShellUpdater.h"
#include "Commands/PushCommand.h"
#include "PatchViewDlg.h"
#include "COMError.h"
#include "Globals.h"

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
	, m_itemsCount(0)
	, m_bSelectFilesForCommit(TRUE)
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
	DDX_Control(pDX, IDC_SELECTALL, m_SelectAll);
	DDX_Text(pDX, IDC_BUGID, m_sBugID);
	DDX_Check(pDX, IDC_WHOLE_PROJECT, m_bWholeProject);
	DDX_Control(pDX, IDC_SPLITTER, m_wndSplitter);
	DDX_Check(pDX, IDC_KEEPLISTS, m_bKeepChangeList);
	DDX_Check(pDX,IDC_COMMIT_AMEND,m_bCommitAmend);
	DDX_Control(pDX,IDC_VIEW_PATCH,m_ctrlShowPatch);
}

BEGIN_MESSAGE_MAP(CCommitDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_SELECTALL, OnBnClickedSelectall)
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
	ON_BN_CLICKED(IDC_SHOWUNVERSIONED, OnBnClickedShowunversioned)
	ON_NOTIFY(SCN_UPDATEUI, IDC_LOGMESSAGE, OnScnUpdateUI)
//	ON_BN_CLICKED(IDC_HISTORY, OnBnClickedHistory)
	ON_BN_CLICKED(IDC_BUGTRAQBUTTON, OnBnClickedBugtraqbutton)
	ON_EN_CHANGE(IDC_LOGMESSAGE, OnEnChangeLogmessage)
	ON_REGISTERED_MESSAGE(CGitStatusListCtrl::SVNSLNM_ITEMCOUNTCHANGED, OnGitStatusListCtrlItemCountChanged)
	ON_REGISTERED_MESSAGE(CGitStatusListCtrl::SVNSLNM_NEEDSREFRESH, OnGitStatusListCtrlNeedsRefresh)
	ON_REGISTERED_MESSAGE(CGitStatusListCtrl::SVNSLNM_ADDFILE, OnFileDropped)
	ON_REGISTERED_MESSAGE(CGitStatusListCtrl::SVNSLNM_CHECKCHANGED, &CCommitDlg::OnGitStatusListCtrlCheckChanged)
	ON_REGISTERED_MESSAGE(CGitStatusListCtrl::SVNSLNM_ITEMCHANGED, &CCommitDlg::OnGitStatusListCtrlItemChanged)
	
	ON_REGISTERED_MESSAGE(WM_AUTOLISTREADY, OnAutoListReady) 
	ON_WM_TIMER()
    ON_WM_SIZE()
	ON_STN_CLICKED(IDC_EXTERNALWARNING, &CCommitDlg::OnStnClickedExternalwarning)
	ON_BN_CLICKED(IDC_SIGNOFF, &CCommitDlg::OnBnClickedSignOff)
	ON_STN_CLICKED(IDC_COMMITLABEL, &CCommitDlg::OnStnClickedCommitlabel)
    ON_BN_CLICKED(IDC_COMMIT_AMEND, &CCommitDlg::OnBnClickedCommitAmend)
    ON_BN_CLICKED(IDC_WHOLE_PROJECT, &CCommitDlg::OnBnClickedWholeProject)
	ON_STN_CLICKED(IDC_BUGIDLABEL, &CCommitDlg::OnStnClickedBugidlabel)
	ON_COMMAND(ID_FOCUS_MESSAGE,&CCommitDlg::OnFocusMessage)
	ON_STN_CLICKED(IDC_VIEW_PATCH, &CCommitDlg::OnStnClickedViewPatch)
	ON_WM_MOVE()
	ON_WM_MOVING()
	ON_WM_SIZING()
	ON_NOTIFY(HDN_ITEMCHANGED, 0, &CCommitDlg::OnHdnItemchangedFilelist)
END_MESSAGE_MAP()

BOOL CCommitDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();
	
	CAppUtils::GetCommitTemplate(this->m_sLogMessage);

	if(PathFileExists(g_Git.m_CurrentDir+_T("\\.git\\MERGE_MSG")))
	{
		CStdioFile file;
		if(file.Open(g_Git.m_CurrentDir+_T("\\.git\\MERGE_MSG"), CFile::modeRead))
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
	m_regAddBeforeCommit = CRegDWORD(_T("Software\\TortoiseGit\\AddBeforeCommit"), TRUE);
	m_bShowUnversioned = m_regAddBeforeCommit;

	m_History.SetMaxHistoryItems((LONG)CRegDWORD(_T("Software\\TortoiseGit\\MaxHistoryItems"), 25));

	m_regKeepChangelists = CRegDWORD(_T("Software\\TortoiseGit\\KeepChangeLists"), FALSE);
	m_bKeepChangeList = m_regKeepChangelists;

	m_hAccel = LoadAccelerators(AfxGetResourceHandle(),MAKEINTRESOURCE(IDR_ACC_COMMITDLG));

//	GitConfig config;
//	m_bWholeProject = config.KeepLocks();

	if(this->m_pathList.GetCount() == 0)
		m_bWholeProject =true;
	
	if(this->m_pathList.GetCount() == 1 && m_pathList[0].IsEmpty())
		m_bWholeProject =true;

	UpdateData(FALSE);
	
	m_ListCtrl.Init(SVNSLC_COLEXT | SVNSLC_COLSTATUS | SVNSLC_COLADD |SVNSLC_COLDEL, _T("CommitDlg"),(SVNSLC_POPALL ^ (SVNSLC_POPCOMMIT | SVNSLC_POPSAVEAS)));
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
	AddAnchor(IDC_MESSAGEGROUP, TOP_LEFT, TOP_RIGHT);
//	AddAnchor(IDC_HISTORY, TOP_LEFT);
	AddAnchor(IDC_LOGMESSAGE, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_SIGNOFF,   TOP_RIGHT);
	AddAnchor(IDC_VIEW_PATCH,TOP_RIGHT);
	AddAnchor(IDC_LISTGROUP, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SPLITTER, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_FILELIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SHOWUNVERSIONED, BOTTOM_LEFT);
	AddAnchor(IDC_SELECTALL, BOTTOM_LEFT);
	AddAnchor(IDC_EXTERNALWARNING, BOTTOM_RIGHT);
	AddAnchor(IDC_STATISTICS, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_TEXT_INFO,  TOP_RIGHT);
	AddAnchor(IDC_WHOLE_PROJECT, BOTTOM_LEFT);
	AddAnchor(IDC_KEEPLISTS, BOTTOM_LEFT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	AddAnchor(IDC_COMMIT_AMEND,TOP_LEFT);
	
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

	//this->UpdateData(TRUE);
	//this->m_bCommitAmend=FALSE;
	//this->UpdateData(FALSE);

	this->m_ctrlShowPatch.SetURL(CString());

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

	CString id;
	GetDlgItemText(IDC_BUGID, id);
	if (!m_ProjectProperties.CheckBugID(id))
	{
		ShowBalloon(IDC_BUGID, IDS_COMMITDLG_ONLYNUMBERS, IDI_EXCLAMATION);
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

	m_ListCtrl.WriteCheckedNamesToPathList(m_selectedPathList);
#if 0
	CRegDWORD regUnversionedRecurse (_T("Software\\TortoiseGit\\UnversionedRecurse"), TRUE);
	if (!(DWORD)regUnversionedRecurse)
	{
		// Find unversioned directories which are marked for commit. The user might expect them
		// to be added recursively since he cannot the the files. Let's ask the user if he knows
		// what he is doing.
		int nListItems = m_ListCtrl.GetItemCount();
		for (int j=0; j<nListItems; j++)
		{
			const CGitStatusListCtrl::FileEntry * entry = m_ListCtrl.GetListEntry(j);
			if (entry->IsChecked() && (entry->status == Git_wc_status_unversioned) && entry->IsFolder() )
			{
				if (CMessageBox::Show(this->m_hWnd, IDS_COMMITDLG_UNVERSIONEDFOLDERWARNING, IDS_APPNAME, MB_YESNO | MB_ICONWARNING)!=IDYES)
					return;
			}
		}
	}
#endif
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
				CMessageBox::Show(m_hWnd, sErr, _T("TortoiseSVN"), MB_ICONERROR);
			}
			else
			{
				CString sError = temp;
				if (!sError.IsEmpty())
				{
					CMessageBox::Show(m_hWnd, sError, _T("TortoiseSVN"), MB_ICONERROR);
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

	for (int j=0; j<nListItems; j++)
	{
		//const CGitStatusListCtrl::FileEntry * entry = m_ListCtrl.GetListEntry(j);
		CTGitPath *entry = (CTGitPath*)m_ListCtrl.GetItemData(j);
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

			if(g_Git.Run(cmd,&out,CP_ACP))
			{
				CMessageBox::Show(NULL,out,_T("TortoiseGit"),MB_OK|MB_ICONERROR);
				bAddSuccess = false ;
				break;
			}

			if( entry->m_Action & CTGitPath::LOGACTIONS_REPLACED)
				cmd.Format(_T("git.exe rm -- \"%s\""), entry->GetGitOldPathString());

			g_Git.Run(cmd,&out,CP_ACP);

			nchecked++;

			//checkedLists.insert(entry->GetGitPathString());
//			checkedfiles += _T("\"")+entry->GetGitPathString()+_T("\" ");
		}
		else
		{
			//uncheckedLists.insert(entry->GetGitPathString());
			if(entry->m_Action & CTGitPath::LOGACTIONS_ADDED)
			{	//To init git repository, there are not HEAD, so we can use git reset command
				cmd.Format(_T("git.exe rm --cache -- \"%s\""),entry->GetGitPathString());
				if(g_Git.Run(cmd,&out,CP_ACP))
				{
					CMessageBox::Show(NULL,out,_T("TortoiseGit"),MB_OK|MB_ICONERROR);
					bAddSuccess = false ;
					bCloseCommitDlg=false;
					break;
				}
				
			}
			else if(!( entry->m_Action & CTGitPath::LOGACTIONS_UNVER ) )
			{
				cmd.Format(_T("git.exe reset -- \"%s\""),entry->GetGitPathString());
				if(g_Git.Run(cmd,&out,CP_ACP))
				{
					/* when reset a unstage file will report error. 
					CMessageBox::Show(NULL,out,_T("TortoiseGit"),MB_OK|MB_ICONERROR);
					bAddSuccess = false ;
					bCloseCommitDlg=false;
					break;
					*/
				}
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

		CShellUpdater::Instance().AddPathForUpdate(*entry);
	}

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

	BOOL bIsMerge=false;
	if(PathFileExists(g_Git.m_CurrentDir+_T("\\.git\\MERGE_HEAD")))
	{
		bIsMerge=true;
	}
	//if(checkedfiles.GetLength()>0)
	if( bAddSuccess && (nchecked||m_bCommitAmend||bIsMerge) )
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
		cmd.Format(_T("git.exe commit %s -F \"%s\""),amend, tempfile);
		
		CCommitProgressDlg progress;
		progress.m_bBufferAll=true; // improve show speed when there are many file added. 
		progress.m_GitCmd=cmd;
		progress.m_bShowCommand = FALSE;	// don't show the commit command
		progress.m_PreText = out;			// show any output already generated in log window

		progress.m_PostCmdList.Add( IsGitSVN? _T("&DCommit"): _T("&Push"));
		progress.m_PostCmdList.Add(_T("&ReCommit"));
		
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

				CString hash=g_Git.GetHash(CString(_T("HEAD")));
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
		
	}else if(bAddSuccess)
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

    if(m_bWholeProject)
		pList=NULL;
    else
		pList = &m_pathList;
    
	success=m_ListCtrl.GetStatus(pList);

	//m_ListCtrl.UpdateFileList(git_revnum_t(GIT_REV_ZERO));
	if(this->m_bShowUnversioned)
		m_ListCtrl.UpdateFileList(CGitStatusListCtrl::FILELIST_UNVER,true,pList);
	
	m_ListCtrl.CheckIfChangelistsArePresent(false);

	DWORD dwShow = SVNSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALSFROMDIFFERENTREPOS | SVNSLC_SHOWLOCKS | SVNSLC_SHOWINCHANGELIST;
	dwShow |= DWORD(m_regAddBeforeCommit) ? SVNSLC_SHOWUNVERSIONED : 0;
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
	CString logmsg;
	GetDlgItemText(IDC_LOGMESSAGE, logmsg);
	DialogEnableWindow(IDOK, logmsg.GetLength() >= m_ProjectProperties.nMinLogSize);
	if (!success)
	{
		if (!m_ListCtrl.GetLastErrorMessage().IsEmpty())
			m_ListCtrl.SetEmptyString(m_ListCtrl.GetLastErrorMessage());
		m_ListCtrl.Show(dwShow);
	}
	if ((m_ListCtrl.GetItemCount()==0)&&(m_ListCtrl.HasUnversionedItems())
		 && !PathFileExists(g_Git.m_CurrentDir+_T("\\.git\\MERGE_HEAD")))
	{
		if (CMessageBox::Show(m_hWnd, IDS_COMMITDLG_NOTHINGTOCOMMITUNVERSIONED, IDS_APPNAME, MB_ICONINFORMATION | MB_YESNO)==IDYES)
		{
			m_bShowUnversioned = TRUE;
			GetDlgItem(IDC_SHOWUNVERSIONED)->SendMessage(BM_SETCHECK, BST_CHECKED);
			DWORD dwShow = (DWORD)(SVNSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALSFROMDIFFERENTREPOS | SVNSLC_SHOWUNVERSIONED | SVNSLC_SHOWLOCKS);
			m_ListCtrl.UpdateFileList(CGitStatusListCtrl::FILELIST_UNVER);
			m_ListCtrl.Show(dwShow,dwShow&(~CTGitPath::LOGACTIONS_UNVER));
		}
	}

	CTGitPath commonDir = m_ListCtrl.GetCommonDirectory(false);

    if(this->m_bWholeProject)   
        SetWindowText(m_sWindowTitle + _T(" - ") + commonDir.GetWinPathString() + CString(_T(" (Whole Project)")));
    else
	    SetWindowText(m_sWindowTitle + _T(" - ") + commonDir.GetWinPathString());

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
	if (m_bRunThread)
	{
		DialogEnableWindow(IDC_SHOWUNVERSIONED, true);
        DialogEnableWindow(IDC_WHOLE_PROJECT, true);
		DialogEnableWindow(IDC_SELECTALL, true);
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

void CCommitDlg::OnCancel()
{
	m_bCancelled = true;
	m_pathwatcher.Stop();

	if (m_bBlock)
		return;
	
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
			dwShow |= SVNSLC_SHOWUNVERSIONED;
		else
			dwShow &= ~SVNSLC_SHOWUNVERSIONED;
		if(dwShow & SVNSLC_SHOWUNVERSIONED)
		{
            if(m_bWholeProject)
                m_ListCtrl.GetStatus(NULL,false,false,true);
            else
			    m_ListCtrl.GetStatus(&this->m_pathList,false,false,true);
		}
		m_ListCtrl.Show(dwShow,0,true,0,true);
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
			int ret = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (LPCSTR)buffer, readbytes, NULL, 0);
			wchar_t * pWideBuf = new wchar_t[ret];
			int ret2 = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (LPCSTR)buffer, readbytes, pWideBuf, ret);
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
		TCHAR buf[MAX_STATUS_STRING_LENGTH];
		int nListItems = m_ListCtrl.GetItemCount();
		for (int i=0; i<nListItems; ++i)
		{
			CTGitPath * entry = (CTGitPath*)m_ListCtrl.GetItemData(i);
			if (entry&&entry->m_Checked)
			{
				CString line;
				CString status = entry->GetActionName();
				if(entry->m_Action & CTGitPath::LOGACTIONS_UNVER)
					status = _T("Add");

				//git_wc_status_kind status = entry->status;
				WORD langID = (WORD)CRegStdWORD(_T("Software\\TortoiseGit\\LanguageID"), GetUserDefaultLangID());
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
				cmd.Format(_T("git.exe diff -- \"%s\""),p->GetGitPathString());
				g_Git.Run(cmd,&out,CP_ACP);

			}

		}

		m_patchViewdlg.m_ctrlPatchView.SetText(out);
		m_patchViewdlg.m_ctrlPatchView.Call(SCI_SETREADONLY, TRUE);
		m_patchViewdlg.m_ctrlPatchView.Call(SCI_GOTOPOS, 0);

	}

}
LRESULT CCommitDlg::OnGitStatusListCtrlItemChanged(WPARAM wparam, LPARAM lparam)
{
	TRACE("OnGitStatusListCtrlItemChanged %d\r\n", wparam);
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
#if 0
	BOOL bValidLogSize = FALSE;

    if (m_cLogMessage.GetText().GetLength() >= m_ProjectProperties.nMinLogSize)
		bValidLogSize = !m_bBlock;

	LONG nSelectedItems = m_ListCtrl.GetSelected();
	DialogEnableWindow(IDOK, bValidLogSize && nSelectedItems>0);
#endif
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
	RemoveAnchor(IDC_LISTGROUP);
	RemoveAnchor(IDC_FILELIST);
	RemoveAnchor(IDC_TEXT_INFO);
	RemoveAnchor(IDC_VIEW_PATCH);

	CSplitterControl::ChangeHeight(&m_cLogMessage, delta, CW_TOPALIGN);
	CSplitterControl::ChangeHeight(GetDlgItem(IDC_MESSAGEGROUP), delta, CW_TOPALIGN);
	CSplitterControl::ChangeHeight(&m_ListCtrl, -delta, CW_BOTTOMALIGN);
	CSplitterControl::ChangeHeight(GetDlgItem(IDC_LISTGROUP), -delta, CW_BOTTOMALIGN);
	CSplitterControl::ChangePos(GetDlgItem(IDC_SIGNOFF),0,delta);
	CSplitterControl::ChangePos(GetDlgItem(IDC_COMMIT_AMEND),0,delta);
	CSplitterControl::ChangePos(GetDlgItem(IDC_TEXT_INFO),0,delta);
	CSplitterControl::ChangePos(GetDlgItem(IDC_VIEW_PATCH),0,delta);
	
	AddAnchor(IDC_VIEW_PATCH,TOP_RIGHT);
	AddAnchor(IDC_MESSAGEGROUP, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_LOGMESSAGE, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_SPLITTER, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_LISTGROUP, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_FILELIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SIGNOFF,TOP_RIGHT);
	AddAnchor(IDC_COMMIT_AMEND,TOP_LEFT);
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


void CCommitDlg::OnBnClickedSignOff()
{
	// TODO: Add your control notification handler code here
	CString str;
	CString username;
	CString email;
	username=g_Git.GetUserName();
	email=g_Git.GetUserEmail();
	username.Remove(_T('\n'));
	email.Remove(_T('\n'));
	str.Format(_T("Signed-off-by: %s <%s>\n"),username,email);

	m_cLogMessage.SetText(m_cLogMessage.GetText()+_T("\r\n\r\n")+str);
}

void CCommitDlg::OnStnClickedCommitlabel()
{
	// TODO: Add your control notification handler code here
}

void CCommitDlg::OnBnClickedCommitAmend()
{
    // TODO: Add your control notification handler code here
	this->UpdateData();
	if(this->m_bCommitAmend && this->m_AmendStr.IsEmpty())
	{
		GitRev rev;
		rev.GetCommit(CString(_T("HEAD")));
		m_AmendStr=rev.m_Subject+_T("\n\n")+rev.m_Body;
	}

	if(this->m_bCommitAmend)
	{
		this->m_NoAmendStr=this->m_cLogMessage.GetText();
		m_cLogMessage.SetText(m_AmendStr);

	}else
	{
		this->m_AmendStr=this->m_cLogMessage.GetText();
		m_cLogMessage.SetText(m_NoAmendStr);

	}

}

void CCommitDlg::OnBnClickedWholeProject()
{
    // TODO: Add your control notification handler code here
    m_tooltips.Pop();	// hide the tooltips
	UpdateData();
    m_ListCtrl.Clear();
	if (!m_bBlock)
	{
	    if(m_bWholeProject)
            m_ListCtrl.GetStatus(NULL,true,false,true);
        else
		    m_ListCtrl.GetStatus(&this->m_pathList,true,false,true);
		
		m_ListCtrl.Show(m_ListCtrl.GetShowFlags());
	}

   	CTGitPath commonDir = m_ListCtrl.GetCommonDirectory(false);

    if(this->m_bWholeProject)   
        SetWindowText(m_sWindowTitle + _T(" - ") + CString(_T("Whole Project")));
    else
	    SetWindowText(m_sWindowTitle + _T(" - ") + commonDir.GetWinPathString());

}

void CCommitDlg::OnStnClickedBugidlabel()
{
	// TODO: Add your control notification handler code here
}

void CCommitDlg::OnFocusMessage()
{
	m_cLogMessage.SetFocus();
}

void CCommitDlg::OnScnUpdateUI(NMHDR *pNMHDR, LRESULT *pResult)
{
	SCNotification *pHead =(SCNotification *)pNMHDR;
	
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
	// TODO: Add your control notification handler code here
	
	m_patchViewdlg.m_pProjectProperties = &this->m_ProjectProperties;
	m_patchViewdlg.m_ParentCommitDlg = this;
	if(!IsWindow(this->m_patchViewdlg.m_hWnd))
	{
		m_patchViewdlg.Create(IDD_PATCH_VIEW,this);
		CRect rect;
		this->GetWindowRect(&rect);
		
		m_patchViewdlg.SetWindowPos(NULL,rect.right,rect.top,rect.Width(),rect.Height(),
				SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
		
		m_patchViewdlg.m_ctrlPatchView.MoveWindow(0,0,rect.Width(),rect.Height());
		m_patchViewdlg.ShowWindow(SW_SHOW);
		
		ShowViewPatchText(false);
		FillPatchView();
	}
	else
	{
		m_patchViewdlg.ShowWindow(SW_HIDE);
		m_patchViewdlg.DestroyWindow();
		ShowViewPatchText(true);
	}
	this->m_ctrlShowPatch.Invalidate();
}

void CCommitDlg::OnMove(int x, int y)
{
	__super::OnMove(x, y);

	// TODO: Add your message handler code here
}

void CCommitDlg::OnMoving(UINT fwSide, LPRECT pRect)
{
	__super::OnMoving(fwSide, pRect);

	// TODO: Add your message handler code here
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
			int width = patchrect.Width();
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
	// TODO: Add your message handler code here
}

void CCommitDlg::OnHdnItemchangedFilelist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;
	TRACE("Item Changed\r\n");
}
