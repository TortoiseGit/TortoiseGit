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
#include "HistoryCombo.h"
// CSubmoduleAddDlg dialog

class CSubmoduleAddDlg : public CHorizontalResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CSubmoduleAddDlg)

public:
	CSubmoduleAddDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CSubmoduleAddDlg();
	BOOL OnInitDialog();
// Dialog Data
	enum { IDD = IDD_SUBMODULE_ADD };

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	afx_msg void OnRepBrowse();
	afx_msg void OnPathBrowse();
	afx_msg void OnBranchCheck();
	afx_msg void OnBnClickedPuttykeyfileBrowse();
	afx_msg void OnBnClickedPuttykeyAutoload();
	virtual void OnOK() override;
	DECLARE_MESSAGE_MAP()
public:
	CHistoryCombo m_Repository;
	CHistoryCombo m_PathCtrl;
	CHistoryCombo m_PuttyKeyCombo;
	BOOL m_bBranch;
	BOOL m_bForce;
	BOOL m_bAutoloadPuttyKeyFile;
	CString m_strBranch;
	CString m_strPath;
	CString m_strRepos;
	CString m_strProject;
	CString	m_strPuttyKeyFile;
};
