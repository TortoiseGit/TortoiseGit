// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2016 - TortoiseGit
// Copyright (C) 2009-2010 - TortoiseSVN

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
#include "scope_exit_noexcept.h"

/**
 * A helper class for invoking CreateProcess(). The lpProcessInformation
 * can point to an uninitialized struct - it's memset to all zeroes inside.
 */

class CCreateProcessHelper
{
public:
	static bool CreateProcess(LPCTSTR lpApplicationName, LPTSTR lpCommandLine, LPCTSTR lpCurrentDirectory, LPPROCESS_INFORMATION lpProcessInformation);
	static bool CreateProcess(LPCTSTR lpApplicationName, LPTSTR lpCommandLine, LPPROCESS_INFORMATION lpProcessInformation);

	static bool CreateProcessDetached(LPCTSTR lpApplicationName, LPTSTR lpCommandLine, LPCTSTR lpCurrentDirectory);
	static bool CreateProcessDetached(LPCTSTR lpApplicationName, LPTSTR lpCommandLine);
	static bool CreateProcessDetached(LPCTSTR lpApplicationName, LPCTSTR lpCommandLine);
};

inline bool CCreateProcessHelper::CreateProcess(LPCTSTR applicationName,
	LPTSTR commandLine, LPCTSTR currentDirectory,
	LPPROCESS_INFORMATION processInfo)
{
	STARTUPINFO startupInfo = { 0 };
	startupInfo.cb = sizeof(STARTUPINFO);

	memset(processInfo, 0, sizeof(PROCESS_INFORMATION));
	const BOOL result = ::CreateProcess(applicationName, commandLine, nullptr, nullptr, FALSE, CREATE_UNICODE_ENVIRONMENT, nullptr, currentDirectory, &startupInfo, processInfo);
	return result != 0;
}

inline bool CCreateProcessHelper::CreateProcess(LPCTSTR applicationName,
	LPTSTR commandLine, LPPROCESS_INFORMATION processInformation)
{
	return CreateProcess( applicationName, commandLine, nullptr, processInformation );
}

inline bool CCreateProcessHelper::CreateProcessDetached(LPCTSTR lpApplicationName,
	LPTSTR lpCommandLine, LPCTSTR lpCurrentDirectory)
{
	PROCESS_INFORMATION process;
	if (!CreateProcess(lpApplicationName, lpCommandLine, lpCurrentDirectory, &process))
		return false;

	CloseHandle(process.hThread);
	CloseHandle(process.hProcess);
	return true;
}

inline bool CCreateProcessHelper::CreateProcessDetached(LPCTSTR lpApplicationName,
	LPTSTR lpCommandLine)
{
	return CreateProcessDetached(lpApplicationName, lpCommandLine, nullptr);
}

inline bool CCreateProcessHelper::CreateProcessDetached(LPCTSTR lpApplicationName, LPCTSTR lpCommandLine)
{
	// CreateProcess may modify buffer specified by lpCommandLine:
	// https://msdn.microsoft.com/en-us/library/windows/desktop/ms682425
	// [[
	// The Unicode version of this function, CreateProcessW, can modify
	// the contents of this string. Therefore, this parameter cannot be a
	// pointer to read - only memory(such as a const variable or a literal
	// string).If this parameter is a constant string, the function may
	// cause an access violation.
	// ]]
	LPWSTR commandLineBuf = nullptr;
	if (lpCommandLine)
		commandLineBuf = _wcsdup(lpCommandLine);
	SCOPE_EXIT { free(reinterpret_cast<void*>(commandLineBuf)); };

	return CreateProcessDetached(lpApplicationName, commandLineBuf, nullptr);
}
