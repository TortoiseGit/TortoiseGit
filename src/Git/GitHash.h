// TortoiseGit - a Windows shell extension for easy version control

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
#define GIT_HASH_SHA1_SIZE 20
#define GIT_HASH_SHA256_SIZE 32
#define GIT_HASH_MAX_SIZE GIT_HASH_SHA256_SIZE

/* also see gitdll.c */
static_assert(sizeof(git_oid::id) == GIT_HASH_MAX_SIZE, "hash size needs to be the same as in libgit2");

#define GIT_REV_SHA1_ZERO_C "0000000000000000000000000000000000000000"
#define GIT_REV_SHA1_ZERO _T(GIT_REV_SHA1_ZERO_C)
#define GIT_REV_SHA256_ZERO_C "0000000000000000000000000000000000000000000000000000000000000000"
#define GIT_REV_SHA256_ZERO _T(GIT_REV_SHA256_ZERO_C)
static_assert(sizeof(GIT_REV_SHA1_ZERO_C) == 2 * GIT_HASH_SHA1_SIZE + 1);
static_assert(sizeof(GIT_REV_SHA256_ZERO_C) == 2 * GIT_HASH_SHA256_SIZE + 1);

/** The type of object id. */
enum class GIT_HASH_TYPE : unsigned char
{
	GIT_HASH_UNKNOWN = 0, /**< UNKNOWN, not initialized yet */
	GIT_HASH_SHA1 = 1, /**< SHA1 */
	GIT_HASH_SHA256 = 2 /**< SHA256 */
};
#define GIT_HASH_DEFAULT GIT_HASH_SHA1

static_assert(GIT_OID_SHA1 == (int)GIT_HASH_TYPE::GIT_HASH_SHA1, "needs to be the same in libgit2");
static_assert(GIT_OID_SHA256 == (int)GIT_HASH_TYPE::GIT_HASH_SHA256, "needs to be the same in libgit2");

class CGitHash;
template<>
struct std::hash<CGitHash>;

class CGitHash
{
private:
	/** type of object id */
	GIT_HASH_TYPE m_type = GIT_HASH_TYPE::GIT_HASH_DEFAULT;

	/** raw binary formatted id */
	unsigned char m_hash[GIT_HASH_MAX_SIZE]{};

public:
	CGitHash() = default;
	CGitHash(GIT_HASH_TYPE type)
		: m_type(type)
	{
		ASSERT(type != GIT_HASH_TYPE::GIT_HASH_UNKNOWN);
	}
	CGitHash(const git_oid* oid)
	{
		git_oid_cpy(reinterpret_cast<git_oid*>(this), oid);
	}
	CGitHash(const git_oid& oid)
		: CGitHash(&oid)
	{
	}
	CGitHash& operator = (const git_oid* oid)
	{
		git_oid_cpy(reinterpret_cast<git_oid*>(this), oid);
		return *this;
	}
	CGitHash& operator = (const git_oid& oid)
	{
		git_oid_cpy(reinterpret_cast<git_oid*>(this), &oid);
		return *this;
	}

	inline constexpr static int HashLength(GIT_HASH_TYPE type)
	{
		if (type == GIT_HASH_TYPE::GIT_HASH_SHA1)
			return GIT_HASH_SHA1_SIZE;
		else if (type == GIT_HASH_TYPE::GIT_HASH_SHA256)
			return GIT_HASH_SHA256_SIZE;

		ASSERT(false);
		return 0;
	}

private:
	inline int HashLength() const
	{
		return HashLength(m_type);
	}

public:
	static CGitHash FromHexStrTry(const CString& str, GIT_HASH_TYPE type)
	{
		if (!IsValidHash(str, type))
			return CGitHash(type);

		return FromHexStr(str, type);
	}

	static CGitHash FromHexStr(const CString& str, GIT_HASH_TYPE type)
	{
		CGitHash hash{ type };
		const int hashLength = HashLength(type);
		for (int i = 0; i < hashLength; ++i)
		{
			unsigned char a;
			a=0;
			for (int j = 2 * i; j <= 2 * i + 1; ++j)
			{
				a =a<<4;

				wchar_t ch = str[j];
				if (ch >= L'0' && ch <= L'9')
					a |= (ch - L'0') & 0xF;
				else if (ch >=L'A' && ch <= L'F')
					a |= ((ch - L'A') & 0xF) + 10 ;
				else if (ch >=L'a' && ch <= L'f')
					a |= ((ch - L'a') & 0xF) + 10;
			}
			hash.m_hash[i] = a;
		}
		return hash;
	}

	static CGitHash FromRaw(const unsigned char* raw, unsigned int type)
	{
		CGitHash hash{ static_cast<GIT_HASH_TYPE>(type) };
		memcpy(hash.m_hash, raw, HashLength(static_cast<GIT_HASH_TYPE>(type)));
		return hash;
	}

	static CGitHash FromHexStr(const char* str, GIT_HASH_TYPE type)
	{
		CGitHash hash{ type };
		const int hashLength = HashLength(type);
		for (int i = 0; i < hashLength; ++i)
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
			hash.m_hash[i] = a;
		}
		return hash;
	}

	void Empty()
	{
		memset(m_hash, 0, GIT_HASH_MAX_SIZE);
	}
	bool IsEmpty() const
	{
		const int hashLength = HashLength(m_type);
		for (int i = 0; i < hashLength; ++i)
		{
			if(m_hash[i] != 0)
				return false;
		}
		return true;
	}

	CString ToString() const
	{
		ASSERT(m_type != GIT_HASH_TYPE::GIT_HASH_UNKNOWN);
		const int hashLength = HashLength(m_type);
		CString str;
		str.Preallocate(hashLength * 2);
		for (int i = 0; i < hashLength; ++i)
			str.AppendFormat(L"%02x", m_hash[i]);
		return str;
	}

	CString ToString(int len) const
	{
		ASSERT(len >= 0 && len <= HashLength() * 2);
		CString str { ToString() };
		str.Truncate(len);
		return str;
	}

	operator const git_oid*() const
	{
		static_assert(sizeof(CGitHash) == sizeof(git_oid), "must be equal");
		static_assert(&(((CGitHash*)nullptr)->m_hash) == &(((git_oid*)nullptr)->id), "must be equal");
		static_assert(&(((CGitHash*)nullptr)->m_type) == (void*)&(((git_oid*)nullptr)->type), "must be equal");
		return reinterpret_cast<const git_oid*>(this);
	}

	const unsigned char* ToRaw() const
	{
		return m_hash;
	}

	int HashType() const
	{
		return static_cast<int>(m_type);
	}

	bool operator == (const CGitHash &hash) const
	{
		ASSERT(m_type != GIT_HASH_TYPE::GIT_HASH_UNKNOWN);
		return memcmp(m_hash, hash.m_hash, HashLength()) == 0 && hash.m_type == m_type;
	}

	static friend bool operator<(const CGitHash& left, const CGitHash& right)
	{
		ASSERT(left.m_type == right.m_type);
		return memcmp(left.m_hash, right.m_hash, HashLength(left.m_type)) < 0;
	}

	static friend bool operator>(const CGitHash& left, const CGitHash& right)
	{
		ASSERT(left.m_type == right.m_type);
		return memcmp(left.m_hash, right.m_hash, HashLength(left.m_type)) > 0;
	}

	static friend bool operator != (const CGitHash& left, const CGitHash& right)
	{
		ASSERT(left.m_type == right.m_type);
		return left.m_type != right.m_type || memcmp(left.m_hash, right.m_hash, HashLength(left.m_type)) != 0;
	}

	bool MatchesPrefix(const CGitHash& hash, const CString& hashString, size_t prefixLen) const
	{
		ASSERT(prefixLen <= 2 * static_cast<size_t>(HashLength()));
		if (memcmp(m_hash, hash.m_hash, prefixLen >> 1))
			return false;
		return prefixLen == 2 * static_cast<size_t>(HashLength()) || wcsncmp(ToString(), hashString, prefixLen) == 0;
	}

	static bool IsValidHash(const CString& possibleHash, GIT_HASH_TYPE type)
	{
		ASSERT(type != GIT_HASH_TYPE::GIT_HASH_UNKNOWN);
		if (possibleHash.GetLength() != 2 * HashLength(type))
			return false;
		for (int i = 0; i < possibleHash.GetLength(); ++i)
		{
			if (!((possibleHash[i] >= '0' && possibleHash[i] <= '9') || (possibleHash[i] >= 'a' && possibleHash[i] <= 'f') || (possibleHash[i] >= 'A' && possibleHash[i] <= 'F')))
				return false;
		}
		return true;
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
			static_assert(sizeof(size_t) <= sizeof(k.m_hash));
			return reinterpret_cast<const size_t&>(k.m_hash);
		}
	};
}
