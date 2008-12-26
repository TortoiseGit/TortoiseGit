#pragma once

#include "HintListCtrl.h"
#include "XPTheme.h"
#include "resource.h"
#include "Git.h"
#include "ProjectProperties.h"
#include "StandAloneDlg.h"
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
#include "GitLogList.h"

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

class CGitLogList : public CHintListCtrl
{
	DECLARE_DYNAMIC(CGitLogList)

public:
	CGitLogList();
	virtual ~CGitLogList();
	BOOL m_bNoDispUpdates;
	BOOL m_bStrictStopped;
	BOOL m_bShowBugtraqColumn;
	BOOL m_bSearchIndex;
	BOOL m_bCancelled;
	bool				m_hasWC;
	GitRev				m_wcRev;

	enum
	{
		LOGLIST_GRAPH,
		LOGLIST_ACTION,
		LOGLIST_MESSAGE,
		LOGLIST_AUTHOR,
		LOGLIST_DATE,
		LOGLIST_BUG,
		LOGLIST_MESSAGE_MAX=250
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
	ID_REVPROPS
	};
	void InsertGitColumn();
	void ResizeAllListCtrlCols();
	void CopySelectionToClipBoard();
	void DiffSelectedRevWithPrevious();
	bool IsSelectionContinuous();
	int  FillGitLog();
	inline int ShownCountWithStopped() const { return (int)m_arShownList.GetCount() + (m_bStrictStopped ? 1 : 0); }

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnNMCustomdrawLoglist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnGetdispinfoLoglist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	void OnNMDblclkLoglist(NMHDR * /*pNMHDR*/, LRESULT *pResult);
	afx_msg void OnLvnOdfinditemLoglist(NMHDR *pNMHDR, LRESULT *pResult);
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	CPtrArray			m_arShownList;
	CXPTheme			m_Theme;
	BOOL				m_bVista;
	BOOL				m_bThreadRunning;
	
	HICON				m_hModifiedIcon;
	HICON				m_hReplacedIcon;
	HICON				m_hAddedIcon;
	HICON				m_hDeletedIcon;

	HFONT				m_boldFont;

	CRegDWORD			m_regMaxBugIDColWidth;


	int					m_nSearchIndex;
	CLogDataVector		m_logEntries;

};


