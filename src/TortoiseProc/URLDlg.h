// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2007 - TortoiseSVN

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
 * A simple dialog, used for entering an URL.
 */
class CURLDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CURLDlg)

public:
	CURLDlg(CWnd* pParent = nullptr);
	virtual ~CURLDlg();

	CString m_url;

	enum { IDD = IDD_URL };
protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnSizing(UINT fwSide, LPRECT pRect);

	DECLARE_MESSAGE_MAP()

	CHistoryCombo	m_URLCombo;
	int				m_height;
};
