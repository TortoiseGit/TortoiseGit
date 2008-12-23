#pragma once


// CSwitchTagPage dialog

class CSwitchTagPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CSwitchTagPage)

public:
	CSwitchTagPage();
	virtual ~CSwitchTagPage();

// Dialog Data
	enum { IDD = IDD_SWITCH_TAG1 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};
