// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2015-2016 - TortoiseGit

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

	CGitHash hash;
	hash.ConvertFromStrA("8d1861316061748cfee7e075dc138287978102ab");
	EXPECT_FALSE(hash.IsEmpty());
	EXPECT_STREQ(L"8d1861316061748cfee7e075dc138287978102ab", hash.ToString());
	EXPECT_TRUE(hash == hash);
	EXPECT_FALSE(hash != hash);
	EXPECT_FALSE(hash == empty);
	EXPECT_TRUE(hash != empty);

	CGitHash hash2(L"8d1861316061748cfee7e075dc138287978102ab");
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
	CGitHash hash3((char*)chararray);
	EXPECT_FALSE(hash3.IsEmpty());
	EXPECT_STREQ(L"8d1861316061748cfee7e075dc138287978102ab", hash3.ToString());
	EXPECT_TRUE(hash3 == hash);
	EXPECT_FALSE(hash3 != hash);
	EXPECT_FALSE(hash3 == empty);
	EXPECT_TRUE(hash3 != empty);

	CGitHash hash4;
	hash4 = L"8d1861316061748cfee7e075dc138287978102ab";
	EXPECT_TRUE(hash4 == hash);

	CGitHash hash5;
	hash5 = hash;
	EXPECT_TRUE(hash5 == hash);

	CGitHash hash6;
	hash6 = chararray;
	EXPECT_TRUE(hash6 == hash);

	CGitHash hash7(L"invalid");
	EXPECT_TRUE(hash7.IsEmpty());

	CGitHash hash8(L"01234567");
	EXPECT_TRUE(hash8.IsEmpty());
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
