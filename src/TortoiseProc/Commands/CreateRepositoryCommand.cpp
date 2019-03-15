// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2019 - TortoiseGit

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
#include "CreateRepositoryCommand.h"
#include "ShellUpdater.h"
#include "MessageBox.h"
#include "UnicodeUtils.h"
#include "IconExtractor.h"
#include "CreateRepoDlg.h"
#include "SmartHandle.h"
#include "AppUtils.h"

static bool CheckSpecialFolder(CString &folder)
{
	// Drive root
	if (folder == "\\" || folder.GetLength() == 2 && folder[1] == ':' || folder.GetLength() == 3 && folder[1] == ':' && folder[2] == '\\')
		return true;

	// UNC root
	if (folder.GetLength() > 2 && CStringUtils::StartsWith(folder, L"\\\\"))
	{
		int index = folder.Find('\\', 2);
		if (index < 0)
			return true;
		else if (folder.GetLength() == index - 1)
			return true;
	}

	TCHAR path[MAX_PATH + 1];
	int code[] = { CSIDL_DESKTOPDIRECTORY, CSIDL_PROFILE, CSIDL_PERSONAL, CSIDL_WINDOWS, CSIDL_SYSTEM, CSIDL_PROGRAM_FILES, CSIDL_SYSTEMX86, CSIDL_PROGRAM_FILESX86 };
	for (int i = 0; i < _countof(code); i++)
	{
		path[0] = L'\0';
		if (SUCCEEDED(SHGetFolderPath(nullptr, code[i], nullptr, 0, path)))
			if (folder == path)
				return true;
	}

	return false;
}

bool CreateRepositoryCommand::Execute()
{
	CString folder = this->orgCmdLinePath.GetWinPath();
	if (folder.IsEmpty())
		folder = g_Git.m_CurrentDir;
	if (folder.IsEmpty())
		GetCurrentDirectory(MAX_PATH, CStrBuf(folder, MAX_PATH));
	if (CheckSpecialFolder(folder))
	{
		CString message;
		message.Format(IDS_WARN_GITINIT_SPECIALFOLDER, static_cast<LPCTSTR>(folder));
		if (CMessageBox::Show(GetExplorerHWND(), message, L"TortoiseGit", 1, IDI_ERROR, CString(MAKEINTRESOURCE(IDS_ABORTBUTTON)), CString(MAKEINTRESOURCE(IDS_PROCEEDBUTTON))) == 1)
			return false;
	}

	CCreateRepoDlg dlg;
	dlg.m_folder = folder;
	if(dlg.DoModal() == IDOK)
	{
		CString message;
		message.Format(IDS_WARN_GITINIT_FOLDERNOTEMPTY, static_cast<LPCTSTR>(folder));
		if (dlg.m_bBare && PathIsDirectory(folder) && !PathIsDirectoryEmpty(folder) && CMessageBox::Show(GetExplorerHWND(), message, L"TortoiseGit", 1, IDI_ERROR, CString(MAKEINTRESOURCE(IDS_ABORTBUTTON)), CString(MAKEINTRESOURCE(IDS_PROCEEDBUTTON))) == 1)
			return false;

		git_repository_init_options options = GIT_REPOSITORY_INIT_OPTIONS_INIT;
		options.flags = GIT_REPOSITORY_INIT_MKPATH | GIT_REPOSITORY_INIT_EXTERNAL_TEMPLATE;
		options.flags |= dlg.m_bBare ? GIT_REPOSITORY_INIT_BARE : 0;
		CAutoRepository repo;
		if (git_repository_init_ext(repo.GetPointer(), CUnicodeUtils::GetUTF8(folder), &options))
		{
			CMessageBox::Show(GetExplorerHWND(), CGit::GetLibGit2LastErr(L"Could not initialize a new repository."), L"TortoiseGit", MB_OK | MB_ICONERROR);
			return false;
		}

		if (!dlg.m_bBare)
			CShellUpdater::Instance().AddPathForUpdate(orgCmdLinePath);
		else
			CAppUtils::SetupBareRepoIcon(folder);

		CString str;
		str.Format(IDS_PROC_REPOCREATED, static_cast<LPCTSTR>(folder));
		CMessageBox::Show(GetExplorerHWND(), str, L"TortoiseGit", MB_OK | MB_ICONINFORMATION);
		return true;
	}
	return false;
}
