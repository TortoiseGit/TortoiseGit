// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2009 - TortoiseSVN
// Copyright (C) 2008-2018 - TortoiseGit

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
#include "StandAloneDlg.h"
#include "TGitPath.h"
#include "registry.h"
#include "RegHistory.h"
#include "SplitterControl.h"
#include "Colors.h"
#include "LogDlgHelper.h"
#include "FilterEdit.h"
#include <regex>
#include "GitLogList.h"
#include "GitStatusListCtrl.h"
#include "HyperLink.h"
#include "GravatarPictureBox.h"
#include "PatchViewDlg.h"
#include "MenuButton.h"
#include "GestureEnabledControl.h"

#define LOGFILTER_TIMER	101
#define LOGFTIME_TIMER	102
#define LOG_FILLPATCHVTIMER	103
#define LOG_HEADER_ORDER_TIMER 104
#define FILEFILTER_TIMER	105

typedef int (__cdecl *GENERICCOMPAREFN)(const void * elem1, const void * elem2);

enum AllBranchType
{
	None = 0,
	AllBranches = 1,
	AllLocalBranches = 2,
	AllBasicRefs = 3,
};

/**
 * \ingroup TortoiseProc
 * Shows log messages of a single file or folder in a listbox.
 */
class CLogDlg : public CResizableStandAloneDialog, IFilterEditValidator, IHasPatchView
{
	DECLARE_DYNAMIC(CLogDlg)

	friend class CStoreSelection;

public:
	CLogDlg(CWnd* pParent = nullptr); // standard constructor
	virtual ~CLogDlg();
	void SetParams(const CTGitPath& orgPath, const CTGitPath& path, CString hightlightRevision, CString range, DWORD limit, int limitScale = -1);
	void SetFilter(const CString& findstr, LONG findtype, bool findregex);
	bool IsThreadRunning() {return !!m_LogList.m_bThreadRunning;}
	void SetSelect(bool bSelect) {m_bSelect = bSelect;}
	void ContinuousSelection(bool bCont = true) {m_bSelectionMustBeContinuous = bCont;}
	void SingleSelection(bool bSingle = true) {m_bSelectionMustBeSingle = bSingle;}
	void ShowWorkingTreeChanges(bool bShow = false) { m_bShowWC = bShow; }
	void SetRange(const CString& range);
	void ShowStartRef();
	afx_msg LRESULT OnRefLogChanged(WPARAM wParam, LPARAM lParam);
	/**
	 * Provides selected commit hash if available, call after OK return from here
	 * Empty if none
	**/
	std::vector<CGitHash> GetSelectedHash(){ return m_sSelectedHash; }

// Dialog Data
	enum { IDD = IDD_LOGMESSAGE };

	void	FillLogMessageCtrl(bool bShow = true);

	void UpdateLogInfoLabel();

	afx_msg void OnFind()
	{
		m_LogList.OnFind();
	}
protected:
	//implement the virtual methods from Git base class
	virtual bool Validate(LPCTSTR string) override;

	virtual void DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support

	afx_msg LRESULT	OnTaskbarBtnCreated(WPARAM wParam, LPARAM lParam);
	CComPtr<ITaskbarList3>	m_pTaskbarList;

	afx_msg LRESULT OnRefreshSelection(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnClickedInfoIcon(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnClickedCancelFilter(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnLogListLoading(WPARAM wParam, LPARAM lParam);

	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnLvnItemchangedLoglist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnItemchangedLogmsg(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnEnLinkMsgview(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedStatbutton();
	afx_msg void OnNMCustomdrawChangedFileList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelectSearchField();
	afx_msg void OnExitClearFilter();

	afx_msg void OnEnChangeSearchedit();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnDtnDatetimechangeDateto(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDtnDatetimechangeDatefrom(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnCbnSelchangeJumpType();
	afx_msg void OnBnClickedJumpUp();
	afx_msg void OnBnClickedJumpDown();
	afx_msg void OnLvnColumnclick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedWalkBehaviour();
	afx_msg void OnBnClickedView();
	afx_msg void OnBnClickShowWholeProject();
	void OnBnClickedHidepaths();
	afx_msg void OnBnClickedAllBranch();
	void OnBnClickedFollowRenames();
	void HandleShowLabels(bool var, int flag);
	void OnBnClickedCompressedGraph();
	afx_msg void OnBnClickedBrowseRef();

	afx_msg void GoBack();
	afx_msg void GoForward();
	afx_msg void GoBackAndSelect();
	afx_msg void GoForwardAndSelect();
	void GoBackForward(bool select, bool bForward);

	afx_msg void OnDtnDropdownDatefrom(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	void OnBnClickedFirstParent();
	afx_msg void OnBnClickedRefresh();
	afx_msg void OnRefresh();
	afx_msg void OnFocusFilter();
	afx_msg void OnEditCopy();
	afx_msg void OnEnChangeFileFilter();
	afx_msg void OnEnscrollMsgview();
	afx_msg LRESULT OnResetWcRev(WPARAM, LPARAM);

	virtual void OnCancel() override;
	virtual void OnOK() override;
	virtual BOOL OnInitDialog() override;
	virtual BOOL PreTranslateMessage(MSG* pMsg) override;
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnPaint();

	void OnPasteGitHash();
	void JumpToGitHash(CString hash);

	DECLARE_MESSAGE_MAP()

private:
	CRegDWORD m_regbAllBranch;
	CRegDWORD m_regbShowTags;
	CRegDWORD m_regbShowLocalBranches;
	CRegDWORD m_regbShowRemoteBranches;
	CRegDWORD m_regbShowOtherRefs;
	CRegDWORD m_regbShowGravatar;
	CRegDWORD m_regShowWholeProject;
	CRegDWORD m_regAddBeforeCommit;

	void Refresh (bool clearfilter = false);
	void MoveToSameTop(CWnd *pWndRef, CWnd *pWndTarget);
	void AddMainAnchors();
	void RemoveMainAnchors();
	void DoSizeV1(int delta);
	void DoSizeV2(int delta);
	void SetSplitterRange();
	void SetFilterCueText();

	void CopyChangedSelectionToClipBoard();
	void SortShownListArray();

	void SetSortArrow(CListCtrl * control, int nColumn, bool bAscending);
	void SortByColumn(int nSortColumn, bool bAscending);

	void EnableOKButton();

	void SaveSplitterPos();
	void CheckRegexpTooltip();
	void SetDlgTitle();
	CString GetAbsoluteUrlFromRelativeUrl(const CString& url);
	void ShowGravatar();

	CPatchViewDlg		m_patchViewdlg;
	void FillPatchView(bool onlySetTimer = false);
	CWnd* GetPatchViewParentWnd() override { return this; }
	void TogglePatchView() override;
	LRESULT OnFileListCtrlItemChanged(WPARAM /*wparam*/, LPARAM /*lparam*/);
	afx_msg LRESULT	OnGitStatusListCtrlNeedsRefresh(WPARAM, LPARAM);
	afx_msg void OnMoving(UINT fwSide, LPRECT pRect);
	afx_msg void OnSizing(UINT fwSide, LPRECT pRect);

	virtual LRESULT DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam) override;

public:
	CWnd *				m_pNotifyWindow;
	WORD				m_wParam;
private:
	CString				m_sRelativeRoot;
	CString				m_sRepositoryRoot;
	CGitLogList			m_LogList;
	CGitStatusListCtrl  m_ChangedFileListCtrl;
	CFilterEdit			m_cFilter;
	CString				m_sFilterText;
	DWORD				m_SelectedFilters;
	CGestureEnabledControlTmpl<CHyperLink> m_staticRef;
	CProgressCtrl		m_LogProgress;
	CTGitPath			m_path;
	CTGitPath			m_orgPath;
	CString				m_hightlightRevision;

	CMenuButton			m_ctrlView;
	CMenuButton			m_ctrlWalkBehavior;

	std::vector<CGitHash>	m_sSelectedHash;	// set to selected commit hash on OK if appropriate
	bool				m_bSelectionMustBeContinuous;
	bool				m_bSelectionMustBeSingle;
	bool				m_bShowWC;

	CFilterEdit			m_cFileFilter;

	BOOL				m_iHidePaths;
	bool				m_bFirstParent;
	BOOL				m_bAllBranch;		// variable for checkbox only
	AllBranchType		m_AllBranchType;	// variable for actual branch type
	BOOL				m_bWholeProject;
	bool				m_bFollowRenames;
	BOOL				m_bShowUnversioned;
	bool				m_bShowTags;
	bool				m_bShowLocalBranches;
	bool				m_bShowRemoteBranches;
	bool				m_bShowOtherRefs;
	bool				m_bShowGravatar;
	bool				m_bShowDescribe;
	bool				m_bShowBranchRevNo;
	bool				m_bNoMerges;
	int					m_iCompressedGraph;
	bool				m_bNavigatingWithSelect;
	bool				m_bAsteriskLogPrefix;

	bool				m_bFilterWithRegex;
	bool				m_bFilterCaseSensitively;

	CFont				m_logFont;
	CSplitterControl	m_wndSplitter1;
	CSplitterControl	m_wndSplitter2;
	CRect				m_DlgOrigRect;
	CRect				m_MsgViewOrigRect;
	CRect				m_LogListOrigRect;
	CRect				m_ChgOrigRect;

	//volatile LONG		m_bNoDispUpdates;
	CGestureEnabledControlTmpl<CDateTimeCtrl> m_DateFrom;
	CRegString			m_regLastSelectedFromDate;
	CDateTimeCtrl		m_DateTo;
	CComboBox			m_JumpType;
	CButton				m_JumpUp;
	CButton				m_JumpDown;
	int					m_nSortColumn;
	bool				m_bAscending;
	static int			m_nSortColumnPathList;
	static bool			m_bAscendingPathList;
	CString				m_sTitle;
	bool				m_bSelect;
	CString				m_sLogInfo;

	CColors				m_Colors;
	CImageList			m_imgList;

	HACCEL				m_hAccel;

	CRegHistory			m_History;

	CGravatar			m_gravatar;

	CBrush				m_Brush;
};
static UINT WM_REVSELECTED = RegisterWindowMessage(L"TORTOISEGit_REVSELECTED_MSG");
static UINT WM_REVLIST = RegisterWindowMessage(L"TORTOISEGit_REVLIST_MSG");
static UINT WM_REVLISTONERANGE = RegisterWindowMessage(L"TORTOISEGit_REVLISTONERANGE_MSG");
