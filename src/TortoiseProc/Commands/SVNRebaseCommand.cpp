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
#include "SVNRebaseCommand.h"

#include "SysProgressDlg.h"
#include "ProgressDlg.h"
#include "MessageBox.h"
#include "RenameDlg.h"
#include "Git.h"
#include "ShellUpdater.h"
#include "rebasedlg.h"

bool SVNRebaseCommand::Execute()
{
	bool isStash = false;

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
			isStash = true;
		}
		else
		{
			return false;
		}
	}

	CRebaseDlg dlg;

//	dlg.m_PreCmd=_T("git.exe svn fetch");

	CString cmd, out, err;
	cmd = _T("git.exe config svn-remote.svn.fetch");

	if (!g_Git.Run(cmd, &out, &err, CP_ACP))
	{
		int start = out.Find(_T(':'));
		if( start >=0 )
			out=out.Mid(start);

		if(out.Left(5) == _T(":refs"))
			out=out.Mid(6);

		start = 0;
		out=out.Tokenize(_T("\n"),start);
	}
	else
	{
		CMessageBox::Show(NULL, out + L"\n" + err, _T("TortoiseGit"), MB_OK|MB_ICONERROR);
		return false;
	}

	dlg.m_Upstream=out;

	CGitHash UpStreamOldHash,HeadHash,UpStreamNewHash;
	UpStreamOldHash=g_Git.GetHash(out);
	HeadHash = g_Git.GetHash(_T("HEAD"));
	CProgressDlg progress;
	progress.m_GitCmd=_T("git.exe svn fetch");
	progress.m_bAutoCloseOnSuccess = true;

	if(progress.DoModal()!=IDOK)
		return false;

	if(progress.m_GitStatus)
		return false;

	UpStreamNewHash=g_Git.GetHash(out);

	//everything updated
	if(UpStreamNewHash==HeadHash)
	{
		CMessageBox::Show(NULL,_T("Everything Updated"),_T("TortoiseGit"),MB_OK);
		if(isStash)
			askIfUserWantsToStashPop();

		return true;
	}

	//fast forward;
	CString ff;
	if(g_Git.IsFastForward(CString(_T("HEAD")),out))
	{
		cmd.Format(_T("git.exe reset --hard %s"),out);
		if(g_Git.Run(cmd,&ff,CP_ACP))
		{
			CMessageBox::Show(NULL,ff,_T("TortoiseGit"),MB_OK|MB_ICONERROR);
			return false;
		}
		else
		{
			CMessageBox::Show(NULL,CString(_T("Fast Forward: "))+ff,_T("TortoiseGit"),MB_OK);
			if(isStash)
				askIfUserWantsToStashPop();

			return true;
		}
	}

	//need rebase
	if(dlg.DoModal() == IDOK)
	{
		return true;
	}
	return false;
}

void SVNRebaseCommand::askIfUserWantsToStashPop()
{
	if(CMessageBox::Show(NULL, IDS_DCOMMIT_STASH_POP, IDS_APPNAME, MB_YESNO|MB_ICONINFORMATION) == IDYES)
	{
		CString cmd,out;
		cmd=_T("git.exe stash pop");
		if(g_Git.Run(cmd,&out,CP_ACP))
		{
			CMessageBox::Show(NULL,out,_T("TortoiseGit"), MB_OK);
		}
	}
}
