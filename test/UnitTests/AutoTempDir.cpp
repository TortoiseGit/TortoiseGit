// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2015-2016 - TortoiseGit

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
	GetTempFileName(temppath, L"tgit-tests", 0, szTempName);
	DeleteFile(szTempName);
	CreateDirectory(szTempName, nullptr);
	tempdir = szTempName;
}

void CAutoTempDir::DeleteDirectoryRecursive(const CString& dir)
{
	WIN32_FIND_DATA ffd;
	HANDLE hp = FindFirstFile(dir + L"\\*", &ffd);
	do
	{
		if (!wcscmp(ffd.cFileName, L".") || !wcscmp(ffd.cFileName, L".."))
			continue;
		if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
		{
			CString subdir = dir + L'\\' + ffd.cFileName;
			DeleteDirectoryRecursive(subdir);
		}
		else
		{
			CString file = dir + L'\\' + ffd.cFileName;
			bool failed = !DeleteFile(file);
			if (failed && GetLastError() == ERROR_ACCESS_DENIED)
			{
				SetFileAttributes(file, GetFileAttributes(file) & ~FILE_ATTRIBUTE_READONLY);
				failed = !DeleteFile(file);
			}
		}
	} while(FindNextFile(hp, &ffd));
	FindClose(hp);

	RemoveDirectory(dir);
}

CAutoTempDir::~CAutoTempDir()
{
	if (!tempdir.IsEmpty())
		DeleteDirectoryRecursive(tempdir);
}

CString CAutoTempDir::GetTempDir() const
{
	return tempdir;
}
