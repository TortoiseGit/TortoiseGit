#pragma once

#include "SettingsPropPage.h"
#include "Tooltip.h"
#include "registry.h"
#include "afxwin.h"
// CSettingGitConfig dialog

class CSettingGitConfig : public ISettingsPropPage
{
	DECLARE_DYNAMIC(CSettingGitConfig)

public:
	CSettingGitConfig();
	virtual ~CSettingGitConfig();
	UINT GetIconID() {return IDI_GENERAL;}
// Dialog Data
	enum { IDD = IDD_SETTINGIT_CONFIG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	BOOL OnInitDialog();
    BOOL OnApply();

	DECLARE_MESSAGE_MAP()
public:
    CString m_UserName;
    CString m_UserEmail;
    BOOL m_bGlobal;
    afx_msg void OnBnClickedCheckGlobal();
    afx_msg void OnEnChangeGitUsername();
    afx_msg void OnEnChangeGitUseremail();
};
