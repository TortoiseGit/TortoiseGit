// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2012 - TortoiseGit

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

/**
 * \ingroup TortoiseProc
 * crashes the application to test the crash handler.
 */
class AssumeValidCommand : public Command
{
public:
	/**
	 * Executes the command.
	 */
	virtual bool			Execute()
	{
		CString cmdTemplate;
		if (parser.HasKey(_T("unset")))
		{
			cmdTemplate = _T("git.exe update-index --no-assume-unchanged \"%s\"");
		}
		else
		{
			cmdTemplate = _T("git.exe update-index --assume-unchanged \"%s\"");
		}
		for (int i = 0; i < pathList.GetCount(); i++)
		{
			CString cmd, output;
			cmd.Format(cmdTemplate, pathList[i].GetGitPathString());
			if (g_Git.Run(cmd, &output, CP_UTF8))
			{
				MessageBox(NULL, output, _T("TortoiseGit"), MB_ICONERROR);
			}
		}
		return true;
	}
};
