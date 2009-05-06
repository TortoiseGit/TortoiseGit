#pragma once
#include "afxwin.h"


// CConfirmDelRefDlg dialog

class CConfirmDelRefDlg : public CDialog
{
	DECLARE_DYNAMIC(CConfirmDelRefDlg)

public:
	CConfirmDelRefDlg(CString completeRefName, CWnd* pParent = NULL);   // standard constructor
	virtual ~CConfirmDelRefDlg();

// Dialog Data
	enum { IDD = IDD_DIALOG_DELETE_REF };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();

private:
	CString m_completeRefName;
public:
	CButton m_butForce;
	CStatic m_statMessage;
	CButton m_butOK;
protected:
	virtual void OnOK();
	virtual void OnBnForce();
};
