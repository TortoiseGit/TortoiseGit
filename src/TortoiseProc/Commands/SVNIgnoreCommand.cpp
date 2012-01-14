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
#include "SVNIgnoreCommand.h"

#include "SysProgressDlg.h"
#include "ProgressDlg.h"
#include "MessageBox.h"
#include "RenameDlg.h"
#include "Git.h"
#include "ShellUpdater.h"
#include "SVNIgnoreTypeDlg.h"

bool SVNIgnoreCommand::Execute()
{
	CSVNIgnoreTypeDlg dlg;
	CProgressDlg progress;

	if( dlg.DoModal() == IDOK)
	{
		switch (dlg.m_SVNIgnoreType)
		{
		case 0:
			progress.m_GitCmd=_T("git.exe svn show-ignore");
			progress.m_LogFile=g_Git.m_CurrentDir+_T("\\.git\\info\\exclude");
			progress.m_bShowCommand=false;
			progress.m_PreText=_T("git.exe svn show-ignore > .git\\info\\exclude");
			break;
		case 1:
			progress.m_GitCmd=_T("git.exe svn create-ignore");
			break;
		default:
			CMessageBox::Show(NULL,_T("Unkown SVN Ignore Type"),_T("TortoiseGit"),MB_OK|MB_ICONERROR);
			return FALSE;

		}
	}

	if(progress.DoModal()==IDOK)
	{
		if( dlg.m_SVNIgnoreType == 0 )
		{

		}
		return progress.m_GitStatus == 0;
	}
	return false;
}
