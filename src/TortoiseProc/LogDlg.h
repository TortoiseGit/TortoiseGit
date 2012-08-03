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
#pragma once

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
#include <regex>
#include "GitLogList.h"
#include "GitStatusListCtrl.h"
#include "HyperLink.h"
#include "Win7.h"

using namespace std;


#define LOGFILTER_TIMER	101
#define LOGFTIME_TIMER	102

typedef int (__cdecl *GENERICCOMPAREFN)(const void * elem1, const void * elem2);

/**
 * \ingroup TortoiseProc
 * Shows log messages of a single file or folder in a listbox.
 */
class CLogDlg : public CResizableStandAloneDialog, IFilterEditValidator
{
	DECLARE_DYNAMIC(CLogDlg)

	friend class CStoreSelection;

public:
	CLogDlg(CWnd* pParent = NULL); // standard constructor
	virtual ~CLogDlg();
	void SetParams(const CTGitPath& orgPath, const CTGitPath& path, CString hightlightRevision, CString startrev, CString endrev, int limit);
	void SetFilter(const CString& findstr, LONG findtype, bool findregex);
	bool IsThreadRunning() {return !!m_LogList.m_bThreadRunning;}
	void SetDialogTitle(const CString& sTitle) {m_sTitle = sTitle;}
	void SetSelect(bool bSelect) {m_bSelect = bSelect;}
	void ContinuousSelection(bool bCont = true) {m_bSelectionMustBeContinuous = bCont;}
	void SingleSelection(bool bSingle = true) {m_bSelectionMustBeSingle = bSingle;}
	void SetStartRef(const CString& StartRef);
	void ShowStartRef();
	afx_msg LRESULT OnRefLogChanged(WPARAM wParam, LPARAM lParam);
	/**
	 * Provides selected commit hash if available, call after OK return from here
	 * Empty if none
	**/
	CString GetSelectedHash(){ return m_sSelectedHash; }

// Dialog Data
	enum { IDD = IDD_LOGMESSAGE };

	void	FillLogMessageCtrl(bool bShow = true);
	CString	GetTagInfo(GitRev* pLogEntry);

	void UpdateLogInfoLabel();

	afx_msg void OnFind()
	{
		m_LogList.OnFind();
	}
protected:
	//implement the virtual methods from Git base class
	virtual BOOL Cancel();
	virtual bool Validate(LPCTSTR string);

	virtual void DoDataExchange(CDataExchange* pDX); // DDX/DDV support

	afx_msg LRESULT	OnTaskbarBtnCreated(WPARAM wParam, LPARAM lParam);
	CComPtr<ITaskbarList3>	m_pTaskbarList;

	afx_msg LRESULT OnClickedInfoIcon(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnClickedCancelFilter(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnLogListLoading(WPARAM wParam, LPARAM lParam);

	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnNMDblclkLoglist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnItemchangedLoglist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedHelp();
	afx_msg void OnEnLinkMsgview(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedStatbutton();

	afx_msg void OnNMCustomdrawChangedFileList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnGetdispinfoChangedFileList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnEnChangeSearchedit();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnDtnDatetimechangeDateto(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDtnDatetimechangeDatefrom(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnColumnclick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickShowWholeProject();
	afx_msg void OnBnClickedHidepaths();
	afx_msg void OnBnClickedAllBranch();
	afx_msg void OnBnClickedFollowRenames();
	afx_msg void OnBnClickedBrowseRef();

	afx_msg void OnDtnDropdownDatefrom(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDtnDropdownDateto(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnBnClickedFirstParent();
	afx_msg void OnBnClickedRefresh();
	afx_msg void OnRefresh();
	afx_msg void OnFocusFilter();
	afx_msg void OnEditCopy();

	virtual void OnCancel();
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	void	DoDiffFromLog(INT_PTR selIndex, GitRev *rev1, GitRev *rev2, bool blame, bool unified);

	DECLARE_MESSAGE_MAP()

private:
	CRegDWORD m_regbAllBranch;

	void Refresh (bool clearfilter = false);
	void DoSizeV1(int delta);
	void DoSizeV2(int delta);
	void AdjustMinSize();
	void SetSplitterRange();
	void SetFilterCueText();

	void CopySelectionToClipBoard();
	void CopyChangedSelectionToClipBoard();
	CTGitPathList GetChangedPathsFromSelectedRevisions(bool bRelativePaths = false, bool bUseFilter = true);
	void SortShownListArray();

	void SetSortArrow(CListCtrl * control, int nColumn, bool bAscending);
	void SortByColumn(int nSortColumn, bool bAscending);

	void EnableOKButton();

	void SaveSplitterPos();
	bool ValidateRegexp(LPCTSTR regexp_str, tr1::wregex& pat, bool bMatchCase);
	void CheckRegexpTooltip();
	void GetChangedPaths(std::vector<CString>& changedpaths, std::vector<LogChangedPath*>& changedlogpaths);
	void SetDlgTitle();
	CString GetAbsoluteUrlFromRelativeUrl(const CString& url);


	virtual LRESULT DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam);

public:
	CWnd *				m_pNotifyWindow;
	WORD				m_wParam;
private:
	CString				m_sRelativeRoot;
	CString				m_sRepositoryRoot;
	CGitLogList			m_LogList;
	CGitStatusListCtrl  m_ChangedFileListCtrl;
	CFilterEdit			m_cFilter;
	CHyperLink			m_staticRef;
	CProgressCtrl		m_LogProgress;
	CMenuButton			m_btnShowWholeProject;
	CTGitPath			m_path;
	CTGitPath			m_orgPath;
	CString				m_hightlightRevision;

	CString				m_LogRevision;

	CString				m_sSelectedHash;	// set to selected commit hash on OK if appropriate
	bool				m_bSelectionMustBeContinuous;
	bool				m_bSelectionMustBeSingle;
	bool				m_bCancelled;

	BOOL				m_bFirstParent;
	BOOL				m_bAllBranch;
	BOOL				m_bWholeProject;
	BOOL				m_bFollowRenames;

	CTGitPathList	*   m_currentChangedArray;
	LogChangedPathArray m_CurrentFilteredChangedArray;
	CTGitPathList		m_currentChangedPathList;

	bool				m_bFilterWithRegex;

	CFont				m_logFont;
	CSplitterControl	m_wndSplitter1;
	CSplitterControl	m_wndSplitter2;
	CRect				m_DlgOrigRect;
	CRect				m_MsgViewOrigRect;
	CRect				m_LogListOrigRect;
	CRect				m_ChgOrigRect;

	//volatile LONG		m_bNoDispUpdates;
	CDateTimeCtrl		m_DateFrom;
	CDateTimeCtrl		m_DateTo;
	int					m_limit;
	int					m_nSortColumn;
	bool				m_bAscending;
	static int			m_nSortColumnPathList;
	static bool			m_bAscendingPathList;
	CButton				m_cHidePaths;
	CString				m_sTitle;
	bool				m_bSelect;
	CString				m_sLogInfo;

	CToolTips			m_tooltips;

	CColors				m_Colors;
	CImageList			m_imgList;

	HACCEL				m_hAccel;
};
static UINT WM_REVSELECTED = RegisterWindowMessage(_T("TORTOISEGit_REVSELECTED_MSG"));
static UINT WM_REVLIST = RegisterWindowMessage(_T("TORTOISEGit_REVLIST_MSG"));
static UINT WM_REVLISTONERANGE = RegisterWindowMessage(_T("TORTOISEGit_REVLISTONERANGE_MSG"));
