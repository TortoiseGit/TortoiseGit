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
#include "RepositoryFixtures.h"
#include "gitindex.h"
#include "gitdll.h"

extern CGitAdminDirMap g_AdminDirMap; // not optimal yet

class GitIndexCBasicGitFixture : public CBasicGitFixture
{
protected:
	virtual void SetUp()
	{
		CBasicGitFixture::SetUp();
		g_AdminDirMap.clear();
	}
};

class GitIndexCBasicGitWithEmptyRepositoryFixture : public CBasicGitWithEmptyRepositoryFixture
{
protected:
	virtual void SetUp()
	{
		CBasicGitWithEmptyRepositoryFixture::SetUp();
		g_AdminDirMap.clear();
	}
};

class GitIndexCBasicGitWithTestRepoFixture : public CBasicGitWithTestRepoFixture
{
protected:
	virtual void SetUp()
	{
		CBasicGitWithTestRepoFixture::SetUp();
		g_AdminDirMap.clear();
	}
};

INSTANTIATE_TEST_CASE_P(GitIndex, GitIndexCBasicGitFixture, testing::Values(LIBGIT2));
INSTANTIATE_TEST_CASE_P(GitIndex, GitIndexCBasicGitWithEmptyRepositoryFixture, testing::Values(LIBGIT2));
INSTANTIATE_TEST_CASE_P(GitIndex, GitIndexCBasicGitWithTestRepoFixture, testing::Values(LIBGIT2));

TEST_P(GitIndexCBasicGitFixture, EmptyDir)
{
	CGitIndexList indexList;
	EXPECT_EQ(-1, indexList.ReadIndex(m_Dir.GetTempDir()));
	EXPECT_EQ(0, indexList.size());
	EXPECT_FALSE(indexList.m_bHasConflicts);
}


static void ReadAndCheckIndex(CGitIndexList& indexList, const CString& gitdir, int offset = 0)
{
	EXPECT_EQ(0, indexList.ReadIndex(gitdir));
	ASSERT_EQ(14 + offset, indexList.size());

	EXPECT_STREQ(L"ansi.txt", indexList[offset].m_FileName);
	EXPECT_EQ(102, indexList[offset].m_Size);
	EXPECT_EQ(8, indexList[offset].m_Flags);
	EXPECT_EQ(0, indexList[offset].m_FlagsExtended);
	EXPECT_STREQ(L"961bdffbfce1bc617fb594091c3229f1cc674d76", indexList[offset].m_IndexHash.ToString());
	EXPECT_STREQ(L"copy/ansi.txt", indexList[1 + offset].m_FileName);
	EXPECT_EQ(103, indexList[1 + offset].m_Size);
	EXPECT_EQ(13, indexList[1 + offset].m_Flags);
	EXPECT_EQ(0, indexList[1 + offset].m_FlagsExtended);
	EXPECT_STREQ(L"4c44667203f943dc5dbdf3cb526cb7ec24f60c09", indexList[1 + offset].m_IndexHash.ToString());
	EXPECT_STREQ(L"copy/utf16-le-nobom.txt", indexList[5 + offset].m_FileName);
	EXPECT_EQ(218, indexList[5 + offset].m_Size);
	EXPECT_EQ(23, indexList[5 + offset].m_Flags);
	EXPECT_EQ(0, indexList[5 + offset].m_FlagsExtended);
	EXPECT_STREQ(L"fbea9ccd85c33fcdb542d8c73f910ea0e70c3ddc", indexList[5 + offset].m_IndexHash.ToString());
	EXPECT_STREQ(L"utf8-nobom.txt", indexList[13 + offset].m_FileName);
	EXPECT_EQ(139, indexList[13 + offset].m_Size);
	EXPECT_EQ(14, indexList[13 + offset].m_Flags);
	EXPECT_EQ(0, indexList[13 + offset].m_FlagsExtended);
	EXPECT_STREQ(L"c225b3f14869ec8b6da32d52bd15dba0b043031d", indexList[13 + offset].m_IndexHash.ToString());
	EXPECT_FALSE(indexList.m_bHasConflicts);
}

TEST_P(GitIndexCBasicGitWithTestRepoFixture, ReadIndex)
{
	CGitIndexList indexList;
	ReadAndCheckIndex(indexList, m_Dir.GetTempDir());

	CString testFile = m_Dir.GetTempDir() + L"\\1.txt";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)testFile, L"this is testing file."));
	CString output;
	EXPECT_EQ(0, m_Git.Run(_T("git.exe add 1.txt"), &output, CP_UTF8));
	EXPECT_TRUE(output.IsEmpty());

	ReadAndCheckIndex(indexList, m_Dir.GetTempDir(), 1);

	EXPECT_STREQ(L"1.txt", indexList[0].m_FileName);
	EXPECT_EQ(21, indexList[0].m_Size);
	EXPECT_EQ(5, indexList[0].m_Flags);
	EXPECT_EQ(0, indexList[0].m_FlagsExtended);
	EXPECT_STREQ(L"e4aac1275dfc440ec521a76e9458476fe07038bb", indexList[0].m_IndexHash.ToString());

	EXPECT_EQ(0, m_Git.Run(_T("git.exe rm -f 1.txt"), &output, CP_UTF8));

	ReadAndCheckIndex(indexList, m_Dir.GetTempDir());

	output.Empty();
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)testFile, L"this is testing file."));
	EXPECT_EQ(0, m_Git.Run(_T("git.exe add -N 1.txt"), &output, CP_UTF8));
	EXPECT_TRUE(output.IsEmpty());

	ReadAndCheckIndex(indexList, m_Dir.GetTempDir(), 1);

	EXPECT_STREQ(L"1.txt", indexList[0].m_FileName);
	EXPECT_EQ(0, indexList[0].m_Size);
	EXPECT_EQ(16389, indexList[0].m_Flags);
	EXPECT_EQ(GIT_IDXENTRY_INTENT_TO_ADD, indexList[0].m_FlagsExtended);
	EXPECT_STREQ(L"e69de29bb2d1d6434b8b29ae775ad8c2e48c5391", indexList[0].m_IndexHash.ToString());
}

TEST_P(GitIndexCBasicGitWithTestRepoFixture, GetFileStatus)
{
	CGitIndexList indexList;
	ReadAndCheckIndex(indexList, m_Dir.GetTempDir());

	git_wc_status_kind status = git_wc_status_none;
	EXPECT_EQ(0, indexList.GetFileStatus(m_Dir.GetTempDir(), L"does-not-exist.txt", &status, 10, 20));
	EXPECT_EQ(git_wc_status_unversioned, status);

	__int64 time = -1;
	__int64 filesize = -1;
	status = git_wc_status_none;
	bool skipworktree = false;
	EXPECT_EQ(-1, CGit::GetFileModifyTime(CombinePath(m_Dir.GetTempDir(), L"ansi.txt"), &time, nullptr, &filesize));
	EXPECT_EQ(0, indexList.GetFileStatus(m_Dir.GetTempDir(), L"ansi.txt", &status, time, filesize));
	EXPECT_EQ(git_wc_status_modified, status);

	CString output;
	EXPECT_EQ(0, m_Git.Run(_T("git.exe reset --hard"), &output, CP_UTF8));

	EXPECT_EQ(0, CGit::GetFileModifyTime(CombinePath(m_Dir.GetTempDir(), L"ansi.txt"), &time, nullptr, &filesize));
	status = git_wc_status_none;
	EXPECT_EQ(0, indexList.GetFileStatus(m_Dir.GetTempDir(), L"ansi.txt", &status, time, filesize));
	EXPECT_EQ(git_wc_status_normal, status);

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)CombinePath(m_Dir.GetTempDir(), L"ansi.txt"), L"this is testing file."));
	EXPECT_EQ(0, CGit::GetFileModifyTime(CombinePath(m_Dir.GetTempDir(), L"ansi.txt"), &time, nullptr, &filesize));
	status = git_wc_status_none;
	EXPECT_EQ(0, indexList.GetFileStatus(m_Dir.GetTempDir(), L"ansi.txt", &status, time, filesize, nullptr, nullptr, nullptr, nullptr, &skipworktree));
	EXPECT_EQ(git_wc_status_modified, status);
	EXPECT_FALSE(skipworktree);

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)CombinePath(m_Dir.GetTempDir(), L"just-added.txt"), L"this is testing file."));
	EXPECT_EQ(0, m_Git.Run(_T("git.exe add -- just-added.txt"), &output, CP_UTF8));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)CombinePath(m_Dir.GetTempDir(), L"noted-as-added.txt"), L"this is testing file."));
	EXPECT_EQ(0, m_Git.Run(_T("git.exe add -N -- noted-as-added.txt"), &output, CP_UTF8));

	EXPECT_EQ(0, m_Git.Run(_T("git.exe update-index --skip-worktree -- ansi.txt"), &output, CP_UTF8));
	EXPECT_EQ(0, indexList.ReadIndex(m_Dir.GetTempDir()));
	EXPECT_FALSE(indexList.m_bHasConflicts);
	status = git_wc_status_none;
	EXPECT_EQ(0, indexList.GetFileStatus(m_Dir.GetTempDir(), L"ansi.txt", &status, time, filesize, nullptr, nullptr, nullptr, nullptr, &skipworktree));
	EXPECT_EQ(git_wc_status_normal, status);
	EXPECT_TRUE(skipworktree);

	EXPECT_EQ(0, CGit::GetFileModifyTime(CombinePath(m_Dir.GetTempDir(), L"just-added.txt"), &time, nullptr, &filesize));
	status = git_wc_status_none;
	EXPECT_EQ(0, indexList.GetFileStatus(m_Dir.GetTempDir(), L"just-added.txt", &status, time, filesize));
	EXPECT_EQ(git_wc_status_normal, status);

	EXPECT_EQ(0, CGit::GetFileModifyTime(CombinePath(m_Dir.GetTempDir(), L"noted-as-added.txt"), &time, nullptr, &filesize));
	status = git_wc_status_none;
	EXPECT_EQ(0, indexList.GetFileStatus(m_Dir.GetTempDir(), L"noted-as-added.txt", &status, time, filesize));
	EXPECT_EQ(git_wc_status_added, status);

	EXPECT_EQ(0, m_Git.Run(_T("git.exe update-index --no-skip-worktree ansi.txt"), &output, CP_UTF8));

	Sleep(1000);
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)CombinePath(m_Dir.GetTempDir(), L"just-added.txt"), L"this IS testing file."));
	EXPECT_EQ(0, CGit::GetFileModifyTime(CombinePath(m_Dir.GetTempDir(), L"just-added.txt"), &time, nullptr, &filesize));
	status = git_wc_status_none;
	EXPECT_EQ(0, indexList.GetFileStatus(m_Dir.GetTempDir(), L"just-added.txt", &status, time, filesize));
	EXPECT_EQ(git_wc_status_modified, status);

	output.Empty();
	EXPECT_EQ(0, m_Git.Run(_T("git.exe checkout --force forconflict"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());

	output.Empty();
	EXPECT_EQ(1, m_Git.Run(_T("git.exe merge simple-conflict"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());

	EXPECT_EQ(0, indexList.ReadIndex(m_Dir.GetTempDir()));
	EXPECT_EQ(9, indexList.size());
	EXPECT_TRUE(indexList.m_bHasConflicts);

	EXPECT_EQ(0, CGit::GetFileModifyTime(CombinePath(m_Dir.GetTempDir(), L"ansi.txt"), &time, nullptr, &filesize));
	status = git_wc_status_none;
	EXPECT_EQ(0, indexList.GetFileStatus(m_Dir.GetTempDir(), L"ansi.txt", &status, time, filesize));
	EXPECT_EQ(git_wc_status_conflicted, status);
}

TEST(GitIndex, SearchInSortVector)
{
	std::vector<CGitFileName> vector;
	EXPECT_EQ(-1, SearchInSortVector(vector, L"something", 9));
	EXPECT_EQ(-1, SearchInSortVector(vector, L"something", 0));
	EXPECT_EQ(-1, SearchInSortVector(vector, L"something", -1));

	vector.push_back(CGitFileName(L"One"));
	EXPECT_EQ(-1, SearchInSortVector(vector, L"something", 9));
	EXPECT_EQ(-1, SearchInSortVector(vector, L"something", -1));
	EXPECT_EQ(0, SearchInSortVector(vector, L"something", 0)); // do we really need this behavior?
	EXPECT_EQ(0, SearchInSortVector(vector, L"one", 3));
	EXPECT_EQ(0, SearchInSortVector(vector, L"one", -1));
	EXPECT_EQ(-1, SearchInSortVector(vector, L"one/", 4));
	EXPECT_EQ(-1, SearchInSortVector(vector, L"One", 3));

	vector.push_back(CGitFileName(L"tWo"));
	EXPECT_EQ(-1, SearchInSortVector(vector, L"something", 9));
	EXPECT_EQ(0, SearchInSortVector(vector, L"one", 3));
	EXPECT_EQ(-1, SearchInSortVector(vector, L"One", 3));
	EXPECT_EQ(1, SearchInSortVector(vector, L"two", 3));
	EXPECT_EQ(-1, SearchInSortVector(vector, L"tWo", 3));
	EXPECT_EQ(1, SearchInSortVector(vector, L"t", 1));
	EXPECT_EQ(-1, SearchInSortVector(vector, L"0", 1));
	EXPECT_EQ(-1, SearchInSortVector(vector, L"z", 1));

	vector.push_back(CGitFileName(L"a"));
	vector.push_back(CGitFileName(L"b/1"));
	vector.push_back(CGitFileName(L"b/2"));
	vector.push_back(CGitFileName(L"b/3"));
	vector.push_back(CGitFileName(L"b/4"));
	vector.push_back(CGitFileName(L"b/5"));
	std::sort(vector.begin(), vector.end(), SortCGitFileName);
	EXPECT_EQ(2, SearchInSortVector(vector, L"b/2", 3));
	EXPECT_EQ(2, SearchInSortVector(vector, L"b/2", -1));
	EXPECT_EQ(3, SearchInSortVector(vector, L"b/", 2));
	EXPECT_EQ(-1, SearchInSortVector(vector, L"b/6", 3));
	EXPECT_EQ(0, SearchInSortVector(vector, L"a", 1));
	EXPECT_EQ(6, SearchInSortVector(vector, L"one", 3));
	EXPECT_EQ(7, SearchInSortVector(vector, L"two", 3));
}

TEST(GitIndex, GetRangeInSortVector)
{
	std::vector<CGitFileName> vector;

	int start = -2;
	int end = -2;
	EXPECT_EQ(-1, GetRangeInSortVector(vector, L"something", 9, &start, &end, -1));
	EXPECT_EQ(-1, GetRangeInSortVector(vector, L"something", 9, &start, &end, 0));
	EXPECT_EQ(-1, GetRangeInSortVector(vector, L"something", 9, &start, nullptr, 0));
	EXPECT_EQ(-1, GetRangeInSortVector(vector, L"something", 9, nullptr, &end, 0));
	EXPECT_EQ(-1, GetRangeInSortVector(vector, L"something", 9, nullptr, &end, 1));
	EXPECT_EQ(-1, GetRangeInSortVector(vector, L"", 0, &start, &end, 0));
	EXPECT_EQ(-1, start);
	EXPECT_EQ(-1, end);

	vector.push_back(CGitFileName(L"a"));
	start = end = -2;
	EXPECT_EQ(-1, GetRangeInSortVector(vector, L"something", 9, &start, &end, -1));
	EXPECT_EQ(-1, GetRangeInSortVector(vector, L"something", 9, &start, nullptr, 0));
	EXPECT_EQ(-1, GetRangeInSortVector(vector, L"something", 9, nullptr, &end, 0));
	EXPECT_EQ(-1, GetRangeInSortVector(vector, L"something", 9, nullptr, &end, 1));

	EXPECT_EQ(-1, GetRangeInSortVector(vector, L"a", 1, &start, &end, -1));
	EXPECT_EQ(-1, GetRangeInSortVector(vector, L"a", 1, &start, nullptr, 0));
	EXPECT_EQ(-1, GetRangeInSortVector(vector, L"a", 1, nullptr, &end, 0));
	EXPECT_EQ(-1, GetRangeInSortVector(vector, L"a", 1, nullptr, &end, 1));
	start = end = -2;
	EXPECT_EQ(0, GetRangeInSortVector(vector, L"", 0, &start, &end, 0));
	EXPECT_EQ(0, start);
	EXPECT_EQ(0, end);
	start = end = -2;
	EXPECT_EQ(-1, GetRangeInSortVector(vector, L"", 0, &start, &end, 1));

	start = end = -2;
	EXPECT_EQ(-1, GetRangeInSortVector(vector, L"0", 1, &start, &end, 0));
	EXPECT_EQ(-1, start);
	EXPECT_EQ(-1, end);
	start = end = -2;
	EXPECT_EQ(-1, GetRangeInSortVector(vector, L"b", 1, &start, &end, 0));
	EXPECT_EQ(-1, start);
	EXPECT_EQ(-1, end);

	start = end = -2;
	EXPECT_EQ(0, GetRangeInSortVector(vector, L"a", 1, &start, &end, 0));
	EXPECT_EQ(0, start);
	EXPECT_EQ(0, end);

	vector.push_back(CGitFileName(L"b/1"));
	vector.push_back(CGitFileName(L"b/2"));
	vector.push_back(CGitFileName(L"b/3"));
	vector.push_back(CGitFileName(L"b/4"));
	vector.push_back(CGitFileName(L"b/5"));

	start = end = -2;
	EXPECT_EQ(0, GetRangeInSortVector(vector, L"a", 1, &start, &end, 0));
	EXPECT_EQ(0, start);
	EXPECT_EQ(0, end);

	start = end = -2;
	EXPECT_EQ(0, GetRangeInSortVector(vector, L"b/", 2, &start, &end, 1));
	EXPECT_EQ(1, start);
	EXPECT_EQ(5, end);

	start = end = -2;
	EXPECT_EQ(0, GetRangeInSortVector(vector, L"b/", 2, &start, &end, 2));
	EXPECT_EQ(1, start);
	EXPECT_EQ(5, end);

	start = end = -2;
	EXPECT_EQ(0, GetRangeInSortVector(vector, L"b/", 2, &start, &end, 4));
	EXPECT_EQ(1, start);
	EXPECT_EQ(5, end);

	start = end = -2;
	EXPECT_EQ(0, GetRangeInSortVector(vector, L"b/", 2, &start, &end, 5));
	EXPECT_EQ(1, start);
	EXPECT_EQ(5, end);

	EXPECT_EQ(-1, GetRangeInSortVector(vector, L"b/", 2, &start, &end, 6)); // 6 is >= vector.size()

	start = end = -2;
	EXPECT_EQ(-1, GetRangeInSortVector(vector, L"c/", 2, &start, &end, 0));
	EXPECT_EQ(-1, start);
	EXPECT_EQ(-1, end);

	start = end = -2;
	EXPECT_EQ(-1, GetRangeInSortVector(vector, L"c/", 2, &start, &end, 5));
	EXPECT_EQ(-1, start);
	EXPECT_EQ(-1, end);

	vector.push_back(CGitFileName(L"c"));
	vector.push_back(CGitFileName(L"d"));

	start = end = -2;
	EXPECT_EQ(0, GetRangeInSortVector(vector, L"b/", 2, &start, &end, 1));
	EXPECT_EQ(1, start);
	EXPECT_EQ(5, end);

	start = end = -2;
	EXPECT_EQ(0, GetRangeInSortVector(vector, L"b/", 2, &start, &end, 2));
	EXPECT_EQ(1, start);
	EXPECT_EQ(5, end);

	start = end = -2;
	EXPECT_EQ(0, GetRangeInSortVector(vector, L"b/", 2, &start, &end, 4));
	EXPECT_EQ(1, start);
	EXPECT_EQ(5, end);

	start = end = -2;
	EXPECT_EQ(0, GetRangeInSortVector(vector, L"b/", 2, &start, &end, 5));
	EXPECT_EQ(1, start);
	EXPECT_EQ(5, end);

	start = end = -2;
	EXPECT_EQ(-1, GetRangeInSortVector(vector, L"c/", 2, &start, &end, 6));
	EXPECT_EQ(-1, start);
	EXPECT_EQ(-1, end);

	start = end = -2;
	EXPECT_EQ(0, GetRangeInSortVector(vector, L"c", 1, &start, &end, 6));
	EXPECT_EQ(6, start);
	EXPECT_EQ(6, end);

	start = end = -2;
	EXPECT_EQ(0, GetRangeInSortVector(vector, L"", 0, &start, &end, 0));
	EXPECT_EQ(0, start);
	EXPECT_EQ(7, end);
}

TEST(GitIndex, CGitIgnoreItem)
{
	CAutoTempDir tempDir;
	CGitIgnoreItem ignoreItem;

	int type = DT_DIR;
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("not-ignored", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("subdir/not-ignored", type));
	type = DT_REG;
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("not-ignored", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("subdir/not-ignored", type));

	EXPECT_EQ(-1, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), L"does-not-exist", false));
	EXPECT_EQ(-1, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), L"does-not-exist", true));

	CString ignoreFile = tempDir.GetTempDir() + L"\\.gitignore";

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)ignoreFile, L""));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, false));
	EXPECT_STREQ("", ignoreItem.m_BaseDir);
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, true));
	EXPECT_STREQ("", ignoreItem.m_BaseDir);

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)ignoreFile, L"#"));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, false));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, true));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)ignoreFile, L"# comment"));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, false));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, true));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)ignoreFile, L"\n"));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, false));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, true));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)ignoreFile, L"\n#"));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, false));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, true));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)ignoreFile, L"*.tmp\n"));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, false));
	type = DT_DIR;
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("not-ignored", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("text.tmp", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/text.tmp", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("1.tmp.1", type));
	type = DT_REG;
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("not-ignored", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("text.tmp", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/text.tmp", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("1.tmp.1", type));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, true));
	type = DT_DIR;
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("not-ignored", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("text.tmp", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/text.tmp", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("1.tmp.1", type));
	type = DT_REG;
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("not-ignored", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("text.tmp", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/text.tmp", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("1.tmp.1", type));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)ignoreFile, L"some-file\n"));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, false));
	type = DT_DIR;
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("not-ignored", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("subdir/not-ignored", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-file", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/some-file", type));
	type = DT_REG;
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("not-ignored", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("subdir/not-ignored", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-file", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/some-file", type));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, true));
	type = DT_DIR;
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("not-ignored", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("subdir/not-ignored", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-file", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/some-file", type));
	type = DT_REG;
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("not-ignored", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("subdir/not-ignored", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-file", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/some-file", type));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)ignoreFile, L"\n\nsome-file\n"));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, false));
	type = DT_DIR;
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-file", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/some-file", type));
	type = DT_REG;
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-file", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/some-file", type));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, true));
	type = DT_DIR;
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-file", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/some-file", type));
	type = DT_REG;
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-file", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/some-file", type));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)ignoreFile, L"/some-file"));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, false));
	type = DT_DIR;
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-file", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("subdir/some-file", type));
	type = DT_REG;
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-file", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("subdir/some-file", type));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, true));
	type = DT_DIR;
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-file", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("subdir/some-file", type));
	type = DT_REG;
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-file", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("subdir/some-file", type));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)ignoreFile, L"some-dir/"));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, false));
	type = DT_DIR;
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-dir", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("some-dir/some-file", type));
	type = DT_REG;
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("some-file", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("some-dir", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("some-dir/some-file", type));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, true));
	type = DT_DIR;
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-dir", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("some-dir/some-file", type));
	type = DT_REG;
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("some-file", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("some-dir", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("some-dir/some-file", type));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)ignoreFile, L"some-*\n!some-file"));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, false));
	type = DT_DIR;
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-dir", type));
	EXPECT_EQ(0, ignoreItem.IsPathIgnored("some-file", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("something", type));
	type = DT_REG;
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-dir", type));
	EXPECT_EQ(0, ignoreItem.IsPathIgnored("some-file", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("something", type));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, true));
	type = DT_DIR;
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-dir", type));
	EXPECT_EQ(0, ignoreItem.IsPathIgnored("some-file", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("something", type));
	type = DT_REG;
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-dir", type));
	EXPECT_EQ(0, ignoreItem.IsPathIgnored("some-file", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("something", type));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)ignoreFile, L"some-file\nanother/dir/*"));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, false));
	type = DT_DIR;
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("some-dir", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-file", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("another", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("another/dir", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("another/dir/some", type));
	type = DT_REG;
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("some-dir", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-file", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("another", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("another/dir", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("another/dir/some", type));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, true));
	type = DT_DIR;
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("some-dir", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-file", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("another", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("another/dir", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("another/dir/some", type));
	type = DT_REG;
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("some-dir", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-file", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("another", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("another/dir", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("another/dir/some", type));

	EXPECT_TRUE(::CreateDirectory(tempDir.GetTempDir() + L"\\subdir", nullptr));
	ignoreFile = tempDir.GetTempDir() + L"\\subdir\\.gitignore";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)ignoreFile, L"/something"));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, false));
	EXPECT_STREQ("subdir/", ignoreItem.m_BaseDir);
	type = DT_DIR;
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("something", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("something/more", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("some-dir/something", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/something", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("subdir/something/more", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("subdir/some-dir/something", type));
	type = DT_REG;
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("something", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("something/more", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("some-dir/something", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/something", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("subdir/something/more", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("subdir/some-dir/something", type));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, true));
	EXPECT_STREQ("", ignoreItem.m_BaseDir);
	type = DT_DIR;
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("something", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("something/more", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("some-dir/something", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("subdir/something", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("subdir/something/more", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("subdir/some-dir/something", type));
	type = DT_REG;
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("something", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("something/more", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("some-dir/something", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("subdir/something", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("subdir/something/more", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("subdir/some-dir/something", type));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)ignoreFile, L"something"));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, false));
	EXPECT_STREQ("subdir/", ignoreItem.m_BaseDir);
	type = DT_DIR;
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("something", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("something/more", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-dir/something", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/something", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("subdir/something/more", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/some-dir/something", type));
	type = DT_REG;
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("something", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("something/more", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-dir/something", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/something", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("subdir/something/more", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/some-dir/something", type));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, true));
	EXPECT_STREQ("", ignoreItem.m_BaseDir);
	type = DT_DIR;
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("something", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("something/more", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-dir/something", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/something", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("subdir/something/more", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/some-dir/something", type));
	type = DT_REG;
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("something", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("something/more", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-dir/something", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/something", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("subdir/something/more", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/some-dir/something", type));
}
