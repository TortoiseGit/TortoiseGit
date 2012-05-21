// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2012 - TortoiseGit
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

#include "StdAfx.h"
#include "UnicodeUtils.h"
#include "GitAdminDir.h"
#include "Git.h"
GitAdminDir g_GitAdminDir;

GitAdminDir::GitAdminDir()
	: m_nInit(0)
//	, m_bVSNETHack(false)
//	, m_pool(NULL)
{
}

GitAdminDir::~GitAdminDir()
{
//	if (m_nInit)
//		 (m_pool);
}

bool GitAdminDir::Init()
{
	if (m_nInit==0)
	{
#if 0
		m_bVSNETHack = false;
		m_pool = svn_pool_create(NULL);
		size_t ret = 0;
		getenv_s(&ret, NULL, 0, "GIT_ASP_DOT_NET_HACK");
		if (ret)
		{
			svn_error_clear(svn_wc_set_adm_dir("_git", m_pool));
			m_bVSNETHack = true;
		}
#endif
	}
	m_nInit++;
	return true;
}

bool GitAdminDir::Close()
{
	m_nInit--;
	if (m_nInit>0)
		return false;
#if 0
	svn_pool_destroy(m_pool);
#endif
	return true;
}

CString GitAdminDir::GetSuperProjectRoot(const CString& path)
{
	CString projectroot=path;

	do
	{
		if(CGit::GitPathFileExists(projectroot + _T("\\.gitmodules")))
		{
			return projectroot;
		}

		projectroot = projectroot.Left(projectroot.ReverseFind('\\'));

	}while(projectroot.ReverseFind('\\')>0);

	return _T("");

}

bool GitAdminDir::IsAdminDirName(const CString& name) const
{
#if 0
	CStringA nameA = CUnicodeUtils::GetUTF8(name).MakeLower();
	return !!svn_wc_is_adm_dir(nameA, m_pool);
#endif
	return name == ".git";
}
CString GitAdminDir::GetGitTopDir(const CString& path)
{
	CString str;
	str=_T("");
	HasAdminDir(path,!!PathIsDirectory(path),&str);
	return str;
}

bool GitAdminDir::HasAdminDir(const CString& path) const
{
	return HasAdminDir(path, !!PathIsDirectory(path));
}

bool GitAdminDir::HasAdminDir(const CString& path,CString *ProjectTopDir) const
{
	return HasAdminDir(path, !!PathIsDirectory(path),ProjectTopDir);
}

bool GitAdminDir::HasAdminDir(const CString& path, bool bDir,CString *ProjectTopDir) const
{
	if (path.IsEmpty())
		return false;
	CString sDirName = path;
	if (!bDir)
	{
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

		int x = sDirName.ReverseFind(_T('\\'));
		if (x < 2)
			break;

		sDirName = sDirName.Left(x);
	}

	return false;

}
/**
 * Returns the .git-path (if .git is a file, read the repository path and return it)
 * adminDir always ends with "\"
 */
bool GitAdminDir::GetAdminDirPath(const CString &projectTopDir, CString &adminDir) const
{
	if (IsBareRepo(projectTopDir))
	{
		adminDir = projectTopDir;
		adminDir.TrimRight('\\');
		adminDir.Append(_T("\\"));
		return true;
	}

	CString sDotGitPath = projectTopDir + _T("\\") + g_GitAdminDir.GetAdminDirName();
	if (CTGitPath(sDotGitPath).IsDirectory())
	{
		sDotGitPath.TrimRight('\\');
		sDotGitPath.Append(_T("\\"));
		adminDir = sDotGitPath;
		return true;
	}
	else
	{
		FILE *pFile;
		_tfopen_s(&pFile, sDotGitPath, _T("r"));

		if (!pFile)
			return false;

		char s[MAX_PATH + 8] = {0};
		fgets(s, sizeof(s), pFile);

		fclose(pFile);
		CString gitPath(s);
		if (gitPath.Find(L"gitdir: ") != 0)
			return -1;
		gitPath = gitPath.Trim().Mid(8); // 8 = len("gitdir: ")
		gitPath.Replace('/', '\\');
		gitPath.TrimRight('\\');
		gitPath.Append(_T("\\"));
		if (gitPath.GetLength() > 0 && gitPath[0] == _T('.'))
		{
			gitPath = projectTopDir + _T("\\") + gitPath;
			PathCanonicalize(adminDir.GetBuffer(MAX_PATH), gitPath.GetBuffer());
			adminDir.ReleaseBuffer();
			gitPath.ReleaseBuffer();
			return true;
		}
		adminDir = gitPath;
		return true;
	}
}

bool GitAdminDir::IsAdminDirPath(const CString& path) const
{
	if (path.IsEmpty())
		return false;
	bool bIsAdminDir = false;
	CString lowerpath = path;
	lowerpath.MakeLower();
	int ind = -1;
	int ind1 = 0;
	while ((ind1 = lowerpath.Find(_T("\\.git"), ind1))>=0)
	{
		ind = ind1++;
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

bool GitAdminDir::IsBareRepo(const CString& path) const
{
	if (path.IsEmpty())
		return false;

	if (IsAdminDirPath(path))
		return false;

	if (!PathFileExists(path + _T("\\HEAD")) || !PathFileExists(path + _T("\\config")))
		return false;

	if (!PathFileExists(path + _T("\\objects\\")) || !PathFileExists(path + _T("\\refs\\")))
		return false;

	return true;
}
