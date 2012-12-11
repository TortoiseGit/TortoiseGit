// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2010-2011 - TortoiseSVN

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
#include "SVG.h"
#include "SmartHandle.h"

SVG::SVG()
	: viewportWidth(1000)
	, viewportHeight(1000)
{
}

SVG::~SVG()
{
}


bool SVG::Save( const CString& path )
{
	DWORD dwWritten = 0;
	CAutoFile hFile = CreateFile(path, GENERIC_WRITE, FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (!hFile)
		return false;

	CStringA header;
	header.Format("<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"%d\" height=\"%d\" viewBox=\"0 0 %d %d\">\r\n", viewportWidth, viewportHeight, viewportWidth, viewportHeight);
	CStringA footer = "\r\n</svg>";

	if (!WriteFile(hFile, header, (DWORD)header.GetLength(), &dwWritten, NULL))
		return false;

	for (std::vector<CStringA>::const_iterator it = objects.begin(); it != objects.end(); ++it)
	{
		if (!WriteFile(hFile, *it, (DWORD)it->GetLength(), &dwWritten, NULL))
			return false;
		if (!WriteFile(hFile, "\r\n", (DWORD)2, &dwWritten, NULL))
			return false;
	}
	if (!WriteFile(hFile, footer, (DWORD)footer.GetLength(), &dwWritten, NULL))
		return false;

	return true;
}


void SVG::RoundedRectangle( int x, int y, int width, int height, Gdiplus::Color stroke, int penWidth, Gdiplus::Color fill, int radius /*= 0*/ )
{
	CStringA sObj;
	sObj.Format("<rect x=\"%d\" y=\"%d\" height=\"%d\" width=\"%d\" rx=\"%d\" ry=\"%d\" style=\"stroke:#%06lx; stroke-width:%d; fill: #%06lx\"/>",
		x, y, height, width, radius, radius, GetColor(stroke), penWidth, GetColor(fill));

	objects.push_back(sObj);
}

void SVG::Polygon( const Gdiplus::PointF * points, int numPoints, Gdiplus::Color stroke, int penWidth,  Gdiplus::Color fill )
{
	CStringA pointstring;
	CStringA sTemp;
	for (int i = 0; i < numPoints; ++i)
	{
		sTemp.Format("%d,%d ", (int)points[i].X, (int)points[i].Y);
		pointstring += sTemp;
	}
	pointstring.TrimRight();

	CStringA sObj;
	sObj.Format("<polygon points=\"%s\" style=\"stroke:#%06lx; stroke-width:%d; fill:#%06lx;\"/>",
		(LPCSTR)pointstring, GetColor(stroke), penWidth, GetColor(fill));

	objects.push_back(sObj);
}

void SVG::GradientRectangle( int x, int y, int width, int height, Gdiplus::Color topColor, Gdiplus::Color bottomColor, Gdiplus::Color stroke )
{
	CStringA sObj;
	sObj.Format(
		"<g>\
<defs>\
<linearGradient id=\"linearGradient%d\" \
x1=\"0%%\" y1=\"0%%\" \
x2=\"0%%\" y2=\"100%%\" \
spreadMethod=\"pad\">\
<stop offset=\"0%%\" stop-color=\"#%06lx\" stop-opacity=\"1\"/>\
<stop offset=\"100%%\" stop-color=\"#%06lx\" stop-opacity=\"1\"/>\
</linearGradient>\
</defs>\
<rect x=\"%d\" y=\"%d\" height=\"%d\" width=\"%d\" style=\"stroke:#%06lx; fill::url(#linearGradient%d)\"/>\
</g>",
		(int)objects.size(), GetColor(topColor), GetColor(bottomColor), x, y, height, width, GetColor(stroke), (int)objects.size());

	objects.push_back(sObj);
}

void SVG::PolyBezier( const POINT * points, int numPoints, Gdiplus::Color stroke )
{
	if (numPoints == 0)
		return;

	CStringA pointstring;
	CStringA sTemp;
	sTemp.Format("M%d,%d ", points[0].x, points[0].y);
	pointstring += sTemp;

	for (int i = 1; i < numPoints; i += 3)
	{
		sTemp.Format("C%d,%d %d,%d %d,%d", points[i].x, points[i].y, points[i+1].x, points[i+1].y, points[i+2].x, points[i+2].y);
		pointstring += sTemp;
	}
	pointstring.TrimRight();

	CStringA sObj;
	sObj.Format("<path d=\"%s\" style=\"stroke:#%06lx; fill:none;\"/>",
		(LPCSTR)pointstring, GetColor(stroke));

	objects.push_back(sObj);
}

void SVG::Ellipse( int x, int y, int width, int height, Gdiplus::Color stroke, int penWidth, Gdiplus::Color fill )
{
	int cx = x + width/2;
	int cy = y + height/2;
	int rx = width/2;
	int ry = height/2;
	CStringA sObj;
	sObj.Format("<ellipse  cx=\"%d\" cy=\"%d\" rx=\"%d\" ry=\"%d\" style=\"stroke:#%06lx; stroke-width:%d; fill: #%06lx\"/>",
		cx, cy, rx, ry, GetColor(stroke), penWidth, GetColor(fill));

	objects.push_back(sObj);
}

void SVG::CenteredText( int x, int y, LPCSTR font, int fontsize, bool italic, bool bold, Gdiplus::Color color, LPCSTR text )
{
	CStringA sObj;
	sObj.Format("<text x=\"%d\"  y=\"%d\" \
style=\"font-family:%s;\
font-size:%dpt;\
font-style:%s;\
font-weight:%s;\
stroke:none;\
text-anchor: middle;\
fill:#%06lx;\">%s</text>",
		x, y, font, fontsize, italic ? "italic" : "none", bold ? "bold" : "none", GetColor(color), text);

	objects.push_back(sObj);
}

DWORD SVG::GetColor( Gdiplus::Color c )
{
	return ((DWORD)c.GetRed() << 16) | ((DWORD)c.GetGreen() << 8) | ((DWORD)c.GetBlue());
}

