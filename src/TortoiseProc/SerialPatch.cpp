// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2013, 2015-2016 - TortoiseGit

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

int CSerialPatch::Parse(const CString& pathfile, bool parseBody)
{
	m_PathFile = pathfile;

	CFile PatchFile;

	if (!PatchFile.Open(m_PathFile, CFile::modeRead))
		return -1;

	PatchFile.Read(CStrBufA(m_Body, (UINT)PatchFile.GetLength()), (UINT)PatchFile.GetLength());
	PatchFile.Close();

	try
	{
		int start = 0;
		CStringA one;
		one = m_Body.Tokenize("\n", start);

		if (start == -1)
			return -1;
		one = m_Body.Tokenize("\n", start);
		if (one.GetLength()>6)
			CGit::StringAppend(&m_Author, (BYTE*)(LPCSTR)one + 6, CP_UTF8, one.GetLength() - 6);
		m_Author.TrimRight(L'\r');

		if (start == -1)
			return -1;
		one = m_Body.Tokenize("\n", start);
		if (one.GetLength()>6)
			CGit::StringAppend(&m_Date, (BYTE*)(LPCSTR)one + 6, CP_UTF8, one.GetLength() - 6);
		m_Date.TrimRight(L'\r');

		if (start == -1)
			return -1;
		one = m_Body.Tokenize("\n", start);
		if (one.GetLength()>9)
		{
			CGit::StringAppend(&m_Subject, (BYTE*)(LPCSTR)one + 9, CP_UTF8, one.GetLength() - 9);
			while (m_Body.GetLength() > start && m_Body.GetAt(start) == L' ')
			{
				one = m_Body.Tokenize("\n", start);
				CGit::StringAppend(&m_Subject, (BYTE*)(LPCSTR)one, CP_UTF8, one.GetLength());
			}
			m_Subject.TrimRight(L'\r');
		}

		if (!parseBody)
			return 0;

		while (start > 0)
		{
			if (m_Body.Mid(start - 1, 2) == L"\n\n")
				break;
			if (m_Body.Mid(start - 4, 4) == L"\r\n\r\n")
			{
				--start;
				break;
			}
			m_Body.Tokenize("\n", start);
		}

		if (start == -1)
			return -1;

		if (start + 1 < m_Body.GetLength())
			CGit::StringAppend(&m_strBody, (BYTE*)(LPCSTR)m_Body + start + 1, CP_UTF8, m_Body.GetLength() - start - 1);
	}
	catch (CException *)
	{
		return -1;
	}

	return 0;
}
