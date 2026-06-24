// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2026 - TortoiseGit

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
#pragma once
#include "stdafx.h"
#include "InaccessibleCommand.h"
#include "MessageBox.h"
#include "PathUtils.h"

bool InaccessibleCommand ::Execute()
{
	CString errorParts;
	g_Git.m_CurrentDir = orgCmdLinePath.GetWinPathString();
	::SetCurrentDirectory(orgCmdLinePath.GetWinPath());
	try
	{
		g_Git.CheckAndInitDll();
	}
	catch (const char* msg)
	{
		errorParts = L"libgit reported: " + CUnicodeUtils::GetUnicode(msg) + L"\n";
	}
	if (!g_Git.GetGitRepository())
		errorParts += L"----------------------------\n" + g_Git.GetLibGit2LastErr();
	if (errorParts.IsEmpty())
		errorParts = L"no errors were reported";
	errorParts.TrimRight();

	CString error;
	error.AppendFormat(IDS_INACCESSIBLE, static_cast<LPCWSTR>(errorParts));

	if (errorParts.Find(L"libgit reported: not a git repository") == -1 && !orgCmdLinePath.GetWinPathString().IsEmpty() &&
		(PathFileExists(orgCmdLinePath.GetWinPathString() + L"\\.git") && !PathIsDirectory(orgCmdLinePath.GetWinPathString() + L"\\.git")
			|| errorParts.Find(L"detected dubious ownership in repository at '" + orgCmdLinePath.GetGitPathString() + L"'") != -1
			|| errorParts.Find(L"'" + orgCmdLinePath.GetGitPathString() + L"' is not owned by current user") != -1))
	{
		CString addSafeDirectory;
		addSafeDirectory.Format(IDS_PROC_ADDSAFEDIRECTORY, orgCmdLinePath.GetWinPath());
		if (CMessageBox::Show(GetExplorerHWND(), error, L"TortoiseGit", 1, IDI_INFORMATION, CString(MAKEINTRESOURCE(IDS_OKBUTTON)), addSafeDirectory) == 2)
		{
			CAutoConfig config(true);
			int err = git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitGlobalXDGConfig()), GIT_CONFIG_LEVEL_XDG, nullptr, FALSE);
			if (!err && (PathFileExists(g_Git.GetGitGlobalConfig()) || !PathFileExists(g_Git.GetGitGlobalXDGConfig())))
				err = git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitGlobalConfig()), GIT_CONFIG_LEVEL_GLOBAL, nullptr, FALSE);
			if (err)
			{
				::MessageBox(GetExplorerHWND(), g_Git.GetLibGit2LastErr(), L"TortoiseGit", MB_ICONEXCLAMATION);
				return false;
			}
			CString path = orgCmdLinePath.GetGitPathString();
			path.TrimRight(L"/");
			if (git_config_set_multivar(config, "safe.directory", "$^", CUnicodeUtils::GetUTF8(path)) < 0)
			{
				MessageBox(GetExplorerHWND(), g_Git.GetLibGit2LastErr(), L"TortoiseGit", MB_ICONEXCLAMATION);
				return false;
			}
		}
		return true;
	}

	::MessageBox(GetExplorerHWND(), error, L"TortoiseGit", MB_ICONINFORMATION);
	return true;
}
