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
#include "GitRevRefBrowser.h"

class GitRevRefBrowserCBasicGitWithTestRepoFixture : public CBasicGitWithTestRepoFixture
{
};

class GitRevRefBrowserCBasicGitWithTestRepoBareFixture : public CBasicGitWithTestRepoBareFixture
{
};

INSTANTIATE_TEST_CASE_P(GitRevRefBrowser, GitRevRefBrowserCBasicGitWithTestRepoFixture, testing::Values(GIT_CLI));
INSTANTIATE_TEST_CASE_P(GitRevRefBrowser, GitRevRefBrowserCBasicGitWithTestRepoBareFixture, testing::Values(GIT_CLI));

static void GetGitRevRefMap()
{
	g_Git.SetConfigValue(_T("branch.master.description"), _T("test"));
	g_Git.SetConfigValue(_T("branch.subdir/branch.description"), _T("multi\nline"));

	MAP_REF_GITREVREFBROWSER refMap;
	CString err;
	EXPECT_EQ(0, GitRevRefBrowser::GetGitRevRefMap(refMap, err));
	EXPECT_TRUE(err.IsEmpty());
	EXPECT_EQ(11, refMap.size());

	GitRevRefBrowser rev = refMap[L"refs/heads/master"];
	EXPECT_STREQ(_T("7c3cbfe13a929d2291a574dca45e4fd2d2ac1aa6"), rev.m_CommitHash.ToString());
	EXPECT_STREQ(_T("Sven Strickroth"), rev.GetAuthorName());
	EXPECT_STREQ(_T("2015-03-07 18:03:58"), rev.GetAuthorDate().FormatGmt(L"%Y-%m-%d %H:%M:%S"));
	EXPECT_STREQ(_T("Changed ASCII file"), rev.GetSubject());
	EXPECT_STREQ(_T("refs/remotes/origin/master"), rev.m_UpstreamRef);
	EXPECT_STREQ(_T("test"), rev.m_Description);

	rev = refMap[L"refs/heads/subdir/branch"];
	EXPECT_STREQ(_T("4c5c93d2a0b368bc4570d5ec02ab03b9c4334d44"), rev.m_CommitHash.ToString());
	EXPECT_STREQ(_T("Sven Strickroth"), rev.GetAuthorName());
	EXPECT_STREQ(_T("2015-03-16 12:52:29"), rev.GetAuthorDate().FormatGmt(L"%Y-%m-%d %H:%M:%S"));
	EXPECT_STREQ(_T("Several actions"), rev.GetSubject());
	EXPECT_STREQ(_T(""), rev.m_UpstreamRef);
	EXPECT_STREQ(_T("multi\nline"), rev.m_Description);

	rev = refMap[L"refs/tags/also-signed"];
	EXPECT_STREQ(_T("e89cb722e0f9b2eb763bb059dc099ee6c502a6d8"), rev.m_CommitHash.ToString());
	EXPECT_STREQ(_T(""), rev.GetAuthorName());
	EXPECT_TRUE(rev.GetAuthorDate() == 0);
	EXPECT_STREQ(_T("Also signed"), rev.GetSubject());
	EXPECT_STREQ(_T(""), rev.m_UpstreamRef);
	EXPECT_STREQ(_T(""), rev.m_Description);

	refMap.clear();
	EXPECT_EQ(0, GitRevRefBrowser::GetGitRevRefMap(refMap, err, [](const CString& refName) { return wcsncmp(refName, L"refs/heads/", 11) == 0; }));
	EXPECT_TRUE(err.IsEmpty());
	EXPECT_EQ(5, refMap.size());
	EXPECT_TRUE(refMap.find(L"refs/heads/master") != refMap.end());
}

TEST_P(GitRevRefBrowserCBasicGitWithTestRepoFixture, GetGitRevRefMap)
{
	GetGitRevRefMap();
}

TEST_P(GitRevRefBrowserCBasicGitWithTestRepoBareFixture, GetGitRevRefMap)
{
	GetGitRevRefMap();
}
