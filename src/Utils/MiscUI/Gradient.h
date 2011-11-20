// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2007 - TortoiseSVN

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
 * Class for drawing gradients.
 * There are two methods for each gradient. One using standard API function and the other using GDI.
 * Since not all systems (e.g. WinCE and the like) don't have GDI you must define USE_GDI_GRADIENT to
 * use those.
 */
class CGradient
{
public:
	CGradient(void);
	~CGradient(void);

	/**
	 * Fills a rectangle with a two color gradient.
	 * \param pDC the device context to draw on
	 * \param rect the rectangle to fill
	 * \param colorStart the starting color. This is either the color used on the left (if bHorz == TRUE) or top
	 * \param colorEnd the ending color. This is either the color used on the right (if bHorz == TRUE) or bottom
	 * \param bHorz if TRUE then the gradient is drawn from left to right, otherwise from top to bottom
	 * \param nSteps the steps the gradient shall have. The more the smoother the gradient will be but also slower.
	 */
	static void Draw(CDC * pDC, CRect rect, COLORREF colorStart, COLORREF colorEnd, BOOL bHorz = TRUE, UINT nSteps = 64);
	/**
	 * Fills a rectangle with a three color gradient.
	 * \param pDC the device context to draw on
	 * \param rect the rectangle to fill
	 * \param colorStart the starting color. This is either the color used on the left (if bHorz == TRUE) or top
	 * \param colorMid the middle color.
	 * \param colorEnd the ending color. This is either the color used on the right (if bHorz == TRUE) or bottom
	 * \param bHorz if TRUE then the gradient is drawn from left to right, otherwise from top to bottom
	 * \param nSteps the steps the gradient shall have. The more the smoother the gradient will be but also slower.
	 */
	static void Draw(CDC * pDC, CRect rect, COLORREF colorStart, COLORREF colorMid, COLORREF colorFinish, BOOL bHorz = TRUE, UINT nSteps = 64);
#ifdef USE_GDI_GRADIENT
	/**
	 * Fills a rectangle with a two color gradient.
	 * \param pDC the device context to draw on
	 * \param rect the rectangle to fill
	 * \param colorStart the starting color. This is either the color used on the left (if bHorz == TRUE) or top
	 * \param colorEnd the ending color. This is either the color used on the right (if bHorz == TRUE) or bottom
	 * \param bHorz if TRUE then the gradient is drawn from left to right, otherwise from top to bottom
	 */
	static void DrawGDI(CDC * pDC, CRect rect, COLORREF colorStart, COLORREF colorEnd, BOOL bHorz = TRUE);
	/**
	 * Fills a rectangle with a three color gradient.
	 * \param pDC the device context to draw on
	 * \param rect the rectangle to fill
	 * \param colorStart the starting color. This is either the color used on the left (if bHorz == TRUE) or top
	 * \param colorMid the middle color.
	 * \param colorEnd the ending color. This is either the color used on the right (if bHorz == TRUE) or bottom
	 * \param bHorz if TRUE then the gradient is drawn from left to right, otherwise from top to bottom
	 */
	static void DrawGDI(CDC * pDC, CRect rect, COLORREF colorStart, COLORREF colorMid, COLORREF colorEnd, BOOL bHorz = TRUE);
#endif
private:
	static void SplitRect(const CRect& rSource, CRect& rHalf1, CRect& rHalf2, BOOL bHorz);
};
