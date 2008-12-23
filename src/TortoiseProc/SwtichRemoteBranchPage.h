#pragma once


// CSwtichRemoteBranchPage dialog

class CSwtichRemoteBranchPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CSwtichRemoteBranchPage)

public:
	CSwtichRemoteBranchPage();
	virtual ~CSwtichRemoteBranchPage();

// Dialog Data
	enum { IDD = IDD_SWITCH_REMOTEL_BRANCH };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};
