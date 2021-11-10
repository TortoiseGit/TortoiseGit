// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2021 - TortoiseGit
// Copyright (C) 2008 - TortoiseSVN

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
#include "SysInfo.h"
#include "PathUtils.h"
#include "StringUtils.h"

static bool InitializeIsWin11OrLater()
{
	PWSTR pszPath = nullptr;
	if (SHGetKnownFolderPath(FOLDERID_System, KF_FLAG_CREATE, nullptr, &pszPath) != S_OK)
		return false;
	CString sysPath{ pszPath };
	CoTaskMemFree(pszPath);
	auto explorerVersion = CPathUtils::GetVersionFromFile(sysPath + L"\\shell32.dll");
	std::vector<int> versions;
	stringtok(versions, explorerVersion, true, L".");
	return versions.size() > 3 && versions[2] >= 22000;
}

SysInfo::SysInfo()
{
	m_bIsWindows11OrLater = InitializeIsWin11OrLater();
}

SysInfo::~SysInfo()
{
}

const SysInfo& SysInfo::Instance()
{
	static SysInfo instance;
	return instance;
}
