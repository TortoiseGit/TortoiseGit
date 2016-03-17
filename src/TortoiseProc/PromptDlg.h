// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng

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

#include "resource.h"
#include "StandAloneDlg.h"

/**
 * \ingroup TortoiseProc
 *
 * A dialog box which is used by git authentication callback
 * to prompt the user for authentication data.
 */
class CPromptDlg : public CDialog
{
	DECLARE_DYNAMIC(CPromptDlg)

public:
	CPromptDlg(CWnd* pParent = nullptr);
	virtual ~CPromptDlg();

	void	SetHide(BOOL hide);

	enum { IDD = IDD_PROMPT };
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
public:
	CString		m_info;
	CString		m_sPass;
	CEdit		m_pass;
	BOOL		m_hide;
	BOOL		m_saveCheck;
	HWND		m_hParentWnd;
};
