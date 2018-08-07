// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2015-2018 - TortoiseGit
// Copyright (C) 2003-2008, 2013-2014 - TortoiseSVN

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
#include "PathUtils.h"

TEST(CPathUtils, GetFileNameFromPath)
{
	CString test(L"d:\\test\\filename.ext");
	EXPECT_STREQ(L"filename.ext", CPathUtils::GetFileNameFromPath(test));
	test = L"filename.ext";
	EXPECT_STREQ(L"filename.ext", CPathUtils::GetFileNameFromPath(test));
	test = L"d:/test/filename";
	EXPECT_STREQ(L"filename", CPathUtils::GetFileNameFromPath(test));
	test = L"d:\\test\\filename";
	EXPECT_STREQ(L"filename", CPathUtils::GetFileNameFromPath(test));
	test = L"filename";
	EXPECT_STREQ(L"filename", CPathUtils::GetFileNameFromPath(test));
	test.Empty();
	EXPECT_STREQ(L"", CPathUtils::GetFileNameFromPath(test));
}

TEST(CPathUtils, ExtTest)
{
	CString test(L"d:\\test\\filename.ext");
	EXPECT_STREQ(L".ext", CPathUtils::GetFileExtFromPath(test));
	test = L"filename.ext";
	EXPECT_STREQ(L".ext", CPathUtils::GetFileExtFromPath(test));
	test = L"d:\\test\\filename";
	EXPECT_STREQ(L"", CPathUtils::GetFileExtFromPath(test));
	test = L"filename";
	EXPECT_STREQ(L"", CPathUtils::GetFileExtFromPath(test));
	test.Empty();
	EXPECT_STREQ(L"", CPathUtils::GetFileExtFromPath(test));
}

TEST(CPathUtils, ParseTests)
{
	CString test(L"test 'd:\\testpath with spaces' test");
	EXPECT_STREQ(L"d:\\testpath with spaces", CPathUtils::ParsePathInString(test));
	test = L"d:\\testpath with spaces";
	EXPECT_STREQ(L"d:\\testpath with spaces", CPathUtils::ParsePathInString(test));
}

TEST(CPathUtils, MakeSureDirectoryPathExists)
{
	CAutoTempDir tmpDir;
	EXPECT_TRUE(PathIsDirectory(tmpDir.GetTempDir()));
	EXPECT_FALSE(PathFileExists(tmpDir.GetTempDir() + L"\\sub"));
	EXPECT_FALSE(PathIsDirectory(tmpDir.GetTempDir() + L"\\sub"));

	EXPECT_TRUE(CPathUtils::MakeSureDirectoryPathExists(tmpDir.GetTempDir() + L"\\sub\\sub\\dir"));
	EXPECT_TRUE(PathIsDirectory(tmpDir.GetTempDir() + L"\\sub"));
	EXPECT_TRUE(PathIsDirectory(tmpDir.GetTempDir() + L"\\sub\\sub"));
	EXPECT_TRUE(PathIsDirectory(tmpDir.GetTempDir() + L"\\sub\\sub\\dir"));

	EXPECT_TRUE(CPathUtils::MakeSureDirectoryPathExists(tmpDir.GetTempDir() + L"\\sub/asub/adir"));
	EXPECT_TRUE(PathIsDirectory(tmpDir.GetTempDir() + L"\\sub"));
	EXPECT_TRUE(PathIsDirectory(tmpDir.GetTempDir() + L"\\sub\\asub"));
	EXPECT_TRUE(PathIsDirectory(tmpDir.GetTempDir() + L"\\sub\\asub\\adir"));
}

TEST(CPathUtils, EnsureTrailingPathDelimiter)
{
	CString tPath;
	CPathUtils::EnsureTrailingPathDelimiter(tPath);
	EXPECT_STREQ(tPath, L"");

	tPath = L"C:";
	CPathUtils::EnsureTrailingPathDelimiter(tPath);
	EXPECT_STREQ(tPath, L"C:\\");

	tPath = L"C:\\";
	CPathUtils::EnsureTrailingPathDelimiter(tPath);
	EXPECT_STREQ(tPath, L"C:\\");

	tPath = L"C:\\my\\path";
	CPathUtils::EnsureTrailingPathDelimiter(tPath);
	EXPECT_STREQ(tPath, L"C:\\my\\path\\");

	tPath = L"C:\\my\\path\\";
	CPathUtils::EnsureTrailingPathDelimiter(tPath);
	EXPECT_STREQ(tPath, L"C:\\my\\path\\");
}

TEST(CPathUtils, BuildPathWithPathDelimiter)
{
	EXPECT_STREQ(CPathUtils::BuildPathWithPathDelimiter(L""), L"");
	EXPECT_STREQ(CPathUtils::BuildPathWithPathDelimiter(L"C:"), L"C:\\");
	EXPECT_STREQ(CPathUtils::BuildPathWithPathDelimiter(L"C:\\"), L"C:\\");
	EXPECT_STREQ(CPathUtils::BuildPathWithPathDelimiter(L"C:\\my\\path"), L"C:\\my\\path\\");
	EXPECT_STREQ(CPathUtils::BuildPathWithPathDelimiter(L"C:\\my\\path\\"), L"C:\\my\\path\\");
}

TEST(CPathUtils, TrimTrailingPathDelimiter)
{
	CString tPath;
	CPathUtils::TrimTrailingPathDelimiter(tPath);
	EXPECT_STREQ(tPath, L"");

	tPath = L"C:";
	CPathUtils::TrimTrailingPathDelimiter(tPath);
	EXPECT_STREQ(tPath, L"C:");

	tPath = L"C:\\";
	CPathUtils::TrimTrailingPathDelimiter(tPath);
	EXPECT_STREQ(tPath, L"C:");

	tPath = L"C:\\my\\path";
	CPathUtils::TrimTrailingPathDelimiter(tPath);
	EXPECT_STREQ(tPath, L"C:\\my\\path");

	tPath = L"C:\\my\\path\\";
	CPathUtils::TrimTrailingPathDelimiter(tPath);
	EXPECT_STREQ(tPath, L"C:\\my\\path");

	tPath = L"C:\\my\\path\\\\";
	CPathUtils::TrimTrailingPathDelimiter(tPath);
	EXPECT_STREQ(tPath, L"C:\\my\\path");
}

TEST(CPathUtils, ExpandFileName)
{
	EXPECT_STREQ(CPathUtils::ExpandFileName(L"C:\\my\\path\\da\\da\\da"), L"C:\\my\\path\\da\\da\\da");
	EXPECT_STREQ(CPathUtils::ExpandFileName(L"C:\\my\\path\\da\\da\\da\\"), L"C:\\my\\path\\da\\da\\da\\");

	EXPECT_STREQ(CPathUtils::ExpandFileName(L"C:\\my\\path\\\\da\\da\\da"), L"C:\\my\\path\\da\\da\\da");
	EXPECT_STREQ(CPathUtils::ExpandFileName(L"C:\\my\\path\\.\\da\\da\\da"), L"C:\\my\\path\\da\\da\\da");
	EXPECT_STREQ(CPathUtils::ExpandFileName(L"C:\\my\\path\\.\\da\\da\\da\\"), L"C:\\my\\path\\da\\da\\da\\");

	EXPECT_STREQ(CPathUtils::ExpandFileName(L"C:\\my\\path\\..\\da\\da\\da"), L"C:\\my\\da\\da\\da");
	EXPECT_STREQ(CPathUtils::ExpandFileName(L"C:\\my\\path\\..\\da\\da\\da\\"), L"C:\\my\\da\\da\\da\\");

	// "\\.\\C:\\"
	EXPECT_STREQ(CPathUtils::ExpandFileName(L"\\\\.\\C:\\my\\path\\da\\da\\da"), L"\\\\.\\C:\\my\\path\\da\\da\\da");
	EXPECT_STREQ(CPathUtils::ExpandFileName(L"\\\\.\\C:\\my\\path\\da\\da\\da\\"), L"\\\\.\\C:\\my\\path\\da\\da\\da\\");

	EXPECT_STREQ(CPathUtils::ExpandFileName(L"\\\\.\\C:\\my\\path\\\\da\\da\\da"), L"\\\\.\\C:\\my\\path\\da\\da\\da");
	EXPECT_STREQ(CPathUtils::ExpandFileName(L"\\\\.\\C:\\my\\path\\.\\da\\da\\da"), L"\\\\.\\C:\\my\\path\\da\\da\\da");
	EXPECT_STREQ(CPathUtils::ExpandFileName(L"\\\\.\\C:\\my\\path\\.\\da\\da\\da\\"), L"\\\\.\\C:\\my\\path\\da\\da\\da\\");

	EXPECT_STREQ(CPathUtils::ExpandFileName(L"\\\\.\\C:\\my\\path\\..\\da\\da\\da"), L"\\\\.\\C:\\my\\da\\da\\da");
	EXPECT_STREQ(CPathUtils::ExpandFileName(L"\\\\.\\C:\\my\\path\\..\\da\\da\\da\\"), L"\\\\.\\C:\\my\\da\\da\\da\\");

	// UNC paths
	EXPECT_STREQ(CPathUtils::ExpandFileName(L"\\\\DACOMPUTER\\my\\path\\.\\da\\da\\da"), L"\\\\DACOMPUTER\\my\\path\\da\\da\\da");
	EXPECT_STREQ(CPathUtils::ExpandFileName(L"\\\\DACOMPUTER\\my\\path\\.\\da\\da\\da\\"), L"\\\\DACOMPUTER\\my\\path\\da\\da\\da\\");

	EXPECT_STREQ(CPathUtils::ExpandFileName(L"\\\\DACOMPUTER\\my\\path\\..\\da\\da\\da"), L"\\\\DACOMPUTER\\my\\da\\da\\da");
	EXPECT_STREQ(CPathUtils::ExpandFileName(L"\\\\DACOMPUTER\\my\\path\\..\\da\\da\\da\\"), L"\\\\DACOMPUTER\\my\\da\\da\\da\\");
}

TEST(CPathUtils, IsSamePath)
{
	EXPECT_TRUE(CPathUtils::IsSamePath(L"", L""));

	EXPECT_TRUE(CPathUtils::IsSamePath(L"C:\\my\\path\\da\\da\\da", L"C:\\my\\path\\da\\da\\da"));
	EXPECT_TRUE(CPathUtils::IsSamePath(L"C:\\my\\path\\da\\da\\da", L"c:\\my\\pAth\\DA\\da\\da"));
	EXPECT_TRUE(CPathUtils::IsSamePath(L"C:\\my\\path\\da\\da\\da", L"C:\\my\\path\\da\\da\\da\\"));
	EXPECT_TRUE(CPathUtils::IsSamePath(L"C:\\my\\path\\da\\da\\da", L"C:\\my\\path\\da\\da\\da\\."));
	EXPECT_TRUE(CPathUtils::IsSamePath(L"C:\\my\\path\\da\\da\\da", L"C:\\my\\path\\da\\da\\da\\.\\"));
	EXPECT_TRUE(CPathUtils::IsSamePath(L"C:\\my\\path\\da\\da\\da", L"C:\\my\\path\\.\\.\\da\\da\\da"));
	EXPECT_TRUE(CPathUtils::IsSamePath(L"C:\\my\\path\\da\\da\\da", L"C:\\my\\path\\..\\path\\da\\da\\da"));
	EXPECT_TRUE(CPathUtils::IsSamePath(L"C:\\my\\path\\da\\da\\da", L"C:\\my\\path\\..\\path\\da\\da\\da\\bla\\.."));

	EXPECT_FALSE(CPathUtils::IsSamePath(L"C:\\my\\path\\da\\da\\da", L"C:\\this\\is\\a\\new\\path"));
	EXPECT_FALSE(CPathUtils::IsSamePath(L"C:\\my\\path\\da\\da\\da", L"C:\\my\\path\\da\\da\\da\\.."));
	EXPECT_FALSE(CPathUtils::IsSamePath(L"C:\\my\\path\\da\\da\\da", L"C:\\my\\path\\da\\da\\da\\..\\"));
	EXPECT_FALSE(CPathUtils::IsSamePath(L"C:\\my\\path\\da\\da\\da", L"C:\\my\\path\\..\\path\\da\\da\\da\\.\\da"));
}
