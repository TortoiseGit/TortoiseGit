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
#include "cursor.h"
#include "InputDlg.h"
#include "PropDlg.h"
#include "SVNProgressDlg.h"
#include "ProgressDlg.h"
//#include "RepositoryBrowser.h"
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
//#include "RevisionRangeDlg.h"
//#include "BrowseFolder.h"
//#include "BlameDlg.h"
//#include "Blame.h"
//#include "GitHelpers.h"
#include "GitStatus.h"
//#include "LogDlgHelper.h"
//#include "CachedLogInfo.h"
//#include "RepositoryInfo.h"
//#include "EditPropertiesDlg.h"
#include "FileDiffDlg.h"


const UINT CLogDlg::m_FindDialogMessage = RegisterWindowMessage(FINDMSGSTRING);


IMPLEMENT_DYNAMIC(CLogDlg, CResizableStandAloneDialog)
CLogDlg::CLogDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CLogDlg::IDD, pParent)
	, m_logcounter(0)
	, m_nSearchIndex(0)
	, m_wParam(0)
	, m_currentChangedArray(NULL)
	, m_nSortColumn(0)
	, m_bShowedAll(false)
	, m_bSelect(false)
	, m_regLastStrict(_T("Software\\TortoiseGit\\LastLogStrict"), FALSE)
	
	, m_bSelectionMustBeContinuous(false)
	, m_bShowBugtraqColumn(false)
	, m_lowestRev(_T(""))
	
	, m_sLogInfo(_T(""))
	, m_pFindDialog(NULL)
	, m_bCancelled(FALSE)
	, m_pNotifyWindow(NULL)
	
	, m_bAscending(FALSE)

	, m_limit(0)
	, m_childCounter(0)
	, m_maxChild(0)
	, m_bIncludeMerges(FALSE)
	, m_hAccel(NULL)
	, m_bVista(false)
{
	m_bFilterWithRegex = !!CRegDWORD(_T("Software\\TortoiseGit\\UseRegexFilter"), TRUE);
	
}

CLogDlg::~CLogDlg()
{
	
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
	DDX_Check(pDX, IDC_CHECK_STOPONCOPY, m_bStrict);
	DDX_Text(pDX, IDC_SEARCHEDIT, m_LogList.m_sFilterText);
	DDX_Control(pDX, IDC_DATEFROM, m_DateFrom);
	DDX_Control(pDX, IDC_DATETO, m_DateTo);
	DDX_Control(pDX, IDC_HIDEPATHS, m_cHidePaths);
	DDX_Control(pDX, IDC_GETALL, m_btnShow);
	DDX_Text(pDX, IDC_LOGINFO, m_sLogInfo);
	DDX_Check(pDX, IDC_INCLUDEMERGE, m_bIncludeMerges);
	DDX_Control(pDX, IDC_SEARCHEDIT, m_cFilter);
}

BEGIN_MESSAGE_MAP(CLogDlg, CResizableStandAloneDialog)
	ON_REGISTERED_MESSAGE(m_FindDialogMessage, OnFindDialogMessage) 
	ON_BN_CLICKED(IDC_GETALL, OnBnClickedGetall)
	//ON_NOTIFY(NM_DBLCLK, IDC_LOGMSG, OnNMDblclkChangedFileList)
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
	ON_BN_CLICKED(IDC_NEXTHUNDRED, OnBnClickedNexthundred)
	//ON_NOTIFY(NM_CUSTOMDRAW, IDC_LOGMSG, OnNMCustomdrawChangedFileList)
	//ON_NOTIFY(LVN_GETDISPINFO, IDC_LOGMSG, OnLvnGetdispinfoChangedFileList)
	ON_NOTIFY(LVN_COLUMNCLICK,IDC_LOGLIST	, OnLvnColumnclick)
	//ON_NOTIFY(LVN_COLUMNCLICK, IDC_LOGMSG, OnLvnColumnclickChangedFileList)
	ON_BN_CLICKED(IDC_HIDEPATHS, OnBnClickedHidepaths)
	
	ON_BN_CLICKED(IDC_CHECK_STOPONCOPY, &CLogDlg::OnBnClickedCheckStoponcopy)
	ON_NOTIFY(DTN_DROPDOWN, IDC_DATEFROM, &CLogDlg::OnDtnDropdownDatefrom)
	ON_NOTIFY(DTN_DROPDOWN, IDC_DATETO, &CLogDlg::OnDtnDropdownDateto)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_INCLUDEMERGE, &CLogDlg::OnBnClickedIncludemerge)
	ON_BN_CLICKED(IDC_REFRESH, &CLogDlg::OnBnClickedRefresh)
	ON_COMMAND(ID_LOGDLG_REFRESH,&CLogDlg::OnRefresh)
	ON_COMMAND(ID_LOGDLG_FIND,&CLogDlg::OnFind)
	ON_COMMAND(ID_LOGDLG_FOCUSFILTER,&CLogDlg::OnFocusFilter)
	ON_COMMAND(ID_EDIT_COPY, &CLogDlg::OnEditCopy)
END_MESSAGE_MAP()

void CLogDlg::SetParams(const CTGitPath& path, GitRev pegrev, GitRev startrev, GitRev endrev, int limit, BOOL bStrict /* = FALSE */, BOOL bSaveStrict /* = TRUE */)
{
	m_path = path;
	m_pegrev = pegrev;
	m_startrev = startrev;
	m_LogRevision = startrev;
	m_endrev = endrev;
	m_hasWC = !path.IsUrl();
	m_bStrict = bStrict;
	m_bSaveStrict = bSaveStrict;
	m_limit = limit;
	if (::IsWindow(m_hWnd))
		UpdateData(FALSE);
}

BOOL CLogDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	m_hAccel = LoadAccelerators(AfxGetResourceHandle(),MAKEINTRESOURCE(IDR_ACC_LOGDLG));

	OSVERSIONINFOEX inf;
	SecureZeroMemory(&inf, sizeof(OSVERSIONINFOEX));
	inf.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	GetVersionEx((OSVERSIONINFO *)&inf);
	WORD fullver = MAKEWORD(inf.dwMinorVersion, inf.dwMajorVersion);
	m_bVista = (fullver >= 0x0600);

	// use the state of the "stop on copy/rename" option from the last time
	if (!m_bStrict)
		m_bStrict = m_regLastStrict;
	UpdateData(FALSE);
	CString temp;
	if (m_limit)
		temp.Format(IDS_LOG_SHOWNEXT, m_limit);
	else
		temp.Format(IDS_LOG_SHOWNEXT, (int)(DWORD)CRegDWORD(_T("Software\\TortoiseGit\\NumberOfLogs"), 100));

	SetDlgItemText(IDC_NEXTHUNDRED, temp);

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

	
	// if there is a working copy, load the project properties
	// to get information about the bugtraq: integration
	if (m_hasWC)
		m_ProjectProperties.ReadProps(m_path);

	// the bugtraq issue id column is only shown if the bugtraq:url or bugtraq:regex is set
	if ((!m_ProjectProperties.sUrl.IsEmpty())||(!m_ProjectProperties.sCheckRe.IsEmpty()))
		m_bShowBugtraqColumn = true;

	//theme.SetWindowTheme(m_LogList.GetSafeHwnd(), L"Explorer", NULL);
	//theme.SetWindowTheme(m_ChangedFileListCtrl.GetSafeHwnd(), L"Explorer", NULL);

	// set up the columns
	m_LogList.DeleteAllItems();
	m_LogList.InsertGitColumn();

	m_ChangedFileListCtrl.Init(SVNSLC_COLEXT | SVNSLC_COLSTATUS |IDS_STATUSLIST_COLADD|IDS_STATUSLIST_COLDEL , _T("LogDlg"),(SVNSLC_POPALL ^ SVNSLC_POPCOMMIT),false);

	GetDlgItem(IDC_LOGLIST)->UpdateData(FALSE);

	m_logcounter = 0;
	m_sMessageBuf.Preallocate(100000);

	// set the dialog title to "Log - path/to/whatever/we/show/the/log/for"
	SetDlgTitle(false);

	m_tooltips.Create(this);
	CheckRegexpTooltip();

	SetSplitterRange();
	
	// the filter control has a 'cancel' button (the red 'X'), we need to load its bitmap
	m_cFilter.SetCancelBitmaps(IDI_CANCELNORMAL, IDI_CANCELPRESSED);
	m_cFilter.SetInfoIcon(IDI_LOGFILTER);
	m_cFilter.SetValidator(this);
	
	AdjustControlSize(IDC_HIDEPATHS);
	AdjustControlSize(IDC_CHECK_STOPONCOPY);
	AdjustControlSize(IDC_INCLUDEMERGE);

	GetClientRect(m_DlgOrigRect);
	m_LogList.GetClientRect(m_LogListOrigRect);
	GetDlgItem(IDC_MSGVIEW)->GetClientRect(m_MsgViewOrigRect);
	m_ChangedFileListCtrl.GetClientRect(m_ChgOrigRect);

	m_DateFrom.SendMessage(DTM_SETMCSTYLE, 0, MCS_WEEKNUMBERS|MCS_NOTODAY|MCS_NOTRAILINGDATES|MCS_NOSELCHANGEONNAV);
	m_DateTo.SendMessage(DTM_SETMCSTYLE, 0, MCS_WEEKNUMBERS|MCS_NOTODAY|MCS_NOTRAILINGDATES|MCS_NOSELCHANGEONNAV);

	// resizable stuff
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
	AddAnchor(IDC_CHECK_STOPONCOPY, BOTTOM_LEFT);
	AddAnchor(IDC_INCLUDEMERGE, BOTTOM_LEFT);
	AddAnchor(IDC_GETALL, BOTTOM_LEFT);
	AddAnchor(IDC_NEXTHUNDRED, BOTTOM_LEFT);
	AddAnchor(IDC_REFRESH, BOTTOM_LEFT);
	AddAnchor(IDC_STATBUTTON, BOTTOM_RIGHT);
	AddAnchor(IDC_PROGRESS, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);

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
		if (m_bSelectionMustBeContinuous)
			DialogEnableWindow(IDOK, (m_LogList.GetSelectedCount()!=0)&&(m_LogList.IsSelectionContinuous()));
		else
			DialogEnableWindow(IDOK, m_LogList.GetSelectedCount()!=0);
	}
	else
	{
		// the dialog is used to just view log messages
		GetDlgItemText(IDOK, temp);
		SetDlgItemText(IDCANCEL, temp);
		GetDlgItem(IDOK)->ShowWindow(SW_HIDE);
	}
	
	// set the choices for the "Show All" button
	temp.LoadString(IDS_LOG_SHOWALL);
	m_btnShow.AddEntry(temp);
	temp.LoadString(IDS_LOG_SHOW_WHOLE);
	m_btnShow.AddEntry(temp);
	m_btnShow.SetCurrentEntry((LONG)CRegDWORD(_T("Software\\TortoiseGit\\ShowAllEntry")));

	m_mergedRevs.clear();

	// first start a thread to obtain the log messages without
	// blocking the dialog
	//m_tTo = 0;
	//m_tFrom = (DWORD)-1;

	m_LogList.m_Path=m_path;
	m_LogList.FetchLogAsync(this);

	GetDlgItem(IDC_LOGLIST)->SetFocus();
	return FALSE;
}

LRESULT CLogDlg::OnLogListLoading(WPARAM wParam, LPARAM lParam)
{
	int cur=(int)wParam;

	if( cur == GITLOG_START )
	{
		CString temp;
		temp.LoadString(IDS_PROGRESSWAIT);

		// change the text of the close button to "Cancel" since now the thread
		// is running, and simply closing the dialog doesn't work.
		if (!GetDlgItem(IDOK)->IsWindowVisible())
		{
			temp.LoadString(IDS_MSGBOX_CANCEL);
			SetDlgItemText(IDCANCEL, temp);
		}

		// We use a progress bar while getting the logs	
		m_LogProgress.SetRange32(0, 100);
		m_LogProgress.SetPos(0);

		GetDlgItem(IDC_PROGRESS)->ShowWindow(TRUE);

		//DialogEnableWindow(IDC_GETALL, FALSE);
		DialogEnableWindow(IDC_NEXTHUNDRED, FALSE);
		DialogEnableWindow(IDC_CHECK_STOPONCOPY, FALSE);
		DialogEnableWindow(IDC_INCLUDEMERGE, FALSE);
		DialogEnableWindow(IDC_STATBUTTON, FALSE);
		DialogEnableWindow(IDC_REFRESH, FALSE);
		DialogEnableWindow(IDC_HIDEPATHS,FALSE);
	}

	if( cur == GITLOG_END)
	{
		
		if (!m_bShowedAll)
			DialogEnableWindow(IDC_NEXTHUNDRED, TRUE);

		DialogEnableWindow(IDC_GETALL, TRUE);
		//DialogEnableWindow(IDC_INCLUDEMERGE, TRUE);
		DialogEnableWindow(IDC_STATBUTTON, TRUE);
		DialogEnableWindow(IDC_REFRESH, TRUE);

//		PostMessage(WM_TIMER, LOGFILTER_TIMER);

		//CTime time=m_LogList.GetOldestTime();
		CTime begin,end;
		m_LogList.GetTimeRange(begin,end);
		m_DateFrom.SetTime(&begin);
		m_DateTo.SetTime(&end);
	}

	m_LogProgress.SetPos(cur);
	return 0;
}
void CLogDlg::SetDlgTitle(bool bOffline)
{
	if (m_sTitle.IsEmpty())
		GetWindowText(m_sTitle);

	if (bOffline)
	{
		CString sTemp;
		if (m_path.IsUrl())
			sTemp.Format(IDS_LOG_DLGTITLEOFFLINE, (LPCTSTR)m_sTitle, (LPCTSTR)m_path.GetUIPathString());
		else if (m_path.IsDirectory())
			sTemp.Format(IDS_LOG_DLGTITLEOFFLINE, (LPCTSTR)m_sTitle, (LPCTSTR)m_path.GetWinPathString());
		else
			sTemp.Format(IDS_LOG_DLGTITLEOFFLINE, (LPCTSTR)m_sTitle, (LPCTSTR)m_path.GetFilename());
		SetWindowText(sTemp);
	}
	else
	{
		if (m_path.IsUrl())
			SetWindowText(m_sTitle + _T(" - ") + m_path.GetUIPathString());
		else if (m_path.IsEmpty())
			SetWindowText(m_sTitle + _T(" - ") + CString(_T("Whole Project")));
		else if (m_path.IsDirectory())
			SetWindowText(m_sTitle + _T(" - ") + m_path.GetWinPathString());
		else
			SetWindowText(m_sTitle + _T(" - ") + m_path.GetFilename());
	}
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
		if (m_bSelectionMustBeContinuous)
			DialogEnableWindow(IDOK, (m_LogList.GetSelectedCount()!=0)&&(m_LogList.IsSelectionContinuous()));
		else
			DialogEnableWindow(IDOK, m_LogList.GetSelectedCount()!=0);
	}
	else
		DialogEnableWindow(IDOK, TRUE);
}

void CLogDlg::FillLogMessageCtrl(bool bShow /* = true*/)
{
	// we fill here the log message rich edit control,
	// and also populate the changed files list control
	// according to the selected revision(s).

	CWnd * pMsgView = GetDlgItem(IDC_MSGVIEW);
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
		GitRev* pLogEntry = reinterpret_cast<GitRev *>(m_LogList.m_arShownList.GetAt(selIndex));

		if(!pLogEntry->m_IsFull)
		{
			pMsgView->SetWindowText(_T("load ..."));
		}else
		{
			// set the log message text
			pMsgView->SetWindowText(_T("Commit:")+pLogEntry->m_CommitHash+_T("\r\n\r\n*")+pLogEntry->m_Subject+_T("\n\n")+pLogEntry->m_Body);
			// turn bug ID's into links if the bugtraq: properties have been set
			// and we can find a match of those in the log message
			m_ProjectProperties.FindBugID(pLogEntry->m_Body, pMsgView);
			CAppUtils::FormatTextInRichEditControl(pMsgView);

			m_ChangedFileListCtrl.UpdateWithGitPathList(pLogEntry->m_Files);
			m_ChangedFileListCtrl.m_CurrentVersion=pLogEntry->m_CommitHash;
			m_ChangedFileListCtrl.Show(SVNSLC_SHOWVERSIONED);

			m_ChangedFileListCtrl.SetRedraw(TRUE);
			return;
		}
#if 0
		// fill in the changed files list control
		if ((m_cHidePaths.GetState() & 0x0003)==BST_CHECKED)
		{
			m_CurrentFilteredChangedArray.RemoveAll();
			for (INT_PTR c = 0; c < m_currentChangedArray->GetCount(); ++c)
			{
				LogChangedPath * cpath = m_currentChangedArray->GetAt(c);
				if (cpath == NULL)
					continue;
				if (m_currentChangedArray->GetAt(c)->sPath.Left(m_sRelativeRoot.GetLength()).Compare(m_sRelativeRoot)==0)
				{
					m_CurrentFilteredChangedArray.Add(cpath);
				}
			}
			m_currentChangedArray = &m_CurrentFilteredChangedArray;
		}
#endif
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
	CAppUtils::ResizeAllListCtrlCols(&m_ChangedFileListCtrl);
	// sort according to the settings
	if (m_nSortColumnPathList > 0)
		SetSortArrow(&m_ChangedFileListCtrl, m_nSortColumnPathList, m_bAscendingPathList);
	else
		SetSortArrow(&m_ChangedFileListCtrl, -1, false);
	m_ChangedFileListCtrl.SetRedraw(TRUE);

}

void CLogDlg::OnBnClickedGetall()
{
	GetAll();
}

void CLogDlg::GetAll(bool bForceAll /* = false */)
{

	// fetch all requested log messages, either the specified range or
	// really *all* available log messages.
	///UpdateData();
	INT_PTR entry = m_btnShow.GetCurrentEntry();
	if (bForceAll)
		entry = 0;

	switch (entry)
	{
	case 0:	// show all branch;
		m_LogList.m_bAllBranch=true;
		break;
	case 1: // show whole project
		m_LogList.m_Path.Reset();
		SetWindowText(m_sTitle + _T(" - "));
		break;
	}
	m_LogList.m_bExitThread=TRUE;
	DWORD ret =::WaitForSingleObject(m_LogList.m_LoadingThread->m_hThread,20000);
	if(ret == WAIT_TIMEOUT)
		m_LogList.TerminateThread();
	
	m_LogList.Clear();
	m_LogList.FetchLogAsync(this);

}

void CLogDlg::OnBnClickedRefresh()
{
	m_limit = 0;
	Refresh (true);
}

void CLogDlg::Refresh (bool autoGoOnline)
{
	m_LogList.Refresh();
}

void CLogDlg::OnBnClickedNexthundred()
{
#if 0
	UpdateData();
	// we have to fetch the next X log messages.
	if (m_logEntries.size() < 1)
	{
		// since there weren't any log messages fetched before, just
		// fetch all since we don't have an 'anchor' to fetch the 'next'
		// messages from.
		return GetAll(true);
	}
	git_revnum_t rev = m_logEntries[m_logEntries.size()-1]->Rev;

	if (rev < 1)
		return;		// do nothing! No more revisions to get

	m_startrev = rev;
	m_endrev = 0;
	m_bCancelled = FALSE;

    // rev is is revision we already have and we will receive it again
    // -> fetch one extra revision to get NumberOfLogs *new* revisions

	m_limit = (int)(DWORD)CRegDWORD(_T("Software\\TortoiseGit\\NumberOfLogs"), 100) +1;
	InterlockedExchange(&m_bNoDispUpdates, TRUE);
	SetSortArrow(&m_LogList, -1, true);
	InterlockedExchange(&m_bThreadRunning, TRUE);
	// We need to create CStoreSelection on the heap or else
	// the variable will run out of the scope before the
	// thread ends. Therefore we let the thread delete
	// the instance.
	m_pStoreSelection = new CStoreSelection(this);

	// since we fetch the log from the last revision we already have,
	// we have to remove that revision entry to avoid getting it twice
	m_logEntries.pop_back();
	if (AfxBeginThread(LogThreadEntry, this)==NULL)
	{
		InterlockedExchange(&m_bThreadRunning, FALSE);
		CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	InterlockedExchange(&m_bNoDispUpdates, TRUE);
	GetDlgItem(IDC_LOGLIST)->UpdateData(FALSE);
#endif
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
	// canceling means stopping the working thread if it's still running.
	// we do this by using the Subversion cancel callback.
	// But canceling can also mean just to close the dialog, depending on the
	// text shown on the cancel button (it could simply read "OK").
	CString temp, temp2;
	GetDlgItemText(IDOK, temp);
	temp2.LoadString(IDS_MSGBOX_CANCEL);
	if ((temp.Compare(temp2)==0)||(this->IsThreadRunning()))
	{
		//m_bCancelled = true;
		//return;
		if(m_LogList.m_bThreadRunning)
		{
			//m_LogList.m_bExitThread=true;
			//WaitForSingleObject(m_LogList.m_LoadingThread->m_hThread,INFINITE);
			m_LogList.TerminateThread();
		}

		//m_LogList.TerminateThread();
	}
	UpdateData();
	if (m_bSaveStrict)
		m_regLastStrict = m_bStrict;
	CRegDWORD reg = CRegDWORD(_T("Software\\TortoiseGit\\ShowAllEntry"));
	reg = m_btnShow.GetCurrentEntry();
	SaveSplitterPos();
	__super::OnCancel();
}

CString CLogDlg::MakeShortMessage(const CString& message)
{
	bool bFoundShort = true;
	CString sShortMessage = m_ProjectProperties.GetLogSummary(message);
	if (sShortMessage.IsEmpty())
	{
		bFoundShort = false;
		sShortMessage = message;
	}
	// Remove newlines and tabs 'cause those are not shown nicely in the list control
	sShortMessage.Replace(_T("\r"), _T(""));
	sShortMessage.Replace(_T("\t"), _T(" "));
	
	// Suppose the first empty line separates 'summary' from the rest of the message.
	int found = sShortMessage.Find(_T("\n\n"));
	// To avoid too short 'short' messages 
	// (e.g. if the message looks something like "Bugfix:\n\n*done this\n*done that")
	// only use the empty newline as a separator if it comes after at least 15 chars.
	if ((!bFoundShort)&&(found >= 15))
	{
		sShortMessage = sShortMessage.Left(found);
	}
	sShortMessage.Replace('\n', ' ');
	return sShortMessage;
}

BOOL CLogDlg::Log(git_revnum_t rev, const CString& author, const CString& date, const CString& message, LogChangedPathArray * cpaths,  int filechanges, BOOL copies, DWORD actions, BOOL haschildren)
{
#if 0
	if (rev == SVN_INVALID_REVNUM)
	{
		m_childCounter--;
		return TRUE;
	}

	// this is the callback function which receives the data for every revision we ask the log for
	// we store this information here one by one.
	m_logcounter += 1;
	if (m_startrev == -1)
		m_startrev = rev;
	if (m_limit != 0)
	{
		m_limitcounter--;
		m_LogProgress.SetPos(m_limit - m_limitcounter);
	}
	else if (m_startrev.IsNumber() && m_startrev.IsNumber())
		m_LogProgress.SetPos((git_revnum_t)m_startrev-rev+(git_revnum_t)m_endrev);
	__time64_t ttime = time/1000000L;
	if (m_tTo < (DWORD)ttime)
		m_tTo = (DWORD)ttime;
	if (m_tFrom > (DWORD)ttime)
		m_tFrom = (DWORD)ttime;
	if ((m_lowestRev > rev)||(m_lowestRev < 0))
		m_lowestRev = rev;
	// Add as many characters from the log message to the list control
	PLOGENTRYDATA pLogItem = new LOGENTRYDATA;
	pLogItem->bCopies = !!copies;
	
	// find out if this item was copied in the revision
	BOOL copiedself = FALSE;
	if (copies)
	{
		for (INT_PTR cpPathIndex = 0; cpPathIndex < cpaths->GetCount(); ++cpPathIndex)
		{
			LogChangedPath * cpath = cpaths->GetAt(cpPathIndex);
			if (!cpath->sCopyFromPath.IsEmpty() && (cpath->sPath.Compare(m_sSelfRelativeURL) == 0))
			{
				// note: this only works if the log is fetched top-to-bottom
				// but since we do that, it shouldn't be a problem
				m_sSelfRelativeURL = cpath->sCopyFromPath;
				copiedself = TRUE;
				break;
			}
		}
	}
	pLogItem->bCopiedSelf = copiedself;
	pLogItem->tmDate = ttime;
	pLogItem->sAuthor = author;
	pLogItem->sDate = date;
	pLogItem->sShortMessage = MakeShortMessage(message);
	pLogItem->dwFileChanges = filechanges;
	pLogItem->actions = actions;
	pLogItem->haschildren = haschildren;
	pLogItem->childStackDepth = m_childCounter;
	m_maxChild = max(m_childCounter, m_maxChild);
	if (haschildren)
		m_childCounter++;
	pLogItem->sBugIDs = m_ProjectProperties.FindBugID(message).Trim();
	
	// split multi line log entries and concatenate them
	// again but this time with \r\n as line separators
	// so that the edit control recognizes them
	try
	{
		if (message.GetLength()>0)
		{
			m_sMessageBuf = message;
			m_sMessageBuf.Replace(_T("\n\r"), _T("\n"));
			m_sMessageBuf.Replace(_T("\r\n"), _T("\n"));
			if (m_sMessageBuf.Right(1).Compare(_T("\n"))==0)
				m_sMessageBuf = m_sMessageBuf.Left(m_sMessageBuf.GetLength()-1);
		}
		else
			m_sMessageBuf.Empty();
        pLogItem->sMessage = m_sMessageBuf;
        pLogItem->Rev = rev;

        // move-construct path array

        pLogItem->pArChangedPaths = new LogChangedPathArray (*cpaths);
        cpaths->RemoveAll();
	}
	catch (CException * e)
	{
		::MessageBox(NULL, _T("not enough memory!"), _T("TortoiseGit"), MB_ICONERROR);
		e->Delete();
		m_bCancelled = TRUE;
	}
	m_logEntries.push_back(pLogItem);
	m_arShownList.Add(pLogItem);
#endif
	return TRUE;
}

GitRev g_rev;
//this is the thread function which calls the subversion function




void CLogDlg::CopyChangedSelectionToClipBoard()
{
#if 0
	POSITION pos = m_LogList.GetFirstSelectedItemPosition();
	if (pos == NULL)
		return;	// nothing is selected, get out of here

	CString sPaths;

	PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(m_LogList.GetNextSelectedItem(pos)));
	if (pos)
	{
		POSITION pos = m_ChangedFileListCtrl.GetFirstSelectedItemPosition();
		while (pos)
		{
			int nItem = m_ChangedFileListCtrl.GetNextSelectedItem(pos);
			sPaths += m_currentChangedPathList[nItem].GetGitPathString();
			sPaths += _T("\r\n");
		}
	}
	else
	{
		// only one revision is selected in the log dialog top pane
		// but multiple items could be selected  in the changed items list
		POSITION pos = m_ChangedFileListCtrl.GetFirstSelectedItemPosition();
		while (pos)
		{
			int nItem = m_ChangedFileListCtrl.GetNextSelectedItem(pos);
			LogChangedPath * changedlogpath = pLogEntry->pArChangedPaths->GetAt(nItem);

			if ((m_cHidePaths.GetState() & 0x0003)==BST_CHECKED)
			{
				// some items are hidden! So find out which item the user really selected
				INT_PTR selRealIndex = -1;
				for (INT_PTR hiddenindex=0; hiddenindex<pLogEntry->pArChangedPaths->GetCount(); ++hiddenindex)
				{
					if (pLogEntry->pArChangedPaths->GetAt(hiddenindex)->sPath.Left(m_sRelativeRoot.GetLength()).Compare(m_sRelativeRoot)==0)
						selRealIndex++;
					if (selRealIndex == nItem)
					{
						changedlogpath = pLogEntry->pArChangedPaths->GetAt(hiddenindex);
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
	sPaths.Trim();
	CStringUtils::WriteAsciiStringToClipboard(sPaths, GetSafeHwnd());
#endif
}

BOOL CLogDlg::IsDiffPossible(LogChangedPath * changedpath, git_revnum_t rev)
{
#if 0
	CString added, deleted;
	if (changedpath == NULL)
		return false;

	if ((rev > 1)&&(changedpath->action != LOGACTIONS_DELETED))
	{
		if (changedpath->action == LOGACTIONS_ADDED) // file is added
		{
			if (changedpath->lCopyFromRev == 0)
				return FALSE; // but file was not added with history
		}
		return TRUE;
	}
#endif
	return FALSE;
}

void CLogDlg::OnContextMenu(CWnd* pWnd, CPoint point)
{
	// we have two separate context menus:
	// one shown on the log message list control,
	// the other shown in the changed-files list control
	int selCount = m_LogList.GetSelectedCount();
	if (pWnd == &m_LogList)
	{
		//ShowContextMenuForRevisions(pWnd, point);
	}
	else if (pWnd == &m_ChangedFileListCtrl)
	{
		//ShowContextMenuForChangedpaths(pWnd, point);
	}
	else if ((selCount == 1)&&(pWnd == GetDlgItem(IDC_MSGVIEW)))
	{
		POSITION pos = m_LogList.GetFirstSelectedItemPosition();
		int selIndex = -1;
		if (pos)
			selIndex = m_LogList.GetNextSelectedItem(pos);
		if ((point.x == -1) && (point.y == -1))
		{
			CRect rect;
			GetDlgItem(IDC_MSGVIEW)->GetClientRect(&rect);
			ClientToScreen(&rect);
			point = rect.CenterPoint();
		}
		CString sMenuItemText;
		CMenu popup;
		if (popup.CreatePopupMenu())
		{
			// add the 'default' entries
			sMenuItemText.LoadString(IDS_SCIEDIT_COPY);
			popup.AppendMenu(MF_STRING | MF_ENABLED, WM_COPY, sMenuItemText);
			sMenuItemText.LoadString(IDS_SCIEDIT_SELECTALL);
			popup.AppendMenu(MF_STRING | MF_ENABLED, EM_SETSEL, sMenuItemText);

			//if (selIndex >= 0)
			//{
			//	popup.AppendMenu(MF_SEPARATOR);
			//	sMenuItemText.LoadString(IDS_LOG_POPUP_EDITLOG);
			//	popup.AppendMenu(MF_STRING | MF_ENABLED, CGitLogList::ID_EDITAUTHOR, sMenuItemText);
			//}

			int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
			switch (cmd)
			{
			case 0:
				break;	// no command selected
			case EM_SETSEL:
			case WM_COPY:
				::SendMessage(GetDlgItem(IDC_MSGVIEW)->GetSafeHwnd(), cmd, 0, -1);
				break;
			case CGitLogList::ID_EDITAUTHOR:
				EditLogMessage(selIndex);
				break;
			}
		}
	}
}


LRESULT CLogDlg::OnFindDialogMessage(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
#if 0
    ASSERT(m_pFindDialog != NULL);

    if (m_pFindDialog->IsTerminating())
    {
	    // invalidate the handle identifying the dialog box.
        m_pFindDialog = NULL;
        return 0;
    }

    if(m_pFindDialog->FindNext())
    {
        //read data from dialog
        CString FindText = m_pFindDialog->GetFindString();
        bool bMatchCase = (m_pFindDialog->MatchCase() == TRUE);
		bool bFound = false;
		tr1::wregex pat;
		bool bRegex = ValidateRegexp(FindText, pat, bMatchCase);

		tr1::regex_constants::match_flag_type flags = tr1::regex_constants::match_not_null;

		int i;
		for (i = this->m_nSearchIndex; i<m_arShownList.GetCount()&&!bFound; i++)
		{
			if (bRegex)
			{
				PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(i));

				if (regex_search(wstring((LPCTSTR)pLogEntry->sMessage), pat, flags))
				{
					bFound = true;
					break;
				}
				LogChangedPathArray * cpatharray = pLogEntry->pArChangedPaths;
				for (INT_PTR cpPathIndex = 0; cpPathIndex<cpatharray->GetCount(); ++cpPathIndex)
				{
					LogChangedPath * cpath = cpatharray->GetAt(cpPathIndex);
					if (regex_search(wstring((LPCTSTR)cpath->sCopyFromPath), pat, flags))
					{
						bFound = true;
						--i;
						break;
					}
					if (regex_search(wstring((LPCTSTR)cpath->sPath), pat, flags))
					{
						bFound = true;
						--i;
						break;
					}
				}
			}
			else
			{
				if (bMatchCase)
				{
					if (m_logEntries[i]->sMessage.Find(FindText) >= 0)
					{
						bFound = true;
						break;
					}
					PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(i));
					LogChangedPathArray * cpatharray = pLogEntry->pArChangedPaths;
					for (INT_PTR cpPathIndex = 0; cpPathIndex<cpatharray->GetCount(); ++cpPathIndex)
					{
						LogChangedPath * cpath = cpatharray->GetAt(cpPathIndex);
						if (cpath->sCopyFromPath.Find(FindText)>=0)
						{
							bFound = true;
							--i;
							break;
						}
						if (cpath->sPath.Find(FindText)>=0)
						{
							bFound = true;
							--i;
							break;
						}
					}
				}
				else
				{
				    PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(i));
					CString msg = pLogEntry->sMessage;
					msg = msg.MakeLower();
					CString find = FindText.MakeLower();
					if (msg.Find(find) >= 0)
					{
						bFound = TRUE;
						break;
					}
					LogChangedPathArray * cpatharray = pLogEntry->pArChangedPaths;
					for (INT_PTR cpPathIndex = 0; cpPathIndex<cpatharray->GetCount(); ++cpPathIndex)
					{
						LogChangedPath * cpath = cpatharray->GetAt(cpPathIndex);
						CString lowerpath = cpath->sCopyFromPath;
						lowerpath.MakeLower();
						if (lowerpath.Find(find)>=0)
						{
							bFound = TRUE;
							--i;
							break;
						}
						lowerpath = cpath->sPath;
						lowerpath.MakeLower();
						if (lowerpath.Find(find)>=0)
						{
							bFound = TRUE;
							--i;
							break;
						}
					}
				} 
			}
		} // for (i = this->m_nSearchIndex; i<m_arShownList.GetItemCount()&&!bFound; i++)
		if (bFound)
		{
			this->m_nSearchIndex = (i+1);
			m_LogList.EnsureVisible(i, FALSE);
			m_LogList.SetItemState(m_LogList.GetSelectionMark(), 0, LVIS_SELECTED);
			m_LogList.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
			m_LogList.SetSelectionMark(i);
			FillLogMessageCtrl();
			UpdateData(FALSE);
			m_nSearchIndex++;
			if (m_nSearchIndex >= m_arShownList.GetCount())
				m_nSearchIndex = (int)m_arShownList.GetCount()-1;
		}
    } // if(m_pFindDialog->FindNext()) 
	UpdateLogInfoLabel();
#endif
    return 0;
}

void CLogDlg::OnOK()
{
#if 0 
	// since the log dialog is also used to select revisions for other
	// dialogs, we have to do some work before closing this dialog
	if (GetFocus() != GetDlgItem(IDOK))
		return;	// if the "OK" button doesn't have the focus, do nothing: this prevents closing the dialog when pressing enter
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
			pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(m_LogList.GetNextSelectedItem(pos)));
			m_selectedRevs.AddRevision(pLogEntry->Rev);
			git_revnum_t lowerRev = pLogEntry->Rev;
			git_revnum_t higherRev = lowerRev;
			while (pos)
			{
			    pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(m_LogList.GetNextSelectedItem(pos)));
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
						LogChangedPath * pData = pLogEntry->pArChangedPaths->GetAt(cp);
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
	if (m_bSaveStrict)
		m_regLastStrict = m_bStrict;
	CRegDWORD reg = CRegDWORD(_T("Software\\TortoiseGit\\ShowAllEntry"));
	reg = m_btnShow.GetCurrentEntry();
	SaveSplitterPos();
#endif 
}

void CLogDlg::OnNMDblclkChangedFileList(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	// a double click on an entry in the changed-files list has happened
	*pResult = 0;

	DiffSelectedFile();
}

void CLogDlg::DiffSelectedFile()
{
#if 0
	if (m_bThreadRunning)
		return;
	UpdateLogInfoLabel();
	INT_PTR selIndex = m_ChangedFileListCtrl.GetSelectionMark();
	if (selIndex < 0)
		return;
	if (m_ChangedFileListCtrl.GetSelectedCount() == 0)
		return;
	// find out if there's an entry selected in the log list
	POSITION pos = m_LogList.GetFirstSelectedItemPosition();
	PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(m_LogList.GetNextSelectedItem(pos)));
	git_revnum_t rev1 = pLogEntry->Rev;
	git_revnum_t rev2 = rev1;
	if (pos)
	{
		while (pos)
		{
			// there's at least a second entry selected in the log list: several revisions selected!
			pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(m_LogList.GetNextSelectedItem(pos)));
			if (pLogEntry)
			{
				rev1 = max(rev1,(long)pLogEntry->Rev);
				rev2 = min(rev2,(long)pLogEntry->Rev);
			}
		}
		rev2--;
		// now we have both revisions selected in the log list, so we can do a diff of the selected
		// entry in the changed files list with these two revisions.
		DoDiffFromLog(selIndex, rev1, rev2, false, false);
	}
	else
	{
		rev2 = rev1-1;
		// nothing or only one revision selected in the log list
		LogChangedPath * changedpath = pLogEntry->pArChangedPaths->GetAt(selIndex);

		if ((m_cHidePaths.GetState() & 0x0003)==BST_CHECKED)
		{
			// some items are hidden! So find out which item the user really clicked on
			INT_PTR selRealIndex = -1;
			for (INT_PTR hiddenindex=0; hiddenindex<pLogEntry->pArChangedPaths->GetCount(); ++hiddenindex)
			{
				if (pLogEntry->pArChangedPaths->GetAt(hiddenindex)->sPath.Left(m_sRelativeRoot.GetLength()).Compare(m_sRelativeRoot)==0)
					selRealIndex++;
				if (selRealIndex == selIndex)
				{
					selIndex = hiddenindex;
					changedpath = pLogEntry->pArChangedPaths->GetAt(selIndex);
					break;
				}
			}
		}

		if (IsDiffPossible(changedpath, rev1))
		{
			// diffs with renamed files are possible
			if ((changedpath)&&(!changedpath->sCopyFromPath.IsEmpty()))
				rev2 = changedpath->lCopyFromRev;
			else
			{
				// if the path was modified but the parent path was 'added with history'
				// then we have to use the copy from revision of the parent path
				CTGitPath cpath = CTGitPath(changedpath->sPath);
				for (int flist = 0; flist < pLogEntry->pArChangedPaths->GetCount(); ++flist)
				{
					CTGitPath p = CTGitPath(pLogEntry->pArChangedPaths->GetAt(flist)->sPath);
					if (p.IsAncestorOf(cpath))
					{
						if (!pLogEntry->pArChangedPaths->GetAt(flist)->sCopyFromPath.IsEmpty())
							rev2 = pLogEntry->pArChangedPaths->GetAt(flist)->lCopyFromRev;
					}
				}
			}
			DoDiffFromLog(selIndex, rev1, rev2, false, false);
		}
		else 
		{
			CTGitPath tempfile = CTempFiles::Instance().GetTempFilePath(false, CTGitPath(changedpath->sPath));
			CTGitPath tempfile2 = CTempFiles::Instance().GetTempFilePath(false, CTGitPath(changedpath->sPath));
			GitRev r = rev1;
			// deleted files must be opened from the revision before the deletion
			if (changedpath->action == LOGACTIONS_DELETED)
				r = rev1-1;
			m_bCancelled = false;

			CProgressDlg progDlg;
			progDlg.SetTitle(IDS_APPNAME);
			progDlg.SetAnimation(IDR_DOWNLOAD);
			CString sInfoLine;
			sInfoLine.Format(IDS_PROGRESSGETFILEREVISION, (LPCTSTR)(m_sRepositoryRoot + changedpath->sPath), (LPCTSTR)r.ToString());
			progDlg.SetLine(1, sInfoLine, true);
			SetAndClearProgressInfo(&progDlg);
			progDlg.ShowModeless(m_hWnd);

			if (!Cat(CTGitPath(m_sRepositoryRoot + changedpath->sPath), r, r, tempfile))
			{
				m_bCancelled = false;
				if (!Cat(CTGitPath(m_sRepositoryRoot + changedpath->sPath), GitRev::REV_HEAD, r, tempfile))
				{
					progDlg.Stop();
					SetAndClearProgressInfo((HWND)NULL);
					CMessageBox::Show(m_hWnd, GetLastErrorMessage(), _T("TortoiseGit"), MB_ICONERROR);
					return;
				}
			}
			progDlg.Stop();
			SetAndClearProgressInfo((HWND)NULL);

			CString sName1, sName2;
			sName1.Format(_T("%s - Revision %ld"), (LPCTSTR)CPathUtils::GetFileNameFromPath(changedpath->sPath), (git_revnum_t)rev1);
			sName2.Format(_T("%s - Revision %ld"), (LPCTSTR)CPathUtils::GetFileNameFromPath(changedpath->sPath), (git_revnum_t)rev1-1);
			CAppUtils::DiffFlags flags;
			flags.AlternativeTool(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
			if (changedpath->action == LOGACTIONS_DELETED)
				CAppUtils::StartExtDiff(tempfile, tempfile2, sName2, sName1, flags);
			else
				CAppUtils::StartExtDiff(tempfile2, tempfile, sName2, sName1, flags);
		}
	}
#endif 
}


void CLogDlg::DoDiffFromLog(INT_PTR selIndex, GitRev* rev1, GitRev* rev2, bool blame, bool unified)
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
				rev1->m_CommitHash.Left(6),
				(*m_currentChangedArray)[selIndex].GetFileExtension());

	CString file2;
	file2.Format(_T("%s\\%s_%s%s"),
				temppath,						
				(*m_currentChangedArray)[selIndex].GetBaseFilename(),
				rev2->m_CommitHash.Left(6),
				(*m_currentChangedArray)[selIndex].GetFileExtension());

	CString cmd;

	cmd.Format(_T("git.exe cat-file -p %s:%s"),rev1->m_CommitHash,(*m_currentChangedArray)[selIndex].GetGitPathString());
	g_Git.RunLogFile(cmd,file1);
	cmd.Format(_T("git.exe cat-file -p %s:%s"),rev2->m_CommitHash,(*m_currentChangedArray)[selIndex].GetGitPathString());
	g_Git.RunLogFile(cmd,file2);

	CAppUtils::DiffFlags flags;
	CAppUtils::StartExtDiff(file1,file2,_T("A"),_T("B"),flags);

#if 0
	//get the filename
	CString filepath;
	if (Git::PathIsURL(m_path))
	{
		filepath = m_path.GetGitPathString();
	}
	else
	{
		filepath = GetURLFromPath(m_path);
		if (filepath.IsEmpty())
		{
			theApp.DoWaitCursor(-1);
			CString temp;
			temp.Format(IDS_ERR_NOURLOFFILE, (LPCTSTR)filepath);
			CMessageBox::Show(this->m_hWnd, temp, _T("TortoiseGit"), MB_ICONERROR);
			TRACE(_T("could not retrieve the URL of the file!\n"));
			EnableOKButton();
			theApp.DoWaitCursor(-11);
			return;		//exit
		}
	}
	m_bCancelled = FALSE;
	filepath = GetRepositoryRoot(CTGitPath(filepath));

	CString firstfile, secondfile;
	if (m_LogList.GetSelectedCount()==1)
	{
		int s = m_LogList.GetSelectionMark();
		PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(s));
		LogChangedPath * changedpath = pLogEntry->pArChangedPaths->GetAt(selIndex);
		firstfile = changedpath->sPath;
		secondfile = firstfile;
		if ((rev2 == rev1-1)&&(changedpath->lCopyFromRev > 0)) // is it an added file with history?
		{
			secondfile = changedpath->sCopyFromPath;
			rev2 = changedpath->lCopyFromRev;
		}
	}
	else
	{
		firstfile = m_currentChangedPathList[selIndex].GetGitPathString();
		secondfile = firstfile;
	}

	firstfile = filepath + firstfile.Trim();
	secondfile = filepath + secondfile.Trim();

	GitDiff diff(this, this->m_hWnd, true);
	diff.SetAlternativeTool(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
	diff.SetHEADPeg(m_LogRevision);
	if (unified)
	{
		if (PromptShown())
			diff.ShowUnifiedDiff(CTGitPath(secondfile), rev2, CTGitPath(firstfile), rev1);
		else
			CAppUtils::StartShowUnifiedDiff(m_hWnd, CTGitPath(secondfile), rev2, CTGitPath(firstfile), rev1, GitRev(), m_LogRevision);
	}
	else
	{
		if (diff.ShowCompare(CTGitPath(secondfile), rev2, CTGitPath(firstfile), rev1, GitRev(), false, blame))
		{
			if (firstfile.Compare(secondfile)==0)
			{
				git_revnum_t baseRev = 0;
				diff.DiffProps(CTGitPath(firstfile), rev2, rev1, baseRev);
			}
		}
	}

#endif

	theApp.DoWaitCursor(-1);
	EnableOKButton();
}

BOOL CLogDlg::Open(bool bOpenWith,CString changedpath, git_revnum_t rev)
{
#if 0
	DialogEnableWindow(IDOK, FALSE);
	SetPromptApp(&theApp);
	theApp.DoWaitCursor(1);
	CString filepath;
	if (Git::PathIsURL(m_path))
	{
		filepath = m_path.GetGitPathString();
	}
	else
	{
		filepath = GetURLFromPath(m_path);
		if (filepath.IsEmpty())
		{
			theApp.DoWaitCursor(-1);
			CString temp;
			temp.Format(IDS_ERR_NOURLOFFILE, (LPCTSTR)filepath);
			CMessageBox::Show(this->m_hWnd, temp, _T("TortoiseGit"), MB_ICONERROR);
			TRACE(_T("could not retrieve the URL of the file!\n"));
			EnableOKButton();
			return FALSE;
		}
	}
	m_bCancelled = false;
	filepath = GetRepositoryRoot(CTGitPath(filepath));
	filepath += changedpath;

	CProgressDlg progDlg;
	progDlg.SetTitle(IDS_APPNAME);
	progDlg.SetAnimation(IDR_DOWNLOAD);
	CString sInfoLine;
	sInfoLine.Format(IDS_PROGRESSGETFILEREVISION, (LPCTSTR)filepath, (LPCTSTR)GitRev(rev).ToString());
	progDlg.SetLine(1, sInfoLine, true);
	SetAndClearProgressInfo(&progDlg);
	progDlg.ShowModeless(m_hWnd);

	CTGitPath tempfile = CTempFiles::Instance().GetTempFilePath(false, CTGitPath(filepath), rev);
	m_bCancelled = false;
	if (!Cat(CTGitPath(filepath), GitRev(rev), rev, tempfile))
	{
		progDlg.Stop();
		SetAndClearProgressInfo((HWND)NULL);
		CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseGit"), MB_ICONERROR);
		EnableOKButton();
		theApp.DoWaitCursor(-1);
		return FALSE;
	}
	progDlg.Stop();
	SetAndClearProgressInfo((HWND)NULL);
	SetFileAttributes(tempfile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
	if (!bOpenWith)
	{
		int ret = (int)ShellExecute(this->m_hWnd, NULL, tempfile.GetWinPath(), NULL, NULL, SW_SHOWNORMAL);
		if (ret <= HINSTANCE_ERROR)
			bOpenWith = true;
	}
	if (bOpenWith)
	{
		CString cmd = _T("RUNDLL32 Shell32,OpenAs_RunDLL ");
		cmd += tempfile.GetWinPathString() + _T(" ");
		CAppUtils::LaunchApplication(cmd, NULL, false);
	}
	EnableOKButton();
	theApp.DoWaitCursor(-1);
#endif
	return TRUE;
}

void CLogDlg::EditAuthor(const CLogDataVector& logs)
{
#if 0
	CString url;
	CString name;
	if (logs.size() == 0)
		return;
	DialogEnableWindow(IDOK, FALSE);
	SetPromptApp(&theApp);
	theApp.DoWaitCursor(1);
	if (Git::PathIsURL(m_path))
		url = m_path.GetGitPathString();
	else
	{
		url = GetURLFromPath(m_path);
	}
	name = Git_PROP_REVISION_AUTHOR;

	CString value = RevPropertyGet(name, CTGitPath(url), logs[0]->Rev);
	CString sOldValue = value;
	value.Replace(_T("\n"), _T("\r\n"));
	CInputDlg dlg(this);
	dlg.m_sHintText.LoadString(IDS_LOG_AUTHOR);
	dlg.m_sInputText = value;
	dlg.m_sTitle.LoadString(IDS_LOG_AUTHOREDITTITLE);
	dlg.m_pProjectProperties = &m_ProjectProperties;
	dlg.m_bUseLogWidth = false;
	if (dlg.DoModal() == IDOK)
	{
		dlg.m_sInputText.Replace(_T("\r"), _T(""));

		LogCache::CCachedLogInfo* toUpdate 
			= GetLogCache (CTGitPath (m_sRepositoryRoot));

		CProgressDlg progDlg;
		progDlg.SetTitle(IDS_APPNAME);
		progDlg.SetLine(1, CString(MAKEINTRESOURCE(IDS_PROGRESSWAIT)));
		progDlg.SetTime(true);
		progDlg.SetShowProgressBar(true);
		progDlg.ShowModeless(m_hWnd);
		for (DWORD i=0; i<logs.size(); ++i)
		{
			if (!RevPropertySet(name, dlg.m_sInputText, sOldValue, CTGitPath(url), logs[i]->Rev))
			{
				progDlg.Stop();
				CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseGit"), MB_ICONERROR);
				break;
			}
			else
			{

				logs[i]->sAuthor = dlg.m_sInputText;
				m_LogList.Invalidate();

				// update the log cache 

				if (toUpdate != NULL)
				{
					// log caching is active

					LogCache::CCachedLogInfo newInfo;
					newInfo.Insert ( logs[i]->Rev
						, (const char*) CUnicodeUtils::GetUTF8 (logs[i]->sAuthor)
						, ""
						, 0
						, LogCache::CRevisionInfoContainer::HAS_AUTHOR);

					toUpdate->Update (newInfo);
				}
			}
			progDlg.SetProgress64(i, logs.size());
		}
		progDlg.Stop();
	}
	theApp.DoWaitCursor(-1);
	EnableOKButton();
#endif
}

void CLogDlg::EditLogMessage(int index)
{
#if 0
	CString url;
	CString name;
	DialogEnableWindow(IDOK, FALSE);
	SetPromptApp(&theApp);
	theApp.DoWaitCursor(1);
	if (Git::PathIsURL(m_path))
		url = m_path.GetGitPathString();
	else
	{
		url = GetURLFromPath(m_path);
	}
	name = Git_PROP_REVISION_LOG;

	PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(index));
	m_bCancelled = FALSE;
	CString value = RevPropertyGet(name, CTGitPath(url), pLogEntry->Rev);
	CString sOldValue = value;
	value.Replace(_T("\n"), _T("\r\n"));
	CInputDlg dlg(this);
	dlg.m_sHintText.LoadString(IDS_LOG_MESSAGE);
	dlg.m_sInputText = value;
	dlg.m_sTitle.LoadString(IDS_LOG_MESSAGEEDITTITLE);
	dlg.m_pProjectProperties = &m_ProjectProperties;
	dlg.m_bUseLogWidth = true;
	if (dlg.DoModal() == IDOK)
	{
		dlg.m_sInputText.Replace(_T("\r"), _T(""));
		if (!RevPropertySet(name, dlg.m_sInputText, sOldValue, CTGitPath(url), pLogEntry->Rev))
		{
			CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseGit"), MB_ICONERROR);
		}
		else
		{
			pLogEntry->sShortMessage = MakeShortMessage(dlg.m_sInputText);
			// split multi line log entries and concatenate them
			// again but this time with \r\n as line separators
			// so that the edit control recognizes them
			if (dlg.m_sInputText.GetLength()>0)
			{
				m_sMessageBuf = dlg.m_sInputText;
				dlg.m_sInputText.Replace(_T("\n\r"), _T("\n"));
				dlg.m_sInputText.Replace(_T("\r\n"), _T("\n"));
				if (dlg.m_sInputText.Right(1).Compare(_T("\n"))==0)
					dlg.m_sInputText = dlg.m_sInputText.Left(dlg.m_sInputText.GetLength()-1);
			} 
			else
				dlg.m_sInputText.Empty();
			pLogEntry->sMessage = dlg.m_sInputText;
			pLogEntry->sBugIDs = m_ProjectProperties.FindBugID(dlg.m_sInputText);
			CWnd * pMsgView = GetDlgItem(IDC_MSGVIEW);
			pMsgView->SetWindowText(_T(" "));
			pMsgView->SetWindowText(dlg.m_sInputText);
			m_ProjectProperties.FindBugID(dlg.m_sInputText, pMsgView);
			m_LogList.Invalidate();
        
            // update the log cache 

            LogCache::CCachedLogInfo* toUpdate 
                = GetLogCache (CTGitPath (m_sRepositoryRoot));
            if (toUpdate != NULL)
            {
                // log caching is active

                LogCache::CCachedLogInfo newInfo;
                newInfo.Insert ( pLogEntry->Rev
                               , ""
                               , (const char*) CUnicodeUtils::GetUTF8 (pLogEntry->sMessage)
                               , 0
                               , LogCache::CRevisionInfoContainer::HAS_COMMENT);

                toUpdate->Update (newInfo);
            }
        }
	}
	theApp.DoWaitCursor(-1);
	EnableOKButton();
#endif
}
#if 0
BOOL CLogDlg::PreTranslateMessage(MSG* pMsg)
{
	// Skip Ctrl-C when copying text out of the log message or search filter
	BOOL bSkipAccelerator = ( pMsg->message == WM_KEYDOWN && pMsg->wParam=='C' && (GetFocus()==GetDlgItem(IDC_MSGVIEW) || GetFocus()==GetDlgItem(IDC_SEARCHEDIT) ) && GetKeyState(VK_CONTROL)&0x8000 );
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam=='\r')
	{
		if (GetFocus()==GetDlgItem(IDC_LOGLIST))
		{
			if (CRegDWORD(_T("Software\\TortoiseGit\\DiffByDoubleClickInLog"), FALSE))
			{
				DiffSelectedRevWithPrevious();
				return TRUE;
			}
		}
		if (GetFocus()==GetDlgItem(IDC_LOGMSG))
		{
			DiffSelectedFile();
			return TRUE;
		}
	}
	if (m_hAccel && !bSkipAccelerator)
	{
		int ret = TranslateAccelerator(m_hWnd, m_hAccel, pMsg);
		if (ret)
			return TRUE;
	}
	
	m_tooltips.RelayEvent(pMsg);
	return __super::PreTranslateMessage(pMsg);
}
#endif

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
		m_nSearchIndex = pNMLV->iItem;
		if (pNMLV->iSubItem != 0)
			return;
		if ((pNMLV->iItem == m_LogList.m_arShownList.GetCount())&&(m_bStrict)&&(1/*m_bStrictStopped*/))
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
			url = m_ProjectProperties.GetBugIDUrl(url);
			url = GetAbsoluteUrlFromRelativeUrl(url);
		}
		if (!url.IsEmpty())
			ShellExecute(this->m_hWnd, _T("open"), url, NULL, NULL, SW_SHOWDEFAULT);
	}
	*pResult = 0;
}

void CLogDlg::OnBnClickedStatbutton()
{

	if (this->IsThreadRunning())
		return;
	if (m_LogList.m_arShownList.IsEmpty())
		return;		// nothing is shown, so no statistics.
	// the statistics dialog expects the log entries to be sorted by date
	SortByColumn(3, false);
	CPtrArray shownlist;
	m_LogList.RecalculateShownList(&shownlist);
	// create arrays which are aware of the current filter
	CStringArray m_arAuthorsFiltered;
	CDWordArray m_arDatesFiltered;
	CDWordArray m_arFileChangesFiltered;
	for (INT_PTR i=0; i<shownlist.GetCount(); ++i)
	{
		GitRev* pLogEntry = reinterpret_cast<GitRev*>(shownlist.GetAt(i));
		CString strAuthor = pLogEntry->m_AuthorName;
		if ( strAuthor.IsEmpty() )
		{
			strAuthor.LoadString(IDS_STATGRAPH_EMPTYAUTHOR);
		}
		m_arAuthorsFiltered.Add(strAuthor);
		m_arDatesFiltered.Add(pLogEntry->m_AuthorDate.GetTime());
		m_arFileChangesFiltered.Add(pLogEntry->m_Files.GetCount());
	}
	CStatGraphDlg dlg;
	dlg.m_parAuthors = &m_arAuthorsFiltered;
	dlg.m_parDates = &m_arDatesFiltered;
	dlg.m_parFileChanges = &m_arFileChangesFiltered;
	dlg.m_path = m_path;
	dlg.DoModal();
	// restore the previous sorting
	SortByColumn(m_nSortColumn, m_bAscending);
	OnTimer(LOGFILTER_TIMER);

}

#if 0
void CLogDlg::OnNMCustomdrawLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{

	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>( pNMHDR );
	// Take the default processing unless we set this to something else below.
	*pResult = CDRF_DODEFAULT;

	if (m_bNoDispUpdates)
		return;

	switch (pLVCD->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		{
			*pResult = CDRF_NOTIFYITEMDRAW;
			return;
		}
		break;
	case CDDS_ITEMPREPAINT:
		{
			// This is the prepaint stage for an item. Here's where we set the
			// item's text color. 
			
			// Tell Windows to send draw notifications for each subitem.
			*pResult = CDRF_NOTIFYSUBITEMDRAW;

			COLORREF crText = GetSysColor(COLOR_WINDOWTEXT);

			if (m_arShownList.GetCount() > (INT_PTR)pLVCD->nmcd.dwItemSpec)
			{
				GitRev* data = (GitRev*)m_arShownList.GetAt(pLVCD->nmcd.dwItemSpec);
				if (data)
				{
#if 0
					if (data->bCopiedSelf)
					{
						// only change the background color if the item is not 'hot' (on vista with themes enabled)
						if (!theme.IsAppThemed() || !m_bVista || ((pLVCD->nmcd.uItemState & CDIS_HOT)==0))
							pLVCD->clrTextBk = GetSysColor(COLOR_MENU);
					}

					if (data->bCopies)
						crText = m_Colors.GetColor(CColors::Modified);
#endif
//					if ((data->childStackDepth)||(m_mergedRevs.find(data->Rev) != m_mergedRevs.end()))
//						crText = GetSysColor(COLOR_GRAYTEXT);
//					if (data->Rev == m_wcRev)
//					{
//						SelectObject(pLVCD->nmcd.hdc, m_boldFont);
						// We changed the font, so we're returning CDRF_NEWFONT. This
						// tells the control to recalculate the extent of the text.
//						*pResult = CDRF_NOTIFYSUBITEMDRAW | CDRF_NEWFONT;
//					}
				}
			}
			if (m_arShownList.GetCount() == (INT_PTR)pLVCD->nmcd.dwItemSpec)
			{
				if (m_bStrictStopped)
					crText = GetSysColor(COLOR_GRAYTEXT);
			}
			// Store the color back in the NMLVCUSTOMDRAW struct.
			pLVCD->clrText = crText;
			return;
		}
		break;
	case CDDS_ITEMPREPAINT|CDDS_ITEM|CDDS_SUBITEM:
		{
			if ((m_bStrictStopped)&&(m_arShownList.GetCount() == (INT_PTR)pLVCD->nmcd.dwItemSpec))
			{
				pLVCD->nmcd.uItemState &= ~(CDIS_SELECTED|CDIS_FOCUS);
			}
			if (pLVCD->iSubItem == 1)
			{
				*pResult = CDRF_DODEFAULT;

				if (m_arShownList.GetCount() <= (INT_PTR)pLVCD->nmcd.dwItemSpec)
					return;

				int		nIcons = 0;
				int		iconwidth = ::GetSystemMetrics(SM_CXSMICON);
				int		iconheight = ::GetSystemMetrics(SM_CYSMICON);

				GitRev* pLogEntry = reinterpret_cast<GitRev *>(m_arShownList.GetAt(pLVCD->nmcd.dwItemSpec));

				// Get the selected state of the
				// item being drawn.
				LVITEM   rItem;
				SecureZeroMemory(&rItem, sizeof(LVITEM));
				rItem.mask  = LVIF_STATE;
				rItem.iItem = pLVCD->nmcd.dwItemSpec;
				rItem.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
				m_LogList.GetItem(&rItem);

				CRect rect;
				m_LogList.GetSubItemRect(pLVCD->nmcd.dwItemSpec, pLVCD->iSubItem, LVIR_BOUNDS, rect);

				// Fill the background
				if (theme.IsAppThemed() && m_bVista)
				{
					theme.Open(m_hWnd, L"Explorer");
					int state = LISS_NORMAL;
					if (rItem.state & LVIS_SELECTED)
					{
						if (::GetFocus() == m_LogList.m_hWnd)
							state |= LISS_SELECTED;
						else
							state |= LISS_SELECTEDNOTFOCUS;
					}
					else
					{
#if 0
						if (pLogEntry->bCopiedSelf)
						{
							// unfortunately, the pLVCD->nmcd.uItemState does not contain valid
							// information at this drawing stage. But we can check the whether the
							// previous stage changed the background color of the item
							if (pLVCD->clrTextBk == GetSysColor(COLOR_MENU))
							{
								HBRUSH brush;
								brush = ::CreateSolidBrush(::GetSysColor(COLOR_MENU));
								if (brush)
								{
									::FillRect(pLVCD->nmcd.hdc, &rect, brush);
									::DeleteObject(brush);
								}
							}
						}
#endif
					}

					if (theme.IsBackgroundPartiallyTransparent(LVP_LISTDETAIL, state))
						theme.DrawParentBackground(m_hWnd, pLVCD->nmcd.hdc, &rect);

					theme.DrawBackground(pLVCD->nmcd.hdc, LVP_LISTDETAIL, state, &rect, NULL);
				}
				else
				{
					HBRUSH brush;
					if (rItem.state & LVIS_SELECTED)
					{
						if (::GetFocus() == m_LogList.m_hWnd)
							brush = ::CreateSolidBrush(::GetSysColor(COLOR_HIGHLIGHT));
						else
							brush = ::CreateSolidBrush(::GetSysColor(COLOR_BTNFACE));
					}
					else
					{
						//if (pLogEntry->bCopiedSelf)
						//	brush = ::CreateSolidBrush(::GetSysColor(COLOR_MENU));
						//else
							brush = ::CreateSolidBrush(::GetSysColor(COLOR_WINDOW));
					}
					if (brush == NULL)
						return;

					::FillRect(pLVCD->nmcd.hdc, &rect, brush);
					::DeleteObject(brush);
				}

				// Draw the icon(s) into the compatible DC
				if (pLogEntry->m_Action & CTGitPath::LOGACTIONS_MODIFIED)
					::DrawIconEx(pLVCD->nmcd.hdc, rect.left + ICONITEMBORDER, rect.top, m_hModifiedIcon, iconwidth, iconheight, 0, NULL, DI_NORMAL);
				nIcons++;

				if (pLogEntry->m_Action & CTGitPath::LOGACTIONS_ADDED)
					::DrawIconEx(pLVCD->nmcd.hdc, rect.left+nIcons*iconwidth + ICONITEMBORDER, rect.top, m_hAddedIcon, iconwidth, iconheight, 0, NULL, DI_NORMAL);
				nIcons++;

				if (pLogEntry->m_Action & CTGitPath::LOGACTIONS_DELETED)
					::DrawIconEx(pLVCD->nmcd.hdc, rect.left+nIcons*iconwidth + ICONITEMBORDER, rect.top, m_hDeletedIcon, iconwidth, iconheight, 0, NULL, DI_NORMAL);
				nIcons++;

				if (pLogEntry->m_Action & CTGitPath::LOGACTIONS_REPLACED)
					::DrawIconEx(pLVCD->nmcd.hdc, rect.left+nIcons*iconwidth + ICONITEMBORDER, rect.top, m_hReplacedIcon, iconwidth, iconheight, 0, NULL, DI_NORMAL);
				nIcons++;
				*pResult = CDRF_SKIPDEFAULT;
				return;
			}
		}
		break;
	}
	*pResult = CDRF_DODEFAULT;

}
#endif

void CLogDlg::OnNMCustomdrawChangedFileList(NMHDR *pNMHDR, LRESULT *pResult)
{

	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>( pNMHDR );
	// Take the default processing unless we set this to something else below.
	*pResult = CDRF_DODEFAULT;

//	if (m_bNoDispUpdates)
//		return;

	// First thing - check the draw stage. If it's the control's prepaint
	// stage, then tell Windows we want messages for every item.

	if ( CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage )
	{
		*pResult = CDRF_NOTIFYITEMDRAW;
	}
	else if ( CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage )
	{
		// This is the prepaint stage for an item. Here's where we set the
		// item's text color. Our return value will tell Windows to draw the
		// item itself, but it will use the new color we set here.

		// Tell Windows to paint the control itself.
		*pResult = CDRF_DODEFAULT;

		COLORREF crText = GetSysColor(COLOR_WINDOWTEXT);
		bool bGrayed = false;
#if 0
		if ((m_cHidePaths.GetState() & 0x0003)==BST_INDETERMINATE)
		{
			if ((m_currentChangedArray)&&((m_currentChangedArray->GetCount() > (INT_PTR)pLVCD->nmcd.dwItemSpec)))
			{
				//if ((*m_currentChangedArray)[(pLVCD->nmcd.dwItemSpec)]sPath.Left(m_sRelativeRoot.GetLength()).Compare(m_sRelativeRoot)!=0)
				{
					crText = GetSysColor(COLOR_GRAYTEXT);
					bGrayed = true;
				}
			}
			else if (m_currentChangedPathList.GetCount() > (INT_PTR)pLVCD->nmcd.dwItemSpec)
			{
				//if (m_currentChangedPathList[pLVCD->nmcd.dwItemSpec].GetGitPathString().Left(m_sRelativeRoot.GetLength()).Compare(m_sRelativeRoot)!=0)
				{
					crText = GetSysColor(COLOR_GRAYTEXT);
					bGrayed = true;
				}
			}
		}

#endif
		if ((!bGrayed)&&(m_currentChangedArray)&&(m_currentChangedArray->GetCount() > (INT_PTR)pLVCD->nmcd.dwItemSpec))
		{
			DWORD action = ((*m_currentChangedArray)[pLVCD->nmcd.dwItemSpec]).m_Action;
			if (action == CTGitPath::LOGACTIONS_MODIFIED)
				crText = m_Colors.GetColor(CColors::Modified);
			if (action == CTGitPath::LOGACTIONS_REPLACED)
				crText = m_Colors.GetColor(CColors::Deleted);
			if (action == CTGitPath::LOGACTIONS_ADDED)
				crText = m_Colors.GetColor(CColors::Added);
			if (action == CTGitPath::LOGACTIONS_DELETED)
				crText = m_Colors.GetColor(CColors::Deleted);
		}

		// Store the color back in the NMLVCUSTOMDRAW struct.
		pLVCD->clrText = crText;
	}
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
	RECT * rect = (LPRECT)lParam;
	CPoint point;
	CString temp;
	point = CPoint(rect->left, rect->bottom);
#define LOGMENUFLAGS(x) (MF_STRING | MF_ENABLED | (m_LogList.m_nSelectedFilter == x ? MF_CHECKED : MF_UNCHECKED))
	CMenu popup;
	if (popup.CreatePopupMenu())
	{
		temp.LoadString(IDS_LOG_FILTER_ALL);
		popup.AppendMenu(LOGMENUFLAGS(LOGFILTER_ALL), LOGFILTER_ALL, temp);

		popup.AppendMenu(MF_SEPARATOR, NULL);
		
		temp.LoadString(IDS_LOG_FILTER_MESSAGES);
		popup.AppendMenu(LOGMENUFLAGS(LOGFILTER_MESSAGES), LOGFILTER_MESSAGES, temp);
		temp.LoadString(IDS_LOG_FILTER_PATHS);
		popup.AppendMenu(LOGMENUFLAGS(LOGFILTER_PATHS), LOGFILTER_PATHS, temp);
		temp.LoadString(IDS_LOG_FILTER_AUTHORS);
		popup.AppendMenu(LOGMENUFLAGS(LOGFILTER_AUTHORS), LOGFILTER_AUTHORS, temp);
		temp.LoadString(IDS_LOG_FILTER_REVS);
		popup.AppendMenu(LOGMENUFLAGS(LOGFILTER_REVS), LOGFILTER_REVS, temp);
		temp.LoadString(IDS_LOG_FILTER_BUGIDS);
		popup.AppendMenu(LOGMENUFLAGS(LOGFILTER_BUGID), LOGFILTER_BUGID, temp);
		
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
				CheckRegexpTooltip();
			}
			else
			{
				m_LogList.m_nSelectedFilter = selection;
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
	CString temp;
	switch (m_LogList.m_nSelectedFilter)
	{
	case LOGFILTER_ALL:
		temp.LoadString(IDS_LOG_FILTER_ALL);
		break;
	case LOGFILTER_MESSAGES:
		temp.LoadString(IDS_LOG_FILTER_MESSAGES);
		break;
	case LOGFILTER_PATHS:
		temp.LoadString(IDS_LOG_FILTER_PATHS);
		break;
	case LOGFILTER_AUTHORS:
		temp.LoadString(IDS_LOG_FILTER_AUTHORS);
		break;
	case LOGFILTER_REVS:
		temp.LoadString(IDS_LOG_FILTER_REVS);
		break;
	}
	// to make the cue banner text appear more to the right of the edit control
	temp = _T("   ")+temp;
	m_cFilter.SetCueBanner(temp);
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
	if (nIDEvent == LOGFILTER_TIMER)
	{
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
	} // if (nIDEvent == LOGFILTER_TIMER)
	DialogEnableWindow(IDC_STATBUTTON, !(((this->IsThreadRunning())||(m_LogList.m_arShownList.IsEmpty()))));
	__super::OnTimer(nIDEvent);
}

void CLogDlg::OnDtnDatetimechangeDateto(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	CTime _time;
	m_DateTo.GetTime(_time);
	try
	{
		CTime time(_time.GetYear(), _time.GetMonth(), _time.GetDay(), 23, 59, 59);
		if (time.GetTime() != m_LogList.m_To.GetTime())
		{
			m_LogList.m_To = (DWORD)time.GetTime();
			SetTimer(LOGFILTER_TIMER, 10, NULL);
		}
	}
	catch (CAtlException)
	{
	}
	
	*pResult = 0;
}

void CLogDlg::OnDtnDatetimechangeDatefrom(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	CTime _time;
	m_DateFrom.GetTime(_time);
	try
	{
		CTime time(_time.GetYear(), _time.GetMonth(), _time.GetDay(), 0, 0, 0);
		if (time.GetTime() != m_LogList.m_From.GetTime())
		{
			m_LogList.m_From = (DWORD)time.GetTime();
			SetTimer(LOGFILTER_TIMER, 10, NULL);
		}
	}
	catch (CAtlException)
	{
	}
	
	*pResult = 0;
}



CTGitPathList CLogDlg::GetChangedPathsFromSelectedRevisions(bool bRelativePaths /* = false */, bool bUseFilter /* = true */)
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
			PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(nextpos));
			LogChangedPathArray * cpatharray = pLogEntry->pArChangedPaths;
			for (INT_PTR cpPathIndex = 0; cpPathIndex<cpatharray->GetCount(); ++cpPathIndex)
			{
				LogChangedPath * cpath = cpatharray->GetAt(cpPathIndex);
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

void CLogDlg::SortByColumn(int nSortColumn, bool bAscending)
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
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	const int nColumn = pNMLV->iSubItem;
	m_bAscending = nColumn == m_nSortColumn ? !m_bAscending : TRUE;
	m_nSortColumn = nColumn;
	SortByColumn(m_nSortColumn, m_bAscending);
	SetSortArrow(&m_LogList, m_nSortColumn, !!m_bAscending);
	SortShownListArray();
	m_LogList.Invalidate();
	UpdateLogInfoLabel();
	// the "next 100" button only makes sense if the log messages
	// are sorted by revision in descending order
	if ((m_nSortColumn)||(m_bAscending))
	{
		DialogEnableWindow(IDC_NEXTHUNDRED, false);
	}
	else
	{
		DialogEnableWindow(IDC_NEXTHUNDRED, true);
	}
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
void CLogDlg::OnLvnColumnclickChangedFileList(NMHDR *pNMHDR, LRESULT *pResult)
{
#if 0
	if (this->IsThreadRunning())
		return;		//no sorting while the arrays are filled
	if (m_currentChangedArray == NULL)
		return;
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	const int nColumn = pNMLV->iSubItem;
	m_bAscendingPathList = nColumn == m_nSortColumnPathList ? !m_bAscendingPathList : TRUE;
	m_nSortColumnPathList = nColumn;
//	qsort(m_currentChangedArray->GetData(), m_currentChangedArray->GetSize(), sizeof(LogChangedPath*), (GENERICCOMPAREFN)SortCompare);

	SetSortArrow(&m_ChangedFileListCtrl, m_nSortColumnPathList, m_bAscendingPathList);
	m_ChangedFileListCtrl.Invalidate();
	*pResult = 0;
#endif
}

int CLogDlg::m_nSortColumnPathList = 0;
bool CLogDlg::m_bAscendingPathList = false;

int CLogDlg::SortCompare(const void * pElem1, const void * pElem2)
{
#if 0
	LogChangedPath * cpath1 = *((LogChangedPath**)pElem1);
	LogChangedPath * cpath2 = *((LogChangedPath**)pElem2);

	if (m_bAscendingPathList)
		std::swap (cpath1, cpath2);

	int cmp = 0;
	switch (m_nSortColumnPathList)
	{
	case 0:	// action
			cmp = cpath2->GetAction().Compare(cpath1->GetAction());
			if (cmp)
				return cmp;
			// fall through
	case 1:	// path
			cmp = cpath2->sPath.CompareNoCase(cpath1->sPath);
			if (cmp)
				return cmp;
			// fall through
	case 2:	// copy from path
			cmp = cpath2->sCopyFromPath.Compare(cpath1->sCopyFromPath);
			if (cmp)
				return cmp;
			// fall through
	case 3:	// copy from revision
			return cpath2->lCopyFromRev > cpath1->lCopyFromRev;
	}
#endif
	return 0;
}

#if 0
void CLogDlg::ResizeAllListCtrlCols()
{

	const int nMinimumWidth = ICONITEMBORDER+16*4;
	int maxcol = ((CHeaderCtrl*)(m_LogList.GetDlgItem(0)))->GetItemCount()-1;
	int nItemCount = m_LogList.GetItemCount();
	TCHAR textbuf[MAX_PATH];
	CHeaderCtrl * pHdrCtrl = (CHeaderCtrl*)(m_LogList.GetDlgItem(0));
	if (pHdrCtrl)
	{
		for (int col = 0; col <= maxcol; col++)
		{
			HDITEM hdi = {0};
			hdi.mask = HDI_TEXT;
			hdi.pszText = textbuf;
			hdi.cchTextMax = sizeof(textbuf);
			pHdrCtrl->GetItem(col, &hdi);
			int cx = m_LogList.GetStringWidth(hdi.pszText)+20; // 20 pixels for col separator and margin
			for (int index = 0; index<nItemCount; ++index)
			{
				// get the width of the string and add 14 pixels for the column separator and margins
				int linewidth = m_LogList.GetStringWidth(m_LogList.GetItemText(index, col)) + 14;
				if (index < m_arShownList.GetCount())
				{
					GitRev * pCurLogEntry = reinterpret_cast<GitRev*>(m_arShownList.GetAt(index));
					if ((pCurLogEntry)&&(pCurLogEntry->m_CommitHash == m_wcRev.m_CommitHash))
					{
						// set the bold font and ask for the string width again
						m_LogList.SendMessage(WM_SETFONT, (WPARAM)m_boldFont, NULL);
						linewidth = m_LogList.GetStringWidth(m_LogList.GetItemText(index, col)) + 14;
						// restore the system font
						m_LogList.SendMessage(WM_SETFONT, NULL, NULL);
					}
				}
				if (index == 0)
				{
					// add the image size
					CImageList * pImgList = m_LogList.GetImageList(LVSIL_SMALL);
					if ((pImgList)&&(pImgList->GetImageCount()))
					{
						IMAGEINFO imginfo;
						pImgList->GetImageInfo(0, &imginfo);
						linewidth += (imginfo.rcImage.right - imginfo.rcImage.left);
						linewidth += 3;	// 3 pixels between icon and text
					}
				}
				if (cx < linewidth)
					cx = linewidth;
			}
			// Adjust columns "Actions" containing icons
			if (col == this->LOGLIST_ACTION)
			{
				if (cx < nMinimumWidth)
				{
					cx = nMinimumWidth;
				}
			}
			
			if (col == this->LOGLIST_MESSAGE)
			{
				if (cx > LOGLIST_MESSAGE_MAX)
				{
					cx = LOGLIST_MESSAGE_MAX;
				}

			}
			// keep the bug id column small
			if ((col == 4)&&(m_bShowBugtraqColumn))
			{
				if (cx > (int)(DWORD)m_regMaxBugIDColWidth)
				{
					cx = (int)(DWORD)m_regMaxBugIDColWidth;
				}
			}

			m_LogList.SetColumnWidth(col, cx);
		}
	}

}
#endif

void CLogDlg::OnBnClickedHidepaths()
{
	FillLogMessageCtrl();
	m_ChangedFileListCtrl.Invalidate();
}



void CLogDlg::OnBnClickedCheckStoponcopy()
{
#if 0
	if (!GetDlgItem(IDC_GETALL)->IsWindowEnabled())
		return;

	// ignore old fetch limits when switching
	// between copy-following and stop-on-copy
	// (otherwise stop-on-copy will limit what
	// we see immediately after switching to
	// copy-following)

	m_endrev = 0;

	// now, restart the query
#endif
	Refresh();
}

void CLogDlg::OnBnClickedIncludemerge()
{
#if 0
	m_endrev = 0;

	m_limit = 0;
#endif
	Refresh();
}

void CLogDlg::UpdateLogInfoLabel()
{

	git_revnum_t rev1 ;
	git_revnum_t rev2 ;
	long selectedrevs = 0;
	int count =m_LogList.m_arShownList.GetCount();
	if (count)
	{
		rev1 = (reinterpret_cast<GitRev*>(m_LogList.m_arShownList.GetAt(0)))->m_CommitHash;
		//pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(m_arShownList.GetCount()-1));
		rev2 =  (reinterpret_cast<GitRev*>(m_LogList.m_arShownList.GetAt(count-1)))->m_CommitHash;
		selectedrevs = m_LogList.GetSelectedCount();
	}
	CString sTemp;
	sTemp.Format(_T("Showing %ld revision(s), from revision %s to revision %s - %ld revision(s) selected"), count, rev2.Left(6), rev1.Left(6), selectedrevs);
	m_sLogInfo = sTemp;

	UpdateData(FALSE);
}

#if 0
void CLogDlg::ShowContextMenuForChangedpaths(CWnd* /*pWnd*/, CPoint point)
{

	int selIndex = m_ChangedFileListCtrl.GetSelectionMark();
	if ((point.x == -1) && (point.y == -1))
	{
		CRect rect;
		m_ChangedFileListCtrl.GetItemRect(selIndex, &rect, LVIR_LABEL);
		m_ChangedFileListCtrl.ClientToScreen(&rect);
		point = rect.CenterPoint();
	}
	if (selIndex < 0)
		return;
	int s = m_LogList.GetSelectionMark();
	if (s < 0)
		return;
	std::vector<CString> changedpaths;
	std::vector<LogChangedPath*> changedlogpaths;
	POSITION pos = m_LogList.GetFirstSelectedItemPosition();
	if (pos == NULL)
		return;	// nothing is selected, get out of here

	bool bOneRev = true;
	int sel=m_LogList.GetNextSelectedItem(pos);
	GitRev * pLogEntry = reinterpret_cast<GitRev *>(m_LogList.m_arShownList.GetAt(sel));
	GitRev * rev1 = pLogEntry;
	GitRev * rev2 = reinterpret_cast<GitRev *>(m_LogList.m_arShownList.GetAt(sel+1));
#if 0
	bool bOneRev = true;
	if (pos)
	{
		while (pos)
		{
			pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(m_LogList.GetNextSelectedItem(pos)));
			if (pLogEntry)
			{
				rev1 = max(rev1,(git_revnum_t)pLogEntry->Rev);
				rev2 = min(rev2,(git_revnum_t)pLogEntry->Rev);
				bOneRev = false;
			}				
		}
		if (!bOneRev)
			rev2--;
		POSITION pos = m_ChangedFileListCtrl.GetFirstSelectedItemPosition();
		while (pos)
		{
			int nItem = m_ChangedFileListCtrl.GetNextSelectedItem(pos);
			changedpaths.push_back(m_currentChangedPathList[nItem].GetGitPathString());
		}
	}
	else
	{
		// only one revision is selected in the log dialog top pane
		// but multiple items could be selected  in the changed items list
		rev2 = rev1-1;

		POSITION pos = m_ChangedFileListCtrl.GetFirstSelectedItemPosition();
		while (pos)
		{
			int nItem = m_ChangedFileListCtrl.GetNextSelectedItem(pos);
			LogChangedPath * changedlogpath = pLogEntry->pArChangedPaths->GetAt(nItem);

			if (m_ChangedFileListCtrl.GetSelectedCount() == 1)
			{
				if ((changedlogpath)&&(!changedlogpath->sCopyFromPath.IsEmpty()))
					rev2 = changedlogpath->lCopyFromRev;
				else
				{
					// if the path was modified but the parent path was 'added with history'
					// then we have to use the copy from revision of the parent path
					CTGitPath cpath = CTGitPath(changedlogpath->sPath);
					for (int flist = 0; flist < pLogEntry->pArChangedPaths->GetCount(); ++flist)
					{
						CTGitPath p = CTGitPath(pLogEntry->pArChangedPaths->GetAt(flist)->sPath);
						if (p.IsAncestorOf(cpath))
						{
							if (!pLogEntry->pArChangedPaths->GetAt(flist)->sCopyFromPath.IsEmpty())
								rev2 = pLogEntry->pArChangedPaths->GetAt(flist)->lCopyFromRev;
						}
					}
				}
			}
			if ((m_cHidePaths.GetState() & 0x0003)==BST_CHECKED)
			{
				// some items are hidden! So find out which item the user really clicked on
				INT_PTR selRealIndex = -1;
				for (INT_PTR hiddenindex=0; hiddenindex<pLogEntry->pArChangedPaths->GetCount(); ++hiddenindex)
				{
					if (pLogEntry->pArChangedPaths->GetAt(hiddenindex)->sPath.Left(m_sRelativeRoot.GetLength()).Compare(m_sRelativeRoot)==0)
						selRealIndex++;
					if (selRealIndex == nItem)
					{
						selIndex = hiddenindex;
						changedlogpath = pLogEntry->pArChangedPaths->GetAt(selIndex);
						break;
					}
				}
			}
			if (changedlogpath)
			{
				changedpaths.push_back(changedlogpath->sPath);
				changedlogpaths.push_back(changedlogpath);
			}
		}
	}
#endif
	//entry is selected, now show the popup menu
	CIconMenu popup;
	if (popup.CreatePopupMenu())
	{
		bool bEntryAdded = false;
		if (m_ChangedFileListCtrl.GetSelectedCount() == 1)
		{
//			if ((!bOneRev)||(IsDiffPossible(changedlogpaths[0], rev1)))
			{
				popup.AppendMenuIcon(CGitLogList::ID_DIFF, IDS_LOG_POPUP_DIFF, IDI_DIFF);
				popup.AppendMenuIcon(CGitLogList::ID_BLAMEDIFF, IDS_LOG_POPUP_BLAMEDIFF, IDI_BLAME);
				popup.SetDefaultItem(CGitLogList::ID_DIFF, FALSE);
				popup.AppendMenuIcon(CGitLogList::ID_GNUDIFF1, IDS_LOG_POPUP_GNUDIFF_CH, IDI_DIFF);
				bEntryAdded = true;
			}
//			if (rev2 == rev1-1)
			{
				if (bEntryAdded)
					popup.AppendMenu(MF_SEPARATOR, NULL);
				popup.AppendMenuIcon(CGitLogList::ID_OPEN, IDS_LOG_POPUP_OPEN, IDI_OPEN);
				popup.AppendMenuIcon(CGitLogList::ID_OPENWITH, IDS_LOG_POPUP_OPENWITH, IDI_OPEN);
				popup.AppendMenuIcon(CGitLogList::ID_BLAME, IDS_LOG_POPUP_BLAME, IDI_BLAME);
				popup.AppendMenu(MF_SEPARATOR, NULL);
				if (m_hasWC)
					popup.AppendMenuIcon(CGitLogList::ID_REVERTREV, IDS_LOG_POPUP_REVERTREV, IDI_REVERT);
				popup.AppendMenuIcon(CGitLogList::ID_POPPROPS, IDS_REPOBROWSE_SHOWPROP, IDI_PROPERTIES);			// "Show Properties"
				popup.AppendMenuIcon(CGitLogList::ID_LOG, IDS_MENULOG, IDI_LOG);						// "Show Log"				
				popup.AppendMenuIcon(CGitLogList::ID_GETMERGELOGS, IDS_LOG_POPUP_GETMERGELOGS, IDI_LOG);		// "Show merge log"
				popup.AppendMenuIcon(CGitLogList::ID_SAVEAS, IDS_LOG_POPUP_SAVE, IDI_SAVEAS);
				bEntryAdded = true;
				if (!m_ProjectProperties.sWebViewerPathRev.IsEmpty())
				{
					popup.AppendMenu(MF_SEPARATOR, NULL);
					popup.AppendMenuIcon(CGitLogList::ID_VIEWPATHREV, IDS_LOG_POPUP_VIEWPATHREV);
				}
				if (popup.GetDefaultItem(0,FALSE)==-1)
					popup.SetDefaultItem(CGitLogList::ID_OPEN, FALSE);
			}
		}
		else if (changedlogpaths.size())
		{
			// more than one entry is selected
			popup.AppendMenuIcon(CGitLogList::ID_SAVEAS, IDS_LOG_POPUP_SAVE);
			bEntryAdded = true;
		}

		if (!bEntryAdded)
			return;
		int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
		bool bOpenWith = false;
		bool bMergeLog = false;
		m_bCancelled = false;
		
		switch (cmd)
		{
		case CGitLogList::ID_DIFF:
			{
				DoDiffFromLog(selIndex, rev1, rev2, false, false);
			}
			break;
#if 0
		case ID_BLAMEDIFF:
			{
				DoDiffFromLog(selIndex, rev1, rev2, true, false);
			}
			break;
		case ID_GNUDIFF1:
			{
				DoDiffFromLog(selIndex, rev1, rev2, false, true);
			}
			break;
		case ID_REVERTREV:
			{
				SetPromptApp(&theApp);
				theApp.DoWaitCursor(1);
				CString sUrl;
				if (Git::PathIsURL(m_path))
				{
					sUrl = m_path.GetGitPathString();
				}
				else
				{
					sUrl = GetURLFromPath(m_path);
					if (sUrl.IsEmpty())
					{
						theApp.DoWaitCursor(-1);
						CString temp;
						temp.Format(IDS_ERR_NOURLOFFILE, m_path.GetWinPath());
						CMessageBox::Show(this->m_hWnd, temp, _T("TortoiseGit"), MB_ICONERROR);
						EnableOKButton();
						theApp.DoWaitCursor(-1);
						break;		//exit
					}
				}
				// find the working copy path of the selected item from the URL
				m_bCancelled = false;
				CString sUrlRoot = GetRepositoryRoot(CTGitPath(sUrl));

				CString fileURL = changedpaths[0];
				fileURL = sUrlRoot + fileURL.Trim();
				// firstfile = (e.g.) http://mydomain.com/repos/trunk/folder/file1
				// sUrl = http://mydomain.com/repos/trunk/folder
				CString sUnescapedUrl = CPathUtils::PathUnescape(sUrl);
				// find out until which char the urls are identical
				int i=0;
				while ((i<fileURL.GetLength())&&(i<sUnescapedUrl.GetLength())&&(fileURL[i]==sUnescapedUrl[i]))
					i++;
				int leftcount = m_path.GetWinPathString().GetLength()-(sUnescapedUrl.GetLength()-i);
				CString wcPath = m_path.GetWinPathString().Left(leftcount);
				wcPath += fileURL.Mid(i);
				wcPath.Replace('/', '\\');
				CGitProgressDlg dlg;
				if (changedlogpaths[0]->action == LOGACTIONS_DELETED)
				{
					// a deleted path! Since the path isn't there anymore, merge
					// won't work. So just do a copy url->wc
					dlg.SetCommand(CGitProgressDlg::GitProgress_Copy);
					dlg.SetPathList(CTGitPathList(CTGitPath(fileURL)));
					dlg.SetUrl(wcPath);
					dlg.SetRevision(rev2);
				}
				else
				{
					if (!PathFileExists(wcPath))
					{
						// seems the path got renamed
						// tell the user how to work around this.
						CMessageBox::Show(this->m_hWnd, IDS_LOG_REVERTREV_ERROR, IDS_APPNAME, MB_ICONERROR);
						EnableOKButton();
						theApp.DoWaitCursor(-1);
						break;		//exit
					}
					dlg.SetCommand(CGitProgressDlg::GitProgress_Merge);
					dlg.SetPathList(CTGitPathList(CTGitPath(wcPath)));
					dlg.SetUrl(fileURL);
					dlg.SetSecondUrl(fileURL);
					GitRevRangeArray revarray;
					revarray.AddRevRange(rev1, rev2);
					dlg.SetRevisionRanges(revarray);
				}
				CString msg;
				msg.Format(IDS_LOG_REVERT_CONFIRM, (LPCTSTR)wcPath);
				if (CMessageBox::Show(this->m_hWnd, msg, _T("TortoiseGit"), MB_YESNO | MB_ICONQUESTION) == IDYES)
				{
					dlg.DoModal();
				}
				theApp.DoWaitCursor(-1);
			}
			break;
		case ID_POPPROPS:
			{
				DialogEnableWindow(IDOK, FALSE);
				SetPromptApp(&theApp);
				theApp.DoWaitCursor(1);
				CString filepath;
				if (Git::PathIsURL(m_path))
				{
					filepath = m_path.GetGitPathString();
				}
				else
				{
					filepath = GetURLFromPath(m_path);
					if (filepath.IsEmpty())
					{
						theApp.DoWaitCursor(-1);
						CString temp;
						temp.Format(IDS_ERR_NOURLOFFILE, (LPCTSTR)filepath);
						CMessageBox::Show(this->m_hWnd, temp, _T("TortoiseGit"), MB_ICONERROR);
						TRACE(_T("could not retrieve the URL of the file!\n"));
						EnableOKButton();
						break;
					}
				}
				filepath = GetRepositoryRoot(CTGitPath(filepath));
				filepath += changedpaths[0];
				CPropDlg dlg;
				dlg.m_rev = rev1;
				dlg.m_Path = CTGitPath(filepath);
				dlg.DoModal();
				EnableOKButton();
				theApp.DoWaitCursor(-1);
			}
			break;
		case ID_SAVEAS:
			{
				DialogEnableWindow(IDOK, FALSE);
				SetPromptApp(&theApp);
				theApp.DoWaitCursor(1);
				CString filepath;
				if (Git::PathIsURL(m_path))
				{
					filepath = m_path.GetGitPathString();
				}
				else
				{
					filepath = GetURLFromPath(m_path);
					if (filepath.IsEmpty())
					{
						theApp.DoWaitCursor(-1);
						CString temp;
						temp.Format(IDS_ERR_NOURLOFFILE, (LPCTSTR)filepath);
						CMessageBox::Show(this->m_hWnd, temp, _T("TortoiseGit"), MB_ICONERROR);
						TRACE(_T("could not retrieve the URL of the file!\n"));
						EnableOKButton();
						break;
					}
				}
				m_bCancelled = false;
				CString sRoot = GetRepositoryRoot(CTGitPath(filepath));
				// if more than one entry is selected, we save them
				// one by one into a folder the user has selected
				bool bTargetSelected = false;
				CTGitPath TargetPath;
				if (m_ChangedFileListCtrl.GetSelectedCount() > 1)
				{
					CBrowseFolder browseFolder;
					browseFolder.SetInfo(CString(MAKEINTRESOURCE(IDS_LOG_SAVEFOLDERTOHINT)));
					browseFolder.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
					CString strSaveAsDirectory;
					if (browseFolder.Show(GetSafeHwnd(), strSaveAsDirectory) == CBrowseFolder::OK) 
					{
						TargetPath = CTGitPath(strSaveAsDirectory);
						bTargetSelected = true;
					}
				}
				else
				{
					// Display the Open dialog box. 
					CString revFilename;
					CString temp;
					temp = CPathUtils::GetFileNameFromPath(changedpaths[0]);
					int rfind = temp.ReverseFind('.');
					if (rfind > 0)
						revFilename.Format(_T("%s-%ld%s"), (LPCTSTR)temp.Left(rfind), rev1, (LPCTSTR)temp.Mid(rfind));
					else
						revFilename.Format(_T("%s-%ld"), (LPCTSTR)temp, rev1);
					bTargetSelected = CAppUtils::FileOpenSave(revFilename, NULL, IDS_LOG_POPUP_SAVE, IDS_COMMONFILEFILTER, false, m_hWnd);
					TargetPath.SetFromWin(revFilename);
				}
				if (bTargetSelected)
				{
					CProgressDlg progDlg;
					progDlg.SetTitle(IDS_APPNAME);
					progDlg.SetAnimation(IDR_DOWNLOAD);
					for (std::vector<LogChangedPath*>::iterator it = changedlogpaths.begin(); it!= changedlogpaths.end(); ++it)
					{
						GitRev getrev = ((*it)->action == LOGACTIONS_DELETED) ? rev2 : rev1;

						CString sInfoLine;
						sInfoLine.Format(IDS_PROGRESSGETFILEREVISION, (LPCTSTR)filepath, (LPCTSTR)getrev.ToString());
						progDlg.SetLine(1, sInfoLine, true);
						SetAndClearProgressInfo(&progDlg);
						progDlg.ShowModeless(m_hWnd);

						CTGitPath tempfile = TargetPath;
						if (changedpaths.size() > 1)
						{
							// if multiple items are selected, then the TargetPath
							// points to a folder and we have to append the filename
							// to save to to that folder.
							CString sName = (*it)->sPath;
							int slashpos = sName.ReverseFind('/');
							if (slashpos >= 0)
								sName = sName.Mid(slashpos);
							tempfile.AppendPathString(sName);
							// one problem here:
							// a user could have selected multiple items which
							// have the same filename but reside in different
							// directories, e.g.
							// /folder1/file1
							// /folder2/file1
							// in that case, the second 'file1' will overwrite
							// the already saved 'file1'.
							// 
							// we could maybe find the common root of all selected
							// items and then create sub folders to save those files
							// there.
							// But I think we should just leave it that way: to check
							// out multiple items at once, the better way is still to
							// use the export command from the top pane of the log dialog.
						}
						filepath = sRoot + (*it)->sPath;
						if (!Cat(CTGitPath(filepath), getrev, getrev, tempfile))
						{
							progDlg.Stop();
							SetAndClearProgressInfo((HWND)NULL);
							CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseGit"), MB_ICONERROR);
							EnableOKButton();
							theApp.DoWaitCursor(-1);
							break;
						}
					}
					progDlg.Stop();
					SetAndClearProgressInfo((HWND)NULL);
				}
				EnableOKButton();
				theApp.DoWaitCursor(-1);
			}
			break;
		case ID_OPENWITH:
			bOpenWith = true;
		case ID_OPEN:
			{
				GitRev getrev = pLogEntry->pArChangedPaths->GetAt(selIndex)->action == LOGACTIONS_DELETED ? rev2 : rev1;
				Open(bOpenWith,changedpaths[0],getrev);
			}
			break;
		case ID_BLAME:
			{
				CString filepath;
				if (Git::PathIsURL(m_path))
				{
					filepath = m_path.GetGitPathString();
				}
				else
				{
					filepath = GetURLFromPath(m_path);
					if (filepath.IsEmpty())
					{
						theApp.DoWaitCursor(-1);
						CString temp;
						temp.Format(IDS_ERR_NOURLOFFILE, (LPCTSTR)filepath);
						CMessageBox::Show(this->m_hWnd, temp, _T("TortoiseGit"), MB_ICONERROR);
						TRACE(_T("could not retrieve the URL of the file!\n"));
						EnableOKButton();
						break;
					}
				}
				filepath = GetRepositoryRoot(CTGitPath(filepath));
				filepath += changedpaths[0];
				CBlameDlg dlg;
				dlg.EndRev = rev1;
				if (dlg.DoModal() == IDOK)
				{
					CBlame blame;
					CString tempfile;
					CString logfile;
					tempfile = blame.BlameToTempFile(CTGitPath(filepath), dlg.StartRev, dlg.EndRev, dlg.EndRev, logfile, _T(""), dlg.m_bIncludeMerge, TRUE, TRUE);
					if (!tempfile.IsEmpty())
					{
						if (dlg.m_bTextView)
						{
							//open the default text editor for the result file
							CAppUtils::StartTextViewer(tempfile);
						}
						else
						{
							CString sParams = _T("/path:\"") + filepath + _T("\" ");
							if(!CAppUtils::LaunchTortoiseBlame(tempfile, logfile, CPathUtils::GetFileNameFromPath(filepath),sParams))
							{
								break;
							}
						}
					}
					else
					{
						CMessageBox::Show(this->m_hWnd, blame.GetLastErrorMessage(), _T("TortoiseGit"), MB_ICONERROR);
					}
				}
			}
			break;
		case ID_GETMERGELOGS:
			bMergeLog = true;
			// fall through
		case ID_LOG:
			{
				DialogEnableWindow(IDOK, FALSE);
				SetPromptApp(&theApp);
				theApp.DoWaitCursor(1);
				CString filepath;
				if (Git::PathIsURL(m_path))
				{
					filepath = m_path.GetGitPathString();
				}
				else
				{
					filepath = GetURLFromPath(m_path);
					if (filepath.IsEmpty())
					{
						theApp.DoWaitCursor(-1);
						CString temp;
						temp.Format(IDS_ERR_NOURLOFFILE, (LPCTSTR)filepath);
						CMessageBox::Show(this->m_hWnd, temp, _T("TortoiseGit"), MB_ICONERROR);
						TRACE(_T("could not retrieve the URL of the file!\n"));
						EnableOKButton();
						break;
					}
				}
				m_bCancelled = false;
				filepath = GetRepositoryRoot(CTGitPath(filepath));
				filepath += changedpaths[0];
				git_revnum_t logrev = rev1;
				if (changedlogpaths[0]->action == LOGACTIONS_DELETED)
				{
					// if the item got deleted in this revision,
					// fetch the log from the previous revision where it
					// still existed.
					logrev--;
				}
				CString sCmd;
				sCmd.Format(_T("\"%s\" /command:log /path:\"%s\" /startrev:%ld"), (LPCTSTR)(CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe")), (LPCTSTR)filepath, logrev);
				if (bMergeLog)
					sCmd += _T(" /merge");
				CAppUtils::LaunchApplication(sCmd, NULL, false);
				EnableOKButton();
				theApp.DoWaitCursor(-1);
			}
			break;
		case ID_VIEWPATHREV:
			{
				PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(m_LogList.GetSelectionMark()));
				GitRev rev = pLogEntry->Rev;
				CString relurl = changedpaths[0];
				CString url = m_ProjectProperties.sWebViewerPathRev;
				url.Replace(_T("%REVISION%"), rev.ToString());
				url.Replace(_T("%PATH%"), relurl);
				relurl = relurl.Mid(relurl.Find('/'));
				url.Replace(_T("%PATH1%"), relurl);
				if (!url.IsEmpty())
					ShellExecute(this->m_hWnd, _T("open"), url, NULL, NULL, SW_SHOWDEFAULT);					
			}
			break;
#endif
		default:
			break;
		} // switch (cmd)

	} // if (popup.CreatePopupMenu())
}
#endif

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
	if (GetDlgItem(IDC_GETALL)->IsWindowEnabled())
	{
		m_limit = 0;
		Refresh (true);
	}
}

void CLogDlg::OnFind()
{
	if (!m_pFindDialog)
	{
		m_pFindDialog = new CFindReplaceDialog();
		m_pFindDialog->Create(TRUE, NULL, NULL, FR_HIDEUPDOWN | FR_HIDEWHOLEWORD, this);									
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
		m_LogList.StartFilter();
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