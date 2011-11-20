// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2007 - TortoiseSVN

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

/**
 * \ingroup TortoiseProc
 * Shows a dialog telling the user what TSVN is and to RTFM, then starts an
 * instance of the explorer.
 */
class RTFMCommand : public Command
{
public:
	/**
	 * Executes the command.
	 */
	virtual bool			Execute()
	{
		// If the user tries to start TortoiseProc from the link in the programs start menu
		// show an explanation about what TSVN is (shell extension) and open up an explorer window
		CMessageBox::Show(hwndExplorer, IDS_PROC_RTFM, IDS_APPNAME, MB_ICONINFORMATION);
		TCHAR path[MAX_PATH];
		SHGetFolderPath(hwndExplorer, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, path);
		ShellExecute(0, _T("explore"), path, NULL, NULL, SW_SHOWNORMAL);
		return true;
	}
};


