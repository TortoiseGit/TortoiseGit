// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2010-2013, 2016 - TortoiseGit

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

// touch.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "resource.h"

int APIENTRY _tWinMain(HINSTANCE /*hInstance*/,
						HINSTANCE /*hPrevInstance*/,
						LPTSTR lpCmdLine,
						int /*nCmdShow*/)
{
	SetDllDirectory(L"");

	if (!lpCmdLine[0])
		return -1;

	if(lpCmdLine[0] == '\"')
	{
		++lpCmdLine;
		for (size_t i = 0; lpCmdLine[i]; ++i)
			if(lpCmdLine[i]== '\"')
			{
				lpCmdLine[i] = L'\0';
				break;
			}
	}
	HANDLE handle = CreateFile(lpCmdLine, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if(handle == INVALID_HANDLE_VALUE)
		return -1;
	CloseHandle(handle);

	DWORD attr=GetFileAttributes(lpCmdLine);
	SetFileAttributes(lpCmdLine,attr);
	return 0;
}
