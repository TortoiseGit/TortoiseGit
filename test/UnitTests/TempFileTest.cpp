// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2019 - TortoiseGit

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
#include "TempFile.h"

TEST(CTempFiles, UniqueName)
{
	auto path1 = CTempFiles::Instance().GetTempFilePath(false, CTGitPath(L"something.cpp"));
	EXPECT_TRUE(path1.Exists());
	auto path2 = CTempFiles::Instance().GetTempFilePath(false, CTGitPath(L"something.cpp"));
	EXPECT_TRUE(path2.Exists());

	EXPECT_STRNE(path1.GetWinPathString(), path2.GetWinPathString());

	path1.Delete(false, false);
	path2.Delete(false, false);
}

TEST(CTempFiles, LongName)
{
	auto path = CTempFiles::Instance().GetTempFilePath(false, CTGitPath(L"something012345789012345789012345789012345789012345789012345789012345789012345789012345789012345789-100-012345789012345789012345789012345789012345789012345789012345789012345789012345789012345789-200-012345789012345789012345789012345789012345789012345789-256.extension"));
	ASSERT_LT(path.GetWinPathString().GetLength(), MAX_PATH);
	EXPECT_TRUE(path.Exists());

	path.Delete(false, false);
}

TEST(CTempFiles, ValidName)
{
	auto path1 = CTempFiles::Instance().GetTempFilePath(false, CTGitPath(L"invalid?file|name.txt"), CGitHash::FromHexStr(L"012345678901234567890abcdef123456789abfd"));
	EXPECT_TRUE(path1.Exists());
	path1.Delete(false, false);

	auto path2 = CTempFiles::Instance().GetTempFilePath(false, CTGitPath(L">"));
	EXPECT_TRUE(path2.Exists());
	path2.Delete(false, false);

	auto path3 = CTempFiles::Instance().GetTempFilePath(false, CTGitPath(L".gitattribute"));
	EXPECT_TRUE(path3.Exists());
	path3.Delete(false, false);
}
