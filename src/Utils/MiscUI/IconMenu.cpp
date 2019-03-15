// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2010-2011, 2013, 2016, 2019 - TortoiseGit
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
#include "stdafx.h"
#include "IconMenu.h"
#include "registry.h"
#include "LoadIconEx.h"

CIconMenu::CIconMenu(void) : CMenu()
{
	bShowIcons = !!DWORD(CRegDWORD(L"Software\\TortoiseGit\\ShowAppContextMenuIcons", TRUE));
}

CIconMenu::~CIconMenu(void)
{
	for (const auto& iconhandle : iconhandles)
		DestroyIcon(iconhandle.second);
}

BOOL CIconMenu::CreateMenu()
{
	if (__super::CreateMenu() == FALSE)
		return FALSE;

	SetMenuStyle();

	return TRUE;
}

BOOL CIconMenu::CreatePopupMenu()
{
	if (__super::CreatePopupMenu() == FALSE)
		return FALSE;

	SetMenuStyle();

	return TRUE;
}

BOOL CIconMenu::SetMenuStyle(void)
{
	MENUINFO MenuInfo = { 0 };

	MenuInfo.cbSize  = sizeof(MenuInfo);
	MenuInfo.fMask   = MIM_STYLE | MIM_APPLYTOSUBMENUS;
	MenuInfo.dwStyle = MNS_CHECKORBMP;

	SetMenuInfo(&MenuInfo);

	return TRUE;
}

BOOL CIconMenu::AppendMenuIcon(UINT_PTR nIDNewItem, LPCTSTR lpszNewItem, UINT uIcon /* = 0 */, HMENU hsubmenu /* = nullptr */)
{
	TCHAR menutextbuffer[255] = {0};
	wcscpy_s(menutextbuffer, lpszNewItem);

	MENUITEMINFO info = {0};
	info.cbSize = sizeof(info);
	info.fMask = MIIM_STRING | MIIM_FTYPE | MIIM_ID;
	info.fType = MFT_STRING;

	if (hsubmenu)
	{
		info.fMask |= MIIM_SUBMENU;
		info.hSubMenu = hsubmenu;
	}

	info.wID = static_cast<UINT>(nIDNewItem);
	info.dwTypeData = menutextbuffer;
	if ((uIcon != 0) && bShowIcons)
	{
		info.fMask |= MIIM_BITMAP;
		info.hbmpItem = bitmapUtils.IconToBitmapPARGB32(AfxGetResourceHandle(), uIcon);

		icons[nIDNewItem] = uIcon;
	}

	return InsertMenuItem(static_cast<UINT>(nIDNewItem), &info);
}

BOOL CIconMenu::AppendMenuIcon(UINT_PTR nIDNewItem, UINT_PTR nNewItem, UINT uIcon /* = 0 */, HMENU hsubmenu /* = nullptr */)
{
	CString temp;
	temp.LoadString(static_cast<UINT>(nNewItem));

	return AppendMenuIcon(nIDNewItem, temp, uIcon, hsubmenu);
}

void CIconMenu::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	if (!lpDrawItemStruct || lpDrawItemStruct->CtlType != ODT_MENU)
		return;		//not for a menu
	HICON hIcon = nullptr;
	bool bDestroyIcon = true;
	int iconWidth = GetSystemMetrics(SM_CXSMICON);
	int iconHeight = GetSystemMetrics(SM_CYSMICON);
	if (iconhandles.find(lpDrawItemStruct->itemID) != iconhandles.end())
	{
		hIcon = iconhandles[lpDrawItemStruct->itemID];
		bDestroyIcon = false;
	}
	else
		hIcon = LoadIconEx(AfxGetResourceHandle(), MAKEINTRESOURCE(icons[lpDrawItemStruct->itemID]), iconWidth, iconHeight);
	if (!hIcon)
		return;
	DrawIconEx(lpDrawItemStruct->hDC,
		lpDrawItemStruct->rcItem.left - iconWidth,
		lpDrawItemStruct->rcItem.top + (lpDrawItemStruct->rcItem.bottom - lpDrawItemStruct->rcItem.top - iconHeight) / 2,
		hIcon, iconWidth, iconHeight,
		0, nullptr, DI_NORMAL);
	if (bDestroyIcon)
		DestroyIcon(hIcon);
}

void CIconMenu::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
	if (!lpMeasureItemStruct)
		return;
	lpMeasureItemStruct->itemWidth += 2;
	if (lpMeasureItemStruct->itemHeight < 16)
		lpMeasureItemStruct->itemHeight = 16;
}

BOOL CIconMenu::SetMenuItemData(UINT_PTR nIDNewItem, LONG_PTR data)
{
	MENUITEMINFO menuinfo ={0};
	menuinfo.cbSize = sizeof(menuinfo);
	GetMenuItemInfo(static_cast<UINT>(nIDNewItem), &menuinfo);
	menuinfo.dwItemData =data;
	menuinfo.fMask |= MIIM_DATA;
	return SetMenuItemInfo(static_cast<UINT>(nIDNewItem), &menuinfo);
}

LONG_PTR CIconMenu::GetMenuItemData(UINT_PTR nIDNewItem)
{
	MENUITEMINFO menuinfo ={0};
	menuinfo.fMask |= MIIM_DATA;
	menuinfo.cbSize = sizeof(menuinfo);
	GetMenuItemInfo(static_cast<UINT>(nIDNewItem), &menuinfo);

	return menuinfo.dwItemData;
}
