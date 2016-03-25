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

#include "StandAloneDlg.h"
#include "resource.h"

/**
 * \ingroup TortoiseProc
 * Helper dialog to ask the user for authentication data.
 */
class CSimplePrompt : public CDialog
{
	DECLARE_DYNAMIC(CSimplePrompt)

public:
	CSimplePrompt(CWnd* pParent = nullptr);
	virtual ~CSimplePrompt();

	enum { IDD = IDD_SIMPLEPROMPT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
public:
	CString		m_sUsername;
	CString		m_sPassword;
	CString		m_sRealm;
	BOOL		m_bSaveAuthentication;
	HWND		m_hParentWnd;
};
