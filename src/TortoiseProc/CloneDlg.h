#pragma once

#include "StandAloneDlg.h"
#include "HistoryCombo.h"



// CCloneDlg dialog

class CCloneDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNCREATE(CCloneDlg)

public:
	CCloneDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CCloneDlg();
// Overrides
	virtual void OnOK();
	virtual void OnCancel();

// Dialog Data
	enum { IDD = IDD_CLONE};

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBnClickedCloneBrowseUrl();
	afx_msg void OnBnClickedCloneDirBrowse();
	afx_msg void OnEnChangeCloneDir();
	CString m_Directory;
	CHistoryCombo	m_URLCombo;
    CHistoryCombo   m_PuttyKeyCombo;
    CString m_strPuttyKeyFile;
	CString m_URL;
    BOOL    m_bAutoloadPuttyKeyFile;
    afx_msg void OnBnClickedPuttykeyfileBrowse();
    afx_msg void OnBnClickedPuttykeyAutoload();
};
