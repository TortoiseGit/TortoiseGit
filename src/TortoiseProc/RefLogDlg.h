// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2012 - TortoiseGit

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
#include "RefLoglist.h"
// CRefLogDlg dialog

class CRefLogDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CRefLogDlg)

public:
	CRefLogDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CRefLogDlg();

// Dialog Data
	enum { IDD = IDD_REFLOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	BOOL OnInitDialog();
	afx_msg void OnCbnSelchangeRef();
	afx_msg LRESULT OnRefLogChanged(WPARAM wParam, LPARAM lParam);
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedClearStash();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	DECLARE_MESSAGE_MAP()

	void			Refresh();

	CHistoryCombo	m_ChooseRef;

	CRefLogList		m_RefList;

public:
	CString			m_CurrentBranch;
	CString			m_SelectedHash;
};
