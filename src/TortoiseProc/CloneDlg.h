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
	CString m_ModuleName;
	CString m_OldURL;

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
	BOOL	m_bSVN;
	BOOL	m_bSVNTrunk;
	BOOL	m_bSVNTags;
	BOOL	m_bSVNBranch;
	BOOL	m_bSVNFrom;

	CString	m_strSVNTrunk;
	CString m_strSVNTags;
	CString m_strSVNBranchs;
	int m_nSVNFrom;

    afx_msg void OnBnClickedPuttykeyfileBrowse();
    afx_msg void OnBnClickedPuttykeyAutoload();
	afx_msg void OnCbnSelchangeUrlcombo();
	afx_msg void OnCbenBegineditUrlcombo(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnCbenEndeditUrlcombo(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnCbnEditchangeUrlcombo();

	afx_msg void OnBnClickedCheckSvn();
	afx_msg void OnBnClickedCheckSvnTrunk();
	afx_msg void OnBnClickedCheckSvnTag();
	afx_msg void OnBnClickedCheckSvnBranch();
	afx_msg void OnBnClickedCheckSvnFrom();
};
