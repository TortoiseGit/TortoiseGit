// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2017, 2019 - TortoiseGit
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
	int n = 0;
	for (;;)
	{
		n = sDirName.Find(L"\\.git", n);
		if (n < 0)
			break;

		// check for actual .git dir (and not .gitignore or something else), continue search if false match
		n += 5;
		if (sDirName[n] == L'\\' || sDirName[n] == 0)
		{
			if (IsAdminDirPath)
				*IsAdminDirPath = true;
			return false;
		}
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

			if (!PathFileExists(adminDir + L"\\objects\\") || !PathFileExists(adminDir + L"\\refs\\"))
				return false;
#endif

			return true;
		}
		else if (IsBareRepo(sDirName))
			return false;

		int x = sDirName.ReverseFind(L'\\');
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

	int size = 65536;
	CStringA commonDirA;
	int length = static_cast<int>(fread(commonDirA.GetBufferSetLength(size), sizeof(char), size, pFile));
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

	CString sDotGitPath = CPathUtils::BuildPathWithPathDelimiter(projectTopDir) + GetAdminDirName();
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

CString GitAdminDir::ReadGitLink(const CString& topDir, const CString& dotGitPath)
{
	CAutoFILE pFile = _wfsopen(dotGitPath, L"r", SH_DENYWR);

	if (!pFile)
		return L"";

	int size = 65536;
	auto buffer = std::make_unique<char[]>(size);
	int length = static_cast<int>(fread(buffer.get(), sizeof(char), size, pFile));
	CStringA gitPathA(buffer.get(), length);
	if (length < 8 || !CStringUtils::StartsWith(gitPathA, "gitdir: "))
		return L"";
	CString gitPath = CUnicodeUtils::GetUnicode(gitPathA);
	// trim after converting to UTF-16, because CStringA trim does not work when having UTF-8 chars
	gitPath = gitPath.Trim().Mid(static_cast<int>(wcslen(L"gitdir: ")));
	if (gitPath.IsEmpty())
		return gitPath;

	gitPath.Replace('/', '\\');
	if (gitPath[0] == L'\\' || gitPath.GetLength() >= 2 && gitPath[1] == L':')
	{
		CPathUtils::TrimTrailingPathDelimiter(gitPath);
		return gitPath;
	}
	gitPath = CPathUtils::BuildPathWithPathDelimiter(topDir) + gitPath;
	CString adminDir;
	PathCanonicalize(CStrBuf(adminDir, MAX_PATH), gitPath);
	return adminDir;
}

bool GitAdminDir::IsAdminDirPath(const CString& path)
{
	if (path.IsEmpty())
		return false;
	bool bIsAdminDir = false;
	CString lowerpath = path;
	lowerpath.MakeLower();
	int ind1 = 0;
	while ((ind1 = lowerpath.Find(L"\\.git", ind1)) >= 0)
	{
		int ind = ind1++;
		if (ind == (lowerpath.GetLength() - 5))
		{
			bIsAdminDir = true;
			break;
		}
		else if (lowerpath.Find(L"\\.git\\", ind) >= 0)
		{
			bIsAdminDir = true;
			break;
		}
	}

	return bIsAdminDir;
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

	if (!PathFileExists(path + L"\\objects\\") || !PathFileExists(path + L"\\refs\\"))
		return false;

	return true;
}
