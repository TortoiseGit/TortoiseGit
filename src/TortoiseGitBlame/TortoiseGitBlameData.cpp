// TortoiseGitBlame - a Viewer for Git Blames

// Copyright (C) 2008-2013 - TortoiseGit
// Copyright (C) 2010-2013 Sven Strickroth <email@cs-ware.de>
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
#include "TortoiseGitBlame.h"
#include "CommonAppUtils.h"
#include "TortoiseGitBlameDoc.h"
#include "TortoiseGitBlameData.h"
#include "MainFrm.h"
#include "EditGotoDlg.h"
#include "LoglistUtils.h"
#include "FileTextLines.h"
#include "UnicodeUtils.h"
#include "MenuEncode.h"
#include "gitdll.h"
#include "SysInfo.h"
#include "StringUtils.h"
#include "TGitPath.h"

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

void CTortoiseGitBlameData::ParseBlameOutput(BYTE_VECTOR &data, CGitHashMap & HashToRev, DWORD dateFormat, bool bRelativeTimes)
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
		int lineEnd = data.findData((const BYTE*)"\n", 1, lineBegin);
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
						int originalLineNumberEnd = data.findData((const BYTE*)" ", 1, originalLineNumberBegin);
						if (originalLineNumberEnd >= 0)
						{
							originalLineNumber = atoi(CStringA((LPCSTR)&data[originalLineNumberBegin], originalLineNumberEnd - originalLineNumberBegin));
							int finalLineNumberBegin = originalLineNumberEnd + 1;
							int finalLineNumberEnd = (numberOfSubsequentLines == 0) ? data.findData((const BYTE*)" ", 1, finalLineNumberBegin) : lineEnd;
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
					int tokenEnd = data.findData((const BYTE*)" ", 1, tokenBegin);
					if (tokenEnd >= 0)
					{
						if (!strncmp("filename", (const char*)&data[tokenBegin], tokenEnd - tokenBegin))
						{
							int filenameBegin = tokenEnd + 1;
							int filenameEnd = lineEnd;
							filename = CUnicodeUtils::GetUnicode(CStringA((LPCSTR)&data[filenameBegin], filenameEnd-filenameBegin));
							hashToFilename.insert(std::make_pair(hash, filename));
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
		GitRev *pRev = GetRevForHash(HashToRev, hash);
		if (pRev)
		{
			authors.push_back(pRev->GetAuthorName());
			dates.push_back(CLoglistUtils::FormatDateAndTime(pRev->GetAuthorDate(), dateFormat, true, bRelativeTimes));
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

int CTortoiseGitBlameData::FindFirstLineWrapAround(const CString& what, int line, bool bCaseSensitive)
{
	CString whatNormalized(what);
	if (!bCaseSensitive)
	{
		whatNormalized.MakeLower();
	}

	CStringA whatNormalizedUtf8 = CUnicodeUtils::GetUTF8(whatNormalized);

	bool bFound = false;

	int i = line;
	int numberOfLines = GetNumberOfLines();
	if (line < 0 || line + 1 >= numberOfLines)
		i = 0;

	do
	{
		if (bCaseSensitive)
		{
			if (m_Authors[i].Find(whatNormalized) >= 0)
				bFound = true;
			else if (m_Utf8Lines[i].Find(whatNormalizedUtf8) >=0)
				bFound = true;
		}
		else
		{
			if (m_Authors[i].MakeLower().Find(whatNormalized) >= 0)
				bFound = true;
			else if (m_Utf8Lines[i].MakeLower().Find(whatNormalizedUtf8) >=0)
				bFound = true;
		}

		if(bFound)
		{
			break;
		}
		else
		{
			++i;
			if (i >= numberOfLines)
				i = 0;
		}
	} while (i != line);

	return bFound ? i : -1;
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
