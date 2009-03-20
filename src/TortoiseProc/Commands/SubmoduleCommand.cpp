// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2009 - TortoiseGit

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
#include "SubmoduleCommand.h"

#include "MessageBox.h"
#include "RenameDlg.h"
#include "InputLogDlg.h"
#include "Git.h"
#include "DirFileEnum.h"
#include "ShellUpdater.h"
#include "SubmoduleAddDlg.h"
#include "ProgressDlg.h"

bool SubmoduleAddCommand::Execute()
{
	bool bRet = false;
	CSubmoduleAddDlg dlg;
	dlg.m_strPath = cmdLinePath.GetDirectory().GetWinPathString();
	dlg.m_strProject = g_Git.m_CurrentDir;
	if( dlg.DoModal() == IDOK )
	{
		CString cmd;
		if(dlg.m_strPath.Left(g_Git.m_CurrentDir.GetLength()) == g_Git.m_CurrentDir)
			dlg.m_strPath = dlg.m_strPath.Right(dlg.m_strPath.GetLength()-g_Git.m_CurrentDir.GetLength()-1);
		
		CString branch;
		if(dlg.m_bBranch)
			branch.Format(_T(" -b %s "), dlg.m_strBranch);

		cmd.Format(_T("git.exe submodule add %s -- \"%s\"  \"%s\""),
						branch,
						dlg.m_strRepos, dlg.m_strPath);

		CProgressDlg progress;
		progress.m_GitCmd=cmd;
		progress.DoModal();

		bRet = TRUE;
	}
	return bRet;
}

bool SubmoduleUpdateCommand::Execute()
{
	bool bRet = false;


	return bRet;
}
