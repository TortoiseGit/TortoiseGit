// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng

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
 * \ingroup Utils
 * Helper class to show a tooltip for e.g. a scrollbar while it is dragged.
 * Example use:
 * In the OnVScroll() message handler
 * \code
 * switch (nSBCode)
 * {
 *   case SB_THUMBPOSITION:
 *    m_ScrollTool.Clear();
 *    break;
 *   case SB_THUMBTRACK:
 *    m_ScrollTool.Init(&thumbpoint);
 *    m_ScrollTool.SetText(&thumbpoint, L"Line: %*ld", maxchars, nTrackPos);
 *    break;
 * }
 *
 */
class CScrollTool : public CWnd
{
public:
	CScrollTool();

public:
	/**
	 * Initializes the tooltip control.
	 * \param pos the position in screen coordinates where the tooltip should be shown
	 * \param bRightAligned if set to true, the tooltip is right aligned with pos,
	 *        depending on the text width shown in the tooltip
	 */
	bool Init(LPPOINT pos, bool bRightAligned = false);
	/**
	 * Sets the text which should be shown in the tooltip.
	 * \param pos the position in screen coordinates where the tooltip should be shown
	 * \fmt a format string
	 */
	void SetText(LPPOINT pos, const TCHAR * fmt, ...);
	/**
	 * Removes the tooltip control.
	 */
	void Clear();
	/**
	 * Returns the width of \c szText in pixels for the tooltip control
	 */
	LONG GetTextWidth(LPCTSTR szText);

	virtual ~CScrollTool();

protected:
	DECLARE_MESSAGE_MAP()
private:
	TOOLINFO ti;
	bool m_bInitCalled;
	bool m_bRightAligned;
};
