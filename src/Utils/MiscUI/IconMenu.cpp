// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2011 - TortoiseGit
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
#include "StdAfx.h"
#include "IconMenu.h"
#include "SysInfo.h"

CIconMenu::CIconMenu(void) : CMenu()
	, pfnBeginBufferedPaint(NULL)
	, pfnEndBufferedPaint(NULL)
	, pfnGetBufferedPaintBits(NULL)
{
	if (SysInfo::Instance().IsVistaOrLater())
	{
		HMODULE hUxTheme = ::GetModuleHandle (_T("UXTHEME.DLL"));

		pfnGetBufferedPaintBits = (FN_GetBufferedPaintBits)::GetProcAddress(hUxTheme, "GetBufferedPaintBits");
		pfnBeginBufferedPaint = (FN_BeginBufferedPaint)::GetProcAddress(hUxTheme, "BeginBufferedPaint");
		pfnEndBufferedPaint = (FN_EndBufferedPaint)::GetProcAddress(hUxTheme, "EndBufferedPaint");
	}
}

CIconMenu::~CIconMenu(void)
{
	std::map<UINT, HBITMAP>::iterator it;
	for (it = bitmaps.begin(); it != bitmaps.end(); ++it)
	{
		::DeleteObject(it->second);
	}
	bitmaps.clear();
}

BOOL CIconMenu::AppendMenuIcon(UINT_PTR nIDNewItem, LPCTSTR lpszNewItem, UINT uIcon /* = 0 */,  HMENU hsubmenu)
{
	TCHAR menutextbuffer[255] = {0};
	_tcscpy_s(menutextbuffer, 255, lpszNewItem);

	if (uIcon == 0)
		return CMenu::AppendMenu(MF_STRING | MF_ENABLED, nIDNewItem, menutextbuffer);

	MENUITEMINFO info = {0};
	info.cbSize = sizeof(info);
	info.fMask = MIIM_STRING | MIIM_FTYPE | MIIM_ID;
	info.fType = MFT_STRING;
	
	if(hsubmenu)
	{
		info.fMask |= MIIM_SUBMENU;
		info.hSubMenu = hsubmenu;
	}

	
	info.wID = nIDNewItem;
	info.dwTypeData = menutextbuffer;
	if (SysInfo::Instance().IsVistaOrLater())
	{
		info.fMask |= MIIM_BITMAP;
		info.hbmpItem = IconToBitmapPARGB32(uIcon);
	}
	else
	{
		info.fMask |= MIIM_BITMAP;
		info.hbmpItem = HBMMENU_CALLBACK;
	}
	icons[nIDNewItem] = uIcon;
	return InsertMenuItem(nIDNewItem, &info);
}

BOOL CIconMenu::AppendMenuIcon(UINT_PTR nIDNewItem, UINT_PTR nNewItem, UINT uIcon /* = 0 */,  HMENU hsubmenu)
{
	CString temp;
	temp.LoadString(nNewItem);

	return AppendMenuIcon(nIDNewItem, temp, uIcon, hsubmenu);
}

void CIconMenu::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	if ((lpDrawItemStruct==NULL)||(lpDrawItemStruct->CtlType != ODT_MENU))
		return;		//not for a menu
	HICON hIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(icons[lpDrawItemStruct->itemID]), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	if (hIcon == NULL)
		return;
	DrawIconEx(lpDrawItemStruct->hDC,
		lpDrawItemStruct->rcItem.left - 16,
		lpDrawItemStruct->rcItem.top + (lpDrawItemStruct->rcItem.bottom - lpDrawItemStruct->rcItem.top - 16) / 2,
		hIcon, 16, 16,
		0, NULL, DI_NORMAL);
	DestroyIcon(hIcon);
}

void CIconMenu::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
	if (lpMeasureItemStruct==NULL)
		return;
	lpMeasureItemStruct->itemWidth += 2;
	if (lpMeasureItemStruct->itemHeight < 16)
		lpMeasureItemStruct->itemHeight = 16;
}

HBITMAP CIconMenu::IconToBitmapPARGB32(UINT uIcon)
{
	std::map<UINT, HBITMAP>::iterator bitmap_it = bitmaps.lower_bound(uIcon);
	if (bitmap_it != bitmaps.end() && bitmap_it->first == uIcon)
		return bitmap_it->second;

	HICON hIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(uIcon), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	if (!hIcon)
		return NULL;

	if (pfnBeginBufferedPaint == NULL || pfnEndBufferedPaint == NULL || pfnGetBufferedPaintBits == NULL)
		return NULL;

	SIZE sizIcon;
	sizIcon.cx = GetSystemMetrics(SM_CXSMICON);
	sizIcon.cy = GetSystemMetrics(SM_CYSMICON);

	RECT rcIcon;
	SetRect(&rcIcon, 0, 0, sizIcon.cx, sizIcon.cy);
	HBITMAP hBmp = NULL;

	HDC hdcDest = CreateCompatibleDC(NULL);
	if (hdcDest)
	{
		if (SUCCEEDED(Create32BitHBITMAP(hdcDest, &sizIcon, NULL, &hBmp)))
		{
			HBITMAP hbmpOld = (HBITMAP)SelectObject(hdcDest, hBmp);
			if (hbmpOld)
			{
				BLENDFUNCTION bfAlpha = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
				BP_PAINTPARAMS paintParams = {0};
				paintParams.cbSize = sizeof(paintParams);
				paintParams.dwFlags = BPPF_ERASE;
				paintParams.pBlendFunction = &bfAlpha;

				HDC hdcBuffer;
				HPAINTBUFFER hPaintBuffer = pfnBeginBufferedPaint(hdcDest, &rcIcon, BPBF_DIB, &paintParams, &hdcBuffer);
				if (hPaintBuffer)
				{
					if (DrawIconEx(hdcBuffer, 0, 0, hIcon, sizIcon.cx, sizIcon.cy, 0, NULL, DI_NORMAL))
					{
						// If icon did not have an alpha channel we need to convert buffer to PARGB
						ConvertBufferToPARGB32(hPaintBuffer, hdcDest, hIcon, sizIcon);
					}

					// This will write the buffer contents to the destination bitmap
					pfnEndBufferedPaint(hPaintBuffer, TRUE);
				}

				SelectObject(hdcDest, hbmpOld);
			}
		}

		DeleteDC(hdcDest);
	}

	DestroyIcon(hIcon);

	if(hBmp)
		bitmaps.insert(bitmap_it, std::make_pair(uIcon, hBmp));
	return hBmp;
}

HRESULT CIconMenu::Create32BitHBITMAP(HDC hdc, const SIZE *psize, __deref_opt_out void **ppvBits, __out HBITMAP* phBmp)
{
	*phBmp = NULL;

	BITMAPINFO bmi;
	SecureZeroMemory(&bmi, sizeof(bmi));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biCompression = BI_RGB;

	bmi.bmiHeader.biWidth = psize->cx;
	bmi.bmiHeader.biHeight = psize->cy;
	bmi.bmiHeader.biBitCount = 32;

	HDC hdcUsed = hdc ? hdc : GetDC(NULL);
	if (hdcUsed)
	{
		*phBmp = CreateDIBSection(hdcUsed, &bmi, DIB_RGB_COLORS, ppvBits, NULL, 0);
		if (hdc != hdcUsed)
		{
			ReleaseDC(NULL, hdcUsed);
		}
	}
	return (NULL == *phBmp) ? E_OUTOFMEMORY : S_OK;
}

HRESULT CIconMenu::ConvertBufferToPARGB32(HPAINTBUFFER hPaintBuffer, HDC hdc, HICON hicon, SIZE& sizIcon)
{
	RGBQUAD *prgbQuad;
	int cxRow;
	HRESULT hr = pfnGetBufferedPaintBits(hPaintBuffer, &prgbQuad, &cxRow);
	if (SUCCEEDED(hr))
	{
		Gdiplus::ARGB *pargb = reinterpret_cast<Gdiplus::ARGB *>(prgbQuad);
		if (!HasAlpha(pargb, sizIcon, cxRow))
		{
			ICONINFO info;
			if (GetIconInfo(hicon, &info))
			{
				if (info.hbmMask)
				{
					hr = ConvertToPARGB32(hdc, pargb, info.hbmMask, sizIcon, cxRow);
				}

				DeleteObject(info.hbmColor);
				DeleteObject(info.hbmMask);
			}
		}
	}

	return hr;
}

bool CIconMenu::HasAlpha(__in Gdiplus::ARGB *pargb, SIZE& sizImage, int cxRow)
{
	ULONG cxDelta = cxRow - sizImage.cx;
	for (ULONG y = sizImage.cy; y; --y)
	{
		for (ULONG x = sizImage.cx; x; --x)
		{
			if (*pargb++ & 0xFF000000)
			{
				return true;
			}
		}

		pargb += cxDelta;
	}

	return false;
}

HRESULT CIconMenu::ConvertToPARGB32(HDC hdc, __inout Gdiplus::ARGB *pargb, HBITMAP hbmp, SIZE& sizImage, int cxRow)
{
	BITMAPINFO bmi;
	SecureZeroMemory(&bmi, sizeof(bmi));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biCompression = BI_RGB;

	bmi.bmiHeader.biWidth = sizImage.cx;
	bmi.bmiHeader.biHeight = sizImage.cy;
	bmi.bmiHeader.biBitCount = 32;

	HRESULT hr = E_OUTOFMEMORY;
	HANDLE hHeap = GetProcessHeap();
	void *pvBits = HeapAlloc(hHeap, 0, bmi.bmiHeader.biWidth * 4 * bmi.bmiHeader.biHeight);
	if (pvBits)
	{
		hr = E_UNEXPECTED;
		if (GetDIBits(hdc, hbmp, 0, bmi.bmiHeader.biHeight, pvBits, &bmi, DIB_RGB_COLORS) == bmi.bmiHeader.biHeight)
		{
			ULONG cxDelta = cxRow - bmi.bmiHeader.biWidth;
            Gdiplus::ARGB *pargbMask = static_cast<Gdiplus::ARGB *>(pvBits);

			for (ULONG y = bmi.bmiHeader.biHeight; y; --y)
			{
				for (ULONG x = bmi.bmiHeader.biWidth; x; --x)
				{
					if (*pargbMask++)
					{
						// transparent pixel
						*pargb++ = 0;
					}
					else
					{
						// opaque pixel
						*pargb++ |= 0xFF000000;
					}
				}

				pargb += cxDelta;
			}

			hr = S_OK;
		}

		HeapFree(hHeap, 0, pvBits);
	}

	return hr;
}

BOOL CIconMenu::SetMenuItemData(UINT_PTR nIDNewItem, LONG_PTR data)
{

	MENUITEMINFO menuinfo ={0};
	menuinfo.cbSize = sizeof(menuinfo);
	GetMenuItemInfo(nIDNewItem,&menuinfo);
	menuinfo.dwItemData =data;
	menuinfo.fMask |= MIIM_DATA;
	return SetMenuItemInfo(nIDNewItem ,&menuinfo);

}

LONG_PTR CIconMenu::GetMenuItemData(UINT_PTR nIDNewItem)
{
	MENUITEMINFO menuinfo ={0};
	menuinfo.fMask |= MIIM_DATA;
	menuinfo.cbSize = sizeof(menuinfo);
	GetMenuItemInfo(nIDNewItem,&menuinfo);

	return menuinfo.dwItemData;
}
