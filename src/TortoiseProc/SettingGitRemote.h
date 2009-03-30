#pragma once
#include "afxwin.h"


// CSettingGitRemote dialog

class CSettingGitRemote : public CPropertyPage
{
	DECLARE_DYNAMIC(CSettingGitRemote)

public:
	CSettingGitRemote();
	virtual ~CSettingGitRemote();

// Dialog Data
	enum { IDD = IDD_SETTINREMOTE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
    CListBox m_ctrlRemoteList;
    CString m_strRemote;
    CString m_strUrl;
    CButton m_bAutoLoad;
    CString m_strPuttyKeyfile;
    afx_msg void OnBnClickedButtonBrowse();
    afx_msg void OnBnClickedButtonAdd();
    afx_msg void OnLbnSelchangeListRemote();
};
