#pragma once
#include "afxcmn.h"
#include "StandAloneDlg.h"
#include "HyperLink.h"
// CSendMailDlg dialog
#include "ACEdit.h"
#include "RegHistory.h"
#include "TGitPath.h"

class CSendMailDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CSendMailDlg)

public:
	CSendMailDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSendMailDlg();

// Dialog Data
	enum { IDD = IDD_SENDMAIL };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()

	CHyperLink  m_SmtpSetup;

	CACEdit		m_ctrlCC;
	CACEdit		m_ctrlTO;
	CRegHistory m_AddressReg;
public:
	CString m_To;
	CString m_CC;
	CString m_Subject;
	BOOL m_bAttachment;
	BOOL m_bCombine;
	CListCtrl m_ctrlList;
	CTGitPathList m_PathList;

	afx_msg void OnBnClickedSendmailCombine();
	afx_msg void OnBnClickedOk();
	afx_msg void OnLvnItemchangedSendmailPatchs(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkSendmailPatchs(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnEnChangeSendmailSubject();
};
