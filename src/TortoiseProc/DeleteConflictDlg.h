#pragma once

#include "StandAloneDlg.h"
// CDeleteConflictDlg dialog

class CDeleteConflictDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CDeleteConflictDlg)

public:
	CDeleteConflictDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDeleteConflictDlg();

// Dialog Data
	enum { IDD = IDD_RESOLVE_CONFLICT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()

public:
	CString m_LocalStatus;
public:
	CString m_RemoteStatus;
	BOOL	m_bShowModifiedButton;
	CString m_File;
	BOOL	m_bIsDelete;
public:
	afx_msg void OnBnClickedDelete();
public:
	afx_msg void OnBnClickedModify();
};
