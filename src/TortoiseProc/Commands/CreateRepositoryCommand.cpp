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
		if (!PathIsDirectoryEmpty(folder) && CMessageBox::Show(hwndExplorer, message, _T("TortoiseGit"), 1, IDI_ERROR, CString(MAKEINTRESOURCE(IDS_ABORTBUTTON)), CString(MAKEINTRESOURCE(IDS_PROCEEDBUTTON))) == 1)
		{
			return false;
		}

		git_repository *repo;
		CStringA path(CUnicodeUtils::GetMulti(folder, CP_UTF8));
		if (git_repository_init(&repo, path.GetBuffer(), dlg.m_bBare))
		{
			path.ReleaseBuffer();
			CMessageBox::Show(hwndExplorer, CGit::GetLibGit2LastErr(_T("Could not initialize a new repository.")), _T("TortoiseGit"), MB_OK | MB_ICONERROR);
			return false;
		}
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
