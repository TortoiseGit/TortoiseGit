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

#include "stdafx.h"
#include "TortoiseProc.h"
#include "cursor.h"
#include "InputDlg.h"
#include "GITProgressDlg.h"
#include "ProgressDlg.h"
//#include "CopyDlg.h"
#include "StatGraphDlg.h"
#include "Logdlg.h"
#include "MessageBox.h"
#include "Registry.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "StringUtils.h"
#include "UnicodeUtils.h"
#include "TempFile.h"
//#include "GitInfo.h"
//#include "GitDiff.h"
#include "IconMenu.h"
//#include "BrowseFolder.h"
//#include "GitHelpers.h"
#include "GitStatus.h"
//#include "LogDlgHelper.h"
//#include "CachedLogInfo.h"
#include "FileDiffDlg.h"
#include "BrowseRefsDlg.h"
#include "SmartHandle.h"

IMPLEMENT_DYNAMIC(CLogDlg, CResizableStandAloneDialog)
CLogDlg::CLogDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CLogDlg::IDD, pParent)
	, m_wParam(0)
	, m_currentChangedArray(NULL)
	, m_nSortColumn(0)
	, m_bFollowRenames(FALSE)
	, m_bSelect(false)

	, m_bSelectionMustBeContinuous(false)

	, m_sLogInfo(_T(""))

	, m_bCancelled(FALSE)
	, m_pNotifyWindow(NULL)

	, m_bAscending(FALSE)

	, m_limit(0)
	, m_hAccel(NULL)
{
	m_bFilterWithRegex = !!CRegDWORD(_T("Software\\TortoiseGit\\UseRegexFilter"), TRUE);

	CString str;
	str=g_Git.m_CurrentDir;
	str.Replace(_T(":"),_T("_"));
	str=CString(_T("Software\\TortoiseGit\\LogDialog\\AllBranch\\"))+str;

	m_regbAllBranch=CRegDWORD(str,FALSE);

	m_bAllBranch=m_regbAllBranch;

	m_bFirstParent=FALSE;
	m_bWholeProject=FALSE;
}

CLogDlg::~CLogDlg()
{

	m_regbAllBranch=m_bAllBranch;

	m_CurrentFilteredChangedArray.RemoveAll();

}

void CLogDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LOGLIST, m_LogList);
	DDX_Control(pDX, IDC_LOGMSG, m_ChangedFileListCtrl);
	DDX_Control(pDX, IDC_PROGRESS, m_LogProgress);
	DDX_Control(pDX, IDC_SPLITTERTOP, m_wndSplitter1);
	DDX_Control(pDX, IDC_SPLITTERBOTTOM, m_wndSplitter2);
	DDX_Text(pDX, IDC_SEARCHEDIT, m_LogList.m_sFilterText);
	DDX_Control(pDX, IDC_DATEFROM, m_DateFrom);
	DDX_Control(pDX, IDC_DATETO, m_DateTo);
	DDX_Control(pDX, IDC_HIDEPATHS, m_cHidePaths);
	DDX_Text(pDX, IDC_LOGINFO, m_sLogInfo);
	DDX_Check(pDX, IDC_LOG_FIRSTPARENT, m_bFirstParent);
	DDX_Check(pDX, IDC_LOG_ALLBRANCH,m_bAllBranch);
	DDX_Check(pDX, IDC_LOG_FOLLOWRENAMES, m_bFollowRenames);
	DDX_Check(pDX, IDC_SHOWWHOLEPROJECT,m_bWholeProject);
	DDX_Control(pDX, IDC_SEARCHEDIT, m_cFilter);
	DDX_Control(pDX, IDC_STATIC_REF, m_staticRef);
}

BEGIN_MESSAGE_MAP(CLogDlg, CResizableStandAloneDialog)
	ON_WM_CONTEXTMENU()
	ON_WM_SETCURSOR()
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LOGLIST, OnLvnItemchangedLoglist)
	ON_NOTIFY(EN_LINK, IDC_MSGVIEW, OnEnLinkMsgview)
	ON_BN_CLICKED(IDC_STATBUTTON, OnBnClickedStatbutton)

	ON_MESSAGE(WM_FILTEREDIT_INFOCLICKED, OnClickedInfoIcon)
	ON_MESSAGE(WM_FILTEREDIT_CANCELCLICKED, OnClickedCancelFilter)

	ON_MESSAGE(MSG_LOAD_PERCENTAGE,OnLogListLoading)

	ON_EN_CHANGE(IDC_SEARCHEDIT, OnEnChangeSearchedit)
	ON_WM_TIMER()
	ON_NOTIFY(DTN_DATETIMECHANGE, IDC_DATETO, OnDtnDatetimechangeDateto)
	ON_NOTIFY(DTN_DATETIMECHANGE, IDC_DATEFROM, OnDtnDatetimechangeDatefrom)
	ON_BN_CLICKED(IDC_SHOWWHOLEPROJECT, OnBnClickShowWholeProject)
	ON_NOTIFY(LVN_COLUMNCLICK,IDC_LOGLIST, OnLvnColumnclick)
	ON_BN_CLICKED(IDC_HIDEPATHS, OnBnClickedHidepaths)
	ON_COMMAND(MSG_FETCHED_DIFF, OnBnClickedHidepaths)
	ON_BN_CLICKED(IDC_LOG_ALLBRANCH, OnBnClickedAllBranch)
	ON_BN_CLICKED(IDC_LOG_FOLLOWRENAMES, OnBnClickedFollowRenames)

	ON_NOTIFY(DTN_DROPDOWN, IDC_DATEFROM, &CLogDlg::OnDtnDropdownDatefrom)
	ON_NOTIFY(DTN_DROPDOWN, IDC_DATETO, &CLogDlg::OnDtnDropdownDateto)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_LOG_FIRSTPARENT, &CLogDlg::OnBnClickedFirstParent)
	ON_BN_CLICKED(IDC_REFRESH, &CLogDlg::OnBnClickedRefresh)
	ON_STN_CLICKED(IDC_STATIC_REF, &CLogDlg::OnBnClickedBrowseRef)
	ON_COMMAND(ID_LOGDLG_REFRESH, &CLogDlg::OnBnClickedRefresh)
	ON_COMMAND(ID_LOGDLG_FIND, &CLogDlg::OnFind)
	ON_COMMAND(ID_LOGDLG_FOCUSFILTER, &CLogDlg::OnFocusFilter)
	ON_COMMAND(ID_EDIT_COPY, &CLogDlg::OnEditCopy)
	ON_MESSAGE(MSG_REFLOG_CHANGED, OnRefLogChanged)
	ON_REGISTERED_MESSAGE(WM_TASKBARBTNCREATED, OnTaskbarBtnCreated)
END_MESSAGE_MAP()

void CLogDlg::SetParams(const CTGitPath& orgPath, const CTGitPath& path, CString hightlightRevision, CString startrev, CString endrev, int limit /* = FALSE */)
{
	m_orgPath = orgPath;
	m_path = path;
	m_hightlightRevision = hightlightRevision;

	if (startrev == GIT_REV_ZERO)
		startrev.Empty();
	if (endrev == GIT_REV_ZERO)
		endrev.Empty();

	this->m_LogList.m_startrev = startrev;
	m_LogRevision = startrev;
	this->m_LogList.m_endrev = endrev;

	if(!endrev.IsEmpty())
		this->SetStartRef(endrev);

	m_limit = limit;
	if (::IsWindow(m_hWnd))
		UpdateData(FALSE);
}

void CLogDlg::SetFilter(const CString& findstr, LONG findtype, bool findregex)
{
	m_LogList.m_sFilterText = findstr;
	if (findtype)
		m_LogList.m_SelectedFilters = findtype;
	m_LogList.m_bFilterWithRegex = m_bFilterWithRegex = findregex;
}

BOOL CLogDlg::OnInitDialog()
{
	CString temp;
	CResizableStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	// Let the TaskbarButtonCreated message through the UIPI filter. If we don't
	// do this, Explorer would be unable to send that message to our window if we
	// were running elevated. It's OK to make the call all the time, since if we're
	// not elevated, this is a no-op.
	CHANGEFILTERSTRUCT cfs = { sizeof(CHANGEFILTERSTRUCT) };
	typedef BOOL STDAPICALLTYPE ChangeWindowMessageFilterExDFN(HWND hWnd, UINT message, DWORD action, PCHANGEFILTERSTRUCT pChangeFilterStruct);
	CAutoLibrary hUser = ::LoadLibrary(_T("user32.dll"));
	if (hUser)
	{
		ChangeWindowMessageFilterExDFN *pfnChangeWindowMessageFilterEx = (ChangeWindowMessageFilterExDFN*)GetProcAddress(hUser, "ChangeWindowMessageFilterEx");
		if (pfnChangeWindowMessageFilterEx)
		{
			pfnChangeWindowMessageFilterEx(m_hWnd, WM_TASKBARBTNCREATED, MSGFLT_ALLOW, &cfs);
		}
	}
	m_pTaskbarList.Release();
	m_pTaskbarList.CoCreateInstance(CLSID_TaskbarList);

	m_hAccel = LoadAccelerators(AfxGetResourceHandle(),MAKEINTRESOURCE(IDR_ACC_LOGDLG));

	// use the state of the "stop on copy/rename" option from the last time
	UpdateData(FALSE);

	// set the font to use in the log message view, configured in the settings dialog
	CAppUtils::CreateFontForLogs(m_logFont);
	GetDlgItem(IDC_MSGVIEW)->SetFont(&m_logFont);
	// automatically detect URLs in the log message and turn them into links
	GetDlgItem(IDC_MSGVIEW)->SendMessage(EM_AUTOURLDETECT, TRUE, NULL);
	// make the log message rich edit control send a message when the mouse pointer is over a link
	GetDlgItem(IDC_MSGVIEW)->SendMessage(EM_SETEVENTMASK, NULL, ENM_LINK);
	//m_LogList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_SUBITEMIMAGES);

	// the "hide unrelated paths" checkbox should be indeterminate
	m_cHidePaths.SetCheck(BST_INDETERMINATE);


	//SetWindowTheme(m_LogList.GetSafeHwnd(), L"Explorer", NULL);
	//SetWindowTheme(m_ChangedFileListCtrl.GetSafeHwnd(), L"Explorer", NULL);

	// set up the columns
	m_LogList.DeleteAllItems();

	m_LogList.m_Path=m_path;
	m_LogList.m_hasWC = m_LogList.m_bShowWC = !g_GitAdminDir.IsBareRepo(g_Git.m_CurrentDir);
	m_LogList.InsertGitColumn();

	m_ChangedFileListCtrl.Init(GITSLC_COLEXT | GITSLC_COLSTATUS |GITSLC_COLADD|GITSLC_COLDEL, _T("LogDlg"), (GITSLC_POPALL ^ (GITSLC_POPCOMMIT|GITSLC_POPIGNORE|GITSLC_POPRESTORE)), false, m_LogList.m_hasWC, GITSLC_COLEXT | GITSLC_COLSTATUS | GITSLC_COLADD| GITSLC_COLDEL);

	GetDlgItem(IDC_LOGLIST)->UpdateData(FALSE);

	SetDlgTitle();

	m_tooltips.Create(this);
	CheckRegexpTooltip();

	SetSplitterRange();

	// the filter control has a 'cancel' button (the red 'X'), we need to load its bitmap
	m_cFilter.SetCancelBitmaps(IDI_CANCELNORMAL, IDI_CANCELPRESSED);
	m_cFilter.SetInfoIcon(IDI_LOGFILTER);
	m_cFilter.SetValidator(this);

	AdjustControlSize(IDC_HIDEPATHS);
	AdjustControlSize(IDC_LOG_FIRSTPARENT);
	AdjustControlSize(IDC_LOG_ALLBRANCH);
	AdjustControlSize(IDC_SHOWWHOLEPROJECT);
	AdjustControlSize(IDC_LOG_FOLLOWRENAMES);

	GetClientRect(m_DlgOrigRect);
	m_LogList.GetClientRect(m_LogListOrigRect);
	GetDlgItem(IDC_MSGVIEW)->GetClientRect(m_MsgViewOrigRect);
	m_ChangedFileListCtrl.GetClientRect(m_ChgOrigRect);

	m_DateFrom.SendMessage(DTM_SETMCSTYLE, 0, MCS_WEEKNUMBERS|MCS_NOTODAY|MCS_NOTRAILINGDATES|MCS_NOSELCHANGEONNAV);
	m_DateTo.SendMessage(DTM_SETMCSTYLE, 0, MCS_WEEKNUMBERS|MCS_NOTODAY|MCS_NOTRAILINGDATES|MCS_NOSELCHANGEONNAV);

	m_staticRef.SetURL(CString());

	// resizable stuff
	AddAnchor(IDC_STATIC_REF, TOP_LEFT);
	//AddAnchor(IDC_BUTTON_BROWSE_REF, TOP_LEFT);
	AddAnchor(IDC_FROMLABEL, TOP_LEFT);
	AddAnchor(IDC_DATEFROM, TOP_LEFT);
	AddAnchor(IDC_TOLABEL, TOP_LEFT);
	AddAnchor(IDC_DATETO, TOP_LEFT);

	SetFilterCueText();
	AddAnchor(IDC_SEARCHEDIT, TOP_LEFT, TOP_RIGHT);

	AddAnchor(IDC_LOGLIST, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_SPLITTERTOP, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_MSGVIEW, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SPLITTERBOTTOM, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_LOGMSG, BOTTOM_LEFT, BOTTOM_RIGHT);

	AddAnchor(IDC_LOGINFO, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_HIDEPATHS, BOTTOM_LEFT);
	AddAnchor(IDC_LOG_ALLBRANCH,BOTTOM_LEFT);
	AddAnchor(IDC_LOG_FOLLOWRENAMES, BOTTOM_LEFT);
	AddAnchor(IDC_LOG_FIRSTPARENT, BOTTOM_LEFT);
	AddAnchor(IDC_SHOWWHOLEPROJECT, BOTTOM_LEFT);
	AddAnchor(IDC_REFRESH, BOTTOM_LEFT);
	AddAnchor(IDC_STATBUTTON, BOTTOM_RIGHT);
	AddAnchor(IDC_PROGRESS, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);

	if(this->m_bAllBranch)
		m_LogList.m_ShowMask|=CGit::LOG_INFO_ALL_BRANCH;
	else
		m_LogList.m_ShowMask&=~CGit::LOG_INFO_ALL_BRANCH;

//	SetPromptParentWindow(m_hWnd);

	if (hWndExplorer)
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	EnableSaveRestore(_T("LogDlg"));

	DWORD yPos1 = CRegDWORD(_T("Software\\TortoiseGit\\TortoiseProc\\ResizableState\\LogDlgSizer1"));
	DWORD yPos2 = CRegDWORD(_T("Software\\TortoiseGit\\TortoiseProc\\ResizableState\\LogDlgSizer2"));
	RECT rcDlg, rcLogList, rcChgMsg;
	GetClientRect(&rcDlg);
	m_LogList.GetWindowRect(&rcLogList);
	ScreenToClient(&rcLogList);
	m_ChangedFileListCtrl.GetWindowRect(&rcChgMsg);
	ScreenToClient(&rcChgMsg);
	if (yPos1)
	{
		RECT rectSplitter;
		m_wndSplitter1.GetWindowRect(&rectSplitter);
		ScreenToClient(&rectSplitter);
		int delta = yPos1 - rectSplitter.top;

		if ((rcLogList.bottom + delta > rcLogList.top)&&(rcLogList.bottom + delta < rcChgMsg.bottom - 30))
		{
			m_wndSplitter1.SetWindowPos(NULL, 0, yPos1, 0, 0, SWP_NOSIZE);
			DoSizeV1(delta);
		}
	}
	if (yPos2)
	{
		RECT rectSplitter;
		m_wndSplitter2.GetWindowRect(&rectSplitter);
		ScreenToClient(&rectSplitter);
		int delta = yPos2 - rectSplitter.top;

		if ((rcChgMsg.top + delta < rcChgMsg.bottom)&&(rcChgMsg.top + delta > rcLogList.top + 30))
		{
			m_wndSplitter2.SetWindowPos(NULL, 0, yPos2, 0, 0, SWP_NOSIZE);
			DoSizeV2(delta);
		}
	}


	if (m_bSelect)
	{
		// the dialog is used to select revisions
		// enable the OK button if appropriate
		EnableOKButton();
	}
	else
	{
		// the dialog is used to just view log messages
		// hide the OK button and set text on Cancel button to OK
		GetDlgItemText(IDOK, temp);
		SetDlgItemText(IDCANCEL, temp);
		GetDlgItem(IDOK)->ShowWindow(SW_HIDE);
	}

	// first start a thread to obtain the log messages without
	// blocking the dialog
	//m_tTo = 0;
	//m_tFrom = (DWORD)-1;

	// scroll to user selected or current revision
	if (!m_hightlightRevision.IsEmpty() && m_hightlightRevision.GetLength() >= GIT_HASH_SIZE)
		m_LogList.m_lastSelectedHash = m_hightlightRevision;
	else if (!m_LogList.m_endrev.IsEmpty() && m_LogList.m_endrev.GetLength() >= GIT_HASH_SIZE)
		m_LogList.m_lastSelectedHash = m_hightlightRevision;
	else
	{
		try
		{
			m_LogList.m_lastSelectedHash = g_Git.GetHash(_T("HEAD"));
		}
		catch (char* msg)
		{
			CString err(msg);
			MessageBox(_T("Could not get HEAD hash.\nlibgit reports:\n") + err, _T("TortoiseGit"), MB_ICONERROR);
		}
	}

	if (m_path.IsEmpty() || m_path.IsDirectory())
		DialogEnableWindow(IDC_LOG_FOLLOWRENAMES, FALSE);

	m_LogList.FetchLogAsync(this);

	GetDlgItem(IDC_LOGLIST)->SetFocus();

	ShowStartRef();
	return FALSE;
}

LRESULT CLogDlg::OnLogListLoading(WPARAM wParam, LPARAM /*lParam*/)
{
	int cur=(int)wParam;

	if( cur == GITLOG_START )
	{
		CString temp;
		temp.LoadString(IDS_PROGRESSWAIT);

		this->m_LogList.ShowText(temp, true);

		// We use a progress bar while getting the logs
		m_LogProgress.SetRange32(0, 100);
		m_LogProgress.SetPos(0);
		if (m_pTaskbarList)
		{
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NORMAL);
			m_pTaskbarList->SetProgressValue(m_hWnd, 0, 100);
		}

		GetDlgItem(IDC_PROGRESS)->ShowWindow(TRUE);

		//DialogEnableWindow(IDC_SHOWWHOLEPROJECT, FALSE);
		//DialogEnableWindow(IDC_LOG_FIRSTPARENT, FALSE);
		DialogEnableWindow(IDC_STATBUTTON, FALSE);
		//DialogEnableWindow(IDC_REFRESH, FALSE);
		DialogEnableWindow(IDC_HIDEPATHS,FALSE);

	}
	else if( cur == GITLOG_END)
	{
		if(this->m_LogList.HasText())
		{
			this->m_LogList.ClearText();
		}
		UpdateLogInfoLabel();

		if (m_pTaskbarList)
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NOPROGRESS);

		DialogEnableWindow(IDC_SHOWWHOLEPROJECT, !m_bFollowRenames);

		DialogEnableWindow(IDC_STATBUTTON, !(m_LogList.m_arShownList.IsEmpty() || m_LogList.m_arShownList.GetCount() == 1 && m_LogList.m_bShowWC));
		DialogEnableWindow(IDC_REFRESH, TRUE);
		DialogEnableWindow(IDC_HIDEPATHS,TRUE);

//		PostMessage(WM_TIMER, LOGFILTER_TIMER);
		GetDlgItem(IDC_PROGRESS)->ShowWindow(FALSE);
		//CTime time=m_LogList.GetOldestTime();
		CTime begin,end;
		m_LogList.GetTimeRange(begin,end);

		if(m_LogList.m_From == -1)
			m_DateFrom.SetTime(&begin);

		if(m_LogList.m_To == -1)
			m_DateTo.SetTime(&end);


	}
	else
	{
		if(this->m_LogList.HasText())
		{
			this->m_LogList.ClearText();
			this->m_LogList.Invalidate();
		}
		UpdateLogInfoLabel();
		m_LogProgress.SetPos(cur);
		if (m_pTaskbarList)
		{
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NORMAL);
			m_pTaskbarList->SetProgressValue(m_hWnd, cur, 100);
		}
	}
	return 0;
}
void CLogDlg::SetDlgTitle()
{
	if (m_sTitle.IsEmpty())
		GetWindowText(m_sTitle);

	if (m_LogList.m_Path.IsEmpty() || m_orgPath.GetWinPathString().IsEmpty())
	{
		CAppUtils::SetWindowTitle(m_hWnd, g_Git.m_CurrentDir, m_sTitle);
	}
	else
		CAppUtils::SetWindowTitle(m_hWnd, m_orgPath.GetWinPathString(), m_sTitle);
}

void CLogDlg::CheckRegexpTooltip()
{
	CWnd *pWnd = GetDlgItem(IDC_SEARCHEDIT);
	// Since tooltip describes regexp features, show it only if regexps are enabled.
	if (m_bFilterWithRegex)
	{
		m_tooltips.AddTool(pWnd, IDS_LOG_FILTER_REGEX_TT);
	}
	else
		m_tooltips.DelTool(pWnd);
}

void CLogDlg::EnableOKButton()
{
	if (m_bSelect)
	{
		// the dialog is used to select revisions
		if (m_bSelectionMustBeSingle)
		{
			// enable OK button if only a single revision is selected
			DialogEnableWindow(IDOK, (m_LogList.GetSelectedCount()==1));
		}
		else if (m_bSelectionMustBeContinuous)
			DialogEnableWindow(IDOK, (m_LogList.GetSelectedCount()!=0)&&(m_LogList.IsSelectionContinuous()));
		else
			DialogEnableWindow(IDOK, m_LogList.GetSelectedCount()!=0);
	}
	else
		DialogEnableWindow(IDOK, TRUE);
}

CString CLogDlg::GetTagInfo(GitRev* pLogEntry)
{
	CString cmd;
	CString output;

	if(m_LogList.m_HashMap.find(pLogEntry->m_CommitHash) != m_LogList.m_HashMap.end())
	{
		STRING_VECTOR &vector = m_LogList.m_HashMap[pLogEntry->m_CommitHash];
		for(int i=0;i<vector.size();i++)
		{
			if(vector[i].Find(_T("refs/tags/")) == 0 )
			{
				CString tag= vector[i];
				int start = vector[i].Find(_T("^{}"));
				if(start>0)
					tag=tag.Left(start);
				else
					continue;

				cmd.Format(_T("git.exe cat-file	tag %s"), tag);

				if(g_Git.Run(cmd, &output, NULL, CP_UTF8) == 0 )
					output+=_T("\n");
			}
		}
	}

	if(!output.IsEmpty())
	{
		output = _T("\n*") + CString(MAKEINTRESOURCE(IDS_PROC_LOG_TAGINFO)) + _T("*\n\n") + output;
	}

	return output;
}

void CLogDlg::FillLogMessageCtrl(bool bShow /* = true*/)
{
	// we fill here the log message rich edit control,
	// and also populate the changed files list control
	// according to the selected revision(s).

	CRichEditCtrl * pMsgView = (CRichEditCtrl*)GetDlgItem(IDC_MSGVIEW);
	// empty the log message view
	pMsgView->SetWindowText(_T(" "));
	// empty the changed files list
	m_ChangedFileListCtrl.SetRedraw(FALSE);
//	InterlockedExchange(&m_bNoDispUpdates, TRUE);
	m_currentChangedArray = NULL;
	//m_ChangedFileListCtrl.SetExtendedStyle ( LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER );
	m_ChangedFileListCtrl.DeleteAllItems();

	// if we're not here to really show a selected revision, just
	// get out of here after clearing the views, which is what is intended
	// if that flag is not set.
	if (!bShow)
	{
		// force a redraw
		m_ChangedFileListCtrl.Invalidate();
//		InterlockedExchange(&m_bNoDispUpdates, FALSE);
		m_ChangedFileListCtrl.SetRedraw(TRUE);
		return;
	}

	// depending on how many revisions are selected, we have to do different
	// tasks.
	int selCount = m_LogList.GetSelectedCount();
	if (selCount == 0)
	{
		// if nothing is selected, we have nothing more to do
//		InterlockedExchange(&m_bNoDispUpdates, FALSE);
		m_ChangedFileListCtrl.SetRedraw(TRUE);
		return;
	}
	else if (selCount == 1)
	{
		// if one revision is selected, we have to fill the log message view
		// with the corresponding log message, and also fill the changed files
		// list fully.
		POSITION pos = m_LogList.GetFirstSelectedItemPosition();
		int selIndex = m_LogList.GetNextSelectedItem(pos);
		if (selIndex >= m_LogList.m_arShownList.GetCount())
		{
//			InterlockedExchange(&m_bNoDispUpdates, FALSE);
			m_ChangedFileListCtrl.SetRedraw(TRUE);
			return;
		}
		GitRev* pLogEntry = reinterpret_cast<GitRev *>(m_LogList.m_arShownList.SafeGetAt(selIndex));

		{
			// set the log message text
			pMsgView->SetWindowText(CString(MAKEINTRESOURCE(IDS_HASH)) + _T(": ") + pLogEntry->m_CommitHash.ToString() + _T("\r\n\r\n"));
			// turn bug ID's into links if the bugtraq: properties have been set
			// and we can find a match of those in the log message

			pMsgView->SetSel(-1,-1);
			CHARFORMAT2 format;
			SecureZeroMemory(&format, sizeof(CHARFORMAT2));
			format.cbSize = sizeof(CHARFORMAT2);
			format.dwMask = CFM_BOLD;
			format.dwEffects = CFE_BOLD;
			pMsgView->SendMessage(EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&format);

			CString msg=_T("* ");
			msg+=pLogEntry->GetSubject();
			pMsgView->ReplaceSel(msg);

			pMsgView->SetSel(-1,-1);
			format.dwEffects = 0;
			pMsgView->SendMessage(EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&format);

			msg=_T("\n");
			msg+=pLogEntry->GetBody();

			if(!pLogEntry->m_Notes.IsEmpty())
			{
				msg+= _T("\n*") + CString(MAKEINTRESOURCE(IDS_NOTES)) + _T("* ");
				msg+= pLogEntry->m_Notes;
				msg+= _T("\n\n");
			}

			msg+=GetTagInfo(pLogEntry);

			pMsgView->ReplaceSel(msg);

			CString text;
			pMsgView->GetWindowText(text);
			// the rich edit control doesn't count the CR char!
			// to be exact: CRLF is treated as one char.
			text.Remove('\r');

			m_LogList.m_ProjectProperties.FindBugID(text, pMsgView);
			CAppUtils::FormatTextInRichEditControl(pMsgView);

			int HidePaths=m_cHidePaths.GetState() & 0x0003;
			CString matchpath=this->m_path.GetGitPathString();

			int count = pLogEntry->GetFiles(&m_LogList).GetCount();
			for(int i=0;i<count && (!matchpath.IsEmpty());i++)
			{
				if( m_bWholeProject )
					break;

				((CTGitPath&)pLogEntry->GetFiles(&m_LogList)[i]).m_Action &= ~(CTGitPath::LOGACTIONS_HIDE|CTGitPath::LOGACTIONS_GRAY);

				if(pLogEntry->GetFiles(&m_LogList)[i].GetGitPathString().Left(matchpath.GetLength()) != matchpath && pLogEntry->GetFiles(&m_LogList)[i].GetGitOldPathString().Left(matchpath.GetLength()) != matchpath)
				{
					if(HidePaths==BST_CHECKED)
						((CTGitPath&)pLogEntry->GetFiles(&m_LogList)[i]).m_Action |= CTGitPath::LOGACTIONS_HIDE;
					if(HidePaths==BST_INDETERMINATE)
						((CTGitPath&)pLogEntry->GetFiles(&m_LogList)[i]).m_Action |= CTGitPath::LOGACTIONS_GRAY;
				}
			}

			m_ChangedFileListCtrl.UpdateWithGitPathList(pLogEntry->GetFiles(&m_LogList));
			m_ChangedFileListCtrl.m_CurrentVersion=pLogEntry->m_CommitHash;
			m_ChangedFileListCtrl.Show(GITSLC_SHOWVERSIONED);

			m_ChangedFileListCtrl.SetBusyString(CString(MAKEINTRESOURCE(IDS_PROC_LOG_FETCHINGFILES)));

			if(!pLogEntry->m_IsDiffFiles)
				m_ChangedFileListCtrl.SetBusy(TRUE);
			else
				m_ChangedFileListCtrl.SetBusy(FALSE);

			m_ChangedFileListCtrl.SetRedraw(TRUE);
			return;
		}

	}
	else
	{
		// more than one revision is selected:
		// the log message view must be emptied
		// the changed files list contains all the changed paths from all
		// selected revisions, with 'doubles' removed
		m_currentChangedPathList = GetChangedPathsFromSelectedRevisions(true);
	}

	// redraw the views
//	InterlockedExchange(&m_bNoDispUpdates, FALSE);
#if 0
	if (m_currentChangedArray)
	{
		m_ChangedFileListCtrl.SetItemCountEx(m_currentChangedArray->GetCount());
		m_ChangedFileListCtrl.RedrawItems(0, m_currentChangedArray->GetCount());
	}
	else if (m_currentChangedPathList.GetCount())
	{
		m_ChangedFileListCtrl.SetItemCountEx(m_currentChangedPathList.GetCount());
		m_ChangedFileListCtrl.RedrawItems(0, m_currentChangedPathList.GetCount());
	}
	else
	{
		m_ChangedFileListCtrl.SetItemCountEx(0);
		m_ChangedFileListCtrl.Invalidate();
	}
#endif
	// sort according to the settings
	if (m_nSortColumnPathList > 0)
		SetSortArrow(&m_ChangedFileListCtrl, m_nSortColumnPathList, m_bAscendingPathList);
	else
		SetSortArrow(&m_ChangedFileListCtrl, -1, false);
	m_ChangedFileListCtrl.SetRedraw(TRUE);

}

void CLogDlg::OnBnClickedRefresh()
{
	Refresh (true);
}

void CLogDlg::Refresh (bool clearfilter /*autoGoOnline*/)
{
	m_limit = 0;
	m_LogList.Refresh(clearfilter);
	ShowStartRef();
	FillLogMessageCtrl(false);
}



BOOL CLogDlg::Cancel()
{
	return m_bCancelled;
}

void CLogDlg::SaveSplitterPos()
{
	if (!IsIconic())
	{
		CRegDWORD regPos1 = CRegDWORD(_T("Software\\TortoiseGit\\TortoiseProc\\ResizableState\\LogDlgSizer1"));
		CRegDWORD regPos2 = CRegDWORD(_T("Software\\TortoiseGit\\TortoiseProc\\ResizableState\\LogDlgSizer2"));
		RECT rectSplitter;
		m_wndSplitter1.GetWindowRect(&rectSplitter);
		ScreenToClient(&rectSplitter);
		regPos1 = rectSplitter.top;
		m_wndSplitter2.GetWindowRect(&rectSplitter);
		ScreenToClient(&rectSplitter);
		regPos2 = rectSplitter.top;
	}
}

void CLogDlg::OnCancel()
{
	this->ShowWindow(SW_HIDE);

	// canceling means stopping the working thread if it's still running.
	m_LogList.SafeTerminateAsyncDiffThread();
	if (this->IsThreadRunning())
	{
		m_LogList.SafeTerminateThread();
	}
	UpdateData();

	SaveSplitterPos();
	__super::OnCancel();
}

void CLogDlg::CopyChangedSelectionToClipBoard()
{

	POSITION pos = m_LogList.GetFirstSelectedItemPosition();
	if (pos == NULL)
		return;	// nothing is selected, get out of here

	CString sPaths;

//	CGitRev* pLogEntry = reinterpret_cast<CGitRev* >(m_LogList.m_arShownList.SafeGetAt(m_LogList.GetNextSelectedItem(pos)));
//	if (pos)
	{
		POSITION pos = m_ChangedFileListCtrl.GetFirstSelectedItemPosition();
		while (pos)
		{
			int nItem = m_ChangedFileListCtrl.GetNextSelectedItem(pos);
			CTGitPath *path = (CTGitPath*)m_ChangedFileListCtrl.GetItemData(nItem);
			if(path)
				sPaths += path->GetGitPathString();
			sPaths += _T("\r\n");
		}
	}
#if 0
	else
	{
		// only one revision is selected in the log dialog top pane
		// but multiple items could be selected  in the changed items list
		POSITION pos = m_ChangedFileListCtrl.GetFirstSelectedItemPosition();
		while (pos)
		{
			int nItem = m_ChangedFileListCtrl.GetNextSelectedItem(pos);
			LogChangedPath * changedlogpath = pLogEntry->pArChangedPaths->SafeGetAt(nItem);

			if ((m_cHidePaths.GetState() & 0x0003)==BST_CHECKED)
			{
				// some items are hidden! So find out which item the user really selected
				INT_PTR selRealIndex = -1;
				for (INT_PTR hiddenindex=0; hiddenindex<pLogEntry->pArChangedPaths->GetCount(); ++hiddenindex)
				{
					if (pLogEntry->pArChangedPaths->SafeGetAt(hiddenindex)->sPath.Left(m_sRelativeRoot.GetLength()).Compare(m_sRelativeRoot)==0)
						selRealIndex++;
					if (selRealIndex == nItem)
					{
						changedlogpath = pLogEntry->pArChangedPaths->SafeGetAt(hiddenindex);
						break;
					}
				}
			}
			if (changedlogpath)
			{
				sPaths += changedlogpath->sPath;
				sPaths += _T("\r\n");
			}
		}
	}
#endif
	sPaths.Trim();
	CStringUtils::WriteAsciiStringToClipboard(sPaths, GetSafeHwnd());

}

void CLogDlg::OnContextMenu(CWnd* pWnd, CPoint point)
{
	// we have two separate context menus:
	// one shown on the log message list control,
	// the other shown in the changed-files list control
	int selCount = m_LogList.GetSelectedCount();
	if ((selCount == 1)&&(pWnd == GetDlgItem(IDC_MSGVIEW)))
	{
		POSITION pos = m_LogList.GetFirstSelectedItemPosition();
		int selIndex = -1;
		if (pos)
			selIndex = m_LogList.GetNextSelectedItem(pos);

		GitRev *pRev = ((GitRev*)m_LogList.m_arShownList[selIndex]);

		if ((point.x == -1) && (point.y == -1))
		{
			CRect rect;
			GetDlgItem(IDC_MSGVIEW)->GetClientRect(&rect);
			ClientToScreen(&rect);
			point = rect.CenterPoint();
		}
		CString sMenuItemText;
		CIconMenu popup;
		if (popup.CreatePopupMenu())
		{
			// add the 'default' entries
			sMenuItemText.LoadString(IDS_SCIEDIT_COPY);
			popup.AppendMenu(MF_STRING | MF_ENABLED, WM_COPY, sMenuItemText);
			sMenuItemText.LoadString(IDS_SCIEDIT_SELECTALL);
			popup.AppendMenu(MF_STRING | MF_ENABLED, EM_SETSEL, sMenuItemText);
			sMenuItemText.LoadString(IDS_EDIT_NOTES);
			popup.AppendMenuIcon( CGitLogList::ID_EDITNOTE, sMenuItemText, IDI_EDIT);

			int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
			switch (cmd)
			{
			case 0:
				break;	// no command selected
			case EM_SETSEL:
			case WM_COPY:
				::SendMessage(GetDlgItem(IDC_MSGVIEW)->GetSafeHwnd(), cmd, 0, -1);
				break;
			case CGitLogList::ID_EDITNOTE:
				CAppUtils::EditNote(pRev);
				this->FillLogMessageCtrl(true);
				break;
			}
		}
	}
}

void CLogDlg::OnOK()
{
	// since the log dialog is also used to select revisions for other
	// dialogs, we have to do some work before closing this dialog
	if (GetFocus() != GetDlgItem(IDOK))
		return;	// if the "OK" button doesn't have the focus, do nothing: this prevents closing the dialog when pressing enter

	m_LogList.SafeTerminateAsyncDiffThread();
	if (this->IsThreadRunning())
	{
		m_LogList.SafeTerminateThread();
	}
	UpdateData();
	// check that one and only one row is selected
	if (m_LogList.GetSelectedCount() == 1)
	{
		// get the selected row
		POSITION pos = m_LogList.GetFirstSelectedItemPosition();
		int selIndex = m_LogList.GetNextSelectedItem(pos);
		if (selIndex < m_LogList.m_arShownList.GetCount())
		{
			// all ok, pick up the revision
			GitRev* pLogEntry = reinterpret_cast<GitRev *>(m_LogList.m_arShownList.SafeGetAt(selIndex));
			// extract the hash
			m_sSelectedHash = pLogEntry->m_CommitHash;
		}
	}
	UpdateData(FALSE);
	SaveSplitterPos();
	__super::OnOK();

	#if 0
	if (!GetDlgItem(IDOK)->IsWindowVisible() && GetFocus() != GetDlgItem(IDCANCEL))
		return; // the Cancel button works as the OK button. But if the cancel button has not the focus, do nothing.

	CString temp;
	CString buttontext;
	GetDlgItemText(IDOK, buttontext);
	temp.LoadString(IDS_MSGBOX_CANCEL);
	if (temp.Compare(buttontext) != 0)
		__super::OnOK();	// only exit if the button text matches, and that will match only if the thread isn't running anymore
	m_bCancelled = TRUE;
	m_selectedRevs.Clear();
	m_selectedRevsOneRange.Clear();
	if (m_pNotifyWindow)
	{
		int selIndex = m_LogList.GetSelectionMark();
		if (selIndex >= 0)
		{
			PLOGENTRYDATA pLogEntry = NULL;
			POSITION pos = m_LogList.GetFirstSelectedItemPosition();
			pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.SafeGetAt(m_LogList.GetNextSelectedItem(pos)));
			m_selectedRevs.AddRevision(pLogEntry->Rev);
			git_revnum_t lowerRev = pLogEntry->Rev;
			git_revnum_t higherRev = lowerRev;
			while (pos)
			{
				pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.SafeGetAt(m_LogList.GetNextSelectedItem(pos)));
				git_revnum_t rev = pLogEntry->Rev;
				m_selectedRevs.AddRevision(pLogEntry->Rev);
				if (lowerRev > rev)
					lowerRev = rev;
				if (higherRev < rev)
					higherRev = rev;
			}
			if (m_sFilterText.IsEmpty() && m_nSortColumn == 0 && IsSelectionContinuous())
			{
				m_selectedRevsOneRange.AddRevRange(lowerRev, higherRev);
			}
			BOOL bSentMessage = FALSE;
			if (m_LogList.GetSelectedCount() == 1)
			{
				// if only one revision is selected, check if the path/url with which the dialog was started
				// was directly affected in that revision. If it was, then check if our path was copied from somewhere.
				// if it was copied, use the copy from revision as lowerRev
				if ((pLogEntry)&&(pLogEntry->pArChangedPaths)&&(lowerRev == higherRev))
				{
					CString sUrl = m_path.GetGitPathString();
					if (!m_path.IsUrl())
					{
						sUrl = GetURLFromPath(m_path);
					}
					sUrl = sUrl.Mid(m_sRepositoryRoot.GetLength());
					for (int cp = 0; cp < pLogEntry->pArChangedPaths->GetCount(); ++cp)
					{
						LogChangedPath * pData = pLogEntry->pArChangedPaths->SafeGetAt(cp);
						if (pData)
						{
							if (sUrl.Compare(pData->sPath) == 0)
							{
								if (!pData->sCopyFromPath.IsEmpty())
								{
									lowerRev = pData->lCopyFromRev;
									m_pNotifyWindow->SendMessage(WM_REVSELECTED, m_wParam & (MERGE_REVSELECTSTART), lowerRev);
									m_pNotifyWindow->SendMessage(WM_REVSELECTED, m_wParam & (MERGE_REVSELECTEND), higherRev);
									m_pNotifyWindow->SendMessage(WM_REVLIST, m_selectedRevs.GetCount(), (LPARAM)&m_selectedRevs);
									bSentMessage = TRUE;
								}
							}
						}
					}
				}
			}
			if ( !bSentMessage )
			{
				m_pNotifyWindow->SendMessage(WM_REVSELECTED, m_wParam & (MERGE_REVSELECTSTART | MERGE_REVSELECTMINUSONE), lowerRev);
				m_pNotifyWindow->SendMessage(WM_REVSELECTED, m_wParam & (MERGE_REVSELECTEND | MERGE_REVSELECTMINUSONE), higherRev);
				m_pNotifyWindow->SendMessage(WM_REVLIST, m_selectedRevs.GetCount(), (LPARAM)&m_selectedRevs);
				if (m_selectedRevsOneRange.GetCount())
					m_pNotifyWindow->SendMessage(WM_REVLISTONERANGE, 0, (LPARAM)&m_selectedRevsOneRange);
			}
		}
	}
	UpdateData();
	CRegDWORD reg = CRegDWORD(_T("Software\\TortoiseGit\\ShowAllEntry"));
	SaveSplitterPos();
#endif
}

void CLogDlg::DoDiffFromLog(INT_PTR selIndex, GitRev* rev1, GitRev* rev2, bool /*blame*/, bool /*unified*/)
{
	DialogEnableWindow(IDOK, FALSE);
//	SetPromptApp(&theApp);
	theApp.DoWaitCursor(1);

	CString temppath;
	GetTempPath(temppath);

	CString file1;
	file1.Format(_T("%s%s_%s%s"),
				temppath,
				(*m_currentChangedArray)[selIndex].GetBaseFilename(),
				rev1->m_CommitHash.ToString().Left(g_Git.GetShortHASHLength()),
				(*m_currentChangedArray)[selIndex].GetFileExtension());

	CString file2;
	file2.Format(_T("%s\\%s_%s%s"),
				temppath,
				(*m_currentChangedArray)[selIndex].GetBaseFilename(),
				rev2->m_CommitHash.ToString().Left(g_Git.GetShortHASHLength()),
				(*m_currentChangedArray)[selIndex].GetFileExtension());

	CString cmd;

	g_Git.GetOneFile(rev1->m_CommitHash.ToString(), (CTGitPath &)(*m_currentChangedArray)[selIndex],file1);

	g_Git.GetOneFile(rev2->m_CommitHash.ToString(), (CTGitPath &)(*m_currentChangedArray)[selIndex],file2);

	CAppUtils::DiffFlags flags;
	CAppUtils::StartExtDiff(file1,file2,_T("A"),_T("B"),flags);

	theApp.DoWaitCursor(-1);
	EnableOKButton();
}

BOOL CLogDlg::PreTranslateMessage(MSG* pMsg)
{
	// Skip Ctrl-C when copying text out of the log message or search filter
	bool bSkipAccelerator = (pMsg->message == WM_KEYDOWN && (pMsg->wParam == 'C' || pMsg->wParam == VK_INSERT) && (GetFocus() == GetDlgItem(IDC_MSGVIEW) || GetFocus() == GetDlgItem(IDC_SEARCHEDIT)) && GetKeyState(VK_CONTROL) & 0x8000);
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam=='\r')
	{
		if (GetFocus()==GetDlgItem(IDC_LOGLIST))
		{
			if (CRegDWORD(_T("Software\\TortoiseGit\\DiffByDoubleClickInLog"), FALSE))
			{
				m_LogList.DiffSelectedRevWithPrevious();
				return TRUE;
			}
		}
	}
	if (m_hAccel && !bSkipAccelerator)
	{
		int ret = TranslateAccelerator(m_hWnd, m_hAccel, pMsg);
		if (ret)
			return TRUE;
	}

	if(::IsWindow(m_tooltips.m_hWnd))
		m_tooltips.RelayEvent(pMsg);
	return __super::PreTranslateMessage(pMsg);
}


BOOL CLogDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	//if (this->IsThreadRunning())
	if(m_LogList.m_bNoDispUpdates)
	{
		// only show the wait cursor over the list control
		if ((pWnd)&&
			((pWnd == GetDlgItem(IDC_LOGLIST))||
			(pWnd == GetDlgItem(IDC_MSGVIEW))||
			(pWnd == GetDlgItem(IDC_LOGMSG))))
		{
			HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_WAIT));
			SetCursor(hCur);
			return TRUE;
		}
	}
	if ((pWnd) && (pWnd == GetDlgItem(IDC_MSGVIEW)))
		return CResizableStandAloneDialog::OnSetCursor(pWnd, nHitTest, message);

	HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
	SetCursor(hCur);
	return CResizableStandAloneDialog::OnSetCursor(pWnd, nHitTest, message);
}

void CLogDlg::OnBnClickedHelp()
{
	OnHelp();
}

void CLogDlg::OnLvnItemchangedLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;
	//if (this->IsThreadRunning())
	if(m_LogList.m_bNoDispUpdates)
		return;
	if (pNMLV->iItem >= 0)
	{
		this->m_LogList.m_nSearchIndex = pNMLV->iItem;
		GitRev* pLogEntry = reinterpret_cast<GitRev *>(m_LogList.m_arShownList.SafeGetAt(pNMLV->iItem));
		m_LogList.m_lastSelectedHash = pLogEntry->m_CommitHash;
		if (pNMLV->iSubItem != 0)
			return;
		if ((pNMLV->iItem == m_LogList.m_arShownList.GetCount()))
		{
			// remove the selected state
			if (pNMLV->uChanged & LVIF_STATE)
			{
				m_LogList.SetItemState(pNMLV->iItem, 0, LVIS_SELECTED);
				FillLogMessageCtrl();
				UpdateData(FALSE);
				UpdateLogInfoLabel();
			}
			return;
		}
		if (pNMLV->uChanged & LVIF_STATE)
		{
			FillLogMessageCtrl();
			UpdateData(FALSE);
		}
	}
	else
	{
		m_LogList.m_lastSelectedHash.Empty();
		FillLogMessageCtrl();
		UpdateData(FALSE);
	}
	EnableOKButton();
	UpdateLogInfoLabel();
}

void CLogDlg::OnEnLinkMsgview(NMHDR *pNMHDR, LRESULT *pResult)
{
	ENLINK *pEnLink = reinterpret_cast<ENLINK *>(pNMHDR);
	if (pEnLink->msg == WM_LBUTTONUP)
	{
		CString url, msg;
		GetDlgItemText(IDC_MSGVIEW, msg);
		msg.Replace(_T("\r\n"), _T("\n"));
		url = msg.Mid(pEnLink->chrg.cpMin, pEnLink->chrg.cpMax-pEnLink->chrg.cpMin);
		if (!::PathIsURL(url))
		{
			url = m_LogList.m_ProjectProperties.GetBugIDUrl(url);
			url = GetAbsoluteUrlFromRelativeUrl(url);
		}
		if (!url.IsEmpty())
			ShellExecute(this->m_hWnd, _T("open"), url, NULL, NULL, SW_SHOWDEFAULT);
	}
	*pResult = 0;
}

class CDateSorter
{
public:
	class CCommitPointer
	{
	public:
		CCommitPointer():m_cont(NULL){}
		CCommitPointer(const CCommitPointer& P_Right)
		: m_cont(NULL)
		{
			*this = P_Right;
		}

		CCommitPointer& operator = (const CCommitPointer& P_Right)
		{
			if(IsPointer())
			{
				(*m_cont->m_parDates)[m_place]			= P_Right.GetDate();
				(*m_cont->m_parFileChanges)[m_place]	= P_Right.GetChanges();
				(*m_cont->m_parAuthors)[m_place]		= P_Right.GetAuthor();
			}
			else
			{
				m_Date								= P_Right.GetDate();
				m_Changes							= P_Right.GetChanges();
				m_csAuthor							= P_Right.GetAuthor();
			}
			return *this;
		}

		void Clone(const CCommitPointer& P_Right)
		{
			m_cont = P_Right.m_cont;
			m_place = P_Right.m_place;
			m_Date = P_Right.m_Date;
			m_Changes = P_Right.m_Changes;
			m_csAuthor = P_Right.m_csAuthor;
		}

		DWORD		 GetDate()		const {return IsPointer() ? (*m_cont->m_parDates)[m_place] : m_Date;}
		DWORD		 GetChanges()	const {return IsPointer() ? (*m_cont->m_parFileChanges)[m_place] : m_Changes;}
		CString		 GetAuthor()	const {return IsPointer() ? (*m_cont->m_parAuthors)[m_place] : m_csAuthor;}

		bool		IsPointer() const {return m_cont != NULL;}
		//When pointer
		CDateSorter* m_cont;
		int			 m_place;

		//When element
		DWORD		 m_Date;
		DWORD		 m_Changes;
		CString		 m_csAuthor;

	};
	class iterator : public std::iterator<std::random_access_iterator_tag, CCommitPointer>
	{
	public:
		CCommitPointer m_ptr;

		iterator(){}
		iterator(const iterator& P_Right){*this = P_Right;}
		iterator& operator=(const iterator& P_Right)
		{
			m_ptr.Clone(P_Right.m_ptr);
			return *this;
		}

		CCommitPointer& operator*(){return m_ptr;}
		CCommitPointer* operator->(){return &m_ptr;}
		const CCommitPointer& operator*()const{return m_ptr;}
		const CCommitPointer* operator->()const{return &m_ptr;}

		iterator& operator+=(size_t P_iOffset){m_ptr.m_place += (int)P_iOffset;return *this;}
		iterator& operator-=(size_t P_iOffset){m_ptr.m_place -= (int)P_iOffset;return *this;}
		iterator operator+(size_t P_iOffset)const{iterator it(*this); it += P_iOffset;return it;}
		iterator operator-(size_t P_iOffset)const{iterator it(*this); it -= P_iOffset;return it;}

		iterator& operator++(){++m_ptr.m_place;return *this;}
		iterator& operator--(){--m_ptr.m_place;return *this;}
		iterator operator++(int){iterator it(*this);++*this;return it;}
		iterator operator--(int){iterator it(*this);--*this;return it;}

		size_t operator-(const iterator& P_itRight)const{return m_ptr.m_place - P_itRight->m_place;}

		bool operator<(const iterator& P_itRight)const{return m_ptr.m_place < P_itRight->m_place;}
		bool operator!=(const iterator& P_itRight)const{return m_ptr.m_place != P_itRight->m_place;}
		bool operator==(const iterator& P_itRight)const{return m_ptr.m_place == P_itRight->m_place;}
		bool operator>(const iterator& P_itRight)const{return m_ptr.m_place > P_itRight->m_place;}
	};
	iterator begin()
	{
		iterator it;
		it->m_place = 0;
		it->m_cont = this;
		return it;
	}
	iterator end()
	{
		iterator it;
		it->m_place = (int)m_parDates->GetCount();
		it->m_cont = this;
		return it;
	}

	CDWordArray	*	m_parDates;
	CDWordArray	*	m_parFileChanges;
	CStringArray *	m_parAuthors;
};

class CDateSorterLess
{
public:
	bool operator () (const CDateSorter::CCommitPointer& P_Left, const CDateSorter::CCommitPointer& P_Right) const
	{
		return P_Left.GetDate() > P_Right.GetDate(); //Last date first
	}

};

void CLogDlg::OnBnClickedStatbutton()
{
	if (this->IsThreadRunning())
		return;
	if (m_LogList.m_arShownList.IsEmpty() || m_LogList.m_arShownList.GetCount() == 1 && m_LogList.m_bShowWC)
		return;		// nothing or just the working copy changes are shown, so no statistics.
	// the statistics dialog expects the log entries to be sorted by date
	SortByColumn(3, false);
	CThreadSafePtrArray shownlist(NULL);
	m_LogList.RecalculateShownList(&shownlist);
	// create arrays which are aware of the current filter
	CStringArray m_arAuthorsFiltered;
	CDWordArray m_arDatesFiltered;
	CDWordArray m_arFileChangesFiltered;
	for (INT_PTR i=0; i<shownlist.GetCount(); ++i)
	{
		GitRev* pLogEntry = reinterpret_cast<GitRev*>(shownlist.SafeGetAt(i));

		// do not take working dir changes into statistics
		if (pLogEntry->m_CommitHash.IsEmpty()) {
			continue;
		}

		CString strAuthor = pLogEntry->GetAuthorName();
		if ( strAuthor.IsEmpty() )
		{
			strAuthor.LoadString(IDS_STATGRAPH_EMPTYAUTHOR);
		}
		m_arAuthorsFiltered.Add(strAuthor);
		m_arDatesFiltered.Add(pLogEntry->GetCommitterDate().GetTime());
		m_arFileChangesFiltered.Add(pLogEntry->GetFiles(&m_LogList).GetCount());
	}

	CDateSorter W_Sorter;
	W_Sorter.m_parAuthors		= &m_arAuthorsFiltered;
	W_Sorter.m_parDates			= &m_arDatesFiltered;
	W_Sorter.m_parFileChanges	= &m_arFileChangesFiltered;
	std::sort(W_Sorter.begin(), W_Sorter.end(), CDateSorterLess());

	CStatGraphDlg dlg;
	dlg.m_parAuthors = &m_arAuthorsFiltered;
	dlg.m_parDates = &m_arDatesFiltered;
	dlg.m_parFileChanges = &m_arFileChangesFiltered;
	dlg.m_path = m_orgPath;
	dlg.DoModal();
	// restore the previous sorting
	SortByColumn(m_nSortColumn, m_bAscending);
	OnTimer(LOGFILTER_TIMER);
}

void CLogDlg::DoSizeV1(int delta)
{

	RemoveAnchor(IDC_LOGLIST);
	RemoveAnchor(IDC_SPLITTERTOP);
	RemoveAnchor(IDC_MSGVIEW);
	RemoveAnchor(IDC_SPLITTERBOTTOM);
	RemoveAnchor(IDC_LOGMSG);
	CSplitterControl::ChangeHeight(&m_LogList, delta, CW_TOPALIGN);
	CSplitterControl::ChangeHeight(GetDlgItem(IDC_MSGVIEW), -delta, CW_BOTTOMALIGN);
	AddAnchor(IDC_LOGLIST, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_SPLITTERTOP, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_MSGVIEW, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SPLITTERBOTTOM, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_LOGMSG, BOTTOM_LEFT, BOTTOM_RIGHT);
	ArrangeLayout();
	AdjustMinSize();
	SetSplitterRange();
	m_LogList.Invalidate();
	GetDlgItem(IDC_MSGVIEW)->Invalidate();

}

void CLogDlg::DoSizeV2(int delta)
{

	RemoveAnchor(IDC_LOGLIST);
	RemoveAnchor(IDC_SPLITTERTOP);
	RemoveAnchor(IDC_MSGVIEW);
	RemoveAnchor(IDC_SPLITTERBOTTOM);
	RemoveAnchor(IDC_LOGMSG);
	CSplitterControl::ChangeHeight(GetDlgItem(IDC_MSGVIEW), delta, CW_TOPALIGN);
	CSplitterControl::ChangeHeight(&m_ChangedFileListCtrl, -delta, CW_BOTTOMALIGN);
	AddAnchor(IDC_LOGLIST, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_SPLITTERTOP, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_MSGVIEW, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SPLITTERBOTTOM, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_LOGMSG, BOTTOM_LEFT, BOTTOM_RIGHT);
	ArrangeLayout();
	AdjustMinSize();
	SetSplitterRange();
	GetDlgItem(IDC_MSGVIEW)->Invalidate();
	m_ChangedFileListCtrl.Invalidate();

}

void CLogDlg::AdjustMinSize()
{
	// adjust the minimum size of the dialog to prevent the resizing from
	// moving the list control too far down.
	CRect rcChgListView;
	m_ChangedFileListCtrl.GetClientRect(rcChgListView);
	CRect rcLogList;
	m_LogList.GetClientRect(rcLogList);

	SetMinTrackSize(CSize(m_DlgOrigRect.Width(),
		m_DlgOrigRect.Height()-m_ChgOrigRect.Height()-m_LogListOrigRect.Height()-m_MsgViewOrigRect.Height()
		+rcChgListView.Height()+rcLogList.Height()+60));
}

LRESULT CLogDlg::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_NOTIFY:
		if (wParam == IDC_SPLITTERTOP)
		{
			SPC_NMHDR* pHdr = (SPC_NMHDR*) lParam;
			DoSizeV1(pHdr->delta);
		}
		else if (wParam == IDC_SPLITTERBOTTOM)
		{
			SPC_NMHDR* pHdr = (SPC_NMHDR*) lParam;
			DoSizeV2(pHdr->delta);
		}
		break;
	}

	return CResizableDialog::DefWindowProc(message, wParam, lParam);
}

void CLogDlg::SetSplitterRange()
{
	if ((m_LogList)&&(m_ChangedFileListCtrl))
	{
		CRect rcTop;
		m_LogList.GetWindowRect(rcTop);
		ScreenToClient(rcTop);
		CRect rcMiddle;
		GetDlgItem(IDC_MSGVIEW)->GetWindowRect(rcMiddle);
		ScreenToClient(rcMiddle);
		m_wndSplitter1.SetRange(rcTop.top+30, rcMiddle.bottom-20);
		CRect rcBottom;
		m_ChangedFileListCtrl.GetWindowRect(rcBottom);
		ScreenToClient(rcBottom);
		m_wndSplitter2.SetRange(rcMiddle.top+30, rcBottom.bottom-20);
	}
}

LRESULT CLogDlg::OnClickedInfoIcon(WPARAM /*wParam*/, LPARAM lParam)
{
	// FIXME: x64 version would get this function called with unexpected parameters.
	if (!lParam)
		return 0;

	RECT * rect = (LPRECT)lParam;
	CPoint point;
	CString temp;
	point = CPoint(rect->left, rect->bottom);
#define LOGMENUFLAGS(x) (MF_STRING | MF_ENABLED | (m_LogList.m_SelectedFilters & x ? MF_CHECKED : MF_UNCHECKED))
	CMenu popup;
	if (popup.CreatePopupMenu())
	{
		temp.LoadString(IDS_LOG_FILTER_SUBJECT);
		popup.AppendMenu(LOGMENUFLAGS(LOGFILTER_SUBJECT), LOGFILTER_SUBJECT, temp);

		temp.LoadString(IDS_LOG_FILTER_MESSAGES);
		popup.AppendMenu(LOGMENUFLAGS(LOGFILTER_MESSAGES), LOGFILTER_MESSAGES, temp);

		temp.LoadString(IDS_LOG_FILTER_PATHS);
		popup.AppendMenu(LOGMENUFLAGS(LOGFILTER_PATHS), LOGFILTER_PATHS, temp);

		temp.LoadString(IDS_LOG_FILTER_AUTHORS);
		popup.AppendMenu(LOGMENUFLAGS(LOGFILTER_AUTHORS), LOGFILTER_AUTHORS, temp);

		temp.LoadString(IDS_LOG_FILTER_REVS);
		popup.AppendMenu(LOGMENUFLAGS(LOGFILTER_REVS), LOGFILTER_REVS, temp);

		if (m_LogList.m_bShowBugtraqColumn == TRUE) {
			temp.LoadString(IDS_LOG_FILTER_BUGIDS);
			popup.AppendMenu(LOGMENUFLAGS(LOGFILTER_BUGID), LOGFILTER_BUGID, temp);
		}

		popup.AppendMenu(MF_SEPARATOR, NULL);

		temp.LoadString(IDS_LOG_FILTER_REGEX);
		popup.AppendMenu(MF_STRING | MF_ENABLED | (m_bFilterWithRegex ? MF_CHECKED : MF_UNCHECKED), LOGFILTER_REGEX, temp);

		m_tooltips.Pop();
		int selection = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
		if (selection != 0)
		{

			if (selection == LOGFILTER_REGEX)
			{
				m_bFilterWithRegex = !m_bFilterWithRegex;
				CRegDWORD b = CRegDWORD(_T("Software\\TortoiseGit\\UseRegexFilter"), TRUE);
				b = m_bFilterWithRegex;
				m_LogList.m_bFilterWithRegex = m_bFilterWithRegex;
				SetFilterCueText();
				CheckRegexpTooltip();
			}
			else
			{
				m_LogList.m_SelectedFilters ^= selection;
				SetFilterCueText();
			}
			SetTimer(LOGFILTER_TIMER, 1000, NULL);
		}
	}
	return 0L;
}

LRESULT CLogDlg::OnClickedCancelFilter(WPARAM /*wParam*/, LPARAM /*lParam*/)
{

	KillTimer(LOGFILTER_TIMER);

	m_LogList.m_sFilterText.Empty();
	UpdateData(FALSE);
	theApp.DoWaitCursor(1);
	CStoreSelection storeselection(this);
	FillLogMessageCtrl(false);

	m_LogList.RemoveFilter();

	Refresh();

	CTime begin,end;
	m_LogList.GetTimeRange(begin,end);
	m_DateFrom.SetTime(&begin);
	m_DateTo.SetTime(&end);

	theApp.DoWaitCursor(-1);
	GetDlgItem(IDC_SEARCHEDIT)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_SEARCHEDIT)->ShowWindow(SW_SHOW);
	GetDlgItem(IDC_SEARCHEDIT)->SetFocus();
	UpdateLogInfoLabel();

	return 0L;
}


void CLogDlg::SetFilterCueText()
{
	CString temp(MAKEINTRESOURCE(IDS_LOG_FILTER_BY));
	temp += _T(" ");

	if (m_LogList.m_SelectedFilters & LOGFILTER_SUBJECT)
	{
		temp += CString(MAKEINTRESOURCE(IDS_LOG_FILTER_SUBJECT));
	}

	if (m_LogList.m_SelectedFilters & LOGFILTER_MESSAGES)
	{
		if (temp.ReverseFind(_T(' ')) != temp.GetLength() - 1)
			temp += _T(", ");
		temp += CString(MAKEINTRESOURCE(IDS_LOG_FILTER_MESSAGES));
	}

	if (m_LogList.m_SelectedFilters & LOGFILTER_PATHS)
	{
		if (temp.ReverseFind(_T(' ')) != temp.GetLength() - 1)
			temp += _T(", ");
		temp += CString(MAKEINTRESOURCE(IDS_LOG_FILTER_PATHS));
	}

	if (m_LogList.m_SelectedFilters & LOGFILTER_AUTHORS)
	{
		if (temp.ReverseFind(_T(' ')) != temp.GetLength() - 1)
			temp += _T(", ");
		temp += CString(MAKEINTRESOURCE(IDS_LOG_FILTER_AUTHORS));
	}

	if (m_LogList.m_SelectedFilters & LOGFILTER_REVS)
	{
		if (temp.ReverseFind(_T(' ')) != temp.GetLength() - 1)
			temp += _T(", ");
		temp += CString(MAKEINTRESOURCE(IDS_LOG_FILTER_REVS));
	}

	if (m_LogList.m_SelectedFilters & LOGFILTER_BUGID)
	{
		if (temp.ReverseFind(_T(' ')) != temp.GetLength() - 1)
			temp += _T(", ");
		temp += CString(MAKEINTRESOURCE(IDS_LOG_FILTER_BUGIDS));
	}

	// to make the cue banner text appear more to the right of the edit control
	temp = _T("   ")+temp;
	m_cFilter.SetCueBanner(temp.TrimRight());
}

bool CLogDlg::Validate(LPCTSTR string)
{
	if (!m_bFilterWithRegex)
		return true;
	tr1::wregex pat;
	return m_LogList.ValidateRegexp(string, pat, false);
}


void CLogDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == LOGFTIME_TIMER)
	{
		KillTimer(LOGFTIME_TIMER);
		m_limit = 0;
		m_LogList.Refresh(FALSE);
		FillLogMessageCtrl(false);
	}

	if (nIDEvent == LOGFILTER_TIMER)
	{
		KillTimer(LOGFILTER_TIMER);
		m_limit = 0;
		m_LogList.Refresh(FALSE);
		FillLogMessageCtrl(false);

#if 0
		/* we will use git built-in grep to filter log */
		if (this->IsThreadRunning())
		{
			// thread still running! So just restart the timer.
			SetTimer(LOGFILTER_TIMER, 1000, NULL);
			return;
		}
		CWnd * focusWnd = GetFocus();
		bool bSetFocusToFilterControl = ((focusWnd != GetDlgItem(IDC_DATEFROM))&&(focusWnd != GetDlgItem(IDC_DATETO))
			&& (focusWnd != GetDlgItem(IDC_LOGLIST)));
		if (m_LogList.m_sFilterText.IsEmpty())
		{
			DialogEnableWindow(IDC_STATBUTTON, !(((this->IsThreadRunning())||(m_LogList.m_arShownList.IsEmpty()))));
			// do not return here!
			// we also need to run the filter if the filter text is empty:
			// 1. to clear an existing filter
			// 2. to rebuild the m_arShownList after sorting
		}
		theApp.DoWaitCursor(1);
		CStoreSelection storeselection(this);
		KillTimer(LOGFILTER_TIMER);
		FillLogMessageCtrl(false);

		// now start filter the log list
		m_LogList.StartFilter();

		if ( m_LogList.GetItemCount()==1 )
		{
			m_LogList.SetSelectionMark(0);
			m_LogList.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED);
		}
		theApp.DoWaitCursor(-1);
		GetDlgItem(IDC_SEARCHEDIT)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_SEARCHEDIT)->ShowWindow(SW_SHOW);
		if (bSetFocusToFilterControl)
			GetDlgItem(IDC_SEARCHEDIT)->SetFocus();
		UpdateLogInfoLabel();
#endif
	} // if (nIDEvent == LOGFILTER_TIMER)
	DialogEnableWindow(IDC_STATBUTTON, !(((this->IsThreadRunning())||(m_LogList.m_arShownList.IsEmpty() || m_LogList.m_arShownList.GetCount() == 1 && m_LogList.m_bShowWC))));
	__super::OnTimer(nIDEvent);
}

void CLogDlg::OnDtnDatetimechangeDateto(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	try
	{
		CTime _time;
		m_DateTo.GetTime(_time);

		CTime time(_time.GetYear(), _time.GetMonth(), _time.GetDay(), 23, 59, 59);
		if (time.GetTime() != m_LogList.m_To)
		{
			m_LogList.m_To = (DWORD)time.GetTime();
			SetTimer(LOGFTIME_TIMER, 10, NULL);
		}
	}
	catch (...)
	{
		CMessageBox::Show(NULL,_T("Invalidate Parameter"),_T("TortoiseGit"),MB_OK|MB_ICONERROR);
	}

	*pResult = 0;
}

void CLogDlg::OnDtnDatetimechangeDatefrom(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{

	try
	{
		CTime _time;
		m_DateFrom.GetTime(_time);

		CTime time(_time.GetYear(), _time.GetMonth(), _time.GetDay(), 0, 0, 0);
		if (time.GetTime() != m_LogList.m_From)
		{
			m_LogList.m_From = (DWORD)time.GetTime();
			SetTimer(LOGFTIME_TIMER, 10, NULL);
		}
	}
	catch (...)
	{
		CMessageBox::Show(NULL,_T("Invalidate Parameter"),_T("TortoiseGit"),MB_OK|MB_ICONERROR);
	}

	*pResult = 0;
}



CTGitPathList CLogDlg::GetChangedPathsFromSelectedRevisions(bool /*bRelativePaths*/ /* = false */, bool /*bUseFilter*/ /* = true */)
{
	CTGitPathList pathList;
#if 0

	if (m_sRepositoryRoot.IsEmpty() && (bRelativePaths == false))
	{
		m_sRepositoryRoot = GetRepositoryRoot(m_path);
	}
	if (m_sRepositoryRoot.IsEmpty() && (bRelativePaths == false))
		return pathList;

	POSITION pos = m_LogList.GetFirstSelectedItemPosition();
	if (pos != NULL)
	{
		while (pos)
		{
			int nextpos = m_LogList.GetNextSelectedItem(pos);
			if (nextpos >= m_arShownList.GetCount())
				continue;
			PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.SafeGetAt(nextpos));
			LogChangedPathArray * cpatharray = pLogEntry->pArChangedPaths;
			for (INT_PTR cpPathIndex = 0; cpPathIndex<cpatharray->GetCount(); ++cpPathIndex)
			{
				LogChangedPath * cpath = cpatharray->SafeGetAt(cpPathIndex);
				if (cpath == NULL)
					continue;
				CTGitPath path;
				if (!bRelativePaths)
					path.SetFromGit(m_sRepositoryRoot);
				path.AppendPathString(cpath->sPath);
				if ((!bUseFilter)||
					((m_cHidePaths.GetState() & 0x0003)!=BST_CHECKED)||
					(cpath->sPath.Left(m_sRelativeRoot.GetLength()).Compare(m_sRelativeRoot)==0))
					pathList.AddPath(path);

			}
		}
	}
	pathList.RemoveDuplicates();
#endif
	return pathList;
}

void CLogDlg::SortByColumn(int /*nSortColumn*/, bool /*bAscending*/)
{
#if 0
	switch(nSortColumn)
	{
	case 0: // Revision
		{
			if(bAscending)
				std::sort(m_logEntries.begin(), m_logEntries.end(), CLogDataVector::AscRevSort());
			else
				std::sort(m_logEntries.begin(), m_logEntries.end(), CLogDataVector::DescRevSort());
		}
		break;
	case 1: // action
		{
			if(bAscending)
				std::sort(m_logEntries.begin(), m_logEntries.end(), CLogDataVector::AscActionSort());
			else
				std::sort(m_logEntries.begin(), m_logEntries.end(), CLogDataVector::DescActionSort());
		}
		break;
	case 2: // Author
		{
			if(bAscending)
				std::sort(m_logEntries.begin(), m_logEntries.end(), CLogDataVector::AscAuthorSort());
			else
				std::sort(m_logEntries.begin(), m_logEntries.end(), CLogDataVector::DescAuthorSort());
		}
		break;
	case 3: // Date
		{
			if(bAscending)
				std::sort(m_logEntries.begin(), m_logEntries.end(), CLogDataVector::AscDateSort());
			else
				std::sort(m_logEntries.begin(), m_logEntries.end(), CLogDataVector::DescDateSort());
		}
		break;
	case 4: // Message or bug id
		if (m_bShowBugtraqColumn)
		{
			if(bAscending)
				std::sort(m_logEntries.begin(), m_logEntries.end(), CLogDataVector::AscBugIDSort());
			else
				std::sort(m_logEntries.begin(), m_logEntries.end(), CLogDataVector::DescBugIDSort());
			break;
		}
		// fall through here
	case 5: // Message
		{
			if(bAscending)
				std::sort(m_logEntries.begin(), m_logEntries.end(), CLogDataVector::AscMessageSort());
			else
				std::sort(m_logEntries.begin(), m_logEntries.end(), CLogDataVector::DescMessageSort());
		}
		break;
	default:
		ATLASSERT(0);
		break;
	}
#endif
}

void CLogDlg::OnLvnColumnclick(NMHDR *pNMHDR, LRESULT *pResult)
{
	if (this->IsThreadRunning())
		return;		//no sorting while the arrays are filled
#if 0
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	const int nColumn = pNMLV->iSubItem;
	m_bAscending = nColumn == m_nSortColumn ? !m_bAscending : TRUE;
	m_nSortColumn = nColumn;
	SortByColumn(m_nSortColumn, m_bAscending);
	SetSortArrow(&m_LogList, m_nSortColumn, !!m_bAscending);
	SortShownListArray();
	m_LogList.Invalidate();
	UpdateLogInfoLabel();
#else
	UNREFERENCED_PARAMETER(pNMHDR);
#endif
	*pResult = 0;
}

void CLogDlg::SortShownListArray()
{
	// make sure the shown list still matches the filter after sorting.
	OnTimer(LOGFILTER_TIMER);
	// clear the selection states
	POSITION pos = m_LogList.GetFirstSelectedItemPosition();
	while (pos)
	{
		m_LogList.SetItemState(m_LogList.GetNextSelectedItem(pos), 0, LVIS_SELECTED);
	}
	m_LogList.SetSelectionMark(-1);
}

void CLogDlg::SetSortArrow(CListCtrl * control, int nColumn, bool bAscending)
{
	if (control == NULL)
		return;
	// set the sort arrow
	CHeaderCtrl * pHeader = control->GetHeaderCtrl();
	HDITEM HeaderItem = {0};
	HeaderItem.mask = HDI_FORMAT;
	for (int i=0; i<pHeader->GetItemCount(); ++i)
	{
		pHeader->GetItem(i, &HeaderItem);
		HeaderItem.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
		pHeader->SetItem(i, &HeaderItem);
	}
	if (nColumn >= 0)
	{
		pHeader->GetItem(nColumn, &HeaderItem);
		HeaderItem.fmt |= (bAscending ? HDF_SORTUP : HDF_SORTDOWN);
		pHeader->SetItem(nColumn, &HeaderItem);
	}
}

int CLogDlg::m_nSortColumnPathList = 0;
bool CLogDlg::m_bAscendingPathList = false;

void CLogDlg::OnBnClickedHidepaths()
{
	FillLogMessageCtrl();
	m_ChangedFileListCtrl.Invalidate();
}

void CLogDlg::UpdateLogInfoLabel()
{

	CGitHash rev1 ;
	CGitHash rev2 ;
	long selectedrevs = 0;
	int count = (int)m_LogList.m_arShownList.GetCount();
	int start = 0;
	if (count)
	{
		rev1 = (reinterpret_cast<GitRev*>(m_LogList.m_arShownList.SafeGetAt(0)))->m_CommitHash;
		if(this->m_LogList.m_bShowWC && rev1.IsEmpty()&&(count>1))
			start = 1;
		rev1 = (reinterpret_cast<GitRev*>(m_LogList.m_arShownList.SafeGetAt(start)))->m_CommitHash;
		//pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.SafeGetAt(m_arShownList.GetCount()-1));
		rev2 =  (reinterpret_cast<GitRev*>(m_LogList.m_arShownList.SafeGetAt(count-1)))->m_CommitHash;
		selectedrevs = m_LogList.GetSelectedCount();
	}
	CString sTemp;
	sTemp.Format(IDS_PROC_LOG_STATS,
		count - start,
		rev2.ToString().Left(g_Git.GetShortHASHLength()), rev1.ToString().Left(g_Git.GetShortHASHLength()), selectedrevs);

	if(selectedrevs == 1)
	{
		CString str=m_ChangedFileListCtrl.GetStatisticsString(true);
		str.Replace(_T('\n'), _T(' '));
		sTemp += _T("\r\n") + str;
	}
	m_sLogInfo = sTemp;

	UpdateData(FALSE);
}

void CLogDlg::OnDtnDropdownDatefrom(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	// the date control should not show the "today" button
	CMonthCalCtrl * pCtrl = m_DateFrom.GetMonthCalCtrl();
	if (pCtrl)
		SetWindowLongPtr(pCtrl->GetSafeHwnd(), GWL_STYLE, LONG_PTR(pCtrl->GetStyle() | MCS_NOTODAY));
	*pResult = 0;
}

void CLogDlg::OnDtnDropdownDateto(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	// the date control should not show the "today" button
	CMonthCalCtrl * pCtrl = m_DateTo.GetMonthCalCtrl();
	if (pCtrl)
		SetWindowLongPtr(pCtrl->GetSafeHwnd(), GWL_STYLE, LONG_PTR(pCtrl->GetStyle() | MCS_NOTODAY));
	*pResult = 0;
}

void CLogDlg::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);
	//set range
	SetSplitterRange();
}

void CLogDlg::OnRefresh()
{
	{
		m_limit = 0;
		this->m_LogProgress.SetPos(0);

		Refresh (false);
	}
}

void CLogDlg::OnFocusFilter()
{
	GetDlgItem(IDC_SEARCHEDIT)->SetFocus();
}

void CLogDlg::OnEditCopy()
{
	if (GetFocus() == &m_ChangedFileListCtrl)
		CopyChangedSelectionToClipBoard();
	else
		m_LogList.CopySelectionToClipBoard();
}

CString CLogDlg::GetAbsoluteUrlFromRelativeUrl(const CString& url)
{
	// is the URL a relative one?
	if (url.Left(2).Compare(_T("^/")) == 0)
	{
		// URL is relative to the repository root
		CString url1 = m_sRepositoryRoot + url.Mid(1);
		TCHAR buf[INTERNET_MAX_URL_LENGTH];
		DWORD len = url.GetLength();
		if (UrlCanonicalize((LPCTSTR)url1, buf, &len, 0) == S_OK)
			return CString(buf, len);
		return url1;
	}
	else if (url[0] == '/')
	{
		// URL is relative to the server's hostname
		CString sHost;
		// find the server's hostname
		int schemepos = m_sRepositoryRoot.Find(_T("//"));
		if (schemepos >= 0)
		{
			sHost = m_sRepositoryRoot.Left(m_sRepositoryRoot.Find('/', schemepos+3));
			CString url1 = sHost + url;
			TCHAR buf[INTERNET_MAX_URL_LENGTH];
			DWORD len = url.GetLength();
			if (UrlCanonicalize((LPCTSTR)url, buf, &len, 0) == S_OK)
				return CString(buf, len);
			return url1;
		}
	}
	return url;
}


void CLogDlg::OnEnChangeSearchedit()
{
	UpdateData();
	if (m_LogList.m_sFilterText.IsEmpty())
	{
		CStoreSelection storeselection(this);
		// clear the filter, i.e. make all entries appear
		theApp.DoWaitCursor(1);
		KillTimer(LOGFILTER_TIMER);
		FillLogMessageCtrl(false);

		Refresh();
		//m_LogList.StartFilter();
#if 0
		InterlockedExchange(&m_bNoDispUpdates, TRUE);
		m_arShownList.RemoveAll();
		for (DWORD i=0; i<m_logEntries.size(); ++i)
		{
			if (IsEntryInDateRange(i))
				m_arShownList.Add(m_logEntries[i]);
		}
		InterlockedExchange(&m_bNoDispUpdates, FALSE);
		m_LogList.DeleteAllItems();
		m_LogList.SetItemCountEx(ShownCountWithStopped());
		m_LogList.RedrawItems(0, ShownCountWithStopped());
		m_LogList.SetRedraw(false);
		ResizeAllListCtrlCols();
		m_LogList.SetRedraw(true);
#endif
		theApp.DoWaitCursor(-1);
		GetDlgItem(IDC_SEARCHEDIT)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_SEARCHEDIT)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_SEARCHEDIT)->SetFocus();
		DialogEnableWindow(IDC_STATBUTTON, !(((this->IsThreadRunning())||(m_LogList.m_arShownList.IsEmpty()))));
		return;
	}
	if (Validate(m_LogList.m_sFilterText))
		SetTimer(LOGFILTER_TIMER, 1000, NULL);
	else
		KillTimer(LOGFILTER_TIMER);

}

void CLogDlg::OnBnClickedAllBranch()
{
	this->UpdateData();

	if(this->m_bAllBranch)
		m_LogList.m_ShowMask|=CGit::LOG_INFO_ALL_BRANCH;
	else
		m_LogList.m_ShowMask&=~CGit::LOG_INFO_ALL_BRANCH;

	OnRefresh();

	FillLogMessageCtrl(false);
}

void CLogDlg::OnBnClickedFollowRenames()
{
	this->UpdateData();

	if(m_bFollowRenames)
	{
		m_LogList.m_ShowMask |= CGit::LOG_INFO_FOLLOW;
		if (m_bAllBranch)
		{
			
			m_bAllBranch = FALSE;
			m_LogList.m_ShowMask &=~ CGit::LOG_INFO_ALL_BRANCH;
		}

	}
	else
		m_LogList.m_ShowMask &= ~CGit::LOG_INFO_FOLLOW;

	DialogEnableWindow(IDC_LOG_ALLBRANCH, !m_bFollowRenames);
	DialogEnableWindow(IDC_SHOWWHOLEPROJECT, !m_bFollowRenames);

	OnRefresh();

	FillLogMessageCtrl(false);
}

void CLogDlg::OnBnClickedBrowseRef()
{
	CString newRef = CBrowseRefsDlg::PickRef(false,m_LogList.GetStartRef());
	if(newRef.IsEmpty())
		return;

	SetStartRef(newRef);
	((CButton*)GetDlgItem(IDC_LOG_ALLBRANCH))->SetCheck(0);

	OnBnClickedAllBranch();
}

void CLogDlg::ShowStartRef()
{
	//Show ref name on top
	if(!::IsWindow(m_hWnd))
		return;
	if(m_bAllBranch)
	{
		m_staticRef.SetWindowText(CString(MAKEINTRESOURCE(IDS_PROC_LOG_ALLBRANCHES)));
		m_staticRef.Invalidate(TRUE);
		return;
	}

	CString showStartRef = m_LogList.GetStartRef();
	if(showStartRef.IsEmpty())
	{
		//Ref name is HEAD
		if (g_Git.Run(L"git symbolic-ref HEAD", &showStartRef, NULL, CP_UTF8))
			showStartRef = CString(MAKEINTRESOURCE(IDS_PROC_LOG_NOBRANCH));
		showStartRef.Trim(L"\r\n\t ");
	}


	if(wcsncmp(showStartRef,L"refs/",5) == 0)
		showStartRef = showStartRef.Mid(5);
	if(wcsncmp(showStartRef,L"heads/",6) == 0)
		showStartRef = showStartRef.Mid(6);

	m_staticRef.SetWindowText(showStartRef);
	m_staticRef.Invalidate(TRUE);
}

void CLogDlg::SetStartRef(const CString& StartRef)
{
	m_LogList.SetStartRef(StartRef);

	if (m_hightlightRevision.IsEmpty())
	{
		m_hightlightRevision = g_Git.GetHash(StartRef);
		m_LogList.m_lastSelectedHash = m_hightlightRevision;
	}

	ShowStartRef();
}


void CLogDlg::OnBnClickedFirstParent()
{
	this->UpdateData();

	if(this->m_bFirstParent)
		m_LogList.m_ShowMask|=CGit::LOG_INFO_FIRST_PARENT;
	else
		m_LogList.m_ShowMask&=~CGit::LOG_INFO_FIRST_PARENT;

	OnRefresh();

	FillLogMessageCtrl(false);

}

void CLogDlg::OnBnClickShowWholeProject()
{
	this->UpdateData();

	if(this->m_bWholeProject)
	{
		m_LogList.m_Path.Reset();
		SetDlgTitle();
		DialogEnableWindow(IDC_LOG_FOLLOWRENAMES, FALSE);
	}
	else
	{
		m_LogList.m_Path=m_path;
		DialogEnableWindow(IDC_LOG_FOLLOWRENAMES, !(m_path.IsEmpty() || m_path.IsDirectory()));
	}

	SetDlgTitle();

	OnRefresh();

	FillLogMessageCtrl(false);

}

LRESULT CLogDlg::OnRefLogChanged(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(lParam);
	ShowStartRef();
	return 0;
}

LRESULT CLogDlg::OnTaskbarBtnCreated(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	m_pTaskbarList.Release();
	m_pTaskbarList.CoCreateInstance(CLSID_TaskbarList);
	return 0;
}
