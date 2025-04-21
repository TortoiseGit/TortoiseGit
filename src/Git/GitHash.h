﻿// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2023, 2025 - TortoiseGit

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

/* also see gitdll.c */
static_assert(sizeof(git_oid) == GIT_HASH_SIZE, "hash size needs to be the same as in libgit2");
static_assert(sizeof(git_oid) == GIT_HASH_SIZE, "hash size needs to be the same as in libgit2");
static_assert(sizeof(git_oid::id) == GIT_HASH_SIZE, "hash size needs to be the same as in libgit2");

#define GIT_REV_ZERO_C "0000000000000000000000000000000000000000"
#define GIT_REV_ZERO _T(GIT_REV_ZERO_C)

class CGitHash;
template<>
struct std::hash<CGitHash>;

class CGitHash
{
private:
	unsigned char m_hash[GIT_HASH_SIZE]{};

public:
	CGitHash() = default;
	CGitHash(const git_oid* oid)
	{
		git_oid_cpy(reinterpret_cast<git_oid*>(m_hash), oid);
	}
	CGitHash(const git_oid& oid)
	{
		git_oid_cpy(reinterpret_cast<git_oid*>(m_hash), &oid);
	}
	CGitHash& operator = (const git_oid* oid)
	{
		git_oid_cpy(reinterpret_cast<git_oid*>(m_hash), oid);
		return *this;
	}
	CGitHash& operator = (const git_oid& oid)
	{
		git_oid_cpy(reinterpret_cast<git_oid*>(m_hash), &oid);
		return *this;
	}

#ifdef TGIT_TESTS_ONLY
	static CGitHash FromHexStr(const wchar_t* str, bool* isHash = nullptr)
	{
		return FromHexStr(std::wstring_view(str), isHash);
	}
#endif

	template <typename T>
	static CGitHash FromHexStr(const T& str, bool* isHash = nullptr)
	{
		static_assert(std::is_assignable_v<T, const CString&> || std::is_assignable_v<T, const CStringA&> || std::is_assignable_v<T, const std::string_view&> || std::is_assignable_v<T, const std::wstring_view&>, "only applicable to 'const CString&', 'const CStringA&', 'std::string_view&' and 'std::string_view&'");
		if constexpr (!(std::is_assignable_v<T, const std::string_view&> || std::is_assignable_v<T, const std::wstring_view&>))
		{
			if (str.GetLength() != 2 * GIT_HASH_SIZE)
			{
				if (isHash)
					*isHash = false;
				return CGitHash();
			}
		}
		else
		{
			if (str.size() != 2 * GIT_HASH_SIZE)
			{
				if (isHash)
					*isHash = false;
				return CGitHash();
			}
		}

		CGitHash hash;
		for (int i = 0; i < GIT_HASH_SIZE; ++i)
		{
			unsigned char a = 0;
			for (int j = 2 * i; j <= 2 * i + 1; ++j)
			{
				a = a << 4;

				const auto ch = str[j];
				static_assert('_' == L'_', "This method expects that char and wchar_t literals are comparable for ASCII characters");
				if (ch >= '0' && ch <= '9')
					a |= (ch - '0') & 0xF;
				else if (ch >= 'A' && ch <= 'F')
					a |= ((ch - 'A') & 0xF) + 10;
				else if (ch >= 'a' && ch <= 'f')
					a |= ((ch - 'a') & 0xF) + 10;
				else
				{
					if (isHash)
						*isHash = false;
					return CGitHash();
				}
			}
			hash.m_hash[i] = a;
		}
		if (isHash)
			*isHash = true;
		return hash;
	}

	static CGitHash FromRaw(const unsigned char* raw)
	{
		CGitHash hash;
		memcpy(hash.m_hash, raw, GIT_HASH_SIZE);
		return hash;
	}

	void Empty()
	{
		memset(m_hash,0, GIT_HASH_SIZE);
	}
	inline bool IsEmpty() const
	{
		static const unsigned char empty[GIT_HASH_SIZE]{};
		return memcmp(m_hash, empty, GIT_HASH_SIZE) == 0;
	}

	CString ToString() const
	{
		CString str;
		str.Preallocate(GIT_HASH_SIZE * 2);
		for (int i = 0; i < GIT_HASH_SIZE; ++i)
			str.AppendFormat(L"%02x", m_hash[i]);
		return str;
	}

	CString ToString(int len) const
	{
		ASSERT(len >= 0 && len <= GIT_HASH_SIZE * 2);
		CString str { ToString() };
		str.Truncate(len);
		return str;
	}

	operator bool() = delete;

	operator const git_oid*() const
	{
		return reinterpret_cast<const git_oid*>(m_hash);
	}

	const unsigned char* ToRaw() const
	{
		return m_hash;
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

	friend struct std::hash<CGitHash>;
};

namespace std
{
	template <>
	struct hash<CGitHash>
	{
		std::size_t operator()(const CGitHash& k) const
		{
			return reinterpret_cast<const size_t&>(k.m_hash);
		}
	};
}
