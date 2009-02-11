#pragma once
#include "afxcmn.h"
#include "afxwin.h"


// CRebaseDlg dialog

class CRebaseDlg : public CDialog
{
	DECLARE_DYNAMIC(CRebaseDlg)

public:
	CRebaseDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CRebaseDlg();

// Dialog Data
	enum { IDD = IDD_REBASE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
    bool m_bPickupAll;
    bool m_bSquashALL;
    
    afx_msg void OnBnClickedPickAll();
    afx_msg void OnBnClickedSquashAll();
    afx_msg void OnBnClickedEditAll();
    afx_msg void OnBnClickedRebaseSplit();
    CProgressCtrl m_ProgressBar;
    CStatic m_CtrlStatusText;
    BOOL m_bPickAll;
    BOOL m_bSquashAll;
    BOOL m_bEditAll;
};
