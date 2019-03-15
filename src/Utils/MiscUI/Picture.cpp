// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2015-2019 - TortoiseGit
// Copyright (C) 2003-2015, 2017 - TortoiseSVN

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
#include <Shlwapi.h>
#include <locale>
#include <algorithm>
#include "Picture.h"
#include "SmartHandle.h"
#include <atlbase.h>
#include <Wincodec.h>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "gdiplus.lib")
// note: linking with Windowscodecs.lib does not make the exe require the dll
// which means it still runs on XP even if the WIC isn't installed - WIC won't work
// but the exe still runs
#pragma comment(lib, "Windowscodecs.lib")

#define HIMETRIC_INCH 2540

CPicture::CPicture()
	: m_IPicture(nullptr)
	, m_Height(0)
	, m_Weight(0)
	, m_Width(0)
	, pBitmap(nullptr)
	, pBitmapBuffer(nullptr)
	, m_ip(InterpolationModeDefault)
	, hIcons(nullptr)
	, lpIcons(nullptr)
	, nCurrentIcon(0)
	, bIsIcon(false)
	, bIsTiff(false)
	, m_nSize(0)
	, m_ColorDepth(0)
	, gdiplusToken(NULL)
{
}

CPicture::~CPicture()
{
	FreePictureData(); // Important - Avoid Leaks...
	if (gdiplusToken)
		GdiplusShutdown(gdiplusToken);
}


void CPicture::FreePictureData()
{
	if (m_IPicture)
	{
		m_IPicture->Release();
		m_IPicture = nullptr;
		m_Height = 0;
		m_Weight = 0;
		m_Width = 0;
		m_nSize = 0;
	}
	if (hIcons)
	{
		auto lpIconDir = reinterpret_cast<LPICONDIR>(lpIcons);
		if (lpIconDir)
		{
			for (int i = 0; i < lpIconDir->idCount; ++i)
			{
				DestroyIcon(hIcons[i]);
			}
		}
		delete[] hIcons;
		hIcons = nullptr;
	}
	delete[] lpIcons;
	lpIcons = nullptr;
	delete pBitmap;
	pBitmap = nullptr;
	delete[] pBitmapBuffer;
	pBitmapBuffer = nullptr;
}

// Util function to ease loading of FreeImage library
static FARPROC s_GetProcAddressEx(HMODULE hDll, const char* procName, bool& valid)
{
	FARPROC proc = nullptr;

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
	TCHAR buf[100] = { 0 };
	if (bAbbrev)
		StrFormatByteSize(m_nSize, buf, _countof(buf));
	else
		swprintf_s(buf, L"%lu Bytes", m_nSize);

	return tstring(buf);
}

bool CPicture::Load(tstring sFilePathName)
{
	bool bResult = false;
	bIsIcon = false;
	lpIcons = nullptr;
	//CFile PictureFile;
	//CFileException e;
	FreePictureData(); // Important - Avoid Leaks...

	// No-op if no file specified
	if (sFilePathName.empty())
		return true;

	// Load & initialize the GDI+ library
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

	// Attempt to load using GDI+
	pBitmap = new Bitmap(sFilePathName.c_str(), FALSE);
	GUID guid;
	pBitmap->GetRawFormat(&guid);

	if (pBitmap->GetLastStatus() != Ok)
	{
		delete pBitmap;
		pBitmap = nullptr;
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
	auto lowerfilename = sFilePathName;
	std::transform(lowerfilename.begin(), lowerfilename.end(), lowerfilename.begin(), ::towlower);
	bIsIcon = (guid == ImageFormatIcon) || (wcsstr(lowerfilename.c_str(), L".ico")) || (wcsstr(lowerfilename.c_str(), L".cur"));
	bIsTiff = (guid == ImageFormatTIFF) || (wcsstr(lowerfilename.c_str(), L".tiff"));
	m_Name = sFilePathName;

	if (bIsIcon)
	{
		// Icon file, get special treatment...
		if (pBitmap)
		{
			// Cleanup first...
			delete (pBitmap);
			pBitmap = nullptr;
			bIsIcon = true;
		}

		CAutoFile hFile = CreateFile(sFilePathName.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (hFile)
		{
			BY_HANDLE_FILE_INFORMATION fileinfo;
			if (GetFileInformationByHandle(hFile, &fileinfo))
			{
				lpIcons = new BYTE[fileinfo.nFileSizeLow];
				DWORD readbytes;
				if (ReadFile(hFile, lpIcons, fileinfo.nFileSizeLow, &readbytes, nullptr))
				{
					// we have the icon. Now gather the information we need later
					if (readbytes >= sizeof(ICONDIR))
					{
						// we are going to open same file second time so we have to close the file now
						hFile.CloseHandle();

						auto lpIconDir = reinterpret_cast<LPICONDIR>(lpIcons);
						if ((lpIconDir->idCount) && ((lpIconDir->idCount * sizeof(ICONDIR)) <= fileinfo.nFileSizeLow))
						{
							try
							{
								bResult = false;
								nCurrentIcon = 0;
								hIcons = new HICON[lpIconDir->idCount];
								// check that the pointers point to data that we just loaded
								if ((reinterpret_cast<BYTE*>(lpIconDir->idEntries) > reinterpret_cast<BYTE*>(lpIconDir)) &&
									(reinterpret_cast<BYTE*>(lpIconDir->idEntries) + (lpIconDir->idCount * sizeof(ICONDIRENTRY)) < reinterpret_cast<BYTE*>(lpIconDir) + fileinfo.nFileSizeLow))
								{
									m_Width = lpIconDir->idEntries[0].bWidth == 0 ? 256 : lpIconDir->idEntries[0].bWidth;
									m_Height = lpIconDir->idEntries[0].bHeight == 0 ? 256 : lpIconDir->idEntries[0].bHeight;
									bResult = true;
									for (int i = 0; i < lpIconDir->idCount; ++i)
									{
										hIcons[i] = static_cast<HICON>(LoadImage(nullptr, sFilePathName.c_str(), IMAGE_ICON,
																	 lpIconDir->idEntries[i].bWidth == 0 ? 256 : lpIconDir->idEntries[i].bWidth,
																	 lpIconDir->idEntries[i].bHeight == 0 ? 256 : lpIconDir->idEntries[i].bHeight,
																	 LR_LOADFROMFILE));
										if (hIcons[i] == nullptr)
										{
											// if the icon couldn't be loaded, the data is most likely corrupt
											delete[] lpIcons;
											lpIcons = nullptr;
											bResult = false;
											break;
										}
									}
								}
							}
							catch (...)
							{
								delete[] lpIcons;
								lpIcons = nullptr;
								bResult = false;
							}
						}
						else
						{
							delete[] lpIcons;
							lpIcons = nullptr;
							bResult = false;
						}
					}
					else
					{
						delete[] lpIcons;
						lpIcons = nullptr;
						bResult = false;
					}
				}
				else
				{
					delete[] lpIcons;
					lpIcons = nullptr;
				}
			}
		}
	}
	else if (pBitmap)   // Image loaded successfully with GDI+
	{
		m_Height = pBitmap->GetHeight();
		m_Width = pBitmap->GetWidth();
		bResult = true;
	}

	// If still failed to load the file...
	if (!bResult)
	{
		// try WIC
		IWICImagingFactory* pFactory = nullptr;
		HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory,
									  nullptr,
									  CLSCTX_INPROC_SERVER,
									  IID_IWICImagingFactory,
									  reinterpret_cast<LPVOID*>(&pFactory));

		// Create a decoder from the file.
		if (SUCCEEDED(hr))
		{
			IWICBitmapDecoder* pDecoder = nullptr;
			hr = pFactory->CreateDecoderFromFilename(m_Name.c_str(),
													 nullptr,
													 GENERIC_READ,
													 WICDecodeMetadataCacheOnDemand,
													 &pDecoder);
			if (SUCCEEDED(hr))
			{
				IWICBitmapFrameDecode* pBitmapFrameDecode = nullptr;
				hr = pDecoder->GetFrame(0, &pBitmapFrameDecode);
				if (SUCCEEDED(hr))
				{
					IWICBitmapSource* pSource = nullptr;
					pSource = pBitmapFrameDecode;
					pSource->AddRef();

					IWICFormatConverter* piFormatConverter = nullptr;
					hr = pFactory->CreateFormatConverter(&piFormatConverter);
					if (SUCCEEDED(hr))
					{
						hr = piFormatConverter->Initialize(pSource, GUID_WICPixelFormat24bppBGR, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);
						if (SUCCEEDED(hr))
						{
							UINT uWidth = 0;
							UINT uHeight = 0;
							hr = piFormatConverter->GetSize(&uWidth, &uHeight);
							m_Height = uHeight;
							m_Width = uWidth;
							if (SUCCEEDED(hr))
							{
								WICPixelFormatGUID pixelFormat = { 0 };
								hr = piFormatConverter->GetPixelFormat(&pixelFormat);

								if (SUCCEEDED(hr))
								{
									UINT cbStride = uWidth * 3;
									// Force the stride to be a multiple of sizeof(DWORD)
									cbStride = ((cbStride + sizeof(DWORD) - 1) / sizeof(DWORD)) * sizeof(DWORD);

									UINT cbBufferSize = cbStride * uHeight;
									// note: the buffer must exist during the lifetime of the pBitmap object created below
									pBitmapBuffer = new BYTE[cbBufferSize];

									if (pBitmapBuffer != NULL)
									{
										WICRect rc = { 0, 0, static_cast<int>(uWidth), static_cast<int>(uHeight) };
										hr = piFormatConverter->CopyPixels(&rc, cbStride, cbStride * uHeight, pBitmapBuffer);
										if (SUCCEEDED(hr))
										{
											pBitmap = new Bitmap(uWidth, uHeight, cbStride, PixelFormat24bppRGB, pBitmapBuffer);
											bResult = true;
										}
										else
										{
											delete[] pBitmapBuffer;
											pBitmapBuffer = nullptr;
										}
									}
									else
										hr = ERROR_NOT_ENOUGH_MEMORY;
								}
							}
						}
						piFormatConverter->Release();
					}
					pSource->Release();
					pBitmapFrameDecode->Release();
				}
				pDecoder->Release();
			}
			pFactory->Release();
		}
	}

	if (!bResult)
	{
		// Attempt to load the FreeImage library as an optional DLL to support additional formats

		// NOTE: Currently just loading via FreeImage & using GDI+ for drawing.
		// It might be nice to remove this dependency in the future.
		HMODULE hFreeImageLib = LoadLibrary(L"FreeImage.dll");

		// FreeImage DLL functions
		typedef const char* (__stdcall *FreeImage_GetVersion_t)(void);
		typedef int(__stdcall *FreeImage_GetFileType_t)(const TCHAR *filename, int size);
		typedef int(__stdcall *FreeImage_GetFIFFromFilename_t)(const TCHAR *filename);
		typedef void*       (__stdcall *FreeImage_Load_t)(int format, const TCHAR *filename, int flags);
		typedef void(__stdcall *FreeImage_Unload_t)(void* dib);
		typedef int(__stdcall *FreeImage_GetColorType_t)(void* dib);
		typedef unsigned(__stdcall *FreeImage_GetWidth_t)(void* dib);
		typedef unsigned(__stdcall *FreeImage_GetHeight_t)(void* dib);
		typedef void(__stdcall *FreeImage_ConvertToRawBits_t)(BYTE *bits, void *dib, int pitch, unsigned bpp, unsigned red_mask, unsigned green_mask, unsigned blue_mask, BOOL topdown);

		//FreeImage_GetVersion_t FreeImage_GetVersion = nullptr;
		FreeImage_GetFileType_t FreeImage_GetFileType = nullptr;
		FreeImage_GetFIFFromFilename_t FreeImage_GetFIFFromFilename = nullptr;
		FreeImage_Load_t FreeImage_Load = nullptr;
		FreeImage_Unload_t FreeImage_Unload = nullptr;
		//FreeImage_GetColorType_t FreeImage_GetColorType = nullptr;
		FreeImage_GetWidth_t FreeImage_GetWidth = nullptr;
		FreeImage_GetHeight_t FreeImage_GetHeight = nullptr;
		FreeImage_ConvertToRawBits_t  FreeImage_ConvertToRawBits = nullptr;

		if (hFreeImageLib)
		{
			bool exportsValid = true;

			//FreeImage_GetVersion = reinterpret_cast<FreeImage_GetVersion_t>(s_GetProcAddressEx(hFreeImageLib, "_FreeImage_GetVersion@0", valid));
			FreeImage_GetWidth = reinterpret_cast<FreeImage_GetWidth_t>(s_GetProcAddressEx(hFreeImageLib, "_FreeImage_GetWidth@4", exportsValid));
			FreeImage_GetHeight = reinterpret_cast<FreeImage_GetHeight_t>(s_GetProcAddressEx(hFreeImageLib, "_FreeImage_GetHeight@4", exportsValid));
			FreeImage_Unload = reinterpret_cast<FreeImage_Unload_t>(s_GetProcAddressEx(hFreeImageLib, "_FreeImage_Unload@4", exportsValid));
			FreeImage_ConvertToRawBits = reinterpret_cast<FreeImage_ConvertToRawBits_t>(s_GetProcAddressEx(hFreeImageLib, "_FreeImage_ConvertToRawBits@32", exportsValid));

#ifdef UNICODE
			FreeImage_GetFileType = reinterpret_cast<FreeImage_GetFileType_t>(s_GetProcAddressEx(hFreeImageLib, "_FreeImage_GetFileTypeU@8", exportsValid));
			FreeImage_GetFIFFromFilename = reinterpret_cast<FreeImage_GetFIFFromFilename_t>(s_GetProcAddressEx(hFreeImageLib, "_FreeImage_GetFIFFromFilenameU@4", exportsValid));
			FreeImage_Load = reinterpret_cast<FreeImage_Load_t>(s_GetProcAddressEx(hFreeImageLib, "_FreeImage_LoadU@12", exportsValid));
#else
			FreeImage_GetFileType = reinterpret_cast<FreeImage_GetFileType_t>(s_GetProcAddressEx(hFreeImageLib, "_FreeImage_GetFileType@8", exportsValid));
			FreeImage_GetFIFFromFilename = reinterpret_cast<FreeImage_GetFIFFromFilename_t>(s_GetProcAddressEx(hFreeImageLib, "_FreeImage_GetFIFFromFilename@4", exportsValid));
			FreeImage_Load = reinterpret_cast<FreeImage_Load_t>(s_GetProcAddressEx(hFreeImageLib, "_FreeImage_Load@12", exportsValid));
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
								FreeImage_ConvertToRawBits(static_cast<BYTE*>(bitmapData.Scan0), dib, bitmapData.Stride, 32, 0xff << RED_SHIFT, 0xff << GREEN_SHIFT, 0xff << BLUE_SHIFT, FALSE);

								pBitmap->UnlockBits(&bitmapData);

								m_Width = width;
								m_Height = height;
								bResult = true;
							}
							else    // Failed to lock the destination Bitmap
							{
								delete pBitmap;
								pBitmap = nullptr;
							}
						}
						else    // Bitmap allocation failed
						{
							delete pBitmap;
							pBitmap = nullptr;
						}

						FreeImage_Unload(dib);
						dib = nullptr;
					}
				}
			}

			FreeLibrary(hFreeImageLib);
			hFreeImageLib = nullptr;
		}
	}

	if ((bResult) && (m_nSize == 0))
	{
		CAutoFile hFile = CreateFile(sFilePathName.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_HIDDEN, nullptr);
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

	HGLOBAL hGlobalMem = GlobalAlloc(GMEM_MOVEABLE, nSize);

	if (!hGlobalMem)
	{
		return(false);
	}

	void* pData = GlobalLock(hGlobalMem);
	if (!pData)
	{
		GlobalFree(hGlobalMem);
		return false;
	}
	memcpy(pData, pBuffer, nSize);
	GlobalUnlock(hGlobalMem);

	IStream* pStream = nullptr;

	if ((CreateStreamOnHGlobal(hGlobalMem, true, &pStream) == S_OK) && (pStream))
	{
		HRESULT hr = OleLoadPicture(pStream, nSize, false, IID_IPicture, reinterpret_cast<LPVOID*>(&m_IPicture));
		pStream->Release();
		pStream = nullptr;

		bResult = hr == S_OK;
	}

	FreeResource(hGlobalMem); // 16Bit Windows Needs This (32Bit - Automatic Release)

	return(bResult);
}

bool CPicture::Show(HDC hDC, RECT DrawRect)
{
	if (!hDC)
		return false;
	if (bIsIcon && lpIcons)
	{
			::DrawIconEx(hDC, DrawRect.left, DrawRect.top, hIcons[nCurrentIcon], DrawRect.right - DrawRect.left, DrawRect.bottom - DrawRect.top, 0, nullptr, DI_NORMAL);
		return true;
	}
	if (!m_IPicture && !pBitmap)
		return false;

	if (m_IPicture)
	{
		long Width = 0;
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
		graphics.SetPixelOffsetMode(PixelOffsetModeHalf);
		ImageAttributes attr;
		attr.SetWrapMode(WrapModeTileFlipXY);
		Rect rect(DrawRect.left, DrawRect.top, DrawRect.right - DrawRect.left, DrawRect.bottom - DrawRect.top);
		graphics.DrawImage(pBitmap, rect, 0, 0, m_Width, m_Height, UnitPixel, &attr);
		return true;
	}

	return(false);
}

bool CPicture::UpdateSizeOnDC(HDC hDC)
{
	if (!hDC || !m_IPicture) { m_Height = 0; m_Width = 0; return(false); };

	m_IPicture->get_Height(&m_Height);
	m_IPicture->get_Width(&m_Width);

	// Get Current DPI - Dot Per Inch
	int CurrentDPI_X = GetDeviceCaps(hDC, LOGPIXELSX);
	int CurrentDPI_Y = GetDeviceCaps(hDC, LOGPIXELSY);

	m_Height = MulDiv(m_Height, CurrentDPI_Y, HIMETRIC_INCH);
	m_Width = MulDiv(m_Width, CurrentDPI_X, HIMETRIC_INCH);

	return(true);
}

UINT CPicture::GetColorDepth() const
{
	if (bIsIcon && lpIcons)
	{
		auto lpIconDir = reinterpret_cast<LPICONDIR>(lpIcons);
		return lpIconDir->idEntries[nCurrentIcon].wBitCount;
	}

	// try first with WIC to get the pixel format since GDI+ often returns 32-bit even if it's not
	// Create the image factory.
	UINT bpp = 0;
	IWICImagingFactory* pFactory = nullptr;
	HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory,
								  nullptr,
								  CLSCTX_INPROC_SERVER,
								  IID_IWICImagingFactory,
								  reinterpret_cast<LPVOID*>(&pFactory));

	// Create a decoder from the file.
	if (SUCCEEDED(hr))
	{
		IWICBitmapDecoder* pDecoder = nullptr;
		hr = pFactory->CreateDecoderFromFilename(m_Name.c_str(),
												 nullptr,
												 GENERIC_READ,
												 WICDecodeMetadataCacheOnDemand,
												 &pDecoder);
		if (SUCCEEDED(hr))
		{
			IWICBitmapFrameDecode* pBitmapFrameDecode = nullptr;
			hr = pDecoder->GetFrame(0, &pBitmapFrameDecode);
			if (SUCCEEDED(hr))
			{
				IWICBitmapSource* pSource = nullptr;
				pSource = pBitmapFrameDecode;
				pSource->AddRef();
				WICPixelFormatGUID pixelFormat;
				hr = pSource->GetPixelFormat(&pixelFormat);
				if (SUCCEEDED(hr))
				{
					IWICComponentInfo* piCompInfo = nullptr;
					hr = pFactory->CreateComponentInfo(pixelFormat, &piCompInfo);
					if (SUCCEEDED(hr))
					{
						IWICPixelFormatInfo* piPixelFormatInfo = nullptr;
						hr = piCompInfo->QueryInterface(IID_IWICPixelFormatInfo, reinterpret_cast<LPVOID*>(&piPixelFormatInfo));
						if (SUCCEEDED(hr))
						{
							hr = piPixelFormatInfo->GetBitsPerPixel(&bpp);
							piPixelFormatInfo->Release();
						}
						piCompInfo->Release();
					}
					pSource->Release();
				}
				pBitmapFrameDecode->Release();
			}
			pDecoder->Release();
		}
		pFactory->Release();
	}
	if (bpp)
		return bpp;

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
			// note: GDI+ converts images with bit depths > 32 automatically
			// on loading, so PixelFormat48bppRGB, PixelFormat64bppARGB and
			// PixelFormat64bppPARGB will never be used here.
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
	if (!pBitmap)
		return 0;
	UINT count = pBitmap->GetFrameDimensionsCount();
	auto pDimensionIDs = static_cast<GUID*>(malloc(sizeof(GUID) * count));

	pBitmap->GetFrameDimensionsList(pDimensionIDs, count);

	UINT frameCount = pBitmap->GetFrameCount(&pDimensionIDs[dimension]);

	free(pDimensionIDs);
	return frameCount;
}

UINT CPicture::GetNumberOfDimensions()
{
	if (bIsIcon && lpIcons)
	{
		auto lpIconDir = reinterpret_cast<LPICONDIR>(lpIcons);
		return lpIconDir->idCount;
	}
	return pBitmap ? pBitmap->GetFrameDimensionsCount() : 0;
}

long CPicture::SetActiveFrame(UINT frame)
{
	if (bIsIcon && lpIcons)
	{
		nCurrentIcon = frame - 1;
		m_Height = GetHeight();
		m_Width = GetWidth();
		return 0;
	}
	if (!pBitmap)
		return 0;
	UINT count = 0;
	count = pBitmap->GetFrameDimensionsCount();
	auto pDimensionIDs = static_cast<GUID*>(malloc(sizeof(GUID) * count));

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
	auto pPropertyItem = static_cast<PropertyItem*>(malloc(nSize));

	Status s = pBitmap->GetPropertyItem(PropertyTagFrameDelay, nSize, pPropertyItem);

	UINT prevframe = frame;
	if (prevframe > 0)
		prevframe--;
	long delay = 0;
	if (s == Ok)
	{
		delay = reinterpret_cast<long*>(pPropertyItem->value)[prevframe] * 10;
	}
	free(pPropertyItem);
	m_Height = GetHeight();
	m_Width = GetWidth();
	return delay;
}

UINT CPicture::GetHeight() const
{
	if ((bIsIcon) && (lpIcons))
	{
		auto lpIconDir = reinterpret_cast<LPICONDIR>(lpIcons);
		return lpIconDir->idEntries[nCurrentIcon].bHeight == 0 ? 256 : lpIconDir->idEntries[nCurrentIcon].bHeight;
	}
	return pBitmap ? pBitmap->GetHeight() : 0;
}

UINT CPicture::GetWidth() const
{
	if ((bIsIcon) && (lpIcons))
	{
		auto lpIconDir = reinterpret_cast<LPICONDIR>(lpIcons);
		return lpIconDir->idEntries[nCurrentIcon].bWidth == 0 ? 256 : lpIconDir->idEntries[nCurrentIcon].bWidth;
	}
	return pBitmap ? pBitmap->GetWidth() : 0;
}

PixelFormat CPicture::GetPixelFormat() const
{
	if ((bIsIcon) && (lpIcons))
	{
		auto lpIconDir = reinterpret_cast<LPICONDIR>(lpIcons);
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
