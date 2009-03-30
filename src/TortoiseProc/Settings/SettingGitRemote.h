#pragma once
#include "afxwin.h"
#include "SettingsPropPage.h"
#include "Tooltip.h"
#include "registry.h"
#include "afxwin.h"

// CSettingGitRemote dialog

class CSettingGitRemote : public ISettingsPropPage
{
	DECLARE_DYNAMIC(CSettingGitRemote)

public:
	CSettingGitRemote();
	virtual ~CSettingGitRemote();
	UINT GetIconID() {return IDI_GENERAL;}
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
