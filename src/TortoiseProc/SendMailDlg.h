#pragma once
#include "afxcmn.h"
#include "StandAloneDlg.h"
#include "HyperLink.h"
// CSendMailDlg dialog

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

public:
	CString m_To;
	CString m_CC;
	CString m_Subject;
	BOOL m_bAttachment;
	BOOL m_bBranch;
	CListCtrl m_ctrlList;
	afx_msg void OnBnClickedSendmailCombine();
};
