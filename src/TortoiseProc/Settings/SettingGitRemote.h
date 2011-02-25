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
	};
	CSettingGitRemote(CString cmdPath);
	virtual ~CSettingGitRemote();
	UINT GetIconID() {return IDI_GITREMOTE;}
// Dialog Data
	enum { IDD = IDD_SETTINREMOTE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

	afx_msg void OnBnClickedButtonBrowse();
	afx_msg void OnBnClickedButtonAdd();
	afx_msg void OnLbnSelchangeListRemote();
	afx_msg void OnEnChangeEditRemote();
	afx_msg void OnEnChangeEditUrl();
	afx_msg void OnEnChangeEditPuttyKey();
	afx_msg void OnBnClickedButtonRemove();

	BOOL OnInitDialog();
	BOOL OnApply();

	BOOL IsRemoteExist(CString &remote);

	void Save(CString key, CString value);

	int			m_ChangedMask;

	CString		m_cmdPath;

	CListBox	m_ctrlRemoteList;
	CString		m_strRemote;
	CString		m_strUrl;

	CString		m_strPuttyKeyfile;
};
