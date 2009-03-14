#pragma once
#include "afxcmn.h"


// CRefLogDlg dialog

class CRefLogDlg : public CDialog
{
	DECLARE_DYNAMIC(CRefLogDlg)

public:
	CRefLogDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CRefLogDlg();

// Dialog Data
	enum { IDD = IDD_REFLOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CComboBoxEx m_ChooseRef;
public:
	CListCtrl m_RefList;
public:
	afx_msg void OnBnClickedOk();
};
