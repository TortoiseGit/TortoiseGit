// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2015-2017, 2019 - TortoiseGit

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
#include "GitRev.h"

class GitRevCBasicGitWithTestRepoFixture : public CBasicGitWithTestRepoFixture
{
};

class GitRevCBasicGitWithTestRepoBareFixture : public CBasicGitWithTestRepoBareFixture
{
};

INSTANTIATE_TEST_CASE_P(GitRev, GitRevCBasicGitWithTestRepoFixture, testing::Values(LIBGIT, LIBGIT2, LIBGIT2_ALL));
INSTANTIATE_TEST_CASE_P(GitRev, GitRevCBasicGitWithTestRepoBareFixture, testing::Values(LIBGIT, LIBGIT2, LIBGIT2_ALL));

static void GetRevParsingTests()
{
	GitRev rev;
	EXPECT_TRUE(rev.m_CommitHash.IsEmpty());
	EXPECT_EQ(0, rev.GetCommit(L"HEAD"));
	EXPECT_STREQ(L"7c3cbfe13a929d2291a574dca45e4fd2d2ac1aa6", rev.m_CommitHash.ToString());
	EXPECT_STREQ(L"Sven Strickroth", rev.GetAuthorName());
	EXPECT_STREQ(L"email@cs-ware.de", rev.GetAuthorEmail());
	EXPECT_STREQ(L"2015-03-07 18:03:58", rev.GetAuthorDate().FormatGmt(L"%Y-%m-%d %H:%M:%S"));
	EXPECT_STREQ(L"Sven Strickroth", rev.GetCommitterName());
	EXPECT_STREQ(L"email@cs-ware.de", rev.GetCommitterEmail());
	EXPECT_STREQ(L"2015-03-07 18:03:58", rev.GetCommitterDate().FormatGmt(L"%Y-%m-%d %H:%M:%S"));
	EXPECT_STREQ(L"Changed ASCII file", rev.GetSubject());
	EXPECT_STREQ(L"", rev.GetBody());
	EXPECT_STREQ(L"", rev.GetLastErr());
	EXPECT_EQ(0, rev.ParentsCount());
	EXPECT_EQ(0, rev.GetParentFromHash(rev.m_CommitHash));
	ASSERT_EQ(1, rev.ParentsCount());
	EXPECT_STREQ(L"1fc3c9688e27596d8717b54f2939dc951568f6cb", rev.m_ParentHash[0].ToString());
	EXPECT_STREQ(L"", rev.GetLastErr());
	rev.Clear();
	EXPECT_EQ(0, rev.GetCommit(GitRev::GetWorkingCopy()));
	EXPECT_TRUE(rev.m_CommitHash.IsEmpty());
	EXPECT_STREQ(L"", rev.GetAuthorName());
	EXPECT_STREQ(L"", rev.GetAuthorEmail());
	EXPECT_STREQ(L"", rev.GetCommitterName());
	EXPECT_STREQ(L"", rev.GetCommitterEmail());
	EXPECT_STREQ(L"Working Tree", rev.GetSubject());
	EXPECT_STREQ(L"", rev.GetBody());
	EXPECT_EQ(0, rev.ParentsCount());
	EXPECT_STREQ(L"", rev.GetLastErr());
	rev.Clear();
	EXPECT_STREQ(L"", rev.GetLastErr());
	EXPECT_EQ(0, rev.GetCommit(L"aa5b97f89cea6863222823c8289ce392d06d1691"));
	EXPECT_STREQ(L"aa5b97f89cea6863222823c8289ce392d06d1691", rev.m_CommitHash.ToString());
	EXPECT_STREQ(L"Another dummy with ümlaut", rev.GetAuthorName());
	EXPECT_STREQ(L"anotherduemmy@example.com", rev.GetAuthorEmail());
	EXPECT_STREQ(L"2015-03-14 22:30:06", rev.GetAuthorDate().FormatGmt(L"%Y-%m-%d %H:%M:%S"));
	EXPECT_STREQ(L"Another dummy with ümlaut", rev.GetCommitterName());
	EXPECT_STREQ(L"anotherduemmy@example.com", rev.GetCommitterEmail());
	EXPECT_STREQ(L"2015-03-14 22:30:06", rev.GetCommitterDate().FormatGmt(L"%Y-%m-%d %H:%M:%S"));
	EXPECT_STREQ(L"Subject line", rev.GetSubject());
	EXPECT_STREQ(L"\nalso some more lines\n\nhere in body\n\nSigned-off-by: Another dummy with ümlaut <anotherduemmy@example.com>\n", rev.GetBody());
	EXPECT_STREQ(L"", rev.GetLastErr());
	rev.Clear();
	EXPECT_TRUE(rev.m_CommitHash.IsEmpty());
	EXPECT_EQ(0, rev.GetCommit(L"1fc3c9688e27596d8717b54f2939dc951568f6cb"));
	EXPECT_STREQ(L"1fc3c9688e27596d8717b54f2939dc951568f6cb", rev.m_CommitHash.ToString());
	EXPECT_STREQ(L"Some other User", rev.GetAuthorName());
	EXPECT_STREQ(L"dummy@example.com", rev.GetAuthorEmail());
	EXPECT_STREQ(L"2015-03-07 18:03:39", rev.GetAuthorDate().FormatGmt(L"%Y-%m-%d %H:%M:%S"));
	EXPECT_STREQ(L"Sven Strickroth", rev.GetCommitterName());
	EXPECT_STREQ(L"email@cs-ware.de", rev.GetCommitterEmail());
	EXPECT_STREQ(L"2015-03-07 18:03:39", rev.GetCommitterDate().FormatGmt(L"%Y-%m-%d %H:%M:%S"));
	EXPECT_STREQ(L"Added an ascii file", rev.GetSubject());
	EXPECT_STREQ(L"", rev.GetBody());
	EXPECT_STREQ(L"", rev.GetLastErr());
	rev.Clear();
	EXPECT_EQ(-1, rev.GetCommit(L"does-not-exist"));
	EXPECT_STRNE(L"", rev.GetLastErr());
	EXPECT_TRUE(rev.m_CommitHash.IsEmpty());
	rev.Clear();
	CGitHash hash(CGitHash::FromHexStr(L"aa5b97f89cea6863222823c8289ce392d06d1691"));
	EXPECT_EQ(0, rev.GetCommitFromHash(hash));
	EXPECT_EQ(hash, rev.m_CommitHash);
	EXPECT_STREQ(L"Another dummy with ümlaut", rev.GetAuthorName());
	EXPECT_STREQ(L"anotherduemmy@example.com", rev.GetAuthorEmail());
	EXPECT_STREQ(L"2015-03-14 22:30:06", rev.GetAuthorDate().FormatGmt(L"%Y-%m-%d %H:%M:%S"));
	EXPECT_STREQ(L"Another dummy with ümlaut", rev.GetCommitterName());
	EXPECT_STREQ(L"anotherduemmy@example.com", rev.GetCommitterEmail());
	EXPECT_STREQ(L"2015-03-14 22:30:06", rev.GetCommitterDate().FormatGmt(L"%Y-%m-%d %H:%M:%S"));
	EXPECT_STREQ(L"Subject line", rev.GetSubject());
	EXPECT_STREQ(L"\nalso some more lines\n\nhere in body\n\nSigned-off-by: Another dummy with ümlaut <anotherduemmy@example.com>\n", rev.GetBody());
	EXPECT_STREQ(L"", rev.GetLastErr());
	rev.Clear();
	EXPECT_EQ(0, rev.GetCommit(L"8d1ebbcc7eeb63af10ff8bcf7712afb9fcc90b8a"));
	EXPECT_STREQ(L"8d1ebbcc7eeb63af10ff8bcf7712afb9fcc90b8a", rev.m_CommitHash.ToString());
	EXPECT_STREQ(L"Sven Strickroth", rev.GetAuthorName());
	EXPECT_STREQ(L"email@cs-ware.de", rev.GetAuthorEmail());
	EXPECT_STREQ(L"2015-03-04 17:50:24", rev.GetAuthorDate().FormatGmt(L"%Y-%m-%d %H:%M:%S"));
	EXPECT_STREQ(L"Sven Strickroth", rev.GetCommitterName());
	EXPECT_STREQ(L"email@cs-ware.de", rev.GetCommitterEmail());
	EXPECT_STREQ(L"2015-03-04 17:50:24", rev.GetCommitterDate().FormatGmt(L"%Y-%m-%d %H:%M:%S"));
	EXPECT_STREQ(L"Merge branch 'for-merge' into subdir/branch", rev.GetSubject());
	EXPECT_STREQ(L"", rev.GetBody());
	EXPECT_EQ(0, rev.ParentsCount());
	EXPECT_EQ(0, rev.GetParentFromHash(rev.m_CommitHash));
	ASSERT_EQ(2, rev.ParentsCount());
	EXPECT_STREQ(L"3686b9cf74f1a4ef96d6bfe736595ef9abf0fb8d", rev.m_ParentHash[0].ToString());
	EXPECT_STREQ(L"1ce788330fd3a306c8ad37654063ceee13a7f172", rev.m_ParentHash[1].ToString());
	EXPECT_STREQ(L"", rev.GetLastErr());
	rev.Clear();
	EXPECT_TRUE(rev.m_CommitHash.IsEmpty());
	EXPECT_EQ(0, rev.GetCommit(L"844309789a13614b52d5e7cbfe6350dd73d1dc72")); // root-commit
	EXPECT_STREQ(L"844309789a13614b52d5e7cbfe6350dd73d1dc72", rev.m_CommitHash.ToString());
	EXPECT_STREQ(L"Sven Strickroth", rev.GetAuthorName());
	EXPECT_STREQ(L"email@cs-ware.de", rev.GetAuthorEmail());
	EXPECT_STREQ(L"2015-03-04 17:35:13", rev.GetAuthorDate().FormatGmt(L"%Y-%m-%d %H:%M:%S"));
	EXPECT_STREQ(L"Sven Strickroth", rev.GetCommitterName());
	EXPECT_STREQ(L"email@cs-ware.de", rev.GetCommitterEmail());
	EXPECT_STREQ(L"2015-03-04 17:35:13", rev.GetCommitterDate().FormatGmt(L"%Y-%m-%d %H:%M:%S"));
	EXPECT_STREQ(L"added ansi file", rev.GetSubject());
	EXPECT_STREQ(L"", rev.GetBody());
	EXPECT_STREQ(L"", rev.GetLastErr());
	EXPECT_EQ(0, rev.ParentsCount());
	EXPECT_EQ(0, rev.GetParentFromHash(rev.m_CommitHash));
	EXPECT_EQ(0, rev.ParentsCount());
	EXPECT_STREQ(L"", rev.GetLastErr());
	rev.Clear();
	// GPG signed commit which was also amended with different dates
	EXPECT_EQ(0, rev.GetCommit(L"signed-commit"));
	EXPECT_STREQ(L"4c5c93d2a0b368bc4570d5ec02ab03b9c4334d44", rev.m_CommitHash.ToString());
	EXPECT_STREQ(L"Sven Strickroth", rev.GetAuthorName());
	EXPECT_STREQ(L"email@cs-ware.de", rev.GetAuthorEmail());
	EXPECT_STREQ(L"2015-03-16 12:52:29", rev.GetAuthorDate().FormatGmt(L"%Y-%m-%d %H:%M:%S"));
	EXPECT_STREQ(L"Sven Strickroth", rev.GetCommitterName());
	EXPECT_STREQ(L"email@cs-ware.de", rev.GetCommitterEmail());
	EXPECT_STREQ(L"2015-03-16 13:06:08", rev.GetCommitterDate().FormatGmt(L"%Y-%m-%d %H:%M:%S"));
	EXPECT_STREQ(L"Several actions", rev.GetSubject());
	EXPECT_STREQ(L"\n* amended with different date\n* make utf16-be-nobom.txt a symlink ti ascii.txt\n* remove utf8-bom.txt\n* Copied ascii.txt\n\nSigned-off-by: Sven Strickroth <email@cs-ware.de>\n", rev.GetBody());
	EXPECT_STREQ(L"", rev.GetLastErr());
	EXPECT_EQ(0, rev.ParentsCount());
	EXPECT_EQ(0, rev.GetParentFromHash(rev.m_CommitHash));
	ASSERT_EQ(1, rev.ParentsCount());
	EXPECT_STREQ(L"aa5b97f89cea6863222823c8289ce392d06d1691", rev.m_ParentHash[0].ToString());
	EXPECT_STREQ(L"", rev.GetLastErr());
	rev.Clear();
	// commit with different committer
	EXPECT_EQ(0, rev.GetCommit(L"subdir/branch"));
	EXPECT_STREQ(L"31ff87c86e9f6d3853e438cb151043f30f09029a", rev.m_CommitHash.ToString());
	EXPECT_STREQ(L"Sven Strickroth", rev.GetAuthorName());
	EXPECT_STREQ(L"email@cs-ware.de", rev.GetAuthorEmail());
	EXPECT_STREQ(L"2015-03-16 12:52:29", rev.GetAuthorDate().FormatGmt(L"%Y-%m-%d %H:%M:%S"));
	EXPECT_STREQ(L"a", rev.GetCommitterName());
	EXPECT_STREQ(L"a@a.com", rev.GetCommitterEmail());
	EXPECT_STREQ(L"2017-07-29 15:05:49", rev.GetCommitterDate().FormatGmt(L"%Y-%m-%d %H:%M:%S"));
	EXPECT_STREQ(L"Several actions", rev.GetSubject());
	EXPECT_STREQ(L"\n* amended with different date\n* make utf16-be-nobom.txt a symlink ti ascii.txt\n* remove utf8-bom.txt\n* Copied ascii.txt\n\nSigned-off-by: Sven Strickroth <email@cs-ware.de>\n", rev.GetBody());
	EXPECT_STREQ(L"", rev.GetLastErr());
	EXPECT_EQ(0, rev.ParentsCount());
	EXPECT_EQ(0, rev.GetParentFromHash(rev.m_CommitHash));
	ASSERT_EQ(1, rev.ParentsCount());
	EXPECT_STREQ(L"aa5b97f89cea6863222823c8289ce392d06d1691", rev.m_ParentHash[0].ToString());
	EXPECT_STREQ(L"", rev.GetLastErr());
}

TEST_P(GitRevCBasicGitWithTestRepoFixture, GitRevParsing)
{
	GetRevParsingTests();
}

TEST_P(GitRevCBasicGitWithTestRepoBareFixture, GitRevParsing)
{
	GetRevParsingTests();
}

TEST(GitRev, Constants)
{
	EXPECT_STREQ(L"HEAD", GitRev::GetHead());
	EXPECT_STREQ(L"0000000000000000000000000000000000000000", GitRev::GetWorkingCopy());
}
