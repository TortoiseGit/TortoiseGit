// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2017, 2019-2021, 2023 - TortoiseGit

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
#include "GitHash.h"
#include <unordered_map>

enum
{
	TGIT_GIT_SUCCESS=0,
	TGIT_GIT_ERROR_OPEN_PIP,
	TGIT_GIT_ERROR_CREATE_PROCESS,
	TGIT_GIT_ERROR_GET_EXIT_CODE
};

class CGitByteArray : public std::vector<char>
{
public:
	size_t find(char data, size_t start = 0) const
	{
		const size_t end = size();
		for (size_t i = start; i < end; ++i)
			if ((*this)[i] == data)
				return i;
		return npos;
	}
	size_t RevertFind(char data, size_t start = npos) const
	{
		if (start == npos)
		{
			if (empty())
				return npos;
			start = size() - 1;
		}

		for (size_t i = start + 1; i-- > 0;)
			if ((*this)[i] == data)
				return i;
		return npos;
	}
	size_t findNextString(size_t start = 0) const
	{
		size_t pos = start;
		const size_t end = size();
		do
		{
			pos=find(0,pos);
			if (pos == npos)
				break;
			++pos;
			if (pos >= end)
				return npos;

		} while ((*this)[pos] == 0);

		return pos;
	}
	size_t append(const std::vector<char>& v, size_t start = 0, size_t end = npos)
	{
		if (end == npos)
			end = v.size();

		if (end <= start)
			return 0;
		insert(this->end(), v.cbegin() + start, v.cbegin() + end);
		return 0;
	}
	void append(const char* data, size_t dataSize)
	{
		if (dataSize == 0)
			return;
		const size_t oldsize = size();
		resize(oldsize + dataSize);
		memcpy(this->data() + oldsize, data, dataSize);
	}
	static const size_t npos = static_cast<size_t>(-1); // bad/missing length/position
	static_assert(MAXSIZE_T == npos, "NPOS must equal MAXSIZE_T");
	static_assert(-1 == static_cast<int>(npos), "NPOS must equal -1");
};

class CGitGuardedByteArray : public CGitByteArray
{
private:
	CGitGuardedByteArray(const CGitGuardedByteArray&) = delete;
	CGitGuardedByteArray& operator=(const CGitGuardedByteArray&) = delete;
public:
	CGitGuardedByteArray() {}
	~CGitGuardedByteArray() {}
	CComAutoCriticalSection	m_critSec;
};

struct TGitRef
{
	CString name;
	CGitHash hash;
	operator const CString&() const { return name; }
};

using STRING_VECTOR = std::vector<CString>;
using MAP_HASH_NAME = std::unordered_map<CGitHash, STRING_VECTOR>;
using MAP_STRING_STRING = std::map<CString, CString>;
using REF_VECTOR = std::vector<TGitRef>;
using BYTE_VECTOR = CGitByteArray;
