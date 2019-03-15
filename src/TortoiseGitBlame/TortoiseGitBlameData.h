// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2013, 2015-2019 - TortoiseGit

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
// CTortoiseGitBlameData.h : interface of the CTortoiseGitBlameData class
//

#pragma once

#include "GitHash.h"
#include "gitlogcache.h"
#include <unordered_set>

class CTortoiseGitBlameData
{
// Implementation
public:
	CTortoiseGitBlameData();
	virtual ~CTortoiseGitBlameData();

public:
	int GetEncode(unsigned char * buffer, int size, int *bomoffset);
	int GetEncode(int *bomoffset);
	void ParseBlameOutput(BYTE_VECTOR &data, CGitHashMap & HashToRev, DWORD dateFormat, bool bRelativeTimes);
	// updates sourcecode lines to the given encoding, encode==0 detects the encoding, returns the used encoding
	int UpdateEncoding(int encode = 0);

	BOOL IsValidLine(int line)
	{
		return line >= 0 && line < static_cast<int>(m_Hash.size());
	}
	int FindNextLine(CGitHash& commithash, int line, bool bUpOrDown=false);
	// find first line with the given hash starting with given "line"
	int FindFirstLine(CGitHash& commithash, int line)
	{
		int numberOfLines = static_cast<int>(GetNumberOfLines());
		for (int i = (line >= 0 ? line : 0); i < numberOfLines; ++i)
		{
			if (m_Hash[i] == commithash)
				return i;
		}
		return -1;
	}
	// find first line of the current block with the given hash starting with given "line"
	int FindFirstLineInBlock(CGitHash& commithash, int line)
	{
		while (line >= 0)
		{
			if (m_Hash[line] != commithash)
				return line++;
			--line;
		}
		return line;
	}
	enum SearchDirection{ SearchNext = 0, SearchPrevious = 1 };
	int FindFirstLineWrapAround(SearchDirection direction, const CString& what, int line, bool bCaseSensitive, std::function<void()> wraparound);

	size_t GetNumberOfLines() const
	{
		return m_Hash.size();
	}

	CGitHash& GetHash(size_t line)
	{
		return m_Hash[line];
	}

	void GetHashes(std::unordered_set<CGitHash>& hashes)
	{
		hashes.clear();
		for (const auto& hash : m_Hash)
		{
			hashes.insert(hash);
		}
	}

	const CString& GetDate(size_t line) const
	{
		return m_Dates[line];
	}

	const CString& GetAuthor(size_t line) const
	{
		return m_Authors[line];
	}

	const CString& GetFilename(size_t line) const
	{
		return m_Filenames[line];
	}

	int GetOriginalLineNumber(size_t line) const
	{
		return m_OriginalLineNumbers[line];
	}

	const CStringA& GetUtf8Line(size_t line) const
	{
		return m_Utf8Lines[line];
	}

	bool ContainsOnlyFilename(const CString &filename) const;

	GitRevLoglist* GetRev(int line, CGitHashMap& hashToRev)
	{
		return GetRevForHash(hashToRev, GetHash(line));
	}

private:
	static GitRevLoglist* GetRevForHash(CGitHashMap& HashToRev, const CGitHash& hash, CString* err = nullptr);
	static CString UnquoteFilename(CStringA& s);

	std::vector<CGitHash>		m_Hash;
	std::vector<CString>		m_Dates;
	std::vector<CString>		m_Authors;
	std::vector<CString>		m_Filenames;
	std::vector<int>			m_OriginalLineNumbers;
	std::vector<BYTE_VECTOR>	m_RawLines;

	int m_encode;
	std::vector<CStringA> m_Utf8Lines;
};
