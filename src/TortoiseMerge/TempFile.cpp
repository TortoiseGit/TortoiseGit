// TortoiseGitMerge - a Windows shell extension for easy version control

// Copyright (C) 2019 - TortoiseGit
// Copyright (C) 2003-2012 - TortoiseSVN

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
#include "TempFile.h"
#include "PathUtils.h"
#include "DirFileEnum.h"
#include "SmartHandle.h"

CTempFiles::CTempFiles(void)
{
}

CTempFiles::~CTempFiles(void)
{
	m_TempFileList.DeleteAllFiles(false, false);
}

CTempFiles& CTempFiles::Instance()
{
	static CTempFiles instance;
	return instance;
}

CTGitPath CTempFiles::ConstructTempPath(const CTGitPath& path)
{
	DWORD len = ::GetTempPath(0, nullptr);
	auto temppath = std::make_unique<TCHAR[]>(len + 1);
	auto tempF = std::make_unique<TCHAR[]>(len + 50);
	::GetTempPath (len+1, temppath.get());
	CTGitPath tempfile;
	CString possibletempfile;
	if (path.IsEmpty())
	{
		::GetTempFileName(temppath.get(), L"tsm", 0, tempF.get());
		tempfile = CTGitPath (tempF.get());
	}
	else
	{
		int i=0;
		do
		{
			// use the UI path, which does unescaping for urls
			CString filename = path.GetUIFileOrDirectoryName();
			// remove illegal chars which could be present in urls
			filename.Remove('?');
			filename.Remove('*');
			filename.Remove('<');
			filename.Remove('>');
			filename.Remove('|');
			filename.Remove('"');
			// the inner loop assures that the resulting path is < MAX_PATH
			// if that's not possible without reducing the 'filename' to less than 5 chars, use a path
			// that's longer than MAX_PATH (in that case, we can't really do much to avoid longer paths)
			do
			{
				possibletempfile.Format(L"%s%s.tsm%3.3x.tmp%s", temppath.get(), static_cast<LPCTSTR>(filename), i, static_cast<LPCTSTR>(path.GetFileExtension()));
				tempfile.SetFromWin(possibletempfile);
				filename.Truncate(std::max(0, filename.GetLength() - 1));
			} while (   (filename.GetLength() > 4)
					 && (tempfile.GetWinPathString().GetLength() >= MAX_PATH));
			i++;
			// now create the temp file in a thread safe way, so that subsequent calls to GetTempFile() return different filenames.
			CAutoFile hFile = CreateFile(tempfile.GetWinPath(), GENERIC_READ, FILE_SHARE_READ, nullptr, CREATE_NEW, FILE_ATTRIBUTE_TEMPORARY, nullptr);
			if (hFile || GetLastError() != ERROR_FILE_EXISTS)
				break;
		} while (true);
	}

	// caller has to actually grab the file path

	return tempfile;
}

CTGitPath CTempFiles::CreateTempPath (bool bRemoveAtEnd, const CTGitPath& path, bool directory)
{
	bool succeeded = false;
	for (int retryCount = 0; retryCount < MAX_RETRIES; ++retryCount)
	{
		CTGitPath tempfile = ConstructTempPath (path);

		// now create the temp file / directory, so that subsequent calls to GetTempFile() return
		// different filenames.
		// Handle races, i.e. name collisions.

		if (directory)
		{
			DeleteFile(tempfile.GetWinPath());
			if (CreateDirectory(tempfile.GetWinPath(), nullptr) == FALSE)
			{
				if (GetLastError() != ERROR_ALREADY_EXISTS)
					return CTGitPath();
			}
			else
				succeeded = true;
		}
		else
		{
			CAutoFile hFile = CreateFile(tempfile.GetWinPath(), GENERIC_READ, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, nullptr);
			if (!hFile)
			{
				if (GetLastError() != ERROR_ALREADY_EXISTS)
					return CTGitPath();
			}
			else
			{
				succeeded = true;
			}
		}

		// done?

		if (succeeded)
		{
			if (bRemoveAtEnd)
				m_TempFileList.AddPath(tempfile);

			return tempfile;
		}
	}

	// give up

	return CTGitPath();
}

CTGitPath CTempFiles::GetTempFilePath(bool bRemoveAtEnd, const CTGitPath& path /* = CTGitPath() */)
{
	return CreateTempPath (bRemoveAtEnd, path, false);
}

CString CTempFiles::GetTempFilePathString()
{
	return CreateTempPath (true, CTGitPath(), false).GetWinPathString();
}

CTGitPath CTempFiles::GetTempDirPath(bool bRemoveAtEnd, const CTGitPath& path /* = CTGitPath() */)
{
	return CreateTempPath (bRemoveAtEnd, path, true);
}

void CTempFiles::DeleteOldTempFiles(LPCTSTR wildCard)
{
	DWORD len = ::GetTempPath(0, nullptr);
	auto path = std::make_unique<TCHAR[]>(len + 100);
	len = ::GetTempPath (len+100, path.get());
	if (len == 0)
		return;

	CSimpleFileFind finder = CSimpleFileFind(path.get(), wildCard);
	FILETIME systime_;
	::GetSystemTimeAsFileTime(&systime_);
	__int64 systime = static_cast<__int64>(systime_.dwHighDateTime) << 32 | systime_.dwLowDateTime;
	while (finder.FindNextFileNoDirectories())
	{
		CString filepath = finder.GetFilePath();

		FILETIME createtime_ = finder.GetCreateTime();
		__int64 createtime = static_cast<__int64>(createtime_.dwHighDateTime) << 32 | createtime_.dwLowDateTime;
		createtime += 864000000000LL;      //only delete files older than a day
		if (createtime < systime)
		{
			::SetFileAttributes(filepath, FILE_ATTRIBUTE_NORMAL);
			::DeleteFile(filepath);
		}
	}
}

