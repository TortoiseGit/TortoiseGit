// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009, 2011-2013, 2015-2019 - TortoiseGit
// Copyright (C) 2008 - TortoiseSVN

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
#include "PasteCopyCommand.h"

#include "SysProgressDlg.h"
#include "MessageBox.h"
#include "RenameDlg.h"
#include "Git.h"
#include "ShellUpdater.h"

bool PasteCopyCommand::Execute()
{
	CString sDroppath = parser.GetVal(L"droptarget");
	CTGitPath dropPath(sDroppath);
	if (dropPath.IsAdminDir())
		return FALSE;

	if(!dropPath.HasAdminDir(&g_Git.m_CurrentDir))
		return FALSE;
	//SVN svn;
	//SVNStatus status;
	unsigned long count = 0;
	CString sNewName;
	orgPathList.RemoveAdminPaths();
	CSysProgressDlg progress;
	progress.SetTitle(IDS_PROC_COPYING);
	progress.SetTime(true);
	progress.ShowModeless(CWnd::FromHandle(GetExplorerHWND()));
	for (int nPath = 0; nPath < orgPathList.GetCount(); ++nPath)
	{
		const CTGitPath& sourcePath = orgPathList[nPath];

		CTGitPath fullDropPath = dropPath;
		if (sNewName.IsEmpty())
			fullDropPath.AppendPathString(sourcePath.GetFileOrDirectoryName());
		else
			fullDropPath.AppendPathString(sNewName);

		// Check for a drop-on-to-ourselves
		if (sourcePath.IsEquivalentTo(fullDropPath))
		{
			// Offer a rename
			progress.Stop();
			CRenameDlg dlg;
			dlg.m_windowtitle.Format(IDS_PROC_NEWNAMECOPY, static_cast<LPCTSTR>(sourcePath.GetUIFileOrDirectoryName()));
			if (dlg.DoModal() != IDOK)
				return FALSE;
			// rebuild the progress dialog
			progress.EnsureValid();
			progress.SetTitle(IDS_PROC_COPYING);
			progress.SetTime(true);
			progress.SetProgress(count, orgPathList.GetCount());
			progress.ShowModeless(CWnd::FromHandle(GetExplorerHWND()));
			// Rebuild the destination path, with the new name
			fullDropPath.SetFromUnknown(sDroppath);
			fullDropPath.AppendPathString(dlg.m_name);
		}

		//svn_wc_status_kind s = status.GetAllStatus(sourcePath);
		//if ((s == svn_wc_status_none)||(s == svn_wc_status_unversioned)||(s == svn_wc_status_ignored))
		{
			// source file is unversioned: move the file to the target, then add it
			CopyFile(sourcePath.GetWinPath(), fullDropPath.GetWinPath(), FALSE);
			CString cmd,output;
			cmd.Format(L"git.exe add -- \"%s\"", fullDropPath.GetWinPath());
			if (g_Git.Run(cmd, &output, CP_UTF8))
			{
				TRACE(L"%s\n", static_cast<LPCTSTR>(output));
				CMessageBox::Show(GetExplorerHWND(), output, L"TortoiseGit", MB_ICONERROR);
				return FALSE;		//get out of here
			}
			else
				CShellUpdater::Instance().AddPathForUpdate(fullDropPath);
		}
		//else
		//{
		//	if (!svn.Copy(CTSVNPathList(sourcePath), fullDropPath, SVNRev::REV_WC, SVNRev()))
		//	{
		//		TRACE(L"%s\n", static_cast<LPCTSTR>(svn.GetLastErrorMessage()));
		//		CMessageBox::Show(GetExplorerHWND(), svn.GetLastErrorMessage(), L"TortoiseSVN", MB_ICONERROR);
		//		return FALSE;		//get out of here
		//	}
		//	else
		//		CShellUpdater::Instance().AddPathForUpdate(fullDropPath);
		//}
		++count;
		if (progress.IsValid())
		{
			progress.FormatPathLine(1, IDS_PROC_COPYINGPROG, sourcePath.GetWinPath());
			progress.FormatPathLine(2, IDS_PROC_CPYMVPROG2, fullDropPath.GetWinPath());
			progress.SetProgress(count, orgPathList.GetCount());
		}
		if ((progress.IsValid())&&(progress.HasUserCancelled()))
		{
			CMessageBox::Show(GetExplorerHWND(), IDS_USERCANCELLED, IDS_APPNAME, MB_ICONINFORMATION);
			return false;
		}
	}
	return true;
}