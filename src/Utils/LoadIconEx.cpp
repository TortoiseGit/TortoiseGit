// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2018-2019 - TortoiseGit
// Copyright (C) 2018 - TortoiseSVN

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
#include "LoadIconEx.h"
#include <CommCtrl.h>

#pragma comment(lib, "Comctl32.lib")
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

HICON LoadIconEx(HINSTANCE hInstance, LPCWSTR lpIconName)
{
	HICON hIcon = nullptr;
	if (FAILED(LoadIconWithScaleDown(hInstance, lpIconName, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), &hIcon)))
	{
		// fallback, just in case
		hIcon = LoadIcon(hInstance, lpIconName);
	}
	return hIcon;
}

HICON LoadIconEx(HINSTANCE hInstance, LPCWSTR lpIconName, int iconWidth, int iconHeight)
{
	// the docs for LoadIconWithScaleDown don't mention that a size of 0 will
	// use the default system icon size like for e.g. LoadImage or LoadIcon.
	// So we don't assume that this works but do it ourselves.
	if (iconWidth == 0)
		iconWidth = GetSystemMetrics(SM_CXSMICON);
	if (iconHeight == 0)
		iconHeight = GetSystemMetrics(SM_CYSMICON);
	HICON hIcon = nullptr;
	if (FAILED(LoadIconWithScaleDown(hInstance, lpIconName, iconWidth, iconHeight, &hIcon)))
	{
		// fallback, just in case
		hIcon = static_cast<HICON>(LoadImage(hInstance, lpIconName, IMAGE_ICON, iconWidth, iconHeight, LR_DEFAULTCOLOR));
	}
	return hIcon;
}
