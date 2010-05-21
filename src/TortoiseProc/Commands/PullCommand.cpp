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
#include "PullCommand.h"

//#include "SVNProgressDlg.h"
#include "StringUtils.h"
#include "Hooks.h"
#include "MessageBox.h"
#include "PullFetchDlg.h"
#include "ProgressDlg.h"
#include "FileDiffDlg.h"
#include "AppUtils.h"
#include "LogDlg.h"
#include "ChangedDlg.h"

bool PullCommand::Execute()
{
	CPullFetchDlg dlg;
	dlg.m_IsPull=TRUE;
	if(dlg.DoModal()==IDOK)
	{
		CString url;
		url=dlg.m_RemoteURL;

		if(dlg.m_bAutoLoad)
		{
			CAppUtils::LaunchPAgent(NULL,&dlg.m_RemoteURL);
		}
		
		CString cmd;
		CString hashOld = g_Git.GetHash(L"HEAD");
		CString cmdRebase;
		if(dlg.m_bRebase)
			cmdRebase = "--rebase ";
		cmd.Format(_T("git.exe pull -v %s\"%s\" %s"),cmdRebase, url, dlg.m_RemoteBranchName);
		CProgressDlg progress;
		progress.m_GitCmd = cmd;
		progress.m_PostCmdList.Add(_T("Pulled Diff"));
		progress.m_PostCmdList.Add(_T("Pulled Log"));
		//progress.m_PostCmdList.Add(_T("Show Conflict"));
		
		//progress.m_bAutoCloseOnSuccess = true;
		int ret = progress.DoModal();
		
		CString hashNew = g_Git.GetHash(L"HEAD");
		
		if( ret == IDC_PROGRESS_BUTTON1)
		{
			if(hashOld == hashNew)
			{
				if(progress.m_GitStatus == 0)
					CMessageBox::Show(NULL, L"Already up to date.", L"Pull", MB_OK | MB_ICONINFORMATION);
				return TRUE;
			}

			CFileDiffDlg dlg;
			dlg.SetDiff(NULL, hashNew, hashOld);
			dlg.DoModal();

			return TRUE;
		}
		else if ( ret == IDC_PROGRESS_BUTTON1 +1 )
		{
			if(hashOld == hashNew)
			{
				if(progress.m_GitStatus == 0)
					CMessageBox::Show(NULL, L"Already up to date.", L"Pull", MB_OK | MB_ICONINFORMATION);
				return TRUE;
			}


			CLogDlg dlg;
			
			//dlg.SetParams(cmdLinePath);
			dlg.SetParams(CTGitPath(_T("")),_T(""), hashOld, hashNew, 0);
			//	dlg.SetIncludeMerge(!!parser.HasKey(_T("merge")));
			//	val = parser.GetVal(_T("propspath"));
			//	if (!val.IsEmpty())
			//		dlg.SetProjectPropertiesPath(CTSVNPath(val));
			dlg.DoModal();

			
		}else if ( ret == IDC_PROGRESS_BUTTON1 +2 )
		{
			CChangedDlg dlg;
			dlg.DoModal();
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
