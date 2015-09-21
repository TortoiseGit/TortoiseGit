// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2015 - TortoiseGit
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

TEST(CPathUtils, UnescapeTest)
{
	CString test(_T("file:///d:/REpos1/uCOS-100/Trunk/name%20with%20spaces/NewTest%20%%20NewTest"));
	CString test2 = CPathUtils::PathUnescape(test);
	EXPECT_TRUE(test2.Compare(_T("file:///d:/REpos1/uCOS-100/Trunk/name with spaces/NewTest % NewTest")) == 0);
	CStringA test3 = CPathUtils::PathEscape("file:///d:/REpos1/uCOS-100/Trunk/name with spaces/NewTest % NewTest");
	EXPECT_TRUE(test3.Compare("file:///d:/REpos1/uCOS-100/Trunk/name%20with%20spaces/NewTest%20%%20NewTest") == 0);
}

TEST(CPathUtils, ExtTest)
{
	CString test(_T("d:\\test\filename.ext"));
	EXPECT_TRUE(CPathUtils::GetFileExtFromPath(test).Compare(_T(".ext")) == 0);
	test = _T("filename.ext");
	EXPECT_TRUE(CPathUtils::GetFileExtFromPath(test).Compare(_T(".ext")) == 0);
	test = _T("d:\\test\filename");
	EXPECT_TRUE(CPathUtils::GetFileExtFromPath(test).IsEmpty());
	test = _T("filename");
	EXPECT_TRUE(CPathUtils::GetFileExtFromPath(test).IsEmpty());
}

TEST(CPathUtils, ParseTests)
{
	CString test(_T("test 'd:\\testpath with spaces' test"));
	EXPECT_TRUE(CPathUtils::ParsePathInString(test).Compare(_T("d:\\testpath with spaces")) == 0);
	test = _T("d:\\testpath with spaces");
	EXPECT_TRUE(CPathUtils::ParsePathInString(test).Compare(_T("d:\\testpath with spaces")) == 0);
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
