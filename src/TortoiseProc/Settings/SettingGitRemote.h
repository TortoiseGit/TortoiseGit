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
	enum
	{
		REMOTE_NAME		=0x1,
		REMOTE_URL		=0x2,
		REMOTE_PUTTYKEY	=0x4,
		REMOTE_AUTOLOAD	=0x8,
	};
	CSettingGitRemote();
	virtual ~CSettingGitRemote();
	UINT GetIconID() {return IDI_GITREMOTE;}
// Dialog Data
	enum { IDD = IDD_SETTINREMOTE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

	BOOL OnInitDialog();
    BOOL OnApply();

protected:

	void Save(CString key, CString value);

	int	 m_ChangedMask;

public:
    CListBox m_ctrlRemoteList;
    CString m_strRemote;
    CString m_strUrl;
    CButton m_bAutoLoad;
    CString m_strPuttyKeyfile;
    afx_msg void OnBnClickedButtonBrowse();
    afx_msg void OnBnClickedButtonAdd();
    afx_msg void OnLbnSelchangeListRemote();
    afx_msg void OnEnChangeEditRemote();
    afx_msg void OnEnChangeEditUrl();
    afx_msg void OnBnClickedCheckIsautoloadputtykey();
    afx_msg void OnEnChangeEditPuttyKey();
};
