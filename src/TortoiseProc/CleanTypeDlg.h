// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2011 - TortoiseGit

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

// CCleanTypeDlg dialog

class CCleanTypeDlg : public CStandAloneDialog
{
	DECLARE_DYNAMIC(CCleanTypeDlg)

public:
	CCleanTypeDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CCleanTypeDlg();

// Dialog Data
	enum { IDD = IDD_CLEAN };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	CRegDWORD m_regDir;
	CRegDWORD m_regType;

	DECLARE_MESSAGE_MAP()

	virtual	BOOL OnInitDialog();
	virtual	void OnOK();

public:
	BOOL	m_bDir;
	int		m_CleanType;
	afx_msg void OnBnClickedHelp();
};
