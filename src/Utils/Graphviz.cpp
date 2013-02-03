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
#include "stdafx.h"
#include "Graphviz.h"
#include "UnicodeUtils.h"
#include "SmartHandle.h"

void Graphviz::DrawNode(CString id, CString text, CString fontName, int fontSize, Gdiplus::Color /*borderColor*/, Gdiplus::Color backColor, int /*height*/)
{
	content.Append(_T("\t"));
	content.Append(id);

	CString format;
	format.Format(_T(" [label=\"%s\""), text);
	content.Append(format);
	if (m_defaultFontName != fontName)
	{
		format.Format(_T(", fontname=\"%s\""), fontName);
		content.Append(format);
	}

	if (m_defaultFontSize != fontSize)
	{
		format.Format(_T(", fontsize=\"%d\""), fontSize);
		content.Append(format);
	}

	if (m_defaultBackColor.GetValue() != backColor.GetValue())
	{
		format.Format(_T(", color=\"#%06X\""), backColor.GetValue() & 0xffffff);
		content.Append(format);
	}

	content.Append(_T("];\r\n"));
}

void Graphviz::BeginDrawTableNode(CString id, CString fontName, int fontSize, int /*height*/)
{
	m_tableNodeNum = 0;
	content.Append(_T("\t"));
	content.Append(id);

	CString format;
	content.Append(_T("["));
	bool hasAttr = false;
	if (m_defaultFontName != fontName)
	{
		if (hasAttr)
			content.Append(_T(", "));
		format.Format(_T("fontname=\"%s\""), fontName);
		content.Append(format);
		hasAttr = true;
	}

	if (m_defaultFontSize != fontSize)
	{
		if (hasAttr)
			content.Append(_T(", "));
		format.Format(_T("fontsize=\"%d\""), fontSize);
		content.Append(format);
		hasAttr = true;
	}
		
	if (hasAttr)
		content.Append(_T(", "));
	content.Append(_T("color=transparent"));

	content.Append(_T(", label=<\r\n\t<table border=\"0\" cellborder=\"0\" cellpadding=\"5\">\r\n"));
}

void Graphviz::DrawTableNode(CString text, Gdiplus::Color backColor)
{
	CString format;
	format.Format(_T("\t<tr><td port=\"f%d\" bgcolor=\"#%06X\">%s</td></tr>\r\n"), m_tableNodeNum++, backColor.GetValue() & 0xffffff, text);
	content.Append(format);
}

void Graphviz::EndDrawTableNode()
{
	content.Append(_T("\t</table>\r\n\t>];\r\n"));
}

void Graphviz::DrawEdge(CString from, CString to)
{
	content.Append(_T("\t"));
	content.Append(from);
	content.Append(_T("->"));
	content.Append(to);
	content.Append(_T("\r\n"));
}

bool Graphviz::Save(const CString &path)
{
	DWORD dwWritten = 0;
	CAutoFile hFile = CreateFile(path, GENERIC_WRITE, FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (!hFile)
		return false;

	CStringA header;
	header.Format("digraph G {\r\n\tgraph [rankdir=BT];\r\n\tnode [style=\"filled, rounded\", shape=box, fontname=\"Courier New\", fontsize=9, height=0.26, penwidth=0];\r\n");
	CStringA footer = "\r\n}";

	if (!WriteFile(hFile, header, (DWORD)header.GetLength(), &dwWritten, NULL))
		return false;

	CStringA contentA = CUnicodeUtils::GetUTF8(content);
	if (!WriteFile(hFile, contentA, (DWORD)contentA.GetLength(), &dwWritten, NULL))
		return false;
	if (!WriteFile(hFile, footer, (DWORD)footer.GetLength(), &dwWritten, NULL))
		return false;

	return true;
}
