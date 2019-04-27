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
#include "GitRevLoglist.h"
#include "TGitPath.h"

class GitRevLoglistCBasicGitWithTestRepoFixture : public CBasicGitWithTestRepoFixture
{
};

class GitRevLoglistCBasicGitWithTestRepoBareFixture : public CBasicGitWithTestRepoBareFixture
{
};

class GitRevLoglistCBasicGitWithSubmoduleRepoFixture : public CBasicGitWithSubmoduleRepositoryFixture
{
};

class GitRevLoglistCBasicGitWithSubmoduleRepoBareFixture : public CBasicGitWithSubmodulRepoeBareFixture
{
};

class GitRevLoglist2CBasicGitWithTestRepoFixture : public CBasicGitWithTestRepoFixture
{
};

class GitRevLoglist2CBasicGitWithTestRepoBareFixture : public CBasicGitWithTestRepoBareFixture
{
};

INSTANTIATE_TEST_CASE_P(GitRevLoglist, GitRevLoglistCBasicGitWithTestRepoFixture, testing::Values(LIBGIT, LIBGIT2, LIBGIT2_ALL));
INSTANTIATE_TEST_CASE_P(GitRevLoglist, GitRevLoglistCBasicGitWithTestRepoBareFixture, testing::Values(LIBGIT, LIBGIT2, LIBGIT2_ALL));
INSTANTIATE_TEST_CASE_P(GitRevLoglist, GitRevLoglistCBasicGitWithSubmoduleRepoFixture, testing::Values(LIBGIT, LIBGIT2, LIBGIT2_ALL));
INSTANTIATE_TEST_CASE_P(GitRevLoglist, GitRevLoglistCBasicGitWithSubmoduleRepoBareFixture, testing::Values(LIBGIT, LIBGIT2, LIBGIT2_ALL));

INSTANTIATE_TEST_CASE_P(GitRevLoglist, GitRevLoglist2CBasicGitWithTestRepoFixture, testing::Values(GIT_CLI, LIBGIT, LIBGIT2, LIBGIT2_ALL));
INSTANTIATE_TEST_CASE_P(GitRevLoglist, GitRevLoglist2CBasicGitWithTestRepoBareFixture, testing::Values(GIT_CLI, LIBGIT, LIBGIT2, LIBGIT2_ALL));

static void SafeFetchFullInfo(CGit* cGit)
{
	GitRevLoglist rev;
	EXPECT_EQ(FALSE, rev.m_IsSimpleListReady);
	EXPECT_EQ(1, rev.CheckAndDiff());
	EXPECT_STREQ(L"", rev.GetAuthorName());
	EXPECT_EQ(FALSE, rev.m_IsSimpleListReady);
	EXPECT_EQ(FALSE, rev.m_IsFull);
	EXPECT_EQ(FALSE, rev.m_IsUpdateing);
	EXPECT_EQ(FALSE, rev.m_IsDiffFiles);
	EXPECT_EQ(FALSE, rev.m_IsCommitParsed);
	rev.m_IsDiffFiles = 1;
	EXPECT_EQ(0, rev.GetFiles(nullptr).GetCount());
	EXPECT_EQ(-1, rev.SafeFetchFullInfo(cGit));
	EXPECT_STREQ(L"", rev.GetAuthorName());
	EXPECT_EQ(FALSE, rev.m_IsSimpleListReady);
	EXPECT_EQ(TRUE, rev.m_IsUpdateing);
	EXPECT_EQ(FALSE, rev.m_IsFull);
	EXPECT_EQ(TRUE, rev.m_IsDiffFiles);
	EXPECT_EQ(FALSE, rev.m_IsCommitParsed);
	rev.Clear();
	rev.m_CommitHash = CGitHash::FromHexStr(L"7c3cbfe13a929d2291a574dca45e4fd2d2ac1aa6");
	rev.m_IsDiffFiles = TRUE;
	EXPECT_EQ(1, rev.CheckAndDiff());
	EXPECT_EQ(FALSE, rev.m_IsSimpleListReady);
	EXPECT_EQ(FALSE, rev.m_IsUpdateing);
	EXPECT_EQ(FALSE, rev.m_IsFull);
	EXPECT_EQ(FALSE, rev.m_IsCommitParsed);
	EXPECT_STREQ(L"", rev.GetAuthorName());
	rev.m_IsDiffFiles = FALSE;
	EXPECT_EQ(FALSE, rev.m_IsSimpleListReady);
	EXPECT_EQ(FALSE, rev.m_IsUpdateing);
	EXPECT_EQ(FALSE, rev.m_IsFull);
	EXPECT_EQ(FALSE, rev.m_IsDiffFiles);
	EXPECT_EQ(FALSE, rev.m_IsCommitParsed);
	EXPECT_EQ(0, rev.CheckAndDiff());
	EXPECT_STREQ(L"", rev.GetAuthorName());
	EXPECT_EQ(FALSE, rev.m_IsSimpleListReady);
	EXPECT_EQ(FALSE, rev.m_IsUpdateing);
	EXPECT_EQ(TRUE, rev.m_IsFull);
	EXPECT_EQ(TRUE, rev.m_IsDiffFiles);
	EXPECT_EQ(FALSE, rev.m_IsCommitParsed);
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, rev.GetAction(nullptr));
	ASSERT_EQ(1, rev.GetFiles(nullptr).GetCount());
	CTGitPathList list;
	list = rev.GetFiles(nullptr);
	EXPECT_STREQ(L"ascii.txt", list[0].GetGitPathString());
	EXPECT_STREQ(L"", list[0].GetGitOldPathString());
	EXPECT_STREQ(L"2", list[0].m_StatAdd);
	EXPECT_STREQ(L"2", list[0].m_StatDel);
	EXPECT_EQ(0, list[0].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	rev.Clear();
	rev.m_CommitHash = CGitHash::FromHexStr(L"7c3cbfe13a929d2291a574dca45e4fd2d2ac1aa6");
	rev.m_IsUpdateing = TRUE; // do nothing
	EXPECT_EQ(0, rev.SafeFetchFullInfo(cGit));
	EXPECT_STREQ(L"", rev.GetAuthorName());
	EXPECT_EQ(FALSE, rev.m_IsSimpleListReady);
	EXPECT_EQ(TRUE, rev.m_IsUpdateing);
	EXPECT_EQ(FALSE, rev.m_IsFull);
	EXPECT_EQ(FALSE, rev.m_IsDiffFiles);
	EXPECT_EQ(FALSE, rev.m_IsCommitParsed);
	EXPECT_EQ(0U, rev.GetAction(nullptr));
	EXPECT_EQ(0, rev.GetFiles(nullptr).GetCount());
	rev.Clear();
	rev.m_CommitHash = CGitHash::FromHexStr(L"7c3cbfe13a929d2291a574dca45e4fd2d2ac1aa6");
	EXPECT_EQ(0, rev.SafeFetchFullInfo(cGit));
	EXPECT_STREQ(L"", rev.GetAuthorName());
	EXPECT_EQ(FALSE, rev.m_IsSimpleListReady);
	EXPECT_EQ(FALSE, rev.m_IsUpdateing);
	EXPECT_EQ(TRUE, rev.m_IsFull);
	EXPECT_EQ(FALSE, rev.m_IsDiffFiles);
	EXPECT_EQ(FALSE, rev.m_IsCommitParsed);
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, rev.GetAction(nullptr));
	ASSERT_EQ(1, rev.GetFiles(nullptr).GetCount());
	list = rev.GetFiles(nullptr);
	EXPECT_STREQ(L"ascii.txt", list[0].GetGitPathString());
	EXPECT_STREQ(L"", list[0].GetGitOldPathString());
	EXPECT_STREQ(L"2", list[0].m_StatAdd);
	EXPECT_STREQ(L"2", list[0].m_StatDel);
	EXPECT_EQ(0, list[0].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	rev.Clear();
	EXPECT_EQ(0, rev.GetFiles(nullptr).GetCount());
	rev.m_CommitHash = CGitHash::FromHexStr(L"dead91b4aedeaddeaddead2a56d3c473c705dead"); // non-existent commit
	EXPECT_EQ(-1, rev.SafeFetchFullInfo(cGit));
	EXPECT_EQ(FALSE, rev.m_IsSimpleListReady);
	EXPECT_EQ(TRUE, rev.m_IsUpdateing);
	EXPECT_EQ(FALSE, rev.m_IsFull);
	EXPECT_EQ(FALSE, rev.m_IsDiffFiles);
	EXPECT_EQ(FALSE, rev.m_IsCommitParsed);
	rev.Clear();
	rev.m_CommitHash = CGitHash::FromHexStr(L"35c91b4ae2f77f4f21a7aba56d3c473c705d89e6");
	EXPECT_EQ(0, rev.SafeFetchFullInfo(cGit));
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, rev.GetAction(nullptr));
	ASSERT_EQ(4, rev.GetFiles(nullptr).GetCount());
	list = rev.GetFiles(nullptr);
	EXPECT_STREQ(L"copy/ansi.txt", list[0].GetGitPathString());
	EXPECT_STREQ(L"", list[0].GetGitOldPathString());
	EXPECT_STREQ(L"1", list[0].m_StatAdd);
	EXPECT_STREQ(L"1", list[0].m_StatDel);
	EXPECT_EQ(0, list[0].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_STREQ(L"copy/utf16-be-nobom.txt", list[1].GetGitPathString());
	EXPECT_STREQ(L"", list[1].GetGitOldPathString());
	EXPECT_STREQ(L"-", list[1].m_StatAdd);
	EXPECT_STREQ(L"-", list[1].m_StatDel);
	EXPECT_EQ(0, list[1].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);
	EXPECT_STREQ(L"copy/utf8-bom.txt", list[2].GetGitPathString());
	EXPECT_STREQ(L"", list[2].GetGitOldPathString());
	EXPECT_STREQ(L"1", list[2].m_StatAdd);
	EXPECT_STREQ(L"1", list[2].m_StatDel);
	EXPECT_EQ(0, list[2].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[2].m_Action);
	EXPECT_STREQ(L"copy/utf8-nobom.txt", list[3].GetGitPathString());
	EXPECT_STREQ(L"", list[3].GetGitOldPathString());
	EXPECT_STREQ(L"1", list[3].m_StatAdd);
	EXPECT_STREQ(L"1", list[3].m_StatDel);
	EXPECT_EQ(0, list[3].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[3].m_Action);
	rev.Clear();
	rev.m_CommitHash = CGitHash::FromHexStr(L"8d1ebbcc7eeb63af10ff8bcf7712afb9fcc90b8a"); // merge commit
	EXPECT_EQ(0, rev.SafeFetchFullInfo(cGit));
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, rev.GetAction(nullptr));
	ASSERT_EQ(1, rev.GetFiles(nullptr).GetCount());
	list = rev.GetFiles(nullptr);
	EXPECT_STREQ(L"newfiles3.txt", list[0].GetGitPathString());
	EXPECT_STREQ(L"", list[0].GetGitOldPathString());
	EXPECT_STREQ(L"1", list[0].m_StatAdd);
	EXPECT_STREQ(L"0", list[0].m_StatDel);
	EXPECT_EQ(0, list[0].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	rev.Clear();
	rev.m_CommitHash = CGitHash::FromHexStr(L"18da7c332dcad0f37f9977d9176dce0b0c66f3eb"); // stash commit
	EXPECT_EQ(0, rev.SafeFetchFullInfo(cGit));
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, rev.GetAction(nullptr));
	ASSERT_EQ(2, rev.GetFiles(nullptr).GetCount());
	list = rev.GetFiles(nullptr);
	EXPECT_STREQ(L"newfiles.txt", list[0].GetGitPathString());
	EXPECT_STREQ(L"", list[0].GetGitOldPathString());
	EXPECT_STREQ(L"1", list[0].m_StatAdd);
	EXPECT_STREQ(L"1", list[0].m_StatDel);
	EXPECT_EQ(0, list[0].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	EXPECT_STREQ(L"newfiles.txt", list[1].GetGitPathString());
	EXPECT_STREQ(L"", list[1].GetGitOldPathString());
	EXPECT_STREQ(L"1", list[1].m_StatAdd);
	EXPECT_STREQ(L"1", list[1].m_StatDel);
	EXPECT_EQ(1, list[1].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);
	EXPECT_FALSE(list[1].IsDirectory());
	rev.Clear();
	rev.m_CommitHash = CGitHash::FromHexStr(L"844309789a13614b52d5e7cbfe6350dd73d1dc72"); // root commit
	EXPECT_EQ(0, rev.SafeFetchFullInfo(cGit));
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, rev.GetAction(nullptr));
	ASSERT_EQ(1, rev.GetFiles(nullptr).GetCount());
	EXPECT_STREQ(L"ansi.txt", rev.GetFiles(nullptr)[0].GetGitPathString());
	EXPECT_STREQ(L"", rev.GetFiles(nullptr)[0].GetGitOldPathString());
	rev.Clear();
	rev.m_CommitHash = CGitHash::FromHexStr(L"4c5c93d2a0b368bc4570d5ec02ab03b9c4334d44"); // signed commit with renamed file last
	EXPECT_EQ(0, rev.SafeFetchFullInfo(cGit));
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED | CTGitPath::LOGACTIONS_MODIFIED | CTGitPath::LOGACTIONS_REPLACED | CTGitPath::LOGACTIONS_DELETED, rev.GetAction(nullptr));
	ASSERT_EQ(4, rev.GetFiles(nullptr).GetCount());
	list = rev.GetFiles(nullptr);
	EXPECT_STREQ(L"newfiles2 - Cöpy.txt", list[0].GetGitPathString());
	EXPECT_STREQ(L"", list[0].GetGitOldPathString());
	EXPECT_STREQ(L"1", list[0].m_StatAdd);
	EXPECT_STREQ(L"0", list[0].m_StatDel);
	EXPECT_EQ(0, list[0].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	EXPECT_STREQ(L"utf16-be-nobom.txt", list[1].GetGitPathString()); // changed from file to symlink
	EXPECT_STREQ(L"", list[1].GetGitOldPathString());
	EXPECT_STREQ(L"-", list[1].m_StatAdd);
	EXPECT_STREQ(L"-", list[1].m_StatDel);
	EXPECT_EQ(0, list[1].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);
	EXPECT_FALSE(list[1].IsDirectory());
	EXPECT_STREQ(L"utf8-bom.txt", list[2].GetGitPathString());
	EXPECT_STREQ(L"", list[2].GetGitOldPathString());
	EXPECT_STREQ(L"0", list[2].m_StatAdd);
	EXPECT_STREQ(L"9", list[2].m_StatDel);
	EXPECT_EQ(0, list[2].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list[2].m_Action);
	EXPECT_FALSE(list[2].IsDirectory());
	EXPECT_STREQ(L"was-ansi.txt", list[3].GetGitPathString());
	EXPECT_STREQ(L"ansi.txt", list[3].GetGitOldPathString());
	EXPECT_STREQ(L"0", list[3].m_StatAdd);
	EXPECT_STREQ(L"0", list[3].m_StatDel);
	EXPECT_EQ(0, list[3].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED, list[3].m_Action);
	EXPECT_FALSE(list[3].IsDirectory());
	rev.Clear();
	rev.m_CommitHash = CGitHash::FromHexStr(L"31ff87c86e9f6d3853e438cb151043f30f09029a"); // commit with renamed file first
	EXPECT_EQ(0, rev.SafeFetchFullInfo(cGit));
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED | CTGitPath::LOGACTIONS_MODIFIED | CTGitPath::LOGACTIONS_REPLACED | CTGitPath::LOGACTIONS_DELETED, rev.GetAction(nullptr));
	ASSERT_EQ(4, rev.GetFiles(nullptr).GetCount());
	list = rev.GetFiles(nullptr);
	EXPECT_STREQ(L"it-was-ansi.txt", list[0].GetGitPathString());
	EXPECT_STREQ(L"ansi.txt", list[0].GetGitOldPathString());
	EXPECT_STREQ(L"0", list[0].m_StatAdd);
	EXPECT_STREQ(L"0", list[0].m_StatDel);
	EXPECT_EQ(0, list[0].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED, list[0].m_Action);
	EXPECT_FALSE(list[1].IsDirectory());
	EXPECT_STREQ(L"newfiles2 - Cöpy.txt", list[1].GetGitPathString());
	EXPECT_STREQ(L"", list[1].GetGitOldPathString());
	EXPECT_STREQ(L"1", list[1].m_StatAdd);
	EXPECT_STREQ(L"0", list[1].m_StatDel);
	EXPECT_EQ(0, list[1].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list[1].m_Action);
	EXPECT_FALSE(list[1].IsDirectory());
	EXPECT_STREQ(L"utf16-be-nobom.txt", list[2].GetGitPathString()); // changed from file to symlink
	EXPECT_STREQ(L"", list[2].GetGitOldPathString());
	EXPECT_STREQ(L"-", list[2].m_StatAdd);
	EXPECT_STREQ(L"-", list[2].m_StatDel);
	EXPECT_EQ(0, list[2].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[2].m_Action);
	EXPECT_FALSE(list[2].IsDirectory());
	EXPECT_STREQ(L"utf8-bom.txt", list[3].GetGitPathString());
	EXPECT_STREQ(L"", list[3].GetGitOldPathString());
	EXPECT_STREQ(L"0", list[3].m_StatAdd);
	EXPECT_STREQ(L"9", list[3].m_StatDel);
	EXPECT_EQ(0, list[3].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list[3].m_Action);
	EXPECT_FALSE(list[3].IsDirectory());
}

TEST_P(GitRevLoglistCBasicGitWithTestRepoFixture, SafeFetchFullInfo)
{
	SafeFetchFullInfo(&g_Git);
}

TEST_P(GitRevLoglistCBasicGitWithTestRepoBareFixture, SafeFetchFullInfo)
{
	SafeFetchFullInfo(&g_Git);
}

static void SafeFetchFullInfo_Submodule(CGit* cGit, config testConfig)
{
	CTGitPathList list;
	GitRevLoglist rev;
	// for "easy" tests see SafeFetchFullInfo
	rev.m_CommitHash = CGitHash::FromHexStr(L"900539cd24776a94d1b642358ccfdb9d897c8254"); // added submodule
	EXPECT_EQ(0, rev.SafeFetchFullInfo(cGit));
	EXPECT_EQ(TRUE, rev.m_IsFull);
	EXPECT_EQ(FALSE, rev.m_IsDiffFiles);
	EXPECT_EQ(FALSE, rev.m_IsCommitParsed);
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, rev.GetAction(nullptr));
	ASSERT_EQ(1, rev.GetFiles(nullptr).GetCount());
	list = rev.GetFiles(nullptr);
	EXPECT_STREQ(L"something", list[0].GetGitPathString());
	EXPECT_STREQ(L"", list[0].GetGitOldPathString());
	EXPECT_STREQ(L"1", list[0].m_StatAdd);
	EXPECT_STREQ(L"0", list[0].m_StatDel);
	EXPECT_EQ(0, list[0].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list[0].m_Action);
	EXPECT_TRUE(list[0].IsDirectory());
	rev.Clear();
	rev.m_CommitHash = CGitHash::FromHexStr(L"c8d17f57c7b511aff4aa2fbfae158902281cad8e"); // modified submodule
	EXPECT_EQ(0, rev.SafeFetchFullInfo(cGit));
	EXPECT_EQ(TRUE, rev.m_IsFull);
	EXPECT_EQ(FALSE, rev.m_IsDiffFiles);
	EXPECT_EQ(FALSE, rev.m_IsCommitParsed);
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, rev.GetAction(nullptr));
	ASSERT_EQ(1, rev.GetFiles(nullptr).GetCount());
	list = rev.GetFiles(nullptr);
	EXPECT_STREQ(L"something", list[0].GetGitPathString());
	EXPECT_STREQ(L"", list[0].GetGitOldPathString());
	EXPECT_STREQ(L"1", list[0].m_StatAdd);
	EXPECT_STREQ(L"1", list[0].m_StatDel);
	EXPECT_EQ(0, list[0].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_TRUE(list[0].IsDirectory());
	rev.Clear();
	rev.m_CommitHash = CGitHash::FromHexStr(L"4ed8d1f9ce9aedc6ad044d9051cb584a8bc294ac"); // deleted submodule
	EXPECT_EQ(0, rev.SafeFetchFullInfo(cGit));
	EXPECT_EQ(TRUE, rev.m_IsFull);
	EXPECT_EQ(FALSE, rev.m_IsDiffFiles);
	EXPECT_EQ(FALSE, rev.m_IsCommitParsed);
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, rev.GetAction(nullptr));
	ASSERT_EQ(1, rev.GetFiles(nullptr).GetCount());
	list = rev.GetFiles(nullptr);
	EXPECT_STREQ(L"something", list[0].GetGitPathString());
	EXPECT_STREQ(L"", list[0].GetGitOldPathString());
	EXPECT_STREQ(L"0", list[0].m_StatAdd);
	EXPECT_STREQ(L"1", list[0].m_StatDel);
	EXPECT_EQ(0, list[0].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list[0].m_Action);
	EXPECT_TRUE(list[0].IsDirectory());
	rev.Clear();
	rev.m_CommitHash = CGitHash::FromHexStr(L"2e63f1a55bc3dce074897200b226009f575fbcae"); // submodule to file
	EXPECT_EQ(0, rev.SafeFetchFullInfo(cGit));
	EXPECT_EQ(TRUE, rev.m_IsFull);
	EXPECT_EQ(FALSE, rev.m_IsDiffFiles);
	EXPECT_EQ(FALSE, rev.m_IsCommitParsed);
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, rev.GetAction(nullptr));
	ASSERT_EQ(1, rev.GetFiles(nullptr).GetCount());
	list = rev.GetFiles(nullptr);
	EXPECT_STREQ(L"something", list[0].GetGitPathString());
	EXPECT_STREQ(L"", list[0].GetGitOldPathString());
	if (testConfig == LIBGIT2_ALL) // TODO: libgit behaves differently here
	{
		EXPECT_STREQ(L"-", list[0].m_StatAdd);
		EXPECT_STREQ(L"-", list[0].m_StatDel);
	}
	else
	{
		EXPECT_STREQ(L"1", list[0].m_StatAdd);
		EXPECT_STREQ(L"1", list[0].m_StatDel);
	}
	EXPECT_EQ(0, list[0].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	rev.Clear();
	rev.m_CommitHash = CGitHash::FromHexStr(L"07ac6e5916c03747f7485195deb7ec9100d1c2ef"); // file to submodule
	EXPECT_EQ(0, rev.SafeFetchFullInfo(cGit));
	EXPECT_EQ(TRUE, rev.m_IsFull);
	EXPECT_EQ(FALSE, rev.m_IsDiffFiles);
	EXPECT_EQ(FALSE, rev.m_IsCommitParsed);
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, rev.GetAction(nullptr));
	ASSERT_EQ(1, rev.GetFiles(nullptr).GetCount());
	list = rev.GetFiles(nullptr);
	EXPECT_STREQ(L"something", list[0].GetGitPathString());
	EXPECT_STREQ(L"", list[0].GetGitOldPathString());
	if (testConfig == LIBGIT2_ALL) // TODO: libgit behaves differently here
	{
		EXPECT_STREQ(L"-", list[0].m_StatAdd);
		EXPECT_STREQ(L"-", list[0].m_StatDel);
	}
	else
	{
		EXPECT_STREQ(L"1", list[0].m_StatAdd);
		EXPECT_STREQ(L"1", list[0].m_StatDel);
	}
	EXPECT_EQ(0, list[0].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	rev.Clear();
	rev.m_CommitHash = CGitHash::FromHexStr(L"2d2017245cf3d016c64e5ad4eb6b0f1bccd1cf7f"); // merge use third
	EXPECT_EQ(0, rev.SafeFetchFullInfo(cGit));
	EXPECT_EQ(TRUE, rev.m_IsFull);
	EXPECT_EQ(FALSE, rev.m_IsDiffFiles);
	EXPECT_EQ(FALSE, rev.m_IsCommitParsed);
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, rev.GetAction(nullptr));
	ASSERT_EQ(2, rev.GetFiles(nullptr).GetCount());
	list = rev.GetFiles(nullptr);
	EXPECT_STREQ(L"something", list[0].GetGitPathString());
	EXPECT_STREQ(L"", list[0].GetGitOldPathString());
	EXPECT_STREQ(L"1", list[0].m_StatAdd);
	EXPECT_STREQ(L"1", list[0].m_StatDel);
	EXPECT_EQ(0, list[0].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_TRUE(list[0].IsDirectory());
	EXPECT_STREQ(L"something", list[1].GetGitPathString());
	EXPECT_STREQ(L"", list[1].GetGitOldPathString());
	EXPECT_STREQ(L"1", list[1].m_StatAdd);
	EXPECT_STREQ(L"1", list[1].m_StatDel);
	EXPECT_EQ(1, list[1].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);
	EXPECT_TRUE(list[1].IsDirectory());
}

TEST_P(GitRevLoglistCBasicGitWithSubmoduleRepoBareFixture, SafeFetchFullInfo)
{
	SafeFetchFullInfo_Submodule(&g_Git, GetParam());
}

TEST_P(GitRevLoglistCBasicGitWithSubmoduleRepoFixture, SafeFetchFullInfo)
{
	SafeFetchFullInfo_Submodule(&g_Git, GetParam());
}

static void SafeGetSimpleList(CGit* cGit)
{
	GitRevLoglist rev;
	EXPECT_EQ(-1, rev.SafeGetSimpleList(cGit));
	EXPECT_STREQ(L"", rev.GetAuthorName());
	EXPECT_EQ(FALSE, rev.m_IsSimpleListReady);
	EXPECT_EQ(TRUE, rev.m_IsUpdateing);
	EXPECT_EQ(FALSE, rev.m_IsFull);
	EXPECT_EQ(FALSE, rev.m_IsDiffFiles);
	EXPECT_EQ(FALSE, rev.m_IsCommitParsed);
	EXPECT_TRUE(rev.m_SimpleFileList.empty());
	rev.Clear();
	rev.m_CommitHash = CGitHash::FromHexStr(L"7c3cbfe13a929d2291a574dca45e4fd2d2ac1aa6");
	rev.m_IsUpdateing = TRUE; // do nothing
	EXPECT_EQ(0, rev.SafeGetSimpleList(cGit));
	EXPECT_STREQ(L"", rev.GetAuthorName());
	EXPECT_EQ(FALSE, rev.m_IsSimpleListReady);
	EXPECT_EQ(TRUE, rev.m_IsUpdateing);
	EXPECT_EQ(FALSE, rev.m_IsFull);
	EXPECT_EQ(FALSE, rev.m_IsDiffFiles);
	EXPECT_EQ(FALSE, rev.m_IsCommitParsed);
	EXPECT_TRUE(rev.m_SimpleFileList.empty());
	rev.Clear();
	rev.m_CommitHash = CGitHash::FromHexStr(L"7c3cbfe13a929d2291a574dca45e4fd2d2ac1aa6");
	EXPECT_EQ(0, rev.SafeGetSimpleList(cGit));
	EXPECT_STREQ(L"", rev.GetAuthorName());
	EXPECT_EQ(TRUE, rev.m_IsSimpleListReady);
	EXPECT_EQ(FALSE, rev.m_IsUpdateing);
	EXPECT_EQ(FALSE, rev.m_IsFull);
	EXPECT_EQ(FALSE, rev.m_IsDiffFiles);
	EXPECT_EQ(FALSE, rev.m_IsCommitParsed);
	ASSERT_EQ(1U, rev.m_SimpleFileList.size());
	EXPECT_STREQ(L"ascii.txt", rev.m_SimpleFileList[0]);
	rev.Clear();
	rev.m_CommitHash = CGitHash::FromHexStr(L"dead91b4aedeaddeaddead2a56d3c473c705dead"); // non-existent commit
	EXPECT_EQ(-1, rev.SafeGetSimpleList(cGit));
	EXPECT_EQ(FALSE, rev.m_IsSimpleListReady);
	EXPECT_EQ(TRUE, rev.m_IsUpdateing);
	EXPECT_EQ(FALSE, rev.m_IsFull);
	EXPECT_EQ(FALSE, rev.m_IsDiffFiles);
	EXPECT_EQ(FALSE, rev.m_IsCommitParsed);
	EXPECT_TRUE(rev.m_SimpleFileList.empty());
	rev.Clear();
	rev.m_CommitHash = CGitHash::FromHexStr(L"35c91b4ae2f77f4f21a7aba56d3c473c705d89e6");
	EXPECT_EQ(0, rev.SafeGetSimpleList(cGit));
	EXPECT_EQ(TRUE, rev.m_IsSimpleListReady);
	ASSERT_EQ(4U, rev.m_SimpleFileList.size());
	EXPECT_STREQ(L"copy/ansi.txt", rev.m_SimpleFileList[0]);
	EXPECT_STREQ(L"copy/utf16-be-nobom.txt", rev.m_SimpleFileList[1]);
	EXPECT_STREQ(L"copy/utf8-bom.txt", rev.m_SimpleFileList[2]);
	EXPECT_STREQ(L"copy/utf8-nobom.txt", rev.m_SimpleFileList[3]);
	rev.Clear();
	rev.m_CommitHash = CGitHash::FromHexStr(L"8d1ebbcc7eeb63af10ff8bcf7712afb9fcc90b8a"); // merge commit
	EXPECT_EQ(0, rev.SafeGetSimpleList(cGit));
	ASSERT_EQ(1U, rev.m_SimpleFileList.size());
	EXPECT_STREQ(L"newfiles3.txt", rev.m_SimpleFileList[0]);
	rev.Clear();
	rev.m_CommitHash = CGitHash::FromHexStr(L"18da7c332dcad0f37f9977d9176dce0b0c66f3eb"); // stash commit
	EXPECT_EQ(0, rev.SafeGetSimpleList(cGit));
	ASSERT_EQ(1U, rev.m_SimpleFileList.size());
	EXPECT_STREQ(L"newfiles.txt", rev.m_SimpleFileList[0]);
	rev.Clear();
	rev.m_CommitHash = CGitHash::FromHexStr(L"844309789a13614b52d5e7cbfe6350dd73d1dc72"); // root commit
	EXPECT_EQ(0, rev.SafeGetSimpleList(cGit));
	ASSERT_EQ(1U, rev.m_SimpleFileList.size());
	EXPECT_STREQ(L"ansi.txt", rev.m_SimpleFileList[0]);
	rev.Clear();
	rev.m_CommitHash = CGitHash::FromHexStr(L"4c5c93d2a0b368bc4570d5ec02ab03b9c4334d44");
	EXPECT_EQ(0, rev.SafeGetSimpleList(cGit));
	ASSERT_EQ(5U, rev.m_SimpleFileList.size());
	EXPECT_STREQ(L"ansi.txt", rev.m_SimpleFileList[0]); // has same hash as was-ansi.txt?!
	EXPECT_STREQ(L"newfiles2 - Cöpy.txt", rev.m_SimpleFileList[1]);
	EXPECT_STREQ(L"utf16-be-nobom.txt", rev.m_SimpleFileList[2]);
	EXPECT_STREQ(L"utf8-bom.txt", rev.m_SimpleFileList[3]);
	EXPECT_STREQ(L"was-ansi.txt", rev.m_SimpleFileList[4]);
}

TEST_P(GitRevLoglistCBasicGitWithTestRepoFixture, SafeGetSimpleList)
{
	SafeGetSimpleList(&g_Git);
}

TEST_P(GitRevLoglistCBasicGitWithTestRepoBareFixture, SafeGetSimpleList)
{
	SafeGetSimpleList(&g_Git);
}

static void SafeGetSimpleList_Submodule(CGit* cGit)
{
	GitRevLoglist rev;
	// for "easy" tests see SafeGetSimpleList
	rev.m_CommitHash = CGitHash::FromHexStr(L"900539cd24776a94d1b642358ccfdb9d897c8254"); // added submodule
	EXPECT_EQ(0, rev.SafeGetSimpleList(cGit));
	ASSERT_EQ(1U, rev.m_SimpleFileList.size());
	EXPECT_STREQ(L"something", rev.m_SimpleFileList[0]);
	rev.Clear();
	rev.m_CommitHash = CGitHash::FromHexStr(L"c8d17f57c7b511aff4aa2fbfae158902281cad8e"); // modified submodule
	EXPECT_EQ(0, rev.SafeGetSimpleList(cGit));
	ASSERT_EQ(1U, rev.m_SimpleFileList.size());
	EXPECT_STREQ(L"something", rev.m_SimpleFileList[0]);
	rev.Clear();
	rev.m_CommitHash = CGitHash::FromHexStr(L"4ed8d1f9ce9aedc6ad044d9051cb584a8bc294ac"); // deleted submodule
	EXPECT_EQ(0, rev.SafeGetSimpleList(cGit));
	ASSERT_EQ(1U, rev.m_SimpleFileList.size());
	EXPECT_STREQ(L"something", rev.m_SimpleFileList[0]);
	rev.Clear();
	rev.m_CommitHash = CGitHash::FromHexStr(L"2e63f1a55bc3dce074897200b226009f575fbcae"); // submodule to file
	EXPECT_EQ(0, rev.SafeGetSimpleList(cGit));
	ASSERT_EQ(1U, rev.m_SimpleFileList.size());
	EXPECT_STREQ(L"something", rev.m_SimpleFileList[0]);
	rev.Clear();
	rev.m_CommitHash = CGitHash::FromHexStr(L"07ac6e5916c03747f7485195deb7ec9100d1c2ef"); // file to submodule
	EXPECT_EQ(0, rev.SafeGetSimpleList(cGit));
	ASSERT_EQ(1U, rev.m_SimpleFileList.size());
	EXPECT_STREQ(L"something", rev.m_SimpleFileList[0]);
	rev.Clear();
	rev.m_CommitHash = CGitHash::FromHexStr(L"2d2017245cf3d016c64e5ad4eb6b0f1bccd1cf7f"); // merge use third
	EXPECT_EQ(0, rev.SafeGetSimpleList(cGit));
	ASSERT_EQ(1U, rev.m_SimpleFileList.size());
	EXPECT_STREQ(L"something", rev.m_SimpleFileList[0]);
}

TEST_P(GitRevLoglistCBasicGitWithSubmoduleRepoFixture, SafeGetSimpleList)
{
	SafeGetSimpleList_Submodule(&g_Git);
}

TEST_P(GitRevLoglistCBasicGitWithSubmoduleRepoBareFixture, SafeGetSimpleList)
{
	SafeGetSimpleList_Submodule(&g_Git);
}

TEST(GitRevLoglist, IsBoundary)
{
	GitRevLoglist rev;
	EXPECT_EQ(FALSE, rev.IsBoundary());
	rev.m_Mark = L'-';
	EXPECT_EQ(TRUE, rev.IsBoundary());
	rev.Clear();
	EXPECT_EQ(FALSE, rev.IsBoundary());
}

TEST(GitRevLoglist, GetUnRevFiles)
{
	GitRevLoglist rev;
	EXPECT_EQ(0, rev.GetUnRevFiles().GetCount());
	CTGitPathList& list = rev.GetUnRevFiles();
	list.AddPath(CTGitPath("file.txt"));
	EXPECT_EQ(1, rev.GetUnRevFiles().GetCount());
	rev.Clear();
	EXPECT_EQ(0, list.GetCount());
	EXPECT_EQ(0, rev.GetUnRevFiles().GetCount());
}

TEST(GitRevLoglist, GetAction)
{
	GitRevLoglist rev;
	EXPECT_EQ(0U, rev.GetAction(nullptr));
	auto& action = rev.GetAction(nullptr);
	action = 5;
	EXPECT_EQ(5U, rev.GetAction(nullptr));
	rev.Clear();
	EXPECT_EQ(0U, action);
	EXPECT_EQ(0U, rev.GetAction(nullptr));
}

static void GetReflog()
{
	CString err;
	std::vector<GitRevLoglist> revloglist;
	EXPECT_EQ(0, GitRevLoglist::GetRefLog(L"refs/stash", revloglist, err));
	EXPECT_EQ(0U, revloglist.size());
	EXPECT_STREQ(L"", err);

	EXPECT_EQ(0, GitRevLoglist::GetRefLog(L"HEAD", revloglist, err));
	EXPECT_EQ(12U, revloglist.size());
	EXPECT_STREQ(L"", err);

	revloglist.clear();
	EXPECT_EQ(0, GitRevLoglist::GetRefLog(L"refs/heads/does-not-exist", revloglist, err));
	EXPECT_EQ(0U, revloglist.size());
	EXPECT_STREQ(L"", err);

	err.Empty();
	EXPECT_EQ(0, GitRevLoglist::GetRefLog(L"refs/heads/master", revloglist, err));
	ASSERT_EQ(7U, revloglist.size());
	EXPECT_STREQ(L"", err);

	EXPECT_STREQ(L"7c3cbfe13a929d2291a574dca45e4fd2d2ac1aa6", revloglist[0].m_CommitHash.ToString());
	EXPECT_STREQ(L"refs/heads/master@{0}", revloglist[0].m_Ref);
	EXPECT_STREQ(L"reset", revloglist[0].m_RefAction);
	//EXPECT_STREQ(L"Sven Strickroth", revloglist[0].GetCommitterName());
	//EXPECT_STREQ(L"email@cs-ware.de", revloglist[0].GetCommitterEmail());
	EXPECT_STREQ(L"2015-03-16 12:59:51", revloglist[0].GetCommitterDate().FormatGmt(L"%Y-%m-%d %H:%M:%S"));
	EXPECT_STREQ(L"moving to 7c3cbfe13a929d2291a574dca45e4fd2d2ac1aa6", revloglist[0].GetSubject());

	EXPECT_STREQ(L"aa5b97f89cea6863222823c8289ce392d06d1691", revloglist[2].m_CommitHash.ToString());
	EXPECT_STREQ(L"refs/heads/master@{2}", revloglist[2].m_Ref);
	EXPECT_STREQ(L"reset", revloglist[2].m_RefAction);
	//EXPECT_STREQ(L"Dümmy User", revloglist[2].GetCommitterName());
	//EXPECT_STREQ(L"dummy@example.com", revloglist[2].GetCommitterEmail());
	EXPECT_STREQ(L"2015-03-16 12:59:07", revloglist[2].GetCommitterDate().FormatGmt(L"%Y-%m-%d %H:%M:%S"));
	EXPECT_STREQ(L"moving to aa5b97f89cea6863222823c8289ce392d06d1691", revloglist[2].GetSubject());

	EXPECT_STREQ(L"df8019413c88d2aedbf33fc2dac3544312da4c18", revloglist[3].m_CommitHash.ToString());
	EXPECT_STREQ(L"refs/heads/master@{3}", revloglist[3].m_Ref);
	EXPECT_STREQ(L"commit (amend)", revloglist[3].m_RefAction);
	//EXPECT_STREQ(L"Sven Strickroth", revloglist[3].GetCommitterName());
	//EXPECT_STREQ(L"email@cs-ware.de", revloglist[3].GetCommitterEmail());
	EXPECT_STREQ(L"2015-03-16 12:55:00", revloglist[3].GetCommitterDate().FormatGmt(L"%Y-%m-%d %H:%M:%S"));
	EXPECT_STREQ(L"Several actions", revloglist[3].GetSubject());

	EXPECT_STREQ(L"32c344625f14ecb16b6f003a77eb7a3d2c15d470", revloglist[4].m_CommitHash.ToString());
	EXPECT_STREQ(L"refs/heads/master@{4}", revloglist[4].m_Ref);
	EXPECT_STREQ(L"commit", revloglist[4].m_RefAction);
	//EXPECT_STREQ(L"Sven Strickroth", revloglist[4].GetCommitterName());
	//EXPECT_STREQ(L"email@cs-ware.de", revloglist[4].GetCommitterEmail());
	EXPECT_STREQ(L"2015-03-16 12:52:22", revloglist[4].GetCommitterDate().FormatGmt(L"%Y-%m-%d %H:%M:%S"));
	EXPECT_STREQ(L"renamed a file", revloglist[4].GetSubject());

	EXPECT_STREQ(L"7c3cbfe13a929d2291a574dca45e4fd2d2ac1aa6", revloglist[5].m_CommitHash.ToString());
	EXPECT_STREQ(L"refs/heads/master@{5}", revloglist[5].m_Ref);
	EXPECT_STREQ(L"fetch origin +refs/heads/*:refs/heads/*", revloglist[5].m_RefAction);
	//EXPECT_STREQ(L"Dummy author", revloglist[5].GetCommitterName());
	//EXPECT_STREQ(L"a@example.com", revloglist[5].GetCommitterEmail());
	EXPECT_STREQ(L"2015-03-16 12:52:10", revloglist[5].GetCommitterDate().FormatGmt(L"%Y-%m-%d %H:%M:%S"));
	EXPECT_STREQ(L"forced-update", revloglist[5].GetSubject());

	EXPECT_STREQ(L"560deea87853158b22d0c0fd73f60a458d47838a", revloglist[6].m_CommitHash.ToString());
	EXPECT_STREQ(L"refs/heads/master@{6}", revloglist[6].m_Ref);
	EXPECT_STREQ(L"push", revloglist[6].m_RefAction);
	//EXPECT_STREQ(L"Sven Strickroth", revloglist[6].GetCommitterName());
	//EXPECT_STREQ(L"email@cs-ware.de", revloglist[6].GetCommitterEmail());
	EXPECT_STREQ(L"2015-03-16 12:51:40", revloglist[6].GetCommitterDate().FormatGmt(L"%Y-%m-%d %H:%M:%S"));
	EXPECT_STREQ(L"", revloglist[6].GetSubject());
}

TEST_P(GitRevLoglist2CBasicGitWithTestRepoFixture, GetReflog)
{
	GetReflog();
}

TEST_P(GitRevLoglist2CBasicGitWithTestRepoBareFixture, GetReflog)
{
	GetReflog();
}
