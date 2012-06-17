// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2012 - TortoiseGit
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
#include "StdAfx.h"
#include "IgnoreCommand.h"

#include "MessageBox.h"
#include "PathUtils.h"
#include "AppUtils.h"
#include "ShellUpdater.h"

bool IgnoreCommand::Execute()
{
	bool bmask=false;

	if(parser.HasKey(_T("onlymask")))
	{
		bmask=true;
	}

	bool ret = CAppUtils::IgnoreFile(pathList,bmask);

	if (parser.HasKey(_T("delete")))
	{
		int key;

		CString format;

		if(CMessageBox::Show(hwndExplorer, IDS_PROC_KEEPFILELOCAL, IDS_APPNAME, MB_ICONERROR|MB_YESNO) == IDYES)
		{
			format= _T("git.exe update-index --force-remove -- \"%s\"");
		}
		else
		{
			format=_T("git.exe rm -r -f -- \"%s\"");
		}

		CString output;
		CString cmd;
		int nPath;
		for(nPath = 0; nPath < pathList.GetCount(); nPath++)
		{

			cmd.Format(format,pathList[nPath].GetGitPathString());
			if (g_Git.Run(cmd, &output, CP_UTF8))
			{
				key = MessageBox(hwndExplorer, output, _T("TortoiseGit"), MB_ICONERROR | MB_OKCANCEL);
				if(key == IDCANCEL)
					return FALSE;

			}
		}

		output.Format(IDS_PROC_FILESREMOVED, nPath);

		CShellUpdater::Instance().AddPathsForUpdate(pathList);

		MessageBox(hwndExplorer, output, _T("TortoiseGit"), MB_ICONINFORMATION | MB_OK);
	}

	CShellUpdater::Instance().AddPathsForUpdate(orgPathList);
	CShellUpdater::Instance().Flush();

	return ret;
}