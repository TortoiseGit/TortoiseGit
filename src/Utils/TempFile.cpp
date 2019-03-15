// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009, 2011-2013, 2015-2016, 2018-2019 - TortoiseGit
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
//
#include "stdafx.h"
#include "registry.h"
#include "TempFile.h"
#include "TGitPath.h"
#include "SmartHandle.h"
#include "Git.h"

CTempFiles::CTempFiles(void)
{
}

CTempFiles::~CTempFiles(void)
{
	m_TempFileList.DeleteAllFiles(false);
}

CTempFiles& CTempFiles::Instance()
{
	static CTempFiles instance;
	return instance;
}

CTGitPath CTempFiles::GetTempFilePath(bool bRemoveAtEnd, const CTGitPath& path /* = CTGitPath() */, const CGitHash& hash /* = CGitHash() */)
{
	DWORD len = GetTortoiseGitTempPath(0, nullptr);

	auto temppath = std::make_unique<TCHAR[]>(len + 1);
	auto tempF = std::make_unique<TCHAR[]>(len + 50);
	GetTortoiseGitTempPath(len + 1, temppath.get());
	CTGitPath tempfile;
	CString possibletempfile;
	if (path.IsEmpty())
	{
		::GetTempFileName (temppath.get(), L"git", 0, tempF.get());
		tempfile = CTGitPath(tempF.get());
	}
	else
	{
		int i=0;
		do
		{
			if (!hash.IsEmpty())
				possibletempfile.Format(L"%s%s-%s.%3.3x%s", temppath.get(), static_cast<LPCTSTR>(path.GetBaseFilename()), static_cast<LPCTSTR>(hash.ToString().Left(g_Git.GetShortHASHLength())), i, static_cast<LPCTSTR>(path.GetFileExtension()));
			else
				possibletempfile.Format(L"%s%s.%3.3x%s", temppath.get(), static_cast<LPCTSTR>(path.GetBaseFilename()), i, static_cast<LPCTSTR>(path.GetFileExtension()));
			tempfile.SetFromWin(possibletempfile);
			++i;
			// now create the temp file in a thread safe way, so that subsequent calls to GetTempFile() return different filenames.
			CAutoFile hFile = CreateFile(tempfile.GetWinPath(), GENERIC_READ, FILE_SHARE_READ, nullptr, CREATE_NEW, FILE_ATTRIBUTE_TEMPORARY, nullptr);
			if (hFile || GetLastError() != ERROR_FILE_EXISTS)
				break;
		} while (true);
	}
	if (bRemoveAtEnd)
		m_TempFileList.AddPath(tempfile);
	return tempfile;
}
