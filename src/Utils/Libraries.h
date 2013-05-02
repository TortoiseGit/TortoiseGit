// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2010-2011 - TortoiseSVN

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

// helper:
// declares and defines stuff which is not available in the Vista SDK or
// which isn't available in the Win7 SDK but not unless NTDDI_VERSION is
// set to NTDDI_WIN7
#pragma once

void	EnsureGitLibrary(bool bCreate = true);
HRESULT	GetShellLibraryItem(LPWSTR pwszLibraryName, IShellItem2** ppShellItem);
HRESULT	OpenShellLibrary(LPWSTR pwszLibraryName, IShellLibrary** ppShellLib);

EXTERN_C const CLSID FOLDERTYPEID_GITWC;


