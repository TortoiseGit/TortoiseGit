// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2016, 2019 - TortoiseGit

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
#include "PersonalDictionary.h"
#include "StringUtils.h"
#include "PathUtils.h"

#define LCID_INVALID 11 // use 11 here which is not a valid language code so that we don't overwrite dictionaries

static CString GetPath(LONG languageID)
{
	CString path;
	path.Format(L"%s%ld.dic", static_cast<LPCTSTR>(CPathUtils::GetAppDataDirectory()), languageID);
	return path;
}

TEST(CPersonalDictionary, UseDictionary)
{
	ASSERT_FALSE(PathFileExists(GetPath(LCID_INVALID)));
	CPersonalDictionary dict;
	dict.Init(LCID_INVALID);

	for (const CString& word : { L"", L"Test", L"Täst", L"рефакторинг" })
		EXPECT_FALSE(dict.FindWord(word));

	const CString in[] = { L"Test", L"Täst", L"рефакторинг" };

	EXPECT_FALSE(dict.AddWord(L""));
	for (const CString& word : in)
	{
		EXPECT_TRUE(dict.AddWord(word));
		EXPECT_TRUE(dict.FindWord(word));
		EXPECT_TRUE(dict.AddWord(word));
	}

	TCHAR tooLong[PDICT_MAX_WORD_LENGTH + 5] = { 0 };
	for (int i = 0; i < _countof(tooLong); ++i)
		tooLong[i] = L'a';
	EXPECT_FALSE(dict.AddWord(tooLong));
	EXPECT_FALSE(dict.FindWord(tooLong));

	// lowercase words
	EXPECT_FALSE(dict.FindWord(L"test"));
	EXPECT_TRUE(dict.AddWord(L"test"));

	EXPECT_TRUE(dict.Save());
	EXPECT_TRUE(dict.FindWord(L"test")); // Save does not clear the list
	EXPECT_TRUE(PathFileExists(GetPath(LCID_INVALID)));

	// load safed words in ne dictionary
	CPersonalDictionary dict2;
	dict2.Init(LCID_INVALID);
	for (const CString& word : in)
		EXPECT_TRUE(dict2.FindWord(word));
	EXPECT_TRUE(dict2.FindWord(L"test"));
	EXPECT_FALSE(dict2.FindWord(L"test2"));

	EXPECT_TRUE(DeleteFile(GetPath(LCID_INVALID)));
}

TEST(CPersonalDictionary, LoadDictionary)
{
	ASSERT_FALSE(PathFileExists(GetPath(LCID_INVALID)));
	CPersonalDictionary dict;
	dict.Init(LCID_INVALID);

	ASSERT_TRUE(CStringUtils::WriteStringToTextFile(GetPath(LCID_INVALID), L"\nTest\n\u0440\u0435\u0444\u0430\u043A\u0442\u043E\u0440\u0438\u043D\u0433\r\nTäst\n"));

	EXPECT_FALSE(dict.FindWord(L""));
	for (const CString& word : { L"Test", L"Täst", L"\u0440\u0435\u0444\u0430\u043A\u0442\u043E\u0440\u0438\u043D\u0433" })
		EXPECT_TRUE(dict.FindWord(word));

	EXPECT_FALSE(dict.FindWord(L"something"));

	EXPECT_TRUE(DeleteFile(GetPath(LCID_INVALID)));
}
