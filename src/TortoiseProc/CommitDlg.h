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

#include "StandAloneDlg.h"
#include "GitStatusListCtrl.h"
#include "ProjectProperties.h"
#include "RegHistory.h"
#include "Registry.h"
#include "SciEdit.h"
#include "SplitterControl.h"
#include "PathWatcher.h"
#include "BugTraqAssociations.h"
#include "Tooltip.h"
#include "..\IBugTraqProvider\IBugTraqProvider_h.h"
#include "Git.h"
#include "HyperLink.h"
#include "PatchViewDlg.h"

#include <regex>
using namespace std;

#define ENDDIALOGTIMER	100
#define REFRESHTIMER	101

/**
 * \ingroup TortoiseProc
 * Dialog to enter log messages used in a commit.
 */
class CCommitDlg : public CResizableStandAloneDialog, public CSciEditContextMenuInterface
{
	DECLARE_DYNAMIC(CCommitDlg)

public:
	CCommitDlg(CWnd* pParent = NULL); // standard constructor
	virtual ~CCommitDlg();

protected:
	// CSciEditContextMenuInterface
	virtual void		InsertMenuItems(CMenu& mPopup, int& nCmd);
	virtual bool		HandleMenuItemClick(int cmd, CSciEdit * pSciEdit);

public:
	void ShowViewPatchText(bool b=true)
	{
		if(b)
			this->m_ctrlShowPatch.SetWindowText(CString(MAKEINTRESOURCE(IDS_PROC_COMMIT_SHOWPATCH)));
		else
			this->m_ctrlShowPatch.SetWindowText(CString(MAKEINTRESOURCE(IDS_PROC_COMMIT_HIDEPATCH)));

		m_ctrlShowPatch.Invalidate();
	}
private:
	static UINT StatusThreadEntry(LPVOID pVoid);
	UINT StatusThread();
	void UpdateOKButton();
	void FillPatchView();
	void SetDlgTitle();
	CString GetSignedOffByLine();
	CString m_sTitle;

// Dialog Data
	enum { IDD = IDD_COMMITDLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX); // DDX/DDV support

	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual LRESULT DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	afx_msg void OnBnClickedSelectall();
	afx_msg void OnBnClickedHelp();
	afx_msg void OnBnClickedShowunversioned();
	afx_msg void OnBnClickedHistory();
	afx_msg void OnBnClickedBugtraqbutton();
	afx_msg void OnEnChangeLogmessage();
	afx_msg void OnStnClickedExternalwarning();
	afx_msg void OnFocusMessage();
	afx_msg LRESULT OnGitStatusListCtrlItemCountChanged(WPARAM, LPARAM);
	afx_msg LRESULT OnGitStatusListCtrlNeedsRefresh(WPARAM, LPARAM);
	afx_msg LRESULT OnGitStatusListCtrlCheckChanged(WPARAM, LPARAM);
	afx_msg LRESULT OnGitStatusListCtrlItemChanged(WPARAM, LPARAM);

	afx_msg LRESULT OnAutoListReady(WPARAM, LPARAM);
	afx_msg LRESULT OnFileDropped(WPARAM, LPARAM lParam);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	void Refresh();
	void GetAutocompletionList();
	void ScanFile(const CString& sFilePath, const CString& sRegex);
	void DoSize(int delta);
	void SetSplitterRange();
	void SaveSplitterPos();
	void ParseRegexFile(const CString& sFile, std::map<CString, CString>& mapRegex);

	DECLARE_MESSAGE_MAP()

public:
	CString				m_sLogMessage;
	BOOL				m_bKeepChangeList;
	BOOL				m_bDoNotAutoselectSubmodules;
	BOOL				m_bCommitAmend;
	BOOL				m_bNoPostActions;
	bool				m_bSelectFilesForCommit;
	bool				m_bAutoClose;
	CString				m_AmendStr;
	CString				m_sBugID;
	BOOL				m_bWholeProject;
	CTGitPathList		m_pathList;
	CTGitPathList		m_checkedPathList;
	CTGitPathList		m_updatedPathList;
	int					m_PostCmd;
	BOOL				m_bPushAfterCommit;
	BOOL				m_bCreateTagAfterCommit;
	BOOL				m_bAmendDiffToLastCommit;

protected:
	CTGitPathList		m_selectedPathList;
	BOOL				m_bRecursive;
	CSciEdit			m_cLogMessage;
	CString				m_sChangeList;
	INT_PTR				m_itemsCount;
	CComPtr<IBugTraqProvider> m_BugTraqProvider;
	CString				m_NoAmendStr;
	BOOL				m_bCreateNewBranch;
	CString				m_sCreateNewBranch;

	int					CheckHeadDetach();

private:
	CWinThread*			m_pThread;
	std::set<CString>	m_autolist;
	CGitStatusListCtrl	m_ListCtrl;
	BOOL				m_bShowUnversioned;
	volatile LONG		m_bBlock;
	volatile LONG		m_bThreadRunning;
	volatile LONG		m_bRunThread;
	CToolTips			m_tooltips;
	CRegDWORD			m_regAddBeforeCommit;
	CRegDWORD			m_regKeepChangelists;
	CRegDWORD			m_regDoNotAutoselectSubmodules;
	ProjectProperties	m_ProjectProperties;
	CButton				m_SelectAll;
	CString				m_sWindowTitle;
	static UINT			WM_AUTOLISTREADY;
	int					m_nPopupPasteListCmd;
	int					m_nPopupPasteLastMessage;
	int					m_nPopupRecentMessage;
	CRegHistory			m_History;
	bool				m_bCancelled;
	CSplitterControl	m_wndSplitter;
	CRect				m_DlgOrigRect;
	CRect				m_LogMsgOrigRect;
	CPathWatcher		m_pathwatcher;
	CHyperLink			m_ctrlShowPatch;
	CPatchViewDlg		m_patchViewdlg;
	BOOL				m_bSetCommitDateTime;
	CDateTimeCtrl		m_CommitDate;
	CDateTimeCtrl		m_CommitTime;

	CBugTraqAssociation	m_bugtraq_association;
	HACCEL				m_hAccel;
	void				RestoreFiles(bool doNotAsk = false);

protected:
	afx_msg void OnBnClickedSignOff();
	afx_msg void OnBnClickedCommitAmend();
	afx_msg void OnBnClickedWholeProject();
	afx_msg void OnScnUpdateUI(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnStnClickedViewPatch();
	afx_msg void OnMoving(UINT fwSide, LPRECT pRect);
	afx_msg void OnSizing(UINT fwSide, LPRECT pRect);
	afx_msg void OnHdnItemchangedFilelist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedCommitAmenddiff();
	afx_msg void OnBnClickedNoautoselectsubmodules();
	afx_msg void OnBnClickedCommitSetDateTime();
	afx_msg void OnBnClickedCheckNewBranch();
};
