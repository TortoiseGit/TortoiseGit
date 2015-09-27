// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2015 - TortoiseGit
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

GitAdminDir::GitAdminDir()
{
}

GitAdminDir::~GitAdminDir()
{
}

CString GitAdminDir::GetSuperProjectRoot(const CString& path)
{
	CString projectroot=path;

	do
	{
		if (CGit::GitPathFileExists(projectroot + _T("\\.git")))
		{
			if (CGit::GitPathFileExists(projectroot + _T("\\.gitmodules")))
				return projectroot;
			else
				return _T("");
		}

		projectroot = projectroot.Left(projectroot.ReverseFind('\\'));

		// don't check for \\COMPUTERNAME\.git
		if (projectroot[0] == _T('\\') && projectroot[1] == _T('\\') && projectroot.Find(_T('\\'), 2) < 0)
			return _T("");
	}while(projectroot.ReverseFind('\\')>0);

	return _T("");

}

CString GitAdminDir::GetGitTopDir(const CString& path)
{
	CString str;
	HasAdminDir(path,!!PathIsDirectory(path),&str);
	return str;
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

bool GitAdminDir::HasAdminDir(const CString& path, bool bDir, CString* ProjectTopDir)
{
	if (path.IsEmpty())
		return false;
	CString sDirName = path;
	if (!bDir)
	{
		// e.g "C:\"
		if (path.GetLength() <= 3)
			return false;
		sDirName = path.Left(path.ReverseFind(_T('\\')));
	}

	// a .git dir or anything inside it should be left out, only interested in working copy files -- Myagi
	{
	int n = 0;
	for (;;)
	{
		n = sDirName.Find(_T("\\.git"), n);
		if (n < 0)
		{
			break;
		}

		// check for actual .git dir (and not .gitignore or something else), continue search if false match
		n += 5;
		if (sDirName[n] == _T('\\') || sDirName[n] == 0)
		{
			return false;
		}
	}
	}

	for (;;)
	{
		if(CGit::GitPathFileExists(sDirName + _T("\\.git")))
		{
			if(ProjectTopDir)
			{
				*ProjectTopDir=sDirName;
				// Make sure to add the trailing slash to root paths such as 'C:'
				if (sDirName.GetLength() == 2 && sDirName[1] == _T(':'))
					(*ProjectTopDir) += _T("\\");
			}
			return true;
		}
		else if (IsBareRepo(sDirName))
			return false;

		int x = sDirName.ReverseFind(_T('\\'));
		if (x < 2)
			break;

		sDirName = sDirName.Left(x);
		// don't check for \\COMPUTERNAME\.git
		if (sDirName[0] == _T('\\') && sDirName[1] == _T('\\') && sDirName.Find(_T('\\'), 2) < 0)
			break;
	}

	return false;

}
/**
 * Returns the .git-path (if .git is a file, read the repository path and return it)
 * adminDir always ends with "\"
 */
bool GitAdminDir::GetAdminDirPath(const CString &projectTopDir, CString& adminDir)
{
	if (IsBareRepo(projectTopDir))
	{
		adminDir = projectTopDir;
		adminDir.TrimRight('\\');
		adminDir.Append(_T("\\"));
		return true;
	}

	CString sDotGitPath = projectTopDir + _T("\\") + GetAdminDirName();
	if (CTGitPath(sDotGitPath).IsDirectory())
	{
		sDotGitPath.TrimRight('\\');
		sDotGitPath.Append(_T("\\"));
		adminDir = sDotGitPath;
		return true;
	}
	else
	{
		CString result = ReadGitLink(projectTopDir, sDotGitPath);
		if (result.IsEmpty())
			return false;
		adminDir = result + _T("\\");
		return true;
	}
}

CString GitAdminDir::ReadGitLink(const CString& topDir, const CString& dotGitPath)
{
	CAutoFILE pFile = _tfsopen(dotGitPath, _T("r"), SH_DENYWR);

	if (!pFile)
		return _T("");

	int size = 65536;
	auto buffer = std::make_unique<char[]>(size);
	int length = (int)fread(buffer.get(), sizeof(char), size, pFile);
	CStringA gitPathA(buffer.get(), length);
	if (length < 8 || gitPathA.Left(8) != "gitdir: ")
		return _T("");
	CString gitPath = CUnicodeUtils::GetUnicode(gitPathA);
	// trim after converting to UTF-16, because CStringA trim does not work when having UTF-8 chars
	gitPath = gitPath.Trim().Mid(8); // 8 = len("gitdir: ")
	gitPath.Replace('/', '\\');
	gitPath.TrimRight('\\');
	if (!gitPath.IsEmpty() && gitPath[0] == _T('.'))
	{
		gitPath = topDir + _T("\\") + gitPath;
		CString adminDir;
		PathCanonicalize(CStrBuf(adminDir, MAX_PATH), gitPath);
		return adminDir;
	}
	return gitPath;
}

bool GitAdminDir::IsAdminDirPath(const CString& path)
{
	if (path.IsEmpty())
		return false;
	bool bIsAdminDir = false;
	CString lowerpath = path;
	lowerpath.MakeLower();
	int ind1 = 0;
	while ((ind1 = lowerpath.Find(_T("\\.git"), ind1))>=0)
	{
		int ind = ind1++;
		if (ind == (lowerpath.GetLength() - 5))
		{
			bIsAdminDir = true;
			break;
		}
		else if (lowerpath.Find(_T("\\.git\\"), ind)>=0)
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
	if (path[0] == _T('\\') && path[1] == _T('\\'))
	{
		if (path.Find(_T('\\'), 2) < 0)
			return false;
	}

	if (!PathFileExists(path + _T("\\HEAD")) || !PathFileExists(path + _T("\\config")))
		return false;

	if (!PathFileExists(path + _T("\\objects\\")) || !PathFileExists(path + _T("\\refs\\")))
		return false;

	return true;
}
