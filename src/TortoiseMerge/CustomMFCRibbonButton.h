// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2013 - TortoiseSVN

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

/**
 * \ingroup TortoiseMerge
 * a custom MFC Ribbon Button to avoid the click delay
 */

class CCustomMFCRibbonButton : public CMFCRibbonButton
{
public:
	CCustomMFCRibbonButton() : CMFCRibbonButton() {}
	CCustomMFCRibbonButton(UINT nID, LPCTSTR lpszText, int nSmallImageIndex = -1, int nLargeImageIndex = -1, BOOL bAlwaysShowDescription = FALSE)
		: CMFCRibbonButton(nID, lpszText, nSmallImageIndex, nLargeImageIndex, bAlwaysShowDescription) {}
	CCustomMFCRibbonButton(UINT nID, LPCTSTR lpszText, HICON hIcon, BOOL bAlwaysShowDescription = FALSE, HICON hIconSmall = nullptr, BOOL bAutoDestroyIcon = FALSE, BOOL bAlphaBlendIcon = FALSE)
		: CMFCRibbonButton(nID, lpszText, hIcon, bAlwaysShowDescription, hIconSmall, bAutoDestroyIcon, bAlphaBlendIcon) {}

protected:
	// override the OnLButtonUp method because the original one
	// does nothing if the button is still marked as pressed or highlighted:
	// that behavior prevents fast clicks because after each click
	// the window first needs to process various messages until it clears
	// those pressed and highlighted flags.
	virtual void OnLButtonUp(CPoint point)
	{
		ASSERT_VALID(this);

		CMFCRibbonBaseElement::OnLButtonUp(point);

		if (m_bIsDisabled)
		{
			return;
		}

		if (m_bIsDroppedDown)
		{
			if (!m_rectCommand.IsRectEmpty () && m_rectCommand.PtInRect (point) && IsMenuMode ())
			{
				OnClick (point);
			}
			return;
		}

		if (!m_rectCommand.IsRectEmpty() && !m_rectCommand.PtInRect(point))
		{
			return;
		}

		OnClick(point);
	}
};
