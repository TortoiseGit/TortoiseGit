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
#pragma once

#ifdef _DEBUG
#	define BEGIN_TICK   { auto dwTickMeasureBegin = ::GetTickCount64();
#	define END_TICK(s) auto dwTickMeasureEnd = ::GetTickCount64(); TRACE("%s: tick count = %d\n", s, dwTickMeasureEnd-dwTickMeasureBegin); }
#else
#	define BEGIN_TICK
#	define END_TICK(s)
#endif

/**
 * \ingroup CommonClasses
 * returns the string to the given error number.
 * \param err the error number, obtained with GetLastError() or WSAGetLastError() or ...
 */
CString GetLastErrorMessageString(int err);
/*
 * \ingroup CommonClasses
 * returns the string to the GetLastError() function.
 */
CString GetLastErrorMessageString();

#define MS_VC_EXCEPTION 0x406d1388

typedef struct tagTHREADNAME_INFO
{
	DWORD dwType;        // must be 0x1000
	LPCSTR szName;       // pointer to name (in same addr space)
	DWORD dwThreadID;    // thread ID (-1 caller thread)
	DWORD dwFlags;       // reserved for future use, most be zero
} THREADNAME_INFO;

/**
 * Sets a name for a thread. The Thread name must not exceed 9 characters!
 * Inside the current thread you can use -1 for dwThreadID.
 * \param dwThreadID The Thread ID
 * \param szThreadName A name for the thread.
 */
void SetThreadName(DWORD dwThreadID, LPCTSTR szThreadName);
