#pragma once

#include "HintListCtrl.h"
#include "XPTheme.h"
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
#include "HintListCtrl.h"
//#include "GitLogList.h"
#include "lanes.h"
#include "GitLogCache.h"
#include <regex>
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

#define MCS_NOTRAILINGDATES  0x0040
#define MCS_SHORTDAYSOFWEEK  0x0080
#define MCS_NOSELCHANGEONNAV 0x0100

#define DTM_SETMCSTYLE    (DTM_FIRST + 11)

#endif

#define ICONITEMBORDER 5

#define GITLOG_START 0
#define GITLOG_START_ALL 1
#define GITLOG_END   100

#define LOGFILTER_ALL      1
#define LOGFILTER_MESSAGES 2
#define LOGFILTER_PATHS    3
#define LOGFILTER_AUTHORS  4
#define LOGFILTER_REVS	   5
#define LOGFILTER_REGEX	   6
#define LOGFILTER_BUGID    7

//typedef void CALLBACK_PROCESS(void * data, int progress);
#define MSG_LOADED				(WM_USER+110)
#define MSG_LOAD_PERCENTAGE		(WM_USER+111)
#define MSG_REFLOG_CHANGED		(WM_USER+112)

class CGitLogListBase : public CHintListCtrl
{
	DECLARE_DYNAMIC(CGitLogListBase)

public:
	CGitLogListBase();
	virtual ~CGitLogListBase();
	volatile LONG		m_bNoDispUpdates;
	BOOL m_IsIDReplaceAction;
	BOOL m_IsOldFirst;
	BOOL m_IsRebaseReplaceGraph;


	BOOL m_bStrictStopped;
	BOOL m_bShowBugtraqColumn;
	BOOL m_bSearchIndex;
	BOOL m_bCancelled;
	unsigned __int64 m_ContextMenuMask;

	bool				m_hasWC;
	GitRev				m_wcRev;
	volatile LONG 		m_bThreadRunning;
	CLogCache			m_LogCache;

	enum
	{
		LOGLIST_GRAPH,
		LOGLIST_ACTION,
		LOGLIST_MESSAGE,
		LOGLIST_AUTHOR,
		LOGLIST_DATE,
		LOGLIST_BUG,
		LOGLIST_MESSAGE_MAX=300,
		LOGLIST_MESSAGE_MIN=200
	};

	enum 
	{
	// needs to start with 1, since 0 is the return value if *nothing* is clicked on in the context menu
	ID_COMPARE = 1,
	ID_SAVEAS,
	ID_COMPARETWO,
	ID_UPDATE,
	ID_COPY,
	ID_REVERTREV,
	ID_MERGEREV,
	ID_GNUDIFF1,
	ID_GNUDIFF2,
	ID_FINDENTRY,
	ID_OPEN,
	ID_BLAME,
	ID_REPOBROWSE,
	ID_LOG,
	ID_POPPROPS,
	ID_EDITAUTHOR,
	ID_EDITLOG,
	ID_DIFF,
	ID_OPENWITH,
	ID_COPYCLIPBOARD,
	ID_COPYHASH,
	ID_CHECKOUT,
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
	};
	inline unsigned __int64 GetContextMenuBit(int i){ return ((unsigned __int64 )0x1)<<i ;}
	void InsertGitColumn();
	void ResizeAllListCtrlCols();
	void CopySelectionToClipBoard(bool hashonly=FALSE);
	void DiffSelectedRevWithPrevious();
	bool IsSelectionContinuous();
	int  FillGitShortLog();
	int  FillGitLog(CTGitPath *path,int infomask=CGit::	LOG_INFO_STAT| CGit::LOG_INFO_FILESTATE | CGit::LOG_INFO_SHOW_MERGEDFILE,CString *from=NULL,CString *to=NULL);

	inline int ShownCountWithStopped() const { return (int)m_arShownList.GetCount() + (m_bStrictStopped ? 1 : 0); }
	int FetchLogAsync(void * data=NULL);
	CPtrArray			m_arShownList;
	void Refresh();
	void RecalculateShownList(CPtrArray * pShownlist);
	void Clear();

	int					m_nSelectedFilter;
	CLogDataVector		m_logEntries;
	void RemoveFilter();
	void StartFilter();
	bool ValidateRegexp(LPCTSTR regexp_str, tr1::wregex& pat, bool bMatchCase = false );
	CString				m_sFilterText;
	CTime			m_From;
	CTime			m_To;
    
    CTGitPath           m_Path;
    int					m_ShowMask;

	void				GetTimeRange(CTime &oldest,CTime &latest);
	virtual void ContextMenuAction(int cmd,int FirstSelect, int LastSelect)=0;
	void ReloadHashMap()
	{	
		m_HashMap.clear();
		g_Git.GetMapHashToFriendName(m_HashMap);
		m_CurrentBranch=g_Git.GetCurrentBranch();
		this->m_HeadHash=g_Git.GetHash(CString(_T("HEAD"))).Left(40);
	}
	void TerminateThread()
	{
		if(this->m_LoadingThread)
			AfxTermThread((HINSTANCE)m_LoadingThread->m_hThread);
	};

	bool IsInWorkingThread()
	{
		return (AfxGetThread() == m_LoadingThread);
	}

	void SetStartRef(const CString& StartRef)
	{
		m_StartRef=StartRef;
	}

	
	volatile bool		m_bExitThread;
	CWinThread*			m_LoadingThread;
protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnDestroy();
	virtual afx_msg void OnNMCustomdrawLoglist(NMHDR *pNMHDR, LRESULT *pResult);
	virtual afx_msg void OnLvnGetdispinfoLoglist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg LRESULT OnLoad(WPARAM wParam, LPARAM lParam);
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
                                      const COLORREF& col,int top) ; 
	void DrawLine(HDC hdc, int x1, int y1, int x2, int y2){::MoveToEx(hdc,x1,y1,NULL);::LineTo(hdc,x2,y2);}
	/**
	* Save column widths to the registry
	*/
	void SaveColumnWidths();	// save col widths to the registry

	BOOL IsEntryInDateRange(int i);

	int GetHeadIndex();

	bool				m_bFilterWithRegex;

	
	CXPTheme			m_Theme;
	BOOL				m_bVista;
	
	HICON				m_hModifiedIcon;
	HICON				m_hReplacedIcon;
	HICON				m_hAddedIcon;
	HICON				m_hDeletedIcon;

	HFONT				m_boldFont;

	CRegDWORD			m_regMaxBugIDColWidth;
	int					m_nSearchIndex;
	
	void				*m_ProcData;
	CStoreSelection*	m_pStoreSelection;
	MAP_HASH_NAME		m_HashMap;

	CColors				m_Colors;

	CString				m_CurrentBranch;
	CString				m_HeadHash;

	CString				m_StartRef; //Ref of the top-commit
	
	CString				m_ColumnRegKey;

	COLORREF			m_LineColors[Lanes::COLORS_NUM];
	DWORD				m_DateFormat;	// DATE_SHORTDATE or DATE_LONGDATE
	bool				m_bRelativeTimes;	// Show relative times
};


