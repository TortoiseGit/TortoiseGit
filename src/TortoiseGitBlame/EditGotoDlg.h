#pragma once


// CEditGotoDlg dialog

class CEditGotoDlg : public CDialog
{
	DECLARE_DYNAMIC(CEditGotoDlg)

public:
	CEditGotoDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CEditGotoDlg();

// Dialog Data
	enum { IDD = IDD_GOTODLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	DWORD m_LineNumber;
};
