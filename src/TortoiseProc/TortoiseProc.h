// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2011 - TortoiseGit
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

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif


#include "resource.h"		// main symbols


#include "..\\TortoiseShell\\resource.h"
//#include "UnicodeUtils.h"

class CTSVNPath;
class CTSVNPathList;

/**
 * \ingroup TortoiseProc
 * Main class of the TortoiseProc.exe\n
 * It is the entry point when calling the TortoiseProc.exe and
 * handles the command line. Depending on the command line
 * other 'modules' are called, usually dialog boxes which 
 * themselves then execute a specific function.\n\n
 * Many commands are executed using the CSVNProgressDlg which
 * just displays the common notify callbacks of the Subversion commands.
 */


class CTortoiseProcApp : public CWinAppEx
{
public:
	CTortoiseProcApp();
	~CTortoiseProcApp();

// Overrides
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

	BOOL CheckMsysGitDir();

	void CheckUpgrade();
	void CheckForNewerVersion();
	void EnableCrashHandler();
	void InitializeJumpList();
	void DoInitializeJumpList();

	ULONG_PTR m_gdiplusToken;
// Implementation

private:


	DECLARE_MESSAGE_MAP()
private:
	bool	retSuccess;
};

extern CTortoiseProcApp theApp;
extern HWND hWndExplorer;
extern CString sOrigCWD;

