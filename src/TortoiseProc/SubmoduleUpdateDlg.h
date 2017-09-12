// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2012-2016 - TortoiseGit

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
#include "registry.h"
#include "HistoryCombo.h"

class CSubmoduleUpdateDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CSubmoduleUpdateDlg)

public:
	CSubmoduleUpdateDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CSubmoduleUpdateDlg();

// Dialog Data
	enum { IDD = IDD_SUBMODULE_UPDATE };

	static bool s_bSortLogical;

protected:
	virtual BOOL OnInitDialog() override;
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

	afx_msg void OnBnClickedOk();
	afx_msg void OnLbnSelchangeListPath();
	afx_msg void OnBnClickedSelectall();
	afx_msg void OnBnClickedShowWholeProject();

	void Refresh();
	void SetDlgTitle();

public:
	BOOL m_bInit;
	BOOL m_bRecursive;
	BOOL m_bForce;
	BOOL m_bNoFetch;
	BOOL m_bMerge;
	BOOL m_bRebase;
	BOOL m_bRemote;
	STRING_VECTOR m_PathFilterList;
	STRING_VECTOR m_PathList;

protected:
	CRegDWORD		m_regInit;
	CRegDWORD		m_regRecursive;
	CRegDWORD		m_regForce;
	CRegDWORD		m_regNoFetch;
	CRegDWORD		m_regMerge;
	CRegDWORD		m_regRebase;
	CRegDWORD		m_regRemote;
	CRegDWORD		m_regShowWholeProject;
	CListBox	m_PathListBox;
	CRegString		m_regPath;
	CButton			m_SelectAll;
	BOOL			m_bWholeProject;
	CString			m_sTitle;
};
