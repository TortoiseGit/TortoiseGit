// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2010-2011, 2014 - TortoiseSVN

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
	CAutoFile hFile = CreateFile(path, GENERIC_WRITE, FILE_SHARE_DELETE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (!hFile)
		return false;

	CStringA header;
	header.Format("<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"%d\" height=\"%d\" viewBox=\"0 0 %d %d\">\r\n", viewportWidth, viewportHeight, viewportWidth, viewportHeight);
	CStringA footer = "\r\n</svg>";

	if (!WriteFile(hFile, header, static_cast<DWORD>(header.GetLength()), &dwWritten, nullptr))
		return false;

	for (const auto& object : objects)
	{
		if (!WriteFile(hFile, object, static_cast<DWORD>(object.GetLength()), &dwWritten, nullptr))
			return false;
		if (!WriteFile(hFile, "\r\n", DWORD(2), &dwWritten, nullptr))
			return false;
	}
	if (!WriteFile(hFile, footer, static_cast<DWORD>(footer.GetLength()), &dwWritten, nullptr))
		return false;

	return true;
}


void SVG::RoundedRectangle( int x, int y, int width, int height, Gdiplus::Color stroke, int penWidth, Gdiplus::Color fill, int radius /*= 0*/, int mode )
{
	CStringA sObj,tmp;
	if(mode == 3)
	{
		sObj.Format("<rect x=\"%d\" y=\"%d\" height=\"%d\" width=\"%d\" rx=\"%d\" ry=\"%d\" style=\"stroke:#%06lx; stroke-width:%d; fill: #%06lx\"/>",
			x, y, height, width, radius, radius, GetColor(stroke), penWidth, GetColor(fill));
	}else
	{
		sObj += "<path d=\"";
		if(mode & 0x1)
		{
			tmp.Format("M %d %d a %d %d 0 0 1 %d %d ", x, y+radius, radius, radius, radius, -radius);
			sObj += tmp;
			tmp.Format("h %d ", width - 2*radius);
			sObj += tmp;
			tmp.Format("a %d %d 0 0 1 %d %d ", radius, radius, radius, radius);
			sObj += tmp;
		}else
		{
			tmp.Format("M %d %d h %d ",x, y, width);
			sObj += tmp;
		}

		if(mode & 0x2)
		{
			tmp.Format("V %d a %d %d 0 0 1 %d %d ", y+height-radius, radius,radius, -radius, radius);
			sObj += tmp;
			tmp.Format("h %d a %d %d 0 0 1 %d %d z ", - width + 2* radius, radius,radius, -radius, -radius);
			sObj += tmp;
		}else
		{
			tmp.Format("V %d h %d z ", y+height, -width);
			sObj += tmp;
		}
		sObj += L"\" ";
		tmp.Format("style=\"stroke:#%06lx; stroke-width:%d; fill: #%06lx\"/>", GetColor(stroke), penWidth, GetColor(fill));
		sObj += tmp;
	}
	objects.push_back(sObj);
}

void SVG::Polygon( const Gdiplus::PointF * points, int numPoints, Gdiplus::Color stroke, int penWidth,  Gdiplus::Color fill )
{
	CStringA pointstring;
	CStringA sTemp;
	for (int i = 0; i < numPoints; ++i)
	{
		sTemp.Format("%d,%d ", static_cast<int>(points[i].X), static_cast<int>(points[i].Y));
		pointstring += sTemp;
	}
	pointstring.TrimRight();

	CStringA sObj;
	sObj.Format("<polygon points=\"%s\" style=\"stroke:#%06lx; stroke-width:%d; fill:#%06lx;\"/>",
		static_cast<LPCSTR>(pointstring), GetColor(stroke), penWidth, GetColor(fill));

	objects.push_back(sObj);
}

void SVG::DrawPath( const Gdiplus::PointF * points, int numPoints, Gdiplus::Color stroke, int penWidth,  Gdiplus::Color fill )
{
	CStringA pointstring;
	CStringA sTemp;
	for (int i = 0; i < numPoints; ++i)
	{
		sTemp.Format("%c %d %d ", i==0? 'M':'L', static_cast<int>(points[i].X), static_cast<int>(points[i].Y));
		pointstring += sTemp;
	}
	pointstring.TrimRight();

	CStringA sObj;
	sObj.Format("<path d=\"%s\" style=\"stroke:#%06lx; stroke-width:%d; fill:#%06lx;\"/>",
		static_cast<LPCSTR>(pointstring), GetColor(stroke), penWidth, GetColor(fill));

	objects.push_back(sObj);
}

void SVG::Polyline( const Gdiplus::PointF * points, int numPoints, Gdiplus::Color stroke, int penWidth)
{
	CStringA pointstring;
	CStringA sTemp;
	for (int i = 0; i < numPoints; ++i)
	{
		sTemp.Format("%d,%d ", static_cast<int>(points[i].X), static_cast<int>(points[i].Y));
		pointstring += sTemp;
	}
	pointstring.TrimRight();

	CStringA sObj;
	sObj.Format("<polyline points=\"%s\" style=\"stroke:#%06lx; stroke-width:%d; fill:none;\"/>",
		static_cast<LPCSTR>(pointstring), GetColor(stroke), penWidth);

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
		static_cast<int>(objects.size()), GetColor(topColor), GetColor(bottomColor), x, y, height, width, GetColor(stroke), static_cast<int>(objects.size()));

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

void SVG::Text( int x, int y, LPCSTR font, int fontsize, bool italic, bool bold, Gdiplus::Color color, LPCSTR text , int al)
{
	CStringA sObj;
	sObj.Format("<text x=\"%d\"  y=\"%d\" \
style=\"font-family:%s;\
font-size:%dpt;\
font-style:%s;\
font-weight:%s;\
stroke:none;\
text-anchor: %s;\
fill:#%06lx;\">%s</text>",
		x, y, font, fontsize, italic ? "italic" : "none", bold ? "bold" : "none",
		al==SVG::left? "left": al==SVG::middle? "middle": "right",
		GetColor(color),text);

	objects.push_back(sObj);
}

DWORD SVG::GetColor( Gdiplus::Color c ) const
{
	return (static_cast<DWORD>(c.GetRed()) << 16) | (static_cast<DWORD>(c.GetGreen()) << 8) | static_cast<DWORD>(c.GetBlue());
}
