// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2013, 2015-2016, 2018-2019, 2026 - TortoiseGit
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
#include "stdafx.h"
#include "IgnoreCommand.h"

#include "MessageBox.h"
#include "AppUtils.h"
#include "ShellUpdater.h"

bool IgnoreCommand::Execute()
{
	bool bmask=false;

	if (parser.HasKey(L"onlymask"))
		bmask=true;

	bool ret = CAppUtils::IgnoreFile(GetExplorerHWND(), pathList, bmask);
	if (!ret)
		return false;

	if (parser.HasKey(L"delete"))
	{
		CString format;
		if(CMessageBox::Show(GetExplorerHWND(), IDS_PROC_KEEPFILELOCAL, IDS_APPNAME, MB_ICONERROR|MB_YESNO) == IDYES)
			format = L"git.exe rm --cache -r -f -- %s";
		else
			format = L"git.exe rm -r -f -- %s";

		CString output;
		CString cmd;
		int nPath;
		for (nPath = 0; nPath < pathList.GetCount(); ++nPath)
		{
			try
			{
				cmd.Format(format, static_cast<LPCWSTR>(CGit::QuoteParameter(pathList[nPath].GetGitPathString())));
			}
			catch (illegal_git_parameter& e)
			{
				MessageBox(GetExplorerHWND(), e.cause(), L"TortoiseGit", MB_OK | MB_ICONERROR);
				return false;
			}
			if (g_Git.Run(cmd, &output, CP_UTF8))
			{
				if (MessageBox(GetExplorerHWND(), output, L"TortoiseGit", MB_ICONERROR | MB_OKCANCEL) == IDCANCEL)
					return FALSE;
			}
		}

		output.Format(IDS_PROC_FILESREMOVED, nPath);

		CShellUpdater::Instance().AddPathsForUpdate(pathList);

		MessageBox(GetExplorerHWND(), output, L"TortoiseGit", MB_ICONINFORMATION | MB_OK);
	}

	CShellUpdater::Instance().AddPathsForUpdate(orgPathList);
	CShellUpdater::Instance().Flush();

	return ret;
}
