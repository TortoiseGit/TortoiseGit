// TortoiseSVN - a Windows shell extension for easy version control

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
#include "StdAfx.h"
#include "Registry.h"
#include "TempFile.h"

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

CTSVNPath CTempFiles::GetTempFilePath(bool bRemoveAtEnd, const CTSVNPath& path /* = CTSVNPath() */, const SVNRev revision /* = SVNRev() */)
{
	DWORD len = ::GetTempPath(0, NULL);
	TCHAR * temppath = new TCHAR[len+1];
	TCHAR * tempF = new TCHAR[len+50];
	::GetTempPath (len+1, temppath);
	CTSVNPath tempfile;
	CString possibletempfile;
	if (path.IsEmpty())
	{
		::GetTempFileName (temppath, TEXT("svn"), 0, tempF);
		tempfile = CTSVNPath(tempF);
	}
	else
	{
		int i=0;
		do
		{
			if (revision.IsValid())
			{
				possibletempfile.Format(_T("%s%s-rev%s.svn%3.3x.tmp%s"), temppath, (LPCTSTR)path.GetFileOrDirectoryName(), (LPCTSTR)revision.ToString(), i, (LPCTSTR)path.GetFileExtension());
			}
			else
			{
				possibletempfile.Format(_T("%s%s.svn%3.3x.tmp%s"), temppath, (LPCTSTR)path.GetFileOrDirectoryName(), i, (LPCTSTR)path.GetFileExtension());
			}
			tempfile.SetFromWin(possibletempfile);
			i++;
		} while (PathFileExists(tempfile.GetWinPath()));
	}
	//now create the temp file, so that subsequent calls to GetTempFile() return
	//different filenames.
	HANDLE hFile = CreateFile(tempfile.GetWinPath(), GENERIC_READ, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, NULL);
	CloseHandle(hFile);
	delete [] temppath;
	delete [] tempF;
	if (bRemoveAtEnd)
		m_TempFileList.AddPath(tempfile);
	return tempfile;
}
