#pragma once
#include "afxcmn.h"
#include "afxwin.h"
#include "StandAloneDlg.h"
#include "GitStatusListCtrl.h"
#include "SciEdit.h"
// CRebaseDlg dialog

class CRebaseDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CRebaseDlg)

public:
	CRebaseDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CRebaseDlg();

// Dialog Data
	enum { IDD = IDD_REBASE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
public:
   
    afx_msg void OnBnClickedPickAll();
    afx_msg void OnBnClickedSquashAll();
    afx_msg void OnBnClickedEditAll();
    afx_msg void OnBnClickedRebaseSplit();
    CProgressCtrl m_ProgressBar;
    CStatic m_CtrlStatusText;
    BOOL m_bPickAll;
    BOOL m_bSquashAll;
    BOOL m_bEditAll;

	CMFCTabCtrl m_ctrlTabCtrl;
	CGitStatusListCtrl m_FileListCtrl;
	CSciEdit		   m_LogMessageCtrl;
};
