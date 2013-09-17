// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2013 - TortoiseGit

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
		if (dlg.m_bBare && PathIsDirectory(folder) && !PathIsDirectoryEmpty(folder) && CMessageBox::Show(hwndExplorer, message, _T("TortoiseGit"), 1, IDI_ERROR, CString(MAKEINTRESOURCE(IDS_ABORTBUTTON)), CString(MAKEINTRESOURCE(IDS_PROCEEDBUTTON))) == 1)
		{
			return false;
		}

		CStringA templateDir = CUnicodeUtils::GetUTF8(CGit::ms_LastMsysGitDir + _T("\\..\\share\\git-core\\templates"));
		{
			git_config *gitconfig;
			git_config_new(&gitconfig);
			CStringA globalConfigA = CUnicodeUtils::GetUTF8(g_Git.GetGitGlobalConfig());
			git_config_add_file_ondisk(gitconfig, globalConfigA.GetBuffer(), GIT_CONFIG_LEVEL_GLOBAL, FALSE);
			globalConfigA.ReleaseBuffer();
			CStringA globalXDGConfigA = CUnicodeUtils::GetUTF8( g_Git.GetGitGlobalXDGConfig());
			git_config_add_file_ondisk(gitconfig, globalXDGConfigA.GetBuffer(), GIT_CONFIG_LEVEL_XDG, FALSE);
			globalXDGConfigA.ReleaseBuffer();
			CStringA systemConfigA = CUnicodeUtils::GetUTF8(g_Git.ms_LastMsysGitDir + _T("\\..\\etc\\gitconfig"));
			git_config_add_file_ondisk(gitconfig, systemConfigA.GetBuffer(), GIT_CONFIG_LEVEL_SYSTEM, FALSE);
			systemConfigA.ReleaseBuffer();
			giterr_clear();

			const char * value = nullptr;
			if (!git_config_get_string(&value, gitconfig, "init.templatedir"))
				templateDir = value;

			git_config_free(gitconfig);
		}


		git_repository *repo;
		git_repository_init_options options = GIT_REPOSITORY_INIT_OPTIONS_INIT;
		options.flags = GIT_REPOSITORY_INIT_MKPATH | GIT_REPOSITORY_INIT_EXTERNAL_TEMPLATE;
		options.flags |= dlg.m_bBare ? GIT_REPOSITORY_INIT_BARE : 0;
		options.template_path = templateDir.GetBuffer();
		CStringA path(CUnicodeUtils::GetMulti(folder, CP_UTF8));
		if (git_repository_init_ext(&repo, path.GetBuffer(), &options))
		{
			templateDir.ReleaseBuffer();
			path.ReleaseBuffer();
			CMessageBox::Show(hwndExplorer, CGit::GetLibGit2LastErr(_T("Could not initialize a new repository.")), _T("TortoiseGit"), MB_OK | MB_ICONERROR);
			return false;
		}
		templateDir.ReleaseBuffer();
		path.ReleaseBuffer();
		git_repository_free(repo);

		if (!dlg.m_bBare)
			CShellUpdater::Instance().AddPathForUpdate(orgCmdLinePath);
		CString str;
		str.Format(IDS_PROC_REPOCREATED, folder);
		CMessageBox::Show(hwndExplorer, str, _T("TortoiseGit"), MB_OK | MB_ICONINFORMATION);
		return true;
	}
	return false;
}
