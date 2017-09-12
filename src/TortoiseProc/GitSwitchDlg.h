// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2013, 2015-2017 - TortoiseGit

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
#include "ChooseVersion.h"
// CGitSwitchDlg dialog

class CGitSwitchDlg : public CHorizontalResizableStandAloneDialog,public CChooseVersion
{
	DECLARE_DYNAMIC(CGitSwitchDlg)

public:
	CGitSwitchDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CGitSwitchDlg();

// Dialog Data
	enum { IDD = IDD_GITSWITCH };

	BOOL	m_bForce;
	BOOL	m_bMerge;
	BOOL	m_bTrack;
	BOOL	m_bBranch;
	BOOL	m_bBranchOverride;
	CString	m_NewBranch;

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	virtual BOOL OnInitDialog() override;
	afx_msg void OnBnClickedOk();

	afx_msg void OnBnClickedChooseRadioHost();
	afx_msg void OnBnClickedShow();
	afx_msg void OnBnClickedButtonBrowseRefHost(){OnBnClickedButtonBrowseRef();}
	LRESULT OnUpdateGUIHost(WPARAM, LPARAM) { UpdateGUI(); return 0; }

	DECLARE_MESSAGE_MAP()

	afx_msg void OnBnClickedCheckBranch();
	void SetDefaultName(BOOL isUpdateCreateBranch);
	virtual void OnVersionChanged() override;
	afx_msg void OnDestroy();
	afx_msg void OnCbnEditchangeComboboxexVersion();
	afx_msg void OnEnChangeEditBranch();
};
