// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013 - TortoiseGit

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

class Graphviz
{
public:
	void DrawNode(CString id, CString text, CString fontName, int fontSize, Gdiplus::Color borderColor, Gdiplus::Color backColor, int height);

	void BeginDrawTableNode(CString id, CString fontName, int fontSize, int height);

	void DrawTableNode(CString text, Gdiplus::Color backColor);

	void EndDrawTableNode();

	void DrawEdge(CString from, CString to);

	bool Save(const CString &path);

	CString m_defaultFontName;
	int m_defaultFontSize;
	Gdiplus::Color m_defaultBackColor;
	int m_defaultHeight;
	int m_tableNodeNum;

private:
	CString content;
};
