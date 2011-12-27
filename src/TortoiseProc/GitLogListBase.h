// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2011 - TortoiseGit

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
// GitLogList.cpp : implementation file
//
#pragma once

#include "HintListCtrl.h"
#include "CommonResource.h"
#include "Git.h"
#include "ProjectProperties.h"
#include "TGitPath.h"
#include "registry.h"
#include "SplitterControl.h"
#include "Colors.h"
#include "MenuButton.h"
#include "LogDlgHelper.h"
#include "FilterEdit.h"
#include "GitRev.h"
#include "Tooltip.h"
//#include "GitLogList.h"
#include "lanes.h"
#include "GitLogCache.h"
#include <regex>
#include <GitStatusListCtrl.h>
#include "FindDlg.h"

// CGitLogList
#if (NTDDI_VERSION < NTDDI_LONGHORN)

enum LISTITEMSTATES_MINE {
	LISS_NORMAL = 1,
	LISS_HOT = 2,
	LISS_SELECTED = 3,
	LISS_DISABLED = 4,
	LISS_SELECTEDNOTFOCUS = 5,
	LISS_HOTSELECTED = 6,
};

// these defines must be in the order the columns are inserted!


#define MCS_NOTRAILINGDATES  0x0040
#define MCS_SHORTDAYSOFWEEK  0x0080
#define MCS_NOSELCHANGEONNAV 0x0100

#define DTM_SETMCSTYLE    (DTM_FIRST + 11)

#endif

#define ICONITEMBORDER 5

#define GITLOG_START 0
#define GITLOG_START_ALL 1
#define GITLOG_END   100

#define LOGFILTER_ALL		1
#define LOGFILTER_MESSAGES	2
#define LOGFILTER_PATHS		3
#define LOGFILTER_AUTHORS	4
#define LOGFILTER_REVS		5
#define LOGFILTER_REGEX		6
#define LOGFILTER_BUGID		7
#define LOGFILTER_SUBJECT	8

//typedef void CALLBACK_PROCESS(void * data, int progress);
#define MSG_LOADED				(WM_USER+110)
#define MSG_LOAD_PERCENTAGE		(WM_USER+111)
#define MSG_REFLOG_CHANGED		(WM_USER+112)
#define MSG_FETCHED_DIFF		(WM_USER+113)

class CThreadSafePtrArray: public CPtrArray
{
	CComCriticalSection *m_critSec;
public:
	CThreadSafePtrArray(CComCriticalSection *section){ m_critSec = section ;}
	void * SafeGetAt(INT_PTR i)
	{
		if(m_critSec)
			m_critSec->Lock();

		if( i<0 || i>=GetCount())
		{
			if(m_critSec)
				m_critSec->Unlock();

			return NULL;
		}

		if(m_critSec)
			m_critSec->Unlock();

		return GetAt(i);
	}
	INT_PTR SafeAdd(void *newElement)
	{
		INT_PTR ret;
		if(m_critSec)
			m_critSec->Lock();
		ret = Add(newElement);
		if(m_critSec)
			m_critSec->Unlock();
		return ret;
	}

	void  SafeRemoveAll()
	{
		if(m_critSec)
			m_critSec->Lock();
		RemoveAll();
		if(m_critSec)
			m_critSec->Unlock();
	}

};

class CGitLogListBase : public CHintListCtrl
{
	DECLARE_DYNAMIC(CGitLogListBase)

public:
	CGitLogListBase();
	virtual ~CGitLogListBase();
	ProjectProperties	m_ProjectProperties;

	CFilterData m_Filter;

	void UpdateProjectProperties()
	{
		m_ProjectProperties.ReadProps(this->m_Path);

		if ((!m_ProjectProperties.sUrl.IsEmpty())||(!m_ProjectProperties.sCheckRe.IsEmpty()))
			m_bShowBugtraqColumn = true;
		else
			m_bShowBugtraqColumn = false;
	}

	void ResetWcRev()
	{
		m_wcRev.GetBody()=_T("Fetching Status...");
		m_wcRev.m_CallDiffAsync = DiffAsync;
		InterlockedExchange(&m_wcRev.m_IsDiffFiles, FALSE);
	}
	void SetProjectPropertiesPath(const CTGitPath& path) {m_ProjectProperties.ReadProps(path);}

	volatile LONG		m_bNoDispUpdates;
	BOOL m_IsIDReplaceAction;
	BOOL m_IsOldFirst;
	void hideFromContextMenu(unsigned __int64 hideMask, bool exclusivelyShow);
	BOOL m_IsRebaseReplaceGraph;

	void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);

	BOOL m_bStrictStopped;
	BOOL m_bShowBugtraqColumn;
	BOOL m_bSearchIndex;
	BOOL m_bCancelled;
	unsigned __int64 m_ContextMenuMask;

	bool				m_hasWC;
	bool				m_bShowWC;
	GitRev				m_wcRev;
	volatile LONG 		m_bThreadRunning;
	CLogCache			m_LogCache;

	CString m_startrev;
	CString m_endrev;

	// don't forget to bump BLAME_COLUMN_VERSION in GitStatusListCtrlHelpers.cpp if you change columns
	enum
	{
		LOGLIST_GRAPH,
		LOGLIST_REBASE,
		LOGLIST_ID,
		LOGLIST_HASH,
		LOGLIST_ACTION,
		LOGLIST_MESSAGE,
		LOGLIST_AUTHOR,
		LOGLIST_DATE,
		LOGLIST_EMAIL,
		LOGLIST_COMMIT_NAME,
		LOGLIST_COMMIT_EMAIL,
		LOGLIST_COMMIT_DATE,
		LOGLIST_BUG,
		LOGLIST_MESSAGE_MAX=300,
		LOGLIST_MESSAGE_MIN=200,

		GIT_LOG_GRAPH	=		1<< LOGLIST_GRAPH,
		GIT_LOG_REBASE	=		1<< LOGLIST_REBASE,
		GIT_LOG_ID		=		1<< LOGLIST_ID,
		GIT_LOG_HASH	=		1<< LOGLIST_HASH,
		GIT_LOG_ACTIONS	=		1<< LOGLIST_ACTION,
		GIT_LOG_MESSAGE	=		1<< LOGLIST_MESSAGE,
		GIT_LOG_AUTHOR	=		1<< LOGLIST_AUTHOR,
		GIT_LOG_DATE	=		1<< LOGLIST_DATE,
		GIT_LOG_EMAIL	=		1<< LOGLIST_EMAIL,
		GIT_LOG_COMMIT_NAME	=	1<< LOGLIST_COMMIT_NAME,
		GIT_LOG_COMMIT_EMAIL=	1<< LOGLIST_COMMIT_EMAIL,
		GIT_LOG_COMMIT_DATE	=	1<< LOGLIST_COMMIT_DATE,
		GIT_LOGLIST_BUG		=	1<< LOGLIST_BUG,
	};

	enum
	{
	// needs to start with 1, since 0 is the return value if *nothing* is clicked on in the context menu
	ID_COMPARE = 1, // compare revision with WC
	ID_SAVEAS,
	ID_COMPARETWO, // compare two revisions
	ID_UPDATE,
	ID_COPY,
	ID_REVERTREV,
	ID_MERGEREV,
	ID_GNUDIFF1, // compare with WC, unified
	ID_GNUDIFF2, // compare two revisions, unified
	ID_FINDENTRY,
	ID_OPEN,
	ID_BLAME,
	ID_REPOBROWSE,
	ID_LOG,
	ID_POPPROPS,
	ID_EDITNOTE,
	ID_EDITLOG,
	ID_DIFF,
	ID_OPENWITH,
	ID_COPYCLIPBOARD,
	ID_COPYHASH,
	ID_REVERTTOREV,
	ID_BLAMECOMPARE,
	ID_BLAMETWO,
	ID_BLAMEDIFF,
	ID_VIEWREV,
	ID_VIEWPATHREV,
	ID_EXPORT,
	ID_COMPAREWITHPREVIOUS,
	ID_BLAMEWITHPREVIOUS,
	ID_GETMERGELOGS,
	ID_REVPROPS,
	ID_CHERRY_PICK,
	ID_CREATE_BRANCH,
	ID_CREATE_TAG,
	ID_SWITCHTOREV,
	ID_SWITCHBRANCH,
	ID_RESET,
	ID_REBASE_PICK,
	ID_REBASE_EDIT,
	ID_REBASE_SQUASH,
	ID_REBASE_SKIP,
	ID_COMBINE_COMMIT,
	ID_STASH_APPLY,
	ID_REFLOG_DEL,
	ID_REBASE_TO_VERSION,
	ID_CREATE_PATCH,
	ID_DELETE,
	ID_COMMIT,
	ID_PUSH,
	};
	inline unsigned __int64 GetContextMenuBit(int i){ return ((unsigned __int64 )0x1)<<i ;}
	void InsertGitColumn();
	void ResizeAllListCtrlCols();
	void CopySelectionToClipBoard(bool hashonly=FALSE);
	void DiffSelectedRevWithPrevious();
	bool IsSelectionContinuous();
	int  BeginFetchLog();
	int  FillGitLog(CTGitPath *path,int infomask=CGit::	LOG_INFO_STAT| CGit::LOG_INFO_FILESTATE | CGit::LOG_INFO_SHOW_MERGEDFILE,CString *from=NULL,CString *to=NULL);
	BOOL IsMatchFilter(bool bRegex, GitRev *pRev, tr1::wregex &pat);

	CFindDlg *m_pFindDialog;
	static const UINT	m_FindDialogMessage;
	void OnFind();

	inline int ShownCountWithStopped() const { return (int)m_arShownList.GetCount() + (m_bStrictStopped ? 1 : 0); }
	int FetchLogAsync(void * data=NULL);
	CThreadSafePtrArray			m_arShownList;
	void Refresh(BOOL IsCleanFilter=TRUE);
	void RecalculateShownList(CThreadSafePtrArray * pShownlist);
	void Clear();

	int					m_nSelectedFilter;
	bool				m_bFilterWithRegex;
	CLogDataVector		m_logEntries;
	void RemoveFilter();
	void StartFilter();
	bool ValidateRegexp(LPCTSTR regexp_str, tr1::wregex& pat, bool bMatchCase = false );
	CString				m_sFilterText;

	__time64_t			m_From;
	__time64_t			m_To;

	CTGitPath			m_Path;
	int					m_ShowMask;
	CGitHash			m_lastSelectedHash;

	void				GetTimeRange(CTime &oldest,CTime &latest);
	virtual void ContextMenuAction(int cmd,int FirstSelect, int LastSelect, CMenu * menu)=0;
	void ReloadHashMap()
	{
		m_HashMap.clear();
		g_Git.GetMapHashToFriendName(m_HashMap);
		m_CurrentBranch=g_Git.GetCurrentBranch();
		this->m_HeadHash=g_Git.GetHash(_T("HEAD"));
		m_wcRev.m_ParentHash.clear();
		m_wcRev.m_ParentHash.push_back(m_HeadHash);
	}
	void SafeTerminateThread()
	{
		if(m_LoadingThread!=NULL)
		{
			InterlockedExchange(&m_bExitThread,TRUE);
			DWORD ret =::WaitForSingleObject(m_LoadingThread->m_hThread,20000);
			if(ret == WAIT_TIMEOUT)
				::TerminateThread(m_LoadingThread,0);
			m_LoadingThread = NULL;
		}
	};

	bool IsInWorkingThread()
	{
		return (AfxGetThread() == m_LoadingThread);
	}

	void SetStartRef(const CString& StartRef)
	{
		m_StartRef=StartRef;
	}

	CString GetStartRef() const {return m_StartRef;}

	int					m_nSearchIndex;

	volatile LONG		m_bExitThread;
	CWinThread*			m_LoadingThread;
	MAP_HASH_NAME		m_HashMap;

public:
	CString				m_ColumnRegKey;

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnDestroy();
	virtual afx_msg void OnNMCustomdrawLoglist(NMHDR *pNMHDR, LRESULT *pResult);
	virtual afx_msg void OnLvnGetdispinfoLoglist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg LRESULT OnFindDialogMessage(WPARAM wParam, LPARAM lParam);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg LRESULT OnLoad(WPARAM wParam, LPARAM lParam);
	afx_msg void OnHdnBegintrack(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnHdnItemchanging(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnColumnResized(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnColumnMoved(NMHDR *pNMHDR, LRESULT *pResult);
	void OnNMDblclkLoglist(NMHDR * /*pNMHDR*/, LRESULT *pResult);
	afx_msg void OnLvnOdfinditemLoglist(NMHDR *pNMHDR, LRESULT *pResult);
	void PreSubclassWindow();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	static UINT LogThreadEntry(LPVOID pVoid);
	UINT LogThread();
	void FetchLastLogInfo();
	void FetchFullLogInfo(CString &from, CString &to);
	void FillBackGround(HDC hdc, int Index,CRect &rect);
	void DrawTagBranch(HDC,CRect &rect,INT_PTR index);
	void DrawGraph(HDC,CRect &rect,INT_PTR index);

	BOOL GetShortName(CString ref, CString &shortname,CString prefix);
	void paintGraphLane(HDC hdc,int laneHeight, int type, int x1, int x2,
									  const COLORREF& col,const COLORREF& activeColor, int top) ;
	void DrawLine(HDC hdc, int x1, int y1, int x2, int y2){::MoveToEx(hdc,x1,y1,NULL);::LineTo(hdc,x2,y2);}
	/**
	* Save column widths to the registry
	*/
	void SaveColumnWidths();	// save col widths to the registry

	BOOL IsEntryInDateRange(int i);

	int GetHeadIndex();

	std::vector<GitRev*> m_AsynDiffList;
	CComCriticalSection m_AsynDiffListLock;
	HANDLE	m_AsyncDiffEvent;
	volatile LONG m_AsyncThreadExit;
	CWinThread*			m_DiffingThread;

	static int DiffAsync(GitRev *rev, void *data)
	{
		ULONGLONG offset=((CGitLogListBase*)data)->m_LogCache.GetOffset(rev->m_CommitHash);
		if(!offset)
		{
			((CGitLogListBase*)data)->m_AsynDiffListLock.Lock();
			((CGitLogListBase*)data)->m_AsynDiffList.push_back(rev);
			((CGitLogListBase*)data)->m_AsynDiffListLock.Unlock();
			::SetEvent(((CGitLogListBase*)data)->m_AsyncDiffEvent);
		}
		else
		{
			if(((CGitLogListBase*)data)->m_LogCache.LoadOneItem(*rev,offset))
			{
				((CGitLogListBase*)data)->m_AsynDiffListLock.Lock();
				((CGitLogListBase*)data)->m_AsynDiffList.push_back(rev);
				((CGitLogListBase*)data)->m_AsynDiffListLock.Unlock();
				::SetEvent(((CGitLogListBase*)data)->m_AsyncDiffEvent);
			}
			InterlockedExchange(&rev->m_IsDiffFiles, TRUE);
			if(rev->m_IsDiffFiles && rev->m_IsCommitParsed)
				InterlockedExchange(&rev->m_IsFull, TRUE);
		}
		return 0;
	}

	static UINT AsyncThread(LPVOID data)
	{
		return ((CGitLogListBase*)data)->AsyncDiffThread();
	}

	int AsyncDiffThread();
	bool m_AsyncThreadExited;

public:
	void SafeTerminateAsyncDiffThread()
	{
		if(m_DiffingThread!=NULL && m_AsyncThreadExit != TRUE)
		{
			m_AsyncThreadExit = TRUE;
			::SetEvent(m_AsyncDiffEvent);
			DWORD ret = WAIT_TIMEOUT;
			// do not block here, but process messages and ask until the thread ends
			while (ret == WAIT_TIMEOUT && !m_AsyncThreadExited)
			{
				AfxGetThread()->PumpMessage(); // process messages, so that GetTopIndex and so on in the thread work
				ret = ::WaitForSingleObject(m_DiffingThread->m_hThread, 100);
			}
			m_DiffingThread = NULL;
		}
	};

protected:
	CComCriticalSection	m_critSec;

	HICON				m_hModifiedIcon;
	HICON				m_hReplacedIcon;
	HICON				m_hAddedIcon;
	HICON				m_hDeletedIcon;
	HICON				m_hFetchIcon;

	HFONT				m_boldFont;

	CRegDWORD			m_regMaxBugIDColWidth;

	void				*m_ProcData;
	CStoreSelection*	m_pStoreSelection;

	CColors				m_Colors;

	CString				m_CurrentBranch;
	CGitHash			m_HeadHash;

	CString				m_StartRef; //Ref of the top-commit

	COLORREF			m_LineColors[Lanes::COLORS_NUM];
	DWORD				m_DateFormat;	// DATE_SHORTDATE or DATE_LONGDATE
	bool				m_bRelativeTimes;	// Show relative times
	GIT_LOG				m_DllGitLog;

	ColumnManager		m_ColumnManager;
	DWORD				m_dwDefaultColumns;
};


