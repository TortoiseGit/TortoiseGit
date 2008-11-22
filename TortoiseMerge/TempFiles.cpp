// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006,2008 - Stefan Kueng

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
#include ".\tempfiles.h"

CTempFiles::CTempFiles(void)
{
}

CTempFiles::~CTempFiles(void)
{
	for (int i=0; i<m_arTempFileList.GetCount(); i++)
	{
		DeleteFile(m_arTempFileList.GetAt(i));
	}
}

CString CTempFiles::GetTempFilePath()
{
	DWORD len = GetTempPath(0, NULL);
	TCHAR * path = new TCHAR[len+1];
	TCHAR * tempF = new TCHAR[len+100];
	GetTempPath (len+1, path);
	GetTempFileName (path, TEXT("tsm"), 0, tempF);
	CString tempfile = CString(tempF);
	delete [] path;
	delete [] tempF;
	m_arTempFileList.Add(tempfile);
	return tempfile;
}
