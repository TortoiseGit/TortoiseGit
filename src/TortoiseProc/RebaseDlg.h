// TortoiseGit - a Windows shell extension for easy version control

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
#include "afxcmn.h"
#include "afxwin.h"
#include "StandAloneDlg.h"
#include "GitStatusListCtrl.h"
#include "SciEdit.h"
#include "SplitterControl.h"
#include "HistoryCombo.h"
#include "GitLogList.h"
#include "MenuButton.h"
#include "Win7.h"
#include "Tooltip.h"

// CRebaseDlg dialog
#define IDC_REBASE_TAB 0x1000000

#define REBASE_TAB_CONFLICT	0
#define REBASE_TAB_MESSAGE	1
#define REBASE_TAB_LOG		2

#define MSG_REBASE_UPDATE_UI	(WM_USER+151)

class CRebaseDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CRebaseDlg)

public:
	CRebaseDlg(CWnd* pParent = NULL); // standard constructor
	virtual ~CRebaseDlg();

// Dialog Data
	enum { IDD = IDD_REBASE };

	enum REBASE_STAGE
	{
		CHOOSE_BRANCH,
		CHOOSE_COMMIT_PICK_MODE,
		REBASE_START,
		REBASE_CONTINUE,
		REBASE_ABORT,
		REBASE_FINISH,
		REBASE_CONFLICT,
		REBASE_EDIT,
		REBASE_SQUASH_EDIT,
		REBASE_SQUASH_CONFLICT,
		REBASE_DONE,
	};

protected:
	virtual void DoDataExchange(CDataExchange* pDX); // DDX/DDV support
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
	virtual LRESULT DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT OnRebaseUpdateUI(WPARAM wParam, LPARAM lParam);
	void DoSize(int delta);
	void AddRebaseAnchor();
	void RemoveAnchor();

	void SetSplitterRange();
	void SaveSplitterPos();

	void LoadBranchInfo();
	void FetchLogList();
	void SetAllRebaseAction(int action);
	void OnCancel();

	CRect m_DlgOrigRect;
	CRect m_CommitListOrigRect;
	BOOL PreTranslateMessage(MSG* pMsg);
	bool LogListHasFocus(HWND hwnd);

	CSciEdit m_wndOutputRebase;
	void SetContinueButtonText();
	void SetControlEnable();
	void UpdateProgress();
	void UpdateCurrentStatus();
	void ListConflictFile();
	int  DoRebase();
	void Refresh();
	volatile LONG m_bThreadRunning;
	int  RebaseThread();
	static UINT RebaseThreadEntry(LPVOID pVoid){return ((CRebaseDlg *)pVoid)->RebaseThread();};
	BOOL IsEnd();

	BOOL m_IsFastForward;

	CGitHash m_OrigBranchHash;
	CGitHash m_OrigUpstreamHash;

	int VerifyNoConflict();
	CString GetRebaseModeName(int rebasemode);

	CString m_SquashMessage;

	int CheckNextCommitIsSquash();
	int GetCurrentCommitID();
	int FinishRebase();

	CMenuButton m_PostButton;

	afx_msg void OnBnClickedPickAll();
	afx_msg void OnBnClickedSquashAll();
	afx_msg void OnBnClickedEditAll();
	afx_msg void OnBnClickedRebaseSplit();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnCbnSelchangeBranch();
	afx_msg void OnCbnSelchangeUpstream();
	afx_msg void OnBnClickedContinue();
	afx_msg void OnBnClickedAbort();
	afx_msg void OnLvnItemchangedLoglist(NMHDR *pNMHDR, LRESULT *pResult);
	void FillLogMessageCtrl();

	CProgressCtrl		m_ProgressBar;
	CStatic				m_CtrlStatusText;
	CToolTips			m_tooltips;

	CString				m_PreCmd;

	BOOL				m_bPickAll;
	BOOL				m_bSquashAll;
	BOOL				m_bEditAll;

	BOOL				m_bForce;
	BOOL				m_bAddCherryPickedFrom;

public:
	CStringArray		m_PostButtonTexts;
	CGitLogList			m_CommitList;

	CString				m_Upstream;
	CString				m_Branch;

	BOOL				m_IsCherryPick;

protected:
	CSplitterControl	m_wndSplitter;
	CMFCTabCtrl			m_ctrlTabCtrl;
	CGitStatusListCtrl	m_FileListCtrl;
	CSciEdit			m_LogMessageCtrl;

	CHistoryCombo		m_BranchCtrl;
	CHistoryCombo		m_UpstreamCtrl;

	REBASE_STAGE		m_RebaseStage;

	void AddBranchToolTips(CHistoryCombo *pBranch);
	void AddLogString(CString str);
	int StartRebase();
	int CheckRebaseCondition();
	int m_CurrentRebaseIndex;
	int StateAction();
	int GoNext();
	afx_msg void OnBnClickedButtonBrowse();
	afx_msg void OnBnClickedRebaseCheckForce();
	afx_msg void OnBnClickedCheckCherryPickedFrom();
	afx_msg void OnBnClickedRebasePostButton();
	afx_msg void OnBnClickedButtonUp2();
	afx_msg void OnBnClickedButtonDown2();

	afx_msg LRESULT	OnTaskbarBtnCreated(WPARAM wParam, LPARAM lParam);
	CComPtr<ITaskbarList3>	m_pTaskbarList;
};
