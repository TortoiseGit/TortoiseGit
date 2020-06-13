// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2020 - TortoiseGit
// Copyright (C) 2020 - TortoiseSVN

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
#include "ThemeControls.h"
#include "Theme.h"
#include "DPIAware.h"

static const int nImageHorzMargin = 10;
static const int nVertMargin = 5;
static const COLORREF clrDefault = (COLORREF)-1;

void CThemeMFCButton::OnFillBackground(CDC* pDC, const CRect& rectClient)
{
	if (CTheme::Instance().IsDarkTheme())
	{
		if (IsPressed())
			pDC->FillSolidRect(rectClient, RGB(102, 102, 102));
		else if (m_bHighlighted)
			pDC->FillSolidRect(rectClient, RGB(69, 69, 69));
		else
			pDC->FillSolidRect(rectClient, RGB(51, 51, 51));
	}
	else
		__super::OnFillBackground(pDC, rectClient);
}

void CThemeMFCButton::OnDrawBorder(CDC* pDC, CRect& rectClient, UINT uiState)
{
	if (!CTheme::Instance().IsDarkTheme())
		__super::OnDrawBorder(pDC, rectClient, uiState);
	else
	{
		pDC->FillSolidRect(rectClient, RGB(155, 155, 155));
		rectClient.DeflateRect(1, 1);
	}
}

void CThemeMFCButton::OnDraw(CDC* pDC, const CRect& rect, UINT uiState)
{
	if (!CTheme::Instance().IsDarkTheme())
		return __super::OnDraw(pDC, rect, uiState);

	if (IsPressed())
		pDC->FillSolidRect(rect, RGB(102, 102, 102));
	else if (m_bHighlighted)
		pDC->FillSolidRect(rect, RGB(69, 69, 69));
	else
		pDC->FillSolidRect(rect, RGB(51, 51, 51));

	CRect rectText = rect;
	CRect rectImage = rect;

	CString strText;
	GetWindowText(strText);

	if (m_sizeImage.cx != 0)
	{
		if (!strText.IsEmpty())
		{
			if (m_bTopImage)
			{
				rectImage.bottom = rectImage.top + m_sizeImage.cy + GetVertMargin();
				rectText.top = rectImage.bottom;
				rectText.bottom -= GetVertMargin();
			}
			else if (m_bRightImage)
			{
				rectText.right -= m_sizeImage.cx + GetImageHorzMargin() / 2;
				rectImage.left = rectText.right;
				rectImage.right -= GetImageHorzMargin() / 2;
			}
			else
			{
				rectText.left += m_sizeImage.cx + GetImageHorzMargin() / 2;
				rectImage.left += GetImageHorzMargin() / 2;
				rectImage.right = rectText.left;
			}
		}

		// Center image:
		rectImage.DeflateRect((rectImage.Width() - m_sizeImage.cx) / 2, std::max(0L, (rectImage.Height() - m_sizeImage.cy) / 2));
	}
	else
	{
		rectImage.SetRectEmpty();
	}

	// Draw text:
	CFont* pOldFont = SelectFont(pDC);
	ENSURE(pOldFont != nullptr);

	pDC->SetBkMode(TRANSPARENT);
	COLORREF clrText = m_clrRegular == clrDefault ? GetGlobalData()->clrBtnText : m_clrRegular;

	if (m_bHighlighted && m_clrHover != clrDefault)
		clrText = m_clrHover;

	UINT uiDTFlags = DT_END_ELLIPSIS;
	BOOL bIsSingleLine = FALSE;

	if (strText.Find(_T('\n')) < 0)
	{
		uiDTFlags |= DT_VCENTER | DT_SINGLELINE;
		bIsSingleLine = TRUE;
	}
	else
		rectText.DeflateRect(0, GetVertMargin() / 2);

	switch (m_nAlignStyle)
	{
	case ALIGN_LEFT:
		uiDTFlags |= DT_LEFT;
		rectText.left += GetImageHorzMargin() / 2;
		break;

	case ALIGN_RIGHT:
		uiDTFlags |= DT_RIGHT;
		rectText.right -= GetImageHorzMargin() / 2;
		break;

	case ALIGN_CENTER:
		uiDTFlags |= DT_CENTER;
	}

	if (GetExStyle() & WS_EX_LAYOUTRTL)
		uiDTFlags |= DT_RTLREADING;

	if ((uiState & ODS_DISABLED) && m_bGrayDisabled)
	{
		pDC->SetTextColor(GetGlobalData()->clrBtnHilite);

		CRect rectShft = rectText;
		rectShft.OffsetRect(1, 1);
		OnDrawText(pDC, rectShft, strText, uiDTFlags, uiState);

		clrText = GetGlobalData()->clrGrayedText;
	}

	clrText = CTheme::Instance().GetThemeColor(clrText);
	pDC->SetTextColor(clrText);

	if (m_bDelayFullTextTooltipSet)
	{
		BOOL bIsFullText = pDC->GetTextExtent(strText).cx <= rectText.Width();
		SetTooltip(bIsFullText || !bIsSingleLine ? nullptr : static_cast<LPCTSTR>(strText));
		m_bDelayFullTextTooltipSet = FALSE;
	}

	OnDrawText(pDC, rectText, strText, uiDTFlags, uiState);

	// Draw image:
	if (!rectImage.IsRectEmpty())
	{
		if (m_nStdImageId != (CMenuImages::IMAGES_IDS)-1)
		{
			CMenuImages::IMAGES_IDS id = m_nStdImageId;

			if ((uiState & ODS_DISABLED) && m_bGrayDisabled && m_nStdImageDisabledId != 0)
				id = m_nStdImageDisabledId;

			CMenuImages::Draw(pDC, id, rectImage.TopLeft(), m_StdImageState);
		}
		else
		{
			BOOL bIsDisabled = (uiState & ODS_DISABLED) && m_bGrayDisabled;

			CMFCToolBarImages& imageChecked = (bIsDisabled && m_ImageCheckedDisabled.GetCount() != 0) ? m_ImageCheckedDisabled : (m_bHighlighted && m_ImageCheckedHot.GetCount() != 0) ? m_ImageCheckedHot : m_ImageChecked;

			CMFCToolBarImages& image = (bIsDisabled && m_ImageDisabled.GetCount() != 0) ? m_ImageDisabled : (m_bHighlighted && m_ImageHot.GetCount() != 0) ? m_ImageHot : m_Image;

			if (m_bChecked && imageChecked.GetCount() != 0)
			{
				CAfxDrawState ds;

				imageChecked.PrepareDrawImage(ds);
				imageChecked.Draw(pDC, rectImage.left, rectImage.top, 0, FALSE, bIsDisabled && m_ImageCheckedDisabled.GetCount() == 0);
				imageChecked.EndDrawImage(ds);
			}
			else if (image.GetCount() != 0)
			{
				CAfxDrawState ds;

				image.PrepareDrawImage(ds);
				image.Draw(pDC, rectImage.left, rectImage.top, 0, FALSE, bIsDisabled && m_ImageDisabled.GetCount() == 0);
				image.EndDrawImage(ds);
			}
		}
	}

	pDC->SelectObject(pOldFont);
}

void CThemeMFCMenuButton::OnDraw(CDC* pDC, const CRect& rect, UINT uiState)
{
	ASSERT_VALID(pDC);

	CSize sizeArrow = CMenuImages::Size();

	CRect rectParent = rect;
	rectParent.right -= sizeArrow.cx + nImageHorzMargin;

	OnButtonDraw(pDC, rectParent, uiState);

	CRect rectArrow = rect;
	rectArrow.left = rectParent.right;

	if (CTheme::Instance().IsDarkTheme())
	{
		if (IsPressed())
			pDC->FillSolidRect(rectArrow, RGB(102, 102, 102));
		else if (m_bHighlighted)
			pDC->FillSolidRect(rectArrow, RGB(69, 69, 69));
		else
			pDC->FillSolidRect(rectArrow, RGB(51, 51, 51));

		auto hPen = CreatePen(PS_SOLID, CDPIAware::Instance().ScaleX(1), RGB(180, 180, 180));
		auto hOldPen = pDC->SelectObject(hPen);

		auto hBrush = CreateSolidBrush(RGB(255, 255, 255));
		auto hOldBrush = pDC->SelectObject(hBrush);

		auto vmargin = CDPIAware::Instance().ScaleX(6);
		auto hmargin = CDPIAware::Instance().ScaleY(3);
		if (!m_bNoArrow)
		{
			if (m_bRightArrow)
			{
				POINT vertices[] = { { rectArrow.left + hmargin, rectArrow.bottom - vmargin },
									 { rectArrow.left + hmargin, rectArrow.top + vmargin },
									 { rectArrow.right - hmargin, (rectArrow.top + rectArrow.bottom) / 2 } };
				pDC->Polygon(vertices, _countof(vertices));
			}
			else
			{
				POINT vertices[] = { { rectArrow.left + hmargin, rectArrow.top + vmargin },
									 { rectArrow.right - hmargin, rectArrow.top + vmargin },
									 { (rectArrow.left + rectArrow.right) / 2, rectArrow.bottom - vmargin } };
				pDC->Polygon(vertices, _countof(vertices));
			}
		}

		pDC->SelectObject(hOldBrush);
		DeleteObject(hBrush);

		pDC->SelectObject(hOldPen);
		DeleteObject(hPen);
	}
	else
	{
		CMenuImages::Draw(pDC, m_bRightArrow ? CMenuImages::IdArrowRightLarge : CMenuImages::IdArrowDownLarge,
						  rectArrow, (uiState & ODS_DISABLED) ? CMenuImages::ImageGray : CMenuImages::ImageBlack);
	}

	if (m_bDefaultClick && !m_bNoArrow)
	{
		//----------------
		// Draw separator:
		//----------------
		CRect rectSeparator = rectArrow;
		rectSeparator.right = rectSeparator.left + 2;
		rectSeparator.DeflateRect(0, 2);

		if (!m_bWinXPTheme || m_bDontUseWinXPTheme)
		{
			rectSeparator.left += m_sizePushOffset.cx;
			rectSeparator.top += m_sizePushOffset.cy;
		}

		pDC->Draw3dRect(rectSeparator, GetGlobalData()->clrBtnDkShadow, GetGlobalData()->clrBtnHilite);
	}
}

void CThemeMFCMenuButton::OnDrawFocusRect(CDC* pDC, const CRect& rectClient)
{
	__super::OnDrawFocusRect(pDC, rectClient);
}

void CThemeMFCMenuButton::OnDrawBorder(CDC* pDC, CRect& rectClient, UINT uiState)
{
	if (!CTheme::Instance().IsDarkTheme())
		__super::OnDrawBorder(pDC, rectClient, uiState);
	else
	{
		pDC->FillSolidRect(rectClient, RGB(155, 155, 155));
		rectClient.DeflateRect(1, 1);
	}
}

void CThemeMFCMenuButton::OnButtonDraw(CDC* pDC, const CRect& rect, UINT uiState)
{
	if (CTheme::Instance().IsDarkTheme())
	{
		if (IsPressed())
			pDC->FillSolidRect(rect, RGB(102, 102, 102));
		else if (m_bHighlighted)
			pDC->FillSolidRect(rect, RGB(69, 69, 69));
		else
			pDC->FillSolidRect(rect, RGB(51, 51, 51));
	}

	CRect rectText = rect;
	CRect rectImage = rect;

	CString strText;
	GetWindowText(strText);

	if (m_sizeImage.cx != 0)
	{
		if (!strText.IsEmpty())
		{
			if (m_bTopImage)
			{
				rectImage.bottom = rectImage.top + m_sizeImage.cy + GetVertMargin();
				rectText.top = rectImage.bottom;
				rectText.bottom -= GetVertMargin();
			}
			else if (m_bRightImage)
			{
				rectText.right -= m_sizeImage.cx + GetImageHorzMargin() / 2;
				rectImage.left = rectText.right;
				rectImage.right -= GetImageHorzMargin() / 2;
			}
			else
			{
				rectText.left += m_sizeImage.cx + GetImageHorzMargin() / 2;
				rectImage.left += GetImageHorzMargin() / 2;
				rectImage.right = rectText.left;
			}
		}

		// Center image:
		rectImage.DeflateRect((rectImage.Width() - m_sizeImage.cx) / 2, std::max(0L, (rectImage.Height() - m_sizeImage.cy) / 2));
	}
	else
		rectImage.SetRectEmpty();

	// Draw text:
	CFont* pOldFont = SelectFont(pDC);
	ENSURE(pOldFont != nullptr);

	pDC->SetBkMode(TRANSPARENT);
	COLORREF clrText = m_clrRegular == clrDefault ? GetGlobalData()->clrBtnText : m_clrRegular;

	if (m_bHighlighted && m_clrHover != clrDefault)
		clrText = m_clrHover;

	UINT uiDTFlags = DT_END_ELLIPSIS;
	BOOL bIsSingleLine = FALSE;

	if (strText.Find(_T('\n')) < 0)
	{
		uiDTFlags |= DT_VCENTER | DT_SINGLELINE;
		bIsSingleLine = TRUE;
	}
	else
		rectText.DeflateRect(0, GetVertMargin() / 2);

	switch (m_nAlignStyle)
	{
	case ALIGN_LEFT:
		uiDTFlags |= DT_LEFT;
		rectText.left += GetImageHorzMargin() / 2;
		break;

	case ALIGN_RIGHT:
		uiDTFlags |= DT_RIGHT;
		rectText.right -= GetImageHorzMargin() / 2;
		break;

	case ALIGN_CENTER:
		uiDTFlags |= DT_CENTER;
	}

	if (GetExStyle() & WS_EX_LAYOUTRTL)
		uiDTFlags |= DT_RTLREADING;

	if ((uiState & ODS_DISABLED) && m_bGrayDisabled)
	{
		pDC->SetTextColor(GetGlobalData()->clrBtnHilite);

		CRect rectShft = rectText;
		rectShft.OffsetRect(1, 1);
		OnDrawText(pDC, rectShft, strText, uiDTFlags, uiState);

		clrText = GetGlobalData()->clrGrayedText;
	}

	if (clrText != GetGlobalData()->clrBtnText || !CTheme::Instance().IsDarkTheme())
		clrText = CTheme::Instance().GetThemeColor(clrText);
	else
		clrText = CTheme::darkTextColor;
	pDC->SetTextColor(clrText);

	if (m_bDelayFullTextTooltipSet)
	{
		BOOL bIsFullText = pDC->GetTextExtent(strText).cx <= rectText.Width();
		SetTooltip(bIsFullText || !bIsSingleLine ? nullptr : static_cast<LPCTSTR>(strText));
		m_bDelayFullTextTooltipSet = FALSE;
	}

	OnDrawText(pDC, rectText, strText, uiDTFlags, uiState);

	// Draw image:
	if (!rectImage.IsRectEmpty())
	{
		if (m_nStdImageId != (CMenuImages::IMAGES_IDS)-1)
		{
			CMenuImages::IMAGES_IDS id = m_nStdImageId;

			if ((uiState & ODS_DISABLED) && m_bGrayDisabled && m_nStdImageDisabledId != 0)
				id = m_nStdImageDisabledId;

			CMenuImages::Draw(pDC, id, rectImage.TopLeft(), m_StdImageState);
		}
		else
		{
			BOOL bIsDisabled = (uiState & ODS_DISABLED) && m_bGrayDisabled;

			CMFCToolBarImages& imageChecked = (bIsDisabled && m_ImageCheckedDisabled.GetCount() != 0) ? m_ImageCheckedDisabled : (m_bHighlighted && m_ImageCheckedHot.GetCount() != 0) ? m_ImageCheckedHot : m_ImageChecked;

			CMFCToolBarImages& image = (bIsDisabled && m_ImageDisabled.GetCount() != 0) ? m_ImageDisabled : (m_bHighlighted && m_ImageHot.GetCount() != 0) ? m_ImageHot : m_Image;

			if (m_bChecked && imageChecked.GetCount() != 0)
			{
				CAfxDrawState ds;

				imageChecked.PrepareDrawImage(ds);
				imageChecked.Draw(pDC, rectImage.left, rectImage.top, 0, FALSE, bIsDisabled && m_ImageCheckedDisabled.GetCount() == 0);
				imageChecked.EndDrawImage(ds);
			}
			else if (image.GetCount() != 0)
			{
				CAfxDrawState ds;

				image.PrepareDrawImage(ds);
				image.Draw(pDC, rectImage.left, rectImage.top, 0, FALSE, bIsDisabled && m_ImageDisabled.GetCount() == 0);
				image.EndDrawImage(ds);
			}
		}
	}

	pDC->SelectObject(pOldFont);
}
