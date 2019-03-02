// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2016, 2019 - TortoiseGit

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

#include "HorizontalResizableStandAloneDialog.h"
#include "HistoryCombo.h"
#include "MenuButton.h"
#include "registry.h"

// CPushDlg dialog
class CPushDlg : public CHorizontalResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CPushDlg)

public:
	CPushDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CPushDlg();

	// Dialog Data
	enum { IDD = IDD_PUSH };

protected:
	CHistoryCombo	m_BranchRemote;
	CHistoryCombo	m_BranchSource;
	CHistoryCombo	m_Remote;
	CHistoryCombo	m_RemoteURL;
	CMenuButton		m_BrowseLocalRef;
	CComboBox		m_RecurseSubmodulesCombo;
	CRegString		m_RemoteReg;

public:
	CString			m_URL;
	CString			m_BranchSourceName;
	CString			m_BranchRemoteName;

	BOOL			m_bTags;
	BOOL			m_bForce;
	BOOL			m_bForceWithLease;
	BOOL			m_bAutoLoad;
	BOOL			m_bPushAllBranches;
	BOOL			m_bPushAllRemotes;
	BOOL			m_bSetUpstream;
	int				m_RecurseSubmodules;

protected:
	CRegDWORD		m_regPushAllRemotes;
	CRegDWORD		m_regPushAllBranches;
	CRegDWORD		m_regAutoLoad;
	CRegDWORD		m_regRecurseSubmodules;

	BOOL			m_bSetPushRemote;
	BOOL			m_bSetPushBranch;

	virtual BOOL OnInitDialog() override;

	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

	afx_msg void OnBnClickedRd();
	afx_msg void OnCbnSelchangeBranchSource();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedRemoteManage();
	afx_msg void OnBnClickedButtonBrowseSourceBranch();
	afx_msg void OnBnClickedButtonBrowseDestBranch();
	afx_msg void OnBnClickedPushall();
	afx_msg void OnBnClickedForce();
	afx_msg void OnBnClickedForceWithLease();
	afx_msg void OnBnClickedTags();
	afx_msg void OnBnClickedProcPushSetUpstream();
	afx_msg void OnBnClickedProcPushSetPushremote();
	virtual BOOL PreTranslateMessage(MSG* pMsg) override;
	void Refresh();
	void GetRemoteBranch(CString currentBranch);
	void EnDisablePushRemoteArchiveBranch();
};
