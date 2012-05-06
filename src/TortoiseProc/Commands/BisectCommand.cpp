// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2012 - TortoiseGit

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
#include "BisectCommand.h"
#include "BisectStartDlg.h"
#include "AppUtils.h"
#include "ProgressDlg.h"
#include "MessageBox.h"
#include "resource.h"

bool BisectCommand::Execute()
{
	CTGitPath path = g_Git.m_CurrentDir;

	if (this->parser.HasKey(_T("start")) && !path.IsBisectActive())
	{
		if(!g_Git.CheckCleanWorkTree())
		{
			if (CMessageBox::Show(NULL, IDS_ERROR_NOCLEAN_STASH, IDS_APPNAME, MB_YESNO|MB_ICONINFORMATION) == IDYES)
			{
				CString cmd, out;
				cmd = _T("git.exe stash");
				if (g_Git.Run(cmd, &out, CP_UTF8))
				{
					CMessageBox::Show(NULL, out, _T("TortoiseGit"), MB_OK);
					return false;
				}
			}
			else
				return false;
		}

		CBisectStartDlg bisectStartDlg;
		if (bisectStartDlg.DoModal() == IDOK)
		{
			CProgressDlg progress;
			theApp.m_pMainWnd = &progress;
			if (parser.HasVal(_T("closeonend")))
				progress.m_bAutoCloseOnSuccess = !!parser.GetLongVal(_T("closeonend"));
			progress.m_GitCmdList.push_back(_T("git.exe bisect start"));
			progress.m_GitCmdList.push_back(_T("git.exe bisect good ") + bisectStartDlg.m_LastGoodRevision);
			progress.m_GitCmdList.push_back(_T("git.exe bisect bad ") + bisectStartDlg.m_FirstBadRevision);

			if (path.HasSubmodules())
				progress.m_PostCmdList.Add(CString(MAKEINTRESOURCE(IDS_PROC_SUBMODULESUPDATE)));

			int ret = progress.DoModal();
			if (path.HasSubmodules() && ret == IDC_PROGRESS_BUTTON1)
			{
				CString sCmd;
				sCmd.Format(_T("/command:subupdate /bkpath:\"%s\""), g_Git.m_CurrentDir);

				CAppUtils::RunTortoiseProc(sCmd);
				return true;
			}
			else if (ret == IDOK)
				return true;	
		}
		else
		{
			return false;
		}
	}
	else if ((this->parser.HasKey(_T("good")) || this->parser.HasKey(_T("bad")) || this->parser.HasKey(_T("reset"))) && path.IsBisectActive())
	{
		CString cmd = _T("git.exe bisect ");

		if (this->parser.HasKey(_T("good")))
			cmd += _T("good");
		else if (this->parser.HasKey(_T("bad")))
			cmd += _T("bad");
		else if (this->parser.HasKey(_T("reset")))
			cmd += _T("reset");

		if (this->parser.HasKey(_T("ref")) &&! this->parser.HasKey(_T("reset")))
		{
			cmd += _T(" ");
			cmd += this->parser.GetVal(_T("ref"));
		}

		CProgressDlg progress;
		theApp.m_pMainWnd = &progress;
		if (parser.HasVal(_T("closeonend")))
			progress.m_bAutoCloseOnSuccess = !!parser.GetLongVal(_T("closeonend"));
		progress.m_GitCmd = cmd;

		if (path.HasSubmodules())
				progress.m_PostCmdList.Add(CString(MAKEINTRESOURCE(IDS_PROC_SUBMODULESUPDATE)));

		int reset = -1;
		if (!this->parser.HasKey(_T("reset")))
			reset = progress.m_PostCmdList.Add(_T("Bisect reset"));

		int ret = progress.DoModal();
		if (path.HasSubmodules() && ret == IDC_PROGRESS_BUTTON1)
		{
			CString sCmd;
			sCmd.Format(_T("/command:subupdate /bkpath:\"%s\""), g_Git.m_CurrentDir);

			CAppUtils::RunTortoiseProc(sCmd);
			return true;
		}
		else if (reset >= 0 && ret == IDC_PROGRESS_BUTTON1 + reset)
		{
			CAppUtils::RunTortoiseProc(_T("/command:bisect /reset"));
			return true;
		}
		else if (ret == IDOK)
			return true;
	}
	else
	{
		CMessageBox::Show(NULL,_T("Operation unknown or not allowed."), _T("TortoiseGit"), MB_OK|MB_ICONINFORMATION);
	}
	return false;
}
