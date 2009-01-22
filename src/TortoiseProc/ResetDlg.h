#pragma once


// CResetDlg dialog

class CResetDlg : public CDialog
{
	DECLARE_DYNAMIC(CResetDlg)

public:
	CResetDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CResetDlg();

// Dialog Data
	enum { IDD = IDD_RESET };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
    int m_ResetType;
};
