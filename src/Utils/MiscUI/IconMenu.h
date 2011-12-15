// TortoiseGit - a Windows shell extension for easy version control

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
#pragma once
#include <Uxtheme.h>

typedef HRESULT (WINAPI *FN_GetBufferedPaintBits) (HPAINTBUFFER hBufferedPaint, RGBQUAD **ppbBuffer, int *pcxRow);
typedef HPAINTBUFFER (WINAPI *FN_BeginBufferedPaint) (HDC hdcTarget, const RECT *prcTarget, BP_BUFFERFORMAT dwFormat, BP_PAINTPARAMS *pPaintParams, HDC *phdc);
typedef HRESULT (WINAPI *FN_EndBufferedPaint) (HPAINTBUFFER hBufferedPaint, BOOL fUpdateTarget);


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

	BOOL AppendMenuIcon(UINT_PTR nIDNewItem, LPCTSTR lpszNewItem, UINT uIcon = 0, HMENU hsubmenu=0);
	BOOL AppendMenuIcon(UINT_PTR nIDNewItem, UINT_PTR nNewItem, UINT uIcon = 0,HMENU hsubmenu=0);
	void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);

	BOOL SetMenuItemData(UINT_PTR nIDNewItem, LONG_PTR data);
	LONG_PTR GetMenuItemData(UINT_PTR nIDNewItem);

private:
	HBITMAP IconToBitmapPARGB32(UINT uIcon);
	HRESULT Create32BitHBITMAP(HDC hdc, const SIZE *psize, __deref_opt_out void **ppvBits, __out HBITMAP* phBmp);
	HRESULT ConvertBufferToPARGB32(HPAINTBUFFER hPaintBuffer, HDC hdc, HICON hicon, SIZE& sizIcon);
	bool HasAlpha(__in Gdiplus::ARGB *pargb, SIZE& sizImage, int cxRow);
	HRESULT ConvertToPARGB32(HDC hdc, __inout Gdiplus::ARGB *pargb, HBITMAP hbmp, SIZE& sizImage, int cxRow);

private:
	std::map<UINT, HBITMAP>		bitmaps;
	std::map<UINT_PTR, UINT>	icons;

	FN_GetBufferedPaintBits pfnGetBufferedPaintBits;
	FN_BeginBufferedPaint pfnBeginBufferedPaint;
	FN_EndBufferedPaint pfnEndBufferedPaint;
};
