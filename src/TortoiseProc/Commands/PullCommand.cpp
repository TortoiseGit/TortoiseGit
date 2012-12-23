// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2012 - TortoiseGit
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
#include "AppUtils.h"
#include "ChangedDlg.h"

bool PullCommand::Execute()
{
	if (!GitAdminDir().HasAdminDir(g_Git.m_CurrentDir)) {
		CMessageBox::Show(hwndExplorer, IDS_NOWORKINGCOPY, IDS_APPNAME, MB_ICONERROR);
		return false;
	}

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
		CString hashOld;
		try
		{
			hashOld = g_Git.GetHash(_T("HEAD"));
		}
		catch (char* msg)
		{
			CString err(msg);
			MessageBox(NULL, _T("Could not get HEAD hash.\nlibgit reports:\n") + err, _T("TortoiseGit"), MB_ICONERROR);
			ExitProcess(1);
		}
		CString cmdRebase;
		if(dlg.m_bRebase)
			cmdRebase = "--rebase ";

		CString noff;
		CString ffonly;
		CString squash;
		CString nocommit;
		CString notags;
		if (!dlg.m_bFetchTags)
			notags = _T("--no-tags");
		if(dlg.m_bNoFF)
			noff=_T("--no-ff");

		if (dlg.m_bFFonly)
			ffonly = _T("--ff-only");

		if(dlg.m_bSquash)
			squash=_T("--squash");

		if(dlg.m_bNoCommit)
			nocommit=_T("--no-commit");

		int ver = CAppUtils::GetMsysgitVersion();

		if(ver >= 0x01070203) //above 1.7.0.2
			cmdRebase += _T("--progress ");

		cmd.Format(_T("git.exe pull -v %s %s %s %s %s %s \"%s\" %s"), cmdRebase, noff, ffonly, squash, nocommit, notags, url, dlg.m_RemoteBranchName);
		CProgressDlg progress;
		progress.m_GitCmd = cmd;
		progress.m_PostCmdList.Add(CString(MAKEINTRESOURCE(IDS_PROC_PULL_DIFFS)));
		progress.m_PostCmdList.Add(CString(MAKEINTRESOURCE(IDS_PROC_PULL_LOG)));

		CTGitPath gitPath = g_Git.m_CurrentDir;
		if (gitPath.HasSubmodules())
			progress.m_PostCmdList.Add(CString(MAKEINTRESOURCE(IDS_PROC_SUBMODULESUPDATE)));

		//progress.m_PostCmdList.Add(_T("Show Conflict"));

		if (parser.HasVal(_T("closeonend")))
			progress.m_bAutoCloseOnSuccess = !!parser.GetLongVal(_T("closeonend"));

		INT_PTR ret = progress.DoModal();

		if (ret == IDOK && progress.m_GitStatus == 1 && progress.m_LogText.Find(_T("CONFLICT")) >= 0 && CMessageBox::Show(NULL, IDS_SEECHANGES, IDS_APPNAME, MB_YESNO | MB_ICONINFORMATION) == IDYES)
		{
			CChangedDlg dlg;
			dlg.m_pathList.AddPath(CTGitPath());
			dlg.DoModal();

			return FALSE;
		}

		CString hashNew = g_Git.GetHash(L"HEAD");

		if( ret == IDC_PROGRESS_BUTTON1)
		{
			if(hashOld == hashNew)
			{
				if(progress.m_GitStatus == 0)
					CMessageBox::Show(NULL, IDS_UPTODATE, IDS_APPNAME, MB_OK | MB_ICONINFORMATION);
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
					CMessageBox::Show(NULL, IDS_UPTODATE, IDS_APPNAME, MB_OK | MB_ICONINFORMATION);
				return TRUE;
			}


			CLogDlg dlg;

			//dlg.SetParams(cmdLinePath);
			dlg.SetParams(CTGitPath(_T("")),CTGitPath(_T("")),_T(""), hashOld, hashNew, 0);
			//	dlg.SetIncludeMerge(!!parser.HasKey(_T("merge")));
			//	val = parser.GetVal(_T("propspath"));
			//	if (!val.IsEmpty())
			//		dlg.SetProjectPropertiesPath(CTSVNPath(val));
			dlg.DoModal();


		}
		else if (ret == IDC_PROGRESS_BUTTON1 + 2 && gitPath.HasSubmodules())
		{
			CString sCmd;
			sCmd.Format(_T("/command:subupdate /bkpath:\"%s\""), g_Git.m_CurrentDir);

			CAppUtils::RunTortoiseGitProc(sCmd);
		}
	}

	return FALSE;
}
