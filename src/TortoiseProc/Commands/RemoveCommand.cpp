// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2013, 2015-2019 - TortoiseGit
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
#include "RemoveCommand.h"

#include "MessageBox.h"
#include "Git.h"
#include "ShellUpdater.h"

bool RemoveCommand::Execute()
{
	if (!GitAdminDir::HasAdminDir(g_Git.m_CurrentDir))
	{
		CMessageBox::Show(GetExplorerHWND(), IDS_NOWORKINGCOPY, IDS_APPNAME, MB_ICONERROR);
		return false;
	}

	bool bRet = false;
	// removing items from a working copy is done item-by-item so we
	// have a chance to show a progress bar
	//
	// removing items from an URL in the repository requires that we
	// ask the user for a log message.
#if 0
	BOOL bForce = FALSE;
	SVN svn;
	if ((!pathList.IsEmpty())&&(SVN::PathIsURL(pathList[0])))
	{
		// Delete using URL's, not wc paths
		svn.SetPromptApp(&theApp);
		CInputLogDlg dlg;
		CString sUUID;
		svn.GetRepositoryRootAndUUID(pathList[0], sUUID);
		dlg.SetUUID(sUUID);
		CString sHint;
		if (pathList.GetCount() == 1)
			sHint.Format(IDS_INPUT_REMOVEONE, static_cast<LPCTSTR>(pathList)[0].GetSVNPathString());
		else
			sHint.Format(IDS_INPUT_REMOVEMORE, pathList.GetCount());
		dlg.SetActionText(sHint);
		if (dlg.DoModal()==IDOK)
		{
			if (!svn.Remove(pathList, TRUE, parser.HasKey(L"keep"), dlg.GetLogMessage()))
			{
				CMessageBox::Show(GetExplorerHWND(), svn.GetLastErrorMessage(), L"TortoiseGit", MB_ICONERROR);
				return FALSE;
			}
			return true;
		}
		return FALSE;
	}
	else
	{
		for (int nPath = 0; nPath < pathList.GetCount(); ++nPath)
		{
			CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": remove file %s\n", static_cast<LPCTSTR>(pathList)[nPath].GetUIPathString());
			// even though SVN::Remove takes a list of paths to delete at once
			// we delete each item individually so we can prompt the user
			// if something goes wrong or unversioned/modified items are
			// to be deleted
			CTSVNPathList removePathList(pathList[nPath]);
			if (bForce)
			{
				CTSVNPath delPath = removePathList[0];
				delPath.Delete(true);
			}
			if (!svn.Remove(removePathList, bForce, parser.HasKey(L"keep")))
			{
				if ((svn.Err->apr_err == SVN_ERR_UNVERSIONED_RESOURCE) ||
					(svn.Err->apr_err == SVN_ERR_CLIENT_MODIFIED))
				{
					CString msg, yes, no, yestoall;
					if (pathList[nPath].IsDirectory())
						msg.Format(IDS_PROC_REMOVEFORCEFOLDER, pathList[nPath].GetWinPath());
					else
						msg.Format(IDS_PROC_REMOVEFORCE, static_cast<LPCTSTR>(svn.GetLastErrorMessage()));
					yes.LoadString(IDS_MSGBOX_YES);
					no.LoadString(IDS_MSGBOX_NO);
					yestoall.LoadString(IDS_PROC_YESTOALL);
					UINT ret = CMessageBox::Show(GetExplorerHWND(), msg, L"TortoiseGit", 2, IDI_ERROR, yes, no, yestoall);
					if (ret == 3)
						bForce = TRUE;
					if ((ret == 1)||(ret==3))
					{
						CTSVNPath delPath = removePathList[0];
						delPath.Delete(true);
						if (!svn.Remove(removePathList, TRUE, parser.HasKey(L"keep")))
							CMessageBox::Show(GetExplorerHWND(), svn.GetLastErrorMessage(), L"TortoiseGit", MB_ICONERROR);
						else
							bRet = true;
					}
				}
				else
					CMessageBox::Show(GetExplorerHWND(), svn.GetLastErrorMessage(), L"TortoiseGit", MB_ICONERROR);
			}
		}
	}
	if (bRet)
		CShellUpdater::Instance().AddPathsForUpdate(pathList);
#endif

	//we don't ask user about if keep local copy.
	//because there are command "Delete(keep local copy)" at explore context menu
	//int key = CMessageBox::Show(GetExplorerHWND(), L"File will removed from version control\r\n Do you want to keep local copy", L"TortoiseGit", MB_ICONINFORMATION | MB_YESNOCANCEL);
	//if(key == IDCANCEL)

	CString format;
	BOOL keepLocal = parser.HasKey(L"keep");
	if (pathList.GetCount() > 1)
		format.Format(keepLocal ? IDS_WARN_DELETE_MANY_FROM_INDEX : IDS_WARN_DELETE_MANY, pathList.GetCount());
	else
		format.Format(keepLocal ? IDS_WARN_DELETE_ONE_FROM_INDEX : IDS_WARN_REMOVE, static_cast<LPCTSTR>(pathList[0].GetGitPathString()));
	if (CMessageBox::Show(GetExplorerHWND(), format, L"TortoiseGit", 2, IDI_QUESTION, CString(MAKEINTRESOURCE(IDS_REMOVEBUTTON)), CString(MAKEINTRESOURCE(IDS_MSGBOX_ABORT))) == 2)
		return false;

	if (keepLocal)
		format= L"git.exe rm -r -f --cached -- \"%s\"";
	else
		format = L"git.exe rm -r -f -- \"%s\"";

	int nPath;
	for (nPath = 0; nPath < pathList.GetCount(); ++nPath)
	{
		CString cmd;
		CString output;
		cmd.Format(format, static_cast<LPCTSTR>(pathList[nPath].GetGitPathString()));
		if (g_Git.Run(cmd, &output, CP_UTF8))
		{
			if (CMessageBox::Show(GetExplorerHWND(), output, L"TortoiseGit", 2, IDI_ERROR, CString(MAKEINTRESOURCE(IDS_IGNOREBUTTON)), CString(MAKEINTRESOURCE(IDS_ABORTBUTTON))) == 2)
				return FALSE;
		}
	}

	CString output;
	output.Format(IDS_PROC_FILESREMOVED, nPath);

	CShellUpdater::Instance().AddPathsForUpdate(pathList);

	CMessageBox::Show(GetExplorerHWND(), output, L"TortoiseGit", MB_ICONINFORMATION | MB_OK);

	CShellUpdater::Instance().Flush();
	return bRet;
}
