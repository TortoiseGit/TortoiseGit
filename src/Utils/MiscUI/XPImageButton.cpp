#include "stdafx.h"
#include "XPTheme.h"
#include "XPImageButton.h"


IMPLEMENT_DYNAMIC(CXPImageButton, CButton)
CXPImageButton::CXPImageButton()
{
}

CXPImageButton::~CXPImageButton()
{
}

BEGIN_MESSAGE_MAP(CXPImageButton, CButton)
	ON_NOTIFY_REFLECT (NM_CUSTOMDRAW, OnNotifyCustomDraw)
END_MESSAGE_MAP()

void CXPImageButton::OnNotifyCustomDraw(NMHDR * pNotifyStruct, LRESULT* result)
{
	LPNMCUSTOMDRAW pCustomDraw = (LPNMCUSTOMDRAW)pNotifyStruct;
	ASSERT(pCustomDraw->hdr.hwndFrom == m_hWnd);

	DWORD style = GetStyle();
	if ((style & (BS_BITMAP | BS_ICON)) == 0 || !m_xpButton.IsAppThemed() || !m_xpButton.IsActive())
	{
		// not an icon or bitmap. So just let windows handle its drawing.
		*result = CDRF_DODEFAULT;
		return;
	}

	if (pCustomDraw->dwDrawStage == CDDS_PREERASE)
	{
		m_xpButton.DrawParentBackground(m_hWnd, pCustomDraw->hdc, &pCustomDraw->rc);
	}

	if (pCustomDraw->dwDrawStage == CDDS_PREERASE || pCustomDraw->dwDrawStage == CDDS_PREPAINT)
	{
		m_xpButton.Open(m_hWnd, L"BUTTON");
		if (!m_xpButton.IsAppThemed() || !m_xpButton.IsActive())
		{
			*result = CDRF_DODEFAULT;
			return;
		}

		// find the state for DrawThemeBackground()
		int state_id = PBS_NORMAL;
		if (style & WS_DISABLED)
			state_id = PBS_DISABLED;
		else if (pCustomDraw->uItemState & CDIS_SELECTED)
			state_id = PBS_PRESSED;
		else if (pCustomDraw->uItemState & CDIS_HOT)
			state_id = PBS_HOT;
		else if (style & BS_DEFPUSHBUTTON)
			state_id = PBS_DEFAULTED;

		// draw themed button background according to the button state
		m_xpButton.DrawBackground(pCustomDraw->hdc, BP_PUSHBUTTON, state_id, &pCustomDraw->rc, NULL);

		CRect content_rect (pCustomDraw->rc); 
		m_xpButton.GetBackgroundContentRect(pCustomDraw->hdc, BP_PUSHBUTTON, state_id, &pCustomDraw->rc, &content_rect);
		m_xpButton.Close();

		// draw the image
		if (style & BS_BITMAP)
		{
			DrawBitmap(pCustomDraw->hdc, &content_rect, style);
		}
		else
		{
			ASSERT (style & BS_ICON);
			DrawIcon(pCustomDraw->hdc, &content_rect, style);
		}

		// draw the focus rectangle if needed
		if (pCustomDraw->uItemState & CDIS_FOCUS)
		{
			// draw focus rectangle
			DrawFocusRect(pCustomDraw->hdc, &content_rect);
		}

		*result = CDRF_SKIPDEFAULT;
		return;
	}

	ASSERT (false);
	*result = CDRF_DODEFAULT;
}

void CXPImageButton::DrawBitmap(HDC hDC, const CRect& Rect, DWORD style)
{
	HBITMAP hBitmap = GetBitmap();
	if (hBitmap == NULL)
		return;

	// determine the size of the bitmap image
	BITMAPINFO bmi;
	memset (&bmi, 0, sizeof(BITMAPINFO));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	GetDIBits(hDC, hBitmap, 0, 0, NULL, &bmi, DIB_RGB_COLORS);

	// determine position of top-left corner of bitmap
	int x = ImageLeft(bmi.bmiHeader.biWidth, Rect, style);
	int y = ImageTop(bmi.bmiHeader.biHeight, Rect, style);

	// Draw the bitmap
	DrawState(hDC, NULL, NULL, (LPARAM) hBitmap, 0, x, y, bmi.bmiHeader.biWidth, bmi.bmiHeader.biHeight,
			(style & WS_DISABLED) != 0 ? (DST_BITMAP | DSS_DISABLED) : (DST_BITMAP | DSS_NORMAL));
}

void CXPImageButton::DrawIcon(HDC hDC, const CRect& Rect, DWORD style)
{
	HICON hIcon = GetIcon();
	if (hIcon == NULL)
		return;

	// determine the size of the icon
	ICONINFO ii;
	GetIconInfo(hIcon, &ii);
	BITMAPINFO bmi;
	memset(&bmi, 0, sizeof(BITMAPINFO));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	int cx = 0;
	int cy = 0;
	if (ii.hbmColor != NULL)
	{
		GetDIBits(hDC, ii.hbmColor, 0, 0, NULL, &bmi, DIB_RGB_COLORS);
		cx = bmi.bmiHeader.biWidth;
		cy = bmi.bmiHeader.biHeight;
	}
	else
	{
		GetDIBits(hDC, ii.hbmMask, 0, 0, NULL, &bmi, DIB_RGB_COLORS);
		cx = bmi.bmiHeader.biWidth;
		cy = bmi.bmiHeader.biHeight/2;
	}

	// determine the position of the top-left corner of the icon
	int x = ImageLeft (cx, Rect, style);
	int y = ImageTop (cy, Rect, style);

	// Draw the icon
	DrawState(hDC, NULL, NULL, (LPARAM) hIcon, 0, x, y, cx, cy,
		(style & WS_DISABLED) != 0 ? (DST_ICON | DSS_DISABLED) : (DST_ICON | DSS_NORMAL));
	if (ii.hbmColor) 
		DeleteObject(ii.hbmColor);
	if (ii.hbmMask) 
		DeleteObject(ii.hbmMask);
}

int CXPImageButton::ImageLeft(int cx, const CRect& Rect, DWORD style) const
{
	// calculate the left position of the image so it is drawn on left, right or centered (the default)
	// as set by the style settings.
	int x = Rect.left;
	if (cx > Rect.Width())
		cx = Rect.Width();
	else if ((style & BS_CENTER) == BS_LEFT)
		x = Rect.left;
	else if ((style & BS_CENTER) == BS_RIGHT)
		x = Rect.right - cx;
	else
		x = Rect.left + (Rect.Width() - cx)/2;
	return x;
}

int CXPImageButton::ImageTop(int cy, const CRect& Rect, DWORD style) const
{
	// calculate the top position of the image so it is drawn on top, bottom or vertically centered (the default)
	// as set by the style settings.
	int y = Rect.top;
	if (cy > Rect.Height())
		cy = Rect.Height();
	if ((style & BS_VCENTER) == BS_TOP)
		y = Rect.top;
	else if ((style & BS_VCENTER) == BS_BOTTOM)
		y = Rect.bottom - cy;
	else
		y = Rect.top + (Rect.Height() - cy)/2;
	return y;
}
