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
		
		CString depth; 
		if (dlg.m_bDepth)
		{
			
			depth.Format(_T(" --depth %d"),dlg.m_nDepth);
		}

		CString cmd;
		CString progressarg; 
		CString version;
		cmd = _T("git.exe --version");
		if(g_Git.Run(cmd, &version, CP_ACP))
		{
			CMessageBox::Show(NULL,_T("git have not installed"), _T("TortoiseGit"), MB_OK|MB_ICONERROR);
			return false;
		}
		
		int start=0;
		int ver;
		
		CString str=version.Tokenize(_T("."),start);
		int space = str.ReverseFind(_T(' '));
		str=str.Mid(space+1,start);
		ver = _ttol(str);
		ver <<=16;
		version = version.Mid(start);
		start = 0;
		str = version.Tokenize(_T("."),start);
		ver |= _ttol(str)&0xFFFF;

		if(ver >= 0x10007) //above 1.7.0.2
			progressarg = _T("--progress");

	
		cmd.Format(_T("git.exe clone %s -v %s \"%s\" \"%s\""),
						progressarg, 
						depth,
						url,
						dir);

		
		// Handle Git SVN-clone
		if(dlg.m_bSVN)
		{

			//g_Git.m_CurrentDir=dlg.m_Directory;
			cmd.Format(_T("git.exe svn clone \"%s\"  \"%s\""),
				url,dlg.m_Directory);

			if(dlg.m_bSVNTrunk)
				cmd+=_T(" -T ")+dlg.m_strSVNTrunk;

			if(dlg.m_bSVNBranch)
				cmd+=_T(" -b ")+dlg.m_strSVNBranchs;

			if(dlg.m_bSVNTags)
				cmd+=_T(" -t ")+dlg.m_strSVNTags;

			if(dlg.m_bSVNFrom)
			{
				CString str;
				str.Format(_T("%d:HEAD"),dlg.m_nSVNFrom);
				cmd+=_T(" -r ")+str;
			}

		}
		CProgressDlg progress;
		progress.m_GitCmd=cmd;
		if(progress.DoModal()==IDOK)
			return TRUE;
		
	}
	return FALSE;
}
