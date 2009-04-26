#pragma once


// CAddRemoteDlg dialog

class CAddRemoteDlg : public CDialog
{
	DECLARE_DYNAMIC(CAddRemoteDlg)

public:
	CAddRemoteDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CAddRemoteDlg();

// Dialog Data
	enum { IDD = IDD_ADD_REMOTE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CString m_Name;
	CString m_Url;
	afx_msg void OnBnClickedOk();
};
