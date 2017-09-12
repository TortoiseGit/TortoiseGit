// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2014 - TortoiseGit

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
// CResetDlg dialog

class CResetDlg : public CHorizontalResizableStandAloneDialog, public CChooseVersion
{
	DECLARE_DYNAMIC(CResetDlg)

public:
	CResetDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CResetDlg();

// Dialog Data
	enum { IDD = IDD_RESET };

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	virtual BOOL OnInitDialog() override;
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedChooseRadioHost();
	afx_msg void OnBnClickedShow();
	afx_msg void OnBnClickedButtonBrowseRefHost(){OnBnClickedButtonBrowseRef();}
	LRESULT OnUpdateGUIHost(WPARAM, LPARAM) { UpdateGUI(); return 0; }
	virtual void OnVersionChanged() override;
	virtual void OnOK() override;
	afx_msg void OnBnClickedShowModifiedFiles();

	DECLARE_MESSAGE_MAP()
public:
	int m_ResetType;
	CString m_ResetToVersion;
};
