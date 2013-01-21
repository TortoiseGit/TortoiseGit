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
#include "stdafx.h"
#include "Command.h"
#include "CreateRepositoryCommand.h"
#include "ShellUpdater.h"
#include "MessageBox.h"
#include "git.h"
#include "git2.h"
#include "UnicodeUtils.h"

#include "CreateRepoDlg.h"

bool CreateRepositoryCommand::Execute()
{
	CString folder = this->orgCmdLinePath.GetWinPath();
	CCreateRepoDlg dlg;
	dlg.m_folder = folder;
	if(dlg.DoModal() == IDOK)
	{
		CString message;
		message.Format(IDS_WARN_GITINIT_FOLDERNOTEMPTY, folder);
		if (!PathIsDirectoryEmpty(folder) && CMessageBox::Show(hwndExplorer, message, _T("TortoiseGit"), 1, IDI_ERROR, CString(MAKEINTRESOURCE(IDS_ABORTBUTTON)), CString(MAKEINTRESOURCE(IDS_PROCEEDBUTTON))) == 1)
		{
			return false;
		}

		CGit git;
		git.m_CurrentDir = this->orgCmdLinePath.GetWinPath();
		CString output;
		int ret;

		if (g_Git.UsingLibGit2(CGit::GIT_CMD_INIT))
		{
			git_repository *out = NULL;
			ret = git_repository_init(&out, CUnicodeUtils::GetMulti(git.m_CurrentDir, CP_UTF8), dlg.m_bBare);
			if (!ret)
			{
				git_repository_free(out);
				output.Format(_T("Success Create Git Repository at %s"), git.m_CurrentDir);
			}
			else
			{
				const git_error *err = giterr_last();
				CStringA str;
				if (err)
				{
					str.Format("ERROR %d: %s\n", err->klass, err->message);
				}
				else
					str.Format("ERROR: no detailed info\n");
				
				output = CUnicodeUtils::GetUnicode(str, CP_UTF8);
			}
		}else
		{
			if (dlg.m_bBare)
				ret = git.Run(_T("git.exe init-db --bare"), &output, CP_UTF8);
			else
				ret = git.Run(_T("git.exe init-db"), &output, CP_UTF8);
		}

		if (output.IsEmpty()) output = _T("git.Run() had no output");

		if (ret)
		{
			CMessageBox::Show(hwndExplorer, output, _T("TortoiseGit"), MB_ICONERROR);
			return false;
		}
		else
		{
			if (!dlg.m_bBare)
				CShellUpdater::Instance().AddPathForUpdate(orgCmdLinePath);
			CMessageBox::Show(hwndExplorer, output, _T("TortoiseGit"), MB_OK | MB_ICONINFORMATION);
		}
		return true;
	}
	return false;
}
