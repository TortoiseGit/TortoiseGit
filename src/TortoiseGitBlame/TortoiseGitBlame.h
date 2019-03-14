// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2013, 2017 - TortoiseGit

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


// TortoiseGitBlame.h : main header file for the TortoiseGitBlame application
//
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols


// CTortoiseGitBlameApp:
// See TortoiseGitBlame.cpp for the implementation of this class
//

class CTortoiseGitBlameApp : public CWinAppEx
{
public:
	CTortoiseGitBlameApp();
	~CTortoiseGitBlameApp();
	ULONG_PTR m_gdiplusToken;

// Overrides
public:
	virtual BOOL InitInstance() override;

// Implementation
	BOOL  m_bHiColorIcons;

	CString m_Rev;
	afx_msg void OnAppAbout();
	afx_msg void OnFileSettings();
	DECLARE_MESSAGE_MAP()
	virtual int ExitInstance() override;
};

extern CTortoiseGitBlameApp theApp;
extern CString sOrigCWD;
extern CString g_sGroupingUUID;

#ifdef _WIN64
#define APP_X64_STRING "x64"
#else
#define APP_X64_STRING ""
#endif
