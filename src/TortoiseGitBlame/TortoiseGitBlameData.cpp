// TortoiseGitBlame - a Viewer for Git Blames

// Copyright (C) 2008-2015 - TortoiseGit
// Copyright (C) 2003 Don HO <donho@altern.org>

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

// CTortoiseGitBlameData.cpp : implementation of the CTortoiseGitBlameData class
//

#include "stdafx.h"
#include "TortoiseGitBlameData.h"
#include "LoglistUtils.h"
#include "FileTextLines.h"
#include "UnicodeUtils.h"

wchar_t WideCharSwap2(wchar_t nValue)
{
	return (((nValue>> 8)) | (nValue << 8));
}

// CTortoiseGitBlameData construction/destruction

CTortoiseGitBlameData::CTortoiseGitBlameData()
{
	m_encode = -1;
}

CTortoiseGitBlameData::~CTortoiseGitBlameData()
{
}

int CTortoiseGitBlameData::GetEncode(unsigned char *buff, int size, int *bomoffset)
{
	CFileTextLines textlines;
	CFileTextLines::UnicodeType type = textlines.CheckUnicodeType(buff, size);

	if (type == CFileTextLines::UTF8BOM)
	{
		*bomoffset = 3;
		return CP_UTF8;
	}
	if (type == CFileTextLines::UTF8)
		return CP_UTF8;

	if (type == CFileTextLines::UTF16_LE)
		return 1200;
	if (type == CFileTextLines::UTF16_LEBOM)
	{
		*bomoffset = 2;
		return 1200;
	}

	if (type == CFileTextLines::UTF16_BE)
		return 1201;
	if (type == CFileTextLines::UTF16_BEBOM)
	{
		*bomoffset = 2;
		return 1201;
	}

	return GetACP();
}

int CTortoiseGitBlameData::GetEncode(int *bomoffset)
{
	int encoding = 0;
	BYTE_VECTOR rawAll;
	for (auto it = m_RawLines.begin(), it_end = m_RawLines.end(); it != it_end; ++it)
	{
		rawAll.append(&(*it)[0], it->size());
	}
	encoding = GetEncode(&rawAll[0], (int)rawAll.size(), bomoffset);
	return encoding;
}

void CTortoiseGitBlameData::ParseBlameOutput(BYTE_VECTOR &data, CGitHashMap & HashToRev, DWORD dateFormat, bool bRelativeTimes, bool bUtc)
{
	std::map<CGitHash, CString> hashToFilename;

	std::vector<CGitHash>		hashes;
	std::vector<int>			originalLineNumbers;
	std::vector<CString>		filenames;
	std::vector<BYTE_VECTOR>	rawLines;
	std::vector<CString>		authors;
	std::vector<CString>		dates;

	CGitHash hash;
	int originalLineNumber = 0;
	int finalLineNumber = 0;
	int numberOfSubsequentLines = 0;
	CString filename;

	int pos = 0;
	bool expectHash = true;
	while (pos >= 0 && (size_t)pos < data.size())
	{
		if (data[pos] == 0)
			continue;

		int lineBegin = pos;
		int lineEnd = data.find('\n', lineBegin);
		if (lineEnd < 0)
			lineEnd = (int)data.size();

		if (lineEnd > lineBegin)
		{
			if (data[lineBegin] != '\t')
			{
				if (expectHash)
				{
					expectHash = false;
					if (lineEnd - lineBegin > 40)
					{
						hash.ConvertFromStrA((char*)&data[lineBegin]);

						int hashEnd = lineBegin + 40;
						int originalLineNumberBegin = hashEnd + 1;
						int originalLineNumberEnd = data.find(' ', originalLineNumberBegin);
						if (originalLineNumberEnd >= 0)
						{
							originalLineNumber = atoi(CStringA((LPCSTR)&data[originalLineNumberBegin], originalLineNumberEnd - originalLineNumberBegin));
							int finalLineNumberBegin = originalLineNumberEnd + 1;
							int finalLineNumberEnd = (numberOfSubsequentLines == 0) ? data.find(' ', finalLineNumberBegin) : lineEnd;
							if (finalLineNumberEnd >= 0)
							{
								finalLineNumber = atoi(CStringA((LPCSTR)&data[finalLineNumberBegin], finalLineNumberEnd - finalLineNumberBegin));
								if (numberOfSubsequentLines == 0)
								{
									int numberOfSubsequentLinesBegin = finalLineNumberEnd + 1;
									int numberOfSubsequentLinesEnd = lineEnd;
									numberOfSubsequentLines = atoi(CStringA((LPCSTR)&data[numberOfSubsequentLinesBegin], numberOfSubsequentLinesEnd - numberOfSubsequentLinesBegin));
								}
							}
							else
							{
								// parse error
								finalLineNumber = 0;
								numberOfSubsequentLines = 0;
							}
						}
						else
						{
							// parse error
							finalLineNumber = 0;
							numberOfSubsequentLines = 0;
						}

						auto it = hashToFilename.find(hash);
						if (it != hashToFilename.end())
							filename = it->second;
						else
							filename.Empty();
					}
					else
					{
						// parse error
						finalLineNumber = 0;
						numberOfSubsequentLines = 0;
					}
				}
				else
				{
					int tokenBegin = lineBegin;
					int tokenEnd = data.find(' ', tokenBegin);
					if (tokenEnd >= 0)
					{
						if (!strncmp("filename", (const char*)&data[tokenBegin], tokenEnd - tokenBegin))
						{
							int filenameBegin = tokenEnd + 1;
							int filenameEnd = lineEnd;
							CStringA filenameA = CStringA((LPCSTR)&data[filenameBegin], filenameEnd - filenameBegin);
							filename = UnquoteFilename(filenameA);
							auto r = hashToFilename.insert(std::make_pair(hash, filename));
							if (!r.second)
							{
								r.first->second = filename;
							}
						}
					}
				}
			}
			else
			{
				expectHash = true;
				// remove <TAB> at start
				BYTE_VECTOR line;
				if (lineEnd - 1 > lineBegin)
					line.append(&data[lineBegin + 1], lineEnd-lineBegin - 1);

				hashes.push_back(hash);
				filenames.push_back(filename);
				originalLineNumbers.push_back(originalLineNumber);
				rawLines.push_back(line);
				--numberOfSubsequentLines;
			}
		}
		pos = lineEnd + 1;
	}

	for (auto it = hashes.begin(), it_end = hashes.end(); it != it_end; ++it)
	{
		CGitHash hash = *it;
		GitRev *pRev;
		try
		{
			pRev = GetRevForHash(HashToRev, hash);
		}
		catch (char* e)
		{
			MessageBox(nullptr, _T("Could not get revision by hash \"") + hash.ToString() + _T("\".\nlibgit reported:\n") + CString(e), _T("TortoiseGit"), MB_OK);
			return;
		}
		if (pRev)
		{
			authors.push_back(pRev->GetAuthorName());
			dates.push_back(CLoglistUtils::FormatDateAndTime(pRev->GetAuthorDate(), dateFormat, true, bRelativeTimes, bUtc));
		}
		else
		{
			authors.push_back(CString());
			dates.push_back(CString());
		}
	}

	m_Hash.swap(hashes);
	m_OriginalLineNumbers.swap(originalLineNumbers);
	m_Filenames.swap(filenames);
	m_RawLines.swap(rawLines);

	m_Authors.swap(authors);
	m_Dates.swap(dates);
	// reset detected and applied encoding
	m_encode = -1;
	m_Utf8Lines.clear();
}

int CTortoiseGitBlameData::UpdateEncoding(int encode)
{
	int encoding = encode;
	int bomoffset = 0;
	if (encoding==0)
	{
		BYTE_VECTOR all;
		for (auto it = m_RawLines.begin(); it != m_RawLines.end(); ++it)
		{
			if (!it->empty())
				all.append(&(*it)[0], it->size());
		}
		encoding = GetEncode(&all[0], (int)all.size(), &bomoffset);
	}

	if (encoding != m_encode)
	{
		m_encode = encoding;

		m_Utf8Lines.resize(m_RawLines.size());
		for (size_t i_Lines = 0; i_Lines < m_RawLines.size(); ++i_Lines)
		{
			const BYTE_VECTOR& rawLine = m_RawLines[i_Lines];

			int bomoffset = 0;
			CStringA lineUtf8;
			lineUtf8.Empty();

			if (!rawLine.empty())
			{
				if (encoding == 1201)
				{
					CString line;
					int size = (int)((rawLine.size() - bomoffset)/2);
					TCHAR *buffer = line.GetBuffer(size);
					memcpy(buffer, &rawLine[bomoffset], sizeof(TCHAR)*size);
					// swap the bytes to little-endian order to get proper strings in wchar_t format
					wchar_t * pSwapBuf = buffer;
					for (int i = 0; i < size; ++i)
					{
						*pSwapBuf = WideCharSwap2(*pSwapBuf);
						++pSwapBuf;
					}
					line.ReleaseBuffer();

					lineUtf8 = CUnicodeUtils::GetUTF8(line);
				}
				else if (encoding == 1200)
				{
					CString line;
					// the first bomoffset is 2, after that it's 1 (see issue #920)
					// also: don't set bomoffset if called from Encodings menu (i.e. start == 42 and bomoffset == 0); bomoffset gets only set if autodetected
					if (bomoffset == 0 && i_Lines != 0)
					{
						bomoffset = 1;
					}
					int size = (int)((rawLine.size() - bomoffset)/2);
					TCHAR *buffer = line.GetBuffer(size);
					memcpy(buffer, &rawLine[bomoffset], sizeof(TCHAR) * size);
					line.ReleaseBuffer();

					lineUtf8 = CUnicodeUtils::GetUTF8(line);
				}
				else if (encoding == CP_UTF8)
					lineUtf8 = CStringA((LPCSTR)&rawLine[bomoffset], (int)(rawLine.size() - bomoffset));
				else
				{
					CString line = CUnicodeUtils::GetUnicode(CStringA((LPCSTR)&rawLine[bomoffset], (int)(rawLine.size() - bomoffset)), encoding);
					lineUtf8 = CUnicodeUtils::GetUTF8(line);
				}
			}

			m_Utf8Lines[i_Lines] = lineUtf8;
			bomoffset = 0;
		}
	}
	return encoding;
}

int CTortoiseGitBlameData::FindNextLine(CGitHash& CommitHash, int line, bool bUpOrDown)
{
	int startline = line;
	bool findNoMatch = false;
	while (line >= 0 && line < (int)m_Hash.size())
	{
		if (m_Hash[line] != CommitHash)
			findNoMatch = true;

		if (m_Hash[line] == CommitHash && findNoMatch)
		{
			if (line == startline + 2)
				findNoMatch = false;
			else
			{
				if (bUpOrDown)
					line = FindFirstLineInBlock(CommitHash, line);
				return line;
			}
		}
		if (bUpOrDown)
			--line;
		else
			++line;
	}
	return -1;
}

static int FindAsciiLower(const CStringA &str, const CStringA &find)
{
	if (find.GetLength() == 0)
		return 0;

	for (int i = 0; i < str.GetLength(); ++i)
	{
		char c = str[i];
		c += (c >= 'A' && c <= 'Z') ? 32 : 0;
		if (c == find[0])
		{
			bool diff = false;
			int k = 1;
			for (int j = i + 1; j < str.GetLength() && k < find.GetLength(); ++j, ++k)
			{
				char d = str[j];
				d += (d >= 'A' && d <= 'Z') ? 32 : 0;
				if (d != find[k])
				{
					diff = true;
					break;
				}
			}

			if (!diff && k == find.GetLength())
				return i;
		}
	}

	return -1;
}

static int FindUtf8Lower(const CStringA& strA, bool allAscii, const CString &findW, const CStringA &findA)
{
	if (allAscii)
		return FindAsciiLower(strA, findA);

	CString strW = CUnicodeUtils::GetUnicode(strA);
	return strW.MakeLower().Find(findW);
}

int CTortoiseGitBlameData::FindFirstLineWrapAround(SearchDirection direction, const CString& what, int line, bool bCaseSensitive)
{
	bool allAscii = true;
	for (int i = 0; i < what.GetLength(); ++i)
	{
		if (what[i] > 0x7f)
		{
			allAscii = false;
			break;
		}
	}
	CString whatNormalized(what);
	if (!bCaseSensitive)
	{
		whatNormalized.MakeLower();
	}

	CStringA whatNormalizedUtf8 = CUnicodeUtils::GetUTF8(whatNormalized);

	int numberOfLines = GetNumberOfLines();
	int i = line;
	if (direction == SearchPrevious)
	{
		i -= 2;
		if (i < 0)
			i = numberOfLines - 1;
	}
	else if (line < 0 || line + 1 >= numberOfLines)
		i = 0;

	do
	{
		if (bCaseSensitive)
		{
			if (m_Authors[i].Find(whatNormalized) >= 0)
				return i;
			else if (m_Utf8Lines[i].Find(whatNormalizedUtf8) >=0)
				return i;
		}
		else
		{
			if (CString(m_Authors[i]).MakeLower().Find(whatNormalized) >= 0)
				return i;
			else if (FindUtf8Lower(m_Utf8Lines[i], allAscii, whatNormalized, whatNormalizedUtf8) >= 0)
				return i;
		}

		if (direction == SearchNext)
		{
			++i;
			if (i >= numberOfLines)
				i = 0;
		}
		else if (direction == SearchPrevious)
		{
			--i;
			if (i < 0)
				i = numberOfLines - 2;
		}
	} while (i != line);

	return -1;
}

bool CTortoiseGitBlameData::ContainsOnlyFilename(const CString &filename) const
{
	for (auto it = m_Filenames.cbegin(); it != m_Filenames.cend(); ++it)
	{
		if (filename != *it)
			return false;
	}
	return true;
}

GitRev* CTortoiseGitBlameData::GetRevForHash(CGitHashMap & HashToRev, CGitHash& hash)
{
	auto it = HashToRev.find(hash);
	if (it == HashToRev.end())
	{
		GitRev rev;
		rev.GetCommitFromHash(hash);
		it = HashToRev.insert(std::make_pair(hash, rev)).first;
	}
	return &(it->second);
}

CString CTortoiseGitBlameData::UnquoteFilename(CStringA& s)
{
	if (s[0] == '"')
	{
		CStringA ret;
		int i_size = s.GetLength();
		bool isEscaped = false;
		for (int i = 1; i < i_size; ++i)
		{
			char c = s[i];
			if (isEscaped)
			{
				if (c >= '0' && c <= '3')
				{
					if (i + 2 < i_size)
					{
						c = (((c - '0') & 03) << 6) | (((s[i + 1] - '0') & 07) << 3) | ((s[i + 2] - '0') & 07);
						i += 2;
						ret += c;
					}
				}
				else
				{
					switch (c)
					{
					case 'a' : c = '\a'; break;
					case 'b' : c = '\b'; break;
					case 't' : c = '\t'; break;
					case 'n' : c = '\n'; break;
					case 'v' : c = '\v'; break;
					case 'f' : c = '\f'; break;
					case 'r' : c = '\r'; break;
					}
					ret += c;
				}
				isEscaped = false;
			}
			else
			{
				if (c == '\\')
				{
					isEscaped = true;
				}
				else if(c == '"')
				{
					break;
				}
				else
				{
					ret += c;
				}
			}
		}
		return CUnicodeUtils::GetUnicode(ret);
	}
	else
		return CUnicodeUtils::GetUnicode(s);
}
