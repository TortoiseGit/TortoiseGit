#pragma once
#include "afxcmn.h"
#include "StandAloneDlg.h"
#include "HistoryCombo.h"
#include "RefLoglist.h"
// CRefLogDlg dialog

class CRefLogDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CRefLogDlg)

public:
	CRefLogDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CRefLogDlg();

// Dialog Data
	enum { IDD = IDD_REFLOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	BOOL OnInitDialog();
	afx_msg void OnCbnSelchangeRef();	
	afx_msg LRESULT OnRefLogChanged(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
public:
	CHistoryCombo m_ChooseRef;
	CString m_CurrentBranch;
public:
	CRefLogList m_RefList;
public:
	afx_msg void OnBnClickedOk();
};
