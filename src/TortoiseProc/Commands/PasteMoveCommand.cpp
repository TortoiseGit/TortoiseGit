// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008, 2012-2013, 2015-2019 - TortoiseGit

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
#include "PasteMoveCommand.h"

#include "SysProgressDlg.h"
#include "MessageBox.h"
#include "Git.h"
#include "RenameDlg.h"
#include "ShellUpdater.h"

bool PasteMoveCommand::Execute()
{
	CString sDroppath = parser.GetVal(L"droptarget");
	CTGitPath dropPath(sDroppath);
	if (dropPath.IsAdminDir())
		return FALSE;

	if(!dropPath.HasAdminDir(&g_Git.m_CurrentDir))
		return FALSE;

	unsigned long count = 0;
	orgPathList.RemoveAdminPaths();
	CString sNewName;
	CSysProgressDlg progress;
	progress.SetTitle(IDS_PROC_MOVING);
	progress.SetTime(true);
	progress.ShowModeless(CWnd::FromHandle(GetExplorerHWND()));
	for (int nPath = 0; nPath < orgPathList.GetCount(); ++nPath)
	{
		CTGitPath destPath;
		if (sNewName.IsEmpty())
			destPath = CTGitPath(sDroppath + L'\\' + orgPathList[nPath].GetFileOrDirectoryName());
		else
			destPath = CTGitPath(sDroppath + L'\\' + sNewName);
		if (destPath.Exists())
		{
			CString name = orgPathList[nPath].GetFileOrDirectoryName();
			if (!sNewName.IsEmpty())
				name = sNewName;
			progress.Stop();
			CRenameDlg dlg;
			dlg.m_name = name;
			dlg.m_windowtitle.Format(IDS_PROC_NEWNAMEMOVE, static_cast<LPCTSTR>(name));
			if (dlg.DoModal() != IDOK)
				return FALSE;
			destPath.SetFromWin(sDroppath + L'\\' + dlg.m_name);
		}
		CString top;
		orgPathList[nPath].HasAdminDir(&top);
		//git_wc_status_kind s = status.GetAllStatus(orgPathList[nPath]);
		//if (s == git_wc_status_none || s == git_wc_status_unversioned || s == git_wc_status_ignored || top.CompareNoCase(g_Git.m_CurrentDir) != 0)
		if (false)
		{
			// source file is unversioned: move the file to the target, then add it
			MoveFile(orgPathList[nPath].GetWinPath(), destPath.GetWinPath());
			CString cmd,output;
			cmd.Format(L"git.exe add -- \"%s\"", destPath.GetWinPath());
			if (g_Git.Run(cmd, &output, CP_UTF8))
			//if (!Git.Add(CTGitorgPathList(destPath), &props, Git_depth_infinity, true, false, true))
			{
				TRACE(L"%s\n", static_cast<LPCTSTR>(output));
				CMessageBox::Show(GetExplorerHWND(), output, L"TortoiseGit", MB_ICONERROR);
				return FALSE;		//get out of here
			}
			CShellUpdater::Instance().AddPathForUpdate(destPath);
		}
		else
		{
			CString cmd,output;
			cmd.Format(L"git.exe mv \"%s\" \"%s\"", static_cast<LPCTSTR>(orgPathList[nPath].GetGitPathString()), static_cast<LPCTSTR>(destPath.GetGitPathString()));
			if (g_Git.Run(cmd, &output, CP_UTF8))
			//if (!Git.Move(CTGitorgPathList(orgPathList[nPath]), destPath, FALSE))
			{
#if 0
				if (Git.Err && (Git.Err->apr_err == Git_ERR_UNVERSIONED_RESOURCE ||
					Git.Err->apr_err == Git_ERR_CLIENT_MODIFIED))
				{
					// file/folder seems to have local modifications. Ask the user if
					// a force is requested.
					CString temp = Git.GetLastErrorMessage();
					CString sQuestion(MAKEINTRESOURCE(IDS_PROC_FORCEMOVE));
					temp += L'\n' + sQuestion;
					if (CMessageBox::Show(GetExplorerHWND(), temp, L"TortoiseGit", MB_YESNO) == IDYES)
					{
						if (!Git.Move(CTGitPathList(pathList[nPath]), destPath, TRUE))
						{
							CMessageBox::Show(GetExplorerHWND(), Git.GetLastErrorMessage(), L"TortoiseGit", MB_ICONERROR);
							return FALSE;		//get out of here
						}
						CShellUpdater::Instance().AddPathForUpdate(destPath);
					}
				}
				else
#endif
				{
					TRACE(L"%s\n", static_cast<LPCTSTR>(output));
					CMessageBox::Show(GetExplorerHWND(), output, L"TortoiseGit", MB_ICONERROR);
					return FALSE;		//get out of here
				}
			}
			else
				CShellUpdater::Instance().AddPathForUpdate(destPath);
		}
		++count;
		if (progress.IsValid())
		{
			progress.FormatPathLine(1, IDS_PROC_MOVINGPROG, orgPathList[nPath].GetWinPath());
			progress.FormatPathLine(2, IDS_PROC_CPYMVPROG2, destPath.GetWinPath());
			progress.SetProgress(count, orgPathList.GetCount());
		}
		if ((progress.IsValid())&&(progress.HasUserCancelled()))
		{
			CMessageBox::Show(GetExplorerHWND(), IDS_USERCANCELLED, IDS_APPNAME, MB_ICONINFORMATION);
			return FALSE;
		}
	}
	return true;
}
