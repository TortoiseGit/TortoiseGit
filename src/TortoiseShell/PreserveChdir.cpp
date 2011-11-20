// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2007 - TortoiseSVN

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
#include "PreserveChdir.h"

PreserveChdir::PreserveChdir()
{
	DWORD len = GetCurrentDirectory(0, NULL);
	if (len)
	{
		originalCurrentDirectory = new TCHAR[len];
		if (GetCurrentDirectory(len, originalCurrentDirectory)==0)
		{
			delete [] originalCurrentDirectory;
			originalCurrentDirectory = NULL;
		}
	}
	else
		originalCurrentDirectory = NULL;
}

PreserveChdir::~PreserveChdir()
{
	if (originalCurrentDirectory)
	{
		DWORD len = GetCurrentDirectory(0, NULL);
		TCHAR * currentDirectory = new TCHAR[len];

		// _tchdir is an expensive function - don't call it unless we really have to
		GetCurrentDirectory(len, currentDirectory);
		if(_tcscmp(currentDirectory, originalCurrentDirectory) != 0)
		{
			SetCurrentDirectory(originalCurrentDirectory);
		}
		delete [] currentDirectory;
		delete [] originalCurrentDirectory;
		originalCurrentDirectory = NULL;
	}
}
