// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2016, 2018-2019 - TortoiseGit
// Copyright (C) 2007-2008,2012 - TortoiseSVN

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
#include "ResolveCommand.h"
#include "MessageBox.h"
#include "AppUtils.h"
#include "ResolveDlg.h"
#include "GitProgressDlg.h"
#include "ProgressCommands/ResolveProgressCommand.h"

bool ResolveCommand::Execute()
{
	if (!GitAdminDir::HasAdminDir(g_Git.m_CurrentDir))
	{
		CMessageBox::Show(GetExplorerHWND(), IDS_NOWORKINGCOPY, IDS_APPNAME, MB_ICONERROR);
		return false;
	}

	CResolveDlg dlg;
	dlg.m_pathList = pathList;
	INT_PTR ret = IDOK;
	if (!parser.HasKey(L"noquestion"))
		ret = dlg.DoModal();
	if (ret == IDOK)
	{
		if (!dlg.m_pathList.IsEmpty())
		{
			if (parser.HasKey(L"silent"))
			{
				for (int i = 0; i < dlg.m_pathList.GetCount(); ++i)
				{
					CString cmd, out;
					cmd.Format(L"git.exe add -f -- \"%s\"", static_cast<LPCTSTR>(dlg.m_pathList[i].GetGitPathString()));
					if (g_Git.Run(cmd, &out, CP_UTF8))
					{
						MessageBox(GetExplorerHWND(), out, L"TortoiseGit", MB_OK | MB_ICONERROR);
						return false;
					}

					CAppUtils::RemoveTempMergeFile(dlg.m_pathList[i]);
				}

				HWND resolveMsgWnd = parser.HasVal(L"resolvemsghwnd") ? reinterpret_cast<HWND>(parser.GetLongLongVal(L"resolvemsghwnd")) : 0;
				if (resolveMsgWnd && CRegDWORD(L"Software\\TortoiseGit\\RefreshFileListAfterResolvingConflict", TRUE) == TRUE)
				{
					static UINT WM_REVERTMSG = RegisterWindowMessage(L"GITSLNM_NEEDSREFRESH");
					::PostMessage(resolveMsgWnd, WM_REVERTMSG, NULL, NULL);
				}
				return true;
			}
			else
			{
				CGitProgressDlg progDlg(CWnd::FromHandle(GetExplorerHWND()));
				theApp.m_pMainWnd = &progDlg;
				ResolveProgressCommand resolveProgressCommand;
				progDlg.SetCommand(&resolveProgressCommand);
				resolveProgressCommand.SetPathList(dlg.m_pathList);
				progDlg.DoModal();
				return !progDlg.DidErrorsOccur();
			}
		}
	}
	return false;
}
