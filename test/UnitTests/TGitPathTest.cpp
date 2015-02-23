// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2015 - TortoiseGit
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

TEST(CTGitPath, GetDirectoryTest)
{
	// Bit tricky, this test, because we need to know something about the file
	// layout on the machine which is running the test
	TCHAR winDir[MAX_PATH + 1] = { 0 };
	GetWindowsDirectory(winDir, _countof(winDir));
	CString sWinDir(winDir);

	CTGitPath testPath;
	// This is a file which we know will always be there
	testPath.SetFromUnknown(sWinDir + _T("\\win.ini"));
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
	testPath.SetFromUnknown(_T("C:\\"));
	EXPECT_TRUE(testPath.IsDirectory());
	EXPECT_TRUE(testPath.GetDirectory().GetWinPathString().CompareNoCase(_T("C:\\")) == 0);
	EXPECT_TRUE(testPath.GetContainingDirectory().IsEmpty());
	// Try a root UNC path
	testPath.SetFromUnknown(_T("\\MYSTATION"));
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
	testPath.SetFromUnknown(_T("c:\\.gitdir"));
	EXPECT_FALSE(testPath.IsAdminDir());
	testPath.SetFromUnknown(_T("c:\\test.git"));
	EXPECT_FALSE(testPath.IsAdminDir());
	testPath.SetFromUnknown(_T("c:\\.git"));
	EXPECT_TRUE(testPath.IsAdminDir());
	testPath.SetFromUnknown(_T("c:\\.gitdir\\test"));
	EXPECT_FALSE(testPath.IsAdminDir());
	testPath.SetFromUnknown(_T("c:\\.git\\test"));
	EXPECT_TRUE(testPath.IsAdminDir());

	CTGitPathList pathList;
	pathList.AddPath(CTGitPath(_T("c:\\.gitdir")));
	pathList.AddPath(CTGitPath(_T("c:\\.git")));
	pathList.AddPath(CTGitPath(_T("c:\\.git\\test")));
	pathList.AddPath(CTGitPath(_T("c:\\test")));
	pathList.RemoveAdminPaths();
	EXPECT_EQ(2, pathList.GetCount());
	pathList.Clear();
	EXPECT_EQ(0, pathList.GetCount());
	pathList.AddPath(CTGitPath(_T("c:\\test")));
	pathList.RemoveAdminPaths();
	EXPECT_EQ(1, pathList.GetCount());
}

TEST(CTGitPath, SortTest)
{
	CTGitPathList testList;
	CTGitPath testPath;
	testPath.SetFromUnknown(_T("c:/Z"));
	testList.AddPath(testPath);
	testPath.SetFromUnknown(_T("c:/B"));
	testList.AddPath(testPath);
	testPath.SetFromUnknown(_T("c:\\a"));
	testList.AddPath(testPath);
	testPath.SetFromUnknown(_T("c:/Test"));
	testList.AddPath(testPath);

	EXPECT_EQ(4, testList.GetCount());

	testList.SortByPathname();

	EXPECT_EQ(4, testList.GetCount());
	EXPECT_EQ(_T("c:\\a"), testList[0].GetWinPathString());
	EXPECT_EQ(_T("c:\\B"), testList[1].GetWinPathString());
	EXPECT_EQ(_T("c:\\Test"), testList[2].GetWinPathString());
	EXPECT_EQ(_T("c:\\Z"),testList[3].GetWinPathString());
}

TEST(CTGitPath, RawAppendTest)
{
	CTGitPath testPath(_T("c:/test/"));
	testPath.AppendRawString(_T("/Hello"));
	EXPECT_EQ(_T("c:\\test\\Hello"), testPath.GetWinPathString());

	testPath.AppendRawString(_T("\\T2"));
	EXPECT_EQ(_T("c:\\test\\Hello\\T2"), testPath.GetWinPathString());

	CTGitPath testFilePath(_T("C:\\windows\\win.ini"));
	CTGitPath testBasePath(_T("c:/temp/myfile.txt"));
	testBasePath.AppendRawString(testFilePath.GetFileExtension());
	EXPECT_EQ(_T("c:\\temp\\myfile.txt.ini"), testBasePath.GetWinPathString());
}

TEST(CTGitPath, PathAppendTest)
{
	CTGitPath testPath(_T("c:/test/"));
	testPath.AppendPathString(_T("/Hello"));
	EXPECT_EQ(_T("c:\\test\\Hello"), testPath.GetWinPathString());

	testPath.AppendPathString(_T("T2"));
	EXPECT_EQ(_T("c:\\test\\Hello\\T2"), testPath.GetWinPathString());

	CTGitPath testFilePath(_T("C:\\windows\\win.ini"));
	CTGitPath testBasePath(_T("c:/temp/myfile.txt"));
	// You wouldn't want to do this in real life - you'd use append-raw
	testBasePath.AppendPathString(testFilePath.GetFileExtension());
	EXPECT_EQ(_T("c:\\temp\\myfile.txt\\.ini"), testBasePath.GetWinPathString());
}

TEST(CTGitPath, RemoveDuplicatesTest)
{
	CTGitPathList list;
	list.AddPath(CTGitPath(_T("Z")));
	list.AddPath(CTGitPath(_T("A")));
	list.AddPath(CTGitPath(_T("E")));
	list.AddPath(CTGitPath(_T("E")));

	EXPECT_TRUE(list[2].IsEquivalentTo(list[3]));
	EXPECT_EQ(list[2], list[3]);

	EXPECT_EQ(4, list.GetCount());

	list.RemoveDuplicates();

	EXPECT_EQ(3, list.GetCount());

	EXPECT_STREQ(_T("A"), list[0].GetWinPathString());
	EXPECT_STREQ(_T("E"), list[1].GetWinPathString());
	EXPECT_STREQ(_T("Z"), list[2].GetWinPathString());
}

TEST(CTGitPath, RemoveChildrenTest)
{
	CTGitPathList list;
	list.AddPath(CTGitPath(_T("c:\\test")));
	list.AddPath(CTGitPath(_T("c:\\test\\file")));
	list.AddPath(CTGitPath(_T("c:\\testfile")));
	list.AddPath(CTGitPath(_T("c:\\parent")));
	list.AddPath(CTGitPath(_T("c:\\parent\\child")));
	list.AddPath(CTGitPath(_T("c:\\parent\\child1")));
	list.AddPath(CTGitPath(_T("c:\\parent\\child2")));

	EXPECT_EQ(7, list.GetCount());

	list.RemoveChildren();

	EXPECT_EQ(3, list.GetCount());

	list.SortByPathname();

	EXPECT_STREQ(_T("c:\\parent"), list[0].GetWinPathString());
	EXPECT_STREQ(_T("c:\\test"), list[1].GetWinPathString());
	EXPECT_STREQ(_T("c:\\testfile"), list[2].GetWinPathString());
}

TEST(CTGitPath, ContainingDirectoryTest)
{
	CTGitPath testPath;
	testPath.SetFromWin(_T("c:\\a\\b\\c\\d\\e"));
	CTGitPath dir;
	dir = testPath.GetContainingDirectory();
	EXPECT_STREQ(_T("c:\\a\\b\\c\\d"), dir.GetWinPathString());
	dir = dir.GetContainingDirectory();
	EXPECT_STREQ(_T("c:\\a\\b\\c"), dir.GetWinPathString());
	dir = dir.GetContainingDirectory();
	EXPECT_STREQ(_T("c:\\a\\b"), dir.GetWinPathString());
	dir = dir.GetContainingDirectory();
	EXPECT_STREQ(_T("c:\\a"), dir.GetWinPathString());
	dir = dir.GetContainingDirectory();
	EXPECT_STREQ(_T("c:\\"), dir.GetWinPathString());
	dir = dir.GetContainingDirectory();
	EXPECT_TRUE(dir.IsEmpty());
	EXPECT_STREQ(_T(""), dir.GetWinPathString());
}

TEST(CTGitPath, AncestorTest)
{
	CTGitPath testPath;
	testPath.SetFromWin(_T("c:\\windows"));
	EXPECT_FALSE(testPath.IsAncestorOf(CTGitPath(_T("c:\\"))));
	EXPECT_TRUE(testPath.IsAncestorOf(CTGitPath(_T("c:\\windows"))));
	EXPECT_FALSE(testPath.IsAncestorOf(CTGitPath(_T("c:\\windowsdummy"))));
	EXPECT_TRUE(testPath.IsAncestorOf(CTGitPath(_T("c:\\windows\\test.txt"))));
	EXPECT_TRUE(testPath.IsAncestorOf(CTGitPath(_T("c:\\windows\\system32\\test.txt"))));
}

/*TEST(CTGitPath, SubversionPathTest)
{
	CTGitPath testPath;
	testPath.SetFromWin(_T("c:\\"));
	EXPECT_TRUE((testPath.GetGitApiPath(pool), "c:") == 0);
	testPath.SetFromWin(_T("c:\\folder"));
	EXPECT_TRUE(strcmp(testPath.GetGitApiPath(pool), "c:/folder") == 0);
	testPath.SetFromWin(_T("c:\\a\\b\\c\\d\\e"));
	EXPECT_TRUE(strcmp(testPath.GetGitApiPath(pool), "c:/a/b/c/d/e") == 0);
	testPath.SetFromUnknown(_T("http://testing/"));
	EXPECT_TRUE(strcmp(testPath.GetGitApiPath(pool), "http://testing") == 0);
	testPath.SetFromGit(NULL);
	EXPECT_TRUE(strlen(testPath.GetGitApiPath(pool)) == 0);

	testPath.SetFromUnknown(_T("http://testing again"));
	EXPECT_TRUE(strcmp(testPath.GetGitApiPath(pool), "http://testing%20again") == 0);
	testPath.SetFromUnknown(_T("http://testing%20again"));
	EXPECT_TRUE(strcmp(testPath.GetGitApiPath(pool), "http://testing%20again") == 0);
	testPath.SetFromUnknown(_T("http://testing special chars \344\366\374"));
	EXPECT_TRUE(strcmp(testPath.GetGitApiPath(pool), "http://testing%20special%20chars%20%c3%a4%c3%b6%c3%bc") == 0);
}*/

TEST(CTGitPath, GetCommonRootTest)
{
	CTGitPath pathA(_T("C:\\Development\\LogDlg.cpp"));
	CTGitPath pathB(_T("C:\\Development\\LogDlg.h"));
	CTGitPath pathC(_T("C:\\Development\\SomeDir\\LogDlg.h"));

	CTGitPathList list;
	list.AddPath(pathA);
	EXPECT_TRUE(list.GetCommonRoot().GetWinPathString().CompareNoCase(_T("C:\\Development\\LogDlg.cpp")) == 0);
	list.AddPath(pathB);
	EXPECT_TRUE(list.GetCommonRoot().GetWinPathString().CompareNoCase(_T("C:\\Development")) == 0);
	list.AddPath(pathC);
	EXPECT_TRUE(list.GetCommonRoot().GetWinPathString().CompareNoCase(_T("C:\\Development")) == 0);

	list.Clear();
	CString sPathList = _T("D:\\Development\\StExBar\\StExBar\\src\\setup\\Setup64.wxs*D:\\Development\\StExBar\\StExBar\\src\\setup\\Setup.wxs*D:\\Development\\StExBar\\SKTimeStamp\\src\\setup\\Setup.wxs*D:\\Development\\StExBar\\SKTimeStamp\\src\\setup\\Setup64.wxs");
	list.LoadFromAsteriskSeparatedString(sPathList);
	EXPECT_TRUE(list.GetCommonRoot().GetWinPathString().CompareNoCase(_T("D:\\Development\\StExBar")) == 0);

	list.Clear();
	sPathList = _T("c:\\windows\\explorer.exe*c:\\windows");
	list.LoadFromAsteriskSeparatedString(sPathList);
	EXPECT_TRUE(list.GetCommonRoot().GetWinPathString().CompareNoCase(_T("c:\\windows")) == 0);

	list.Clear();
	sPathList = _T("c:\\windows\\*c:\\windows");
	list.LoadFromAsteriskSeparatedString(sPathList);
	EXPECT_TRUE(list.GetCommonRoot().GetWinPathString().CompareNoCase(_T("c:\\windows")) == 0);

	list.Clear();
	sPathList = _T("c:\\windows\\system32*c:\\windows\\system");
	list.LoadFromAsteriskSeparatedString(sPathList);
	EXPECT_TRUE(list.GetCommonRoot().GetWinPathString().CompareNoCase(_T("c:\\windows")) == 0);

	list.Clear();
	sPathList = _T("c:\\windowsdummy*c:\\windows");
	list.LoadFromAsteriskSeparatedString(sPathList);
	EXPECT_TRUE(list.GetCommonRoot().GetWinPathString().CompareNoCase(_T("c:\\")) == 0);
}

TEST(CTGitPath, ValidPathAndUrlTest)
{
	CTGitPath testPath;
	testPath.SetFromWin(_T("c:\\a\\b\\c.test.txt"));
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromWin(_T("c:\\"));
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromWin(_T("D:\\.Net\\SpindleSearch\\"));
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromWin(_T("c"));
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromWin(_T("c:\\test folder\\file"));
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromWin(_T("c:\\folder\\"));
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromWin(_T("c:\\ext.ext.ext\\ext.ext.ext.ext"));
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromWin(_T("c:\\.git"));
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromWin(_T("c:\\com\\file"));
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromWin(_T("c:\\test\\conf"));
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromWin(_T("c:\\LPT"));
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromWin(_T("c:\\test\\LPT"));
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromWin(_T("c:\\com1test"));
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromWin(_T("\\\\?\\c:\\test\\com1test"));
	EXPECT_TRUE(testPath.IsValidOnWindows());

	testPath.SetFromWin(_T("\\\\Share\\filename"));
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromWin(_T("\\\\Share\\filename.extension"));
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromWin(_T("\\\\Share\\.git"));
	EXPECT_TRUE(testPath.IsValidOnWindows());

	// now the negative tests
	testPath.SetFromWin(_T("c:\\test:folder"));
	EXPECT_FALSE(testPath.IsValidOnWindows());
	testPath.SetFromWin(_T("c:\\file<name"));
	EXPECT_FALSE(testPath.IsValidOnWindows());
	testPath.SetFromWin(_T("c:\\something*else"));
	EXPECT_FALSE(testPath.IsValidOnWindows());
	testPath.SetFromWin(_T("c:\\folder\\file?nofile"));
	EXPECT_FALSE(testPath.IsValidOnWindows());
	testPath.SetFromWin(_T("c:\\ext.>ension"));
	EXPECT_FALSE(testPath.IsValidOnWindows());
	testPath.SetFromWin(_T("c:\\com1\\filename"));
	EXPECT_FALSE(testPath.IsValidOnWindows());
	testPath.SetFromWin(_T("c:\\com1"));
	EXPECT_FALSE(testPath.IsValidOnWindows());
	testPath.SetFromWin(_T("c:\\com1\\AuX"));
	EXPECT_FALSE(testPath.IsValidOnWindows());

	testPath.SetFromWin(_T("\\\\Share\\lpt9\\filename"));
	EXPECT_FALSE(testPath.IsValidOnWindows());
	testPath.SetFromWin(_T("\\\\Share\\prn"));
	EXPECT_FALSE(testPath.IsValidOnWindows());
	testPath.SetFromWin(_T("\\\\Share\\NUL"));
	EXPECT_FALSE(testPath.IsValidOnWindows());

	// now come some URL tests
	/*testPath.SetFromGit(_T("http://myserver.com/repos/trunk"));
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromGit(_T("https://myserver.com/repos/trunk/file%20with%20spaces"));
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromGit(_T("svn://myserver.com/repos/trunk/file with spaces"));
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromGit(_T("svn+ssh://www.myserver.com/repos/trunk"));
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromGit(_T("http://localhost:90/repos/trunk"));
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromGit(_T("file:///C:/GitRepos/Tester/Proj1/tags/t2"));
	EXPECT_TRUE(testPath.IsValidOnWindows());
	// and some negative URL tests
	testPath.SetFromGit(_T("httpp://myserver.com/repos/trunk"));
	EXPECT_FALSE(testPath.IsValidOnWindows());
	testPath.SetFromGit(_T("https://myserver.com/rep:os/trunk/file%20with%20spaces"));
	EXPECT_FALSE(testPath.IsValidOnWindows());
	testPath.SetFromGit(_T("svn://myserver.com/rep<os/trunk/file with spaces"));
	EXPECT_FALSE(testPath.IsValidOnWindows());
	testPath.SetFromGit(_T("svn+ssh://www.myserver.com/repos/trunk/prn/"));
	EXPECT_FALSE(testPath.IsValidOnWindows());
	testPath.SetFromGit(_T("http://localhost:90/repos/trunk/com1"));
	EXPECT_FALSE(testPath.IsValidOnWindows());*/
}

TEST(CTGitPath, ListLoadingTest)
{
	TCHAR buf[MAX_PATH] = { 0 };
	GetCurrentDirectory(MAX_PATH, buf);
	CString sPathList(_T("Path1*c:\\path2 with spaces and stuff*\\funnypath\\*"));
	CTGitPathList testList;
	testList.LoadFromAsteriskSeparatedString(sPathList);

	EXPECT_EQ(3, testList.GetCount());
	EXPECT_TRUE(testList[0].GetWinPathString() == CString(buf) + _T("\\Path1"));
	EXPECT_TRUE(testList[1].GetWinPathString() == _T("c:\\path2 with spaces and stuff"));
	EXPECT_TRUE(testList[2].GetWinPathString() == _T("\\funnypath"));

	EXPECT_TRUE(testList.GetCommonRoot().GetWinPathString() == _T(""));
	testList.Clear();
	sPathList = _T("c:\\path2 with spaces and stuff*c:\\funnypath\\*");
	testList.LoadFromAsteriskSeparatedString(sPathList);
	EXPECT_TRUE(testList.GetCommonRoot().GetWinPathString() == _T("c:\\"));
}
