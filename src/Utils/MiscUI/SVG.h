// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2010 - TortoiseSVN

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
#include <GdiPlus.h>
#include <vector>

/**
 * Class to create an svg file.
 * Provides method to add svg primitives like lines, rectangles, text.
 *
 * \remark this class is specifically tailored to the TSVN revision graph.
 * That's why the color params are Gdiplus::Color and not simple COLORREFs,
 * and also the styles and attributes of the used primitives are set
 * the way it's required for the revision graph.
 */
class SVG
{
public:
	SVG();
	virtual ~SVG();

	bool Save(const CString& path);

	void StartGroup() { objects.push_back("<g>"); }
	void EndGroup() { objects.push_back("</g>"); }
	void SetViewSize(int w, int h) { viewportWidth = w; viewportHeight = h; }
	void RoundedRectangle(int x, int y, int width, int height, Gdiplus::Color stroke, int penWidth, Gdiplus::Color fill, int radius = 0);
	void Polygon(const Gdiplus::PointF * points, int numPoints, Gdiplus::Color stroke, int penWidth, Gdiplus::Color fill);
	void GradientRectangle(int x, int y, int width, int height, Gdiplus::Color topColor, Gdiplus::Color bottomColor, Gdiplus::Color stroke);
	void PolyBezier(const POINT * points, int numPoints, Gdiplus::Color stroke);
	void Ellipse(int x, int y, int width, int height, Gdiplus::Color stroke, int penWidth, Gdiplus::Color fill);
	void CenteredText(int x, int y, LPCSTR font, int fontsize, bool italic, bool bold, Gdiplus::Color color, LPCSTR text);
private:
	DWORD GetColor(Gdiplus::Color c);

	std::vector<CStringA>   objects;
	int					 viewportWidth;
	int					 viewportHeight;
};
