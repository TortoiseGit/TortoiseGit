#pragma once


// CSettingSMTP dialog

class CSettingSMTP : public CPropertyPage
{
	DECLARE_DYNAMIC(CSettingSMTP)

public:
	CSettingSMTP();
	virtual ~CSettingSMTP();

// Dialog Data
	enum { IDD = IDD_SETTINGSMTP };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CString m_Server;
	int m_Port;
	CString m_From;
	BOOL m_bAuth;
	CString m_User;
	CString m_Password;
};
