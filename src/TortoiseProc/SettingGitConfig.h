#pragma once


// CSettingGitConfig dialog

class CSettingGitConfig : public CPropertyPage
{
	DECLARE_DYNAMIC(CSettingGitConfig)

public:
	CSettingGitConfig();
	virtual ~CSettingGitConfig();

// Dialog Data
	enum { IDD = IDD_SETTINGIT_CONFIG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
    CString m_UserName;
    CString m_UserEmail;
    BOOL m_bGlobal;
};
