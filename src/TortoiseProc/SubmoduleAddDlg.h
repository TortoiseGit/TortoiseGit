#pragma once
#include "afxcmn.h"


// CSubmoduleAddDlg dialog

class CSubmoduleAddDlg : public CDialog
{
	DECLARE_DYNAMIC(CSubmoduleAddDlg)

public:
	CSubmoduleAddDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSubmoduleAddDlg();

// Dialog Data
	enum { IDD = IDD_SUBMODULE_ADD };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CComboBoxEx m_Repository;
public:
	CComboBoxEx m_PathCtrl;
public:
	BOOL m_bBranch;
public:
	CString m_strBranch;
};
