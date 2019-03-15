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
// GitLogList.cpp : implementation file
//
#pragma once

#include "HintCtrl.h"
#include "ResizableColumnsListCtrl.h"
#include "Git.h"
#include "ProjectProperties.h"
#include "TGitPath.h"
#include "registry.h"
#include "Colors.h"
#include "LogDlgHelper.h"
#include "GitRevLoglist.h"
#include "lanes.h"
#include "GitLogCache.h"
#include <regex>
#include "GitStatusListCtrl.h"
#include "FindDlg.h"
#include <unordered_set>
#include "LogDlgFilter.h"

typedef CComCritSecLock<CComCriticalSection> Locker;

template < typename Cont, typename Pred>
void for_each(Cont& c, Pred&& p)
{
	std::for_each(cbegin(c), cend(c), p);
}

template <typename Cont, typename Pred>
auto find_if(Cont& c, Pred&& p)
{
	return std::find_if(cbegin(c), cend(c), p);
}

// CGitLogList
#define ICONITEMBORDER 5

#define GITLOG_START 0
#define GITLOG_START_ALL 1
#define GITLOG_END   100

#define LOGFILTER_ALL			0xFFFF
#define LOGFILTER_TOGGLE		0x8000
#define LOGFILTER_MESSAGES		0x0001
#define LOGFILTER_PATHS			0x0002
#define LOGFILTER_AUTHORS		0x0004
#define LOGFILTER_REVS			0x0008
#define LOGFILTER_REGEX			0x0010
#define LOGFILTER_BUGID			0x0020
#define LOGFILTER_SUBJECT		0x0040
#define LOGFILTER_REFNAME		0x0080
#define LOGFILTER_EMAILS		0x0100
#define LOGFILTER_NOTES			0x0200
#define LOGFILTER_ANNOTATEDTAG	0x0400
#define LOGFILTER_CASE			0x0800

#define LOGLIST_SHOWNOTHING				0x0000
#define LOGLIST_SHOWLOCALBRANCHES		0x0001
#define LOGLIST_SHOWREMOTEBRANCHES		0x0002
#define LOGLIST_SHOWTAGS				0x0004
#define LOGLIST_SHOWSTASH				0x0008
#define LOGLIST_SHOWBISECT				0x0010
#define LOGLIST_SHOWOTHERREFS			0x0020
#define LOGLIST_SHOWALLREFS				0xFFFF

//typedef void CALLBACK_PROCESS(void * data, int progress);
#define MSG_LOADED				(WM_USER+110)
#define MSG_LOAD_PERCENTAGE		(WM_USER+111)
#define MSG_REFLOG_CHANGED		(WM_USER+112)
#define MSG_FETCHED_DIFF		(WM_USER+113)
#define MSG_COMMITS_REORDERED	(WM_USER+114)

class SelectionHistory
{
#define HISTORYLENGTH 50
public:
	SelectionHistory(void)
	: location(0)
	{
		lastselected.reserve(HISTORYLENGTH);
	}
	void Add(CGitHash &hash)
	{
		if (hash.IsEmpty())
			return;

		size_t size = lastselected.size();

		// re-select last selected commit
		if (size > 0 && hash == lastselected[size - 1])
		{
			// reset location
			if (location != size - 1)
				location = size - 1;
			return;
		}

		// go back and some commit was highlight
		if (size > 0 && location != size - 1)
		{
			// Re-select current one, it may be a forked point.
			if (hash == lastselected[location])
				// Discard it later.
				// That is that discarding forward history when a forked entry is really coming.
				// And user has the chance to Go Forward again in this situation.
				// IOW, (hash != lastselected[location]) means user wants a forked history,
				// and this change saves one step from old behavior.
				return;

			// Discard forward history if any
			while (lastselected.size() - 1 > location)
				lastselected.pop_back();
		}

		if (lastselected.size() >= HISTORYLENGTH)
			lastselected.erase(lastselected.cbegin());

		lastselected.push_back(hash);
		location = lastselected.size() - 1;
	}
	BOOL GoBack(CGitHash& historyEntry)
	{
		if (location < 1)
			return FALSE;

		historyEntry = lastselected[--location];

		return TRUE;
	}
	BOOL GoForward(CGitHash& historyEntry)
	{
		if (location >= lastselected.size() - 1)
			return FALSE;

		historyEntry = lastselected[++location];

		return TRUE;
	}
private:
	std::vector<CGitHash> lastselected;
	size_t location;
};

class CThreadSafePtrArray : public std::vector<GitRevLoglist*>
{
	CComCriticalSection *m_critSec;
public:
	CThreadSafePtrArray(CComCriticalSection* section) : m_critSec(section)
	{
		ATLASSERT(m_critSec);
	}

	GitRevLoglist* SafeGetAt(size_t i)
	{
		Locker lock(*m_critSec);
		if (i >= size())
			return nullptr;

		return (*this)[i];
	}

	void SafeAdd(GitRevLoglist* newElement)
	{
		Locker lock(*m_critSec);
		push_back(newElement);
	}

	void SafeRemoveAt(size_t i)
	{
		Locker lock(*m_critSec);
		if (i >= size())
			return;

		erase(begin() + i);
	}

	void SafeAddFront(GitRevLoglist* newElement)
	{
		Locker lock(*m_critSec);
		insert(cbegin(), newElement);
	}

	void  SafeRemoveAll()
	{
		Locker lock(*m_critSec);
		clear();
	}
};

class IAsyncDiffCB
{
};

class CGitLogListBase : public CHintCtrl<CResizableColumnsListCtrl<CListCtrl>>, public IAsyncDiffCB
{
	DECLARE_DYNAMIC(CGitLogListBase)

public:
	CGitLogListBase();
	virtual ~CGitLogListBase();
	ProjectProperties	m_ProjectProperties;

	void UpdateProjectProperties()
	{
		m_ProjectProperties.ReadProps();

		if ((!m_ProjectProperties.sUrl.IsEmpty())||(!m_ProjectProperties.sCheckRe.IsEmpty()))
			m_bShowBugtraqColumn = true;
		else
			m_bShowBugtraqColumn = false;
	}

	void ResetWcRev(bool refresh = false)
	{
		if (GetSafeHwnd())
		{
			CWnd* pParent = GetParent();
			if (pParent && pParent->GetSafeHwnd())
				pParent->SendMessage(LOGLIST_RESET_WCREV);
		}
		m_wcRev.Clear();
		m_wcRev.GetSubject().LoadString(IDS_LOG_WORKINGDIRCHANGES);
		m_wcRev.m_Mark = L'-';
		m_wcRev.GetBody().LoadString(IDS_LOG_FETCHINGSTATUS);
		m_wcRev.GetBody() = L'\n' + m_wcRev.GetBody();
		m_wcRev.m_CallDiffAsync = DiffAsync;
		InterlockedExchange(&m_wcRev.m_IsDiffFiles, FALSE);
		if (refresh && m_bShowWC)
			m_arShownList[0] = &m_wcRev;
	}

	volatile LONG		m_bNoDispUpdates;
	BOOL m_IsIDReplaceAction;
	BOOL m_IsOldFirst;
protected:
	void hideFromContextMenu(unsigned __int64 hideMask, bool exclusivelyShow);
public:
	BOOL m_IsRebaseReplaceGraph;
	BOOL m_bNoHightlightHead;

protected:
	void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);

public:
	BOOL m_bShowBugtraqColumn;
protected:
	BOOL m_bSearchIndex;
public:
	bool m_bIsCherryPick;
	unsigned __int64 m_ContextMenuMask;

	bool				m_hasWC;
	bool				m_bShowWC;
protected:
	GitRevLoglist		m_wcRev;
public:
	volatile LONG 		m_bThreadRunning;
protected:
	CLogCache			m_LogCache;

public:
	CString	m_sRange;
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
		LOGLIST_SVNREV,
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
		GIT_LOGLIST_SVNREV	=	1<< LOGLIST_SVNREV,
	};

	enum
	{
	// needs to start with 1, since 0 is the return value if *nothing* is clicked on in the context menu
	ID_COMPARE = 1, // compare revision with WC
	ID_SAVEAS,
	ID_COMPARETWO, // compare two revisions
	ID_REVERTREV,
	ID_MERGEREV,
	ID_GNUDIFF1, // compare with WC, unified
	ID_GNUDIFF2, // compare two revisions, unified
	ID_FINDENTRY,
	ID_OPEN,
	ID_BLAME,
	ID_REPOBROWSE,
	ID_LOG,
	ID_EDITNOTE,
	ID_DIFF,
	ID_OPENWITH,
	ID_COPYCLIPBOARD,
	ID_REVERTTOREV,
	ID_BLAMECOMPARE,
	ID_BLAMEDIFF,
	ID_VIEWREV,
	ID_VIEWPATHREV,
	ID_EXPORT,
	ID_COMPAREWITHPREVIOUS,
	ID_BLAMEPREVIOUS,
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
	ID_STASH_SAVE,
	ID_STASH_LIST,
	ID_STASH_POP,
	ID_REFLOG_STASH_APPLY,
	ID_REFLOG_DEL,
	ID_REBASE_TO_VERSION,
	ID_CREATE_PATCH,
	ID_DELETE,
	ID_COMMIT,
	ID_PUSH,
	ID_PULL,
	ID_FETCH,
	ID_SHOWBRANCHES,
	ID_BISECTSTART,
	ID_LOG_VIEWRANGE,
	ID_LOG_VIEWRANGE_REACHABLEFROMONLYONE,
	ID_MERGE_ABORT,
	ID_CLEANUP,
	ID_SUBMODULE_UPDATE,
	ID_BISECTGOOD,
	ID_BISECTBAD,
	ID_BISECTRESET,
	ID_BISECTSKIP,
	ID_SVNDCOMMIT,
	ID_COMPARETWOCOMMITCHANGES,
	// the following can be >= 64 as those are not used for GetContextMenuBit(), all others must be < 64 in order to fit into __int64
	ID_COPYCLIPBOARDFULL,
	ID_COPYCLIPBOARDFULLNOPATHS,
	ID_COPYCLIPBOARDHASH,
	ID_COPYCLIPBOARDAUTHORSFULL,
	ID_COPYCLIPBOARDAUTHORSNAME,
	ID_COPYCLIPBOARDAUTHORSEMAIL,
	ID_COPYCLIPBOARDSUBJECTS,
	ID_COPYCLIPBOARDMESSAGES,
	ID_COPYCLIPBOARDBRANCHTAG,
	};
	enum FilterShow
	{
		FILTERSHOW_REFS = 1,
		FILTERSHOW_MERGEPOINTS = 2,
		FILTERSHOW_ANYCOMMIT = 4,
		FILTERSHOW_ALL = FILTERSHOW_ANYCOMMIT | FILTERSHOW_REFS | FILTERSHOW_MERGEPOINTS
	};
	enum : unsigned int
	{
		// For Rebase only
		LOGACTIONS_REBASE_CURRENT	= 0x08000000,
		LOGACTIONS_REBASE_PICK		= 0x04000000,
		LOGACTIONS_REBASE_SQUASH	= 0x02000000,
		LOGACTIONS_REBASE_EDIT		= 0x01000000,
		LOGACTIONS_REBASE_DONE		= 0x00800000,
		LOGACTIONS_REBASE_SKIP		= 0x00400000,
		LOGACTIONS_REBASE_MASK		= 0x0FC00000,
		LOGACTIONS_REBASE_MODE_MASK	= 0x07C00000,
	};
	static_assert(ID_COMPARETWOCOMMITCHANGES < 64 && ID_COPYCLIPBOARDFULL <= 64, "IDs must be <64 in order to be usable in a bitfield");
	static inline unsigned __int64 GetContextMenuBit(int i) { return unsigned __int64(0x1) << i; }
	static CString GetRebaseActionName(int action);
	void InsertGitColumn();
	void CopySelectionToClipBoard(int toCopy = ID_COPYCLIPBOARDFULL);
	void DiffSelectedRevWithPrevious();
	bool IsSelectionContinuous();
protected:
	int  BeginFetchLog();
	HWND GetParentHWND();
public:
	int  FillGitLog(CTGitPath* path, CString* range = nullptr, int infomask = CGit::LOG_INFO_STAT | CGit::LOG_INFO_FILESTATE | CGit::LOG_INFO_SHOW_MERGEDFILE);
	int  FillGitLog(std::unordered_set<CGitHash>& hashes);
protected:
	CString MessageDisplayStr(GitRev* pLogEntry);
	bool ShouldShowFilter(GitRevLoglist* pRev, const std::unordered_map<CGitHash, std::unordered_set<CGitHash>>& commitChildren, MAP_HASH_NAME& hashMap);
public:
	void ShowGraphColumn(bool bShow);
	CString	GetTagInfo(GitRev* pLogEntry) const;
	CString GetTagInfo(const STRING_VECTOR& refs) const;

	CFindDlg *m_pFindDialog;
	static const UINT	m_FindDialogMessage;
	void OnFind();

protected:
	static const UINT	m_ScrollToMessage;
public:
	static const UINT	m_ScrollToRef;
	static const UINT	m_RebaseActionMessage;
	static const UINT	LOGLIST_RESET_WCREV;

	void FetchLogAsync(void* data = nullptr);
	CThreadSafePtrArray			m_arShownList;
	void Refresh(BOOL IsCleanFilter=TRUE);
	void Clear();

	FilterShow			m_ShowFilter;
	CLogDataVector		m_logEntries;

	std::shared_ptr<CLogDlgFilter> m_LogFilter;
	CFilterData			m_Filter;

	CTGitPath			m_Path;
	int					m_ShowMask;
	CGitHash			m_lastSelectedHash;
	SelectionHistory	m_selectionHistory;
	CGitHash			m_highlight;
	int					m_ShowRefMask;

protected:
	CGitHash			m_superProjectHash;

public:
	void				GetTimeRange(CTime &oldest,CTime &latest);
protected:
	virtual void GetParentHashes(GitRev* pRev, GIT_REV_LIST& parentHash);
	virtual void ContextMenuAction(int cmd, int FirstSelect, int LastSelect, CMenu* menu, MAP_HASH_NAME& hashMap) = 0;
	void UpdateSubmodulePointer()
	{
		m_superProjectHash.Empty();
		if (CRegDWORD(L"Software\\TortoiseGit\\LogShowSuperProjectSubmodulePointer", TRUE) != TRUE)
			return;
		if (GitAdminDir::IsBareRepo(g_Git.m_CurrentDir))
			return;
		CString superprojectRoot;
		GitAdminDir::HasAdminDir(g_Git.m_CurrentDir, false, &superprojectRoot);
		if (superprojectRoot.IsEmpty())
			return;

		CAutoRepository repo(superprojectRoot);
		if (!repo)
			return;
		CAutoIndex index;
		if (git_repository_index(index.GetPointer(), repo))
			return;

		CString submodulePath;
		if (superprojectRoot[superprojectRoot.GetLength() - 1] == L'\\')
			submodulePath = g_Git.m_CurrentDir.Right(g_Git.m_CurrentDir.GetLength() - superprojectRoot.GetLength());
		else
			submodulePath = g_Git.m_CurrentDir.Right(g_Git.m_CurrentDir.GetLength() - superprojectRoot.GetLength() - 1);
		submodulePath.Replace(L'\\', L'/');
		const git_index_entry* entry = git_index_get_bypath(index, CUnicodeUtils::GetUTF8(submodulePath), 0);
		if (!entry)
			return;

		m_superProjectHash = entry->id;
	}
	void ReloadHashMap()
	{
		m_RefLabelPosMap.clear();
		auto newHashMap = std::make_shared<MAP_HASH_NAME>();
		if (g_Git.GetMapHashToFriendName(*newHashMap.get()))
			MessageBox(g_Git.GetGitLastErr(L"Could not get all refs."), L"TortoiseGit", MB_ICONERROR);
		m_HashMap = newHashMap;

		m_CurrentBranch=g_Git.GetCurrentBranch();

		if (g_Git.GetHash(m_HeadHash, L"HEAD"))
		{
			MessageBox(g_Git.GetGitLastErr(L"Could not get HEAD hash. Quitting..."), L"TortoiseGit", MB_ICONERROR);
			ExitProcess(1);
		}

		m_wcRev.m_ParentHash.clear();
		m_wcRev.m_ParentHash.push_back(m_HeadHash);

		FetchRemoteList();
		FetchTrackingBranchList();

		UpdateSubmodulePointer();
	}
	void StartAsyncDiffThread();
	void StartLoadingThread();
public:
	void SafeTerminateThread()
	{
		if (m_LoadingThread && InterlockedExchange(&m_bExitThread, TRUE) == FALSE)
		{
			DWORD ret = WAIT_TIMEOUT;
			for (int i = 0; i < 200 && m_bThreadRunning; ++i)
				ret =::WaitForSingleObject(m_LoadingThread->m_hThread, 100);
			if (ret == WAIT_TIMEOUT && m_bThreadRunning)
				::TerminateThread(m_LoadingThread, 0);
			delete m_LoadingThread;
			m_LoadingThread = nullptr;
		}
	};
protected:
	bool IsInWorkingThread()
	{
		return (AfxGetThread() == m_LoadingThread);
	}
public:
	void SetRange(const CString& range)
	{
		Locker lock(m_critSec);
		m_sRange = range;
	}

	CString GetRange() const { return m_sRange; }

	int					m_nSearchIndex;
protected:
	volatile LONG		m_bExitThread;
	CWinThread*			m_LoadingThread;
public:
	std::shared_ptr<MAP_HASH_NAME> m_HashMap;
protected:
	std::map<CString, std::pair<CString, CString>>	m_TrackingMap;

public:
	void				SetStyle();

	CString				m_ColumnRegKey;

protected:
	typedef struct {
		CString name;
		COLORREF color;
		CString simplifiedName;
		CString fullName;
		bool singleRemote;
		bool hasTracking;
		bool sameName;
		CGit::REF_TYPE refType;
	} REFLABEL;

	DECLARE_MESSAGE_MAP()
	afx_msg void OnDestroy();
	virtual afx_msg void OnNMCustomdrawLoglist(NMHDR* pNMHDR, LRESULT* pResult);
	virtual afx_msg void OnLvnGetdispinfoLoglist(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg LRESULT OnFindDialogMessage(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnScrollToMessage(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnScrollToRef(WPARAM wParam, LPARAM lParam);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg LRESULT OnLoad(WPARAM wParam, LPARAM lParam);
	virtual void OnNMDblclkLoglist(NMHDR * /*pNMHDR*/, LRESULT *pResult);
	afx_msg void OnLvnOdfinditemLoglist(NMHDR *pNMHDR, LRESULT *pResult);
	void PreSubclassWindow() override;
	virtual BOOL PreTranslateMessage(MSG* pMsg) override;
	virtual ULONG GetGestureStatus(CPoint ptTouch) override;
	static UINT LogThreadEntry(LPVOID pVoid);
	UINT LogThread();
	bool IsOnStash(int index);
	bool IsStash(const GitRev * pSelLogEntry);
	bool IsBisect(const GitRev * pSelLogEntry);
	void FetchRemoteList();
	void FetchTrackingBranchList();

	virtual afx_msg BOOL OnToolTipText(UINT id, NMHDR* pNMHDR, LRESULT* pResult);
	virtual INT_PTR OnToolHitTest(CPoint point, TOOLINFO* pTI) const override;
	CString GetToolTipText(int nItem, int nSubItem);

	/** Checks whether a referenfe label is under pt and returns the index/type
	 * pLogEntry  IN: the entry of commit
	 * pt         IN: the mouse position in client coordinate
	 * type       IN: give the specific reference type, then check if it is the same reference type.
	 *            OUT: give CGit::REF_TYPE::UNKNOWN for getting the real type it is.
	 * pShortname OUT: the short name of that reference label
	 * pIndex     OUT: the index value of label of that entry
	 */
	bool IsMouseOnRefLabel(const GitRevLoglist* pLogEntry, const POINT& pt, CGit::REF_TYPE& type, MAP_HASH_NAME& hashMap, CString* pShortname = nullptr, size_t* pIndex = nullptr);
	bool IsMouseOnRefLabelFromPopupMenu(const GitRevLoglist* pLogEntry, const CPoint& pt, CGit::REF_TYPE& type, MAP_HASH_NAME& hashMap, CString* pShortname = nullptr, size_t* pIndex = nullptr);

	void FillBackGround(HDC hdc, DWORD_PTR Index, CRect &rect);
	void DrawTagBranchMessage(NMLVCUSTOMDRAW* pLVCD, CRect& rect, INT_PTR index, std::vector<REFLABEL>& refList);
	void DrawTagBranch(HDC hdc, CDC& W_Dc, HTHEME hTheme, CRect& rect, CRect& rt, LVITEM& rItem, GitRevLoglist* data, std::vector<REFLABEL>& refList);
	void DrawGraph(HDC,CRect &rect,INT_PTR index);
	bool CGitLogListBase::DrawListItemWithMatchesIfEnabled(std::shared_ptr<CLogDlgFilter> filter, DWORD selectedFilter, NMLVCUSTOMDRAW* pLVCD, LRESULT* pResult);
	static void DrawListItemWithMatchesRect(NMLVCUSTOMDRAW* pLVCD, const std::vector<CHARRANGE>& ranges, CRect rect, const CString& text, CColors& colors, HTHEME hTheme = nullptr, int txtState = 0);

public:
	// needs to be called from LogDlg.cpp and FileDiffDlg.cpp
	static LRESULT DrawListItemWithMatches(CFilterHelper* filter, CListCtrl& listCtrl, NMLVCUSTOMDRAW* pLVCD, CColors& colors);

protected:
	void paintGraphLane(HDC hdc,int laneHeight, int type, int x1, int x2,
									  const COLORREF& col,const COLORREF& activeColor, int top) ;
	void DrawLine(HDC hdc, int x1, int y1, int x2, int y2){ ::MoveToEx(hdc, x1, y1, nullptr); ::LineTo(hdc, x2, y2); }
	/**
	* Save column widths to the registry
	*/
	void SaveColumnWidths() override;	// save col widths to the registry

	int GetHeadIndex();

	std::vector<GitRevLoglist*> m_AsynDiffList;
	CComAutoCriticalSection m_AsynDiffListLock;
	HANDLE	m_AsyncDiffEvent;
	volatile LONG m_AsyncThreadExit;
	CWinThread*			m_DiffingThread;
	volatile LONG m_AsyncThreadRunning;

	static int DiffAsync(GitRevLoglist* rev, IAsyncDiffCB* pdata)
	{
		auto data = static_cast<CGitLogListBase*>(pdata);
		ULONGLONG offset = data->m_LogCache.GetOffset(rev->m_CommitHash);
		if (!offset || data->m_LogCache.LoadOneItem(*rev, offset))
		{
			data->m_AsynDiffListLock.Lock();
			data->m_AsynDiffList.push_back(rev);
			data->m_AsynDiffListLock.Unlock();
			::SetEvent(data->m_AsyncDiffEvent);
			return 0;
		}

		InterlockedExchange(&rev->m_IsDiffFiles, TRUE);
		if (!rev->m_IsCommitParsed)
			return 0;
		InterlockedExchange(&rev->m_IsFull, TRUE);
		// we might need to signal that the changed files are now available
		if (data->GetSelectedCount() == 1)
		{
			POSITION pos = data->GetFirstSelectedItemPosition();
			int nItem = data->GetNextSelectedItem(pos);
			if (nItem >= 0)
			{
				GitRevLoglist* data2 = data->m_arShownList.SafeGetAt(nItem);
				if (data2 && data2->m_CommitHash == rev->m_CommitHash)
					data->GetParent()->PostMessage(WM_COMMAND, MSG_FETCHED_DIFF, 0);
			}
		}
		return 0;
	}

	static UINT AsyncThread(LPVOID data)
	{
		return reinterpret_cast<CGitLogListBase*>(data)->AsyncDiffThread();
	}

	int AsyncDiffThread();

public:
	void SafeTerminateAsyncDiffThread()
	{
		if (m_DiffingThread && InterlockedExchange(&m_AsyncThreadExit, TRUE) == FALSE)
		{
			::SetEvent(m_AsyncDiffEvent);
			DWORD ret = WAIT_TIMEOUT;
			// do not block here, but process messages and ask until the thread ends
			while (ret == WAIT_TIMEOUT && m_AsyncThreadRunning)
			{
				MSG msg;
				if (::PeekMessage(&msg, nullptr, 0,0, PM_NOREMOVE))
					AfxGetThread()->PumpMessage(); // process messages, so that GetTopIndex and so on in the thread work
				ret = ::WaitForSingleObject(m_DiffingThread->m_hThread, 100);
			}
			delete m_DiffingThread;
			m_DiffingThread = nullptr;
			InterlockedExchange(&m_AsyncThreadExit, FALSE);
		}
	};

protected:
	CComAutoCriticalSection	m_critSec;

	HICON				m_hModifiedIcon;
	HICON				m_hReplacedIcon;
	HICON				m_hConflictedIcon;
	HICON				m_hAddedIcon;
	HICON				m_hDeletedIcon;
	HICON				m_hFetchIcon;

	CFont				m_boldFont;
	CFont				m_FontItalics;
	CFont				m_boldItalicsFont;

	CRegDWORD			m_regMaxBugIDColWidth;

	void				*m_ProcData;

	CColors				m_Colors;

	CString				m_CurrentBranch;
	CGitHash			m_HeadHash;

	COLORREF			m_LineColors[Lanes::COLORS_NUM];
	DWORD				m_LineWidth;
	DWORD				m_NodeSize;
	DWORD				m_DateFormat;	// DATE_SHORTDATE or DATE_LONGDATE
	bool				m_bRelativeTimes;	// Show relative times
	GIT_LOG				m_DllGitLog;
	CString				m_SingleRemote;
	bool				m_bTagsBranchesOnRightSide;
	bool				m_bFullCommitMessageOnLogLine;
	bool				m_bSymbolizeRefNames;
	bool				m_bIncludeBoundaryCommits;

	DWORD				m_dwDefaultColumns;
	TCHAR               m_wszTip[8192];
	char                m_szTip[8192];
	std::map<CString, CRect> m_RefLabelPosMap; // ref name vs. label position
	int					m_OldTopIndex;

	GIT_MAILMAP			m_pMailmap;

	bool				m_bDragndropEnabled;
	BOOL				m_bDragging;
	int					m_nDropIndex;
	int					m_nDropMarkerLast;
	int					m_nDropMarkerLastHot;
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult);
	void				DrawDropInsertMarker(int nIndex);
	void				DrawDropInsertMarkerLine(int nIndex);

public:
	void				EnableDragnDrop(bool enable) { m_bDragndropEnabled = enable; }
};
