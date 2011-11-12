// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2010-2011 - TortoiseGit

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

#pragma once

#include "StandAloneDlg.h"
#include "registry.h"
#include "tooltip.h"

// CCreateRepoDlg dialog

class CCreateRepoDlg : public CStandAloneDialog
{
	DECLARE_DYNCREATE(CCreateRepoDlg)

public:
	CCreateRepoDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CCreateRepoDlg();

// Dialog Data
	enum { IDD = IDD_CREATEREPO};

protected:
	// Overrides
	virtual void OnOK();
	virtual void OnCancel();

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	CString m_ModuleName;

	DECLARE_MESSAGE_MAP()

public:
	BOOL	m_bBare;
	CString	m_folder;

protected:
	afx_msg void OnBnClickedCheckBare();

	CToolTips	m_tooltips;
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};
