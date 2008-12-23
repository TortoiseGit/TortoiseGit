#pragma once


// CSwitchVersionPage dialog

class CSwitchVersionPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CSwitchVersionPage)

public:
	CSwitchVersionPage();
	virtual ~CSwitchVersionPage();

// Dialog Data
	enum { IDD = IDD_SWITCH_VERSION };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};
