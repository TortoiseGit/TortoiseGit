// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2020 - TortoiseGit

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
#include "I18NHelper.h"

TEST(I18NHelper, DoVersionStringsMatch)
{
	EXPECT_TRUE(CI18NHelper::DoVersionStringsMatch(L"2.9.0.0", L"2.9.0.0"));
	EXPECT_TRUE(CI18NHelper::DoVersionStringsMatch(L"2.9.0.1", L"2.9.0.0"));
	EXPECT_TRUE(CI18NHelper::DoVersionStringsMatch(L"2.9.0.0", L"2.9.0.1"));

	EXPECT_FALSE(CI18NHelper::DoVersionStringsMatch(L"2.9.0.0", L""));
	EXPECT_FALSE(CI18NHelper::DoVersionStringsMatch(L"2.9.0.0", L"\r\n"));
	EXPECT_FALSE(CI18NHelper::DoVersionStringsMatch(L"2.9.0.0", L"..."));
	EXPECT_FALSE(CI18NHelper::DoVersionStringsMatch(L"2.9.0.0", L"2.8.0.0"));
	EXPECT_FALSE(CI18NHelper::DoVersionStringsMatch(L"2.9.0.0", L"2.9.1.0"));
	EXPECT_FALSE(CI18NHelper::DoVersionStringsMatch(L"2.9.0.0", L"2.9.0.0.1"));

	EXPECT_FALSE(CI18NHelper::DoVersionStringsMatch(L"2.11.0", L"2.11.0"));
}
