// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2012 Sven Strickroth, <email@cs-ware.de>

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

class CSubmoduleUpdateDlg : public CStandAloneDialog
{
	DECLARE_DYNAMIC(CSubmoduleUpdateDlg)

public:
	CSubmoduleUpdateDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSubmoduleUpdateDlg();

// Dialog Data
	enum { IDD = IDD_SUBMODULE_UPDATE };

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedHelp();
	afx_msg void OnLbnSelchangeListPath();

public:
	BOOL m_bInit;
	BOOL m_bRecursive;
	BOOL m_bForce;
	BOOL m_bNoFetch;
	BOOL m_bMerge;
	BOOL m_bRebase;
	STRING_VECTOR m_PathFilterList;
	STRING_VECTOR m_PathList;

protected:
	CListBox	m_PathListBox;
	CRegString		m_regPath;
};
