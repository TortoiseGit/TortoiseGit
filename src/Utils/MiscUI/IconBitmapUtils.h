// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2023, 2025 - TortoiseGit
// Copyright (C) 2009, 2011, 2014-2015, 2021, 2025 - TortoiseSVN

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
#pragma warning(push)
#pragma warning(disable : 4458) // declaration of 'xxx' hides class member
#include <GdiPlus.h>
#pragma warning(pop)

/**
 * \ingroup utils
 * provides helper functions for converting icons to bitmaps
 */
class IconBitmapUtils
{
public:
	IconBitmapUtils() = delete;

	static HBITMAP IconToBitmap(HINSTANCE hInst, UINT uIcon);
	static HBITMAP IconToBitmapPARGB32(HICON hIcon, int width, int height);
	static HBITMAP IconToBitmapPARGB32(HINSTANCE hInst, UINT uIcon);
	static HRESULT Create32BitHBITMAP(HDC hdc, const SIZE* psize, __deref_opt_out void** ppvBits, __out HBITMAP* phBmp);
	static HRESULT ConvertBufferToPARGB32(HPAINTBUFFER hPaintBuffer, HDC hdc, HICON hIcon, const SIZE& sizIcon);
	static bool HasAlpha(__in Gdiplus::ARGB* pargb, const SIZE& sizImage, int cxRow);
	static HRESULT ConvertToPARGB32(HDC hdc, __inout Gdiplus::ARGB* pargb, HBITMAP hbmp, const SIZE& sizImage, int cxRow);
};
