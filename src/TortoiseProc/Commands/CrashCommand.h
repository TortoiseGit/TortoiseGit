// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2007-2008 - TortoiseSVN

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
#include "Command.h"

#include "MessageBox.h"
#include "CrashReport.h"

extern CCrashReport crasher;
/**
 * \ingroup TortoiseProc
 * crashes the application to test the crash handler.
 */
class CrashCommand : public Command
{
public:
	/**
	 * Executes the command.
	 */
	virtual bool			Execute() override
	{
		crasher.Enable(!parser.HasKey(L"nocustom"));
		CMessageBox::Show(hwndExplorer, L"You are testing the crashhandler.\n<ct=0x0000FF>Do NOT send the crashreport!!!!</ct>", L"TortoiseGit", MB_ICONINFORMATION);
		CrashProgram();
		CMessageBox::Show(hwndExplorer, IDS_ERR_NOCOMMAND, IDS_APPNAME, MB_ICONERROR);
		return true;
	}

	void CrashProgram()
	{
		// this function is to test the crash reporting utility
		int * a;
		a = nullptr;
		*a = 7;
	}
};


