// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2009,2012 - TortoiseGit

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
#include "BranchCombox.h"
#include "GitLoglist.h"
#include "Win7.h"

// CSyncDlg dialog
#define IDC_SYNC_TAB 0x1000000

#define IDC_CMD_LOG			0x1
#define IDC_IN_LOGLIST		0x2
#define IDC_IN_CHANGELIST	0x3
#define IDC_IN_CONFLICT		0x4
#define IDC_OUT_LOGLIST		0x5
#define IDC_OUT_CHANGELIST	0x6

#define IDT_INPUT		108

class CSyncDlg : public CResizableStandAloneDialog,public CBranchCombox
{
	DECLARE_DYNAMIC(CSyncDlg)

public:
	CSyncDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSyncDlg();

// Dialog Data
	enum { IDD = IDD_SYNC };

	enum {	GIT_COMMAND_PUSH,
			GIT_COMMAND_PULL,
			GIT_COMMAND_FETCH,
			GIT_COMMAND_FETCHANDREBASE,
			GIT_COMMAND_SUBMODULE,
			GIT_COMMAND_REMOTE
		};
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	BRANCH_COMBOX_EVENT_HANDLE();

	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedButtonManage();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnCbnEditchangeComboboxex();
	afx_msg void OnBnClickedButtonPull();
	afx_msg void OnBnClickedButtonPush();
	afx_msg void OnBnClickedButtonApply();
	afx_msg void OnBnClickedButtonEmail();
	afx_msg void OnTimer(UINT_PTR nIDEvent);

	afx_msg LRESULT	OnTaskbarBtnCreated(WPARAM wParam, LPARAM lParam);
	CComPtr<ITaskbarList3>	m_pTaskbarList;

	int					m_CurrentCmd;

	CRegDWORD			m_regPullButton;
	CRegDWORD			m_regPushButton;
	CRegDWORD			m_regSubmoduleButton;
	CRegDWORD			m_regAutoLoadPutty;

	CMFCTabCtrl			m_ctrlTabCtrl;

	CToolTips		m_tooltips;

	BOOL				m_bInited;

	CGitLogList			m_OutLogList;
	CGitLogList			m_InLogList;

	CGitStatusListCtrl	m_OutChangeFileList;
	CGitStatusListCtrl	m_InChangeFileList;
	CGitStatusListCtrl	m_ConflictFileList;

	CRichEditCtrl		m_ctrlCmdOut;

	CTGitPathList		m_arOutChangeList;
	CTGitPathList		m_arInChangeList;

	int					m_CmdOutCurrentPos;

	CWinThread*			m_pThread;

	volatile LONG		m_bBlock;

	void				ParserCmdOutput(char ch);

	virtual void LocalBranchChange(){FetchOutList();};
	virtual void RemoteBranchChange(){};

	void ShowTab(int windowid)
	{
		this->m_ctrlTabCtrl.ShowTab(windowid-1);
		this->m_ctrlTabCtrl.SetActiveTab(windowid-1);
	}

	void FetchOutList(bool force=false);

	bool IsURL();

	void SetRemote(CString remote)
	{
		if(!remote.IsEmpty())
		{
			int index=this->m_ctrlURL.FindStringExact(0,remote);
			if(index>=0)
			{
				m_ctrlURL.SetCurSel(index);
				return;
			}
			this->m_ctrlURL.AddString(remote);
		}
	}

	std::vector<CString> m_GitCmdList;

	bool			m_bAbort;

	int				m_GitCmdStatus;

	CStringA		m_LogText;
	CString			m_OutLocalBranch;
	CString			m_OutRemoteBranch;

	CGitHash		m_oldHash;

	void ShowProgressCtrl(bool bShow=true);
	void ShowInputCtrl(bool bShow=true);
	void SwitchToRun(){ShowProgressCtrl(true);ShowInputCtrl(false);EnableControlButton(false);}
	void SwitchToInput(){ShowProgressCtrl(false);ShowInputCtrl(true);}

	LRESULT OnProgressUpdateUI(WPARAM wParam,LPARAM lParam);

	void UpdateCombox()
	{
		this->m_strLocalBranch = this->m_ctrlLocalBranch.GetString();
		this->m_ctrlRemoteBranch.GetWindowText(this->m_strRemoteBranch);
		this->m_ctrlURL.GetWindowText(this->m_strURL);
		m_strRemoteBranch=m_strRemoteBranch.Trim();
	}

	void AddDiffFileList(CGitStatusListCtrl *pCtrlList, CTGitPathList *pGitList,
							CString &rev1,CString &rev2)
	{
		g_Git.GetCommitDiffList(rev1,rev2,*pGitList);
		pCtrlList->m_Rev1=rev1;
		pCtrlList->m_Rev2=rev2;
		pCtrlList->Show(0,*pGitList);
		pCtrlList->SetEmptyString(CString(_T("No changed file")));
		return;
	}

	void PullComplete();
	void FetchComplete();

	DECLARE_MESSAGE_MAP()

public:
	BOOL			m_bAutoLoadPuttyKey;
	BOOL			m_bForce;
	CString			m_strURL;

protected:
	int				m_Gitverion;

	static UINT		ProgressThreadEntry(LPVOID pVoid){ return ((CSyncDlg*)pVoid) ->ProgressThread(); };
	UINT			ProgressThread();

	CHistoryCombo	m_ctrlURL;
	CButton			m_ctrlDumyButton;
	CMenuButton		m_ctrlPull;
	CMenuButton		m_ctrlPush;
	CStatic			m_ctrlStatus;
	CMenuButton		m_ctrlSubmodule;
	CProgressCtrl	m_ctrlProgress;
	CAnimateCtrl	m_ctrlAnimate;

	void EnableControlButton(bool bEnabled=true);
	afx_msg void OnBnClickedButtonCommit();

	virtual void OnOK();
	void	Refresh();

	afx_msg void OnBnClickedButtonSubmodule();
	afx_msg void OnLvnInLogListColumnClick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedCheckForce();
	afx_msg void OnBnClickedLog();
};
