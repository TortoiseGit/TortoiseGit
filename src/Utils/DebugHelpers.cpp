// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng

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
#include "debughelpers.h"

CString GetLastErrorMessageString(int err)
{
	LPVOID lpMsgBuf;
	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		err,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL 
	);
	CString temp = CString((LPCTSTR)lpMsgBuf);
	LocalFree(lpMsgBuf);
	return temp;
};

CString GetLastErrorMessageString()
{
	LPVOID lpMsgBuf;
	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL 
	);
	CString temp = CString((LPCTSTR)lpMsgBuf);
	LocalFree(lpMsgBuf);
	return temp;
};

void SetThreadName(DWORD dwThreadID, LPCTSTR szThreadName)
{
#ifdef _UNICODE
	char narrow[_MAX_PATH * 3];
	BOOL defaultCharUsed;
	int ret = WideCharToMultiByte(CP_ACP, 0, szThreadName, (int)_tcslen(szThreadName), narrow, _MAX_PATH*3 - 1, ".", &defaultCharUsed);
	narrow[ret] = 0;
#endif
	THREADNAME_INFO info;
	info.dwType = 0x1000;
#ifdef _UNICODE
	info.szName = narrow;
#else
	info.szName = szThreadName;
#endif
	info.dwThreadID = dwThreadID;
	info.dwFlags = 0;

	__try
	{
		RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(DWORD),
			(const ULONG_PTR *)&info);
	}
	__except (EXCEPTION_CONTINUE_EXECUTION)
	{
	}
}

