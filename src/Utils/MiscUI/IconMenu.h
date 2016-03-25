// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2010, 2013 - TortoiseGit
// Copyright (C) 2008-2009, 2011 - TortoiseSVN

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
#include "IconBitmapUtils.h"


/**
 * \ingroup utils
 * Extends the CMenu class with a convenience method to insert a menu
 * entry with an icon.
 *
 * The icons are loaded from the resources and converted to a bitmap.
 * The bitmaps are destroyed together with the CIconMenu object.
 */
class CIconMenu : public CMenu
{
public:
	CIconMenu(void);
	~CIconMenu(void);

	BOOL CreateMenu();
	BOOL CreatePopupMenu();
	BOOL AppendMenuIcon(UINT_PTR nIDNewItem, LPCTSTR lpszNewItem, UINT uIcon = 0, HMENU hsubmenu = nullptr);
	BOOL AppendMenuIcon(UINT_PTR nIDNewItem, UINT_PTR nNewItem, UINT uIcon = 0, HMENU hsubmenu = nullptr);
	void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);

	BOOL SetMenuItemData(UINT_PTR nIDNewItem, LONG_PTR data);
	LONG_PTR GetMenuItemData(UINT_PTR nIDNewItem);

private:
	BOOL SetMenuStyle(void);

private:
	IconBitmapUtils				bitmapUtils;
	std::map<UINT_PTR, UINT>	icons;
	std::map<UINT_PTR, HICON>	iconhandles;
	bool						bShowIcons;
};
