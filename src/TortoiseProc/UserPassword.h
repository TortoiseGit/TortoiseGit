#pragma once


// CUserPassword dialog

class CUserPassword : public CDialog
{
	DECLARE_DYNAMIC(CUserPassword)

public:
	CUserPassword(CWnd* pParent = NULL);   // standard constructor
	virtual ~CUserPassword();

// Dialog Data
	enum { IDD = IDD_USER_PASSWD };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CString m_UserName;
	CString m_Password;
	CString m_URL;
	virtual BOOL OnInitDialog();
};
