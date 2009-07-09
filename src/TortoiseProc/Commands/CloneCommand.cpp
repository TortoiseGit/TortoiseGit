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
	
		CAppUtils::RemoveTrailSlash(dlg.m_Directory);
		CAppUtils::RemoveTrailSlash(dlg.m_URL);

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

		// Handle Git SVN-clone
		if(dlg.m_bSVN)
		{
			WIN32_FILE_ATTRIBUTE_DATA attribs;
			if(GetFileAttributesEx(dlg.m_Directory, GetFileExInfoStandard, &attribs))
			{
				if(!(attribs.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				{
					CString errstr;
					errstr.Format(_T("%s is not valid directory"),dlg.m_Directory);
					CMessageBox::Show(NULL,errstr,_T("TortoiseGit"),MB_OK|MB_ICONERROR);
					return FALSE;
				}
			}	
			else
			{
				DWORD err = GetLastError();
				if(err == ERROR_PATH_NOT_FOUND)
				{
					if(!CAppUtils::CreateMultipleDirectory(dlg.m_Directory))
					{
						CString errstr;
						errstr.Format(_T("Fail create dir: %s"),dlg.m_Directory);
						CMessageBox::Show(NULL,errstr,_T("TortoiseGit"),MB_OK|MB_ICONERROR);
						return FALSE;
					}

				}
				else
				{
					CMessageBox::Show(NULL,_T("Unknow ERROR"),_T("TortoiseGit"),MB_OK|MB_ICONERROR);
					return FALSE;
				}
			}

			g_Git.m_CurrentDir=dlg.m_Directory;
			cmd.Format(_T("git.exe svn clone \"%s\" "),
						url);

			if(dlg.m_bSVNTrunk)
				cmd+=_T(" -T ")+dlg.m_strSVNTrunk;

			if(dlg.m_bSVNBranch)
				cmd+=_T(" -b ")+dlg.m_strSVNBranchs;

			if(dlg.m_bSVNTags)
				cmd+=_T(" -t ")+dlg.m_strSVNTags;

		}
		CProgressDlg progress;
		progress.m_GitCmd=cmd;
		if(progress.DoModal()==IDOK)
			return TRUE;
		
	}
	return FALSE;
}
