// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013-2016, 2018-2019 - TortoiseGit

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
	content.AppendChar(L'\t');
	content.Append(id);

	content.AppendFormat(L" [label=\"%s\"", static_cast<LPCTSTR>(text));
	if (m_defaultFontName != fontName)
		content.AppendFormat(L", fontname=\"%s\"", static_cast<LPCTSTR>(fontName));

	if (m_defaultFontSize != fontSize)
		content.AppendFormat(L", fontsize=\"%d\"", fontSize);

	if (m_defaultBackColor.GetValue() != backColor.GetValue())
		content.AppendFormat(L", color=\"#%06X\"", backColor.GetValue() & 0xffffff);

	content.Append(L"];\r\n");
}

void Graphviz::BeginDrawTableNode(CString id, CString fontName, int fontSize, int /*height*/)
{
	m_tableNodeNum = 0;
	content.AppendChar(L'\t');
	content.Append(id);

	content.AppendChar(L'[');
	bool hasAttr = false;
	if (m_defaultFontName != fontName)
	{
		content.AppendFormat(L"fontname=\"%s\"", static_cast<LPCTSTR>(fontName));
		hasAttr = true;
	}

	if (m_defaultFontSize != fontSize)
	{
		if (hasAttr)
			content.Append(L", ");
		content.AppendFormat(L"fontsize=\"%d\"", fontSize);
		hasAttr = true;
	}

	if (hasAttr)
		content.Append(L", ");
	content.Append(L"color=transparent");

	content.Append(L", label=<\r\n\t<table border=\"0\" cellborder=\"0\" cellpadding=\"5\">\r\n");
}

void Graphviz::DrawTableNode(CString text, Gdiplus::Color backColor)
{
	content.AppendFormat(L"\t<tr><td port=\"f%d\" bgcolor=\"#%06X\">%s</td></tr>\r\n", m_tableNodeNum++, backColor.GetValue() & 0xffffff, static_cast<LPCTSTR>(text));
}

void Graphviz::EndDrawTableNode()
{
	content.Append(L"\t</table>\r\n\t>];\r\n");
}

void Graphviz::DrawEdge(CString from, CString to)
{
	content.AppendChar(L'\t');
	content.Append(from);
	content.Append(L"->");
	content.Append(to);
	content.Append(L"\r\n");
}

bool Graphviz::Save(const CString &path)
{
	DWORD dwWritten = 0;
	CAutoFile hFile = CreateFile(path, GENERIC_WRITE, FILE_SHARE_DELETE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (!hFile)
		return false;

	CStringA header;
	header.Format("digraph G {\r\n\tgraph [rankdir=BT];\r\n\tnode [style=\"filled, rounded\", shape=box, fontname=\"Courier New\", fontsize=9, height=0.26, penwidth=0];\r\n");
	CStringA footer = "\r\n}";

	if (!WriteFile(hFile, header, static_cast<DWORD>(header.GetLength()), &dwWritten, nullptr))
		return false;

	CStringA contentA = CUnicodeUtils::GetUTF8(content);
	if (!WriteFile(hFile, contentA, static_cast<DWORD>(contentA.GetLength()), &dwWritten, nullptr))
		return false;
	if (!WriteFile(hFile, footer, static_cast<DWORD>(footer.GetLength()), &dwWritten, nullptr))
		return false;

	return true;
}
