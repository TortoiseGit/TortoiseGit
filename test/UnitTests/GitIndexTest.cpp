// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2015-2019 - TortoiseGit

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
#include "PathUtils.h"

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

class CBasicGitWithMultiLinkedTestWithSubmoduleRepoFixture : public CBasicGitWithTestRepoCreatorFixture
{
protected:
	virtual void SetUp()
	{
		CBasicGitWithTestRepoCreatorFixture::SetUp();

		// ====Main Work Tree Setup====
		SetUpTestRepo(m_MainWorkTreePath);

		m_Git.m_CurrentDir = m_MainWorkTreePath;
		EXPECT_NE(0, SetCurrentDirectory(m_MainWorkTreePath));

		CString output;
		EXPECT_EQ(0, m_Git.Run(L"git.exe checkout -f master", &output, nullptr, CP_UTF8));
		EXPECT_STRNE(L"", output);

		if (CGit::ms_bCygwinGit)
			return;

		// ====Source of the Sub-Module====
		// Setup the repository in which the submodule will be fetched from
		SetUpTestRepo(m_SubmoduleSource);

		//====Sub-Module Inside of The Main Work Tree (Root Level)====
		output.Empty();
		EXPECT_EQ(0, m_Git.Run(L"git.exe submodule add \"" + m_SubmoduleSource + "\" sub1", &output, nullptr, CP_UTF8));
		EXPECT_STREQ(L"", output);

		output.Empty();
		EXPECT_EQ(0, m_Git.Run(L"git.exe commit -a -m\"Add submodule for testing\"", &output, nullptr, CP_UTF8));
		EXPECT_STRNE(L"", output);

		// ====Linked Work Tree setup (Absolute Path)====
		// Linked worktree using git worktree with an absolute path
		output.Empty();
		EXPECT_EQ(0, m_Git.Run(L"git.exe worktree add -b TestBranch \"" + m_LinkedWorkTreePath + "\"", &output, nullptr, CP_UTF8));
		EXPECT_STRNE(L"", output);
	}

	CString m_MainWorkTreePath = m_Dir.GetTempDir() + L"\\MainWorkTree";
	CString m_LinkedWorkTreePath = m_Dir.GetTempDir() + L"\\LinkedWorkTree";
	CString m_SubmoduleSource = m_Dir.GetTempDir() + L"\\SubmoduleSource";
};

INSTANTIATE_TEST_CASE_P(GitIndex, GitIndexCBasicGitFixture, testing::Values(LIBGIT2));
INSTANTIATE_TEST_CASE_P(GitIndex, GitIndexCBasicGitWithEmptyRepositoryFixture, testing::Values(LIBGIT2));
INSTANTIATE_TEST_CASE_P(GitIndex, GitIndexCBasicGitWithTestRepoFixture, testing::Values(LIBGIT2));
INSTANTIATE_TEST_CASE_P(GitIndex, CBasicGitWithMultiLinkedTestWithSubmoduleRepoFixture, testing::Values(LIBGIT2));

TEST_P(GitIndexCBasicGitFixture, EmptyDir)
{
	CGitIndexList indexList;
	EXPECT_EQ(-1, indexList.ReadIndex(m_Dir.GetTempDir()));
	EXPECT_EQ(0U, indexList.size());
	EXPECT_FALSE(indexList.m_bHasConflicts);
}


static void ReadAndCheckIndex(CGitIndexList& indexList, const CString& gitdir, unsigned int offset = 0)
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
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(testFile, L"this is testing file."));
	CString output;
	EXPECT_EQ(0, m_Git.Run(L"git.exe add 1.txt", &output, CP_UTF8));
	EXPECT_STREQ(L"", output);

	ReadAndCheckIndex(indexList, m_Dir.GetTempDir(), 1);

	EXPECT_STREQ(L"1.txt", indexList[0].m_FileName);
	EXPECT_EQ(21, indexList[0].m_Size);
	EXPECT_EQ(5, indexList[0].m_Flags);
	EXPECT_EQ(0, indexList[0].m_FlagsExtended);
	EXPECT_STREQ(L"e4aac1275dfc440ec521a76e9458476fe07038bb", indexList[0].m_IndexHash.ToString());

	EXPECT_EQ(0, m_Git.Run(L"git.exe rm -f 1.txt", &output, CP_UTF8));

	ReadAndCheckIndex(indexList, m_Dir.GetTempDir());

	output.Empty();
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(testFile, L"this is testing file."));
	EXPECT_EQ(0, m_Git.Run(L"git.exe add -N 1.txt", &output, CP_UTF8));
	EXPECT_STREQ(L"", output);

	ReadAndCheckIndex(indexList, m_Dir.GetTempDir(), 1);

	EXPECT_STREQ(L"1.txt", indexList[0].m_FileName);
	EXPECT_EQ(0, indexList[0].m_Size);
	EXPECT_EQ(16389, indexList[0].m_Flags);
	EXPECT_EQ(GIT_INDEX_ENTRY_INTENT_TO_ADD, indexList[0].m_FlagsExtended);
	EXPECT_STREQ(L"e69de29bb2d1d6434b8b29ae775ad8c2e48c5391", indexList[0].m_IndexHash.ToString());
}

TEST_P(GitIndexCBasicGitWithTestRepoFixture, GetFileStatus)
{
	CGitIndexList indexList;
	ReadAndCheckIndex(indexList, m_Dir.GetTempDir());

	git_wc_status2_t status = { git_wc_status_none, false, false };
	EXPECT_EQ(0, indexList.GetFileStatus(m_Dir.GetTempDir(), L"does-not-exist.txt", status, 10, 20, false));
	EXPECT_EQ(git_wc_status_unversioned, status.status);

	__int64 time = -1;
	__int64 filesize = -1;
	status = { git_wc_status_none, false, false };
	EXPECT_EQ(-1, CGit::GetFileModifyTime(CombinePath(m_Dir.GetTempDir(), L"ansi.txt"), &time, nullptr, &filesize));
	EXPECT_EQ(0, indexList.GetFileStatus(m_Dir.GetTempDir(), L"ansi.txt", status, time, filesize, false));
	EXPECT_EQ(git_wc_status_deleted, status.status);
	filesize = 42; // some arbitrary size, i.e., file exists but is changed
	EXPECT_EQ(0, indexList.GetFileStatus(m_Dir.GetTempDir(), L"ansi.txt", status, time, filesize, false));
	EXPECT_EQ(git_wc_status_modified, status.status);

	CString output;
	EXPECT_EQ(0, m_Git.Run(L"git.exe reset --hard", &output, CP_UTF8));

	EXPECT_EQ(0, CGit::GetFileModifyTime(CombinePath(m_Dir.GetTempDir(), L"ansi.txt"), &time, nullptr, &filesize));
	status = { git_wc_status_none, false, false };
	EXPECT_EQ(0, indexList.GetFileStatus(m_Dir.GetTempDir(), L"ansi.txt", status, time, filesize, false));
	EXPECT_EQ(git_wc_status_normal, status.status);
	EXPECT_FALSE(status.assumeValid);
	EXPECT_FALSE(status.skipWorktree);

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(CombinePath(m_Dir.GetTempDir(), L"ansi.txt"), L"this is testing file."));
	EXPECT_EQ(0, CGit::GetFileModifyTime(CombinePath(m_Dir.GetTempDir(), L"ansi.txt"), &time, nullptr, &filesize));
	status = { git_wc_status_none, false, false };
	EXPECT_EQ(0, indexList.GetFileStatus(m_Dir.GetTempDir(), L"ansi.txt", status, time, filesize, false));
	EXPECT_EQ(git_wc_status_modified, status.status);
	EXPECT_FALSE(status.assumeValid);
	EXPECT_FALSE(status.skipWorktree);

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(CombinePath(m_Dir.GetTempDir(), L"just-added.txt"), L"this is testing file."));
	EXPECT_EQ(0, m_Git.Run(L"git.exe add -- just-added.txt", &output, CP_UTF8));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(CombinePath(m_Dir.GetTempDir(), L"noted-as-added.txt"), L"this is testing file."));
	EXPECT_EQ(0, m_Git.Run(L"git.exe add -N -- noted-as-added.txt", &output, CP_UTF8));

	EXPECT_EQ(0, m_Git.Run(L"git.exe update-index --skip-worktree -- ansi.txt", &output, CP_UTF8));
	EXPECT_EQ(0, indexList.ReadIndex(m_Dir.GetTempDir()));
	EXPECT_FALSE(indexList.m_bHasConflicts);
	status = { git_wc_status_none, false, false };
	EXPECT_EQ(0, indexList.GetFileStatus(m_Dir.GetTempDir(), L"ansi.txt", status, time, filesize, false));
	EXPECT_EQ(git_wc_status_normal, status.status);
	EXPECT_FALSE(status.assumeValid);
	EXPECT_TRUE(status.skipWorktree);

	EXPECT_EQ(0, CGit::GetFileModifyTime(CombinePath(m_Dir.GetTempDir(), L"just-added.txt"), &time, nullptr, &filesize));
	status = { git_wc_status_none, false, false };
	EXPECT_EQ(0, indexList.GetFileStatus(m_Dir.GetTempDir(), L"just-added.txt", status, time, filesize, false));
	EXPECT_EQ(git_wc_status_normal, status.status);

	EXPECT_EQ(0, CGit::GetFileModifyTime(CombinePath(m_Dir.GetTempDir(), L"noted-as-added.txt"), &time, nullptr, &filesize));
	status = { git_wc_status_none, false, false };
	EXPECT_EQ(0, indexList.GetFileStatus(m_Dir.GetTempDir(), L"noted-as-added.txt", status, time, filesize, false));
	EXPECT_EQ(git_wc_status_added, status.status);
	EXPECT_FALSE(status.assumeValid);
	EXPECT_FALSE(status.skipWorktree);

	EXPECT_EQ(0, m_Git.Run(L"git.exe update-index --no-skip-worktree ansi.txt", &output, CP_UTF8));

	Sleep(1000);
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(CombinePath(m_Dir.GetTempDir(), L"just-added.txt"), L"this IS testing file."));
	EXPECT_EQ(0, CGit::GetFileModifyTime(CombinePath(m_Dir.GetTempDir(), L"just-added.txt"), &time, nullptr, &filesize));
	status = { git_wc_status_none, false, false };
	EXPECT_EQ(0, indexList.GetFileStatus(m_Dir.GetTempDir(), L"just-added.txt", status, time, filesize, false));
	EXPECT_EQ(git_wc_status_modified, status.status);

	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe checkout --force forconflict", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);

	output.Empty();
	EXPECT_EQ(1, m_Git.Run(L"git.exe merge simple-conflict", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);

	EXPECT_EQ(0, indexList.ReadIndex(m_Dir.GetTempDir()));
	EXPECT_EQ(9U, indexList.size());
	EXPECT_TRUE(indexList.m_bHasConflicts);

	EXPECT_EQ(0, CGit::GetFileModifyTime(CombinePath(m_Dir.GetTempDir(), L"ansi.txt"), &time, nullptr, &filesize));
	status = { git_wc_status_none, false, false };
	EXPECT_EQ(0, indexList.GetFileStatus(m_Dir.GetTempDir(), L"ansi.txt", status, time, filesize, false));
	EXPECT_EQ(git_wc_status_conflicted, status.status);
}

TEST(GitIndex, SearchInSortVector)
{
	std::vector<CGitFileName> vector;
	EXPECT_EQ(NPOS, SearchInSortVector(vector, L"something", 9, false));
	EXPECT_EQ(NPOS, SearchInSortVector(vector, L"something", 0, false));
	EXPECT_EQ(NPOS, SearchInSortVector(vector, L"something", -1, false));

	vector.push_back(CGitFileName(L"One", 0, 0));
	EXPECT_EQ(NPOS, SearchInSortVector(vector, L"something", 9, false));
	EXPECT_EQ(NPOS, SearchInSortVector(vector, L"something", -1, false));
	EXPECT_EQ(0U, SearchInSortVector(vector, L"something", 0, false)); // do we really need this behavior?
	EXPECT_EQ(0U, SearchInSortVector(vector, L"One", 3, false));
	EXPECT_EQ(0U, SearchInSortVector(vector, L"One", -1, false));
	EXPECT_EQ(NPOS, SearchInSortVector(vector, L"One/", 4, false));
	EXPECT_EQ(NPOS, SearchInSortVector(vector, L"one", 3, false));
	EXPECT_EQ(NPOS, SearchInSortVector(vector, L"one", -1, false));
	EXPECT_EQ(NPOS, SearchInSortVector(vector, L"one/", 4, false));

	vector.push_back(CGitFileName(L"tWo", 0, 0));
	DoSortFilenametSortVector(vector, false);
	EXPECT_EQ(NPOS, SearchInSortVector(vector, L"something", 9, false));
	EXPECT_EQ(0U, SearchInSortVector(vector, L"One", 3, false));
	EXPECT_EQ(NPOS, SearchInSortVector(vector, L"one", 3, false));
	EXPECT_EQ(1U, SearchInSortVector(vector, L"tWo", 3, false));
	EXPECT_EQ(NPOS, SearchInSortVector(vector, L"two", 3, false));
	EXPECT_EQ(1U, SearchInSortVector(vector, L"t", 1, false));
	EXPECT_EQ(NPOS, SearchInSortVector(vector, L"0", 1, false));
	EXPECT_EQ(NPOS, SearchInSortVector(vector, L"z", 1, false));

	vector.push_back(CGitFileName(L"b/1", 0, 0));
	vector.push_back(CGitFileName(L"b/2", 0, 0));
	vector.push_back(CGitFileName(L"a", 0, 0));
	vector.push_back(CGitFileName(L"b/3", 0, 0));
	vector.push_back(CGitFileName(L"b/4", 0, 0));
	vector.push_back(CGitFileName(L"b/5", 0, 0));
	DoSortFilenametSortVector(vector, false);
	EXPECT_EQ(0U, SearchInSortVector(vector, L"One", 3, false));
	EXPECT_EQ(3U, SearchInSortVector(vector, L"b/2", 3, false));
	EXPECT_EQ(3U, SearchInSortVector(vector, L"b/2", -1, false));
	EXPECT_LT(2U, SearchInSortVector(vector, L"b/", 2, false));
	EXPECT_GE(6U, SearchInSortVector(vector, L"b/", 2, false));
	EXPECT_EQ(NPOS, SearchInSortVector(vector, L"b/6", 3, false));
	EXPECT_EQ(1U, SearchInSortVector(vector, L"a", 1, false));
	EXPECT_EQ(7U, SearchInSortVector(vector, L"tWo", 3, false));
}

TEST(GitIndex, SearchInSortVector_IgnoreCase)
{
	std::vector<CGitFileName> vector;
	EXPECT_EQ(NPOS, SearchInSortVector(vector, L"something", 9, true));
	EXPECT_EQ(NPOS, SearchInSortVector(vector, L"something", 0, true));
	EXPECT_EQ(NPOS, SearchInSortVector(vector, L"something", -1, true));

	vector.push_back(CGitFileName(L"One", 0, 0));
	EXPECT_EQ(NPOS, SearchInSortVector(vector, L"something", 9, true));
	EXPECT_EQ(NPOS, SearchInSortVector(vector, L"something", -1, true));
	EXPECT_EQ(0U, SearchInSortVector(vector, L"something", 0, true)); // do we really need this behavior?
	EXPECT_EQ(0U, SearchInSortVector(vector, L"One", 3, true));
	EXPECT_EQ(0U, SearchInSortVector(vector, L"One", -1, true));
	EXPECT_EQ(NPOS, SearchInSortVector(vector, L"One/", 4, true));
	EXPECT_EQ(0U, SearchInSortVector(vector, L"one", 3, true));
	EXPECT_EQ(0U, SearchInSortVector(vector, L"one", -1, true));
	EXPECT_EQ(NPOS, SearchInSortVector(vector, L"one/", 4, true));

	vector.push_back(CGitFileName(L"tWo", 0, 0));
	DoSortFilenametSortVector(vector, true);
	EXPECT_EQ(NPOS, SearchInSortVector(vector, L"something", 9, true));
	EXPECT_EQ(0U, SearchInSortVector(vector, L"One", 3, true));
	EXPECT_EQ(0U, SearchInSortVector(vector, L"one", 3, true));
	EXPECT_EQ(1U, SearchInSortVector(vector, L"tWo", 3, true));
	EXPECT_EQ(1U, SearchInSortVector(vector, L"two", 3, true));
	EXPECT_EQ(1U, SearchInSortVector(vector, L"t", 1, true));
	EXPECT_EQ(NPOS, SearchInSortVector(vector, L"0", 1, true));
	EXPECT_EQ(NPOS, SearchInSortVector(vector, L"z", 1, true));

	vector.push_back(CGitFileName(L"b/1", 0, 0));
	vector.push_back(CGitFileName(L"b/2", 0, 0));
	vector.push_back(CGitFileName(L"a", 0, 0));
	vector.push_back(CGitFileName(L"b/3", 0, 0));
	vector.push_back(CGitFileName(L"b/4", 0, 0));
	vector.push_back(CGitFileName(L"b/5", 0, 0));
	DoSortFilenametSortVector(vector, true);
	EXPECT_EQ(0U, SearchInSortVector(vector, L"a", 1, true));
	EXPECT_EQ(2U, SearchInSortVector(vector, L"b/2", 3, true));
	EXPECT_EQ(2U, SearchInSortVector(vector, L"b/2", -1, true));
	EXPECT_LT(1U, SearchInSortVector(vector, L"b/", 2, true));
	EXPECT_GE(5U, SearchInSortVector(vector, L"b/", 2, true));
	EXPECT_EQ(NPOS, SearchInSortVector(vector, L"b/6", 3, true));
	EXPECT_EQ(6U, SearchInSortVector(vector, L"One", 3, true));
	EXPECT_EQ(7U, SearchInSortVector(vector, L"tWo", 3, true));
}

static void CheckRangeInSortVector(bool ignoreCase)
{
	std::vector<CGitFileName> vector;

	size_t start = NPOS;
	size_t end = NPOS;
	EXPECT_EQ(-1, GetRangeInSortVector(vector, L"something", 9, ignoreCase, &start, &end, NPOS));
	EXPECT_EQ(-1, GetRangeInSortVector(vector, L"something", 9, ignoreCase, &start, &end, 0));
	EXPECT_EQ(-1, GetRangeInSortVector(vector, L"something", 9, ignoreCase, &start, nullptr, 0));
	EXPECT_EQ(-1, GetRangeInSortVector(vector, L"something", 9, ignoreCase, nullptr, &end, 0));
	EXPECT_EQ(-1, GetRangeInSortVector(vector, L"something", 9, ignoreCase, nullptr, &end, 1));
	EXPECT_EQ(-1, GetRangeInSortVector(vector, L"", 0, ignoreCase, &start, &end, 0));
	EXPECT_EQ(NPOS, start);
	EXPECT_EQ(NPOS, end);

	vector.push_back(CGitFileName(L"a", 0, 0));
	start = end = NPOS;
	EXPECT_EQ(-1, GetRangeInSortVector(vector, L"something", 9, ignoreCase, &start, &end, NPOS));
	EXPECT_EQ(-1, GetRangeInSortVector(vector, L"something", 9, ignoreCase, &start, nullptr, 0));
	EXPECT_EQ(-1, GetRangeInSortVector(vector, L"something", 9, ignoreCase, nullptr, &end, 0));
	EXPECT_EQ(-1, GetRangeInSortVector(vector, L"something", 9, ignoreCase, nullptr, &end, 1));

	EXPECT_EQ(-1, GetRangeInSortVector(vector, L"a", 1, ignoreCase, &start, &end, NPOS));
	EXPECT_EQ(-1, GetRangeInSortVector(vector, L"a", 1, ignoreCase, &start, nullptr, 0));
	EXPECT_EQ(-1, GetRangeInSortVector(vector, L"a", 1, ignoreCase, nullptr, &end, 0));
	EXPECT_EQ(-1, GetRangeInSortVector(vector, L"a", 1, ignoreCase, nullptr, &end, 1));
	start = end = NPOS;
	EXPECT_EQ(0, GetRangeInSortVector(vector, L"", 0, ignoreCase, &start, &end, 0));
	EXPECT_EQ(0U, start);
	EXPECT_EQ(0U, end);
	start = end = NPOS;
	EXPECT_EQ(-1, GetRangeInSortVector(vector, L"", 0, ignoreCase, &start, &end, 1));

	start = end = NPOS;
	EXPECT_EQ(-1, GetRangeInSortVector(vector, L"0", 1, ignoreCase, &start, &end, 0));
	EXPECT_EQ(NPOS, start);
	EXPECT_EQ(NPOS, end);
	start = end = NPOS;
	EXPECT_EQ(-1, GetRangeInSortVector(vector, L"b", 1, ignoreCase, &start, &end, 0));
	EXPECT_EQ(NPOS, start);
	EXPECT_EQ(NPOS, end);

	start = end = NPOS;
	EXPECT_EQ(0, GetRangeInSortVector(vector, L"a", 1, ignoreCase, &start, &end, 0));
	EXPECT_EQ(0U, start);
	EXPECT_EQ(0U, end);

	vector.push_back(CGitFileName(L"b/1", 0, 0));
	vector.push_back(CGitFileName(L"b/2", 0, 0));
	vector.push_back(CGitFileName(L"b/3", 0, 0));
	vector.push_back(CGitFileName(L"b/4", 0, 0));
	vector.push_back(CGitFileName(L"b/5", 0, 0));

	start = end = NPOS;
	EXPECT_EQ(0, GetRangeInSortVector(vector, L"a", 1, ignoreCase, &start, &end, 0));
	EXPECT_EQ(0U, start);
	EXPECT_EQ(0U, end);

	start = end = NPOS;
	EXPECT_EQ(0, GetRangeInSortVector(vector, L"b/", 2, ignoreCase, &start, &end, 1));
	EXPECT_EQ(1U, start);
	EXPECT_EQ(5U, end);

	start = end = NPOS;
	EXPECT_EQ(0, GetRangeInSortVector(vector, L"b/", 2, ignoreCase, &start, &end, 2));
	EXPECT_EQ(1U, start);
	EXPECT_EQ(5U, end);

	start = end = NPOS;
	EXPECT_EQ(0, GetRangeInSortVector(vector, L"b/", 2, ignoreCase, &start, &end, 4));
	EXPECT_EQ(1U, start);
	EXPECT_EQ(5U, end);

	start = end = NPOS;
	EXPECT_EQ(0, GetRangeInSortVector(vector, L"b/", 2, ignoreCase, &start, &end, 5));
	EXPECT_EQ(1U, start);
	EXPECT_EQ(5U, end);

	EXPECT_EQ(-1, GetRangeInSortVector(vector, L"b/", 2, ignoreCase, &start, &end, 6)); // 6 is >= vector.size()

	start = end = NPOS;
	EXPECT_EQ(-1, GetRangeInSortVector(vector, L"c/", 2, ignoreCase, &start, &end, 0));
	EXPECT_EQ(NPOS, start);
	EXPECT_EQ(NPOS, end);

	start = end = NPOS;
	EXPECT_EQ(-1, GetRangeInSortVector(vector, L"c/", 2, ignoreCase, &start, &end, 5));
	EXPECT_EQ(NPOS, start);
	EXPECT_EQ(NPOS, end);

	vector.push_back(CGitFileName(L"c", 0, 0));
	vector.push_back(CGitFileName(L"d", 0, 0));

	start = end = NPOS;
	EXPECT_EQ(0, GetRangeInSortVector(vector, L"b/", 2, ignoreCase, &start, &end, 1));
	EXPECT_EQ(1U, start);
	EXPECT_EQ(5U, end);

	start = end = NPOS;
	EXPECT_EQ(0, GetRangeInSortVector(vector, L"b/", 2, ignoreCase, &start, &end, 2));
	EXPECT_EQ(1U, start);
	EXPECT_EQ(5U, end);

	start = end = NPOS;
	EXPECT_EQ(0, GetRangeInSortVector(vector, L"b/", 2, ignoreCase, &start, &end, 4));
	EXPECT_EQ(1U, start);
	EXPECT_EQ(5U, end);

	start = end = NPOS;
	EXPECT_EQ(0, GetRangeInSortVector(vector, L"b/", 2, ignoreCase, &start, &end, 5));
	EXPECT_EQ(1U, start);
	EXPECT_EQ(5U, end);

	start = end = NPOS;
	EXPECT_EQ(-1, GetRangeInSortVector(vector, L"c/", 2, ignoreCase, &start, &end, 6));
	EXPECT_EQ(NPOS, start);
	EXPECT_EQ(NPOS, end);

	start = end = NPOS;
	EXPECT_EQ(0, GetRangeInSortVector(vector, L"c", 1, ignoreCase, &start, &end, 6));
	EXPECT_EQ(6U, start);
	EXPECT_EQ(6U, end);

	start = end = NPOS;
	EXPECT_EQ(0, GetRangeInSortVector(vector, L"", 0, ignoreCase, &start, &end, 0));
	EXPECT_EQ(0U, start);
	EXPECT_EQ(7U, end);
}

TEST(GitIndex, GetRangeInSortVector)
{
	CheckRangeInSortVector(false);
	CheckRangeInSortVector(true);
}

TEST(GitIndex, CGitIgnoreItem)
{
	CAutoTempDir tempDir;
	CGitIgnoreItem ignoreItem;

	int ignoreCase = 0;
	int type = DT_DIR;
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("not-ignored", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("subdir/not-ignored", type));
	type = DT_REG;
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("not-ignored", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("subdir/not-ignored", type));

	EXPECT_EQ(-1, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), L"does-not-exist", false, &ignoreCase));
	EXPECT_EQ(-1, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), L"does-not-exist", true, &ignoreCase));

	CString ignoreFile = tempDir.GetTempDir() + L"\\.gitignore";

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(ignoreFile, L""));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, false, &ignoreCase));
	EXPECT_STREQ("", ignoreItem.m_BaseDir);
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, true, &ignoreCase));
	EXPECT_STREQ("", ignoreItem.m_BaseDir);

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(ignoreFile, L"#"));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, false, &ignoreCase));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, true, &ignoreCase));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(ignoreFile, L"# comment"));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, false, &ignoreCase));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, true, &ignoreCase));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(ignoreFile, L"\n"));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, false, &ignoreCase));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, true, &ignoreCase));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(ignoreFile, L"\n#"));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, false, &ignoreCase));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, true, &ignoreCase));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(ignoreFile, L"*.tmp\n"));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, false, &ignoreCase));
	type = DT_DIR;
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("not-ignored", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("text.tmp", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/text.tmp", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("1.tmp.1", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("text.tMp", type));
	ignoreCase = 1;
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("not-ignored", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("text.tmp", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/text.tmp", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("1.tmp.1", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("text.tMp", type));
	type = DT_REG;
	ignoreCase = 0;
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("not-ignored", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("text.tmp", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/text.tmp", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("1.tmp.1", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("text.tMp", type));
	ignoreCase = 1;
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("not-ignored", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("text.tmp", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/text.tmp", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("1.tmp.1", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("text.tMp", type));
	ignoreCase = 0;
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, true, &ignoreCase));
	type = DT_DIR;
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("not-ignored", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("text.tmp", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/text.tmp", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("1.tmp.1", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("text.tMp", type));
	type = DT_REG;
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("not-ignored", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("text.tmp", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/text.tmp", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("1.tmp.1", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("text.tMp", type));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(ignoreFile, L"*.tMp\n"));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, false, &ignoreCase));
	type = DT_DIR;
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("not-ignored", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("text.tMp", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/text.tMp", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("1.tMp.1", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("text.tmp", type));
	ignoreCase = 1;
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("not-ignored", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("text.tMp", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/text.tMp", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("1.tmp.1", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("text.tmp", type));
	type = DT_REG;
	ignoreCase = 0;
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("not-ignored", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("text.tMp", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/text.tMp", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("1.tMp.1", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("text.tmp", type));
	ignoreCase = 1;
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("not-ignored", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("text.tMp", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/text.tMp", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("1.tMp.1", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("text.tmp", type));
	ignoreCase = 0;
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, true, &ignoreCase));
	type = DT_DIR;
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("not-ignored", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("text.tMp", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/text.tMp", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("1.tMp.1", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("text.tmp", type));
	type = DT_REG;
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("not-ignored", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("text.tMp", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/text.tMp", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("1.tMp.1", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("text.tmp", type));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(ignoreFile, L"some-file\n"));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, false, &ignoreCase));
	type = DT_DIR;
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("not-ignored", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("subdir/not-ignored", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-file", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/some-file", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subDir/some-file", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("subdir/some-File", type));
	ignoreCase = 1;
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("not-ignored", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("subdir/not-ignored", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-file", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/some-file", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subDir/some-file", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/some-File", type));
	type = DT_REG;
	ignoreCase = 0;
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("not-ignored", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("subdir/not-ignored", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-file", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/some-file", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subDir/some-file", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("subdir/some-File", type));
	ignoreCase = 1;
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("not-ignored", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("subdir/not-ignored", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-file", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/some-file", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subDir/some-file", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/some-File", type));
	ignoreCase = 0;
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, true, &ignoreCase));
	type = DT_DIR;
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("not-ignored", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("subdir/not-ignored", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-file", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/some-file", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subDir/some-file", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("subdir/some-File", type));
	ignoreCase = 1;
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("not-ignored", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("subdir/not-ignored", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-file", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/some-file", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subDir/some-file", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/some-File", type));
	ignoreCase = 0;
	type = DT_REG;
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("not-ignored", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("subdir/not-ignored", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-file", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/some-file", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subDir/some-file", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("subdir/some-File", type));
	ignoreCase = 1;
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("not-ignored", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("subdir/not-ignored", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-file", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/some-file", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subDir/some-file", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/some-File", type));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(ignoreFile, L"some-File\n"));
	ignoreCase = 0;
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, false, &ignoreCase));
	type = DT_DIR;
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("not-ignored", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("subdir/not-ignored", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-File", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/some-File", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subDir/some-File", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("subdir/some-file", type));
	ignoreCase = 1;
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("not-ignored", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("subdir/not-ignored", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-file", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/some-File", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subDir/some-File", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/some-file", type));
	type = DT_REG;
	ignoreCase = 0;
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("not-ignored", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("subdir/not-ignored", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-File", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/some-File", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subDir/some-File", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("subdir/some-file", type));
	ignoreCase = 1;
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("not-ignored", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("subdir/not-ignored", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-File", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/some-File", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subDir/some-File", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/some-file", type));

	ignoreCase = 1;
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(ignoreFile, L"\n\nsome-file\n"));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, false, &ignoreCase));
	type = DT_DIR;
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-file", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/some-file", type));
	type = DT_REG;
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-file", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/some-file", type));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, true, &ignoreCase));
	type = DT_DIR;
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-file", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/some-file", type));
	type = DT_REG;
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-file", type));
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("subdir/some-file", type));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(ignoreFile, L"/some-file"));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, false, &ignoreCase));
	type = DT_DIR;
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-file", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("subdir/some-file", type));
	type = DT_REG;
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-file", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("subdir/some-file", type));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, true, &ignoreCase));
	type = DT_DIR;
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-file", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("subdir/some-file", type));
	type = DT_REG;
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-file", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("subdir/some-file", type));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(ignoreFile, L"some-dir/"));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, false, &ignoreCase));
	type = DT_DIR;
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-dir", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("some-dir/some-file", type));
	type = DT_REG;
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("some-file", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("some-dir", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("some-dir/some-file", type));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, true, &ignoreCase));
	type = DT_DIR;
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-dir", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("some-dir/some-file", type));
	type = DT_REG;
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("some-file", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("some-dir", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("some-dir/some-file", type));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(ignoreFile, L"some-*\n!some-file"));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, false, &ignoreCase));
	type = DT_DIR;
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-dir", type));
	EXPECT_EQ(0, ignoreItem.IsPathIgnored("some-file", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("something", type));
	type = DT_REG;
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-dir", type));
	EXPECT_EQ(0, ignoreItem.IsPathIgnored("some-file", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("something", type));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, true, &ignoreCase));
	type = DT_DIR;
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-dir", type));
	EXPECT_EQ(0, ignoreItem.IsPathIgnored("some-file", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("something", type));
	type = DT_REG;
	EXPECT_EQ(1, ignoreItem.IsPathIgnored("some-dir", type));
	EXPECT_EQ(0, ignoreItem.IsPathIgnored("some-file", type));
	EXPECT_EQ(-1, ignoreItem.IsPathIgnored("something", type));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(ignoreFile, L"some-file\nanother/dir/*"));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, false, &ignoreCase));
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
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, true, &ignoreCase));
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
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(ignoreFile, L"/something"));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, false, &ignoreCase));
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
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, true, &ignoreCase));
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

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(ignoreFile, L"something"));
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, false, &ignoreCase));
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
	EXPECT_EQ(0, ignoreItem.FetchIgnoreList(tempDir.GetTempDir(), ignoreFile, true, &ignoreCase));
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

TEST_P(CBasicGitWithMultiLinkedTestWithSubmoduleRepoFixture, AdminDirMap) // Submodule & Test
{
	CString adminDir;
	CString workDir;

	// Test if the main work tree admin directory can be found
	adminDir = g_AdminDirMap.GetAdminDir(m_MainWorkTreePath);
	EXPECT_TRUE(CPathUtils::IsSamePath(CPathUtils::BuildPathWithPathDelimiter(m_MainWorkTreePath) + L".git", adminDir));

	// Test test main work tree reverse lookup
	workDir = g_AdminDirMap.GetWorkingCopy(CPathUtils::BuildPathWithPathDelimiter(m_MainWorkTreePath) + L".git");
	EXPECT_TRUE(CPathUtils::IsSamePath(m_MainWorkTreePath, workDir));

	if (CGit::ms_bCygwinGit)
		return;

	// Test if the linked repository admin directory can be found (**WITHOUT** trailing path delimiter)
	adminDir = g_AdminDirMap.GetAdminDir(m_LinkedWorkTreePath);
	EXPECT_TRUE(CPathUtils::IsSamePath(CPathUtils::BuildPathWithPathDelimiter(m_MainWorkTreePath) + L".git", adminDir));

	// Test if the linked worktree admin directory can be found (**WITH** trailing path delimiter)
	adminDir = g_AdminDirMap.GetWorktreeAdminDir(CPathUtils::BuildPathWithPathDelimiter(m_LinkedWorkTreePath));
	EXPECT_TRUE(CPathUtils::IsSamePath(CPathUtils::BuildPathWithPathDelimiter(m_MainWorkTreePath) + L".git\\worktrees\\LinkedWorkTree", adminDir));

	// Test if the linked worktree admin directory can be found (**WITHOUT** trailing path delimiter)
	adminDir = g_AdminDirMap.GetWorktreeAdminDir(m_LinkedWorkTreePath);
	EXPECT_TRUE(CPathUtils::IsSamePath(CPathUtils::BuildPathWithPathDelimiter(m_MainWorkTreePath) + L".git\\worktrees\\LinkedWorkTree", adminDir));

	// Test reverse lookup on linked worktree admin directory (**WITH** trailing path delimiter)
	workDir = g_AdminDirMap.GetWorkingCopy(CPathUtils::BuildPathWithPathDelimiter(m_MainWorkTreePath) + L".git\\worktrees\\LinkedWorkTree\\");
	EXPECT_TRUE(CPathUtils::IsSamePath(m_LinkedWorkTreePath, workDir));

	// Test reverse lookup on linked worktree admin directory (**WITHOUT** trailing path delimiter)
	workDir = g_AdminDirMap.GetWorkingCopy(CPathUtils::BuildPathWithPathDelimiter(m_MainWorkTreePath) + L".git\\worktrees\\LinkedWorkTree");
	EXPECT_TRUE(CPathUtils::IsSamePath(m_LinkedWorkTreePath, workDir));

	// Test if the sub-module admin directory can be found (**WITH** trailing path delimiter)
	adminDir = g_AdminDirMap.GetAdminDir(CPathUtils::BuildPathWithPathDelimiter(m_MainWorkTreePath) + L"sub1\\");
	EXPECT_TRUE(CPathUtils::IsSamePath(CPathUtils::BuildPathWithPathDelimiter(m_MainWorkTreePath) + L".git\\modules\\sub1", adminDir));

	// Test if the sub-module admin directory can be found (**WITHOUT** trailing path delimiter)
	adminDir = g_AdminDirMap.GetAdminDir(CPathUtils::BuildPathWithPathDelimiter(m_MainWorkTreePath) + L"sub1");
	EXPECT_TRUE(CPathUtils::IsSamePath(CPathUtils::BuildPathWithPathDelimiter(m_MainWorkTreePath) + L".git\\modules\\sub1", adminDir));

	// Test if reverse lookup on submodule works (**WITH** trailing path delimiter)
	workDir = g_AdminDirMap.GetWorkingCopy(CPathUtils::BuildPathWithPathDelimiter(m_MainWorkTreePath) + L".git\\modules\\sub1\\");
	EXPECT_TRUE(CPathUtils::IsSamePath(CPathUtils::BuildPathWithPathDelimiter(m_MainWorkTreePath) + L"sub1", workDir));

	// Test if reverse lookup on submodule works (**WITHOUT** trailing path delimiter)
	workDir = g_AdminDirMap.GetWorkingCopy(CPathUtils::BuildPathWithPathDelimiter(m_MainWorkTreePath) + L".git\\modules\\sub1");
	EXPECT_TRUE(CPathUtils::IsSamePath(CPathUtils::BuildPathWithPathDelimiter(m_MainWorkTreePath) + L"sub1", workDir));
}
