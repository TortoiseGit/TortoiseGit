#pragma once

#include "StandAloneDlg.h"
#include "HistoryCombo.h"
#include "HyperLink.h"
#include "registry.h"
// CPullFetchDlg dialog

class CPullFetchDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CPullFetchDlg)

public:
	CPullFetchDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CPullFetchDlg();

// Dialog Data
	enum { IDD = IDD_PULLFETCH };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	CHistoryCombo	m_Remote;
	CHistoryCombo	m_Other;
	CHistoryCombo	m_RemoteBranch;
	virtual BOOL OnInitDialog();
	CRegString m_RemoteReg;
	
	DECLARE_MESSAGE_MAP()
public:
	BOOL        m_IsPull;
    BOOL        m_bAutoLoad;
	BOOL		m_bRebase;
    BOOL        m_bAutoLoadEnable;
    CHyperLink  m_RemoteManage;

	afx_msg void OnBnClickedRd();
	afx_msg void OnBnClickedOk();
	CString m_RemoteURL;
	CString m_RemoteBranchName;
    afx_msg void OnStnClickedRemoteManage();
	afx_msg void OnBnClickedButtonBrowseRef();

	CString		m_configPullRemote;
	CString		m_configPullBranch;
};
