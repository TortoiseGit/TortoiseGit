#pragma once

#include "StandAloneDlg.h"
// CResetDlg dialog

class CResetDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CResetDlg)

public:
	CResetDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CResetDlg();

// Dialog Data
	enum { IDD = IDD_RESET };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()
public:
	int m_ResetType;
	CString m_ResetToVersion;
};
