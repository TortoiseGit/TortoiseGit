// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2017, 2019 - TortoiseGit

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

class CGitByteArray:public std::vector<BYTE>
{
public:
	size_t find(BYTE data, size_t start = 0) const
	{
		for (size_t i = start, end = size(); i < end; ++i)
			if ((*this)[i] == data)
				return i;
		return npos;
	}
	size_t RevertFind(BYTE data, size_t start = npos) const
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
		size_t end = size();
		do
		{
			pos=find(0,pos);
			if(pos != npos)
				++pos;
			else
				break;

			if (pos >= end)
				return npos;

		}while(at(pos)==0);

		return pos;
	}
	size_t append(std::vector<BYTE> &v, size_t start = 0, size_t end = npos)
	{
		if (end == npos)
			end = v.size();
		for (size_t i = start; i < end; ++i)
			this->push_back(v[i]);
		return 0;
	}
	void append(const BYTE* data, size_t dataSize)
	{
		if (dataSize == 0)
			return;
		size_t oldsize=size();
		resize(oldsize+dataSize);
		memcpy(&*(begin()+oldsize),data,dataSize);
	}
	static const size_t npos = static_cast<size_t>(-1); // bad/missing length/position
	static_assert(MAXSIZE_T == npos, "NPOS must equal MAXSIZE_T");
#pragma warning(push)
#pragma warning(disable: 4309)
	static_assert(-1 == static_cast<int>(npos), "NPOS must equal -1");
#pragma warning(pop)
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

typedef std::vector<CString> STRING_VECTOR;
typedef std::unordered_map<CGitHash, STRING_VECTOR> MAP_HASH_NAME;
typedef std::map<CString, CString> MAP_STRING_STRING;
typedef std::vector<TGitRef> REF_VECTOR;
typedef CGitByteArray BYTE_VECTOR;
