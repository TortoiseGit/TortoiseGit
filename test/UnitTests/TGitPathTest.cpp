﻿// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2015-2023, 2025 - TortoiseGit
// Copyright (C) 2003-2008 - TortoiseSVN

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
#include "TGitPath.h"
#include "Git.h"
#include "StringUtils.h"
#include "PreserveChdir.h"
#include "AutoTempDir.h"

TEST(CTGitPath, GetDirectoryTest)
{
	// Bit tricky, this test, because we need to know something about the file
	// layout on the machine which is running the test
	TCHAR winDir[MAX_PATH + 1] = { 0 };
	ASSERT_NE(0u, GetWindowsDirectory(winDir, _countof(winDir)));
	CString sWinDir(winDir);

	CTGitPath testPath;
	// This is a file which we know will always be there
	testPath.SetFromUnknown(sWinDir + L"\\win.ini");
	EXPECT_FALSE(testPath.IsDirectory());
	EXPECT_STREQ(sWinDir,testPath.GetDirectory().GetWinPathString());
	EXPECT_STREQ(sWinDir, testPath.GetContainingDirectory().GetWinPathString());

	// Now do the test on the win directory itself - It's hard to be sure about the containing directory
	// but we know it must be different to the directory itself
	testPath.SetFromUnknown(sWinDir);
	EXPECT_TRUE(testPath.IsDirectory());
	EXPECT_STREQ(sWinDir, testPath.GetDirectory().GetWinPathString());
	EXPECT_STRNE(sWinDir, testPath.GetContainingDirectory().GetWinPathString());
	EXPECT_GT(sWinDir.GetLength(), testPath.GetContainingDirectory().GetWinPathString().GetLength());

	// Try a root path
	testPath.SetFromUnknown(L"C:\\");
	EXPECT_TRUE(testPath.IsDirectory());
	EXPECT_TRUE(testPath.GetDirectory().GetWinPathString().CompareNoCase(L"C:\\") == 0);
	EXPECT_TRUE(testPath.GetContainingDirectory().IsEmpty());
	// Try a root UNC path
	testPath.SetFromUnknown(L"\\MYSTATION");
	EXPECT_TRUE(testPath.GetContainingDirectory().IsEmpty());

	// test the UI path methods
	testPath.SetFromUnknown(L"c:\\testing%20test");
	EXPECT_TRUE(testPath.GetUIFileOrDirectoryName().CompareNoCase(L"testing%20test") == 0);

	//testPath.SetFromUnknown(L"http://server.com/testing%20special%20chars%20%c3%a4%c3%b6%c3%bc");
	//EXPECT_TRUE(testPath.GetUIFileOrDirectoryName().CompareNoCase(L"testing special chars \344\366\374") == 0);
}

TEST(CTGitPath, AdminDirTest)
{
	CTGitPath testPath;
	testPath.SetFromUnknown(L"c:\\.gitdir");
	EXPECT_FALSE(testPath.IsAdminDir());
	testPath.SetFromUnknown(L"c:\\test.git");
	EXPECT_FALSE(testPath.IsAdminDir());
	testPath.SetFromUnknown(L"c:\\.git");
	EXPECT_TRUE(testPath.IsAdminDir());
	testPath.SetFromUnknown(L"c:\\.gitdir\\test");
	EXPECT_FALSE(testPath.IsAdminDir());
	testPath.SetFromUnknown(L"c:\\.git\\test");
	EXPECT_TRUE(testPath.IsAdminDir());

	CTGitPathList pathList;
	pathList.AddPath(CTGitPath(L"c:\\.gitdir"));
	pathList.AddPath(CTGitPath(L"c:\\.git"));
	pathList.AddPath(CTGitPath(L"c:\\.git\\test"));
	pathList.AddPath(CTGitPath(L"c:\\test"));
	pathList.RemoveAdminPaths();
	EXPECT_EQ(2, pathList.GetCount());
	pathList.Clear();
	EXPECT_EQ(0, pathList.GetCount());
	pathList.AddPath(CTGitPath(L"c:\\test"));
	pathList.RemoveAdminPaths();
	EXPECT_EQ(1, pathList.GetCount());
}

TEST(CTGitPath, SortTest)
{
	CTGitPathList testList;
	CTGitPath testPath;
	testPath.SetFromUnknown(L"c:/Z");
	testList.AddPath(testPath);
	testPath.SetFromUnknown(L"c:/B");
	testList.AddPath(testPath);
	testPath.SetFromUnknown(L"c:\\a");
	testList.AddPath(testPath);
	testPath.SetFromUnknown(L"c:/Test");
	testList.AddPath(testPath);

	EXPECT_EQ(4, testList.GetCount());

	testList.SortByPathname();

	EXPECT_EQ(4, testList.GetCount());
	EXPECT_STREQ(L"c:\\a", testList[0].GetWinPathString());
	EXPECT_STREQ(L"c:\\B", testList[1].GetWinPathString());
	EXPECT_STREQ(L"c:\\Test", testList[2].GetWinPathString());
	EXPECT_STREQ(L"c:\\Z",testList[3].GetWinPathString());
}

TEST(CTGitPath, RawAppendTest)
{
	CTGitPath testPath(L"c:/test/");
	testPath.AppendRawString(L"/Hello");
	EXPECT_STREQ(L"c:\\test\\Hello", testPath.GetWinPathString());

	testPath.AppendRawString(L"\\T2");
	EXPECT_STREQ(L"c:\\test\\Hello\\T2", testPath.GetWinPathString());

	CTGitPath testFilePath(L"C:\\windows\\win.ini");
	CTGitPath testBasePath(L"c:/temp/myfile.txt");
	testBasePath.AppendRawString(testFilePath.GetFileExtension());
	EXPECT_STREQ(L"c:\\temp\\myfile.txt.ini", testBasePath.GetWinPathString());
}

TEST(CTGitPath, PathAppendTest)
{
	CTGitPath testPath(L"c:/test/");
	testPath.AppendPathString(L"/Hello");
	EXPECT_STREQ(L"c:\\test\\Hello", testPath.GetWinPathString());

	testPath.AppendPathString(L"T2");
	EXPECT_STREQ(L"c:\\test\\Hello\\T2", testPath.GetWinPathString());

	CTGitPath testFilePath(L"C:\\windows\\win.ini");
	CTGitPath testBasePath(L"c:/temp/myfile.txt");
	// You wouldn't want to do this in real life - you'd use append-raw
	testBasePath.AppendPathString(testFilePath.GetFileExtension());
	EXPECT_STREQ(L"c:\\temp\\myfile.txt\\.ini", testBasePath.GetWinPathString());
	EXPECT_STREQ(L"c:/temp/myfile.txt/.ini", testBasePath.GetGitPathString());
}

TEST(CTGitPath, RemoveDuplicatesTest)
{
	CTGitPathList list;
	list.AddPath(CTGitPath(L"Z"));
	list.AddPath(CTGitPath(L"A"));
	list.AddPath(CTGitPath(L"E"));
	list.AddPath(CTGitPath(L"E"));

	EXPECT_TRUE(list[2].IsEquivalentTo(list[3]));
	EXPECT_EQ(list[2], list[3]);

	EXPECT_EQ(4, list.GetCount());

	list.RemoveDuplicates();

	EXPECT_EQ(3, list.GetCount());

	EXPECT_STREQ(L"A", list[0].GetWinPathString());
	EXPECT_STREQ(L"E", list[1].GetWinPathString());
	EXPECT_STREQ(L"Z", list[2].GetWinPathString());
}

TEST(CTGitPath, RemoveChildrenTest)
{
	CTGitPathList list;
	list.AddPath(CTGitPath(L"c:\\test"));
	list.AddPath(CTGitPath(L"c:\\test\\file"));
	list.AddPath(CTGitPath(L"c:\\testfile"));
	list.AddPath(CTGitPath(L"c:\\parent"));
	list.AddPath(CTGitPath(L"c:\\parent\\child"));
	list.AddPath(CTGitPath(L"c:\\parent\\child1"));
	list.AddPath(CTGitPath(L"c:\\parent\\child2"));

	EXPECT_EQ(7, list.GetCount());

	list.RemoveChildren();

	EXPECT_EQ(3, list.GetCount());

	list.SortByPathname();

	EXPECT_STREQ(L"c:\\parent", list[0].GetWinPathString());
	EXPECT_STREQ(L"c:\\test", list[1].GetWinPathString());
	EXPECT_STREQ(L"c:\\testfile", list[2].GetWinPathString());

	list = {};
	list.AddPath(CTGitPath(L"D:\\bar"));
	list.AddPath(CTGitPath(L"D:\\bar.txt"));
	list.AddPath(CTGitPath(L"D:\\bar\\blah"));
	EXPECT_EQ(3, list.GetCount());
	list.RemoveChildren();
	EXPECT_EQ(2, list.GetCount());
}

TEST(CTGitPath, ContainingDirectoryTest)
{
	CTGitPath testPath;
	testPath.SetFromWin(L"c:\\a\\b\\c\\d\\e");
	CTGitPath dir;
	dir = testPath.GetContainingDirectory();
	EXPECT_STREQ(L"c:\\a\\b\\c\\d", dir.GetWinPathString());
	dir = dir.GetContainingDirectory();
	EXPECT_STREQ(L"c:\\a\\b\\c", dir.GetWinPathString());
	dir = dir.GetContainingDirectory();
	EXPECT_STREQ(L"c:\\a\\b", dir.GetWinPathString());
	dir = dir.GetContainingDirectory();
	EXPECT_STREQ(L"c:\\a", dir.GetWinPathString());
	dir = dir.GetContainingDirectory();
	EXPECT_STREQ(L"c:\\", dir.GetWinPathString());
	dir = dir.GetContainingDirectory();
	EXPECT_TRUE(dir.IsEmpty());
	EXPECT_STREQ(L"", dir.GetWinPathString());
}

TEST(CTGitPath, AncestorTest)
{
	CTGitPath testPath;
	testPath.SetFromWin(L"c:\\windows");
	EXPECT_FALSE(testPath.IsAncestorOf(CTGitPath(L"c:\\")));
	EXPECT_TRUE(testPath.IsAncestorOf(CTGitPath(L"c:\\windows")));
	EXPECT_FALSE(testPath.IsAncestorOf(CTGitPath(L"c:\\windows test")));
	EXPECT_FALSE(testPath.IsAncestorOf(CTGitPath(L"c:\\windowsdummy")));
	EXPECT_TRUE(testPath.IsAncestorOf(CTGitPath(L"c:\\windows\\test.txt")));
	EXPECT_TRUE(testPath.IsAncestorOf(CTGitPath(L"c:\\windows\\system32\\test.txt")));

	testPath.SetFromWin(L"");
	EXPECT_FALSE(testPath.IsAncestorOf(CTGitPath(L"c:\\windows\\test.txt")));
	EXPECT_TRUE(testPath.IsAncestorOf(CTGitPath(L"test.txt")));
}

/*TEST(CTGitPath, SubversionPathTest)
{
	CTGitPath testPath;
	testPath.SetFromWin(L"c:\\");
	EXPECT_TRUE((testPath.GetGitApiPath(pool), "c:") == 0);
	testPath.SetFromWin(L"c:\\folder");
	EXPECT_TRUE(strcmp(testPath.GetGitApiPath(pool), "c:/folder") == 0);
	testPath.SetFromWin(L"c:\\a\\b\\c\\d\\e");
	EXPECT_TRUE(strcmp(testPath.GetGitApiPath(pool), "c:/a/b/c/d/e") == 0);
	testPath.SetFromUnknown(L"http://testing/");
	EXPECT_TRUE(strcmp(testPath.GetGitApiPath(pool), "http://testing") == 0);
	testPath.SetFromGit(NULL);
	EXPECT_TRUE(strlen(testPath.GetGitApiPath(pool)) == 0);

	testPath.SetFromUnknown(L"http://testing again");
	EXPECT_TRUE(strcmp(testPath.GetGitApiPath(pool), "http://testing%20again") == 0);
	testPath.SetFromUnknown(L"http://testing%20again");
	EXPECT_TRUE(strcmp(testPath.GetGitApiPath(pool), "http://testing%20again") == 0);
	testPath.SetFromUnknown(L"http://testing special chars \344\366\374");
	EXPECT_TRUE(strcmp(testPath.GetGitApiPath(pool), "http://testing%20special%20chars%20%c3%a4%c3%b6%c3%bc") == 0);
}*/

TEST(CTGitPath, GetCommonRootTest)
{
	CTGitPath pathA(L"C:\\Development\\LogDlg.cpp");
	CTGitPath pathB(L"C:\\Development\\LogDlg.h");
	CTGitPath pathC(L"C:\\Development\\SomeDir\\LogDlg.h");

	CTGitPathList list;
	list.AddPath(pathA);
	EXPECT_TRUE(list.GetCommonRoot().GetWinPathString().CompareNoCase(L"C:\\Development\\LogDlg.cpp") == 0);
	list.AddPath(pathB);
	EXPECT_TRUE(list.GetCommonRoot().GetWinPathString().CompareNoCase(L"C:\\Development") == 0);
	list.AddPath(pathC);
	EXPECT_TRUE(list.GetCommonRoot().GetWinPathString().CompareNoCase(L"C:\\Development") == 0);

	list.Clear();
	CString sPathList = L"D:\\Development\\StExBar\\StExBar\\src\\setup\\Setup64.wxs*D:\\Development\\StExBar\\StExBar\\src\\setup\\Setup.wxs*D:\\Development\\StExBar\\SKTimeStamp\\src\\setup\\Setup.wxs*D:\\Development\\StExBar\\SKTimeStamp\\src\\setup\\Setup64.wxs";
	list.LoadFromAsteriskSeparatedString(sPathList);
	EXPECT_TRUE(list.GetCommonRoot().GetWinPathString().CompareNoCase(L"D:\\Development\\StExBar") == 0);

	list.Clear();
	sPathList = L"D:\\Development\\StEYBar\\StExBar\\src\\setup\\Setup64.wxs*D:\\Development\\StExBar\\StExBar\\src\\setup\\Setup.wxs*D:\\Development\\StExBar\\SKTimeStamp\\src\\setup\\Setup.wxs*D:\\Development\\StExBar\\SKTimeStamp\\src\\setup\\Setup64.wxs";
	list.LoadFromAsteriskSeparatedString(sPathList);
	EXPECT_TRUE(list.GetCommonRoot().GetWinPathString().CompareNoCase(L"D:\\Development") == 0);

	list.Clear();
	sPathList = L"c:\\windows\\explorer.exe*c:\\windows";
	list.LoadFromAsteriskSeparatedString(sPathList);
	EXPECT_TRUE(list.GetCommonRoot().GetWinPathString().CompareNoCase(L"c:\\windows") == 0);

	list.Clear();
	sPathList = L"c:\\windows\\*c:\\windows";
	list.LoadFromAsteriskSeparatedString(sPathList);
	EXPECT_TRUE(list.GetCommonRoot().GetWinPathString().CompareNoCase(L"c:\\windows") == 0);

	list.Clear();
	sPathList = L"c:\\windows\\system32*c:\\windows\\system";
	list.LoadFromAsteriskSeparatedString(sPathList);
	EXPECT_TRUE(list.GetCommonRoot().GetWinPathString().CompareNoCase(L"c:\\windows") == 0);

	list.Clear();
	sPathList = L"c:\\windowsdummy*c:\\windows";
	list.LoadFromAsteriskSeparatedString(sPathList);
	EXPECT_TRUE(list.GetCommonRoot().GetWinPathString().CompareNoCase(L"c:\\") == 0);

	list.Clear();
	sPathList = L"c:\\windows*d:\\windows";
	list.LoadFromAsteriskSeparatedString(sPathList);
	EXPECT_STREQ(L"", list.GetCommonRoot().GetWinPathString());
}

TEST(CTGitPath, ValidPathAndUrlTest)
{
	CTGitPath testPath;
	testPath.SetFromWin(L"c:\\a\\b\\c.test.txt");
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromWin(L"c:\\");
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromWin(L"D:\\.Net\\SpindleSearch\\");
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromWin(L"c");
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromWin(L"c:\\test folder\\file");
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromWin(L"c:\\folder\\");
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromWin(L"c:\\ext.ext.ext\\ext.ext.ext.ext");
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromWin(L"c:\\.git");
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromWin(L"c:\\com\\file");
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromWin(L"c:\\test\\conf");
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromWin(L"c:\\LPT");
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromWin(L"c:\\test\\LPT");
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromWin(L"c:\\com1test");
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromWin(L"\\\\?\\c:\\test\\com1test");
	EXPECT_TRUE(testPath.IsValidOnWindows());

	testPath.SetFromWin(L"\\\\Share\\filename");
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromWin(L"\\\\Share\\filename.extension");
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromWin(L"\\\\Share\\.git");
	EXPECT_TRUE(testPath.IsValidOnWindows());

	// now the negative tests
	testPath.SetFromWin(L"c:\\test:folder");
	EXPECT_FALSE(testPath.IsValidOnWindows());
	testPath.SetFromWin(L"c:\\file<name");
	EXPECT_FALSE(testPath.IsValidOnWindows());
	testPath.SetFromWin(L"c:\\something*else");
	EXPECT_FALSE(testPath.IsValidOnWindows());
	testPath.SetFromWin(L"c:\\folder\\file?nofile");
	EXPECT_FALSE(testPath.IsValidOnWindows());
	testPath.SetFromWin(L"c:\\ext.>ension");
	EXPECT_FALSE(testPath.IsValidOnWindows());
	testPath.SetFromWin(L"c:\\com1\\filename");
	EXPECT_FALSE(testPath.IsValidOnWindows());
	testPath.SetFromWin(L"c:\\com1");
	EXPECT_FALSE(testPath.IsValidOnWindows());
	testPath.SetFromWin(L"c:\\com1\\AuX");
	EXPECT_FALSE(testPath.IsValidOnWindows());

	testPath.SetFromWin(L"\\\\Share\\lpt9\\filename");
	EXPECT_FALSE(testPath.IsValidOnWindows());
	testPath.SetFromWin(L"\\\\Share\\prn");
	EXPECT_FALSE(testPath.IsValidOnWindows());
	testPath.SetFromWin(L"\\\\Share\\NUL");
	EXPECT_FALSE(testPath.IsValidOnWindows());

	// now come some URL tests
	/*testPath.SetFromGit(L"http://myserver.com/repos/trunk");
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromGit(L"https://myserver.com/repos/trunk/file%20with%20spaces");
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromGit(L"svn://myserver.com/repos/trunk/file with spaces");
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromGit(L"svn+ssh://www.myserver.com/repos/trunk");
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromGit(L"http://localhost:90/repos/trunk");
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromGit(L"file:///C:/GitRepos/Tester/Proj1/tags/t2");
	EXPECT_TRUE(testPath.IsValidOnWindows());
	// and some negative URL tests
	testPath.SetFromGit(L"httpp://myserver.com/repos/trunk");
	EXPECT_FALSE(testPath.IsValidOnWindows());
	testPath.SetFromGit(L"https://myserver.com/rep:os/trunk/file%20with%20spaces");
	EXPECT_FALSE(testPath.IsValidOnWindows());
	testPath.SetFromGit(L"svn://myserver.com/rep<os/trunk/file with spaces");
	EXPECT_FALSE(testPath.IsValidOnWindows());
	testPath.SetFromGit(L"svn+ssh://www.myserver.com/repos/trunk/prn/");
	EXPECT_FALSE(testPath.IsValidOnWindows());
	testPath.SetFromGit(L"http://localhost:90/repos/trunk/com1");
	EXPECT_FALSE(testPath.IsValidOnWindows());*/
}

TEST(CTGitPath, ListLoadingTest)
{
	TCHAR buf[MAX_PATH] = { 0 };
	GetCurrentDirectory(MAX_PATH, buf);
	CString sPathList(L"Path1*c:\\path2 with spaces and stuff*\\funnypath\\*");
	CTGitPathList testList;
	testList.LoadFromAsteriskSeparatedString(sPathList);

	EXPECT_EQ(3, testList.GetCount());
	EXPECT_STREQ(CString(buf) + L"\\Path1", testList[0].GetWinPathString());
	EXPECT_STREQ(L"c:\\path2 with spaces and stuff", testList[1].GetWinPathString());
	EXPECT_STREQ(L"\\funnypath", testList[2].GetWinPathString());

	EXPECT_STREQ(L"", testList.GetCommonRoot().GetWinPathString());
	testList.Clear();
	sPathList = L"c:\\path2 with spaces and stuff*c:\\funnypath\\*";
	testList.LoadFromAsteriskSeparatedString(sPathList);
	EXPECT_STREQ(L"c:\\", testList.GetCommonRoot().GetWinPathString());
}

TEST(CTGitPath, LoadFromFileAndWriteToFile)
{
	CAutoTempDir tmpDir;
	CTGitPathList testList;
	CString fileNameRead = tmpDir.GetTempDir() + L"\\input";
	CString text;
	CString fileNameWrite = tmpDir.GetTempDir() + L"\\output";

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(fileNameRead, L"Path1\nc:\\path2 with spaces and stuff\n\\funnypath\\", false));
	EXPECT_TRUE(testList.LoadFromFile(fileNameRead));
	EXPECT_EQ(3, testList.GetCount());
	EXPECT_STREQ(L"Path1", testList[0].GetWinPathString());
	EXPECT_STREQ(L"c:\\path2 with spaces and stuff", testList[1].GetWinPathString());
	EXPECT_STREQ(L"\\funnypath", testList[2].GetWinPathString());
	EXPECT_TRUE(testList.WriteToFile(fileNameWrite, true));
	EXPECT_TRUE(CStringUtils::ReadStringFromTextFile(fileNameWrite, text));
	EXPECT_STREQ(L"Path1\r\nc:/path2 with spaces and stuff\r\n/funnypath\r\n", text);
	EXPECT_STREQ(L"", testList.GetCommonRoot().GetWinPathString());

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(fileNameRead, L"Path1\nc:\\path2 with spaces and stuff\n\n\\funnypath\\\n", false));
	EXPECT_TRUE(testList.LoadFromFile(fileNameRead));
	EXPECT_EQ(3, testList.GetCount());
	EXPECT_STREQ(L"Path1", testList[0].GetWinPathString());
	EXPECT_STREQ(L"c:\\path2 with spaces and stuff", testList[1].GetWinPathString());
	EXPECT_STREQ(L"\\funnypath", testList[2].GetWinPathString());
	EXPECT_TRUE(testList.WriteToFile(fileNameWrite, true));
	text.Empty();
	EXPECT_TRUE(CStringUtils::ReadStringFromTextFile(fileNameWrite, text));
	EXPECT_STREQ(L"Path1\r\nc:/path2 with spaces and stuff\r\n/funnypath\r\n", text);

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(fileNameRead, L"c:\\path2 with spaces and stuff\nc:\\funnypath\\\n", false));
	testList.LoadFromFile(fileNameRead);
	EXPECT_STREQ(L"c:\\", testList.GetCommonRoot().GetWinPathString());

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(fileNameRead, L"c:\\path2 with spaces and stuff\n\nc:\\funnypath\\\n", false));
	testList.LoadFromFile(fileNameRead);
	EXPECT_STREQ(L"c:\\", testList.GetCommonRoot().GetWinPathString());
}

TEST(CTGitPath, GetBaseFilename)
{
	CTGitPath testPath;
	EXPECT_STREQ(L"", testPath.GetBaseFilename());

	testPath.SetFromWin(L"filename.extension");
	EXPECT_STREQ(L"filename", testPath.GetBaseFilename());
	testPath.SetFromWin(L"c:\\test.txt");
	EXPECT_STREQ(L"test", testPath.GetBaseFilename());
	testPath.SetFromWin(L"c:\\subfolder\\file.txt");
	EXPECT_STREQ(L"file", testPath.GetBaseFilename());
	testPath.SetFromWin(L"c:\\without");
	EXPECT_STREQ(L"without", testPath.GetBaseFilename());
	testPath.SetFromWin(L"c:\\folder\\");
	EXPECT_STREQ(L"", testPath.GetBaseFilename()); // this is the behavior right now, dunno if this is the best one
	testPath.SetFromWin(L".gitignore");
	EXPECT_STREQ(L".gitignore", testPath.GetBaseFilename());
	testPath.SetFromWin(L"c:\\.gitignore");
	EXPECT_STREQ(L".gitignore", testPath.GetBaseFilename());
	testPath.SetFromWin(L"c:\\test.double.extension");
	EXPECT_STREQ(L"test.double", testPath.GetBaseFilename());
}

TEST(CTGitPath, IsEquivalentTo)
{
	CTGitPath testPath;
	CTGitPath testPath2;

	EXPECT_TRUE(testPath.IsEquivalentTo(testPath2));

	testPath.SetFromUnknown(L"c:\\folder");
	testPath2.SetFromUnknown(L"c:\\folder\\");
	EXPECT_TRUE(testPath.IsEquivalentTo(testPath2));

	testPath.SetFromUnknown(L"c:\\folder\\");
	testPath2.SetFromUnknown(L"c:\\folder");
	EXPECT_TRUE(testPath.IsEquivalentTo(testPath2));

	testPath.SetFromUnknown(L"\\unc\\path");
	testPath2.SetFromUnknown(L"\\unc\\path");
	EXPECT_TRUE(testPath.IsEquivalentTo(testPath2));

	testPath.SetFromUnknown(L"c:\\");
	testPath2.SetFromUnknown(L"c:\\");
	EXPECT_TRUE(testPath.IsEquivalentTo(testPath2));

	testPath2.SetFromUnknown(L"c:/");
	EXPECT_TRUE(testPath.IsEquivalentTo(testPath2));

	testPath2.SetFromUnknown(L"c:");
	EXPECT_TRUE(testPath.IsEquivalentTo(testPath2));

	testPath.SetFromUnknown(L"c:\\test\\1.txt");
	testPath2.SetFromUnknown(L"c:\\test\\1.txt");
	EXPECT_TRUE(testPath.IsEquivalentTo(testPath2));

	testPath2.SetFromUnknown(L"c:/test/1.txt");
	EXPECT_TRUE(testPath.IsEquivalentTo(testPath2));

	testPath2.SetFromUnknown(L"c:\\test\\2.txt");
	EXPECT_FALSE(testPath.IsEquivalentTo(testPath2));

	testPath2.SetFromUnknown(L"c:\\text\\1.txt");
	EXPECT_FALSE(testPath.IsEquivalentTo(testPath2));

	testPath2.SetFromUnknown(L"c:\\tesT\\1.txt");
	EXPECT_FALSE(testPath.IsEquivalentTo(testPath2));

	testPath2.SetFromUnknown(L"c:\\test\\1.tXt");
	EXPECT_FALSE(testPath.IsEquivalentTo(testPath2));

	testPath.SetFromUnknown(L"test/1.txt");
	testPath2.SetFromUnknown(L"test/1.txt");
	EXPECT_TRUE(testPath.IsEquivalentTo(testPath2));

	testPath2.SetFromUnknown(L"test\\1.txt");
	EXPECT_TRUE(testPath.IsEquivalentTo(testPath2));

	testPath2.SetFromUnknown(L"tesT\\1.txt");
	EXPECT_FALSE(testPath.IsEquivalentTo(testPath2));

	testPath2.SetFromUnknown(L"text\\1.txt");
	EXPECT_FALSE(testPath.IsEquivalentTo(testPath2));

	testPath2.SetFromUnknown(L"test\\2.txt");
	EXPECT_FALSE(testPath.IsEquivalentTo(testPath2));
}

TEST(CTGitPath, IsEquivalentToWithoutCase)
{
	CTGitPath testPath;
	CTGitPath testPath2;

	EXPECT_TRUE(testPath.IsEquivalentToWithoutCase(testPath2));

	testPath.SetFromUnknown(L"c:\\folder");
	testPath2.SetFromUnknown(L"c:\\folder\\");
	EXPECT_TRUE(testPath.IsEquivalentToWithoutCase(testPath2));

	testPath.SetFromUnknown(L"c:\\folder\\");
	testPath2.SetFromUnknown(L"c:\\folder");
	EXPECT_TRUE(testPath.IsEquivalentToWithoutCase(testPath2));

	testPath.SetFromUnknown(L"\\unc\\path");
	testPath2.SetFromUnknown(L"\\unc\\path");
	EXPECT_TRUE(testPath.IsEquivalentToWithoutCase(testPath2));

	testPath.SetFromUnknown(L"c:\\");
	testPath2.SetFromUnknown(L"c:\\");
	EXPECT_TRUE(testPath.IsEquivalentToWithoutCase(testPath2));

	testPath2.SetFromUnknown(L"c:/");
	EXPECT_TRUE(testPath.IsEquivalentToWithoutCase(testPath2));

	testPath2.SetFromUnknown(L"c:");
	EXPECT_TRUE(testPath.IsEquivalentToWithoutCase(testPath2));

	testPath.SetFromUnknown(L"c:\\test\\1.txt");
	testPath2.SetFromUnknown(L"c:\\test\\1.txt");
	EXPECT_TRUE(testPath.IsEquivalentToWithoutCase(testPath2));

	testPath2.SetFromUnknown(L"c:/test/1.txt");
	EXPECT_TRUE(testPath.IsEquivalentToWithoutCase(testPath2));

	testPath2.SetFromUnknown(L"c:\\test\\2.txt");
	EXPECT_FALSE(testPath.IsEquivalentToWithoutCase(testPath2));

	testPath2.SetFromUnknown(L"c:\\text\\1.txt");
	EXPECT_FALSE(testPath.IsEquivalentToWithoutCase(testPath2));

	testPath2.SetFromUnknown(L"c:\\tesT\\1.txt");
	EXPECT_TRUE(testPath.IsEquivalentToWithoutCase(testPath2));

	testPath2.SetFromUnknown(L"c:\\test\\1.tXt");
	EXPECT_TRUE(testPath.IsEquivalentToWithoutCase(testPath2));

	testPath.SetFromUnknown(L"test/1.txt");
	testPath2.SetFromUnknown(L"test/1.txt");
	EXPECT_TRUE(testPath.IsEquivalentToWithoutCase(testPath2));

	testPath2.SetFromUnknown(L"test\\1.txt");
	EXPECT_TRUE(testPath.IsEquivalentToWithoutCase(testPath2));

	testPath2.SetFromUnknown(L"tesT\\1.txt");
	EXPECT_TRUE(testPath.IsEquivalentToWithoutCase(testPath2));

	testPath2.SetFromUnknown(L"text\\1.txt");
	EXPECT_FALSE(testPath.IsEquivalentToWithoutCase(testPath2));

	testPath2.SetFromUnknown(L"test\\2.txt");
	EXPECT_FALSE(testPath.IsEquivalentToWithoutCase(testPath2));
}

TEST(CTGitPath, ParserFromLog_Empty)
{
	CGitByteArray byteArray;
	CTGitPathList testList;
	EXPECT_EQ(0, testList.ParserFromLog(byteArray));
	EXPECT_EQ(0, testList.GetCount());
}

TEST(CTGitPath, ParserFromLsFileSimple_Empty)
{
	CGitByteArray byteArray;
	CTGitPathList testList;
	EXPECT_EQ(0, testList.ParserFromLsFileSimple(byteArray, 0));
	EXPECT_EQ(0, testList.GetCount());
}

TEST(CTGitPath, ParserFromLsFileSimple_Clear)
{
	constexpr unsigned int magicValue = 5758345;

	CGitByteArray byteArray;
	CTGitPathList testList;
	constexpr char git_ls_files_output[] = { "ext/LNI/" };
	byteArray.append(git_ls_files_output, sizeof(git_ls_files_output));
	EXPECT_EQ(0, testList.ParserFromLsFileSimple(byteArray, magicValue));
	ASSERT_EQ(1, testList.GetCount());
	EXPECT_STREQ(L"ext/LNI", testList[0].GetGitPathString());
	EXPECT_EQ(magicValue, testList[0].m_Action);
	EXPECT_TRUE(testList[0].IsDirectory());

	constexpr char git_ls_files_output2[] = { "src/gpl.txt" };
	CGitByteArray byteArray2;
	byteArray2.append(git_ls_files_output2, sizeof(git_ls_files_output2));
	EXPECT_EQ(0, testList.ParserFromLsFileSimple(byteArray2, magicValue));
	ASSERT_EQ(1, testList.GetCount());
	EXPECT_STREQ(L"src/gpl.txt", testList[0].GetGitPathString());
	EXPECT_EQ(magicValue, testList[0].m_Action);
	EXPECT_FALSE(testList[0].IsDirectory());

	CGitByteArray byteArray3;
	byteArray3.append(git_ls_files_output, sizeof(git_ls_files_output));
	byteArray3.append(git_ls_files_output2, sizeof(git_ls_files_output2));
	EXPECT_EQ(0, testList.ParserFromLsFileSimple(byteArray3, magicValue));
	ASSERT_EQ(2, testList.GetCount());
	EXPECT_STREQ(L"ext/LNI", testList[0].GetGitPathString());
	EXPECT_EQ(magicValue, testList[0].m_Action);
	EXPECT_TRUE(testList[0].IsDirectory());
	EXPECT_STREQ(L"src/gpl.txt", testList[1].GetGitPathString());
	EXPECT_EQ(magicValue, testList[1].m_Action);
	EXPECT_FALSE(testList[1].IsDirectory());
}

TEST(CTGitPath, ParserFromLsFileSimple_Invalid)
{
	constexpr char git_ls_file_d_z_output[] = { "build.txt\0zzz-added-only-in-index-missing-on-fs.txt" };

	for (int i = 1; i < sizeof(git_ls_file_d_z_output); ++i)
	{
		if (i == 10)
			continue;
		CGitByteArray byteArray;
		CTGitPathList testList;
		byteArray.append(git_ls_file_d_z_output, i);
		EXPECT_EQ(-1, testList.ParserFromLsFileSimple(byteArray, 0));
	}
}

/* git status output for the following tests marked with "(*)"
 * gpl.txt was deleted in index, but still on disk
 * build.txt deleted on disk, but not in index
 * src/Debug-Hints.txt deleted on disk and in index
 * zzz-added-only-in-index-missing-on-fs.txt added in index, and then removed on disk
 * signedness.txt staged new file
 * renamed release.txt -> release-renamed.txt (and changed a line afterwards)
 * changed on disk but not in index test/UnitTests/TGitPathTest.cpp
 * deleted ext/spell
 * staged new submodule ext/json
 * added untracked submodules ext/LNI
 * deleted and unstaged ext/hunspell
 * changed appveyor.yml and staged changes
 * changed README.md, staged changes, and modified again

On branch tests
Your branch and 'origin/tests' have diverged,
and have 2 and 1 different commit each, respectively.
(use "git pull" to merge the remote branch into yours)

Changes to be committed:
  (use "git restore --staged <file>..." to unstage)
        modified:   README.md
        modified:   appveyor.yml
        deleted:    ext/hunspell
        new file:   ext/json
        renamed:    release.txt -> release-renamed.txt
        new file:   signedness.txt
        deleted:    src/Debug-Hints.txt
        deleted:    src/gpl.txt
        new file:   zzz-added-only-in-index-missing-on-fs.txt

Changes not staged for commit:
  (use "git add/rm <file>..." to update what will be committed)
  (use "git restore <file>..." to discard changes in working directory)
  (commit or discard the untracked or modified content in submodules)
        modified:   README.md
        deleted:    build.txt
        modified:   ext/apr (untracked content)
        modified:   ext/apr-util (modified content, untracked content)
        modified:   ext/libgit2 (new commits)
        deleted:    ext/spell
        modified:   release-renamed.txt
        modified:   test/UnitTests/TGitPathTest.cpp
        deleted:    zzz-added-only-in-index-missing-on-fs.txt

Untracked files:
  (use "git add <file>..." to include in what will be committed)
        ext/LNI/
        src/gpl.txt
*/

TEST(CTGitPath, ParserFromLsFileSimple_Deleted)
{
	// as used in CGit::GetWorkingTreeChanges, based on (*)
	// git.exe ls-files -d -z
	constexpr char git_ls_file_d_z_output[] = { "build.txt\0zzz-added-only-in-index-missing-on-fs.txt" };

	CGitByteArray byteArray;
	CTGitPathList testList;
	byteArray.append(git_ls_file_d_z_output, sizeof(git_ls_file_d_z_output));
	EXPECT_EQ(0, testList.ParserFromLsFileSimple(byteArray, CTGitPath::LOGACTIONS_DELETED | CTGitPath::LOGACTIONS_MISSING));
	ASSERT_EQ(2, testList.GetCount());
	EXPECT_STREQ(L"build.txt", testList[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED | CTGitPath::LOGACTIONS_MISSING, testList[0].m_Action);
	EXPECT_STREQ(L"zzz-added-only-in-index-missing-on-fs.txt", testList[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED | CTGitPath::LOGACTIONS_MISSING, testList[1].m_Action);
}

TEST(CTGitPath, ParserFromLsFileSimple_Unversioned)
{
	// as used in CTGitPathList::FillUnRev, based on (*)
	// git.exe ls-files --exclude-standard --full-name --others
	constexpr char git_ls_files_output[] = { "ext/LNI/\0src/gpl.txt" };

	constexpr unsigned int magicValue = 5758345;

	CGitByteArray byteArray;
	CTGitPathList testList;
	byteArray.append(git_ls_files_output, sizeof(git_ls_files_output));
	EXPECT_EQ(0, testList.ParserFromLsFileSimple(byteArray, magicValue));
	ASSERT_EQ(2, testList.GetCount());
	EXPECT_STREQ(L"ext/LNI", testList[0].GetGitPathString());
	EXPECT_EQ(magicValue, testList[0].m_Action);
	EXPECT_TRUE(testList[0].IsDirectory());
	EXPECT_STREQ(L"src/gpl.txt", testList[1].GetGitPathString());
	EXPECT_EQ(magicValue, testList[1].m_Action);
	EXPECT_FALSE(testList[1].IsDirectory());
}

TEST(CTGitPath, ParserFromLog_DiffIndex_Raw_M_C_z)
{
	// as used in DiffCommand::Execute, based on (*)
	// git.exe diff-index --raw HEAD -M -C -z --
	constexpr char git_DiffIndex_Raw_M_C_z_output[] = { ":100644 100644 494108a99b463d68cb77c623c747ef3f2c08349d 0000000000000000000000000000000000000000 M\0README.md\0:100644 100644 ff9587d07866fb94e3d67eefa3c29b74e9b9adc8 a3e85b3d0f5ee1e3a7e46eca742621ddbcf18e9c M\0appveyor.yml\0:100644 000000 feec479b23ad3209ee99ea52f33c55f5ad9145f1 0000000000000000000000000000000000000000 D\0build.txt\0:160000 160000 9bc818fa5743986d0199805e8dcfa07a5dd8d5a2 9bc818fa5743986d0199805e8dcfa07a5dd8d5a2 M\0ext/apr-util\0:160000 000000 4ddd8ed5ca6484b930b111aec50c2750a6119a0f 0000000000000000000000000000000000000000 D\0ext/hunspell\0:000000 160000 0000000000000000000000000000000000000000 254395e5f7731b5595e7c11ac4b929c923582114 A\0ext/json\0:160000 160000 813702617f635cd1961973039af0033da6da2167 0000000000000000000000000000000000000000 M\0ext/libgit2\0:160000 000000 6e60b3ca21fb031486fd35572156232dc6da4c0c 0000000000000000000000000000000000000000 D\0ext/spell\0:100644 100644 a43df5aee238e78c78eaa6dbd015ee0123f53cf3 f8c0f04aead31b46d57e1503abe82b5e4f696bf1 R095\0release.txt\0release-renamed.txt\0:000000 100644 0000000000000000000000000000000000000000 af25e35b4c4cda137100a3a4820e8f7509ed121d A\0signedness.txt\0:100644 000000 6ae4ecc4c9ccee7597284145ffee70e62819ab82 0000000000000000000000000000000000000000 D\0src/Debug-Hints.txt\0:100644 000000 36488d58658d63f9d7a8d5e0637c530df987c4e5 0000000000000000000000000000000000000000 D\0src/gpl.txt\0:100644 100644 4318f4df5fe87c7019972b3b23a7dadb52303294 0000000000000000000000000000000000000000 M\0test/UnitTests/TGitPathTest.cpp" };
	// gpl.txt still exists, but deleted in index
	// build.txt deleted, still in index
	CGitByteArray byteArray;
	byteArray.append(git_DiffIndex_Raw_M_C_z_output, sizeof(git_DiffIndex_Raw_M_C_z_output));
	CTGitPathList testList;
	EXPECT_EQ(0, testList.ParserFromLog(byteArray));
	ASSERT_EQ(13, testList.GetCount());
	int i = 0;
	EXPECT_STREQ(L"README.md", testList [i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[i].m_Action);
	EXPECT_STREQ(L"", testList[i].m_StatAdd);
	EXPECT_STREQ(L"", testList[i].m_StatDel);
	EXPECT_FALSE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"appveyor.yml", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[i].m_Action);
	EXPECT_STREQ(L"", testList[i].m_StatAdd);
	EXPECT_STREQ(L"", testList[i].m_StatDel);
	EXPECT_FALSE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"build.txt", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, testList[i].m_Action);
	EXPECT_STREQ(L"", testList[i].m_StatAdd);
	EXPECT_STREQ(L"", testList[i].m_StatDel);
	EXPECT_FALSE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"ext/apr-util", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[i].m_Action);
	EXPECT_STREQ(L"", testList[i].m_StatAdd);
	EXPECT_STREQ(L"", testList[i].m_StatDel);
	EXPECT_TRUE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"ext/hunspell", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, testList[i].m_Action);
	EXPECT_STREQ(L"", testList[i].m_StatAdd);
	EXPECT_STREQ(L"", testList[i].m_StatDel);
	EXPECT_TRUE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"ext/json", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, testList[i].m_Action);
	EXPECT_STREQ(L"", testList[i].m_StatAdd);
	EXPECT_STREQ(L"", testList[i].m_StatDel);
	EXPECT_TRUE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"ext/libgit2", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[i].m_Action);
	EXPECT_STREQ(L"", testList[i].m_StatAdd);
	EXPECT_STREQ(L"", testList[i].m_StatDel);
	EXPECT_TRUE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"ext/spell", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, testList[i].m_Action);
	EXPECT_STREQ(L"", testList[i].m_StatAdd);
	EXPECT_STREQ(L"", testList[i].m_StatDel);
	EXPECT_TRUE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"release-renamed.txt", testList[i].GetGitPathString());
	EXPECT_STREQ(L"release.txt", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED, testList[i].m_Action);
	EXPECT_STREQ(L"", testList[i].m_StatAdd);
	EXPECT_STREQ(L"", testList[i].m_StatDel);
	EXPECT_FALSE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"signedness.txt", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, testList[i].m_Action);
	EXPECT_STREQ(L"", testList[i].m_StatAdd);
	EXPECT_STREQ(L"", testList[i].m_StatDel);
	EXPECT_FALSE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"src/Debug-Hints.txt", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, testList[i].m_Action);
	EXPECT_STREQ(L"", testList[i].m_StatAdd);
	EXPECT_STREQ(L"", testList[i].m_StatDel);
	EXPECT_FALSE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"src/gpl.txt", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, testList[i].m_Action);
	EXPECT_STREQ(L"", testList[i].m_StatAdd);
	EXPECT_STREQ(L"", testList[i].m_StatDel);
	EXPECT_FALSE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"test/UnitTests/TGitPathTest.cpp", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[i].m_Action);
	EXPECT_STREQ(L"", testList[i].m_StatAdd);
	EXPECT_STREQ(L"", testList[i].m_StatDel);
	EXPECT_FALSE(testList[i].IsDirectory());
}

TEST(CTGitPath, ParserFromLog_DiffIndex_Raw_M_C_Numstat_z)
{
	// as used in CGit::GetWorkingTreeChanges, but here separated; based on (*)
	// git.exe diff-index --raw HEAD --numstat -C -M -z --
	constexpr char git_DiffIndex_Raw_M_C_z_output[] = { ":100644 100644 494108a99b463d68cb77c623c747ef3f2c08349d 0000000000000000000000000000000000000000 M\0README.md\0:100644 100644 ff9587d07866fb94e3d67eefa3c29b74e9b9adc8 a3e85b3d0f5ee1e3a7e46eca742621ddbcf18e9c M\0appveyor.yml\0:100644 000000 feec479b23ad3209ee99ea52f33c55f5ad9145f1 0000000000000000000000000000000000000000 D\0build.txt\0:160000 160000 9bc818fa5743986d0199805e8dcfa07a5dd8d5a2 9bc818fa5743986d0199805e8dcfa07a5dd8d5a2 M\0ext/apr-util\0:160000 000000 4ddd8ed5ca6484b930b111aec50c2750a6119a0f 0000000000000000000000000000000000000000 D\0ext/hunspell\0:000000 160000 0000000000000000000000000000000000000000 254395e5f7731b5595e7c11ac4b929c923582114 A\0ext/json\0:160000 160000 813702617f635cd1961973039af0033da6da2167 0000000000000000000000000000000000000000 M\0ext/libgit2\0:160000 000000 6e60b3ca21fb031486fd35572156232dc6da4c0c 0000000000000000000000000000000000000000 D\0ext/spell\0:100644 100644 a43df5aee238e78c78eaa6dbd015ee0123f53cf3 f8c0f04aead31b46d57e1503abe82b5e4f696bf1 R095\0release.txt\0release-renamed.txt\0:000000 100644 0000000000000000000000000000000000000000 af25e35b4c4cda137100a3a4820e8f7509ed121d A\0signedness.txt\0:100644 000000 6ae4ecc4c9ccee7597284145ffee70e62819ab82 0000000000000000000000000000000000000000 D\0src/Debug-Hints.txt\0:100644 000000 36488d58658d63f9d7a8d5e0637c530df987c4e5 0000000000000000000000000000000000000000 D\0src/gpl.txt\0:100644 100644 4318f4df5fe87c7019972b3b23a7dadb52303294 0000000000000000000000000000000000000000 M\0test/UnitTests/TGitPathTest.cpp\0""3	45	README.md\0""0	79	appveyor.yml\0""0	77	build.txt\0""0	0	ext/apr-util\0""0	1	ext/hunspell\0""1	0	ext/json\0""1	1	ext/libgit2\0""0	1	ext/spell\0""1	0	\0release.txt\0release-renamed.txt\0""1176	0	signedness.txt\0""0	109	src/Debug-Hints.txt\0""0	340	src/gpl.txt\0""162	2	test/UnitTests/TGitPathTest.cpp" };
	CGitByteArray byteArray;
	byteArray.append(git_DiffIndex_Raw_M_C_z_output, sizeof(git_DiffIndex_Raw_M_C_z_output));
	CTGitPathList testList;
	EXPECT_EQ(0, testList.ParserFromLog(byteArray));
	ASSERT_EQ(13, testList.GetCount());
	int i = 0;
	EXPECT_STREQ(L"README.md", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[i].m_Action);
	EXPECT_STREQ(L"3", testList[i].m_StatAdd);
	EXPECT_STREQ(L"45", testList[i].m_StatDel);
	EXPECT_FALSE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"appveyor.yml", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[i].m_Action);
	EXPECT_STREQ(L"0", testList[i].m_StatAdd);
	EXPECT_STREQ(L"79", testList[i].m_StatDel);
	EXPECT_FALSE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"build.txt", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, testList[i].m_Action);
	EXPECT_STREQ(L"0", testList[i].m_StatAdd);
	EXPECT_STREQ(L"77", testList[i].m_StatDel);
	EXPECT_FALSE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"ext/apr-util", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[i].m_Action);
	EXPECT_STREQ(L"0", testList[i].m_StatAdd);
	EXPECT_STREQ(L"0", testList[i].m_StatDel);
	EXPECT_TRUE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"ext/hunspell", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, testList[i].m_Action);
	EXPECT_STREQ(L"0", testList[i].m_StatAdd);
	EXPECT_STREQ(L"1", testList[i].m_StatDel);
	EXPECT_TRUE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"ext/json", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, testList[i].m_Action);
	EXPECT_STREQ(L"1", testList[i].m_StatAdd);
	EXPECT_STREQ(L"0", testList[i].m_StatDel);
	EXPECT_TRUE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"ext/libgit2", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[i].m_Action);
	EXPECT_STREQ(L"1", testList[i].m_StatAdd);
	EXPECT_STREQ(L"1", testList[i].m_StatDel);
	EXPECT_TRUE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"ext/spell", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, testList[i].m_Action);
	EXPECT_STREQ(L"0", testList[i].m_StatAdd);
	EXPECT_STREQ(L"1", testList[i].m_StatDel);
	EXPECT_TRUE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"release-renamed.txt", testList[i].GetGitPathString());
	EXPECT_STREQ(L"release.txt", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED, testList[i].m_Action);
	EXPECT_STREQ(L"1", testList[i].m_StatAdd);
	EXPECT_STREQ(L"0", testList[i].m_StatDel);
	EXPECT_FALSE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"signedness.txt", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, testList[i].m_Action);
	EXPECT_STREQ(L"1176", testList[i].m_StatAdd);
	EXPECT_STREQ(L"0", testList[i].m_StatDel);
	EXPECT_FALSE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"src/Debug-Hints.txt", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, testList[i].m_Action);
	EXPECT_STREQ(L"0", testList[i].m_StatAdd);
	EXPECT_STREQ(L"109", testList[i].m_StatDel);
	EXPECT_FALSE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"src/gpl.txt", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, testList[i].m_Action);
	EXPECT_STREQ(L"0", testList[i].m_StatAdd);
	EXPECT_STREQ(L"340", testList[i].m_StatDel);
	EXPECT_FALSE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"test/UnitTests/TGitPathTest.cpp", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[i].m_Action);
	EXPECT_STREQ(L"162", testList[i].m_StatAdd);
	EXPECT_STREQ(L"2", testList[i].m_StatDel);
	EXPECT_FALSE(testList[i].IsDirectory());
}

TEST(CTGitPath, ParserFromLog_DiffIndex_Raw_Cached_M_C_Numstat_z)
{
	// as used in CGit::GetWorkingTreeChanges, but here separated; based on (*)
	// git.exe diff-index --cached --raw HEAD --numstat -C -M -z --
	constexpr char git_DiffIndex_Raw_Cached_M_C_z_output[] = { ":100644 100644 494108a99b463d68cb77c623c747ef3f2c08349d 5f9dfbd7882a43262a3abdd0e59feee62157f53f M\0README.md\0:100644 100644 ff9587d07866fb94e3d67eefa3c29b74e9b9adc8 a3e85b3d0f5ee1e3a7e46eca742621ddbcf18e9c M\0appveyor.yml\0:160000 000000 4ddd8ed5ca6484b930b111aec50c2750a6119a0f 0000000000000000000000000000000000000000 D\0ext/hunspell\0:000000 160000 0000000000000000000000000000000000000000 254395e5f7731b5595e7c11ac4b929c923582114 A\0ext/json\0:100644 100644 a43df5aee238e78c78eaa6dbd015ee0123f53cf3 a43df5aee238e78c78eaa6dbd015ee0123f53cf3 R100\0release.txt\0release-renamed.txt\0:000000 100644 0000000000000000000000000000000000000000 af25e35b4c4cda137100a3a4820e8f7509ed121d A\0signedness.txt\0:100644 000000 6ae4ecc4c9ccee7597284145ffee70e62819ab82 0000000000000000000000000000000000000000 D\0src/Debug-Hints.txt\0:100644 000000 36488d58658d63f9d7a8d5e0637c530df987c4e5 0000000000000000000000000000000000000000 D\0src/gpl.txt\0:000000 100644 0000000000000000000000000000000000000000 da259ecebe70cdffc1a03b93437995ae43a2baf9 A\0zzz-added-only-in-index-missing-on-fs.txt\0""6	30	README.md\0""0	79	appveyor.yml\0""0	1	ext/hunspell\0""1	0	ext/json\0""0	0	\0release.txt\0release-renamed.txt\0""1176	0	signedness.txt\0""0	109	src/Debug-Hints.txt\0""0	340	src/gpl.txt\0""1	0	zzz-added-only-in-index-missing-on-fs.txt" };
	CGitByteArray byteArray;
	byteArray.append(git_DiffIndex_Raw_Cached_M_C_z_output, sizeof(git_DiffIndex_Raw_Cached_M_C_z_output));
	CTGitPathList testList;
	EXPECT_EQ(0, testList.ParserFromLog(byteArray));
	ASSERT_EQ(9, testList.GetCount());
	int i = 0;
	EXPECT_STREQ(L"README.md", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[i].m_Action);
	EXPECT_STREQ(L"6", testList[i].m_StatAdd);
	EXPECT_STREQ(L"30", testList[i].m_StatDel);
	EXPECT_FALSE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"appveyor.yml", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[i].m_Action);
	EXPECT_STREQ(L"0", testList[i].m_StatAdd);
	EXPECT_STREQ(L"79", testList[i].m_StatDel);
	EXPECT_FALSE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"ext/hunspell", testList[i].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, testList[i].m_Action);
	EXPECT_STREQ(L"0", testList[i].m_StatAdd);
	EXPECT_STREQ(L"1", testList[i].m_StatDel);
	EXPECT_TRUE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"ext/json", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, testList[i].m_Action);
	EXPECT_STREQ(L"1", testList[i].m_StatAdd);
	EXPECT_STREQ(L"0", testList[i].m_StatDel);
	EXPECT_TRUE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"release-renamed.txt", testList[i].GetGitPathString());
	EXPECT_STREQ(L"release.txt", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED, testList[i].m_Action);
	EXPECT_STREQ(L"0", testList[i].m_StatAdd);
	EXPECT_STREQ(L"0", testList[i].m_StatDel);
	EXPECT_FALSE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"signedness.txt", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, testList[i].m_Action);
	EXPECT_STREQ(L"1176", testList[i].m_StatAdd);
	EXPECT_STREQ(L"0", testList[i].m_StatDel);
	EXPECT_FALSE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"src/Debug-Hints.txt", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, testList[i].m_Action);
	EXPECT_STREQ(L"0", testList[i].m_StatAdd);
	EXPECT_STREQ(L"109", testList[i].m_StatDel);
	++i;
	EXPECT_STREQ(L"src/gpl.txt", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, testList[i].m_Action);
	EXPECT_STREQ(L"0", testList[i].m_StatAdd);
	EXPECT_STREQ(L"340", testList[i].m_StatDel);
	EXPECT_FALSE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"zzz-added-only-in-index-missing-on-fs.txt", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, testList[i].m_Action);
	EXPECT_STREQ(L"1", testList[i].m_StatAdd);
	EXPECT_STREQ(L"0", testList[i].m_StatDel);
	EXPECT_FALSE(testList[i].IsDirectory());
}

TEST(CTGitPath, ParserFromLog_DiffIndex_Raw_Cached_M_C_Numstat_z_AND_DiffIndex_Raw_M_C_Numstat_z)
{
	// as used in CGit::GetWorkingTreeChanges; based on (*)
	// just concatenation of the above two
	constexpr char git_DiffIndex_Raw_Cached_M_C_z_output[] = { ":100644 100644 494108a99b463d68cb77c623c747ef3f2c08349d 5f9dfbd7882a43262a3abdd0e59feee62157f53f M\0README.md\0:100644 100644 ff9587d07866fb94e3d67eefa3c29b74e9b9adc8 a3e85b3d0f5ee1e3a7e46eca742621ddbcf18e9c M\0appveyor.yml\0:160000 000000 4ddd8ed5ca6484b930b111aec50c2750a6119a0f 0000000000000000000000000000000000000000 D\0ext/hunspell\0:000000 160000 0000000000000000000000000000000000000000 254395e5f7731b5595e7c11ac4b929c923582114 A\0ext/json\0:100644 100644 a43df5aee238e78c78eaa6dbd015ee0123f53cf3 a43df5aee238e78c78eaa6dbd015ee0123f53cf3 R100\0release.txt\0release-renamed.txt\0:000000 100644 0000000000000000000000000000000000000000 af25e35b4c4cda137100a3a4820e8f7509ed121d A\0signedness.txt\0:100644 000000 6ae4ecc4c9ccee7597284145ffee70e62819ab82 0000000000000000000000000000000000000000 D\0src/Debug-Hints.txt\0:100644 000000 36488d58658d63f9d7a8d5e0637c530df987c4e5 0000000000000000000000000000000000000000 D\0src/gpl.txt\0:000000 100644 0000000000000000000000000000000000000000 da259ecebe70cdffc1a03b93437995ae43a2baf9 A\0zzz-added-only-in-index-missing-on-fs.txt\0""6	30	README.md\0""0	79	appveyor.yml\0""0	1	ext/hunspell\0""1	0	ext/json\0""0	0	\0release.txt\0release-renamed.txt\0""1176	0	signedness.txt\0""0	109	src/Debug-Hints.txt\0""0	340	src/gpl.txt\0""1	0	zzz-added-only-in-index-missing-on-fs.txt\0:100644 100644 494108a99b463d68cb77c623c747ef3f2c08349d 0000000000000000000000000000000000000000 M\0README.md\0:100644 100644 ff9587d07866fb94e3d67eefa3c29b74e9b9adc8 a3e85b3d0f5ee1e3a7e46eca742621ddbcf18e9c M\0appveyor.yml\0:100644 000000 feec479b23ad3209ee99ea52f33c55f5ad9145f1 0000000000000000000000000000000000000000 D\0build.txt\0:160000 160000 9bc818fa5743986d0199805e8dcfa07a5dd8d5a2 9bc818fa5743986d0199805e8dcfa07a5dd8d5a2 M\0ext/apr-util\0:160000 000000 4ddd8ed5ca6484b930b111aec50c2750a6119a0f 0000000000000000000000000000000000000000 D\0ext/hunspell\0:000000 160000 0000000000000000000000000000000000000000 254395e5f7731b5595e7c11ac4b929c923582114 A\0ext/json\0:160000 160000 813702617f635cd1961973039af0033da6da2167 0000000000000000000000000000000000000000 M\0ext/libgit2\0:160000 000000 6e60b3ca21fb031486fd35572156232dc6da4c0c 0000000000000000000000000000000000000000 D\0ext/spell\0:100644 100644 a43df5aee238e78c78eaa6dbd015ee0123f53cf3 f8c0f04aead31b46d57e1503abe82b5e4f696bf1 R095\0release.txt\0release-renamed.txt\0:000000 100644 0000000000000000000000000000000000000000 af25e35b4c4cda137100a3a4820e8f7509ed121d A\0signedness.txt\0:100644 000000 6ae4ecc4c9ccee7597284145ffee70e62819ab82 0000000000000000000000000000000000000000 D\0src/Debug-Hints.txt\0:100644 000000 36488d58658d63f9d7a8d5e0637c530df987c4e5 0000000000000000000000000000000000000000 D\0src/gpl.txt\0:100644 100644 4318f4df5fe87c7019972b3b23a7dadb52303294 0000000000000000000000000000000000000000 M\0test/UnitTests/TGitPathTest.cpp\0""3	45	README.md\0""0	79	appveyor.yml\0""0	77	build.txt\0""0	0	ext/apr-util\0""0	1	ext/hunspell\0""1	0	ext/json\0""1	1	ext/libgit2\0""0	1	ext/spell\0""1	0	\0release.txt\0release-renamed.txt\0""1176	0	signedness.txt\0""0	109	src/Debug-Hints.txt\0""0	340	src/gpl.txt\0""162	2	test/UnitTests/TGitPathTest.cpp" };
	CGitByteArray byteArray;
	byteArray.append(git_DiffIndex_Raw_Cached_M_C_z_output, sizeof(git_DiffIndex_Raw_Cached_M_C_z_output));
	CTGitPathList testList;
	EXPECT_EQ(0, testList.ParserFromLog(byteArray));
	ASSERT_EQ(14, testList.GetCount());
	int i = 0;
	EXPECT_STREQ(L"README.md", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[i].m_Action);
	EXPECT_STREQ(L"3", testList[i].m_StatAdd);
	EXPECT_STREQ(L"45", testList[i].m_StatDel);
	EXPECT_FALSE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"appveyor.yml", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[i].m_Action);
	EXPECT_STREQ(L"0", testList[i].m_StatAdd);
	EXPECT_STREQ(L"79", testList[i].m_StatDel);
	EXPECT_FALSE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"ext/hunspell", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, testList[i].m_Action);
	EXPECT_STREQ(L"0", testList[i].m_StatAdd);
	EXPECT_STREQ(L"1", testList[i].m_StatDel);
	EXPECT_TRUE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"ext/json", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, testList[i].m_Action);
	EXPECT_STREQ(L"1", testList[i].m_StatAdd);
	EXPECT_STREQ(L"0", testList[i].m_StatDel);
	EXPECT_TRUE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"release-renamed.txt", testList[i].GetGitPathString());
	EXPECT_STREQ(L"release.txt", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED, testList[i].m_Action);
	EXPECT_STREQ(L"1", testList[i].m_StatAdd);
	EXPECT_STREQ(L"0", testList[i].m_StatDel);
	EXPECT_FALSE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"signedness.txt", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, testList[i].m_Action);
	EXPECT_STREQ(L"1176", testList[i].m_StatAdd);
	EXPECT_STREQ(L"0", testList[i].m_StatDel);
	EXPECT_FALSE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"src/Debug-Hints.txt", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, testList[i].m_Action);
	EXPECT_STREQ(L"0", testList[i].m_StatAdd);
	EXPECT_STREQ(L"109", testList[i].m_StatDel);
	EXPECT_FALSE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"src/gpl.txt", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, testList[i].m_Action);
	EXPECT_STREQ(L"0", testList[i].m_StatAdd);
	EXPECT_STREQ(L"340", testList[i].m_StatDel);
	EXPECT_FALSE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"zzz-added-only-in-index-missing-on-fs.txt", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, testList[i].m_Action);
	EXPECT_STREQ(L"1", testList[i].m_StatAdd);
	EXPECT_STREQ(L"0", testList[i].m_StatDel);
	EXPECT_FALSE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"build.txt", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, testList[i].m_Action);
	EXPECT_STREQ(L"0", testList[i].m_StatAdd);
	EXPECT_STREQ(L"77", testList[i].m_StatDel);
	EXPECT_FALSE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"ext/apr-util", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[i].m_Action);
	EXPECT_STREQ(L"0", testList[i].m_StatAdd);
	EXPECT_STREQ(L"0", testList[i].m_StatDel);
	EXPECT_TRUE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"ext/libgit2", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[i].m_Action);
	EXPECT_STREQ(L"1", testList[i].m_StatAdd);
	EXPECT_STREQ(L"1", testList[i].m_StatDel);
	EXPECT_TRUE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"ext/spell", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, testList[i].m_Action);
	EXPECT_STREQ(L"0", testList[i].m_StatAdd);
	EXPECT_STREQ(L"1", testList[i].m_StatDel);
	EXPECT_TRUE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"test/UnitTests/TGitPathTest.cpp", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[i].m_Action);
	EXPECT_STREQ(L"162", testList[i].m_StatAdd);
	EXPECT_STREQ(L"2", testList[i].m_StatDel);
	EXPECT_FALSE(testList[i].IsDirectory());
}

TEST(CTGitPath, ParserFromLog_DiffFiles_raw_C_M_numstat_z)
{
	// as used in CGit::GetWorkingTreeChanges, based on (*)
	// git.exe diff-files --raw --numstat -C -M -z --
	constexpr char git_Diff_r_raw_C_M_numstat_z_HEAD_output[] = { ":100644 100644 5f9dfbd7882a43262a3abdd0e59feee62157f53f 0000000000000000000000000000000000000000 M\0README.md\0:100644 000000 feec479b23ad3209ee99ea52f33c55f5ad9145f1 0000000000000000000000000000000000000000 D\0build.txt\0:160000 160000 9bc818fa5743986d0199805e8dcfa07a5dd8d5a2 9bc818fa5743986d0199805e8dcfa07a5dd8d5a2 M\0ext/apr-util\0:160000 160000 813702617f635cd1961973039af0033da6da2167 0000000000000000000000000000000000000000 M\0ext/libgit2\0:160000 000000 6e60b3ca21fb031486fd35572156232dc6da4c0c 0000000000000000000000000000000000000000 D\0ext/spell\0:100644 100644 a43df5aee238e78c78eaa6dbd015ee0123f53cf3 0000000000000000000000000000000000000000 M\0release-renamed.txt\0:100644 100644 4318f4df5fe87c7019972b3b23a7dadb52303294 0000000000000000000000000000000000000000 M\0test/UnitTests/TGitPathTest.cpp\0:100644 000000 da259ecebe70cdffc1a03b93437995ae43a2baf9 0000000000000000000000000000000000000000 D\0zzz-added-only-in-index-missing-on-fs.txt\0""1	19	README.md\0""0	77	build.txt\0""0	0	ext/apr-util\0""1	1	ext/libgit2\0""0	1	ext/spell\0""1	0	release-renamed.txt\0""162	2	test/UnitTests/TGitPathTest.cpp\0""0	1	zzz-added-only-in-index-missing-on-fs.txt" };
	CGitByteArray byteArray;
	byteArray.append(git_Diff_r_raw_C_M_numstat_z_HEAD_output, sizeof(git_Diff_r_raw_C_M_numstat_z_HEAD_output));
	CTGitPathList testList;
	EXPECT_EQ(0, testList.ParserFromLog(byteArray));
	ASSERT_EQ(8, testList.GetCount());
	int i = 0;
	EXPECT_STREQ(L"README.md", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[i].m_Action);
	EXPECT_STREQ(L"1", testList[i].m_StatAdd);
	EXPECT_STREQ(L"19", testList[i].m_StatDel);
	EXPECT_FALSE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"build.txt", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, testList[i].m_Action);
	EXPECT_STREQ(L"0", testList[i].m_StatAdd);
	EXPECT_STREQ(L"77", testList[i].m_StatDel);
	EXPECT_FALSE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"ext/apr-util", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[i].m_Action);
	EXPECT_STREQ(L"0", testList[i].m_StatAdd);
	EXPECT_STREQ(L"0", testList[i].m_StatDel);
	EXPECT_TRUE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"ext/libgit2", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[i].m_Action);
	EXPECT_STREQ(L"1", testList[i].m_StatAdd);
	EXPECT_STREQ(L"1", testList[i].m_StatDel);
	EXPECT_TRUE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"ext/spell", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, testList[i].m_Action);
	EXPECT_STREQ(L"0", testList[i].m_StatAdd);
	EXPECT_STREQ(L"1", testList[i].m_StatDel);
	EXPECT_TRUE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"release-renamed.txt", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString()); // no rename detected here
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[i].m_Action);
	EXPECT_STREQ(L"1", testList[i].m_StatAdd);
	EXPECT_STREQ(L"0", testList[i].m_StatDel);
	EXPECT_FALSE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"test/UnitTests/TGitPathTest.cpp", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[i].m_Action);
	EXPECT_STREQ(L"162", testList[i].m_StatAdd);
	EXPECT_STREQ(L"2", testList[i].m_StatDel);
	EXPECT_FALSE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"zzz-added-only-in-index-missing-on-fs.txt", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, testList[i].m_Action);
	EXPECT_STREQ(L"0", testList[i].m_StatAdd);
	EXPECT_STREQ(L"1", testList[i].m_StatDel);
	EXPECT_FALSE(testList[i].IsDirectory());
}

TEST(CTGitPath, ParserFromLog_Diff_r_raw_C_M_numstat_z_HEAD)
{
	// as used in CGit::GetCommitDiffList, based on (*)
	// git.exe diff -r --raw -C -M --numstat -z HEAD --
	constexpr char git_Diff_r_raw_C_M_numstat_z_HEAD_output[] = { ":100644 100644 494108a99 000000000 M\0README.md\0:100644 100644 ff9587d07 a3e85b3d0 M\0appveyor.yml\0:100644 000000 feec479b2 000000000 D\0build.txt\0:160000 160000 9bc818fa5 9bc818fa5 M\0ext/apr-util\0:160000 000000 4ddd8ed5c 000000000 D\0ext/hunspell\0:000000 160000 000000000 254395e5f A\0ext/json\0:160000 160000 813702617 000000000 M\0ext/libgit2\0:160000 000000 6e60b3ca2 000000000 D\0ext/spell\0:100644 100644 a43df5aee f8c0f04ae R095\0release.txt\0release-renamed.txt\0:000000 100644 000000000 af25e35b4 A\0signedness.txt\0:100644 000000 6ae4ecc4c 000000000 D\0src/Debug-Hints.txt\0:100644 000000 36488d586 000000000 D\0src/gpl.txt\0:100644 100644 4318f4df5 000000000 M\0test/UnitTests/TGitPathTest.cpp\0""3	45	README.md\0""0	79	appveyor.yml\0""0	77	build.txt\0""0	0	ext/apr-util\0""0	1	ext/hunspell\0""1	0	ext/json\0""1	1	ext/libgit2\0""0	1	ext/spell\0""1	0	\0release.txt\0release-renamed.txt\0""1176	0	signedness.txt\0""0	109	src/Debug-Hints.txt\0""0	340	src/gpl.txt\0""162	2	test/UnitTests/TGitPathTest.cpp" };
	CGitByteArray byteArray;
	byteArray.append(git_Diff_r_raw_C_M_numstat_z_HEAD_output, sizeof(git_Diff_r_raw_C_M_numstat_z_HEAD_output));
	CTGitPathList testList;
	EXPECT_EQ(0, testList.ParserFromLog(byteArray));
	ASSERT_EQ(13, testList.GetCount());
	int i = 0;
	EXPECT_STREQ(L"README.md", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[i].m_Action);
	EXPECT_STREQ(L"3", testList[i].m_StatAdd);
	EXPECT_STREQ(L"45", testList[i].m_StatDel);
	EXPECT_FALSE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"appveyor.yml", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[i].m_Action);
	EXPECT_STREQ(L"0", testList[i].m_StatAdd);
	EXPECT_STREQ(L"79", testList[i].m_StatDel);
	EXPECT_FALSE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"build.txt", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, testList[i].m_Action);
	EXPECT_STREQ(L"0", testList[i].m_StatAdd);
	EXPECT_STREQ(L"77", testList[i].m_StatDel);
	EXPECT_FALSE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"ext/apr-util", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[i].m_Action);
	EXPECT_STREQ(L"0", testList[i].m_StatAdd);
	EXPECT_STREQ(L"0", testList[i].m_StatDel);
	EXPECT_TRUE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"ext/hunspell", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, testList[i].m_Action);
	EXPECT_STREQ(L"0", testList[i].m_StatAdd);
	EXPECT_STREQ(L"1", testList[i].m_StatDel);
	EXPECT_TRUE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"ext/json", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, testList[i].m_Action);
	EXPECT_STREQ(L"1", testList[i].m_StatAdd);
	EXPECT_STREQ(L"0", testList[i].m_StatDel);
	EXPECT_TRUE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"ext/libgit2", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[i].m_Action);
	EXPECT_STREQ(L"1", testList[i].m_StatAdd);
	EXPECT_STREQ(L"1", testList[i].m_StatDel);
	EXPECT_TRUE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"ext/spell", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, testList[i].m_Action);
	EXPECT_STREQ(L"0", testList[i].m_StatAdd);
	EXPECT_STREQ(L"1", testList[i].m_StatDel);
	EXPECT_TRUE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"release-renamed.txt", testList[i].GetGitPathString());
	EXPECT_STREQ(L"release.txt", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED, testList[i].m_Action);
	EXPECT_STREQ(L"1", testList[i].m_StatAdd);
	EXPECT_STREQ(L"0", testList[i].m_StatDel);
	EXPECT_FALSE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"signedness.txt", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, testList[i].m_Action);
	EXPECT_STREQ(L"1176", testList[i].m_StatAdd);
	EXPECT_STREQ(L"0", testList[i].m_StatDel);
	EXPECT_FALSE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"src/Debug-Hints.txt", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, testList[i].m_Action);
	EXPECT_STREQ(L"0", testList[i].m_StatAdd);
	EXPECT_STREQ(L"109", testList[i].m_StatDel);
	EXPECT_FALSE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"src/gpl.txt", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, testList[i].m_Action);
	EXPECT_STREQ(L"0", testList[i].m_StatAdd);
	EXPECT_STREQ(L"340", testList[i].m_StatDel);
	EXPECT_FALSE(testList[i].IsDirectory());
	++i;
	EXPECT_STREQ(L"test/UnitTests/TGitPathTest.cpp", testList[i].GetGitPathString());
	EXPECT_STREQ(L"", testList[i].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[i].m_Action);
	EXPECT_STREQ(L"162", testList[i].m_StatAdd);
	EXPECT_STREQ(L"2", testList[i].m_StatDel);
	EXPECT_FALSE(testList[i].IsDirectory());
}

/* git status output for the following tests marked with "(**)"
 * build.txt was renamed to büil國立1dк.txt
 * Ümlautfile.txt new file
 * src/Debug-Hints.txt was modified

On branch master2
Changes to be committed:
  (use "git restore --staged <file>..." to unstage)
		renamed:    build.txt -> "b\303\274il\345\234\213\347\253\2131d\320\272.txt"
		new file:   "\303\234mlautfile.txt"

Changes not staged for commit:
  (use "git add <file>..." to update what will be committed)
  (use "git restore <file>..." to discard changes in working directory)
		modified:   src/Debug-Hints.txt
*/

TEST(CTGitPath, ParserFromLog_DiffIndex_Raw_Cached_M_C_Numstat_z_UTF8)
{
	// as used in CGit::GetWorkingTreeChanges, but here separated; based on (**)
	constexpr char git_DiffIndex_Raw_Cached_M_C_z_output[] = { ":100644 100644 cbb4dfec5cd5a56eb616cfb73852c4f1eef4e4de cbb4dfec5cd5a56eb616cfb73852c4f1eef4e4de R100\0build.txt\0bÃ¼ilåœ‹ç«‹1dÐº.txt\0:000000 100644 0000000000000000000000000000000000000000 0d8c7e67d8e0a17d5529a54a0edd26c0cc0510bd A\0Ãœmlautfile.txt\0""0	0	\0build.txt\0bÃ¼ilåœ‹ç«‹1dÐº.txt\0""1	0	Ãœmlautfile.txt" };
	CGitByteArray byteArray;
	byteArray.append(git_DiffIndex_Raw_Cached_M_C_z_output, sizeof(git_DiffIndex_Raw_Cached_M_C_z_output));
	CTGitPathList testList;
	EXPECT_EQ(0, testList.ParserFromLog(byteArray));
	ASSERT_EQ(2, testList.GetCount());
	EXPECT_STREQ(L"büil\u570B\u7ACB1d\u043A.txt", testList[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED, testList[0].m_Action);
	EXPECT_STREQ(L"0", testList[0].m_StatAdd);
	EXPECT_STREQ(L"0", testList[0].m_StatDel);
	EXPECT_FALSE(testList[0].IsDirectory());
	EXPECT_STREQ(L"build.txt", testList[0].GetGitOldPathString());
	EXPECT_STREQ(L"Ümlautfile.txt", testList[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, testList[1].m_Action);
	EXPECT_STREQ(L"1", testList[1].m_StatAdd);
	EXPECT_STREQ(L"0", testList[1].m_StatDel);
	EXPECT_FALSE(testList[1].IsDirectory());
}

TEST(CTGitPath, ParserFromLog_DiffTree)
{
	// git.exe diff-tree -r --raw -C -M --numstat -z 8a75b51cc7f10b9755fc89837fe78b0c646b8b12~1 8a75b51cc7f10b9755fc89837fe78b0c646b8b12
	constexpr char git_difftree_output[] = { ":100644 100644 748cb10e524d745ccd55bdeff67a00bed35144f5 d8899ee55a66b2bfa7f497bfef90e951d846de03 M\0src/Git/Git.cpp\0:100644 100644 ca24882eb4ef523381c2a179697d0f0e069c4f85 569c603dae424b8dda6726f8fb6157bd6e527d9a M\0src/Git/Git.h\0:100644 100644 575645aee2c712ee92f31b6a57d594297e2e5370 71bff876b3465c6eb621f3f3ecb0657e4a8764d7 M\0src/Git/Git.vcxproj\0:100644 100644 d5a11fbc6da73c8070a5359c78d45075fadba6eb 7284c5914b55eae97078df4a97f9ec791446133d M\0src/Git/Git.vcxproj.filters\0:100644 000000 c298e597d65a049782f03e197d654c01579b4990 0000000000000000000000000000000000000000 D\0src/Git/GitConfig.cpp\0:100644 100644 0667abe736182c63d2f2e6c0f29d85e1d9fd0775 a58a0cd9e08d9d849387fdfa2adfc3066cbb85df R081\0src/Git/GitConfig.h\0src/Git/GitForWindows.h\0:100644 100644 582db6c063e34903ee1348bdffbb48dfe484f8e4 ec5178786fcf587fdd6b19349d53c22f1dbcdfbe M\0src/Git/GitIndex.cpp\0:100644 100644 e4d6608dcb8fe635515f42ed9c9e6fdc1fc6c216 4c0e326cda7a479e02a650367561a8b79a3082f2 M\0src/TortoiseProc/Settings/SetMainPage.cpp\0:100644 100644 52aa2c94e5cfa9a5287c84b20687b3365836f1f8 7eb74479bbc3a2e2089a5d32acdd915aacc0f40b M\0src/TortoiseProc/TortoiseProc.cpp\0""1	1	src/Git/Git.cpp\0""3	0	src/Git/Git.h\0""0	2	src/Git/Git.vcxproj\0""0	6	src/Git/Git.vcxproj.filters\0""0	29	src/Git/GitConfig.cpp\0""1	11	\0src/Git/GitConfig.h\0src/Git/GitForWindows.h\0""0	1	src/Git/GitIndex.cpp\0""1	1	src/TortoiseProc/Settings/SetMainPage.cpp\0""0	1	src/TortoiseProc/TortoiseProc.cpp" };
	CGitByteArray byteArray;
	byteArray.append(git_difftree_output, sizeof(git_difftree_output));
	CTGitPathList testList;
	EXPECT_EQ(0, testList.ParserFromLog(byteArray));
	ASSERT_EQ(9, testList.GetCount());
	EXPECT_STREQ(L"src/Git/Git.cpp", testList[0].GetGitPathString());
	EXPECT_STREQ(L"", testList[0].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[0].m_Action);
	EXPECT_STREQ(L"1", testList[0].m_StatAdd);
	EXPECT_STREQ(L"1", testList[0].m_StatDel);
	EXPECT_FALSE(testList[0].IsDirectory());
	EXPECT_STREQ(L"src/Git/Git.h", testList[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[1].m_Action);
	EXPECT_STREQ(L"3", testList[1].m_StatAdd);
	EXPECT_STREQ(L"0", testList[1].m_StatDel);
	EXPECT_FALSE(testList[1].IsDirectory());
	EXPECT_STREQ(L"src/Git/Git.vcxproj", testList[2].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[2].m_Action);
	EXPECT_STREQ(L"0", testList[2].m_StatAdd);
	EXPECT_STREQ(L"2", testList[2].m_StatDel);
	EXPECT_FALSE(testList[2].IsDirectory());
	EXPECT_STREQ(L"src/Git/Git.vcxproj.filters", testList[3].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[3].m_Action);
	EXPECT_STREQ(L"0", testList[3].m_StatAdd);
	EXPECT_STREQ(L"6", testList[3].m_StatDel);
	EXPECT_FALSE(testList[3].IsDirectory());
	EXPECT_STREQ(L"src/Git/GitConfig.cpp", testList[4].GetGitPathString());
	EXPECT_STREQ(L"", testList[4].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, testList[4].m_Action);
	EXPECT_STREQ(L"0", testList[4].m_StatAdd);
	EXPECT_STREQ(L"29", testList[4].m_StatDel);
	EXPECT_FALSE(testList[4].IsDirectory());
	EXPECT_STREQ(L"src/Git/GitForWindows.h", testList[5].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED, testList[5].m_Action);
	EXPECT_STREQ(L"src/Git/GitConfig.h", testList[5].GetGitOldPathString());
	EXPECT_STREQ(L"1", testList[5].m_StatAdd);
	EXPECT_STREQ(L"11", testList[5].m_StatDel);
	EXPECT_FALSE(testList[5].IsDirectory());
	EXPECT_STREQ(L"src/Git/GitIndex.cpp", testList[6].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[6].m_Action);
	EXPECT_STREQ(L"0", testList[6].m_StatAdd);
	EXPECT_STREQ(L"1", testList[6].m_StatDel);
	EXPECT_FALSE(testList[6].IsDirectory());
	EXPECT_STREQ(L"src/TortoiseProc/Settings/SetMainPage.cpp", testList[7].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[7].m_Action);
	EXPECT_STREQ(L"1", testList[7].m_StatAdd);
	EXPECT_STREQ(L"1", testList[7].m_StatDel);
	EXPECT_FALSE(testList[7].IsDirectory());
	EXPECT_STREQ(L"src/TortoiseProc/TortoiseProc.cpp", testList[8].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[8].m_Action);
	EXPECT_STREQ(L"0", testList[8].m_StatAdd);
	EXPECT_STREQ(L"1", testList[8].m_StatDel);
	EXPECT_FALSE(testList[8].IsDirectory());
}

TEST(CTGitPath, ParserFromLog_DiffTree_Submodule)
{
	// git.exe diff-tree -r --raw -C -M --numstat -z b299da8c23c6258faaf7258ab23fd3e3da971efa~1 b299da8c23c6258faaf7258ab23fd3e3da971efa --
	constexpr char git_difftree_output[] = { ":100644 100644 06c316bd39ea34a10bc069e6cdfac0a3513e7f3c 2ef08cd1cd3b365de242114e66790968534e4588 M\0.gitmodules\0:100644 100644 714bfa6587240be5af0aea261871becbf071da77 0652d174054ee07d457bfdd3dd81ffc65b179852 M\0appveyor.yml\0:100644 100644 5cf50b0900b7754658bc545ea70935d821ba7181 21cb193e2b064cd0b998cd3d568f77f2681a3497 R082\0ext/build/gtest.vcxproj\0ext/build/googletest.vcxproj\0:100644 100644 48992d1ca6d569e6b909c0f914b547dcfd11d695 9addb288e4b4ca5ecf942bcb4e5761093c6dd3a1 R075\0ext/build/gtest.vcxproj.filters\0ext/build/googletest.vcxproj.filters\0:160000 000000 0421b6f358139f02e102c9c332ce19a33faf75be 0000000000000000000000000000000000000000 D\0ext/gmock\0:000000 160000 0000000000000000000000000000000000000000 a2b8a8e07628e5fd60644b6dd99c1b5e7d7f1f47 A\0ext/googletest\0:160000 000000 86dec5c00225b34a0061127e5cc35b6cf1485b79 0000000000000000000000000000000000000000 D\0ext/gtest\0:100644 100644 5170beb2d475f7fad8f6fc43e4c1c77d490067c1 9f24e5fa5488635c15744b048f554ceb9fc9ac46 M\0src/TortoiseGit.sln\0:100644 100644 405bfa8909480496e1024f99b70823e5452b40f3 48fa98e71d415f1b7d775a2f023a75552b8c74a9 M\0test/UnitTests/UnitTests.vcxproj\0""3	6	.gitmodules\0""1	1	appveyor.yml\0""4	4	\0ext/build/gtest.vcxproj\0ext/build/googletest.vcxproj\0""3	3	\0ext/build/gtest.vcxproj.filters\0ext/build/googletest.vcxproj.filters\0""0	1	ext/gmock\0""1	0	ext/googletest\0""0	1	ext/gtest\0""1	1	src/TortoiseGit.sln\0""2	2	test/UnitTests/UnitTests.vcxproj" };
	CGitByteArray byteArray;
	byteArray.append(git_difftree_output, sizeof(git_difftree_output));
	CTGitPathList testList;
	EXPECT_EQ(0, testList.ParserFromLog(byteArray));
	ASSERT_EQ(9, testList.GetCount());
	EXPECT_STREQ(L".gitmodules", testList[0].GetGitPathString());
	EXPECT_STREQ(L"", testList[0].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[0].m_Action);
	EXPECT_STREQ(L"3", testList[0].m_StatAdd);
	EXPECT_STREQ(L"6", testList[0].m_StatDel);
	EXPECT_FALSE(testList[0].IsDirectory());
	EXPECT_STREQ(L"appveyor.yml", testList[1].GetGitPathString());
	EXPECT_STREQ(L"", testList[1].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[1].m_Action);
	EXPECT_STREQ(L"1", testList[1].m_StatAdd);
	EXPECT_STREQ(L"1", testList[1].m_StatDel);
	EXPECT_FALSE(testList[1].IsDirectory());
	EXPECT_STREQ(L"ext/build/googletest.vcxproj", testList[2].GetGitPathString());
	EXPECT_STREQ(L"ext/build/gtest.vcxproj", testList[2].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED, testList[2].m_Action); // TODO: CTGitPath::LOGACTIONS_MODIFIED?, LOGACTIONS_MODIFIED has highter precedence in CTGitPath::GetActionName
	EXPECT_STREQ(L"4", testList[2].m_StatAdd);
	EXPECT_STREQ(L"4", testList[2].m_StatDel);
	EXPECT_FALSE(testList[2].IsDirectory());
	EXPECT_STREQ(L"ext/build/googletest.vcxproj.filters", testList[3].GetGitPathString());
	EXPECT_STREQ(L"ext/build/gtest.vcxproj.filters", testList[3].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED, testList[3].m_Action); // TODO: CTGitPath::LOGACTIONS_MODIFIED?, LOGACTIONS_MODIFIED has highter precedence in CTGitPath::GetActionName
	EXPECT_STREQ(L"3", testList[3].m_StatAdd);
	EXPECT_STREQ(L"3", testList[3].m_StatDel);
	EXPECT_FALSE(testList[3].IsDirectory());
	EXPECT_STREQ(L"ext/gmock", testList[4].GetGitPathString());
	EXPECT_STREQ(L"", testList[4].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, testList[4].m_Action);
	EXPECT_STREQ(L"0", testList[4].m_StatAdd);
	EXPECT_STREQ(L"1", testList[4].m_StatDel);
	EXPECT_TRUE(testList[4].IsDirectory());
	EXPECT_STREQ(L"ext/googletest", testList[5].GetGitPathString());
	EXPECT_STREQ(L"", testList[5].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, testList[5].m_Action);
	EXPECT_STREQ(L"1", testList[5].m_StatAdd);
	EXPECT_STREQ(L"0", testList[5].m_StatDel);
	EXPECT_TRUE(testList[5].IsDirectory());
	EXPECT_STREQ(L"ext/gtest", testList[6].GetGitPathString());
	EXPECT_STREQ(L"", testList[6].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, testList[6].m_Action);
	EXPECT_STREQ(L"0", testList[6].m_StatAdd);
	EXPECT_STREQ(L"1", testList[6].m_StatDel);
	EXPECT_TRUE(testList[6].IsDirectory());
	EXPECT_STREQ(L"src/TortoiseGit.sln", testList[7].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[7].m_Action);
	EXPECT_STREQ(L"1", testList[7].m_StatAdd);
	EXPECT_STREQ(L"1", testList[7].m_StatDel);
	EXPECT_FALSE(testList[7].IsDirectory());
	EXPECT_STREQ(L"test/UnitTests/UnitTests.vcxproj", testList[8].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[8].m_Action);
	EXPECT_STREQ(L"2", testList[8].m_StatAdd);
	EXPECT_STREQ(L"2", testList[8].m_StatDel);
	EXPECT_FALSE(testList[8].IsDirectory());
}

TEST(CTGitPath, ParserFromLog_DiffTree_BinaryFiles)
{
	// git.exe diff-tree -r --raw -C -M --numstat -z c965e5186d44ac91e2cbd8d6cbdd28cb572f5a35 7743bf83b8fc7de4e2760a98a806203ac529adf3 --
	constexpr char git_difftree_output[] = { ":100644 100644 bd6dd5b4e58c75ea6854da6bdbfaf9338e68d17d 1ca5a0857a0f09e3ad94ce49e08b7fd2c92e972a M\0ext/putty/pageant.exe\0:100644 100644 a913f73c10178e99e538d66afc43ef932e52ed81 ae5e0246a29ad7a9287f4f461fe09d4b10d09472 M\0ext/putty/puttygen.exe\0-	-	ext/putty/pageant.exe\0-	-	ext/putty/puttygen.exe" };
	CGitByteArray byteArray;
	byteArray.append(git_difftree_output, sizeof(git_difftree_output));
	CTGitPathList testList;
	EXPECT_EQ(0, testList.ParserFromLog(byteArray));
	ASSERT_EQ(2, testList.GetCount());
	EXPECT_STREQ(L"ext/putty/pageant.exe", testList[0].GetGitPathString());
	EXPECT_STREQ(L"", testList[0].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[0].m_Action);
	EXPECT_STREQ(L"-", testList[0].m_StatAdd);
	EXPECT_STREQ(L"-", testList[0].m_StatDel);
	EXPECT_FALSE(testList[0].IsDirectory());
	EXPECT_STREQ(L"ext/putty/puttygen.exe", testList[1].GetGitPathString());
	EXPECT_STREQ(L"", testList[1].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[1].m_Action);
	EXPECT_STREQ(L"-", testList[1].m_StatAdd);
	EXPECT_STREQ(L"-", testList[1].m_StatDel);
	EXPECT_FALSE(testList[1].IsDirectory());
}

TEST(CTGitPath, ParserFromLog_Invalid)
{
	for (int i = 1; i < 84; ++i)
	{
		constexpr char git_ls_file_u_t_z_output[] = { "M 100644 1f9f46da1ee155aa765d6e379d9d19853358cb07 1	bla.txt\0M 100644 3aa011e7d3609ab9af90c4b10f616312d2be422f 2	bla.txt\0M 100644 56d252d69d535834b9fbfa6f6a633ecd505ea2e6 3	bla.txt" };
		if (i == 60)
			continue;
		CGitByteArray byteArray;
		byteArray.append(git_ls_file_u_t_z_output, i);
		CTGitPathList testList;
		EXPECT_EQ(-1, testList.ParserFromLog(byteArray));
	}
	for (int i = 1; i < 129; ++i)
	{
		constexpr char git_difftree_output[] = { "3	6	.gitmodules\0""1	1	appveyor.yml\0""4	4	\0ext/build/gtest.vcxproj\0ext/build/googletest.vcxproj\0""3	3	\0ext/build/gtest.vcxproj.filters" };
		if (i == 16 || i == 33 || i == 91)
			continue;
		CGitByteArray byteArray;
		byteArray.append(git_difftree_output, i);
		CTGitPathList testList;
		EXPECT_EQ(-1, testList.ParserFromLog(byteArray));
	}
	{
		constexpr char git_difftree_output[] = { "	" };
		CGitByteArray byteArray;
		byteArray.append(git_difftree_output, sizeof(git_difftree_output));
		CGitByteArray byteArray2;
		byteArray2.append(git_difftree_output, sizeof(git_difftree_output) - 1);
		CTGitPathList testList;
		EXPECT_EQ(-1, testList.ParserFromLog(byteArray));
		EXPECT_EQ(-1, testList.ParserFromLog(byteArray2));
	}
	{
		constexpr char git_difftree_output[] = { "		" };
		CGitByteArray byteArray;
		byteArray.append(git_difftree_output, sizeof(git_difftree_output));
		CGitByteArray byteArray2;
		byteArray2.append(git_difftree_output, sizeof(git_difftree_output) - 1);
		CTGitPathList testList;
		EXPECT_EQ(-1, testList.ParserFromLog(byteArray));
		EXPECT_EQ(-1, testList.ParserFromLog(byteArray2));
	}
	{
		constexpr char git_difftree_output[] = { "1	4	" };
		CGitByteArray byteArray;
		byteArray.append(git_difftree_output, sizeof(git_difftree_output));
		CTGitPathList testList;
		EXPECT_EQ(-1, testList.ParserFromLog(byteArray));
	}
	{
		constexpr char git_difftree_output[] = { "		\0" };
		CGitByteArray byteArray;
		byteArray.append(git_difftree_output, sizeof(git_difftree_output));
		CTGitPathList testList;
		EXPECT_EQ(-1, testList.ParserFromLog(byteArray));
	}
	{
		constexpr char git_difftree_output[] = { "		\0def" };
		CGitByteArray byteArray;
		byteArray.append(git_difftree_output, sizeof(git_difftree_output));
		CTGitPathList testList;
		EXPECT_EQ(-1, testList.ParserFromLog(byteArray));
	}
}

static void setFlagOnFileInIndex(CAutoIndex& gitindex, const CString& filename, bool assumevalid, bool skipworktree)
{
	size_t idx = SIZE_T_MAX;
	EXPECT_TRUE(git_index_find(&idx, gitindex, CUnicodeUtils::GetUTF8(filename)) == 0);
	git_index_entry *e = const_cast<git_index_entry *>(git_index_get_byindex(gitindex, idx));
	ASSERT_TRUE(e);
	if (assumevalid == BST_UNCHECKED)
		e->flags &= ~GIT_INDEX_ENTRY_VALID;
	else if (assumevalid == BST_CHECKED)
		e->flags |= GIT_INDEX_ENTRY_VALID;
	if (skipworktree == BST_UNCHECKED)
		e->flags_extended &= ~GIT_INDEX_ENTRY_SKIP_WORKTREE;
	else if (skipworktree == BST_CHECKED)
		e->flags_extended |= GIT_INDEX_ENTRY_SKIP_WORKTREE;
	EXPECT_TRUE(git_index_add(gitindex, e) == 0);
}

TEST(CTGitPath, FillBasedOnIndexFlags)
{
	CAutoTempDir tmpDir;

	CString output;
	CAutoRepository repo;
	EXPECT_TRUE(git_repository_init(repo.GetPointer(), CUnicodeUtils::GetUTF8(tmpDir.GetTempDir()), 0) == 0);

	ASSERT_TRUE(CreateDirectory(tmpDir.GetTempDir() + L"\\a", nullptr));
	ASSERT_TRUE(CreateDirectory(tmpDir.GetTempDir() + L"\\b", nullptr));

	CAutoIndex gitindex;
	EXPECT_TRUE(git_repository_index(gitindex.GetPointer(), repo) == 0);

	CString filenames[] = { L"versioned", L"assume-unchanged", L"skip-worktree", L"unversioned", L"a/versioned", L"a/assume-unchanged", L"a/skip-worktree", L"a/unversioned", L"b/versioned", L"b/assume-unchanged", L"b/skip-worktree", L"b/unversioned" };
	for (const CString& filename : filenames)
	{
		CString filenameWithPath = tmpDir.GetTempDir() + L'\\' + filename;
		EXPECT_TRUE(CStringUtils::WriteStringToTextFile(filenameWithPath, L"something"));
		EXPECT_TRUE(git_index_add_bypath(gitindex, CUnicodeUtils::GetUTF8(filename)) == 0);
	}

	setFlagOnFileInIndex(gitindex, L"assume-unchanged", true, false);
	setFlagOnFileInIndex(gitindex, L"a/assume-unchanged", true, false);
	setFlagOnFileInIndex(gitindex, L"b/assume-unchanged", true, false);
	setFlagOnFileInIndex(gitindex, L"skip-worktree", false, true);
	setFlagOnFileInIndex(gitindex, L"a/skip-worktree", false, true);
	setFlagOnFileInIndex(gitindex, L"b/skip-worktree", false, true);

	EXPECT_TRUE(git_index_write(gitindex) == 0);
	gitindex.Free();
	repo.Free();

	g_Git.m_CurrentDir = tmpDir.GetTempDir();

	CTGitPathList testList;
	EXPECT_TRUE(testList.FillBasedOnIndexFlags(0, 0) == 0);
	EXPECT_EQ(0, testList.GetCount());

	testList.Clear();
	EXPECT_TRUE(testList.FillBasedOnIndexFlags(GIT_INDEX_ENTRY_VALID, 0) == 0);
	EXPECT_EQ(3, testList.GetCount());
	EXPECT_STREQ(L"a/assume-unchanged", testList[0].GetGitPathString());
	EXPECT_STREQ(L"assume-unchanged", testList[1].GetGitPathString());
	EXPECT_STREQ(L"b/assume-unchanged", testList[2].GetGitPathString());

	testList.Clear();
	EXPECT_TRUE(testList.FillBasedOnIndexFlags(0, GIT_INDEX_ENTRY_SKIP_WORKTREE) == 0);
	EXPECT_EQ(3, testList.GetCount());
	EXPECT_STREQ(L"a/skip-worktree", testList[0].GetGitPathString());
	EXPECT_STREQ(L"b/skip-worktree", testList[1].GetGitPathString());
	EXPECT_STREQ(L"skip-worktree", testList[2].GetGitPathString());

	testList.Clear();
	EXPECT_TRUE(testList.FillBasedOnIndexFlags(GIT_INDEX_ENTRY_VALID, GIT_INDEX_ENTRY_SKIP_WORKTREE) == 0);
	EXPECT_EQ(6, testList.GetCount());
	EXPECT_STREQ(L"a/assume-unchanged", testList[0].GetGitPathString());
	EXPECT_STREQ(L"a/skip-worktree", testList[1].GetGitPathString());
	EXPECT_STREQ(L"assume-unchanged", testList[2].GetGitPathString());
	EXPECT_STREQ(L"b/assume-unchanged", testList[3].GetGitPathString());
	EXPECT_STREQ(L"b/skip-worktree", testList[4].GetGitPathString());
	EXPECT_STREQ(L"skip-worktree", testList[5].GetGitPathString());

	CTGitPathList selectList;
	selectList.AddPath(CTGitPath(L"versioned"));
	selectList.AddPath(CTGitPath(L"assume-unchanged"));
	selectList.AddPath(CTGitPath(L"skip-worktree"));
	selectList.AddPath(CTGitPath(L"a/skip-worktree"));
	EXPECT_TRUE(testList.FillBasedOnIndexFlags(GIT_INDEX_ENTRY_VALID, GIT_INDEX_ENTRY_SKIP_WORKTREE, &selectList) == 0);
	EXPECT_EQ(3, testList.GetCount());
	EXPECT_STREQ(L"a/skip-worktree", testList[0].GetGitPathString());
	EXPECT_STREQ(L"assume-unchanged", testList[1].GetGitPathString());
	EXPECT_STREQ(L"skip-worktree", testList[2].GetGitPathString());

	selectList.Clear();
	selectList.AddPath(CTGitPath(L"a"));
	EXPECT_TRUE(testList.FillBasedOnIndexFlags(GIT_INDEX_ENTRY_VALID, GIT_INDEX_ENTRY_SKIP_WORKTREE, &selectList) == 0);
	EXPECT_EQ(2, testList.GetCount());
	EXPECT_STREQ(L"a/assume-unchanged", testList[0].GetGitPathString());
	EXPECT_STREQ(L"a/skip-worktree", testList[1].GetGitPathString());

	selectList.Clear();
	selectList.AddPath(CTGitPath(L"a"));
	EXPECT_TRUE(testList.FillBasedOnIndexFlags(GIT_INDEX_ENTRY_VALID, 0, &selectList) == 0);
	EXPECT_EQ(1, testList.GetCount());
	EXPECT_STREQ(L"a/assume-unchanged", testList[0].GetGitPathString());
}

TEST(CTGitPath, ParserFromLsFile_Empty)
{
	CGitByteArray byteArray;
	CTGitPathList testList;
	EXPECT_EQ(0, testList.ParserFromLsFile(byteArray, GIT_HASH_TYPE::GIT_HASH_SHA1));
	EXPECT_EQ(0, testList.GetCount());
}

TEST(CTGitPath, ParserFromLsFile_NormalRepo)
{
	constexpr char git_ls_file_u_t_z_output[] = { "H 100644 73aea48a4ede6d3ca43bc3273c52e81a5d739447 0	README.md\0H 100644 e2780959232e32e0cce8c1866d36b04db85481a6 0	taskxml.xsd\0H 100644 49aa0e1b2e9fcd752767863a1208ad5b5afb254f 0	whitepaper.md" };
	CGitByteArray byteArray;
	byteArray.append(git_ls_file_u_t_z_output, sizeof(git_ls_file_u_t_z_output));
	CTGitPathList testList;
	EXPECT_EQ(0, testList.ParserFromLsFile(byteArray, GIT_HASH_TYPE::GIT_HASH_SHA1));
	EXPECT_EQ(3, testList.GetCount());
	EXPECT_STREQ(L"README.md", testList[0].GetGitPathString());
	EXPECT_STREQ(L"taskxml.xsd", testList[1].GetGitPathString());
	EXPECT_STREQ(L"whitepaper.md", testList[2].GetGitPathString());
	EXPECT_STREQ(L"", testList[0].GetGitOldPathString());
	EXPECT_STREQ(L"", testList[1].GetGitOldPathString());
	EXPECT_STREQ(L"", testList[2].GetGitOldPathString());
	EXPECT_EQ(0U, testList[0].m_Action);
	EXPECT_EQ(0U, testList[1].m_Action);
	EXPECT_EQ(0U, testList[2].m_Action);
	EXPECT_FALSE(testList[0].IsDirectory());
	EXPECT_FALSE(testList[1].IsDirectory());
	EXPECT_FALSE(testList[2].IsDirectory());
}

TEST(CTGitPath, ParserFromLsFile_RepoWithSubmodule)
{
	constexpr char git_ls_file_u_t_z_output[] = { "H 160000 d4eaf3c5d0994eb0112c17aa3c732022eb9fdf6b 0	ext/gtest" };
	CGitByteArray byteArray;
	byteArray.append(git_ls_file_u_t_z_output, sizeof(git_ls_file_u_t_z_output));
	CTGitPathList testList;
	EXPECT_EQ(0, testList.ParserFromLsFile(byteArray, GIT_HASH_TYPE::GIT_HASH_SHA1));
	ASSERT_EQ(1, testList.GetCount());
	EXPECT_STREQ(L"ext/gtest", testList[0].GetGitPathString());
	EXPECT_STREQ(L"", testList[0].GetGitOldPathString());
	EXPECT_EQ(0U, testList[0].m_Action);
	EXPECT_TRUE(testList[0].IsDirectory());
}

TEST(CTGitPath, ParserFromLsFile_Invalid)
{
	constexpr char git_ls_file_u_t_z_output[] = { "H 160000 d4eaf3c5d0994eb0112c17aa3c732022eb9fdf6b 0	ext/gtest" };
	for (int i = 1; i < sizeof(git_ls_file_u_t_z_output) - 1; ++i)
	{
		CGitByteArray byteArray;
		byteArray.append(git_ls_file_u_t_z_output, i);
		CTGitPathList testList;
		EXPECT_EQ(-1, testList.ParserFromLsFile(byteArray, GIT_HASH_TYPE::GIT_HASH_SHA1));
	}
	{
		constexpr char git_ls_file_u_t_z_outputInvalidFilename[] = { "something" };
		CGitByteArray byteArray;
		byteArray.append(git_ls_file_u_t_z_outputInvalidFilename, sizeof(git_ls_file_u_t_z_outputInvalidFilename));
		CTGitPathList testList;
		EXPECT_EQ(-1, testList.ParserFromLsFile(byteArray, GIT_HASH_TYPE::GIT_HASH_SHA1));
	}
	{
		constexpr char git_ls_file_u_t_z_outputInvalidFilename[] = { "    " };
		CGitByteArray byteArray;
		byteArray.append(git_ls_file_u_t_z_outputInvalidFilename, sizeof(git_ls_file_u_t_z_outputInvalidFilename));
		CTGitPathList testList;
		EXPECT_EQ(-1, testList.ParserFromLsFile(byteArray, GIT_HASH_TYPE::GIT_HASH_SHA1));
	}
	{
		constexpr char git_ls_file_u_t_z_outputInvalidFilename[] = { "   	" };
		CGitByteArray byteArray;
		byteArray.append(git_ls_file_u_t_z_outputInvalidFilename, sizeof(git_ls_file_u_t_z_outputInvalidFilename));
		CTGitPathList testList;
		EXPECT_EQ(-1, testList.ParserFromLsFile(byteArray, GIT_HASH_TYPE::GIT_HASH_SHA1));
	}
	{
		constexpr char git_ls_file_u_t_z_outputInvalidFilename[] = { "   	e" };
		CGitByteArray byteArray;
		byteArray.append(git_ls_file_u_t_z_outputInvalidFilename, sizeof(git_ls_file_u_t_z_outputInvalidFilename));
		CTGitPathList testList;
		EXPECT_EQ(-1, testList.ParserFromLsFile(byteArray, GIT_HASH_TYPE::GIT_HASH_SHA1));
	}
}

TEST(CTGitPath, ParserFromLsFile_Merged_SingleFileConflict)
{
	// as used in CGit::GetWorkingTreeChanges
	constexpr char git_ls_file_u_t_z_output[] = { "M 100644 1f9f46da1ee155aa765d6e379d9d19853358cb07 1	bla.txt\0M 100644 3aa011e7d3609ab9af90c4b10f616312d2be422f 2	bla.txt\0M 100644 56d252d69d535834b9fbfa6f6a633ecd505ea2e6 3	bla.txt" };
	CGitByteArray byteArray;
	byteArray.append(git_ls_file_u_t_z_output, sizeof(git_ls_file_u_t_z_output));
	CTGitPathList testList;
	EXPECT_EQ(0, testList.ParserFromLsFile(byteArray, GIT_HASH_TYPE::GIT_HASH_SHA1));
	ASSERT_EQ(1, testList.GetCount());
	EXPECT_STREQ(L"bla.txt", testList[0].GetGitPathString());
	EXPECT_STREQ(L"", testList[0].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED, testList[0].m_Action);
	EXPECT_FALSE(testList[0].IsDirectory());
}

TEST(CTGitPath, ParserFromLsFile_Merged_SubmoduleConflict_Simple)
{
	constexpr char git_ls_files_u_t_z_output[] = { "M 160000 46a2b8e855d5f6d8b60b81500a9f6779c7f63e63 1	libgit2\0M 160000 533da4ea00703f4ad6d5518e1ce81d20261c40c0 2	libgit2\0M 160000 ab2af775ec467ebb328a7374653f247920f258f3 3	libgit2" };
	CGitByteArray byteArray;
	byteArray.append(git_ls_files_u_t_z_output, sizeof(git_ls_files_u_t_z_output));
	CTGitPathList testList;
	EXPECT_EQ(0, testList.ParserFromLsFile(byteArray, GIT_HASH_TYPE::GIT_HASH_SHA1));
	ASSERT_EQ(1, testList.GetCount());
	EXPECT_STREQ(L"libgit2", testList[0].GetGitPathString());
	EXPECT_STREQ(L"", testList[0].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED, testList[0].m_Action);
	EXPECT_TRUE(testList[0].IsDirectory());
}

TEST(CTGitPath, ParserFromLsFile_Merged_SubmoduleConflict_DeletedModified)
{
	// merged commit where submodule was modified into commit where it was deleted (git status says "deleted by us: libgit2")
	constexpr char git_ls_files_u_t_z_output[] = { "M 160000 46a2b8e855d5f6d8b60b81500a9f6779c7f63e63 1	libgit2\0M 160000 ab2af775ec467ebb328a7374653f247920f258f3 3	libgit2" };
	CGitByteArray byteArray;
	byteArray.append(git_ls_files_u_t_z_output, sizeof(git_ls_files_u_t_z_output));
	CTGitPathList testList;
	EXPECT_EQ(0, testList.ParserFromLsFile(byteArray, GIT_HASH_TYPE::GIT_HASH_SHA1));
	ASSERT_EQ(1, testList.GetCount());
	EXPECT_STREQ(L"libgit2", testList[0].GetGitPathString());
	EXPECT_STREQ(L"", testList[0].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED, testList[0].m_Action);
	EXPECT_TRUE(testList[0].IsDirectory());
}

TEST(CTGitPath, ParserFromLsFile_Merged_SubmoduleConflict_ToNormalDir)
{
	// as used in CGit::GetWorkingTreeChanges
	// merged commit where libgit2 is a normal folder (git status says "added by us: libgit2" and "new file: libgit2/Neues Textdokument.txt")
	constexpr char git_ls_files_u_t_z_output[] = { "M 160000 533da4ea00703f4ad6d5518e1ce81d20261c40c0 2	libgit2" };
	CGitByteArray byteArray;
	byteArray.append(git_ls_files_u_t_z_output, sizeof(git_ls_files_u_t_z_output));
	CTGitPathList testList;
	EXPECT_EQ(0, testList.ParserFromLsFile(byteArray, GIT_HASH_TYPE::GIT_HASH_SHA1));
	ASSERT_EQ(1, testList.GetCount());
	EXPECT_STREQ(L"libgit2", testList[0].GetGitPathString());
	EXPECT_STREQ(L"", testList[0].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED, testList[0].m_Action);
	EXPECT_TRUE(testList[0].IsDirectory());
}

TEST(CTGitPath, ParserFromLsFile_Merged_SubmoduleConflict_FileSubmodule)
{
	// as used in CGit::GetWorkingTreeChanges
	// merged commit where a file with the same name was created (git status says "both added: libgit2")
	constexpr char git_ls_files_u_t_z_output[] = { "M 160000 533da4ea00703f4ad6d5518e1ce81d20261c40c0 2	libgit2\0M 100644 9ae3e601584cc03f8f03f93761416b6599ac7c0d 3	libgit2" };
	CGitByteArray byteArray;
	byteArray.append(git_ls_files_u_t_z_output, sizeof(git_ls_files_u_t_z_output));
	CTGitPathList testList;
	EXPECT_EQ(0, testList.ParserFromLsFile(byteArray, GIT_HASH_TYPE::GIT_HASH_SHA1));
	ASSERT_EQ(1, testList.GetCount());
	EXPECT_STREQ(L"libgit2", testList[0].GetGitPathString());
	EXPECT_STREQ(L"", testList[0].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED, testList[0].m_Action);
	EXPECT_TRUE(testList[0].IsDirectory());
}

TEST(CTGitPath, ParserFromLsFile_Merged_DeletedFileConflict)
{
	// file added, modified on branch A, deleted on branch B, merge branch A on B (git status says: "deleted by us")
	constexpr char git_ls_file_u_t_z_output[] = { "M 100644 24091f0add7afc47ac7cdc80ae4d3866b2ef588c 1	Neues Textdokument.txt\0M 100644 293b6f6293106b6ebb5d54ad482d7561b0f1c9ae 3	Neues Textdokument.txt" };
	CGitByteArray byteArray;
	byteArray.append(git_ls_file_u_t_z_output, sizeof(git_ls_file_u_t_z_output));
	CTGitPathList testList;
	EXPECT_EQ(0, testList.ParserFromLsFile(byteArray, GIT_HASH_TYPE::GIT_HASH_SHA1));
	ASSERT_EQ(1, testList.GetCount());
	EXPECT_STREQ(L"Neues Textdokument.txt", testList[0].GetGitPathString());
	EXPECT_STREQ(L"", testList[0].GetGitOldPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED, testList[0].m_Action);
	EXPECT_FALSE(testList[0].IsDirectory());
}

TEST(CTGitPath, ParserFromLsFile_Merged_MultipleFilesConflict)
{
	// as used in CGit::GetWorkingTreeChanges
	constexpr char git_ls_file_u_t_z_output[] = { "M 100644 0ead9277724fc163fe64e1163cc2ff97d5670e41 1	OSMtracker.sln\0M 100644 7a8a41c7c26d259caba707adea36a8b9ae493c97 2	OSMtracker.sln\0M 100644 dcdd1c25bb0ebfd91082b3de5bd45d3f25418027 3	OSMtracker.sln\0M 100644 cbb00533d69b53e40a97f9bcf1c507a71f3c7353 1	OSMtracker/frmMain.vb\0M 100644 1cbfa36fef1af473884ad2e5820075b581fe33af 2	OSMtracker/frmMain.vb\0M 100644 337331224f438f5f49d5e8a4d4c1bafb66f2e67d 3	OSMtracker/frmMain.vb\0M 100644 786e60a550be11ef8e321222ffe6c0fa0f51f23d 1	OSMtracker/osmTileMap.vb\0M 100644 7fe8b75f56cf0202b4640d74d46240d2ba894115 2	OSMtracker/osmTileMap.vb\0M 100644 53d75915e78d0a53ba05c124d613fd75d625c142 3	OSMtracker/osmTileMap.vb" };
	CGitByteArray byteArray;
	byteArray.append(git_ls_file_u_t_z_output, sizeof(git_ls_file_u_t_z_output));
	CTGitPathList testList;
	EXPECT_EQ(0, testList.ParserFromLsFile(byteArray, GIT_HASH_TYPE::GIT_HASH_SHA1));
	EXPECT_EQ(3, testList.GetCount()); // 3 files are conflicted
	EXPECT_STREQ(L"OSMtracker.sln", testList[0].GetGitPathString());
	EXPECT_STREQ(L"OSMtracker/frmMain.vb", testList[1].GetGitPathString());
	EXPECT_STREQ(L"OSMtracker/osmTileMap.vb", testList[2].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED, testList[0].m_Action);
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED, testList[1].m_Action);
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED, testList[2].m_Action);
	EXPECT_FALSE(testList[0].IsDirectory());
	EXPECT_FALSE(testList[1].IsDirectory());
	EXPECT_FALSE(testList[2].IsDirectory());
}

TEST(CTGitPath, FillUnRev)
{
	CAutoTempDir tmpDir;

	CAutoRepository repo;
	ASSERT_TRUE(git_repository_init(repo.GetPointer(), CUnicodeUtils::GetUTF8(tmpDir.GetTempDir()), false) == 0);

	g_Git.m_CurrentDir = tmpDir.GetTempDir();

	CTGitPathList testList;
	EXPECT_EQ(0, testList.FillUnRev(0));
	EXPECT_EQ(0, testList.GetCount());

	EXPECT_EQ(0, testList.FillUnRev(CTGitPath::LOGACTIONS_IGNORE));
	EXPECT_EQ(0, testList.GetCount());

	CString fileOne = tmpDir.GetTempDir() + L"\\one";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(fileOne, L"something"));

	EXPECT_EQ(0, testList.FillUnRev(0));
	EXPECT_EQ(1, testList.GetCount());
	EXPECT_STREQ(L"one", testList[0].GetGitPathString());

	EXPECT_EQ(0, testList.FillUnRev(CTGitPath::LOGACTIONS_IGNORE));
	EXPECT_EQ(0, testList.GetCount());

	CTGitPathList selectList;
	selectList.AddPath(CTGitPath(L"one"));

	EXPECT_EQ(0, testList.FillUnRev(0, &selectList));
	EXPECT_EQ(1, testList.GetCount());
	EXPECT_STREQ(L"one", testList[0].GetGitPathString());

	EXPECT_EQ(0, testList.FillUnRev(CTGitPath::LOGACTIONS_IGNORE, &selectList));
	EXPECT_EQ(0, testList.GetCount());

	CString gitIgnore = tmpDir.GetTempDir() + L"\\.gitignore";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(gitIgnore, L""));

	EXPECT_EQ(0, testList.FillUnRev(0));
	EXPECT_EQ(2, testList.GetCount());
	EXPECT_STREQ(L".gitignore", testList[0].GetGitPathString());
	EXPECT_STREQ(L"one", testList[1].GetGitPathString());

	EXPECT_EQ(0, testList.FillUnRev(CTGitPath::LOGACTIONS_IGNORE));
	EXPECT_EQ(0, testList.GetCount());

	EXPECT_EQ(0, testList.FillUnRev(0, &selectList));
	EXPECT_EQ(1, testList.GetCount());
	EXPECT_STREQ(L"one", testList[0].GetGitPathString());

	EXPECT_EQ(0, testList.FillUnRev(CTGitPath::LOGACTIONS_IGNORE, &selectList));
	EXPECT_EQ(0, testList.GetCount());

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(gitIgnore, L"/one"));

	EXPECT_EQ(0, testList.FillUnRev(0));
	EXPECT_EQ(1, testList.GetCount());
	EXPECT_STREQ(L".gitignore", testList[0].GetGitPathString());

	EXPECT_EQ(0, testList.FillUnRev(CTGitPath::LOGACTIONS_IGNORE));
	EXPECT_EQ(1, testList.GetCount());
	EXPECT_STREQ(L"one", testList[0].GetGitPathString());

	EXPECT_EQ(0, testList.FillUnRev(0, &selectList));
	EXPECT_EQ(0, testList.GetCount());

	EXPECT_EQ(0, testList.FillUnRev(CTGitPath::LOGACTIONS_IGNORE, &selectList));
	EXPECT_EQ(1, testList.GetCount());
	EXPECT_STREQ(L"one", testList[0].GetGitPathString());

	EXPECT_TRUE(CreateDirectory(tmpDir.GetTempDir() + L"\\subdir", nullptr));
	selectList.Clear();
	selectList.AddPath(CTGitPath(L"subdir"));
	EXPECT_EQ(0, testList.FillUnRev(0, &selectList));
	EXPECT_EQ(0, testList.GetCount());

	EXPECT_EQ(0, testList.FillUnRev(CTGitPath::LOGACTIONS_IGNORE, &selectList));
	EXPECT_EQ(0, testList.GetCount());

	fileOne = tmpDir.GetTempDir() + L"\\subdir\\one";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(fileOne, L"something"));

	EXPECT_EQ(0, testList.FillUnRev(0));
	EXPECT_EQ(2, testList.GetCount());
	EXPECT_STREQ(L".gitignore", testList[0].GetGitPathString());
	EXPECT_STREQ(L"subdir/one", testList[1].GetGitPathString());

	EXPECT_EQ(0, testList.FillUnRev(CTGitPath::LOGACTIONS_IGNORE));
	EXPECT_EQ(1, testList.GetCount());
	EXPECT_STREQ(L"one", testList[0].GetGitPathString());

	EXPECT_EQ(0, testList.FillUnRev(0, &selectList));
	EXPECT_EQ(1, testList.GetCount());
	EXPECT_STREQ(L"subdir/one", testList[0].GetGitPathString());

	EXPECT_EQ(0, testList.FillUnRev(CTGitPath::LOGACTIONS_IGNORE, &selectList));
	EXPECT_EQ(0, testList.GetCount());

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(gitIgnore, L"one"));

	EXPECT_EQ(0, testList.FillUnRev(0));
	EXPECT_EQ(1, testList.GetCount());
	EXPECT_STREQ(L".gitignore", testList[0].GetGitPathString());

	EXPECT_EQ(0, testList.FillUnRev(CTGitPath::LOGACTIONS_IGNORE));
	EXPECT_STREQ(L"one", testList[0].GetGitPathString());
	EXPECT_STREQ(L"subdir/one", testList[1].GetGitPathString());

	EXPECT_EQ(0, testList.FillUnRev(CTGitPath::LOGACTIONS_IGNORE, &selectList));
	EXPECT_EQ(1, testList.GetCount());
	EXPECT_STREQ(L"subdir/one", testList[0].GetGitPathString());
}

TEST(CTGitPath, GetAbbreviatedRename)
{
	CTGitPath test;
	CString newName, oldName;

	// just a failsafe
	test.SetFromGit(newName, &oldName);
	EXPECT_STREQ(L"", test.GetAbbreviatedRename());

	oldName = L"B";
	newName = L"A";
	test.SetFromGit(newName, &oldName);
	EXPECT_STREQ(L"B => A", test.GetAbbreviatedRename());

	oldName = L"C/B/A";
	newName = L"A/B/C";
	test.SetFromGit(newName, &oldName);
	EXPECT_STREQ(L"C/B/A => A/B/C", test.GetAbbreviatedRename());

	oldName = L"B/C";
	newName = L"A/C";
	test.SetFromGit(newName, &oldName);
	EXPECT_STREQ(L"{B => A}/C", test.GetAbbreviatedRename());

	oldName = L"C/B";
	newName = L"C/A";
	test.SetFromGit(newName, &oldName);
	EXPECT_STREQ(L"C/{B => A}", test.GetAbbreviatedRename());

	oldName = L"C/D/E";
	newName = L"C/B/A";
	test.SetFromGit(newName, &oldName);
	EXPECT_STREQ(L"C/{D/E => B/A}", test.GetAbbreviatedRename());

	oldName = L"C/D/A";
	newName = L"D/A";
	test.SetFromGit(newName, &oldName);
	EXPECT_STREQ(L"{C/D => D}/A", test.GetAbbreviatedRename());

	oldName = L"A1/B/C/F";
	newName = L"A2/B/C/F";
	test.SetFromGit(newName, &oldName);
	EXPECT_STREQ(L"{A1 => A2}/B/C/F", test.GetAbbreviatedRename());

	oldName = L"C/D/E";
	newName = L"D/E";
	test.SetFromGit(newName, &oldName);
	EXPECT_STREQ(L"{C/D => D}/E", test.GetAbbreviatedRename());

	oldName = L"D/E";
	newName = L"D/F/E";
	test.SetFromGit(newName, &oldName);
	EXPECT_STREQ(L"D/{ => F}/E", test.GetAbbreviatedRename());

	oldName = L"D/F/E";
	newName = L"D/F/F/E";
	test.SetFromGit(newName, &oldName);
	EXPECT_STREQ(L"D/F/{ => F}/E", test.GetAbbreviatedRename());
}

TEST(CTGitPath, HashStashDir)
{
	CAutoTempDir tmpDir;

	CTGitPath path(tmpDir.GetTempDir());
	// no repository -> no stash
	EXPECT_FALSE(path.HasStashDir());

	path.Reset();
	path.SetFromWin(tmpDir.GetTempDir());

	CAutoRepository repo;
	ASSERT_TRUE(git_repository_init(repo.GetPointer(), CUnicodeUtils::GetUTF8(tmpDir.GetTempDir()), false) == 0);
	// "git commit" follows and requires user to be set
	CString configFile = tmpDir.GetTempDir() + L"\\.git\\config";
	CString text;
	ASSERT_TRUE(CStringUtils::ReadStringFromTextFile(configFile, text));
	text += L"[user]\n  name = User\n  email = user@example.com\n";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(configFile, text));

	g_Git.m_CurrentDir = tmpDir.GetTempDir();

	// empty repository no stash
	EXPECT_FALSE(path.HasStashDir());

	CString file(tmpDir.GetTempDir() + L"\\file");
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(file, L"don't care"));

	CString output;
	EXPECT_EQ(0, g_Git.Run(L"git add file", &output, CP_UTF8));
	EXPECT_EQ(0, g_Git.Run(L"git commit -m \"test\"", &output, CP_UTF8));
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(file, L"don't care even more"));
	EXPECT_EQ(0, g_Git.Run(L"git stash", &output, CP_UTF8));
	EXPECT_TRUE(path.HasStashDir());

	// check for packed stash
	EXPECT_EQ(0, g_Git.Run(L"git pack-refs --all", &output, CP_UTF8));
	EXPECT_TRUE(path.HasStashDir());

	EXPECT_EQ(0, g_Git.Run(L"git stash clear", &output, CP_UTF8));
	EXPECT_FALSE(path.HasStashDir());

	file = tmpDir.GetTempDir() + L"\\.git\\packed-refs";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(file, L"# pack-refs with: peeled fully-peeled\nf3a76c72d89aebd63a0346dd92ecafecdc780822 refs/stashNotReal\n"));
	EXPECT_FALSE(path.HasStashDir());
}

class MockCTGitPath : public CTGitPath
{
public:
	MockCTGitPath()
	{
		ON_CALL(*this, UpdateAttributes()).WillByDefault(testing::Invoke(this, &MockCTGitPath::UpdateAttributes_Real));
	}
	MOCK_METHOD(void, UpdateAttributes, (), (const));

private:
	void UpdateAttributes_Real() { CTGitPath::UpdateAttributes(); }
};

TEST(CTGitPath, SetDirectory_DiskAccess)
{
#if _MSC_VER == 1924 && _MSC_FULL_VER < 192428315
	// there is a compiler bug in MSVC, see <https://developercommunity.visualstudio.com/content/problem/847490/msvc-codegen-error-vector-reverse-iterator-x64-c17.html> and <https://github.com/google/googletest/pull/2635>
	GTEST_SKIP();
#else
	MockCTGitPath path;
	path.SetFromGit(L"bla");
	EXPECT_CALL(path, UpdateAttributes()).Times(1);
	EXPECT_FALSE(path.IsDirectory());
	EXPECT_FALSE(path.IsDirectory());
	EXPECT_FALSE(path.Exists());
	path.UnsetDirectoryStatus();
	EXPECT_FALSE(path.IsDirectory());
	EXPECT_FALSE(path.Exists());

	TCHAR winDir[MAX_PATH + 1] = { 0 };
	ASSERT_NE(0u, GetWindowsDirectory(winDir, _countof(winDir)));
	MockCTGitPath pathWin;
	pathWin.SetFromGit(winDir);
	EXPECT_CALL(pathWin, UpdateAttributes()).Times(1);
	EXPECT_TRUE(pathWin.IsDirectory());
	EXPECT_TRUE(pathWin.IsDirectory());
	EXPECT_TRUE(pathWin.Exists());

	MockCTGitPath pathDir;
	pathDir.SetFromGit(L"bla", false);
	EXPECT_CALL(pathDir, UpdateAttributes()).Times(0);
	EXPECT_FALSE(pathDir.IsDirectory());
	pathDir.UnsetDirectoryStatus();
	EXPECT_FALSE(pathDir.IsDirectory());

	MockCTGitPath pathDir2;
	pathDir2.SetFromGit(L"bla", true);
	EXPECT_CALL(pathDir2, UpdateAttributes()).Times(0);
	EXPECT_TRUE(pathDir2.IsDirectory());
	pathDir2.UnsetDirectoryStatus();
	EXPECT_FALSE(pathDir2.IsDirectory());

	MockCTGitPath path2;
	path2.SetFromGit(L"bla", nullptr, nullptr);
	EXPECT_CALL(path2, UpdateAttributes()).Times(1);
	EXPECT_FALSE(path2.IsDirectory());
	EXPECT_FALSE(path2.IsDirectory());
	path2.UnsetDirectoryStatus();
	EXPECT_FALSE(path2.IsDirectory());

	int isDir = FALSE;
	MockCTGitPath path3;
	path3.SetFromGit(L"bla", nullptr, &isDir);
	EXPECT_CALL(path3, UpdateAttributes()).Times(0);
	EXPECT_FALSE(path3.IsDirectory());
	EXPECT_FALSE(path3.IsDirectory());
	path3.UnsetDirectoryStatus();
	EXPECT_FALSE(path3.IsDirectory());

	isDir = TRUE;
	MockCTGitPath pathDir3;
	pathDir3.SetFromGit(L"bla", nullptr, &isDir);
	EXPECT_CALL(pathDir3, UpdateAttributes()).Times(0);
	EXPECT_TRUE(pathDir3.IsDirectory());
	EXPECT_TRUE(pathDir3.IsDirectory());
	pathDir3.UnsetDirectoryStatus();
	EXPECT_FALSE(pathDir3.IsDirectory());
#endif
}

TEST(CTGitPath, AreAllPathsFiles)
{
	CTGitPathList list;
	EXPECT_TRUE(list.AreAllPathsFiles());

	list.AddPath(CTGitPath(L"C:\\Windows\\explorer.exe"));
	EXPECT_TRUE(list.AreAllPathsFiles());

	list.AddPath(CTGitPath(L"C:\\Windows"));
	EXPECT_FALSE(list.AreAllPathsFiles());

	list.Clear();
	EXPECT_TRUE(list.AreAllPathsFiles());

	// now test relative paths
	PreserveChdir chdir;
	CAutoTempDir tmp;
	SetCurrentDirectory(tmp.GetTempDir());

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(tmp.GetTempDir() + L"\\file1", L"something"));
	list.AddPath(CTGitPath(L"file1"));
	EXPECT_TRUE(list.AreAllPathsFiles());

	EXPECT_TRUE(CreateDirectory(tmp.GetTempDir() + L"\\dir", nullptr));
	list.AddPath(CTGitPath(L"dir"));
	EXPECT_FALSE(list.AreAllPathsFiles());

	list.Clear();
	list.AddPath(CTGitPath()); // equivalent of "."
	EXPECT_FALSE(list.AreAllPathsFiles());

	list.AddPath(CTGitPath(L"file1"));
	EXPECT_FALSE(list.AreAllPathsFiles());
}

TEST(CTGitPath, AreAllPathsDirectories)
{
	CTGitPathList list;
	EXPECT_TRUE(list.AreAllPathsDirectories());

	list.AddPath(CTGitPath(L"C:\\Windows"));
	EXPECT_TRUE(list.AreAllPathsDirectories());

	list.AddPath(CTGitPath(L"C:\\Windows\\explorer.exe"));
	EXPECT_FALSE(list.AreAllPathsDirectories());

	list.Clear();
	// now test relative paths
	PreserveChdir chdir;
	CAutoTempDir tmp;
	SetCurrentDirectory(tmp.GetTempDir());

	EXPECT_TRUE(CreateDirectory(tmp.GetTempDir() + L"\\dir", nullptr));
	list.AddPath(CTGitPath()); // equivalent of "."
	EXPECT_TRUE(list.AreAllPathsDirectories());

	list.AddPath(CTGitPath(L"dir"));
	EXPECT_TRUE(list.AreAllPathsDirectories());

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(tmp.GetTempDir() + L"\\file1", L"something"));
	list.AddPath(CTGitPath(L"file1"));
	EXPECT_FALSE(list.AreAllPathsDirectories());
}

TEST(CTGitPath, IsAnyAncestorOf)
{
	CTGitPathList list;
	CTGitPath file(L"a/file");
	EXPECT_FALSE(list.IsAnyAncestorOf(file));

	list.AddPath(CTGitPath(L"something"));
	EXPECT_FALSE(list.IsAnyAncestorOf(file));

	list.AddPath(CTGitPath(L"a"));
	EXPECT_TRUE(list.IsAnyAncestorOf(file));

	list.Clear();
	list.AddPath(CTGitPath());
	EXPECT_TRUE(list.IsAnyAncestorOf(file));
}

TEST(CTGitPath, ParseFromLFSLocks)
{
	// output from git lfs locks --json
	CString output = L"[{\"id\":\"47814\",\"path\":\"2.zip\",\"owner\":{\"name\":\"Sven Strickröth\"},\"locked_at\":\"2020-07-12T14:16:57Z\"},{\"id\":\"49337\",\"path\":\"subdir/3 - Kopie.zip\",\"owner\":{\"name\":\"Sven Strickroth\"},\"locked_at\":\"2020-08-14T21:28:13Z\"},{\"id\":\"49357\",\"path\":\"3.zip\",\"owner\":{\"name\":\"Sven Strickroth\"},\"locked_at\":\"2020-08-16T11:15:14Z\"}]";

	constexpr unsigned int dummy = 5;

	CTGitPathList locks;
	EXPECT_EQ(0, locks.ParserFromLFSLocks(dummy, output));
	ASSERT_EQ(3, locks.GetCount());

	EXPECT_STREQ(L"2.zip", locks[0].GetWinPathString());
	EXPECT_STREQ(L"Sven Strickröth", locks[0].m_LFSLockOwner);
	EXPECT_EQ(dummy, locks[0].m_Action);

	EXPECT_STREQ(L"subdir\\3 - Kopie.zip", locks[1].GetWinPathString());
	EXPECT_STREQ(L"Sven Strickroth", locks[1].m_LFSLockOwner);
	EXPECT_EQ(dummy, locks[1].m_Action);

	EXPECT_STREQ(L"3.zip", locks[2].GetWinPathString());
	EXPECT_STREQ(L"Sven Strickroth", locks[2].m_LFSLockOwner);
	EXPECT_EQ(dummy, locks[2].m_Action);

	EXPECT_EQ(0, locks.ParserFromLFSLocks(0, L"[]"));
	ASSERT_EQ(0, locks.GetCount());

	locks.AddPath(CTGitPath(L"dummy"));
	EXPECT_EQ(1, locks.GetCount());
	EXPECT_EQ(0, locks.ParserFromLFSLocks(0, L""));
	ASSERT_EQ(0, locks.GetCount());

	CString err;
	EXPECT_EQ(-1, locks.ParserFromLFSLocks(0, L"invalid", &err));
	EXPECT_STRNE(L"", err);
}

TEST(CTGitPath, ParseFromLFSLocks_BrokenEmptyId)
{
	// output from git lfs locks --json (crafted to match crash report)
	CString output = L"[{\"id\":\"3185\",\"path\":\"Something.xlsx\",\"owner\":{\"name\":\"Person1\"},\"locked_at\":\"2023-01-31T15:51:05+01:00\"},{\"id\":\"\",\"path\":\"\",\"locked_at\":\"0001-01-01T00:00:00Z\"},{\"id\":\"3849\",\"path\":\"something else.docx\",\"owner\":{\"name\":\"Person2\"},\"locked_at\":\"2023-10-12T18:15:28+02:00\"}]";
	constexpr unsigned int dummy = 5;

	CTGitPathList locks;
	EXPECT_EQ(0, locks.ParserFromLFSLocks(dummy, output));
	ASSERT_EQ(2, locks.GetCount());

	EXPECT_STREQ(L"Something.xlsx", locks[0].GetWinPathString());
	EXPECT_STREQ(L"Person1", locks[0].m_LFSLockOwner);
	EXPECT_EQ(dummy, locks[0].m_Action);

	EXPECT_STREQ(L"something else.docx", locks[1].GetWinPathString());
	EXPECT_STREQ(L"Person2", locks[1].m_LFSLockOwner);
	EXPECT_EQ(dummy, locks[1].m_Action);
}

TEST(CTGitPath, IsRegisteredSubmoduleOfParentProject)
{
	CAutoTempDir tmp;
	CAutoRepository repo;
	EXPECT_EQ(0, git_repository_init(repo.GetPointer(), CUnicodeUtils::GetUTF8(tmp.GetTempDir()), false));
	EXPECT_TRUE(CreateDirectory(tmp.GetTempDir() + L"\\ext", nullptr));
	EXPECT_TRUE(CreateDirectory(tmp.GetTempDir() + L"\\ext\\libgit2", nullptr));
	EXPECT_TRUE(CreateDirectory(tmp.GetTempDir() + L"\\ext\\spell", nullptr));
	EXPECT_TRUE(CreateDirectory(tmp.GetTempDir() + L"\\ext\\something", nullptr));
	{
		CString topPath;
		CTGitPath path { tmp.GetTempDir() + L"\\ext\\libgit2" };
		EXPECT_FALSE(path.IsRegisteredSubmoduleOfParentProject(&topPath));
		EXPECT_STREQ(tmp.GetTempDir(), topPath);
	}
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(tmp.GetTempDir() + L"\\.gitmodules", L"[submodule \"libgit2-ext\"]\n	path = ext/libgit2\n	url = https://github.com/libgit2/libgit2.git\n[submodule \"ext/spell\"]\n	path = ext/spell\n	url = https://github.com/LibreOffice/dictionaries"));
	{
		CString topPath;
		CTGitPath path { tmp.GetTempDir() + L"\\ext\\libgit2" };
		EXPECT_TRUE(path.IsRegisteredSubmoduleOfParentProject(&topPath));
		EXPECT_STREQ(tmp.GetTempDir(), topPath);
	}
	{
		CString topPath;
		CTGitPath path{ tmp.GetTempDir() + L"\\ext\\spell" };
		EXPECT_TRUE(path.IsRegisteredSubmoduleOfParentProject(&topPath));
		EXPECT_STREQ(tmp.GetTempDir(), topPath);
	}
	{
		CString topPath;
		CTGitPath path{ tmp.GetTempDir() + L"\\ext\\spELL" };
		EXPECT_FALSE(path.IsRegisteredSubmoduleOfParentProject(&topPath));
		EXPECT_STREQ(tmp.GetTempDir(), topPath);
	}
	{
		CString topPath;
		CTGitPath path{ tmp.GetTempDir() + L"\\ext\\something" };
		EXPECT_FALSE(path.IsRegisteredSubmoduleOfParentProject(&topPath));
		EXPECT_STREQ(tmp.GetTempDir(), topPath);
	}
	{
		CString topPath;
		CTGitPath path{ tmp.GetTempDir() + L"\\ext" };
		EXPECT_FALSE(path.IsRegisteredSubmoduleOfParentProject(&topPath));
		EXPECT_STREQ(tmp.GetTempDir(), topPath);
	}
}
