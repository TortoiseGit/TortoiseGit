// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2015 - TortoiseGit

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

class GitRevLoglist2CBasicGitWithTestRepoFixture : public CBasicGitWithTestRepoFixture
{
};

class GitRevLoglist2CBasicGitWithTestRepoBareFixture : public CBasicGitWithTestRepoBareFixture
{
};

INSTANTIATE_TEST_CASE_P(GitRevLoglist, GitRevLoglistCBasicGitWithTestRepoFixture, testing::Values(LIBGIT, LIBGIT2, LIBGIT2_ALL));
INSTANTIATE_TEST_CASE_P(GitRevLoglist, GitRevLoglistCBasicGitWithTestRepoBareFixture, testing::Values(LIBGIT, LIBGIT2, LIBGIT2_ALL));

INSTANTIATE_TEST_CASE_P(GitRevLoglist, GitRevLoglist2CBasicGitWithTestRepoFixture, testing::Values(GIT_CLI, LIBGIT, LIBGIT2, LIBGIT2_ALL));
INSTANTIATE_TEST_CASE_P(GitRevLoglist, GitRevLoglist2CBasicGitWithTestRepoBareFixture, testing::Values(GIT_CLI, LIBGIT, LIBGIT2, LIBGIT2_ALL));

static void SafeFetchFullInfo(CGit* cGit)
{
	GitRevLoglist rev;
	EXPECT_EQ(FALSE, rev.m_IsSimpleListReady);
	EXPECT_EQ(1, rev.CheckAndDiff());
	EXPECT_STREQ(_T(""), rev.GetAuthorName());
	EXPECT_EQ(FALSE, rev.m_IsSimpleListReady);
	EXPECT_EQ(FALSE, rev.m_IsFull);
	EXPECT_EQ(FALSE, rev.m_IsUpdateing);
	EXPECT_EQ(FALSE, rev.m_IsDiffFiles);
	EXPECT_EQ(FALSE, rev.m_IsCommitParsed);
	rev.m_IsDiffFiles = 1;
	EXPECT_EQ(0, rev.GetFiles(nullptr).GetCount());
	EXPECT_EQ(-1, rev.SafeFetchFullInfo(cGit));
	EXPECT_STREQ(_T(""), rev.GetAuthorName());
	EXPECT_EQ(FALSE, rev.m_IsSimpleListReady);
	EXPECT_EQ(TRUE, rev.m_IsUpdateing);
	EXPECT_EQ(FALSE, rev.m_IsFull);
	EXPECT_EQ(TRUE, rev.m_IsDiffFiles);
	EXPECT_EQ(FALSE, rev.m_IsCommitParsed);
	rev.Clear();
	rev.m_CommitHash = _T("7c3cbfe13a929d2291a574dca45e4fd2d2ac1aa6");
	rev.m_IsDiffFiles = TRUE;
	EXPECT_EQ(1, rev.CheckAndDiff());
	EXPECT_EQ(FALSE, rev.m_IsSimpleListReady);
	EXPECT_EQ(FALSE, rev.m_IsUpdateing);
	EXPECT_EQ(FALSE, rev.m_IsFull);
	EXPECT_EQ(FALSE, rev.m_IsCommitParsed);
	EXPECT_STREQ(_T(""), rev.GetAuthorName());
	rev.m_IsDiffFiles = FALSE;
	EXPECT_EQ(FALSE, rev.m_IsSimpleListReady);
	EXPECT_EQ(FALSE, rev.m_IsUpdateing);
	EXPECT_EQ(FALSE, rev.m_IsFull);
	EXPECT_EQ(FALSE, rev.m_IsDiffFiles);
	EXPECT_EQ(FALSE, rev.m_IsCommitParsed);
	EXPECT_EQ(0, rev.CheckAndDiff());
	EXPECT_STREQ(_T(""), rev.GetAuthorName());
	EXPECT_EQ(FALSE, rev.m_IsSimpleListReady);
	EXPECT_EQ(FALSE, rev.m_IsUpdateing);
	EXPECT_EQ(TRUE, rev.m_IsFull);
	EXPECT_EQ(TRUE, rev.m_IsDiffFiles);
	EXPECT_EQ(FALSE, rev.m_IsCommitParsed);
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, rev.GetAction(nullptr));
	ASSERT_EQ(1, rev.GetFiles(nullptr).GetCount());
	CTGitPathList list;
	list = rev.GetFiles(nullptr);
	EXPECT_STREQ(_T("ascii.txt"), list[0].GetGitPathString());
	EXPECT_STREQ(_T("2"), list[0].m_StatAdd);
	EXPECT_STREQ(_T("2"), list[0].m_StatDel);
	EXPECT_EQ(0, list[0].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	rev.Clear();
	rev.m_CommitHash = _T("7c3cbfe13a929d2291a574dca45e4fd2d2ac1aa6");
	rev.m_IsUpdateing = TRUE; // do nothing
	EXPECT_EQ(0, rev.SafeFetchFullInfo(cGit));
	EXPECT_STREQ(_T(""), rev.GetAuthorName());
	EXPECT_EQ(FALSE, rev.m_IsSimpleListReady);
	EXPECT_EQ(TRUE, rev.m_IsUpdateing);
	EXPECT_EQ(FALSE, rev.m_IsFull);
	EXPECT_EQ(FALSE, rev.m_IsDiffFiles);
	EXPECT_EQ(FALSE, rev.m_IsCommitParsed);
	EXPECT_EQ(0, rev.GetAction(nullptr));
	EXPECT_EQ(0, rev.GetFiles(nullptr).GetCount());
	rev.Clear();
	rev.m_CommitHash = _T("7c3cbfe13a929d2291a574dca45e4fd2d2ac1aa6");
	EXPECT_EQ(0, rev.SafeFetchFullInfo(cGit));
	EXPECT_STREQ(_T(""), rev.GetAuthorName());
	EXPECT_EQ(FALSE, rev.m_IsSimpleListReady);
	EXPECT_EQ(FALSE, rev.m_IsUpdateing);
	EXPECT_EQ(TRUE, rev.m_IsFull);
	EXPECT_EQ(FALSE, rev.m_IsDiffFiles);
	EXPECT_EQ(FALSE, rev.m_IsCommitParsed);
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, rev.GetAction(nullptr));
	ASSERT_EQ(1, rev.GetFiles(nullptr).GetCount());
	list = rev.GetFiles(nullptr);
	EXPECT_STREQ(_T("ascii.txt"), list[0].GetGitPathString());
	EXPECT_STREQ(_T("2"), list[0].m_StatAdd);
	EXPECT_STREQ(_T("2"), list[0].m_StatDel);
	EXPECT_EQ(0, list[0].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	rev.Clear();
	EXPECT_EQ(0, rev.GetFiles(nullptr).GetCount());
	rev.m_CommitHash = _T("dead91b4aedeaddeaddead2a56d3c473c705dead"); // non-existent commit
	EXPECT_EQ(-1, rev.SafeFetchFullInfo(cGit));
	EXPECT_EQ(FALSE, rev.m_IsSimpleListReady);
	EXPECT_EQ(TRUE, rev.m_IsUpdateing);
	EXPECT_EQ(FALSE, rev.m_IsFull);
	EXPECT_EQ(FALSE, rev.m_IsDiffFiles);
	EXPECT_EQ(FALSE, rev.m_IsCommitParsed);
	rev.Clear();
	rev.m_CommitHash = _T("35c91b4ae2f77f4f21a7aba56d3c473c705d89e6");
	EXPECT_EQ(0, rev.SafeFetchFullInfo(cGit));
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, rev.GetAction(nullptr));
	ASSERT_EQ(4, rev.GetFiles(nullptr).GetCount());
	list = rev.GetFiles(nullptr);
	EXPECT_STREQ(_T("copy/ansi.txt"), list[0].GetGitPathString());
	EXPECT_STREQ(_T("1"), list[0].m_StatAdd);
	EXPECT_STREQ(_T("1"), list[0].m_StatDel);
	EXPECT_EQ(0, list[0].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_STREQ(_T("copy/utf16-be-nobom.txt"), list[1].GetGitPathString());
	EXPECT_STREQ(_T("-"), list[1].m_StatAdd);
	EXPECT_STREQ(_T("-"), list[1].m_StatDel);
	EXPECT_EQ(0, list[1].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);
	EXPECT_STREQ(_T("copy/utf8-bom.txt"), list[2].GetGitPathString());
	EXPECT_STREQ(_T("1"), list[2].m_StatAdd);
	EXPECT_STREQ(_T("1"), list[2].m_StatDel);
	EXPECT_EQ(0, list[2].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[2].m_Action);
	EXPECT_STREQ(_T("copy/utf8-nobom.txt"), list[3].GetGitPathString());
	EXPECT_STREQ(_T("1"), list[3].m_StatAdd);
	EXPECT_STREQ(_T("1"), list[3].m_StatDel);
	EXPECT_EQ(0, list[3].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[3].m_Action);
	rev.Clear();
	rev.m_CommitHash = _T("8d1ebbcc7eeb63af10ff8bcf7712afb9fcc90b8a"); // merge commit
	EXPECT_EQ(0, rev.SafeFetchFullInfo(cGit));
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, rev.GetAction(nullptr));
	ASSERT_EQ(1, rev.GetFiles(nullptr).GetCount());
	list = rev.GetFiles(nullptr);
	EXPECT_STREQ(_T("newfiles3.txt"), list[0].GetGitPathString());
	EXPECT_STREQ(_T("1"), list[0].m_StatAdd);
	EXPECT_STREQ(_T("0"), list[0].m_StatDel);
	EXPECT_EQ(0, list[0].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list[0].m_Action);
	rev.Clear();
	rev.m_CommitHash = _T("18da7c332dcad0f37f9977d9176dce0b0c66f3eb"); // stash commit
	EXPECT_EQ(0, rev.SafeFetchFullInfo(cGit));
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, rev.GetAction(nullptr));
	ASSERT_EQ(2, rev.GetFiles(nullptr).GetCount());
	list = rev.GetFiles(nullptr);
	EXPECT_STREQ(_T("newfiles.txt"), list[0].GetGitPathString());
	EXPECT_STREQ(_T("1"), list[0].m_StatAdd);
	EXPECT_STREQ(_T("1"), list[0].m_StatDel);
	EXPECT_EQ(0, list[0].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_STREQ(_T("newfiles.txt"), list[1].GetGitPathString());
	EXPECT_STREQ(_T("1"), list[1].m_StatAdd);
	EXPECT_STREQ(_T("1"), list[1].m_StatDel);
	EXPECT_EQ(1, list[1].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);
	rev.Clear();
	rev.m_CommitHash = _T("844309789a13614b52d5e7cbfe6350dd73d1dc72"); // root commit
	EXPECT_EQ(0, rev.SafeFetchFullInfo(cGit));
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, rev.GetAction(nullptr));
	ASSERT_EQ(1, rev.GetFiles(nullptr).GetCount());
	EXPECT_STREQ(_T("ansi.txt"), rev.GetFiles(nullptr)[0].GetGitPathString());
	rev.Clear();
	rev.m_CommitHash = _T("4c5c93d2a0b368bc4570d5ec02ab03b9c4334d44");
	EXPECT_EQ(0, rev.SafeFetchFullInfo(cGit));
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED | CTGitPath::LOGACTIONS_MODIFIED | CTGitPath::LOGACTIONS_REPLACED | CTGitPath::LOGACTIONS_DELETED, rev.GetAction(nullptr));
	ASSERT_EQ(4, rev.GetFiles(nullptr).GetCount());
	list = rev.GetFiles(nullptr);
	EXPECT_STREQ(_T("newfiles2 - Cöpy.txt"), list[0].GetGitPathString());
	EXPECT_STREQ(_T("1"), list[0].m_StatAdd);
	EXPECT_STREQ(_T("0"), list[0].m_StatDel);
	EXPECT_EQ(0, list[0].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list[0].m_Action);
	EXPECT_STREQ(_T("utf16-be-nobom.txt"), list[1].GetGitPathString()); // changed from file to symlink
	EXPECT_STREQ(_T("-"), list[1].m_StatAdd);
	EXPECT_STREQ(_T("-"), list[1].m_StatDel);
	EXPECT_EQ(0, list[1].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);
	EXPECT_STREQ(_T("utf8-bom.txt"), list[2].GetGitPathString());
	EXPECT_STREQ(_T("0"), list[2].m_StatAdd);
	EXPECT_STREQ(_T("9"), list[2].m_StatDel);
	EXPECT_EQ(0, list[2].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list[2].m_Action);
	EXPECT_STREQ(_T("was-ansi.txt"), list[3].GetGitPathString());
	EXPECT_STREQ(_T("0"), list[3].m_StatAdd);
	EXPECT_STREQ(_T("0"), list[3].m_StatDel);
	EXPECT_EQ(0, list[3].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED, list[3].m_Action);
}

TEST_P(GitRevLoglistCBasicGitWithTestRepoFixture, SafeFetchFullInfo)
{
	SafeFetchFullInfo(&g_Git);
}

TEST_P(GitRevLoglistCBasicGitWithTestRepoBareFixture, SafeFetchFullInfo)
{
	SafeFetchFullInfo(&g_Git);
}

static void SafeGetSimpleList(CGit* cGit)
{
	GitRevLoglist rev;
	EXPECT_EQ(-1, rev.SafeGetSimpleList(cGit));
	EXPECT_STREQ(_T(""), rev.GetAuthorName());
	EXPECT_EQ(FALSE, rev.m_IsSimpleListReady);
	EXPECT_EQ(TRUE, rev.m_IsUpdateing);
	EXPECT_EQ(FALSE, rev.m_IsFull);
	EXPECT_EQ(FALSE, rev.m_IsDiffFiles);
	EXPECT_EQ(FALSE, rev.m_IsCommitParsed);
	rev.Clear();
	rev.m_CommitHash = _T("7c3cbfe13a929d2291a574dca45e4fd2d2ac1aa6");
	rev.m_IsDiffFiles = TRUE;
	CTGitPathList list;
	rev.Clear();
	rev.m_CommitHash = _T("7c3cbfe13a929d2291a574dca45e4fd2d2ac1aa6");
	rev.m_IsUpdateing = TRUE; // do nothing
	EXPECT_EQ(0, rev.SafeGetSimpleList(cGit));
	EXPECT_STREQ(_T(""), rev.GetAuthorName());
	EXPECT_EQ(FALSE, rev.m_IsSimpleListReady);
	EXPECT_EQ(TRUE, rev.m_IsUpdateing);
	EXPECT_EQ(FALSE, rev.m_IsFull);
	EXPECT_EQ(FALSE, rev.m_IsDiffFiles);
	EXPECT_EQ(FALSE, rev.m_IsCommitParsed);
	EXPECT_EQ(0, rev.GetAction(nullptr));
	EXPECT_EQ(0, rev.GetFiles(nullptr).GetCount());
	rev.Clear();
	rev.m_CommitHash = _T("7c3cbfe13a929d2291a574dca45e4fd2d2ac1aa6");
	EXPECT_EQ(0, rev.SafeGetSimpleList(cGit));
	EXPECT_STREQ(_T(""), rev.GetAuthorName());
	EXPECT_EQ(TRUE, rev.m_IsSimpleListReady);
	EXPECT_EQ(FALSE, rev.m_IsUpdateing);
	EXPECT_EQ(FALSE, rev.m_IsFull);
	EXPECT_EQ(FALSE, rev.m_IsDiffFiles);
	EXPECT_EQ(FALSE, rev.m_IsCommitParsed);
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, rev.GetAction(nullptr));
	ASSERT_EQ(1, rev.GetFiles(nullptr).GetCount());
	list = rev.GetFiles(nullptr);
	EXPECT_STREQ(_T("ascii.txt"), list[0].GetGitPathString());
	EXPECT_STREQ(_T("2"), list[0].m_StatAdd);
	EXPECT_STREQ(_T("2"), list[0].m_StatDel);
	EXPECT_EQ(0, list[0].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	rev.Clear();
	EXPECT_EQ(0, rev.GetFiles(nullptr).GetCount());
	rev.m_CommitHash = _T("dead91b4aedeaddeaddead2a56d3c473c705dead"); // non-existent commit
	EXPECT_EQ(-1, rev.SafeGetSimpleList(cGit));
	EXPECT_EQ(FALSE, rev.m_IsSimpleListReady);
	EXPECT_EQ(TRUE, rev.m_IsUpdateing);
	EXPECT_EQ(FALSE, rev.m_IsFull);
	EXPECT_EQ(FALSE, rev.m_IsDiffFiles);
	EXPECT_EQ(FALSE, rev.m_IsCommitParsed);
	rev.Clear();
	rev.m_CommitHash = _T("35c91b4ae2f77f4f21a7aba56d3c473c705d89e6");
	EXPECT_EQ(0, rev.SafeGetSimpleList(cGit));
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, rev.GetAction(nullptr));
	ASSERT_EQ(4, rev.GetFiles(nullptr).GetCount());
	list = rev.GetFiles(nullptr);
	EXPECT_STREQ(_T("copy/ansi.txt"), list[0].GetGitPathString());
	EXPECT_STREQ(_T("1"), list[0].m_StatAdd);
	EXPECT_STREQ(_T("1"), list[0].m_StatDel);
	EXPECT_EQ(0, list[0].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_STREQ(_T("copy/utf16-be-nobom.txt"), list[1].GetGitPathString());
	EXPECT_STREQ(_T("-"), list[1].m_StatAdd);
	EXPECT_STREQ(_T("-"), list[1].m_StatDel);
	EXPECT_EQ(0, list[1].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);
	EXPECT_STREQ(_T("copy/utf8-bom.txt"), list[2].GetGitPathString());
	EXPECT_STREQ(_T("1"), list[2].m_StatAdd);
	EXPECT_STREQ(_T("1"), list[2].m_StatDel);
	EXPECT_EQ(0, list[2].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[2].m_Action);
	EXPECT_STREQ(_T("copy/utf8-nobom.txt"), list[3].GetGitPathString());
	EXPECT_STREQ(_T("1"), list[3].m_StatAdd);
	EXPECT_STREQ(_T("1"), list[3].m_StatDel);
	EXPECT_EQ(0, list[3].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[3].m_Action);
	rev.Clear();
	rev.m_CommitHash = _T("8d1ebbcc7eeb63af10ff8bcf7712afb9fcc90b8a"); // merge commit
	EXPECT_EQ(0, rev.SafeGetSimpleList(cGit));
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, rev.GetAction(nullptr));
	ASSERT_EQ(1, rev.GetFiles(nullptr).GetCount());
	list = rev.GetFiles(nullptr);
	EXPECT_STREQ(_T("newfiles3.txt"), list[0].GetGitPathString());
	EXPECT_STREQ(_T("1"), list[0].m_StatAdd);
	EXPECT_STREQ(_T("0"), list[0].m_StatDel);
	EXPECT_EQ(0, list[0].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list[0].m_Action);
	rev.Clear();
	rev.m_CommitHash = _T("18da7c332dcad0f37f9977d9176dce0b0c66f3eb"); // stash commit
	EXPECT_EQ(0, rev.SafeGetSimpleList(cGit));
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, rev.GetAction(nullptr));
	ASSERT_EQ(2, rev.GetFiles(nullptr).GetCount());
	list = rev.GetFiles(nullptr);
	EXPECT_STREQ(_T("newfiles.txt"), list[0].GetGitPathString());
	EXPECT_STREQ(_T("1"), list[0].m_StatAdd);
	EXPECT_STREQ(_T("1"), list[0].m_StatDel);
	EXPECT_EQ(0, list[0].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_STREQ(_T("newfiles.txt"), list[1].GetGitPathString());
	EXPECT_STREQ(_T("1"), list[1].m_StatAdd);
	EXPECT_STREQ(_T("1"), list[1].m_StatDel);
	EXPECT_EQ(1, list[1].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);
	rev.Clear();
	rev.m_CommitHash = _T("844309789a13614b52d5e7cbfe6350dd73d1dc72"); // root commit
	EXPECT_EQ(0, rev.SafeGetSimpleList(cGit));
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, rev.GetAction(nullptr));
	ASSERT_EQ(1, rev.GetFiles(nullptr).GetCount());
	EXPECT_STREQ(_T("ansi.txt"), rev.GetFiles(nullptr)[0].GetGitPathString());
	rev.Clear();
	rev.m_CommitHash = _T("4c5c93d2a0b368bc4570d5ec02ab03b9c4334d44");
	EXPECT_EQ(0, rev.SafeGetSimpleList(cGit));
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED | CTGitPath::LOGACTIONS_MODIFIED | CTGitPath::LOGACTIONS_DELETED | CTGitPath::LOGACTIONS_REPLACED, rev.GetAction(nullptr));
	ASSERT_EQ(4, rev.GetFiles(nullptr).GetCount());
	list = rev.GetFiles(nullptr);
	EXPECT_STREQ(_T("newfiles2 - Cöpy.txt"), list[0].GetGitPathString());
	EXPECT_STREQ(_T("1"), list[0].m_StatAdd);
	EXPECT_STREQ(_T("0"), list[0].m_StatDel);
	EXPECT_EQ(0, list[0].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list[0].m_Action);
	EXPECT_STREQ(_T("utf16-be-nobom.txt"), list[1].GetGitPathString()); // changed from file to symlink
	EXPECT_STREQ(_T("-"), list[1].m_StatAdd);
	EXPECT_STREQ(_T("-"), list[1].m_StatDel);
	EXPECT_EQ(0, list[1].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);
	EXPECT_STREQ(_T("utf8-bom.txt"), list[2].GetGitPathString());
	EXPECT_STREQ(_T("0"), list[2].m_StatAdd);
	EXPECT_STREQ(_T("9"), list[2].m_StatDel);
	EXPECT_EQ(0, list[2].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list[2].m_Action);
	EXPECT_STREQ(_T("was-ansi.txt"), list[3].GetGitPathString());
	EXPECT_STREQ(_T("0"), list[3].m_StatAdd);
	EXPECT_STREQ(_T("0"), list[3].m_StatDel);
	EXPECT_EQ(0, list[3].m_ParentNo);
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED, list[3].m_Action);
}

TEST_P(GitRevLoglistCBasicGitWithTestRepoFixture, SafeGetSimpleList)
{
	SafeGetSimpleList(&g_Git);
}

TEST_P(GitRevLoglistCBasicGitWithTestRepoBareFixture, SafeGetSimpleList)
{
	SafeGetSimpleList(&g_Git);
}

TEST(GitRevLoglist, IsBoundary)
{
	GitRevLoglist rev;
	EXPECT_EQ(FALSE, rev.IsBoundary());
	rev.m_Mark = _T('-');
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
	EXPECT_EQ(0, rev.GetAction(nullptr));
	int& action = rev.GetAction(nullptr);
	action = 5;
	EXPECT_EQ(5, rev.GetAction(nullptr));
	rev.Clear();
	EXPECT_EQ(0, action);
	EXPECT_EQ(0, rev.GetAction(nullptr));
}

static void GetReflog()
{
	CString err;
	std::vector<GitRevLoglist> revloglist;
	EXPECT_EQ(0, GitRevLoglist::GetRefLog(_T("refs/stash"), revloglist, err));
	EXPECT_EQ(5, revloglist.size());
	EXPECT_TRUE(err.IsEmpty());

	EXPECT_EQ(0, GitRevLoglist::GetRefLog(_T("HEAD"), revloglist, err));
	EXPECT_EQ(12, revloglist.size());
	EXPECT_TRUE(err.IsEmpty());

	revloglist.clear();
	EXPECT_EQ(0, GitRevLoglist::GetRefLog(_T("refs/heads/does-not-exist"), revloglist, err));
	EXPECT_EQ(0, revloglist.size());
	EXPECT_TRUE(err.IsEmpty());

	err.Empty();
	EXPECT_EQ(0, GitRevLoglist::GetRefLog(_T("refs/heads/master"), revloglist, err));
	ASSERT_EQ(7, revloglist.size());
	EXPECT_TRUE(err.IsEmpty());

	EXPECT_STREQ(_T("7c3cbfe13a929d2291a574dca45e4fd2d2ac1aa6"), revloglist[0].m_CommitHash.ToString());
	EXPECT_STREQ(_T("refs/heads/master@{0}"), revloglist[0].m_Ref);
	EXPECT_STREQ(_T("reset"), revloglist[0].m_RefAction);
	//EXPECT_STREQ(_T("Sven Strickroth"), revloglist[0].GetCommitterName());
	//EXPECT_STREQ(_T("email@cs-ware.de"), revloglist[0].GetCommitterEmail());
	EXPECT_STREQ(_T("2015-03-16 12:59:51"), revloglist[0].GetCommitterDate().FormatGmt(L"%Y-%m-%d %H:%M:%S"));
	EXPECT_STREQ(_T("moving to 7c3cbfe13a929d2291a574dca45e4fd2d2ac1aa6"), revloglist[0].GetSubject());

	EXPECT_STREQ(_T("aa5b97f89cea6863222823c8289ce392d06d1691"), revloglist[2].m_CommitHash.ToString());
	EXPECT_STREQ(_T("refs/heads/master@{2}"), revloglist[2].m_Ref);
	EXPECT_STREQ(_T("reset"), revloglist[2].m_RefAction);
	//EXPECT_STREQ(_T("Dümmy User"), revloglist[2].GetCommitterName());
	//EXPECT_STREQ(_T("dummy@example.com"), revloglist[2].GetCommitterEmail());
	EXPECT_STREQ(_T("2015-03-16 12:59:07"), revloglist[2].GetCommitterDate().FormatGmt(L"%Y-%m-%d %H:%M:%S"));
	EXPECT_STREQ(_T("moving to aa5b97f89cea6863222823c8289ce392d06d1691"), revloglist[2].GetSubject());

	EXPECT_STREQ(_T("df8019413c88d2aedbf33fc2dac3544312da4c18"), revloglist[3].m_CommitHash.ToString());
	EXPECT_STREQ(_T("refs/heads/master@{3}"), revloglist[3].m_Ref);
	EXPECT_STREQ(_T("commit (amend)"), revloglist[3].m_RefAction);
	//EXPECT_STREQ(_T("Sven Strickroth"), revloglist[3].GetCommitterName());
	//EXPECT_STREQ(_T("email@cs-ware.de"), revloglist[3].GetCommitterEmail());
	EXPECT_STREQ(_T("2015-03-16 12:55:00"), revloglist[3].GetCommitterDate().FormatGmt(L"%Y-%m-%d %H:%M:%S"));
	EXPECT_STREQ(_T("Several actions"), revloglist[3].GetSubject());

	EXPECT_STREQ(_T("32c344625f14ecb16b6f003a77eb7a3d2c15d470"), revloglist[4].m_CommitHash.ToString());
	EXPECT_STREQ(_T("refs/heads/master@{4}"), revloglist[4].m_Ref);
	EXPECT_STREQ(_T("commit"), revloglist[4].m_RefAction);
	//EXPECT_STREQ(_T("Sven Strickroth"), revloglist[4].GetCommitterName());
	//EXPECT_STREQ(_T("email@cs-ware.de"), revloglist[4].GetCommitterEmail());
	EXPECT_STREQ(_T("2015-03-16 12:52:22"), revloglist[4].GetCommitterDate().FormatGmt(L"%Y-%m-%d %H:%M:%S"));
	EXPECT_STREQ(_T("renamed a file"), revloglist[4].GetSubject());

	EXPECT_STREQ(_T("7c3cbfe13a929d2291a574dca45e4fd2d2ac1aa6"), revloglist[5].m_CommitHash.ToString());
	EXPECT_STREQ(_T("refs/heads/master@{5}"), revloglist[5].m_Ref);
	EXPECT_STREQ(_T("fetch origin +refs/heads/*:refs/heads/*"), revloglist[5].m_RefAction);
	//EXPECT_STREQ(_T("Dummy author"), revloglist[5].GetCommitterName());
	//EXPECT_STREQ(_T("a@example.com"), revloglist[5].GetCommitterEmail());
	EXPECT_STREQ(_T("2015-03-16 12:52:10"), revloglist[5].GetCommitterDate().FormatGmt(L"%Y-%m-%d %H:%M:%S"));
	EXPECT_STREQ(_T("forced-update"), revloglist[5].GetSubject());

	EXPECT_STREQ(_T("560deea87853158b22d0c0fd73f60a458d47838a"), revloglist[6].m_CommitHash.ToString());
	EXPECT_STREQ(_T("refs/heads/master@{6}"), revloglist[6].m_Ref);
	EXPECT_STREQ(_T("push"), revloglist[6].m_RefAction);
	//EXPECT_STREQ(_T("Sven Strickroth"), revloglist[6].GetCommitterName());
	//EXPECT_STREQ(_T("email@cs-ware.de"), revloglist[6].GetCommitterEmail());
	EXPECT_STREQ(_T("2015-03-16 12:51:40"), revloglist[6].GetCommitterDate().FormatGmt(L"%Y-%m-%d %H:%M:%S"));
	EXPECT_STREQ(_T(""), revloglist[6].GetSubject());
}

TEST_P(GitRevLoglist2CBasicGitWithTestRepoFixture, GetReflog)
{
	GetReflog();
}

TEST_P(GitRevLoglist2CBasicGitWithTestRepoBareFixture, GetReflog)
{
	GetReflog();
}
