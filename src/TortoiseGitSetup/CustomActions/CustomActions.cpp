// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2008, 2012 - TortoiseSVN

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


/* BIG FAT WARNING: Do not use any functions which require the C-Runtime library
   in this custom action dll! The runtimes might not be installed yet!
*/

#include "stdafx.h"
#include <shlwapi.h>
#include <shellapi.h>
#pragma comment(lib, "shlwapi")
#pragma comment(lib, "shell32")

#define TGIT_CACHE_WINDOW_NAME _T("TGitCacheWindow")

BOOL APIENTRY DllMain( HANDLE /*hModule*/,
					   DWORD  /*ul_reason_for_call*/,
					   LPVOID /*lpReserved*/
					 )
{
	return TRUE;
}

UINT __stdcall TerminateCache(MSIHANDLE /*hModule*/)
{
	HWND hWnd = FindWindow(TGIT_CACHE_WINDOW_NAME, TGIT_CACHE_WINDOW_NAME);
	if (hWnd)
	{
		PostMessage(hWnd, WM_CLOSE, nullptr, nullptr);
		for (int i=0; i<10; ++i)
		{
			Sleep(500);
			if (!IsWindow(hWnd))
			{
				// Cache is gone!
				return ERROR_SUCCESS;
			}
		}
		// Don't return ERROR_FUNCTION_FAILED, because even if the cache is still
		// running, the installer will overwrite the file, and we require a
		// reboot anyway after upgrading.
		return ERROR_SUCCESS;
	}
	// cache wasn't even running
	return ERROR_SUCCESS;
}

UINT __stdcall OpenDonatePage(MSIHANDLE /*hModule*/)
{
	ShellExecute(nullptr, _T("open"), _T("https://tortoisegit.org/donate"), nullptr, nullptr, SW_SHOW);
	return ERROR_SUCCESS;
}

UINT __stdcall MsgBox(MSIHANDLE /*hModule*/)
{
	MessageBox(nullptr, _T("CustomAction \"MsgBox\" running"), _T("Installer"), MB_ICONINFORMATION);
	return ERROR_SUCCESS;
}
