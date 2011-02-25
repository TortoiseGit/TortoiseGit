#pragma once

#include "StandAloneDlg.h"
#include "HistoryCombo.h"
#include "registry.h"
// CPushDlg dialog

class CPushDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CPushDlg)

public:
	CPushDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CPushDlg();

	// Dialog Data
	enum { IDD = IDD_PUSH };

protected:
	CHistoryCombo	m_BranchRemote;
	CHistoryCombo	m_BranchSource;
	CHistoryCombo	m_Remote;
	CHistoryCombo	m_RemoteURL;

public:
	CString			m_URL;
	CString			m_BranchSourceName;
	CString			m_BranchRemoteName;
	CRegString		m_RemoteReg;

	BOOL			m_bTags;
	BOOL			m_bForce;
	BOOL			m_bPack;
	BOOL			m_bAutoLoad;

protected:
	CRegDWORD		m_regAutoLoad;

	virtual BOOL OnInitDialog();

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

	afx_msg void OnBnClickedRd();
	afx_msg void OnCbnSelchangeBranchSource();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedRemoteManage();
	afx_msg void OnBnClickedButtonBrowseSourceBranch();
	afx_msg void OnBnClickedButtonBrowseDestBranch();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	void Refresh();
};
