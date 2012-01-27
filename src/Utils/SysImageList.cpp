// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2006, 2008-2010 - TortoiseSVN

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
#include "SysImageList.h"
#include "TGitPath.h"

// Singleton constructor and destructor (private)

CSysImageList * CSysImageList::instance = 0;

CSysImageList::CSysImageList()
{
	SHFILEINFO ssfi;
	TCHAR windir[MAX_PATH];
	GetWindowsDirectory(windir, _countof(windir));  // MAX_PATH ok.
	HIMAGELIST hSystemImageList =
		(HIMAGELIST)SHGetFileInfo(
			windir,
			0,
			&ssfi, sizeof ssfi,
			SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
	Attach(hSystemImageList);
}

CSysImageList::~CSysImageList()
{
	Detach();
}

// Singleton specific operations

CSysImageList& CSysImageList::GetInstance()
{
	if (instance == 0)
		instance = new CSysImageList;
	return *instance;
}

void CSysImageList::Cleanup()
{
	delete instance;
	instance = 0;
}

// Operations

int CSysImageList::AddIcon(const HICON hIcon)
{
	return this->Add(hIcon);
}

int CSysImageList::GetDirIconIndex() const
{
	return GetFileIcon(_T("Doesn't matter"), FILE_ATTRIBUTE_DIRECTORY, 0);
}

int CSysImageList::GetDirOpenIconIndex() const
{
	return GetFileIcon(_T("Doesn't matter"), FILE_ATTRIBUTE_DIRECTORY, SHGFI_OPENICON);
}

int CSysImageList::GetDefaultIconIndex() const
{
	return GetFileIcon(_T(""), FILE_ATTRIBUTE_NORMAL, 0);
}

int CSysImageList::GetFileIconIndex(const CString& file) const
{
	return GetFileIcon(file, FILE_ATTRIBUTE_NORMAL, 0);
}

int CSysImageList::GetPathIconIndex(const CTGitPath& filePath) const
{
	CString strExtension = filePath.GetFileExtension();
	strExtension.MakeUpper();
	IconIndexMap::iterator it = m_indexCache.lower_bound(strExtension);
	if (it == m_indexCache.end() || strExtension < it->first)
	{
		// We don't have this extension in the map
		int iconIndex = GetFileIconIndex(filePath.GetFilename());
		it = m_indexCache.insert(it, std::make_pair(strExtension, iconIndex));
	}
	// We must have found it
	return it->second;
}

int CSysImageList::GetFileIcon( LPCTSTR file, DWORD attributes, UINT extraFlags ) const
{
	SHFILEINFO sfi;
	SecureZeroMemory(&sfi, sizeof sfi);

	SHGetFileInfo(
		file,
		attributes,
		&sfi, sizeof sfi,
		SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES | extraFlags);

	return sfi.iIcon;
}
