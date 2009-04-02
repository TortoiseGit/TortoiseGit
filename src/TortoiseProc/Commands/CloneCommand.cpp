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
#include "CloneCommand.h"

//#include "SVNProgressDlg.h"
#include "StringUtils.h"
#include "Hooks.h"
#include "MessageBox.h"

#include "CloneDlg.h"
#include "ProgressDlg.h"
#include "AppUtils.h"

bool CloneCommand::Execute()
{
	CCloneDlg dlg;
	dlg.m_Directory=this->orgCmdLinePath.GetWinPathString();
	if(dlg.DoModal()==IDOK)
	{
		if(dlg.m_bAutoloadPuttyKeyFile)
		{
			CAppUtils::LaunchPAgent(&dlg.m_strPuttyKeyFile);
		}
		CString dir=dlg.m_Directory;
		CString url=dlg.m_URL;
		// is this a windows format UNC path, ie starts with \\ 
		if (url.Find(_T("\\\\")) == 0)
		{
			// yes, change all \ to /
			// this should not be necessary but msysgit does not support the use \ here yet
			url.Replace( _T('\\'), _T('/'));
		}
		CString cmd;
		cmd.Format(_T("git.exe clone -v \"%s\" \"%s\""),
						url,
						dir);
		CProgressDlg progress;
		progress.m_GitCmd=cmd;
		if(progress.DoModal()==IDOK)
			return TRUE;
		
	}
	return FALSE;
}
