// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006,2008 - TortoiseSVN

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

/**
 * \ingroup TortoiseProc
 * Dialog asking for a new URL for the working copy.
 */
class CRelocateDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CRelocateDlg)

public:
	CRelocateDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CRelocateDlg();

	enum { IDD = IDD_RELOCATE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedBrowse();
	afx_msg void OnBnClickedHelp();
	afx_msg void OnSizing(UINT fwSide, LPRECT pRect);

	DECLARE_MESSAGE_MAP()

	int				m_height;

public:
	CHistoryCombo m_URLCombo;
	CString m_sToUrl;
	CString m_sFromUrl;
};
