// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2015-2016, 2018-2019 - TortoiseGit

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
#include "GitHash.h"
#include "GitStatus.h"

TEST(CGitHash, Initial)
{
	CGitHash empty;
	EXPECT_TRUE(empty.IsEmpty());
	EXPECT_STREQ(L"0000000000000000000000000000000000000000", GIT_REV_ZERO);
	EXPECT_STREQ(GIT_REV_ZERO, empty.ToString());
	EXPECT_TRUE(empty == empty);
	EXPECT_FALSE(empty != empty);

	CGitHash hash = CGitHash::FromHexStr("8d1861316061748cfee7e075dc138287978102ab");
	EXPECT_FALSE(hash.IsEmpty());
	EXPECT_STREQ(L"8d1861316061748cfee7e075dc138287978102ab", hash.ToString());
	EXPECT_TRUE(hash == hash);
	EXPECT_FALSE(hash != hash);
	EXPECT_FALSE(hash == empty);
	EXPECT_TRUE(hash != empty);

	CGitHash hash2 = CGitHash::FromHexStr(L"8d1861316061748cfee7e075dc138287978102ab");
	EXPECT_FALSE(hash2.IsEmpty());
	EXPECT_STREQ(L"8d1861316061748cfee7e075dc138287978102ab", hash2.ToString());
	EXPECT_TRUE(hash2 == hash);
	EXPECT_FALSE(hash2 != hash);
	EXPECT_FALSE(hash2 == empty);
	EXPECT_TRUE(hash2 != empty);
	hash2.Empty();
	EXPECT_STREQ(GIT_REV_ZERO, hash2.ToString());
	EXPECT_TRUE(hash2 == empty);
	EXPECT_FALSE(hash2 != empty);
	EXPECT_TRUE(hash2.IsEmpty());

	unsigned char chararray[20] = { 0x8D, 0x18, 0x61, 0x31, 0x60, 0x61, 0x74, 0x8C, 0xFE, 0xE7, 0xE0, 0x75, 0xDC, 0x13, 0x82, 0x87, 0x97, 0x81, 0x02, 0xAB };
	CGitHash hash3 = CGitHash::FromRaw(chararray);
	EXPECT_FALSE(hash3.IsEmpty());
	EXPECT_STREQ(L"8d1861316061748cfee7e075dc138287978102ab", hash3.ToString());
	EXPECT_TRUE(hash3 == hash);
	EXPECT_FALSE(hash3 != hash);
	EXPECT_FALSE(hash3 == empty);
	EXPECT_TRUE(hash3 != empty);

	CGitHash hash4;
	hash4 = CGitHash::FromHexStr(L"8d1861316061748cfee7e075dc138287978102ab");
	EXPECT_TRUE(hash4 == hash);

	CGitHash hash5;
	hash5 = hash;
	EXPECT_TRUE(hash5 == hash);

	CGitHash hash6;
	hash6 = CGitHash::FromRaw(chararray);
	EXPECT_TRUE(hash6 == hash);

	CGitHash hash7 = CGitHash::FromHexStrTry(L"invalid");
	EXPECT_TRUE(hash7.IsEmpty());

	CGitHash hash8 = CGitHash::FromHexStrTry(L"01234567");
	EXPECT_TRUE(hash8.IsEmpty());
}

TEST(CGitHash, ToString)
{
	CGitHash hash = CGitHash::FromHexStr(L"0123456789abcdef0123456789abcdef01234567");

	EXPECT_STREQ(L"0123456789abcdef0123456789abcdef01234567", hash.ToString());

	EXPECT_STREQ(L"", hash.ToString(0));
	EXPECT_STREQ(L"0", hash.ToString(1));
	EXPECT_STREQ(L"01", hash.ToString(2));
	EXPECT_STREQ(L"01234", hash.ToString(5));
	EXPECT_STREQ(L"0123456", hash.ToString(7));
	EXPECT_STREQ(L"0123456789", hash.ToString(10));
	EXPECT_STREQ(L"0123456789abcdef0123", hash.ToString(20));
	EXPECT_STREQ(L"0123456789abcdef0123456789abcdef01234567", hash.ToString(40));
}

TEST(CGitHash, IsSHA1Valid)
{
	EXPECT_TRUE(CGitHash::IsValidSHA1(GIT_REV_ZERO));
	EXPECT_TRUE(CGitHash::IsValidSHA1(L"8d1861316061748cfee7e075dc138287978102ab"));
	EXPECT_TRUE(CGitHash::IsValidSHA1(L"8d1861316061748cfee7E075dc138287978102ab"));
	EXPECT_TRUE(CGitHash::IsValidSHA1(L"8D1861316061748CFEE7E075DC138287978102AB"));
	EXPECT_FALSE(CGitHash::IsValidSHA1(L""));
	EXPECT_FALSE(CGitHash::IsValidSHA1(L"8d18613"));
	EXPECT_FALSE(CGitHash::IsValidSHA1(L"master"));
	EXPECT_FALSE(CGitHash::IsValidSHA1(L"refs/heads/master"));
	EXPECT_FALSE(CGitHash::IsValidSHA1(L"8d1861316061748cfee7e075dc138287978102az"));
}

TEST(CGitHash, MatchesPrefix)
{
	CString prefix = L"3012b757c23d16cc915acf60f5e3922d0409187a";
	CGitHash hash = CGitHash::FromHexStr(prefix);
	CGitHash prefixHash = CGitHash::FromHexStr(prefix);
	EXPECT_TRUE(hash.MatchesPrefix(prefixHash, prefix, prefix.GetLength()));

	prefix = L"";
	prefixHash = CGitHash::FromHexStr(L"0000000000000000000000000000000000000000");
	EXPECT_TRUE(hash.MatchesPrefix(prefixHash, prefix, prefix.GetLength()));

	prefix = L"3012b757";
	prefixHash = CGitHash::FromHexStr(L"3012b75700000000000000000000000000000000");
	EXPECT_TRUE(hash.MatchesPrefix(prefixHash, prefix, prefix.GetLength()));

	prefix = L"3012b758";
	prefixHash = CGitHash::FromHexStr(L"3012b75800000000000000000000000000000000");
	EXPECT_FALSE(hash.MatchesPrefix(prefixHash, prefix, prefix.GetLength()));

	prefix = L"a0";
	prefixHash = CGitHash::FromHexStr(L"a000000000000000000000000000000000000000");
	EXPECT_FALSE(hash.MatchesPrefix(prefixHash, prefix, prefix.GetLength()));

	prefix = L"3012b75";
	prefixHash = CGitHash::FromHexStr(L"3012b75000000000000000000000000000000000");
	EXPECT_TRUE(hash.MatchesPrefix(prefixHash, prefix, prefix.GetLength()));

	prefix = L"3012b76";
	prefixHash = CGitHash::FromHexStr(L"3012b76000000000000000000000000000000000");
	EXPECT_FALSE(hash.MatchesPrefix(prefixHash, prefix, prefix.GetLength()));
}

TEST(CGitHash, stdhash)
{
	EXPECT_EQ(std::hash<CGitHash>()(CGitHash::FromHexStr(GIT_REV_ZERO)), size_t(0));
	EXPECT_EQ(std::hash<CGitHash>()(CGitHash::FromHexStr(L"ffffffffffffffff000000000000000000000000")), SIZE_T_MAX);

	EXPECT_EQ(std::hash<CGitHash>()(CGitHash::FromHexStr(L"1000000000000000000000000000000000000000")), size_t(16));
	EXPECT_EQ(std::hash<CGitHash>()(CGitHash::FromHexStr(L"0100000000000000000000000000000000000000")), size_t(1));
	EXPECT_EQ(std::hash<CGitHash>()(CGitHash::FromHexStr(L"0010000000000000000000000000000000000000")), size_t(4096));
	EXPECT_EQ(std::hash<CGitHash>()(CGitHash::FromHexStr(L"0001000000000000000000000000000000000000")), size_t(256));
	EXPECT_EQ(std::hash<CGitHash>()(CGitHash::FromHexStr(L"0000100000000000000000000000000000000000")), size_t(1048576));
	EXPECT_EQ(std::hash<CGitHash>()(CGitHash::FromHexStr(L"0000000100000000000000000000000000000000")), size_t(16777216));
#ifdef WIN64
	EXPECT_EQ(std::hash<CGitHash>()(CGitHash::FromHexStr(L"0000000010000000000000000000000000000000")), size_t(68719476736));
	EXPECT_EQ(std::hash<CGitHash>()(CGitHash::FromHexStr(L"0000000000000010000000000000000000000000")), size_t(1152921504606846976));
	EXPECT_EQ(std::hash<CGitHash>()(CGitHash::FromHexStr(L"0000000000000001000000000000000000000000")), size_t(72057594037927936));
#else
	EXPECT_EQ(std::hash<CGitHash>()(CGitHash::FromHexStr(L"0000000010000000000000000000000000000000")), size_t(0));
	EXPECT_EQ(std::hash<CGitHash>()(CGitHash::FromHexStr(L"0000000000000010000000000000000000000000")), size_t(0));
	EXPECT_EQ(std::hash<CGitHash>()(CGitHash::FromHexStr(L"0000000000000001000000000000000000000000")), size_t(0));
#endif
	EXPECT_EQ(std::hash<CGitHash>()(CGitHash::FromHexStr(L"0000000000000000100000000000000000000000")), size_t(0));
}
