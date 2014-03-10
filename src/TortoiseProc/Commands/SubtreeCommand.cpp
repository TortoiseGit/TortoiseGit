// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2009,2012-2013 - TortoiseGit

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
#include "SubtreeCommand.h"
#include "SubtreeAddDlg.h"
#include "AppUtils.h"
#include "MessageBox.h"
#include "ProgressDlg.h"



bool SubtreeCommand::ExecuteSubtree( CString cmd, CString arg/*=_T("")*/ )
{
	ASSERT(false);
	return true;
}

bool SubtreeAddCommand::Execute()
{
	bool bRet = false;
	CSubtreeAddDlg dlg(cmdLinePath.GetDirectory().GetWinPathString());
// 	dlg.m_strPath = cmdLinePath.GetDirectory().GetWinPathString();
// 	dlg.m_strProject = g_Git.m_CurrentDir;
	if( dlg.DoModal() == IDOK )
	{
 		if (dlg.m_bAutoloadPuttyKeyFile)
 			CAppUtils::LaunchPAgent(&dlg.m_strPuttyKeyFile);
 
 		CString cmd;
		CString args;
 		dlg.m_strPath.Replace(_T('\\'),_T('/'));
// 		dlg.m_strRepos.Replace(_T('\\'),_T('/'));
// 
		// remove the target if it's empty so we can re-create it.
		CTGitPath path = g_Git.m_CurrentDir + _T("\\") + dlg.m_strPath;
		if (path.Exists() && path.IsDirectory() && PathIsDirectoryEmpty(path.GetWinPath()))
			path.Delete(false);

		if (dlg.m_bSquash)
			args = _T("--squash");

		CString commitMessage;
		if (!dlg.m_strLogMesage.IsEmpty())
		{
			commitMessage = dlg.m_strLogMesage;
			commitMessage.Replace(_T("\""), _T("\\\""));
			commitMessage.Format(_T("-m \"%s\""), commitMessage);
		}

 		cmd.Format(_T("git.exe subtree add %s --prefix=\"%s\" \"%s\" %s %s"),
 			args, dlg.m_strPath, dlg.m_URL, dlg.m_BranchName, commitMessage);

		// Debug testing
//		CMessageBox::Show(NULL, cmd, _T("TortoiseGit"), MB_OK| MB_ICONINFORMATION);

 		CProgressDlg progress;
 		progress.m_GitCmd=cmd;
 		progress.DoModal();
 
// 		if (progress.m_GitStatus == 0)
// 		{
// 			if (dlg.m_bAutoloadPuttyKeyFile)
// 			{
// 				SetCurrentDirectory(g_Git.m_CurrentDir);
// 				CGit subgit;
// 				dlg.m_strPath.Replace(_T('/'), _T('\\'));
// 				subgit.m_CurrentDir = PathIsRelative(dlg.m_strPath) ? g_Git.m_CurrentDir + _T("\\") + dlg.m_strPath : dlg.m_strPath;
// 
// 				if (subgit.SetConfigValue(_T("remote.origin.puttykeyfile"), dlg.m_strPuttyKeyFile, CONFIG_LOCAL, CP_UTF8))
// 				{
// 					CMessageBox::Show(NULL, _T("Fail set config remote.origin.puttykeyfile"), _T("TortoiseGit"), MB_OK| MB_ICONERROR);
// 					return FALSE;
// 				}
// 			}
// 		}

		bRet = true;
	}
	return bRet;
}

bool SubtreePushCommand::Execute()
{
	return ExecuteSubtree(_T("push"));
}

bool SubtreePullCommand::Execute()
{
	return ExecuteSubtree(_T("pull"));
}
