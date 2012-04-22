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
#include "StdAfx.h"
#include "AddCommand.h"

#include "AddDlg.h"
#include "GITProgressDlg.h"
#include "ShellUpdater.h"
#include "messagebox.h"

bool AddCommand::Execute()
{
	bool bRet = false;
	if (parser.HasKey(_T("noui")))
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
#if 0
		if (pathList.AreAllPathsFiles())
		{
			SVN svn;
			ProjectProperties props;
			props.ReadPropsPathList(pathList);
			bRet = !!svn.Add(pathList, &props, svn_depth_empty, FALSE, FALSE, TRUE);
			CShellUpdater::Instance().AddPathsForUpdate(pathList);
		}
		else
		{
#endif
			CAddDlg dlg;
			dlg.m_pathList = pathList;
			if (dlg.DoModal() == IDOK)
			{
#if 0
				CString cmd,out;
				int success=0;
				for(int i=0;i<dlg.m_pathList.GetCount();i++)
				{
					cmd.Format(_T("git.exe add -- \"%s\""),dlg.m_pathList[i].GetGitPathString());
					if (g_Git.Run(cmd, &out, CP_UTF8))
					{
						CMessageBox::Show(NULL,out,_T("TortoiseGit"),MB_OK|MB_ICONERROR);
					}
					success++;
				}
				CString message;
				message.Format(_T("%d file added"),success);
				CMessageBox::Show(NULL,message,_T("TortoiseGit"),MB_OK);
				return TRUE;
#endif

				if (dlg.m_pathList.GetCount() == 0)
					return FALSE;
				CGitProgressDlg progDlg;
				theApp.m_pMainWnd = &progDlg;
				progDlg.SetCommand(CGitProgressDlg::GitProgress_Add);
				if (parser.HasVal(_T("closeonend")))
					progDlg.SetAutoClose(parser.GetLongVal(_T("closeonend")));
				progDlg.SetPathList(dlg.m_pathList);
				//ProjectProperties props;
				//props.ReadPropsPathList(dlg.m_pathList);
				//progDlg.SetProjectProperties(props);
				progDlg.SetItemCount(dlg.m_pathList.GetCount());
				progDlg.DoModal();

				CShellUpdater::Instance().AddPathsForUpdate(dlg.m_pathList);

				bRet = !progDlg.DidErrorsOccur();

			}
	//	}
	}
	CShellUpdater::Instance().Flush();
	return bRet;
}
