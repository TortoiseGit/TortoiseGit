// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2010, 2014 - TortoiseSVN

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

// The following structures are taken from iconpro sdk example
#pragma pack(push)
#pragma pack(2)
typedef struct
{
	BYTE	bWidth;			// Width of the image
	BYTE	bHeight;		// Height of the image (times 2)
	BYTE	bColorCount;	// Number of colors in image (0 if >=8bpp)
	BYTE	bReserved;		// Reserved
	WORD	wPlanes;		// Color Planes
	WORD	wBitCount;		// Bits per pixel
	DWORD	dwBytesInRes;	// how many bytes in this resource?
	WORD	nID;			// the ID
} MEMICONDIRENTRY, *LPMEMICONDIRENTRY;
typedef struct
{
	WORD			idReserved;		// Reserved
	WORD			idType;			// resource type (1 for icons)
	WORD			idCount;		// how many images?
	MEMICONDIRENTRY	idEntries[1];	// the entries for each image
} MEMICONDIR, *LPMEMICONDIR;
#pragma pack(pop)

typedef struct
{
	UINT			Width, Height, Colors;	// Width, Height and bpp
	LPBYTE			lpBits;					// ptr to DIB bits
	DWORD			dwNumBytes;				// how many bytes?
	LPBITMAPINFO	lpbi;					// ptr to header
	LPBYTE			lpXOR;					// ptr to XOR image bits
	LPBYTE			lpAND;					// ptr to AND image bits
} ICONIMAGE, *LPICONIMAGE;
typedef struct
{
	BOOL		bHasChanged;		// Has image changed?
	UINT		nNumImages;			// How many images?
	ICONIMAGE	IconImages[1];		// Image entries
} ICONRESOURCE, *LPICONRESOURCE;


// These next two structs represent how the icon information is stored
// in an ICO file.
typedef struct
{
	BYTE	bWidth;				// Width of the image
	BYTE	bHeight;			// Height of the image (times 2)
	BYTE	bColorCount;		// Number of colors in image (0 if >=8bpp)
	BYTE	bReserved;			// Reserved
	WORD	wPlanes;			// Color Planes
	WORD	wBitCount;			// Bits per pixel
	DWORD	dwBytesInRes;		// how many bytes in this resource?
	DWORD	dwImageOffset;		// where in the file is this image
} ICONDIRENTRY, *LPICONDIRENTRY;
typedef struct
{
	WORD			idReserved;		// Reserved
	WORD			idType;			// resource type (1 for icons)
	WORD			idCount;		// how many images?
	ICONDIRENTRY	idEntries[1];	// the entries for each image
} ICONDIR, *LPICONDIR;


class CIconExtractor
{
public:
	CIconExtractor();

    DWORD ExtractIcon(HINSTANCE hResource, LPCTSTR id, LPCTSTR TargetICON);

private:
	DWORD WriteIconToICOFile(LPICONRESOURCE lpIR, LPCTSTR szFileName);
	BOOL AdjustIconImagePointers(LPICONIMAGE lpImage);

	LPSTR FindDIBBits(LPSTR lpbi);
	WORD DIBNumColors(LPSTR lpbi) const;
	WORD PaletteSize(LPSTR lpbi);
	DWORD BytesPerLine(LPBITMAPINFOHEADER lpBMIH) const;
	DWORD WriteICOHeader(HANDLE hFile, UINT nNumEntries) const;
	DWORD CalculateImageOffset(LPICONRESOURCE lpIR, UINT nIndex) const;
};
