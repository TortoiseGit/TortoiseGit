// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseGit

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

bool GitAdminDir::IsAdminDirName(const CString& name) const
{
#if 0
	CStringA nameA = CUnicodeUtils::GetUTF8(name).MakeLower();
	return !!svn_wc_is_adm_dir(nameA, m_pool);
#endif
	return name == ".git";
}

bool GitAdminDir::HasAdminDir(const CString& path) const
{
	return HasAdminDir(path, !!PathIsDirectory(path));
}

bool GitAdminDir::HasAdminDir(const CString& path, bool bDir) const
{
	if (path.IsEmpty())
		return false;
	bool bHasAdminDir = false;
	CString sDirName = path;
	if (!bDir)
	{
		sDirName = path.Left(path.ReverseFind('\\'));
	}
	
	do
	{
		if(PathFileExists(sDirName + _T("\\.git")))
			return true;
		sDirName = sDirName.Left(sDirName.ReverseFind('\\'));

	}while(sDirName.ReverseFind('\\')>0);

	return false;
	
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

