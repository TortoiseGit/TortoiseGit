// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2019, 2021, 2023 - TortoiseGit

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

static bool CheckSpecialFolder(const CString& folder)
{
	// Drive or UNC root
	if (PathIsRoot(folder) || PathIsUNCServer(folder))
		return true;

	static const GUID code[] = { FOLDERID_Desktop, FOLDERID_Profile, FOLDERID_Documents, FOLDERID_Windows, FOLDERID_System, FOLDERID_ProgramFiles, FOLDERID_SystemX86, FOLDERID_ProgramFilesX86 };
	for (int i = 0; i < _countof(code); i++)
	{
		CComHeapPtr<WCHAR> pszPath;
		if (SUCCEEDED(SHGetKnownFolderPath(code[i], 0, nullptr, &pszPath)) && CPathUtils::IsSamePath(folder, CString(pszPath)))
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
		message.Format(IDS_WARN_GITINIT_SPECIALFOLDER, static_cast<LPCWSTR>(folder));
		if (CMessageBox::Show(GetExplorerHWND(), message, L"TortoiseGit", 1, IDI_ERROR, CString(MAKEINTRESOURCE(IDS_ABORTBUTTON)), CString(MAKEINTRESOURCE(IDS_PROCEEDBUTTON))) == 1)
			return false;
	}

	CCreateRepoDlg dlg;
	dlg.m_folder = folder;
	if(dlg.DoModal() == IDOK)
	{
		CString message;
		message.Format(IDS_WARN_GITINIT_FOLDERNOTEMPTY, static_cast<LPCWSTR>(folder));
		if (dlg.m_bBare && PathIsDirectory(folder) && !PathIsDirectoryEmpty(folder) && CMessageBox::Show(GetExplorerHWND(), message, L"TortoiseGit", 1, IDI_ERROR, CString(MAKEINTRESOURCE(IDS_ABORTBUTTON)), CString(MAKEINTRESOURCE(IDS_PROCEEDBUTTON))) == 1)
			return false;

		git_repository_init_options options = GIT_REPOSITORY_INIT_OPTIONS_INIT;
		options.flags = GIT_REPOSITORY_INIT_MKPATH | GIT_REPOSITORY_INIT_EXTERNAL_TEMPLATE;
		options.flags |= dlg.m_bBare ? GIT_REPOSITORY_INIT_BARE : 0;
		CStringA envTemplateDir = CUnicodeUtils::GetUTF8(g_Git.m_Environment.GetEnv(L"GIT_TEMPLATE_DIR"));
		if (!envTemplateDir.IsEmpty())
			options.template_path = envTemplateDir;
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
		str.Format(IDS_PROC_REPOCREATED, static_cast<LPCWSTR>(folder));
		CMessageBox::Show(GetExplorerHWND(), str, L"TortoiseGit", MB_OK | MB_ICONINFORMATION);
		return true;
	}
	return false;
}
