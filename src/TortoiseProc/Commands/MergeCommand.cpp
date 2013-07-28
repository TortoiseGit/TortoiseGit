// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013 - TortoiseGit
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
#include "stdafx.h"
#include "MergeCommand.h"
#include "git.h"
#include "MergeDlg.h"
#include "MergeAbortDlg.h"
#include "MessageBox.h"
#include "Progressdlg.h"
#include "AppUtils.h"
#include "GitProgressDlg.h"

bool MergeCommand::Execute()
{
	if (parser.HasKey(_T("abort")))
	{
		CMergeAbortDlg dlg;
		if (dlg.DoModal() == IDOK)
		{
			CString cmd;
			CString type;
			switch (dlg.m_ResetType)
			{
			case 0:
				type = _T("--mixed");
				break;
			case 1:
				type = _T("--hard");
				break;
			default:
				dlg.m_ResetType = 0;
				type = _T("--mixed");
				break;
			}
			cmd.Format(_T("git.exe reset %s HEAD"), type);

			while (true)
			{
				CProgressDlg progress;
				progress.m_GitCmd = cmd;

				CTGitPath gitPath = g_Git.m_CurrentDir;
				if (gitPath.HasSubmodules() && dlg.m_ResetType == 1)
					progress.m_PostCmdList.Add(CString(MAKEINTRESOURCE(IDS_PROC_SUBMODULESUPDATE)));

				progress.m_PostFailCmdList.Add(CString(MAKEINTRESOURCE(IDS_MSGBOX_RETRY)));

				INT_PTR ret;
				if (g_Git.UsingLibGit2(CGit::GIT_CMD_RESET))
				{
					CGitProgressDlg gitdlg;
					gitdlg.SetCommand(CGitProgressList::GitProgress_Reset);
					gitdlg.SetRevision(_T("HEAD"));
					gitdlg.SetResetType(dlg.m_ResetType + 1);
					ret = gitdlg.DoModal();
				}
				else
					ret = progress.DoModal();

				if (progress.m_GitStatus == 0 && gitPath.HasSubmodules() && dlg.m_ResetType == 1 && ret == IDC_PROGRESS_BUTTON1)
				{
					CString sCmd;
					sCmd.Format(_T("/command:subupdate /bkpath:\"%s\""), g_Git.m_CurrentDir);

					CCommonAppUtils::RunTortoiseGitProc(sCmd);
					return true;
				}
				else if (progress.m_GitStatus != 0 && ret == IDC_PROGRESS_BUTTON1)
					continue;	// retry
				else if (ret == IDOK)
					return true;
				else
					break;
			}
		}
		return false;
	}

	return !!CAppUtils::Merge();
}