// TortoiseSVN - a Windows shell extension for easy version control

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
#include "BranchCommand.h"

#include "MessageBox.h"
//#include "SVNProgressDlg.h"
//#include "ProgressDlg.h"
#include "InputLogDlg.h"
#include "Git.h"
#include "DirFileEnum.h"
#include "ShellUpdater.h"
#include "CreateBranchTagDlg.h"

bool BranchCommand::Execute()
{
	CCreateBranchTagDlg dlg;
	dlg.m_bIsTag=FALSE;
	
	if(dlg.DoModal()==IDOK)
	{
		CString cmd;
		CString force;
		CString track;
		if(dlg.m_bTrack)
			track=_T("--track");

		if(dlg.m_bForce)
			force=_T("-f");

		cmd.Format(_T("git.exe branch %s %s %s %s"),
			track,
			force,
			dlg.m_BranchTagName,
			dlg.m_Base
			);
		CString out;
		if(g_Git.Run(cmd,&out))
		{
			CMessageBox::Show(NULL,out,_T("TortoiseGit"),MB_OK);
		}
		return TRUE;
		
	}
	return FALSE;
}
