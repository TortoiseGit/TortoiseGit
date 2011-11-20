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
#include <mlang.h>


// these functions are taken from an article of the 'old new thing' blog:
// http://blogs.msdn.com/oldnewthing/archive/2004/07/16/185261.aspx


HRESULT TextOutFL(HDC hdc, int x, int y, LPCWSTR psz, int cch)
{
	HRESULT hr;
	IMLangFontLink2 *pfl;
	if (SUCCEEDED(hr = CoCreateInstance(CLSID_CMultiLanguage, NULL,
		CLSCTX_ALL, IID_IMLangFontLink2, (void**)&pfl))) 
	{
			HFONT hfOrig = (HFONT)GetCurrentObject(hdc, OBJ_FONT);
			POINT ptOrig;
			DWORD dwAlignOrig = GetTextAlign(hdc);
			if (!(dwAlignOrig & TA_UPDATECP)) 
			{
				SetTextAlign(hdc, dwAlignOrig | TA_UPDATECP);
			}
			MoveToEx(hdc, x, y, &ptOrig);
			DWORD dwFontCodePages = 0;
			hr = pfl->GetFontCodePages(hdc, hfOrig, &dwFontCodePages);
			if (SUCCEEDED(hr)) 
			{
				while (cch > 0) 
				{
					DWORD dwActualCodePages;
					long cchActual;
					hr = pfl->GetStrCodePages(psz, cch, dwFontCodePages, &dwActualCodePages, &cchActual);
					if (FAILED(hr)) 
					{
						break;
					}

					if (dwActualCodePages & dwFontCodePages) 
					{
						TextOut(hdc, 0, 0, psz, cchActual);
					} 
					else 
					{
						HFONT hfLinked;
						if (FAILED(hr = pfl->MapFont(hdc, dwActualCodePages, 0, &hfLinked))) 
						{
							break;
						}
						SelectObject(hdc, (HGDIOBJ)(HFONT)hfLinked);
						TextOut(hdc, 0, 0, psz, cchActual);
						SelectObject(hdc, (HGDIOBJ)(HFONT)hfOrig);
						pfl->ReleaseFont(hfLinked);
					}
					psz += cchActual;
					cch -= cchActual;
				}
				if (FAILED(hr)) 
				{
					//  We started outputting characters so we have to finish.
					//  Do the rest without font linking since we have no choice.
					TextOut(hdc, 0, 0, psz, cch);
					hr = S_FALSE;
				}
			}

			pfl->Release();

			if (!(dwAlignOrig & TA_UPDATECP)) 
			{
				SetTextAlign(hdc, dwAlignOrig);
				MoveToEx(hdc, ptOrig.x, ptOrig.y, NULL);
			}
	}

	return hr;
}

void TextOutTryFL(HDC hdc, int x, int y, LPCWSTR psz, int cch)
{
	if (FAILED(TextOutFL(hdc, x, y, psz, cch))) 
	{
		TextOut(hdc, x, y, psz, cch);
	}
}

