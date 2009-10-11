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
#include "FetchCommand.h"

//#include "SVNProgressDlg.h"
#include "StringUtils.h"
#include "Hooks.h"
#include "MessageBox.h"
#include "PullFetchDlg.h"
#include "ProgressDlg.h"
#include "AppUtils.h"
#include "RebaseDlg.h"

bool FetchCommand::Execute()
{
	CPullFetchDlg dlg;
	dlg.m_IsPull=FALSE;

	if(dlg.DoModal()==IDOK)
	{
		if(dlg.m_bAutoLoad)
		{
			CAppUtils::LaunchPAgent(NULL,&dlg.m_RemoteURL);
		}

		CString url;
		url=dlg.m_RemoteURL;
		CString cmd;
		cmd.Format(_T("git.exe fetch -v \"%s\" %s"),url, dlg.m_RemoteBranchName);
		CProgressDlg progress;

		if(!dlg.m_bRebase)
		{
			progress.m_changeAbortButtonOnSuccessTo=_T("&Rebase");
		}else
		{
			progress.m_bAutoCloseOnSuccess = true;
		}

		progress.m_GitCmd=cmd;
		int userResponse=progress.DoModal();

		if( (userResponse==IDC_PROGRESS_BUTTON1) || ( progress.m_GitStatus ==0 && dlg.m_bRebase) )
		{
			CRebaseDlg dlg;
			dlg.m_PostButtonText=_T("Email &Patch...");
			int response = dlg.DoModal();
			if(response == IDOK)
			{
				return TRUE;
			}
			if(response == IDC_REBASE_POST_BUTTON)
			{
				CString cmd,out;
				cmd.Format(_T("git.exe  format-patch -o \"%s\" %s..%s"),
					g_Git.m_CurrentDir,
					dlg.m_Upstream,dlg.m_Branch);
				if(g_Git.Run(cmd,&out,CP_ACP))
				{
					CMessageBox::Show(NULL,out,_T("TortoiseGit"),MB_OK|MB_ICONERROR);
					return FALSE;
				}

				CAppUtils::SendPatchMail(cmd,out);
			}
			return TRUE;
		}
	}
#if 0
	CCloneDlg dlg;
	dlg.m_Directory=this->orgCmdLinePath.GetWinPathString();
	if(dlg.DoModal()==IDOK)
	{
		CString dir=dlg.m_Directory;
		CString url=dlg.m_URL;
		CString cmd;
		cmd.Format(_T("git.exe clone %s %s"),
						url,
						dir);
		CProgressDlg progress;
		progress.m_GitCmd=cmd;
		if(progress.DoModal()==IDOK)
			return TRUE;
		
	}
#endif
	return FALSE;
}
