// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2018 - TortoiseGit

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
#define GIT_HASH_SIZE 20

#define GIT_REV_ZERO L"0000000000000000000000000000000000000000"

class CGitHash
{
public:
	unsigned char m_hash[GIT_HASH_SIZE];

	CGitHash()
	{
		memset(m_hash,0, GIT_HASH_SIZE);
	}
	CGitHash(const unsigned char* p)
	{
		memcpy(m_hash,p,GIT_HASH_SIZE);
	}
	CGitHash & operator = (const CString &str)
	{
		CGitHash hash(str);
		*this = hash;
		return *this;
	}
	CGitHash & operator = (const unsigned char *p)
	{
		memcpy(m_hash, p, GIT_HASH_SIZE);
		return *this;
	}
	CGitHash(const CString &str)
	{
		if (!IsValidSHA1(str))
		{
#ifdef ASSERT
			//ASSERT(FALSE); // TODO problematic
#endif
			memset(m_hash, 0, GIT_HASH_SIZE);
			return;
		}

		for (int i = 0; i < GIT_HASH_SIZE; ++i)
		{
			unsigned char a;
			a=0;
			for (int j = 2 * i; j <= 2 * i + 1; ++j)
			{
				a =a<<4;

				TCHAR ch = str[j];
				if (ch >= L'0' && ch <= L'9')
					a |= (ch - L'0') & 0xF;
				else if (ch >=L'A' && ch <= L'F')
					a |= ((ch - L'A') & 0xF) + 10 ;
				else if (ch >=L'a' && ch <= L'f')
					a |= ((ch - L'a') & 0xF) + 10;

			}
			m_hash[i]=a;
		}
	}

	void ConvertFromStrA(const char *str)
	{
		for (int i = 0; i < GIT_HASH_SIZE; ++i)
		{
			unsigned char a;
			a=0;
			for (int j = 2 * i; j <= 2 * i + 1; ++j)
			{
				a =a<<4;

				char ch = str[j];
				if (ch >= '0' && ch <= '9')
					a |= (ch - '0') & 0xF;
				else if (ch >= 'A' && ch <= 'F')
					a |= ((ch - 'A') & 0xF) + 10 ;
				else if (ch >= 'a' && ch <= 'f')
					a |= ((ch - 'a') & 0xF) + 10;

			}
			m_hash[i]=a;
		}
	}
	void Empty()
	{
		memset(m_hash,0, GIT_HASH_SIZE);
	}
	bool IsEmpty() const
	{
		for (int i = 0; i < GIT_HASH_SIZE; ++i)
		{
			if(m_hash[i] != 0)
				return false;
		}
		return true;
	}

	CString ToString() const
	{
		CString str;
		for (int i = 0; i < GIT_HASH_SIZE; ++i)
			str.AppendFormat(L"%02x", m_hash[i]);
		return str;
	}
	operator CString () const
	{
		return ToString();
	}

	bool operator == (const CGitHash &hash) const
	{
		return memcmp(m_hash,hash.m_hash,GIT_HASH_SIZE) == 0;
	}

	static friend bool operator<(const CGitHash& left, const CGitHash& right)
	{
		return memcmp(left.m_hash,right.m_hash,GIT_HASH_SIZE) < 0;
	}

	static friend bool operator>(const CGitHash& left, const CGitHash& right)
	{
		return memcmp(left.m_hash, right.m_hash, GIT_HASH_SIZE) > 0;
	}

	static friend bool operator != (const CGitHash& left, const CGitHash& right)
	{
		return memcmp(left.m_hash, right.m_hash, GIT_HASH_SIZE) != 0;
	}

	bool MatchesPrefix(const CGitHash& hash, const CString& hashString, size_t prefixLen) const
	{
		if (memcmp(m_hash, hash.m_hash, prefixLen >> 1))
			return false;
		return prefixLen == 2 * GIT_HASH_SIZE || wcsncmp(ToString(), hashString, prefixLen) == 0;
	}

	static bool IsValidSHA1(const CString &possibleSHA1)
	{
		if (possibleSHA1.GetLength() != 2 * GIT_HASH_SIZE)
			return false;
		for (int i = 0; i < possibleSHA1.GetLength(); ++i)
		{
			if (!((possibleSHA1[i] >= '0' && possibleSHA1[i] <= '9') || (possibleSHA1[i] >= 'a' && possibleSHA1[i] <= 'f') || (possibleSHA1[i] >= 'A' && possibleSHA1[i] <= 'F')))
				return false;
		}
		return true;
	}
};

namespace std
{
	template <>
	struct hash<CGitHash>
	{
		std::size_t operator()(const CGitHash& k) const
		{
			return *(size_t*)k.m_hash;
		}
	};
}
