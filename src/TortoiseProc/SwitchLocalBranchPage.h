#pragma once


// SwitchLocalBranchPage dialog

class SwitchLocalBranchPage : public CPropertyPage
{
	DECLARE_DYNAMIC(SwitchLocalBranchPage)

public:
	SwitchLocalBranchPage();
	virtual ~SwitchLocalBranchPage();

// Dialog Data
	enum { IDD = IDD_SWITCH_LOCAL_BRANCH };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};
