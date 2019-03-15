// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2018-2019 - TortoiseGit
// Copyright (C) 2010-2012, 2014-2015 - TortoiseSVN

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
#include "IconExtractor.h"
#include "SmartHandle.h"

#define WIDTHBYTES(bits) ((((bits) + 31) >> 5) << 2)

CIconExtractor::CIconExtractor()
{
}

DWORD CIconExtractor::ExtractIcon(HINSTANCE hResource, LPCTSTR id, LPCTSTR TargetICON)
{
	// Find the group icon resource
	HRSRC hRsrc = FindResource(hResource, id, RT_GROUP_ICON);

	if (!hRsrc)
		return GetLastError();

	HGLOBAL hGlobal = nullptr;
	if ((hGlobal = LoadResource(hResource, hRsrc)) == nullptr)
		return GetLastError();

	LPMEMICONDIR lpIcon = nullptr;
	if ((lpIcon = static_cast<LPMEMICONDIR>(LockResource(hGlobal))) == nullptr)
		return GetLastError();

	LPICONRESOURCE lpIR = nullptr;
	if ((lpIR = static_cast<LPICONRESOURCE>(malloc(sizeof(ICONRESOURCE) + ((lpIcon->idCount - 1) * sizeof(ICONIMAGE))))) == nullptr)
		return GetLastError();
	SecureZeroMemory(lpIR, sizeof(ICONRESOURCE) + ((lpIcon->idCount - 1) * sizeof(ICONIMAGE)));

	lpIR->nNumImages = lpIcon->idCount;
	SCOPE_EXIT
	{
		for (UINT i = 0; i < lpIR->nNumImages; ++i)
			free(lpIR->IconImages[i].lpBits);
		free(lpIR);
	};

	// Go through all the icons
	for (UINT i = 0; i < lpIR->nNumImages; ++i)
	{
		// Get the individual icon
		if ((hRsrc = FindResource(hResource, MAKEINTRESOURCE(lpIcon->idEntries[i].nID), RT_ICON)) == nullptr)
			return GetLastError();
		if ((hGlobal = LoadResource(hResource, hRsrc)) == nullptr)
			return GetLastError();

		// Store a copy of the resource locally
		lpIR->IconImages[i].dwNumBytes = SizeofResource(hResource, hRsrc);
		lpIR->IconImages[i].lpBits = static_cast<LPBYTE>(malloc(lpIR->IconImages[i].dwNumBytes));
		if (!lpIR->IconImages[i].lpBits)
			return GetLastError();

		memcpy(lpIR->IconImages[i].lpBits, LockResource(hGlobal), lpIR->IconImages[i].dwNumBytes);

		// Adjust internal pointers
		if (!AdjustIconImagePointers(&(lpIR->IconImages[i])))
			return GetLastError();
	}

	return WriteIconToICOFile(lpIR, TargetICON);
}

DWORD CIconExtractor::WriteIconToICOFile(LPICONRESOURCE lpIR, LPCTSTR szFileName)
{
	CAutoFile hFile = ::CreateFile(szFileName, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	// open the file
	if (!hFile)
		return GetLastError();

	// Write the header
	if (WriteICOHeader(hFile, lpIR->nNumImages))
		return GetLastError();

	// Write the ICONDIRENTRY's
	for (UINT i = 0; i < lpIR->nNumImages; ++i)
	{
		ICONDIRENTRY ide = { 0 };

		// Convert internal format to ICONDIRENTRY
		ide.bWidth = static_cast<BYTE>(lpIR->IconImages[i].Width);
		ide.bHeight = static_cast<BYTE>(lpIR->IconImages[i].Height);
		if (ide.bHeight == 0) // 256x256 icon, both width and height must be 0
			ide.bWidth = 0;
		ide.bReserved = 0;
		ide.wPlanes = lpIR->IconImages[i].lpbi->bmiHeader.biPlanes;
		ide.wBitCount = lpIR->IconImages[i].lpbi->bmiHeader.biBitCount;

		if ((ide.wPlanes * ide.wBitCount) >= 8)
			ide.bColorCount = 0;
		else
			ide.bColorCount = 1 << (ide.wPlanes * ide.wBitCount);
		ide.dwBytesInRes = lpIR->IconImages[i].dwNumBytes;
		ide.dwImageOffset = CalculateImageOffset(lpIR, i);

		// Write the ICONDIRENTRY to disk
		DWORD dwBytesWritten = 0;
		if (!WriteFile(hFile, &ide, sizeof(ICONDIRENTRY), &dwBytesWritten, nullptr))
			return GetLastError();

		if (dwBytesWritten != sizeof(ICONDIRENTRY))
			return GetLastError();
	}

	// Write the image bits for each image
	for (UINT i = 0; i < lpIR->nNumImages; ++i)
	{
		DWORD dwTemp = lpIR->IconImages[i].lpbi->bmiHeader.biSizeImage;
		bool bError = false; // fix size even on error

		// Set the sizeimage member to zero, but not if the icon is PNG
		if (lpIR->IconImages[i].lpbi->bmiHeader.biCompression != 65536)
			lpIR->IconImages[i].lpbi->bmiHeader.biSizeImage = 0;
		DWORD dwBytesWritten = 0;
		if (!WriteFile(hFile, lpIR->IconImages[i].lpBits, lpIR->IconImages[i].dwNumBytes, &dwBytesWritten, nullptr))
			bError = true;

		if (dwBytesWritten != lpIR->IconImages[i].dwNumBytes)
			bError = true;

		// set it back
		lpIR->IconImages[i].lpbi->bmiHeader.biSizeImage = dwTemp;
		if (bError)
			return GetLastError();
	}
	return NO_ERROR;
}

DWORD CIconExtractor::CalculateImageOffset(LPICONRESOURCE lpIR, UINT nIndex) const
{
	// Calculate the ICO header size
	DWORD dwSize = 3 * sizeof(WORD);
	// Add the ICONDIRENTRY's
	dwSize += lpIR->nNumImages * sizeof(ICONDIRENTRY);
	// Add the sizes of the previous images
	for (UINT i = 0; i < nIndex; ++i)
		dwSize += lpIR->IconImages[i].dwNumBytes;

	return dwSize;
}

DWORD CIconExtractor::WriteICOHeader(HANDLE hFile, UINT nNumEntries) const
{
	WORD Output = 0;
	DWORD dwBytesWritten = 0;

	// Write 'reserved' WORD
	if (!WriteFile(hFile, &Output, sizeof(WORD), &dwBytesWritten, nullptr))
		return GetLastError();
	// Did we write a WORD?
	if (dwBytesWritten != sizeof(WORD))
		return GetLastError();
	// Write 'type' WORD (1)
	Output = 1;
	if (!WriteFile(hFile, &Output, sizeof(WORD), &dwBytesWritten, nullptr))
		return GetLastError();
	// Did we write a WORD?
	if (dwBytesWritten != sizeof(WORD))
		return GetLastError();
	// Write Number of Entries
	Output = static_cast<WORD>(nNumEntries);
	if (!WriteFile(hFile, &Output, sizeof(WORD), &dwBytesWritten, nullptr))
		return GetLastError();
	// Did we write a WORD?
	if (dwBytesWritten != sizeof(WORD))
		return GetLastError();

	return NO_ERROR;
}

BOOL CIconExtractor::AdjustIconImagePointers(LPICONIMAGE lpImage)
{
	if (!lpImage)
		return FALSE;

	// BITMAPINFO is at beginning of bits
	lpImage->lpbi = reinterpret_cast<LPBITMAPINFO>(lpImage->lpBits);
	// Width - simple enough
	lpImage->Width = lpImage->lpbi->bmiHeader.biWidth;
	// Icons are stored in funky format where height is doubled - account for it
	lpImage->Height = (lpImage->lpbi->bmiHeader.biHeight) / 2;
	// How many colors?
	lpImage->Colors = lpImage->lpbi->bmiHeader.biPlanes * lpImage->lpbi->bmiHeader.biBitCount;
	// XOR bits follow the header and color table
	lpImage->lpXOR = reinterpret_cast<PBYTE>(FindDIBBits(reinterpret_cast<LPSTR>(lpImage->lpbi)));
	// AND bits follow the XOR bits
	lpImage->lpAND = lpImage->lpXOR + (lpImage->Height * BytesPerLine(reinterpret_cast<LPBITMAPINFOHEADER>(lpImage->lpbi)));

	return TRUE;
}

LPSTR CIconExtractor::FindDIBBits(LPSTR lpbi)
{
	return (lpbi + *reinterpret_cast<LPDWORD>(lpbi) + PaletteSize(lpbi));
}

WORD CIconExtractor::PaletteSize(LPSTR lpbi)
{
	return (DIBNumColors(lpbi) * sizeof(RGBQUAD));
}

DWORD CIconExtractor::BytesPerLine(LPBITMAPINFOHEADER lpBMIH) const
{
	return WIDTHBYTES(lpBMIH->biWidth * lpBMIH->biPlanes * lpBMIH->biBitCount);
}

WORD CIconExtractor::DIBNumColors(LPSTR lpbi) const
{
	DWORD dwClrUsed = reinterpret_cast<LPBITMAPINFOHEADER>(lpbi)->biClrUsed;

	if (dwClrUsed)
		return static_cast<WORD>(dwClrUsed);

	WORD wBitCount = reinterpret_cast<LPBITMAPINFOHEADER>(lpbi)->biBitCount;

	switch (wBitCount)
	{
	case 1:
		return 2;
	case 4:
		return 16;
	case 8:
		return 256;
	default:
		return 0;
	}
	//return 0;
}
