// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2020, 2023, 2025 - TortoiseGit
// Copyright (C) 2003-2008 - TortoiseSVN

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

/**
 * \ingroup TortoiseProc
 * Helper dialog to configure diff/merge tools according to file type.
 */
class CToolAssocDlg : public CStandAloneDialog
{
	DECLARE_DYNAMIC(CToolAssocDlg)

public:
	CToolAssocDlg(const CString& type, bool add, CWnd* pParent = nullptr);
	virtual ~CToolAssocDlg();

	enum { IDD = IDD_TOOLASSOC };

protected:
	void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	BOOL OnInitDialog() override;
	afx_msg void OnBnClickedToolbrowse();

	DECLARE_MESSAGE_MAP()

	bool		m_bAdd = false;
	CString		m_sType;

public:
	CString		m_sExtension;
	CString		m_sTool;
};

