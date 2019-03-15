// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2016, 2019 - TortoiseGit

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

#ifndef WIN64
#define REG_MSYSGIT_INSTALL L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Git_is1\\InstallLocation"
#define REG_MSYSGIT_INSTALL_LOCAL REG_MSYSGIT_INSTALL
#else
#define REG_MSYSGIT_INSTALL_LOCAL L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Git_is1\\InstallLocation"
#define REG_MSYSGIT_INSTALL	L"SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Git_is1\\InstallLocation"
#endif

static bool FindGitForWindows(CString& sMsysGitPath)
{
	CRegString msyslocalinstalldir = CRegString(REG_MSYSGIT_INSTALL_LOCAL, L"", FALSE, HKEY_CURRENT_USER);
	sMsysGitPath = msyslocalinstalldir;
	sMsysGitPath.TrimRight(L'\\');
#ifdef _WIN64
	if (sMsysGitPath.IsEmpty())
	{
		CRegString msysinstalldir = CRegString(REG_MSYSGIT_INSTALL_LOCAL, L"", FALSE, HKEY_LOCAL_MACHINE);
		sMsysGitPath = msysinstalldir;
		sMsysGitPath.TrimRight(L'\\');
	}
#endif
	if (sMsysGitPath.IsEmpty())
	{
		CRegString msysinstalldir = CRegString(REG_MSYSGIT_INSTALL, L"", FALSE, HKEY_LOCAL_MACHINE);
		sMsysGitPath = msysinstalldir;
		sMsysGitPath.TrimRight(L'\\');
	}
	if (!sMsysGitPath.IsEmpty())
	{
		if (PathFileExists(sMsysGitPath + L"\\bin\\git.exe"))
			sMsysGitPath += L"\\bin";
		else if (PathFileExists(sMsysGitPath + L"\\cmd\\git.exe")) // only needed for older Git for Windows 2.x packages
			sMsysGitPath += L"\\cmd";
		else
		{
			CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Git for Windows installation found, but git.exe not exists in %s\n", static_cast<LPCTSTR>(sMsysGitPath));
			sMsysGitPath.Empty();
		}
	}
	CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Found no git.exe\n");
	return !sMsysGitPath.IsEmpty();
}
