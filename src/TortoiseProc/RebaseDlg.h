#pragma once
#include "afxcmn.h"
#include "afxwin.h"
#include "StandAloneDlg.h"
#include "GitStatusListCtrl.h"
#include "SciEdit.h"
#include "SplitterControl.h"
#include "HistoryCombo.h"
#include "Balloon.h"
#include "GitLogList.h"
// CRebaseDlg dialog
#define IDC_REBASE_TAB 0x1000000

#define REBASE_TAB_CONFLICT  0
#define REBASE_TAB_MESSAGE   1
#define REBASE_TAB_LOG		 2

#define MSG_REBASE_UPDATE_UI	(WM_USER+151)

class CRebaseDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CRebaseDlg)

public:
	CRebaseDlg(CWnd* pParent = NULL);   // standard constructor
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
	};

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
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

	CRect				m_DlgOrigRect;
	CRect				m_CommitListOrigRect;
	BOOL PreTranslateMessage(MSG* pMsg);

	CSciEdit m_wndOutputRebase;
	void SetContinueButtonText();
	void SetControlEnable();
	void UpdateProgress();
	void UpdateCurrentStatus();
	void ListConflictFile();
	int  DoRebase();
	volatile LONG 		m_bThreadRunning;
	int  RebaseThread();
	static UINT RebaseThreadEntry(LPVOID pVoid){return ((CRebaseDlg *)pVoid)->RebaseThread();};
	BOOL IsEnd();

public:
   
    afx_msg void OnBnClickedPickAll();
    afx_msg void OnBnClickedSquashAll();
    afx_msg void OnBnClickedEditAll();
    afx_msg void OnBnClickedRebaseSplit();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnCbnSelchangeBranch();
	afx_msg void OnCbnSelchangeUpstream();
	afx_msg void OnBnClickedContinue();

    CProgressCtrl m_ProgressBar;
    CStatic m_CtrlStatusText;
	CBalloon			m_tooltips;

    BOOL m_bPickAll;
    BOOL m_bSquashAll;
    BOOL m_bEditAll;

	CSplitterControl	m_wndSplitter;
	CMFCTabCtrl m_ctrlTabCtrl;
	CGitStatusListCtrl m_FileListCtrl;
	CSciEdit		   m_LogMessageCtrl;
	
	CGitLogList		   m_CommitList;

	CHistoryCombo m_BranchCtrl;
	CHistoryCombo m_UpstreamCtrl;

	REBASE_STAGE	   m_RebaseStage;

	void AddBranchToolTips(CHistoryCombo *pBranch);
	void AddLogString(CString str);
	int	 StartRebase();
	int  CheckRebaseCondition();
	int  m_CurrentRebaseIndex;
	int  StateAction();
	int  GoNext();
};
