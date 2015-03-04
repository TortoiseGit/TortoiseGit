// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2015 - TortoiseGit

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
#include "AutoTempDir.h"
#include "TGitPath.h"

CAutoTempDir::CAutoTempDir()
{
	CString temppath;
	GetTempPath(MAX_PATH, temppath.GetBufferSetLength(MAX_PATH));
	TCHAR szTempName[MAX_PATH] = { 0 };
	temppath.ReleaseBuffer();
	GetTempFileName(temppath, _T("tgit-tests"), 0, szTempName);
	DeleteFile(szTempName);
	CreateDirectory(szTempName, nullptr);
	tempdir = szTempName;
}

CAutoTempDir::~CAutoTempDir()
{
	if (!tempdir.IsEmpty())
		CTGitPath(tempdir).Delete(false);
}

CString CAutoTempDir::GetTempDir() const
{
	return tempdir;
}
