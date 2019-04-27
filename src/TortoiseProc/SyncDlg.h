// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2009, 2012-2019 - TortoiseGit

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
#include "StandAloneDlg.h"
#include "HistoryCombo.h"
#include "MenuButton.h"
#include "registry.h"
#include "BranchCombox.h"
#include "GitLoglist.h"
#include "GitProgressList.h"
#include "GitRefCompareList.h"
#include "GitTagCompareList.h"
#include "SyncTabCtrl.h"
#include "GestureEnabledControl.h"

// CSyncDlg dialog
#define IDC_SYNC_TAB 0x1000000

#define IDC_CMD_LOG			0x1
#define IDC_IN_LOGLIST		0x2
#define IDC_IN_CHANGELIST	0x3
#define IDC_IN_CONFLICT		0x4
#define IDC_OUT_LOGLIST		0x5
#define IDC_OUT_CHANGELIST	0x6
#define IDC_CMD_GIT_PROG	0x7
#define IDC_REFLIST			0x8
#define IDC_TAGCOMPARELIST	0x9

#define IDT_INPUT		108

class CSyncDlg : public CResizableStandAloneDialog,public CBranchCombox
{
	DECLARE_DYNAMIC(CSyncDlg)

public:
	CSyncDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CSyncDlg();

// Dialog Data
	enum { IDD = IDD_SYNC };

	enum {	GIT_COMMAND_PUSH,
			GIT_COMMAND_PULL,
			GIT_COMMAND_FETCH,
			GIT_COMMAND_FETCHANDREBASE,
			GIT_COMMAND_SUBMODULE,
			GIT_COMMAND_REMOTE,
			GIT_COMMAND_STASH
		};
protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	BRANCH_COMBOX_EVENT_HANDLE();

	virtual BOOL OnInitDialog() override;
	afx_msg void OnBnClickedButtonManage();
	virtual BOOL PreTranslateMessage(MSG* pMsg) override;
	afx_msg void OnCbnEditchangeComboboxex();
	afx_msg void OnBnClickedButtonPull();
	afx_msg void OnBnClickedButtonPush();
	afx_msg void OnBnClickedButtonApply();
	afx_msg void OnBnClickedButtonEmail();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnDestroy();
	afx_msg LRESULT OnThemeChanged();

	afx_msg LRESULT	OnTaskbarBtnCreated(WPARAM wParam, LPARAM lParam);
	CComPtr<ITaskbarList3>	m_pTaskbarList;

	int					m_CurrentCmd;

	CRegDWORD			m_regPullButton;
	CRegDWORD			m_regPushButton;
	CRegDWORD			m_regSubmoduleButton;
	CRegDWORD			m_regAutoLoadPutty;

	CSyncTabCtrl		m_ctrlTabCtrl;

	BOOL				m_bInited;

	CGitLogList			m_OutLogList;
	CGitLogList			m_InLogList;
	CGitProgressList	m_GitProgressList;
	std::unique_ptr<ProgressCommand>	progressCommand;

	CGitStatusListCtrl	m_OutChangeFileList;
	CGitStatusListCtrl	m_InChangeFileList;
	CGitStatusListCtrl	m_ConflictFileList;
	ProjectProperties	m_ProjectProperties;

	CGestureEnabledControlTmpl<CRichEditCtrl>	m_ctrlCmdOut;
	CGitRefCompareList	m_refList;
	CGitTagCompareList	m_tagCompareList;

	CTGitPathList		m_arOutChangeList;
	CTGitPathList		m_arInChangeList;

	int					m_CmdOutCurrentPos;

	CWinThread*			m_pThread;

	volatile LONG		m_bBlock;
	CGitGuardedByteArray	m_Databuf;
	int					m_BufStart;

	void				ParserCmdOutput(char ch);

	virtual void LocalBranchChange() override { FetchOutList(); };
	virtual void RemoteBranchChange() override {};

	void ShowTab(int windowid)
	{
		this->m_ctrlTabCtrl.ShowTab(windowid-1);
		this->m_ctrlTabCtrl.SetActiveTab(windowid-1);
	}

	void FetchOutList(bool force=false);

	bool IsURL();

	virtual void SetRemote(CString remote) override
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
	STRING_VECTOR	m_remotelist;

	volatile bool	m_bAbort;
	bool			m_bDone;
	ULONGLONG		m_startTick;
	bool			m_bWantToExit;

	int				m_GitCmdStatus;

	CStringA		m_LogText;
	CString			m_OutLocalBranch;
	CString			m_OutRemoteBranch;

	CGitHash		m_oldHash;
	CGitHash		m_oldRemoteHash;
	MAP_HASH_NAME	m_oldHashMap;
	MAP_HASH_NAME	m_newHashMap;

	void ShowProgressCtrl(bool bShow=true);
	void ShowInputCtrl(bool bShow=true);
	void SwitchToRun(){ShowProgressCtrl(true);ShowInputCtrl(false);EnableControlButton(false);}
	void SwitchToInput(){ShowProgressCtrl(false);ShowInputCtrl(true);}

	LRESULT OnProgressUpdateUI(WPARAM wParam,LPARAM lParam);
	LRESULT OnProgCmdFinish(WPARAM wParam, LPARAM lParam);
	void FillNewRefMap();
	void RunPostAction();
	void UpdateCombox()
	{
		this->m_strLocalBranch = this->m_ctrlLocalBranch.GetString();
		this->m_ctrlRemoteBranch.GetWindowText(this->m_strRemoteBranch);
		this->m_ctrlURL.GetWindowText(this->m_strURL);
		m_strRemoteBranch=m_strRemoteBranch.Trim();
	}

	void AddDiffFileList(CGitStatusListCtrl *pCtrlList, CTGitPathList *pGitList,
							CGitHash& rev1, CGitHash& rev2)
	{
		g_Git.GetCommitDiffList(rev1.ToString(), rev2.ToString(), *pGitList);
		pCtrlList->m_Rev1 = rev1;
		pCtrlList->m_Rev2 = rev2;
		pCtrlList->SetEmptyString(CString(MAKEINTRESOURCE(IDS_COMPAREREV_NODIFF)));
		pCtrlList->UpdateWithGitPathList(*pGitList);
		pCtrlList->Show(GITSLC_SHOWALL);
	}

	void ShowInCommits(const CString& friendname);
	void PullComplete();
	void FetchComplete();
	void StashComplete();

	DECLARE_MESSAGE_MAP()

public:
	BOOL			m_bAutoLoadPuttyKey;
	BOOL			m_bForce;
	CString			m_strURL;
	int				m_seq;

protected:
	static UINT		ProgressThreadEntry(LPVOID pVoid) { return static_cast<CSyncDlg*>(pVoid)->ProgressThread(); };
	UINT			ProgressThread();

	void			StartWorkerThread();

	CHistoryCombo	m_ctrlURL;
	CButton			m_ctrlDumyButton;
	CMenuButton		m_ctrlPull;
	CMenuButton		m_ctrlPush;
	CStatic			m_ctrlStatus;
	CMenuButton		m_ctrlSubmodule;
	CMenuButton		m_ctrlStash;
	CProgressCtrl	m_ctrlProgress;
	CAnimateCtrl	m_ctrlAnimate;
	CStatic			m_ctrlProgLabel;
	int				m_iPullRebase;

	void EnableControlButton(bool bEnabled=true);
	afx_msg void OnBnClickedButtonCommit();

	virtual void	OnOK() override;
	virtual void	OnCancel() override;
	void	Refresh();
	bool	AskSetTrackedBranch();

	afx_msg void OnBnClickedButtonSubmodule();
	afx_msg void OnBnClickedButtonStash();
	afx_msg void OnBnClickedCheckForce();
	afx_msg void OnBnClickedLog();
	afx_msg void OnEnscrollLog();
	afx_msg void OnEnLinkLog(NMHDR* pNMHDR, LRESULT* pResult);

	friend class CSyncTabCtrl;
};
