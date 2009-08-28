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
#include "StashCommand.h"

#include "MessageBox.h"
#include "RenameDlg.h"
#include "InputLogDlg.h"
#include "Git.h"
#include "DirFileEnum.h"
#include "ShellUpdater.h"

#include "AppUtils.h"

bool StashSaveCommand::Execute()
{
	bool bRet = false;

	CString cmd,out;
	cmd=_T("git.exe stash");
	
	if(g_Git.Run(cmd,&out,CP_ACP))
	{
		CMessageBox::Show(NULL,CString(_T("<ct=0x0000FF>Stash Fail!!!</ct>\n"))+out,_T("TortoiseGit"),MB_OK|MB_ICONERROR);
	}else
	{
 		CMessageBox::Show(NULL,CString(_T("<ct=0xff0000>Stash Success</ct>\n"))+out,_T("TortoiseGit"),MB_OK|MB_ICONINFORMATION);
		bRet = true;
	}
	return bRet;
}


bool StashApplyCommand::Execute()
{
	if(CAppUtils::StashApply(_T("")))
		return false;
	return true;
	
}

bool StashPopCommand::Execute()
{
	if(CAppUtils::StashPop())
		return false;
	return true;
	
}
