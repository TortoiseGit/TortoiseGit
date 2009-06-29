// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2007-2008 - TortoiseGit

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
#include "SVNDCommitCommand.h"

#include "SysProgressDlg.h"
#include "ProgressDlg.h"
#include "MessageBox.h"
#include "RenameDlg.h"
#include "Git.h"
#include "ShellUpdater.h"
#include "CommonResource.h"

bool SVNDCommitCommand::Execute()
{
	if(!g_Git.CheckCleanWorkTree())
	{
		if(CMessageBox::Show(NULL,	IDS_ERROR_NOCLEAN_STASH,IDS_APPNAME,MB_YESNO|MB_ICONINFORMATION)==IDYES)
		{
			CString cmd,out;
			cmd=_T("git.exe stash");
			if(g_Git.Run(cmd,&out,CP_ACP))
			{
				CMessageBox::Show(NULL,out,_T("TortoiseGit"),MB_OK);
				return false;
			}

		}else
		{
			return false;
		}
	}

	CProgressDlg progress;
	progress.m_GitCmd=_T("git.exe svn dcommit");
	if(progress.DoModal()==IDOK)
		return TRUE;

	return true;
}
