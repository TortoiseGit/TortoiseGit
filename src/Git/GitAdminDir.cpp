// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2017, 2019, 2023, 2025-2026 - TortoiseGit
// Copyright (C) 2003-2008 - TortoiseSVN

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

#include "stdafx.h"
#include "UnicodeUtils.h"
#include "GitAdminDir.h"
#include "Git.h"
#include "SmartHandle.h"
#include "PathUtils.h"

CString GitAdminDir::GetSuperProjectRoot(const CString& path)
{
	CString projectroot=path;

	do
	{
		if (CGit::GitPathFileExists(projectroot + L"\\.git"))
		{
			if (CGit::GitPathFileExists(projectroot + L"\\.gitmodules"))
				return projectroot;
			else
				return L"";
		}

		projectroot.Truncate(max(0, projectroot.ReverseFind(L'\\')));

		// don't check for \\COMPUTERNAME\.git
		if (projectroot[0] == L'\\' && projectroot[1] == L'\\' && projectroot.Find(L'\\', 2) < 0)
			return L"";
	}while(projectroot.ReverseFind('\\')>0);

	return L"";
}

bool GitAdminDir::IsWorkingTreeOrBareRepo(const CString& path)
{
	return HasAdminDir(path) || IsBareRepo(path);
}

bool GitAdminDir::HasAdminDir(const CString& path)
{
	return HasAdminDir(path, !!PathIsDirectory(path));
}

bool GitAdminDir::HasAdminDir(const CString& path,CString* ProjectTopDir)
{
	return HasAdminDir(path, !!PathIsDirectory(path),ProjectTopDir);
}

bool GitAdminDir::HasAdminDir(const CString& path, bool bDir, CString* ProjectTopDir, bool* IsAdminDirPath)
{
	if (path.IsEmpty())
		return false;
	CString sDirName = path;
	if (!bDir)
	{
		// e.g "C:\"
		if (path.GetLength() <= 3)
			return false;
		sDirName.Truncate(max(0, sDirName.ReverseFind(L'\\')));
	}

	// a .git dir or anything inside it should be left out, only interested in working copy files -- Myagi
	if (GitAdminDir::IsAdminDirPath(sDirName))
	{
		if (IsAdminDirPath)
			*IsAdminDirPath = true;
		return false;
	}

	for (;;)
	{
		if (CGit::GitPathFileExists(sDirName + L"\\.git"))
		{
			// Make sure to add the trailing slash to root paths such as 'C:'
			if (sDirName.GetLength() == 2 && sDirName[1] == L':')
				sDirName += L'\\';

			if (ProjectTopDir)
				*ProjectTopDir = sDirName;

/* This code is not necessary in TGitCache as there are further checks for valid repos, however,
 * TODO: Optimize for usage in TGitCache
 */
#ifndef TGITCACHE
			CString adminDir;
			if (!GetAdminDirPath(sDirName, adminDir))
				return false;

			if (!PathFileExists(adminDir + L"\\HEAD") || !PathFileExists(adminDir + L"\\config"))
				return false;

			if (!PathFileExists(adminDir + L"\\objects\\") || !PathFileExists(adminDir + L"\\refs\\") || !PathIsDirectory(adminDir + L"\\refs\\heads")) // ".git/refs/heads" is a file when reftable-format is used
				return false;
#endif

			return true;
		}
#ifndef TGITCACHE
		// If there is a bare repo inside a regular repo, this needs to return false so that:
		// - GetAdminDirMask() returns 0 causing the context menu of the outer repo to not be shown
		// - CShellExt::IsMemberOf shows no overlays inside the bare repo
		else if (IsBareRepo(sDirName))
			return false;
#endif

		const int x = sDirName.ReverseFind(L'\\');
		if (x < 2)
			break;

		sDirName.Truncate(x);
		// don't check for \\COMPUTERNAME\.git
		if (sDirName[0] == L'\\' && sDirName[1] == L'\\' && sDirName.Find(L'\\', 2) < 0)
			break;
	}

	return false;
}
/**
 * Returns the .git-path (if .git is a file, read the repository path and return it)
 * adminDir always ends with "\"
 */
bool GitAdminDir::GetAdminDirPath(const CString& projectTopDir, CString& adminDir, bool* isWorktree)
{
	CString wtAdminDir;
	if (!GetWorktreeAdminDirPath(projectTopDir, wtAdminDir))
		return false;

	CString pathToCommonDir = wtAdminDir + L"commondir";
	if (!PathFileExists(pathToCommonDir))
	{
		adminDir = wtAdminDir;
		if (isWorktree)
			*isWorktree = false;
		return true;
	}

	CAutoFILE pFile = _wfsopen(pathToCommonDir, L"rb", SH_DENYWR);
	if (!pFile)
		return false;

	constexpr int size = 65536;
	CStringA commonDirA;
	const int length = static_cast<int>(fread(commonDirA.GetBufferSetLength(size), sizeof(char), size, pFile));
	commonDirA.ReleaseBuffer(length);
	CString commonDir = CUnicodeUtils::GetUnicode(commonDirA);
	commonDir.TrimRight(L"\r\n");
	commonDir.Replace(L'/', L'\\');
	if (PathIsRelative(commonDir))
		adminDir = CPathUtils::BuildPathWithPathDelimiter(wtAdminDir + commonDir);
	else
		adminDir = CPathUtils::BuildPathWithPathDelimiter(commonDir);
	if (isWorktree)
		*isWorktree = true;
	return true;
}

bool GitAdminDir::GetWorktreeAdminDirPath(const CString& projectTopDir, CString& adminDir)
{
	if (IsBareRepo(projectTopDir))
	{
		adminDir = CPathUtils::BuildPathWithPathDelimiter(projectTopDir);
		return true;
	}

	CString sDotGitPath = CPathUtils::BuildPathWithPathDelimiter(projectTopDir) + L".git";
	if (CTGitPath(sDotGitPath).IsDirectory())
	{
		adminDir = CPathUtils::BuildPathWithPathDelimiter(sDotGitPath);
		return true;
	}
	else
	{
		CString result = ReadGitLink(projectTopDir, sDotGitPath);
		if (result.IsEmpty())
			return false;
		adminDir = CPathUtils::BuildPathWithPathDelimiter(result);
		return true;
	}
}

static constexpr std::string_view GITDIR_PREFIX = "gitdir: ";

CString GitAdminDir::ReadGitLink(const CString& topDir, const CString& dotGitPath)
{
	CAutoFILE pFile = _wfsopen(dotGitPath, L"r", SH_DENYWR);

	if (!pFile)
		return L"";

	constexpr int size = 65536;
	auto buffer = std::make_unique<char[]>(size);
	const int length = static_cast<int>(fread(buffer.get(), sizeof(char), size, pFile));
	std::string_view gitPathA(buffer.get(), length);
	if (!gitPathA.starts_with(GITDIR_PREFIX))
		return L"";
	CString gitPath = CUnicodeUtils::GetUnicode(CStringUtils::TrimRight(gitPathA.substr(GITDIR_PREFIX.size()), "\r\n"));
	if (gitPath.IsEmpty())
		return gitPath;

	gitPath.Replace('/', '\\');
	if (gitPath.GetLength() >= 2 && (gitPath[1] == L':' || gitPath[0] == L'\\' && gitPath[1] == L'\\'))
	{
		if (gitPath.GetLength() > 2 && gitPath[1] == L':' && gitPath[2] != L'\\') // drive relative paths are unsupported in Git for Windows
			return {};
		CPathUtils::TrimTrailingPathDelimiter(gitPath);
		if (gitPath.IsEmpty())
			return {};
		return gitPath;
	}
	if (gitPath[0] == L'\\')
	{
		if (topDir.GetLength() < 2 || topDir[1] != L':')
			return {};
		CPathUtils::TrimTrailingPathDelimiter(gitPath);
		return topDir.Mid(0, 2) + gitPath;
	}
	gitPath = CPathUtils::BuildPathWithPathDelimiter(topDir) + gitPath;
	CString adminDir;
	PathCanonicalize(CStrBuf(adminDir, MAX_PATH), gitPath);
	return adminDir;
}

bool GitAdminDir::IsAdminDirPath(const WCHAR* path, const WCHAR** found)
{
	ATLASSERT(path);

	// Search for "\\.git\\" in the middle or "\\.git" at the end
	if (auto pFound = wcsstr(path, L"\\.git"); pFound != nullptr && (pFound[wcslen(L"\\.git")] == L'\\' || pFound[wcslen(L"\\.git")] == L'\0'))
	{
		if (found)
			*found = pFound;
		return true;
	}
	return false;
}

bool GitAdminDir::IsBareRepo(const CString& path)
{
	if (path.IsEmpty())
		return false;

	if (IsAdminDirPath(path))
		return false;

	// don't check for \\COMPUTERNAME\HEAD
	if (path[0] == L'\\' && path[1] == L'\\')
	{
		if (path.Find(L'\\', 2) < 0)
			return false;
	}

	if (!PathFileExists(path + L"\\HEAD") || !PathFileExists(path + L"\\config"))
		return false;

	if (!PathFileExists(path + L"\\objects\\") || !PathFileExists(path + L"\\refs\\") || !PathIsDirectory(path + L"\\refs\\heads")) // ".git/refs/heads" is a file when reftable-format is used
		return false;

	return true;
}
