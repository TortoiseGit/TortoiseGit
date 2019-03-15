// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2013, 2015-2016, 2018-2019 - TortoiseGit

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
#include "SerialPatch.h"
#include "Git.h"

CSerialPatch::CSerialPatch()
{}

CSerialPatch::~CSerialPatch()
{}

#define FROMHEADER "From: "
#define DATEHEADER "Date: "
#define SUBJECTHEADER "Subject: "

int CSerialPatch::Parse(const CString& pathfile, bool parseBody)
{
	m_PathFile = pathfile;

	CFile PatchFile;
	if (!PatchFile.Open(m_PathFile, CFile::modeRead))
		return -1;

	PatchFile.Read(CStrBufA(m_Body, static_cast<UINT>(PatchFile.GetLength())), static_cast<UINT>(PatchFile.GetLength()));
	PatchFile.Close();

	int start = 0;
	do
	{
		CStringA line = m_Body.Tokenize("\n", start);
		if (CStringUtils::StartsWith(line, FROMHEADER))
		{
			CGit::StringAppend(&m_Author, static_cast<LPCSTR>(line) + static_cast<int>(strlen(FROMHEADER)), CP_UTF8, line.GetLength() - static_cast<int>(strlen(FROMHEADER)));
			m_Author.TrimRight(L'\r');
		}
		else if (CStringUtils::StartsWith(line, DATEHEADER))
		{
			CGit::StringAppend(&m_Date, static_cast<LPCSTR>(line) + static_cast<int>(strlen(DATEHEADER)), CP_UTF8, line.GetLength() - static_cast<int>(strlen(DATEHEADER)));
			m_Date.TrimRight(L'\r');
		}
		else if (CStringUtils::StartsWith(line, SUBJECTHEADER))
		{
			CGit::StringAppend(&m_Subject, static_cast<LPCSTR>(line) + static_cast<int>(strlen(SUBJECTHEADER)), CP_UTF8, line.GetLength() - static_cast<int>(strlen(SUBJECTHEADER)));
			while (m_Body.GetLength() > start && (m_Body.GetAt(start) == L' ' || m_Body.GetAt(start) == L'\t'))
			{
				line = m_Body.Tokenize("\n", start);
				CGit::StringAppend(&m_Subject, static_cast<LPCSTR>(line), CP_UTF8, line.GetLength());
			}
			m_Subject.TrimRight(L'\r');
		}

		if (start >= 1 && m_Body.Mid(start - 1, 2) == L"\n\n")
			break;
		if (start >= 4 && m_Body.Mid(start - 4, 4) == L"\r\n\r\n")
		{
			--start;
			break;
		}
	} while (start > 0);

	if (!parseBody)
		return 0;

	if (start == -1)
		return -1;

	if (start + 1 < m_Body.GetLength())
		CGit::StringAppend(&m_strBody, static_cast<LPCSTR>(m_Body) + start + 1, CP_UTF8, m_Body.GetLength() - start - 1);

	return 0;
}
