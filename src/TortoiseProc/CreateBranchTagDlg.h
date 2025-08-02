﻿// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2013, 2015-2017, 2021-2023, 2025 - TortoiseGit

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
#include "ChooseVersion.h"

// CCreateBranchTagDlg dialog

class CCreateBranchTagDlg : public CResizableStandAloneDialog,public CChooseVersion
{
	DECLARE_DYNAMIC(CCreateBranchTagDlg)

public:
	CCreateBranchTagDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CCreateBranchTagDlg();

// Dialog Data
	enum { IDD = IDD_NEW_BRANCH_TAG };

	BOOL	m_bForce;
	BOOL	m_bTrack;
	BOOL	m_bIsTag;
	BOOL	m_bSwitch;
	BOOL	m_bSign;
	BOOL	m_bPush;

	CString	m_BranchTagName;
	CString	m_Message;
	CString	m_OldSelectBranch;

protected:
	void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	BOOL OnInitDialog() override;

	CHOOSE_EVENT_RADIO();

	DECLARE_MESSAGE_MAP()

	afx_msg void OnBnClickedOk();
	afx_msg void OnCbnSelchangeComboboxexBranch();
	afx_msg void OnEnChangeBranchTag();

	void OnVersionChanged() override;
	afx_msg void OnDestroy();

	CRegDWORD m_regNewBranch;
	CRegDWORD m_regPushTag;
};
