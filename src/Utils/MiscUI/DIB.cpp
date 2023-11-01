// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2023 - TortoiseGit
// Copyright (C) 2003-2006, 2017 - TortoiseSVN

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
#include "DIB.h"
#include <atlbase.h>
#include <d2d1.h>
#include <d2d1_3.h>
#pragma comment(lib, "d2d1.lib")

CDib::CDib()
{
	DeleteObject();
}

CDib::~CDib()
{
	DeleteObject();
}

int CDib::BytesPerLine(int nWidth, int nBitsPerPixel)
{
	int nBytesPerLine = nWidth * nBitsPerPixel;
	nBytesPerLine = ( (nBytesPerLine + 31) & (~31) ) / 8;
	return nBytesPerLine;
}

void CDib::DeleteObject()
{
	m_pBits = nullptr;
	if (m_hBitmap)
		::DeleteObject(m_hBitmap);
	m_hBitmap = nullptr;

	memset(&m_BMinfo, 0, sizeof(m_BMinfo));
}

void CDib::Create32BitFromPicture (CPictureHolder* pPicture, int iWidth, int iHeight)
{
	CRect r;
	CBitmap newBMP;
	CWindowDC dc(nullptr);
	CDC tempDC;

	tempDC.CreateCompatibleDC(&dc);

	newBMP.CreateDiscardableBitmap(&dc,iWidth,iHeight);

	CBitmap* pOldBitmap = tempDC.SelectObject(&newBMP);

	r.SetRect(0,0,iWidth,iHeight);
	pPicture->Render(&tempDC,r,r);

	BITMAPINFO bi{};
	bi.bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth         = iWidth;
	bi.bmiHeader.biHeight        = iHeight;
	bi.bmiHeader.biPlanes        = 1;
	bi.bmiHeader.biBitCount      = 32;
	bi.bmiHeader.biCompression   = BI_RGB;

	SetBitmap(&bi, nullptr);

	auto pAr = static_cast<DWORD*>(GetDIBits());

	// Copy data into the 32 bit dib..
	for(int i=0;i<iHeight;i++)
	{
		for(int j=0;j<iWidth;j++)
		{
			pAr[(i*iWidth)+j] = FixColorRef(tempDC.GetPixel(j,i));
		}
	}

	tempDC.SelectObject(pOldBitmap);
}

bool CDib::Create32BitFromSVG(const std::string_view svg, int iWidth, int iHeight)
{
	// initialize Direct2D
	D2D1_FACTORY_OPTIONS options = {
#ifdef _DEBUG
		D2D1_DEBUG_LEVEL_INFORMATION
#endif
	};
	CComPtr<ID2D1Factory> factory;
	if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, IID_ID2D1Factory, &options, reinterpret_cast<void**>(&factory))))
		return false;

	BITMAPINFO bi{};
	bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth = iWidth;
	bi.bmiHeader.biHeight = iHeight;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 32;
	bi.bmiHeader.biCompression = BI_RGB;
	// Create a 32 bit bitmap
	SetBitmap(&bi, nullptr);

	CComPtr<ID2D1DCRenderTarget> target;
	D2D1_RENDER_TARGET_PROPERTIES props{};
	props.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
	props.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
	if (FAILED(factory->CreateDCRenderTarget(&props, &target)))
		return false;

	CDC cdc;
	cdc.CreateCompatibleDC(nullptr);
	cdc.SelectObject(m_hBitmap);
	RECT rc = { 0, 0, iWidth, iHeight };
	if (FAILED(target->BindDC(cdc.GetSafeHdc(), &rc)))
		return false;

	// this requires Windows 10 1703
	CComPtr<ID2D1DeviceContext5> dc;
	if (FAILED(target->QueryInterface(&dc)))
		return false;

	CComPtr<IStream> stream(::SHCreateMemStream(reinterpret_cast<const BYTE*>(svg.data()), static_cast<UINT>(svg.size())));
	if (!stream)
		return false;

	// open the SVG as a document
	CComPtr<ID2D1SvgDocument> svgDoc;
	D2D1_SIZE_F size{ static_cast<float>(iWidth), static_cast<float>(iHeight) };
	if (FAILED(dc->CreateSvgDocument(stream, size, &svgDoc)))
		return false;
	CComPtr<ID2D1SvgElement> svgElem;
	svgDoc->GetRoot(&svgElem);
	float fWidth, fHeight;
	if (FAILED(svgElem->GetAttributeValue(L"width", &fWidth)) || !fWidth)
		return false;
	if (FAILED(svgElem->GetAttributeValue(L"height", &fHeight)) || !fHeight)
		return false;
	float fRatioWidth = static_cast<float>(iWidth) / fWidth;
	float fRatioHeight = static_cast<float>(iHeight) / fHeight;
	float fRatio = std::min(fRatioWidth, fRatioHeight);

	// draw it on the render target
	target->BeginDraw();
	target->Clear(D2D1::ColorF(D2D1::ColorF::White));
	target->SetTransform(D2D1::Matrix3x2F::Scale(fRatio, fRatio));
	svgDoc->GetViewportSize();
	dc->DrawSvgDocument(svgDoc);
	return SUCCEEDED(target->EndDraw());
}

BOOL CDib::SetBitmap(const LPBITMAPINFO lpBitmapInfo, const LPVOID lpBits)
{
	DeleteObject();

	if (!lpBitmapInfo)
		return FALSE;

	DWORD dwBitmapInfoSize = sizeof(BITMAPINFO);

	memcpy(&m_BMinfo, lpBitmapInfo, dwBitmapInfoSize);

	auto hDC = ::GetDC(nullptr);
	if (!hDC)
	{
		DeleteObject();
		return FALSE;
	}

	m_hBitmap = CreateDIBSection(hDC, &m_BMinfo,
									DIB_RGB_COLORS, &m_pBits, nullptr, 0);
	::ReleaseDC(nullptr, hDC);
	if (!m_hBitmap)
	{
		DeleteObject();
		return FALSE;
	}

	DWORD dwImageSize = m_BMinfo.bmiHeader.biSizeImage;
	if (dwImageSize == 0)
	{
		int nBytesPerLine = BytesPerLine(lpBitmapInfo->bmiHeader.biWidth,
											lpBitmapInfo->bmiHeader.biBitCount);
		dwImageSize = nBytesPerLine * lpBitmapInfo->bmiHeader.biHeight;
	}

	GdiFlush();

	if (lpBits)
		memcpy(m_pBits, lpBits, dwImageSize);

	return TRUE;
}

BOOL CDib::Draw(CDC* pDC, CPoint ptDest)
{
	if (!m_hBitmap)
		return FALSE;

	CSize size = GetSize();
	CPoint SrcOrigin = CPoint(0,0);

	return SetDIBitsToDevice(pDC->GetSafeHdc(),
								ptDest.x, ptDest.y,
								size.cx, size.cy,
								SrcOrigin.x, SrcOrigin.y,
								SrcOrigin.y, size.cy - SrcOrigin.y,
								GetDIBits(), &m_BMinfo,
								DIB_RGB_COLORS);
}

COLORREF CDib::FixColorRef(COLORREF clr)
{
	int r = GetRValue(clr);
	int g = GetGValue(clr);
	int b =  GetBValue(clr);

	return RGB(b,g,r);
}
