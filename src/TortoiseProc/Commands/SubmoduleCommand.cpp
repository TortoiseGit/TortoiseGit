// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2009,2012 - TortoiseGit

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
#include "SubmoduleUpdateDlg.h"
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

		dlg.m_strPath.Replace(_T('\\'),_T('/'));
		dlg.m_strRepos.Replace(_T('\\'),_T('/'));

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
	CString bkpath;
	if (parser.HasKey(_T("bkpath")))
		bkpath = parser.GetVal(_T("bkpath"));
	else
	{
		bkpath = this->orgPathList[0].GetWinPathString();
		int start = bkpath.ReverseFind(_T('\\'));
		if (start >= 0)
			bkpath = bkpath.Left(start);
	}

	CString super = g_GitAdminDir.GetSuperProjectRoot(bkpath);
	if (super.IsEmpty())
	{
		CMessageBox::Show(NULL,IDS_ERR_NOTFOUND_SUPER_PRJECT,IDS_APPNAME,MB_OK|MB_ICONERROR);
		//change current project root to super project
		return false;
	}

	CSubmoduleUpdateDlg submoduleUpdateDlg;
	if (submoduleUpdateDlg.DoModal() != IDOK)
		return false;

	CProgressDlg progress;

	g_Git.m_CurrentDir = super;

	CString params;
	if (submoduleUpdateDlg.m_bInit)
		params = _T(" --init");
	if (submoduleUpdateDlg.m_bRecursive)
		params += _T(" --recursive");

	for (int i = 0; i < this->orgPathList.GetCount(); i++)
	{
		if (orgPathList[i].IsDirectory())
		{
			CString str;
			str.Format(_T("git.exe submodule update%s \"%s\""), params, ((CTGitPath &)orgPathList[i]).GetSubPath(CTGitPath(super)).GetGitPathString());
			progress.m_GitCmdList.push_back(str);
		}
	}

	progress.DoModal();

	return !progress.m_GitStatus;
}

bool SubmoduleCommand::Execute(CString cmd,  CString arg)
{
	CProgressDlg progress;
	CString bkpath;

	if(parser.HasKey(_T("bkpath")))
	{
		bkpath=parser.GetVal(_T("bkpath"));
	}
	else
	{
		bkpath=this->orgPathList[0].GetWinPathString();
		int start = bkpath.ReverseFind(_T('\\'));
		if( start >= 0 )
			bkpath=bkpath.Left(start);
	}

	CString super=g_GitAdminDir.GetSuperProjectRoot( bkpath );
	if(super.IsEmpty())
	{
		CMessageBox::Show(NULL,IDS_ERR_NOTFOUND_SUPER_PRJECT,IDS_APPNAME,MB_OK|MB_ICONERROR);
		//change current project root to super project
		return false;
	}

	g_Git.m_CurrentDir=super;

	progress.m_Title.Format(_T("Submodule %s"),cmd);

	//progress.m_GitCmd.Format(_T("git.exe submodule update --init "));

	CString str;
	for(int i=0;i<this->orgPathList.GetCount();i++)
	{
		if(orgPathList[i].IsDirectory())
		{
			str.Format(_T("git.exe submodule %s %s \"%s\""),cmd,arg, ((CTGitPath &)orgPathList[i]).GetSubPath(CTGitPath(super)).GetGitPathString());
			progress.m_GitCmdList.push_back(str);
		}
	}

	progress.DoModal();

	return !progress.m_GitStatus;
}
