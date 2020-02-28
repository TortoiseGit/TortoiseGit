// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2020 - TortoiseGit
// Copyright (C) 2011-2012 - TortoiseSVN

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

#pragma warning(push)
#pragma warning(disable:4566)
TEST(UnicodeUtils, CString)
{
	CStringA result = CUnicodeUtils::GetUTF8(L"");
	EXPECT_EQ(0, result.GetLength());
	EXPECT_STREQ(result, "");
	CStringW resultW = CUnicodeUtils::GetUnicode("");
	EXPECT_EQ(0, resultW.GetLength());
	EXPECT_STREQ(resultW, L"");

	result = CUnicodeUtils::GetUTF8(L"Iñtërnâtiônàlizætiøn");
	EXPECT_EQ(27, result.GetLength());
	EXPECT_STREQ(result, "\x49\xC3\xB1\x74\xC3\xAB\x72\x6E\xC3\xA2\x74\x69\xC3\xB4\x6E\xC3\xA0\x6C\x69\x7A\xC3\xA6\x74\x69\xC3\xB8\x6E");
	resultW = CUnicodeUtils::GetUnicode(result);
	EXPECT_STREQ(resultW, L"Iñtërnâtiônàlizætiøn");
	EXPECT_EQ(20, resultW.GetLength());

	result = CUnicodeUtils::GetUTF8(L"<value>退订</value>");
	resultW = CUnicodeUtils::GetUnicode(result);
	EXPECT_STREQ(resultW, L"<value>退订</value>");

	result = CUnicodeUtils::GetUTF8(L"äöü");
	EXPECT_EQ(6, result.GetLength());
	resultW = CUnicodeUtils::GetUnicode(result);
	EXPECT_STREQ(resultW, L"äöü");
	EXPECT_EQ(3, resultW.GetLength());

	resultW = CUnicodeUtils::GetUnicode("\xE4\xF6\xFC\xDF", 1252);
	EXPECT_EQ(4, resultW.GetLength());
	EXPECT_STREQ(resultW, L"äöüß");
	result = CUnicodeUtils::GetUTF8(resultW);
	EXPECT_STREQ(result, "\xC3\xA4\xC3\xB6\xC3\xBC\xC3\x9F");

	result = CUnicodeUtils::GetUTF8(L"Продолжить выполнение скрипта?");
	resultW = CUnicodeUtils::GetUnicode(result);
	EXPECT_STREQ(resultW, L"Продолжить выполнение скрипта?");

	result = CUnicodeUtils::GetUTF8(L"dvostruki klik za automtsko uključivanje alfa");
	resultW = CUnicodeUtils::GetUnicode(result);
	EXPECT_STREQ(resultW, L"dvostruki klik za automtsko uključivanje alfa");

	result = CUnicodeUtils::GetUTF8(L"包含有错误的结构。");
	resultW = CUnicodeUtils::GetUnicode(result);
	EXPECT_STREQ(resultW, L"包含有错误的结构。");

	result = CUnicodeUtils::GetUTF8(L"个文件，共有 %2!d! 个文件");
	resultW = CUnicodeUtils::GetUnicode(result);
	EXPECT_STREQ(resultW, L"个文件，共有 %2!d! 个文件");

	result = CUnicodeUtils::GetUTF8(L"は予期せぬオブジェクトを含んでいます。");
	resultW = CUnicodeUtils::GetUnicode(result);
	EXPECT_STREQ(resultW, L"は予期せぬオブジェクトを含んでいます。");

	result = CUnicodeUtils::GetUTF8(L"Verify that the correct path and file name are given.");
	EXPECT_STREQ(result, "Verify that the correct path and file name are given.");
	resultW = CUnicodeUtils::GetUnicode(result);
	EXPECT_STREQ(resultW, L"Verify that the correct path and file name are given.");

	result = CUnicodeUtils::GetUTF8(L"\U0002070e"); // 𠜎 is 4-byte utf8
	EXPECT_EQ(4, result.GetLength());
	EXPECT_STREQ(result, "\xf0\xa0\x9c\x8e");
	resultW = CUnicodeUtils::GetUnicode("\xf0\xa0\x9c\x8e");
	EXPECT_STREQ(resultW, L"\U0002070e");
	EXPECT_EQ(2, resultW.GetLength());

	resultW = CUnicodeUtils::GetUnicode("\xfe");
	EXPECT_STREQ(resultW, L"\uFFFD");
	EXPECT_EQ(1, resultW.GetLength());

	resultW = CUnicodeUtils::GetUnicode("\xc3\x28"); // Invalid 2 Octet Sequence
	EXPECT_STREQ(resultW, L"\uFFFD(");
	EXPECT_EQ(2, resultW.GetLength());
}

TEST(UnicodeUtils, Std)
{
	std::string result = CUnicodeUtils::StdGetUTF8(L"");
	EXPECT_EQ(0u, result.size());
	EXPECT_STREQ(result.c_str(), "");
	std::wstring resultW = CUnicodeUtils::StdGetUnicode("");
	EXPECT_EQ(0u, resultW.size());
	EXPECT_STREQ(resultW.c_str(), L"");

	result = CUnicodeUtils::GetUTF8(L"Iñtërnâtiônàlizætiøn");
	EXPECT_EQ(27u, result.size());
	EXPECT_STREQ(result.c_str(), "\x49\xC3\xB1\x74\xC3\xAB\x72\x6E\xC3\xA2\x74\x69\xC3\xB4\x6E\xC3\xA0\x6C\x69\x7A\xC3\xA6\x74\x69\xC3\xB8\x6E");
	resultW = CUnicodeUtils::StdGetUnicode(result);
	EXPECT_STREQ(resultW.c_str(), L"Iñtërnâtiônàlizætiøn");
	EXPECT_EQ(20u, resultW.size());

	result = CUnicodeUtils::StdGetUTF8(L"<value>退订</value>");
	resultW = CUnicodeUtils::StdGetUnicode(result);
	EXPECT_STREQ(resultW.c_str(), L"<value>退订</value>");

	result = CUnicodeUtils::StdGetUTF8(L"äöü");
	EXPECT_EQ(6u, result.size());
	resultW = CUnicodeUtils::StdGetUnicode(result);
	EXPECT_STREQ(resultW.c_str(), L"äöü");
	EXPECT_EQ(3u, resultW.size());

	result = CUnicodeUtils::StdGetUTF8(L"äöüß");
	EXPECT_STREQ(result.c_str(), "\xC3\xA4\xC3\xB6\xC3\xBC\xC3\x9F");

	result = CUnicodeUtils::StdGetUTF8(L"Продолжить выполнение скрипта?");
	resultW = CUnicodeUtils::StdGetUnicode(result);
	EXPECT_STREQ(resultW.c_str(), L"Продолжить выполнение скрипта?");

	result = CUnicodeUtils::StdGetUTF8(L"dvostruki klik za automtsko uključivanje alfa");
	resultW = CUnicodeUtils::StdGetUnicode(result);
	EXPECT_STREQ(resultW.c_str(), L"dvostruki klik za automtsko uključivanje alfa");

	result = CUnicodeUtils::StdGetUTF8(L"包含有错误的结构。");
	resultW = CUnicodeUtils::StdGetUnicode(result);
	EXPECT_STREQ(resultW.c_str(), L"包含有错误的结构。");

	result = CUnicodeUtils::StdGetUTF8(L"个文件，共有 %2!d! 个文件");
	resultW = CUnicodeUtils::StdGetUnicode(result);
	EXPECT_STREQ(resultW.c_str(), L"个文件，共有 %2!d! 个文件");

	result = CUnicodeUtils::StdGetUTF8(L"は予期せぬオブジェクトを含んでいます。");
	resultW = CUnicodeUtils::StdGetUnicode(result);
	EXPECT_STREQ(resultW.c_str(), L"は予期せぬオブジェクトを含んでいます。");

	result = CUnicodeUtils::StdGetUTF8(L"Verify that the correct path and file name are given.");
	EXPECT_STREQ(result.c_str(), "Verify that the correct path and file name are given.");
	resultW = CUnicodeUtils::StdGetUnicode(result);
	EXPECT_STREQ(resultW.c_str(), L"Verify that the correct path and file name are given.");

	result = CUnicodeUtils::StdGetUTF8(L"\U0002070e"); // 𠜎 is 4-byte utf8
	EXPECT_EQ(4u, result.size());
	EXPECT_STREQ(result.c_str(), "\xf0\xa0\x9c\x8e");
	resultW = CUnicodeUtils::StdGetUnicode("\xf0\xa0\x9c\x8e");
	EXPECT_STREQ(resultW.c_str(), L"\U0002070e");
	EXPECT_EQ(2u, resultW.size());

	resultW = CUnicodeUtils::StdGetUnicode("\xfe");
	EXPECT_STREQ(resultW.c_str(), L"\uFFFD");
	EXPECT_EQ(1u, resultW.size());

	resultW = CUnicodeUtils::StdGetUnicode("\xc3\x28"); // Invalid 2 Octet Sequence
	EXPECT_STREQ(resultW.c_str(), L"\uFFFD(");
	EXPECT_EQ(2u, resultW.size());
}
#pragma warning(pop)
