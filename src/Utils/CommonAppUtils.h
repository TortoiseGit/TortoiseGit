// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2012 - TortoiseGit
// Copyright (C) 2003-2008,2010 - TortoiseSVN

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

/**
 * \ingroup TortoiseProc
 * An utility class with static functions.
 */
class CCommonAppUtils
{
public:

	/**
	* Launch an external application (usually the diff viewer)
	*/
	static bool LaunchApplication(const CString& sCommandLine, UINT idErrMessageFormat, bool bWaitForStartup, CString *cwd = NULL);

	static bool RunTortoiseProc(const CString& sCommandLine);

	static bool SetListCtrlBackgroundImage(HWND hListCtrl, UINT nID, int width = 128, int height = 128);

	static bool FileOpenSave(CString& path, int * filterindex, UINT title, UINT filter, bool bOpen, HWND hwndOwner = NULL);

protected:
	CCommonAppUtils(void){};
	~CCommonAppUtils(void){};
};
