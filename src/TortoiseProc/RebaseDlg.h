#pragma once
#include "afxcmn.h"
#include "afxwin.h"
#include "StandAloneDlg.h"
#include "GitStatusListCtrl.h"
#include "SciEdit.h"
#include "SplitterControl.h"
#include "HistoryCombo.h"
// CRebaseDlg dialog
#define IDC_REBASE_TAB 0x1000000

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
	};

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
	virtual LRESULT DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	void DoSize(int delta);
	void AddRebaseAnchor();
	void RemoveAnchor();

	void SetSplitterRange();
	void SaveSplitterPos();
	
	void LoadBranchInfo();

	CRect				m_DlgOrigRect;
	CRect				m_CommitListOrigRect;

public:
   
    afx_msg void OnBnClickedPickAll();
    afx_msg void OnBnClickedSquashAll();
    afx_msg void OnBnClickedEditAll();
    afx_msg void OnBnClickedRebaseSplit();
	afx_msg void OnSize(UINT nType, int cx, int cy);

    CProgressCtrl m_ProgressBar;
    CStatic m_CtrlStatusText;
    BOOL m_bPickAll;
    BOOL m_bSquashAll;
    BOOL m_bEditAll;

	CSplitterControl	m_wndSplitter;
	CMFCTabCtrl m_ctrlTabCtrl;
	CGitStatusListCtrl m_FileListCtrl;
	CSciEdit		   m_LogMessageCtrl;
	CListCtrl		   m_CommitList;

	CHistoryCombo	   m_BranchCtrl;
	CHistoryCombo	   m_UpstreamCtrl;

	REBASE_STAGE	   m_RebaseStage;



};
