// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2012 - TortoiseSVN

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

#include "stdafx.h"
#include <olectl.h>
#include "shlwapi.h"
#include <locale>
#include <algorithm>
#include "Picture.h"
#include "SmartHandle.h"

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "gdiplus.lib")

#define HIMETRIC_INCH 2540

CPicture::CPicture()
	: m_IPicture(NULL)
	, m_Height(0)
	, m_Weight(0)
	, m_Width(0)
	, pBitmap(NULL)
	, bHaveGDIPlus(false)
	, m_ip(InterpolationModeDefault)
	, hIcons(NULL)
	, lpIcons(NULL)
	, nCurrentIcon(0)
	, bIsIcon(false)
	, bIsTiff(false)
	, m_nSize(0)
	, m_ColorDepth(0)
	, hGlobal(NULL)
	, gdiplusToken(NULL)
{
}

CPicture::~CPicture()
{
	FreePictureData(); // Important - Avoid Leaks...
	delete pBitmap;
	if (bHaveGDIPlus)
		GdiplusShutdown(gdiplusToken);
}


void CPicture::FreePictureData()
{
	if (m_IPicture != NULL)
	{
		m_IPicture->Release();
		m_IPicture = NULL;
		m_Height = 0;
		m_Weight = 0;
		m_Width = 0;
		m_nSize = 0;
	}
	if (hIcons)
	{
		LPICONDIR lpIconDir = (LPICONDIR)lpIcons;
		if (lpIconDir)
		{
			for (int i=0; i<lpIconDir->idCount; ++i)
			{
				DestroyIcon(hIcons[i]);
			}
		}
		delete [] hIcons;
		hIcons = NULL;
	}
	delete [] lpIcons;
}

// Util function to ease loading of FreeImage library
static FARPROC s_GetProcAddressEx(HMODULE hDll, const char* procName, bool& valid)
{
	FARPROC proc = NULL;

	if (valid)
	{
		proc = GetProcAddress(hDll, procName);

		if (!proc)
			valid = false;
	}

	return proc;
}

tstring CPicture::GetFileSizeAsText(bool bAbbrev /* = true */)
{
	TCHAR buf[100] = {0};
	if (bAbbrev)
		StrFormatByteSize(m_nSize, buf, _countof(buf));
	else
		_stprintf_s(buf, _T("%ld Bytes"), m_nSize);

	return tstring(buf);
}

bool CPicture::Load(tstring sFilePathName)
{
	bool bResult = false;
	bIsIcon = false;
	lpIcons = NULL;
	//CFile PictureFile;
	//CFileException e;
	FreePictureData(); // Important - Avoid Leaks...

	// No-op if no file specified
	if (sFilePathName.empty())
		return true;

	// Load & initialize the GDI+ library if available
	HMODULE hGdiPlusLib = LoadLibrary(_T("gdiplus.dll"));
	if (hGdiPlusLib && GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL) == Ok)
	{
		bHaveGDIPlus = true;
	}
	// Since we loaded the gdiplus.dll only to check if it's available, we
	// can safely free the library here again - GdiplusStartup() loaded it too
	// and reference counting will make sure that it stays loaded until GdiplusShutdown()
	// is called.
	FreeLibrary(hGdiPlusLib);

	// Attempt to load using GDI+ if available
	if (bHaveGDIPlus)
	{
		pBitmap = new Bitmap(sFilePathName.c_str(), FALSE);
		GUID guid;
		pBitmap->GetRawFormat(&guid);

		if (pBitmap->GetLastStatus() != Ok)
		{
			delete pBitmap;
			pBitmap = NULL;
		}

		// gdiplus only loads the first icon found in an icon file
		// so we have to handle icon files ourselves :(

		// Even though gdiplus can load icons, it can't load the new
		// icons from Vista - in Vista, the icon format changed slightly.
		// But the LoadIcon/LoadImage API still can load those icons,
		// at least those dimensions which are also used on pre-Vista
		// systems.
		// For that reason, we don't rely on gdiplus telling us if
		// the image format is "icon" or not, we also check the
		// file extension for ".ico".
		std::transform(sFilePathName.begin(), sFilePathName.end(), sFilePathName.begin(), ::tolower);
		bIsIcon = (guid == ImageFormatIcon) || (_tcsstr(sFilePathName.c_str(), _T(".ico")) != NULL);
		bIsTiff = (guid == ImageFormatTIFF) || (_tcsstr(sFilePathName.c_str(), _T(".tiff")) != NULL);

		if (bIsIcon)
		{
			// Icon file, get special treatment...
			if (pBitmap)
			{
				// Cleanup first...
				delete (pBitmap);
				pBitmap = NULL;
				bIsIcon = true;
			}

			CAutoFile hFile = CreateFile(sFilePathName.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hFile)
			{
				BY_HANDLE_FILE_INFORMATION fileinfo;
				if (GetFileInformationByHandle(hFile, &fileinfo))
				{
					lpIcons = new BYTE[fileinfo.nFileSizeLow];
					DWORD readbytes;
					if (ReadFile(hFile, lpIcons, fileinfo.nFileSizeLow, &readbytes, NULL))
					{
						// we have the icon. Now gather the information we need later
						if (readbytes >= sizeof(ICONDIR))
						{
							// we are going to open same file second time so we have to close the file now
							hFile.CloseHandle();

							LPICONDIR lpIconDir = (LPICONDIR)lpIcons;
							if ((lpIconDir->idCount)&&(lpIconDir->idCount * sizeof(ICONDIR) <= fileinfo.nFileIndexLow))
							{
								try
								{
									nCurrentIcon = 0;
									hIcons = new HICON[lpIconDir->idCount];
									m_Width = lpIconDir->idEntries[0].bWidth;
									m_Height = lpIconDir->idEntries[0].bHeight;
									for (int i=0; i<lpIconDir->idCount; ++i)
									{
										hIcons[i] = (HICON)LoadImage(NULL, sFilePathName.c_str(), IMAGE_ICON,
											lpIconDir->idEntries[i].bWidth,
											lpIconDir->idEntries[i].bHeight,
											LR_LOADFROMFILE);
									}
									bResult = true;
								}
								catch (...)
								{
									delete [] lpIcons;
									lpIcons = NULL;
									bResult = false;
								}
							}
							else
							{
								delete [] lpIcons;
								lpIcons = NULL;
								bResult = false;
							}
						}
						else
						{
							delete [] lpIcons;
							lpIcons = NULL;
							bResult = false;
						}
					}
					else
					{
						delete [] lpIcons;
						lpIcons = NULL;
					}
				}
			}
		}
		else if (pBitmap)	// Image loaded successfully with GDI+
		{
			m_Height = pBitmap->GetHeight();
			m_Width = pBitmap->GetWidth();
			bResult = true;
		}

		// If still failed to load the file...
		if (!bResult)
		{
			// Attempt to load the FreeImage library as an optional DLL to support additional formats

			// NOTE: Currently just loading via FreeImage & using GDI+ for drawing.
			// It might be nice to remove this dependency in the future.
			HMODULE hFreeImageLib = LoadLibrary(_T("FreeImage.dll"));

			// FreeImage DLL functions
			typedef const char* (__stdcall *FreeImage_GetVersion_t)(void);
			typedef int			(__stdcall *FreeImage_GetFileType_t)(const TCHAR *filename, int size);
			typedef int			(__stdcall *FreeImage_GetFIFFromFilename_t)(const TCHAR *filename);
			typedef void*		(__stdcall *FreeImage_Load_t)(int format, const TCHAR *filename, int flags);
			typedef void		(__stdcall *FreeImage_Unload_t)(void* dib);
			typedef int			(__stdcall *FreeImage_GetColorType_t)(void* dib);
			typedef unsigned	(__stdcall *FreeImage_GetWidth_t)(void* dib);
			typedef unsigned	(__stdcall *FreeImage_GetHeight_t)(void* dib);
			typedef void		(__stdcall *FreeImage_ConvertToRawBits_t)(BYTE *bits, void *dib, int pitch, unsigned bpp, unsigned red_mask, unsigned green_mask, unsigned blue_mask, BOOL topdown);

			FreeImage_GetVersion_t FreeImage_GetVersion = NULL;
			FreeImage_GetFileType_t FreeImage_GetFileType = NULL;
			FreeImage_GetFIFFromFilename_t FreeImage_GetFIFFromFilename = NULL;
			FreeImage_Load_t FreeImage_Load = NULL;
			FreeImage_Unload_t FreeImage_Unload = NULL;
			FreeImage_GetColorType_t FreeImage_GetColorType = NULL;
			FreeImage_GetWidth_t FreeImage_GetWidth = NULL;
			FreeImage_GetHeight_t FreeImage_GetHeight = NULL;
			FreeImage_ConvertToRawBits_t  FreeImage_ConvertToRawBits = NULL;

			if (hFreeImageLib)
			{
				bool exportsValid = true;

				//FreeImage_GetVersion = (FreeImage_GetVersion_t)s_GetProcAddressEx(hFreeImageLib, "_FreeImage_GetVersion@0", valid);
				FreeImage_GetWidth = (FreeImage_GetWidth_t)s_GetProcAddressEx(hFreeImageLib, "_FreeImage_GetWidth@4", exportsValid);
				FreeImage_GetHeight = (FreeImage_GetHeight_t)s_GetProcAddressEx(hFreeImageLib, "_FreeImage_GetHeight@4", exportsValid);
				FreeImage_Unload = (FreeImage_Unload_t)s_GetProcAddressEx(hFreeImageLib, "_FreeImage_Unload@4", exportsValid);
				FreeImage_ConvertToRawBits = (FreeImage_ConvertToRawBits_t)s_GetProcAddressEx(hFreeImageLib, "_FreeImage_ConvertToRawBits@32", exportsValid);

#ifdef UNICODE
				FreeImage_GetFileType = (FreeImage_GetFileType_t)s_GetProcAddressEx(hFreeImageLib, "_FreeImage_GetFileTypeU@8", exportsValid);
				FreeImage_GetFIFFromFilename = (FreeImage_GetFIFFromFilename_t)s_GetProcAddressEx(hFreeImageLib, "_FreeImage_GetFIFFromFilenameU@4", exportsValid);
				FreeImage_Load = (FreeImage_Load_t)s_GetProcAddressEx(hFreeImageLib, "_FreeImage_LoadU@12", exportsValid);
#else
				FreeImage_GetFileType = (FreeImage_GetFileType_t)s_GetProcAddressEx(hFreeImageLib, "_FreeImage_GetFileType@8", exportsValid);
				FreeImage_GetFIFFromFilename = (FreeImage_GetFIFFromFilename_t)s_GetProcAddressEx(hFreeImageLib, "_FreeImage_GetFIFFromFilename@4", exportsValid);
				FreeImage_Load = (FreeImage_Load_t)s_GetProcAddressEx(hFreeImageLib, "_FreeImage_Load@12", exportsValid);
#endif

				//const char* version = FreeImage_GetVersion();

				// Check the DLL is using compatible exports
				if (exportsValid)
				{
					// Derive file type from file header.
					int fileType = FreeImage_GetFileType(sFilePathName.c_str(), 0);
					if (fileType < 0)
					{
						// No file header available, attempt to parse file name for extension.
						fileType = FreeImage_GetFIFFromFilename(sFilePathName.c_str());
					}

					// If we have a valid file type
					if (fileType >= 0)
					{
						void* dib = FreeImage_Load(fileType, sFilePathName.c_str(), 0);

						if (dib)
						{
							unsigned width = FreeImage_GetWidth(dib);
							unsigned height = FreeImage_GetHeight(dib);

							// Create a GDI+ bitmap to load into...
							pBitmap = new Bitmap(width, height, PixelFormat32bppARGB);

							if (pBitmap && pBitmap->GetLastStatus() == Ok)
							{
								// Write & convert the loaded data into the GDI+ Bitmap
								Rect rect(0, 0, width, height);
								BitmapData bitmapData;
								if (pBitmap->LockBits(&rect, ImageLockModeWrite, PixelFormat32bppARGB, &bitmapData) == Ok)
								{
									FreeImage_ConvertToRawBits((BYTE*)bitmapData.Scan0, dib, bitmapData.Stride, 32, 0xff << RED_SHIFT, 0xff << GREEN_SHIFT, 0xff << BLUE_SHIFT, FALSE);

									pBitmap->UnlockBits(&bitmapData);

									m_Width = width;
									m_Height = height;
									bResult = true;
								}
								else    // Failed to lock the destination Bitmap
								{
									delete pBitmap;
									pBitmap = NULL;
								}
							}
							else    // Bitmap allocation failed
							{
								delete pBitmap;
								pBitmap = NULL;
							}

							FreeImage_Unload(dib);
							dib = NULL;
						}
					}
				}

				FreeLibrary(hFreeImageLib);
				hFreeImageLib = NULL;
			}
		}
	}
	else    // GDI+ Unavailable...
	{
		int nSize = 0;
		pBitmap = NULL;
		CAutoFile hFile = CreateFile(sFilePathName.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_HIDDEN, NULL);
		if (hFile)
		{
			BY_HANDLE_FILE_INFORMATION fileinfo;
			if (GetFileInformationByHandle(hFile, &fileinfo))
			{
				BYTE * buffer = new BYTE[fileinfo.nFileSizeLow];
				DWORD readbytes;
				if (ReadFile(hFile, buffer, fileinfo.nFileSizeLow, &readbytes, NULL))
				{
					if (LoadPictureData(buffer, readbytes))
					{
						m_nSize = fileinfo.nFileSizeLow;
						bResult = true;
					}
				}
				delete [] buffer;
			}
		}
		else
			return bResult;

		m_Name = sFilePathName;
		m_Weight = nSize; // Update Picture Size Info...

		if(m_IPicture != NULL) // Do Not Try To Read From Memory That Does Not Exist...
		{
			m_IPicture->get_Height(&m_Height);
			m_IPicture->get_Width(&m_Width);
			// Calculate Its Size On a "Standard" (96 DPI) Device Context
			m_Height = MulDiv(m_Height, 96, HIMETRIC_INCH);
			m_Width  = MulDiv(m_Width,  96, HIMETRIC_INCH);
		}
		else // Picture Data Is Not a Known Picture Type
		{
			m_Height = 0;
			m_Width = 0;
			bResult = false;
		}
	}

	if ((bResult)&&(m_nSize == 0))
	{
		CAutoFile hFile = CreateFile(sFilePathName.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_HIDDEN, NULL);
		if (hFile)
		{
			BY_HANDLE_FILE_INFORMATION fileinfo;
			if (GetFileInformationByHandle(hFile, &fileinfo))
			{
				m_nSize = fileinfo.nFileSizeLow;
			}
		}
	}

	m_ColorDepth = GetColorDepth();

	return(bResult);
}

bool CPicture::LoadPictureData(BYTE *pBuffer, int nSize)
{
	bool bResult = false;

	HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, nSize);

	if(hGlobal == NULL)
	{
		return(false);
	}

	void* pData = GlobalLock(hGlobal);
	if (pData == NULL)
		return false;
	memcpy(pData, pBuffer, nSize);
	GlobalUnlock(hGlobal);

	IStream* pStream = NULL;

	if ((CreateStreamOnHGlobal(hGlobal, true, &pStream) == S_OK)&&(pStream))
	{
		HRESULT hr = OleLoadPicture(pStream, nSize, false, IID_IPicture, (LPVOID *)&m_IPicture);
		pStream->Release();
		pStream = NULL;

		bResult = hr == S_OK;
	}

	FreeResource(hGlobal); // 16Bit Windows Needs This (32Bit - Automatic Release)

	return(bResult);
}

bool CPicture::Show(HDC hDC, RECT DrawRect)
{
	if (hDC == NULL)
		return false;
	if (bIsIcon && lpIcons)
	{
		::DrawIconEx(hDC, DrawRect.left, DrawRect.top, hIcons[nCurrentIcon], DrawRect.right-DrawRect.left, DrawRect.bottom-DrawRect.top, 0, NULL, DI_NORMAL);
		return true;
	}
	if ((m_IPicture == NULL)&&(pBitmap == NULL))
		return false;

	if (m_IPicture)
	{
		long Width  = 0;
		long Height = 0;
		m_IPicture->get_Width(&Width);
		m_IPicture->get_Height(&Height);

		HRESULT hr = m_IPicture->Render(hDC,
			DrawRect.left,                  // Left
			DrawRect.top,                   // Top
			DrawRect.right - DrawRect.left, // Right
			DrawRect.bottom - DrawRect.top, // Bottom
			0,
			Height,
			Width,
			-Height,
			&DrawRect);

		if (SUCCEEDED(hr))
			return(true);
	}
	else if (pBitmap)
	{
		Graphics graphics(hDC);
		graphics.SetInterpolationMode(m_ip);
		graphics.SetPixelOffsetMode(PixelOffsetModeHighQuality);
		ImageAttributes attr;
		attr.SetWrapMode(WrapModeTileFlipXY);
		Rect rect(DrawRect.left, DrawRect.top, DrawRect.right-DrawRect.left, DrawRect.bottom-DrawRect.top);
		graphics.DrawImage(pBitmap, rect, 0, 0, m_Width, m_Height, UnitPixel, &attr);
		return true;
	}

	return(false);
}

bool CPicture::UpdateSizeOnDC(HDC hDC)
{
	if(hDC == NULL || m_IPicture == NULL) { m_Height = 0; m_Width = 0; return(false); };

	m_IPicture->get_Height(&m_Height);
	m_IPicture->get_Width(&m_Width);

	// Get Current DPI - Dot Per Inch
	int CurrentDPI_X = GetDeviceCaps(hDC, LOGPIXELSX);
	int CurrentDPI_Y = GetDeviceCaps(hDC, LOGPIXELSY);

	m_Height = MulDiv(m_Height, CurrentDPI_Y, HIMETRIC_INCH);
	m_Width  = MulDiv(m_Width,  CurrentDPI_X, HIMETRIC_INCH);

	return(true);
}

UINT CPicture::GetColorDepth() const
{
	if (bIsIcon && lpIcons)
	{
		LPICONDIR lpIconDir = (LPICONDIR)lpIcons;
		return lpIconDir->idEntries[nCurrentIcon].wBitCount;
	}
	switch (GetPixelFormat())
	{
	case PixelFormat1bppIndexed:
		return 1;
	case PixelFormat4bppIndexed:
		return 4;
	case PixelFormat8bppIndexed:
		return 8;
	case PixelFormat16bppARGB1555:
	case PixelFormat16bppGrayScale:
	case PixelFormat16bppRGB555:
	case PixelFormat16bppRGB565:
		return 16;
	case PixelFormat24bppRGB:
		return 24;
	case PixelFormat32bppARGB:
	case PixelFormat32bppPARGB:
	case PixelFormat32bppRGB:
		return 32;
	case PixelFormat48bppRGB:
		return 48;
	case PixelFormat64bppARGB:
	case PixelFormat64bppPARGB:
		return 64;
	}
	return 0;
}

UINT CPicture::GetNumberOfFrames(int dimension)
{
	if (bIsIcon && lpIcons)
	{
		return 1;
	}
	if (pBitmap == NULL)
		return 0;
	UINT count = pBitmap->GetFrameDimensionsCount();
	GUID* pDimensionIDs = (GUID*)malloc(sizeof(GUID)*count);

	pBitmap->GetFrameDimensionsList(pDimensionIDs, count);

	UINT frameCount = pBitmap->GetFrameCount(&pDimensionIDs[dimension]);

	free(pDimensionIDs);
	return frameCount;
}

UINT CPicture::GetNumberOfDimensions()
{
	if (bIsIcon && lpIcons)
	{
		LPICONDIR lpIconDir = (LPICONDIR)lpIcons;
		return lpIconDir->idCount;
	}
	return pBitmap ? pBitmap->GetFrameDimensionsCount() : 0;
}

long CPicture::SetActiveFrame(UINT frame)
{
	if (bIsIcon && lpIcons)
	{
		nCurrentIcon = frame-1;
		m_Height = GetHeight();
		m_Width = GetWidth();
		return 0;
	}
	if (pBitmap == NULL)
		return 0;
	UINT count = 0;
	count = pBitmap->GetFrameDimensionsCount();
	GUID* pDimensionIDs = (GUID*)malloc(sizeof(GUID)*count);

	pBitmap->GetFrameDimensionsList(pDimensionIDs, count);

	UINT frameCount = pBitmap->GetFrameCount(&pDimensionIDs[0]);

	free(pDimensionIDs);

	if (frame > frameCount)
		return 0;

	GUID pageGuid = FrameDimensionTime;
	if (bIsTiff)
		pageGuid = FrameDimensionPage;
	pBitmap->SelectActiveFrame(&pageGuid, frame);

	// Assume that the image has a property item of type PropertyItemEquipMake.
	// Get the size of that property item.
	int nSize = pBitmap->GetPropertyItemSize(PropertyTagFrameDelay);

	// Allocate a buffer to receive the property item.
	PropertyItem* pPropertyItem = (PropertyItem*) malloc(nSize);

	Status s = pBitmap->GetPropertyItem(PropertyTagFrameDelay, nSize, pPropertyItem);

	UINT prevframe = frame;
	if (prevframe > 0)
		prevframe--;
	long delay = 0;
	if (s == Ok)
	{
		delay = ((long*)pPropertyItem->value)[prevframe] * 10;
	}
	free(pPropertyItem);
	m_Height = GetHeight();
	m_Width = GetWidth();
	return delay;
}

UINT CPicture::GetHeight() const
{
	if ((bIsIcon)&&(lpIcons))
	{
		LPICONDIR lpIconDir = (LPICONDIR)lpIcons;
		return lpIconDir->idEntries[nCurrentIcon].bHeight;
	}
	return pBitmap ? pBitmap->GetHeight() : 0;
}

UINT CPicture::GetWidth() const
{
	if ((bIsIcon)&&(lpIcons))
	{
		LPICONDIR lpIconDir = (LPICONDIR)lpIcons;
		return lpIconDir->idEntries[nCurrentIcon].bWidth;
	}
	return pBitmap ? pBitmap->GetWidth() : 0;
}

PixelFormat CPicture::GetPixelFormat() const
{
	if ((bIsIcon)&&(lpIcons))
	{
		LPICONDIR lpIconDir = (LPICONDIR)lpIcons;
		if (lpIconDir->idEntries[nCurrentIcon].wPlanes == 1)
		{
			if (lpIconDir->idEntries[nCurrentIcon].wBitCount == 1)
				return PixelFormat1bppIndexed;
			if (lpIconDir->idEntries[nCurrentIcon].wBitCount == 4)
				return PixelFormat4bppIndexed;
			if (lpIconDir->idEntries[nCurrentIcon].wBitCount == 8)
				return PixelFormat8bppIndexed;
		}
		if (lpIconDir->idEntries[nCurrentIcon].wBitCount == 32)
		{
			return PixelFormat32bppARGB;
		}
	}
	return pBitmap ? pBitmap->GetPixelFormat() : 0;
}
