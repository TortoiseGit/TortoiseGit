// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2013, 2016, 2018-2019 - TortoiseGit
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
#include "AddCommand.h"
#include "MessageBox.h"
#include "AddDlg.h"
#include "GitProgressDlg.h"
#include "ProgressCommands/AddProgressCommand.h"
#include "ShellUpdater.h"

bool AddCommand::Execute()
{
	if (!GitAdminDir::HasAdminDir(g_Git.m_CurrentDir))
	{
		CMessageBox::Show(GetExplorerHWND(), IDS_NOWORKINGCOPY, IDS_APPNAME, MB_ICONERROR);
		return false;
	}

	bool bRet = false;
	if (parser.HasKey(L"noui"))
	{
#if 0
		SVN svn;
		ProjectProperties props;
		props.ReadPropsPathList(pathList);
		bRet = !!svn.Add(pathList, &props, svn_depth_empty, FALSE, FALSE, TRUE);
		CShellUpdater::Instance().AddPathsForUpdate(pathList);
#endif
	}
	else
	{
		if (pathList.AreAllPathsFiles())
		{
			CGitProgressDlg progDlg;
			theApp.m_pMainWnd = &progDlg;
			AddProgressCommand addCommand;
			progDlg.SetCommand(&addCommand);
			addCommand.SetPathList(pathList);
			progDlg.SetItemCount(pathList.GetCount());
			progDlg.DoModal();

			bRet = !progDlg.DidErrorsOccur();
		}
		else
		{
			CAddDlg dlg;
			dlg.m_pathList = pathList;
			if (dlg.DoModal() == IDOK)
			{
				if (dlg.m_pathList.IsEmpty())
					return FALSE;
				CGitProgressDlg progDlg;
				theApp.m_pMainWnd = &progDlg;
				AddProgressCommand addCommand;
				progDlg.SetCommand(&addCommand);
				addCommand.SetPathList(dlg.m_pathList);
				progDlg.SetItemCount(dlg.m_pathList.GetCount());
				progDlg.DoModal();

				bRet = !progDlg.DidErrorsOccur();

			}
		}
	}
	CShellUpdater::Instance().Flush();
	return bRet;
}
