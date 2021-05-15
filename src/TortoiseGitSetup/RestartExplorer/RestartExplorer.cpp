// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2016, 2021 - TortoiseGit

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

// RestartExplorer.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "resource.h"
#include "CreateProcessHelper.h"
#include <ShlObj.h>
#include "scope_exit_noexcept.h"

int APIENTRY wWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPTSTR /*lpCmdLine*/, int /*nCmdShow*/)
{
	SetDllDirectory(L"");

	int i = 8;
	do
	{
		Sleep(500);
		if (FindWindow(L"Shell_TrayWnd", nullptr))
			return 0;
	} while (i-- > 0);


	PWSTR pszPathWindows;
	if (FAILED(SHGetKnownFolderPath(FOLDERID_Windows, 0, nullptr, &pszPathWindows)))
		return 1;

	SCOPE_EXIT { CoTaskMemFree(pszPathWindows); };

	TCHAR szPathExplorerExe[MAX_PATH];
	if (_snwprintf_s(szPathExplorerExe, _countof(szPathExplorerExe) - 1, L"%s\\explorer.exe", pszPathWindows) <= 0)
		return 1;

	return CCreateProcessHelper::CreateProcessDetached(szPathExplorerExe, nullptr, pszPathWindows) ? 0 : 1;
}
