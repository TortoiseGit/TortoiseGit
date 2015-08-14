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
	EXPECT_EQ(0, rev.GetCommit(_T("HEAD")));
	EXPECT_STREQ(_T("7c3cbfe13a929d2291a574dca45e4fd2d2ac1aa6"), rev.m_CommitHash.ToString());
	EXPECT_STREQ(_T("Sven Strickroth"), rev.GetAuthorName());
	EXPECT_STREQ(_T("email@cs-ware.de"), rev.GetAuthorEmail());
	EXPECT_STREQ(_T("2015-03-07 18:03:58"), rev.GetAuthorDate().FormatGmt(L"%Y-%m-%d %H:%M:%S"));
	EXPECT_STREQ(_T("Sven Strickroth"), rev.GetCommitterName());
	EXPECT_STREQ(_T("email@cs-ware.de"), rev.GetCommitterEmail());
	EXPECT_STREQ(_T("2015-03-07 18:03:58"), rev.GetCommitterDate().FormatGmt(L"%Y-%m-%d %H:%M:%S"));
	EXPECT_STREQ(_T("Changed ASCII file"), rev.GetSubject());
	EXPECT_STREQ(_T(""), rev.GetBody());
	EXPECT_TRUE(rev.GetLastErr().IsEmpty());
	EXPECT_EQ(0, rev.ParentsCount());
	EXPECT_EQ(0, rev.GetParentFromHash(rev.m_CommitHash));
	ASSERT_EQ(1, rev.ParentsCount());
	EXPECT_STREQ(_T("1fc3c9688e27596d8717b54f2939dc951568f6cb"), rev.m_ParentHash[0].ToString());
	EXPECT_TRUE(rev.GetLastErr().IsEmpty());
	rev.Clear();
	EXPECT_EQ(0, rev.GetCommit(GitRev::GetWorkingCopy()));
	EXPECT_TRUE(rev.m_CommitHash.IsEmpty());
	EXPECT_STREQ(_T(""), rev.GetAuthorName());
	EXPECT_STREQ(_T(""), rev.GetAuthorEmail());
	EXPECT_STREQ(_T(""), rev.GetCommitterName());
	EXPECT_STREQ(_T(""), rev.GetCommitterEmail());
	EXPECT_STREQ(_T("Working Copy"), rev.GetSubject());
	EXPECT_STREQ(_T(""), rev.GetBody());
	EXPECT_EQ(0, rev.ParentsCount());
	EXPECT_TRUE(rev.GetLastErr().IsEmpty());
	rev.Clear();
	EXPECT_TRUE(rev.GetLastErr().IsEmpty());
	EXPECT_EQ(0, rev.GetCommit(_T("aa5b97f89cea6863222823c8289ce392d06d1691")));
	EXPECT_STREQ(_T("aa5b97f89cea6863222823c8289ce392d06d1691"), rev.m_CommitHash.ToString());
	EXPECT_STREQ(_T("Another dummy with ümlaut"), rev.GetAuthorName());
	EXPECT_STREQ(_T("anotherduemmy@example.com"), rev.GetAuthorEmail());
	EXPECT_STREQ(_T("2015-03-14 22:30:06"), rev.GetAuthorDate().FormatGmt(L"%Y-%m-%d %H:%M:%S"));
	EXPECT_STREQ(_T("Another dummy with ümlaut"), rev.GetCommitterName());
	EXPECT_STREQ(_T("anotherduemmy@example.com"), rev.GetCommitterEmail());
	EXPECT_STREQ(_T("2015-03-14 22:30:06"), rev.GetCommitterDate().FormatGmt(L"%Y-%m-%d %H:%M:%S"));
	EXPECT_STREQ(_T("Subject line"), rev.GetSubject());
	EXPECT_STREQ(_T("\nalso some more lines\n\nhere in body\n\nSigned-off-by: Another dummy with ümlaut <anotherduemmy@example.com>\n"), rev.GetBody());
	EXPECT_TRUE(rev.GetLastErr().IsEmpty());
	rev.Clear();
	EXPECT_TRUE(rev.m_CommitHash.IsEmpty());
	EXPECT_EQ(0, rev.GetCommit(_T("1fc3c9688e27596d8717b54f2939dc951568f6cb")));
	EXPECT_STREQ(_T("1fc3c9688e27596d8717b54f2939dc951568f6cb"), rev.m_CommitHash.ToString());
	EXPECT_STREQ(_T("Some other User"), rev.GetAuthorName());
	EXPECT_STREQ(_T("dummy@example.com"), rev.GetAuthorEmail());
	EXPECT_STREQ(_T("2015-03-07 18:03:39"), rev.GetAuthorDate().FormatGmt(L"%Y-%m-%d %H:%M:%S"));
	EXPECT_STREQ(_T("Sven Strickroth"), rev.GetCommitterName());
	EXPECT_STREQ(_T("email@cs-ware.de"), rev.GetCommitterEmail());
	EXPECT_STREQ(_T("2015-03-07 18:03:39"), rev.GetCommitterDate().FormatGmt(L"%Y-%m-%d %H:%M:%S"));
	EXPECT_STREQ(_T("Added an ascii file"), rev.GetSubject());
	EXPECT_STREQ(_T(""), rev.GetBody());
	EXPECT_TRUE(rev.GetLastErr().IsEmpty());
	rev.Clear();
	EXPECT_EQ(-1, rev.GetCommit(_T("does-not-exist")));
	EXPECT_FALSE(rev.GetLastErr().IsEmpty());
	EXPECT_TRUE(rev.m_CommitHash.IsEmpty());
	rev.Clear();
	CGitHash hash(_T("aa5b97f89cea6863222823c8289ce392d06d1691"));
	EXPECT_EQ(0, rev.GetCommitFromHash(hash));
	EXPECT_EQ(hash, rev.m_CommitHash);
	EXPECT_STREQ(_T("Another dummy with ümlaut"), rev.GetAuthorName());
	EXPECT_STREQ(_T("anotherduemmy@example.com"), rev.GetAuthorEmail());
	EXPECT_STREQ(_T("2015-03-14 22:30:06"), rev.GetAuthorDate().FormatGmt(L"%Y-%m-%d %H:%M:%S"));
	EXPECT_STREQ(_T("Another dummy with ümlaut"), rev.GetCommitterName());
	EXPECT_STREQ(_T("anotherduemmy@example.com"), rev.GetCommitterEmail());
	EXPECT_STREQ(_T("2015-03-14 22:30:06"), rev.GetCommitterDate().FormatGmt(L"%Y-%m-%d %H:%M:%S"));
	EXPECT_STREQ(_T("Subject line"), rev.GetSubject());
	EXPECT_STREQ(_T("\nalso some more lines\n\nhere in body\n\nSigned-off-by: Another dummy with ümlaut <anotherduemmy@example.com>\n"), rev.GetBody());
	EXPECT_TRUE(rev.GetLastErr().IsEmpty());
	rev.Clear();
	EXPECT_EQ(0, rev.GetCommit(_T("8d1ebbcc7eeb63af10ff8bcf7712afb9fcc90b8a")));
	EXPECT_STREQ(_T("8d1ebbcc7eeb63af10ff8bcf7712afb9fcc90b8a"), rev.m_CommitHash.ToString());
	EXPECT_STREQ(_T("Sven Strickroth"), rev.GetAuthorName());
	EXPECT_STREQ(_T("email@cs-ware.de"), rev.GetAuthorEmail());
	EXPECT_STREQ(_T("2015-03-04 17:50:24"), rev.GetAuthorDate().FormatGmt(L"%Y-%m-%d %H:%M:%S"));
	EXPECT_STREQ(_T("Sven Strickroth"), rev.GetCommitterName());
	EXPECT_STREQ(_T("email@cs-ware.de"), rev.GetCommitterEmail());
	EXPECT_STREQ(_T("2015-03-04 17:50:24"), rev.GetCommitterDate().FormatGmt(L"%Y-%m-%d %H:%M:%S"));
	EXPECT_STREQ(_T("Merge branch 'for-merge' into subdir/branch"), rev.GetSubject());
	EXPECT_STREQ(_T(""), rev.GetBody());
	EXPECT_EQ(0, rev.ParentsCount());
	EXPECT_EQ(0, rev.GetParentFromHash(rev.m_CommitHash));
	ASSERT_EQ(2, rev.ParentsCount());
	EXPECT_STREQ(_T("3686b9cf74f1a4ef96d6bfe736595ef9abf0fb8d"), rev.m_ParentHash[0].ToString());
	EXPECT_STREQ(_T("1ce788330fd3a306c8ad37654063ceee13a7f172"), rev.m_ParentHash[1].ToString());
	EXPECT_TRUE(rev.GetLastErr().IsEmpty());
	rev.Clear();
	EXPECT_TRUE(rev.m_CommitHash.IsEmpty());
	EXPECT_EQ(0, rev.GetCommit(_T("844309789a13614b52d5e7cbfe6350dd73d1dc72"))); // root-commit
	EXPECT_STREQ(_T("844309789a13614b52d5e7cbfe6350dd73d1dc72"), rev.m_CommitHash.ToString());
	EXPECT_STREQ(_T("Sven Strickroth"), rev.GetAuthorName());
	EXPECT_STREQ(_T("email@cs-ware.de"), rev.GetAuthorEmail());
	EXPECT_STREQ(_T("2015-03-04 17:35:13"), rev.GetAuthorDate().FormatGmt(L"%Y-%m-%d %H:%M:%S"));
	EXPECT_STREQ(_T("Sven Strickroth"), rev.GetCommitterName());
	EXPECT_STREQ(_T("email@cs-ware.de"), rev.GetCommitterEmail());
	EXPECT_STREQ(_T("2015-03-04 17:35:13"), rev.GetCommitterDate().FormatGmt(L"%Y-%m-%d %H:%M:%S"));
	EXPECT_STREQ(_T("added ansi file"), rev.GetSubject());
	EXPECT_STREQ(_T(""), rev.GetBody());
	EXPECT_TRUE(rev.GetLastErr().IsEmpty());
	EXPECT_EQ(0, rev.ParentsCount());
	EXPECT_EQ(0, rev.GetParentFromHash(rev.m_CommitHash));
	EXPECT_EQ(0, rev.ParentsCount());
	EXPECT_TRUE(rev.GetLastErr().IsEmpty());
	rev.Clear();
	// GPG signed commit which was also amended with different dates
	EXPECT_EQ(0, rev.GetCommit(_T("subdir/branch")));
	EXPECT_STREQ(_T("4c5c93d2a0b368bc4570d5ec02ab03b9c4334d44"), rev.m_CommitHash.ToString());
	EXPECT_STREQ(_T("Sven Strickroth"), rev.GetAuthorName());
	EXPECT_STREQ(_T("email@cs-ware.de"), rev.GetAuthorEmail());
	EXPECT_STREQ(_T("2015-03-16 12:52:29"), rev.GetAuthorDate().FormatGmt(L"%Y-%m-%d %H:%M:%S"));
	EXPECT_STREQ(_T("Sven Strickroth"), rev.GetCommitterName());
	EXPECT_STREQ(_T("email@cs-ware.de"), rev.GetCommitterEmail());
	EXPECT_STREQ(_T("2015-03-16 13:06:08"), rev.GetCommitterDate().FormatGmt(L"%Y-%m-%d %H:%M:%S"));
	EXPECT_STREQ(_T("Several actions"), rev.GetSubject());
	EXPECT_STREQ(_T("\n* amended with different date\n* make utf16-be-nobom.txt a symlink ti ascii.txt\n* remove utf8-bom.txt\n* Copied ascii.txt\n\nSigned-off-by: Sven Strickroth <email@cs-ware.de>\n"), rev.GetBody());
	EXPECT_TRUE(rev.GetLastErr().IsEmpty());
	EXPECT_EQ(0, rev.ParentsCount());
	EXPECT_EQ(0, rev.GetParentFromHash(rev.m_CommitHash));
	ASSERT_EQ(1, rev.ParentsCount());
	EXPECT_STREQ(_T("aa5b97f89cea6863222823c8289ce392d06d1691"), rev.m_ParentHash[0].ToString());
	EXPECT_TRUE(rev.GetLastErr().IsEmpty());
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
	EXPECT_STREQ(_T("HEAD"), GitRev::GetHead());
	EXPECT_STREQ(_T("0000000000000000000000000000000000000000"), GitRev::GetWorkingCopy());
}
