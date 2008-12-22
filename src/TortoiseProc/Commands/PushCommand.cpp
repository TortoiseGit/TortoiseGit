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
#include "PushCommand.h"

//#include "SVNProgressDlg.h"
#include "StringUtils.h"
#include "Hooks.h"
#include "MessageBox.h"

#include "PushDlg.h"
#include "ProgressDlg.h"

bool PushCommand::Execute()
{
	CPushDlg dlg;
//	dlg.m_Directory=this->orgCmdLinePath.GetWinPathString();
	if(dlg.DoModal()==IDOK)
	{
//		CString dir=dlg.m_Directory;
//		CString url=dlg.m_URL;
		CString cmd;
		CString force;
		CString tags;
		CString thin;

		if(dlg.m_bPack)
			thin=_T("--thin");
		if(dlg.m_bTags)
			tags=_T("--tags");
		if(dlg.m_bForce)
			force=_T("--force");
		
		cmd.Format(_T("git.exe push %s %s %s \"%s\" %s:%s"),
				thin,tags,force,
				dlg.m_URL,
				dlg.m_BranchSourceName,
				dlg.m_BranchRemoteName);

		CProgressDlg progress;
		progress.m_GitCmd=cmd;
		if(progress.DoModal()==IDOK)
			return TRUE;
		
	}
	return FALSE;
}
