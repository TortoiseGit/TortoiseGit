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
#include "ImportPatchCommand.h"

#include "MessageBox.h"
#include "ImportPatchDlg.h"
#include "InputLogDlg.h"
#include "Git.h"
#include "DirFileEnum.h"
#include "ShellUpdater.h"

bool ImportPatchCommand::Execute()
{
	CImportPatchDlg dlg;
//	dlg.m_bIsTag=TRUE;
	CString cmd;
	CString output;
	if(dlg.DoModal()==IDOK)
	{
		for(int i=0;i<dlg.m_PathList.GetCount();i++)
		{			
			cmd.Format(_T("git.exe am \"%s\""),dlg.m_PathList[i].GetGitPathString());
			
			if(g_Git.Run(cmd,&output,CP_ACP))
			{
				CMessageBox::Show(NULL,output,_T("TortoiseGit"),MB_OK);
				return FALSE;
			}
		}
		
		CMessageBox::Show(NULL,CString(_T("Patch applied: "))+output,_T("TortoiseGit"),MB_OK);
		return TRUE;
	}
	
	return FALSE;
}
