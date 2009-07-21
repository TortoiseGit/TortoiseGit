// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2009 - TortoiseGit

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//

#pragma once
#include "afxcmn.h"
#include "afxwin.h"

#include "StandAloneDlg.h"
#include "HistoryCombo.h"
#include "MenuButton.h"
#include "registry.h"
#include "Balloon.h"
#include "BranchCombox.h"
#include "GitLoglist.h"
// CSyncDlg dialog
#define IDC_SYNC_TAB 0x1000000
#define IDC_OUT_LOGLIST 0x1
#define IDC_OUT_CHANGELIST 0x2
class CSyncDlg : public CResizableStandAloneDialog,public CBranchCombox
{
	DECLARE_DYNAMIC(CSyncDlg)

public:
	CSyncDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSyncDlg();

// Dialog Data
	enum { IDD = IDD_SYNC };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	BRANCH_COMBOX_EVENT_HANDLE();

	CRegDWORD m_regPullButton;
	CRegDWORD m_regPushButton;
	CMFCTabCtrl m_ctrlTabCtrl;
	CBalloon			m_tooltips;
	
	BOOL		m_bInited;
	
	CGitLogList	m_OutLogList;
	CGitLogList m_InLogList;

	CGitStatusListCtrl m_OutChangeFileList;
	CGitStatusListCtrl m_InChangeFileList;
	CGitStatusListCtrl m_ConflictFileList;
	CTGitPathList	m_arOutChangeList;

	virtual void LocalBranchChange(){FetchOutList();};
	virtual void RemoteBranchChange(){FetchOutList();};

	void FetchOutList();

	bool IsURL();

	void SetRemote(CString remote)
	{
		if(!remote.IsEmpty())
		{
			this->m_ctrlURL.AddString(remote);
		}
	}
	
	CString m_OutLocalBranch;
	CString m_OutRemoteBranch;
	
	void ShowProgressCtrl(bool bShow=true);
	void ShowInputCtrl(bool bShow=true);
	void SwitchToRun(){ShowProgressCtrl(true);ShowInputCtrl(false);}
	void SwitchToInput(){ShowProgressCtrl(false);ShowInputCtrl(true);}

	DECLARE_MESSAGE_MAP()
public:
	BOOL m_bAutoLoadPuttyKey;
	
	CHistoryCombo m_ctrlURL;
	CButton m_ctrlDumyButton;
	CMenuButton m_ctrlPull;
	CMenuButton m_ctrlPush;
	CMenuButton m_ctrlStatus;
	afx_msg void OnBnClickedButtonPull();
	afx_msg void OnBnClickedButtonPush();
	afx_msg void OnBnClickedButtonApply();
	afx_msg void OnBnClickedButtonEmail();
	CProgressCtrl m_ctrlProgress;
	CAnimateCtrl m_ctrlAnimate;
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedButtonManage();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnCbenEndeditComboboxexUrl(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnCbnEditchangeComboboxexUrl();
};
