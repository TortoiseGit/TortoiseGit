/********************************************************************
*
* Copyright (c) 2002 Sven Wiegand <mail@sven-wiegand.de>
*
* You can use this and modify this in any way you want,
* BUT LEAVE THIS HEADER INTACT.
*
* Redistribution is appreciated.
*
* $Workfile:$
* $Revision:$
* $Modtime:$
* $Author:$
*
* Revision History:
*	$History:$
*
*********************************************************************/

#include "stdafx.h"
#include "PropPageFrameDefault.h"
#include "ExtTextOutFL.h"

namespace TreePropSheet
{


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//-------------------------------------------------------------------
// class CPropPageFrameDefault
//-------------------------------------------------------------------

BEGIN_MESSAGE_MAP(CPropPageFrameDefault, CWnd)
	//{{AFX_MSG_MAP(CPropPageFrameDefault)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


CPropPageFrameDefault::CPropPageFrameDefault()
{
}


CPropPageFrameDefault::~CPropPageFrameDefault()
{
	if (m_Images.GetSafeHandle())
		m_Images.DeleteImageList();
}


/////////////////////////////////////////////////////////////////////
// Overridings

BOOL CPropPageFrameDefault::Create(DWORD dwWindowStyle, const RECT &rect, CWnd *pwndParent, UINT nID)
{
	return CWnd::Create(
		AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW, AfxGetApp()->LoadStandardCursor(IDC_ARROW), GetSysColorBrush(COLOR_3DFACE)),
		L"Page Frame",
		dwWindowStyle, rect, pwndParent, nID);
}


CWnd* CPropPageFrameDefault::GetWnd()
{
	return static_cast<CWnd*>(this);
}


void CPropPageFrameDefault::SetCaption(LPCTSTR lpszCaption, HICON hIcon /*= nullptr*/)
{
	CPropPageFrame::SetCaption(lpszCaption, hIcon);

	// build image list
	if (m_Images.GetSafeHandle())
		m_Images.DeleteImageList();
	if (hIcon)
	{
		ICONINFO	ii;
		if (!GetIconInfo(hIcon, &ii))
			return;

		CBitmap	bmMask;
		bmMask.Attach(ii.hbmMask);
		if (ii.hbmColor) DeleteObject(ii.hbmColor);

		BITMAP	bm;
		bmMask.GetBitmap(&bm);

		if (!m_Images.Create(bm.bmWidth, bm.bmHeight, ILC_COLOR32|ILC_MASK, 0, 1))
			return;

		if (m_Images.Add(hIcon) == -1)
			m_Images.DeleteImageList();
	}
}


CRect CPropPageFrameDefault::CalcMsgArea()
{
	CRect	rect;
	GetClientRect(rect);
	if (IsAppThemed())
	{
		HTHEME hTheme = OpenThemeData(m_hWnd, L"Tab");
		if (hTheme)
		{
			CRect	rectContent;
			CDC		*pDc = GetDC();
			GetThemeBackgroundContentRect(hTheme, pDc->m_hDC, TABP_PANE, 0, rect, rectContent);
			ReleaseDC(pDc);
			CloseThemeData(hTheme);

			if (GetShowCaption())
				rectContent.top = rect.top+GetCaptionHeight()+1;
			rect = rectContent;
		}
	}
	else if (GetShowCaption())
		rect.top+= GetCaptionHeight()+1;

	return rect;
}


CRect CPropPageFrameDefault::CalcCaptionArea()
{
	CRect	rect;
	GetClientRect(rect);
	if (IsAppThemed())
	{
		HTHEME hTheme = OpenThemeData(m_hWnd, L"Tab");
		if (hTheme)
		{
			CRect	rectContent;
			CDC		*pDc = GetDC();
			GetThemeBackgroundContentRect(hTheme, pDc->m_hDC, TABP_PANE, 0, rect, rectContent);
			ReleaseDC(pDc);
			CloseThemeData(hTheme);

			if (GetShowCaption())
				rectContent.bottom = rect.top+GetCaptionHeight();
			else
				rectContent.bottom = rectContent.top;

			rect = rectContent;
		}
	}
	else
	{
		if (GetShowCaption())
			rect.bottom = rect.top+GetCaptionHeight();
		else
			rect.bottom = rect.top;
	}

	return rect;
}

void CPropPageFrameDefault::DrawCaption(CDC *pDc, CRect rect, LPCTSTR lpszCaption, HICON hIcon)
{
	COLORREF	clrLeft = GetSysColor(COLOR_INACTIVECAPTION);
	COLORREF	clrRight = pDc->GetPixel(rect.right-1, rect.top);
	FillGradientRectH(pDc, rect, clrLeft, clrRight);

	double hScale = 1.0;
	double vScale = 1.0;

	// draw icon
	if (hIcon && m_Images.GetSafeHandle() && m_Images.GetImageCount() == 1)
	{
		IMAGEINFO	ii;
		m_Images.GetImageInfo(0, &ii);
		const RECT& rcImage = ii.rcImage;
		SIZE sz;
		sz.cx = rcImage.right - rcImage.left;
		sz.cy = rcImage.bottom - rcImage.top;
		hScale = vScale = max(1.0, rect.Height() / sz.cy * 0.7);
		sz.cx = static_cast<LONG>(sz.cx * hScale + 0.5);
		sz.cy = static_cast<LONG>(sz.cy * vScale + 0.5);
		LONG xOffset = static_cast<LONG>(6 * hScale + 0.5);
		CPoint pt(xOffset, rect.CenterPoint().y - (sz.cy / 2));
		m_Images.DrawEx(pDc, 0, pt, sz, CLR_DEFAULT, CLR_DEFAULT, ILD_TRANSPARENT | ILD_SCALE);
		rect.left += sz.cx + xOffset;
	}

	// draw text
	rect.left += static_cast<LONG>(2 * hScale + 0.5);

	COLORREF	clrPrev = pDc->SetTextColor(GetSysColor(COLOR_CAPTIONTEXT));
	int				nBkStyle = pDc->SetBkMode(TRANSPARENT);

	LOGFONT lf;
	SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &lf, 0);
	CFont font;
	font.CreateFontIndirect(&lf);
	CFont* pFont = pDc->SelectObject(&font);

	pDc->DrawText(lpszCaption, rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

	pDc->SetTextColor(clrPrev);
	pDc->SetBkMode(nBkStyle);
	pDc->SelectObject(pFont);
}


/////////////////////////////////////////////////////////////////////
// Implementation helpers

void CPropPageFrameDefault::FillGradientRectH(CDC *pDc, const RECT &rect, COLORREF clrLeft, COLORREF clrRight)
{
	// pre calculation
	int	nSteps = rect.right-rect.left;
	int	nRRange = GetRValue(clrRight)-GetRValue(clrLeft);
	int	nGRange = GetGValue(clrRight)-GetGValue(clrLeft);
	int	nBRange = GetBValue(clrRight)-GetBValue(clrLeft);

	double	dRStep = static_cast<double>(nRRange) / static_cast<double>(nSteps);
	double	dGStep = static_cast<double>(nGRange) / static_cast<double>(nSteps);
	double	dBStep = static_cast<double>(nBRange) / static_cast<double>(nSteps);

	double	dR = static_cast<double>(GetRValue(clrLeft));
	double	dG = static_cast<double>(GetGValue(clrLeft));
	double	dB = static_cast<double>(GetBValue(clrLeft));

	CPen* pPrevPen = nullptr;
	for (int x = rect.left; x <= rect.right; ++x)
	{
		CPen Pen(PS_SOLID, 1, RGB(static_cast<BYTE>(dR), static_cast<BYTE>(dG), static_cast<BYTE>(dB)));
		pPrevPen = pDc->SelectObject(&Pen);
		pDc->MoveTo(x, rect.top);
		pDc->LineTo(x, rect.bottom);
		pDc->SelectObject(pPrevPen);

		dR+= dRStep;
		dG+= dGStep;
		dB+= dBStep;
	}
}


/////////////////////////////////////////////////////////////////////
// message handlers

void CPropPageFrameDefault::OnPaint()
{
	CPaintDC dc(this);
	Draw(&dc);
}


BOOL CPropPageFrameDefault::OnEraseBkgnd(CDC* pDC)
{
	if (IsAppThemed())
	{
		HTHEME hTheme = OpenThemeData(m_hWnd, L"TREEVIEW");
		if (hTheme)
		{
			CRect	rect;
			GetClientRect(rect);
			DrawThemeBackground(hTheme, pDC->m_hDC, 0, 0, rect, nullptr);
			CloseThemeData(hTheme);
		}
		return TRUE;
	}
	else
	{
		return CWnd::OnEraseBkgnd(pDC);
	}
}



} //namespace TreePropSheet
