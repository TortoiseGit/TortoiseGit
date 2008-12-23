#pragma once


// CGitSwitch dialog

class CGitSwitch : public CDialog
{
	DECLARE_DYNAMIC(CGitSwitch)

public:
	CGitSwitch(CWnd* pParent = NULL);   // standard constructor
	virtual ~CGitSwitch();

// Dialog Data
	enum { IDD = IDD_GITSWITCH };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};
