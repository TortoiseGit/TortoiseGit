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
	g_Git.SetConfigValue(L"branch.master.description", L"test");
	g_Git.SetConfigValue(L"branch.subdir/branch.description", L"multi\nline");

	MAP_REF_GITREVREFBROWSER refMap;
	CString err;
	EXPECT_EQ(0, GitRevRefBrowser::GetGitRevRefMap(refMap, 0, err));
	EXPECT_STREQ(L"", err);
	EXPECT_EQ(12U, refMap.size());

	GitRevRefBrowser rev = refMap[L"refs/heads/master"];
	EXPECT_STREQ(L"7c3cbfe13a929d2291a574dca45e4fd2d2ac1aa6", rev.m_CommitHash.ToString());
	EXPECT_STREQ(L"Sven Strickroth", rev.GetAuthorName());
	EXPECT_STREQ(L"2015-03-07 18:03:58", rev.GetAuthorDate().FormatGmt(L"%Y-%m-%d %H:%M:%S"));
	EXPECT_STREQ(L"Changed ASCII file", rev.GetSubject());
	EXPECT_STREQ(L"refs/remotes/origin/master", rev.m_UpstreamRef);
	EXPECT_STREQ(L"test", rev.m_Description);

	rev = refMap[L"refs/heads/signed-commit"];
	EXPECT_STREQ(L"4c5c93d2a0b368bc4570d5ec02ab03b9c4334d44", rev.m_CommitHash.ToString());
	EXPECT_STREQ(L"Sven Strickroth", rev.GetAuthorName());
	EXPECT_STREQ(L"2015-03-16 12:52:29", rev.GetAuthorDate().FormatGmt(L"%Y-%m-%d %H:%M:%S"));
	EXPECT_STREQ(L"Several actions", rev.GetSubject());
	EXPECT_STREQ(L"", rev.m_UpstreamRef);
	EXPECT_STREQ(L"", rev.m_Description);

	rev = refMap[L"refs/tags/also-signed"];
	EXPECT_STREQ(L"e89cb722e0f9b2eb763bb059dc099ee6c502a6d8", rev.m_CommitHash.ToString());
	EXPECT_STREQ(L"Sven Strickroth", rev.GetAuthorName());
	EXPECT_STREQ(L"2015-03-04 17:45:40", rev.GetAuthorDate().FormatGmt(L"%Y-%m-%d %H:%M:%S"));
	EXPECT_STREQ(L"Also signed", rev.GetSubject());
	EXPECT_STREQ(L"", rev.m_UpstreamRef);
	EXPECT_STREQ(L"", rev.m_Description);

	rev = refMap[L"refs/heads/subdir/branch"];
	EXPECT_STREQ(L"31ff87c86e9f6d3853e438cb151043f30f09029a", rev.m_CommitHash.ToString());
	EXPECT_STREQ(L"Sven Strickroth", rev.GetAuthorName());
	EXPECT_STREQ(L"2015-03-16 12:52:29", rev.GetAuthorDate().FormatGmt(L"%Y-%m-%d %H:%M:%S")); // used here, because author and commit time differ
	EXPECT_STREQ(L"Several actions", rev.GetSubject());
	EXPECT_STREQ(L"", rev.m_UpstreamRef);
	EXPECT_STREQ(L"multi\nline", rev.m_Description);

	refMap.clear();
	EXPECT_EQ(0, GitRevRefBrowser::GetGitRevRefMap(refMap, 0, err, [](const CString& refName) { return CStringUtils::StartsWith(refName, L"refs/heads/"); }));
	EXPECT_STREQ(L"", err);
	EXPECT_EQ(6U, refMap.size());
	EXPECT_TRUE(refMap.find(L"refs/heads/master") != refMap.end());
	for (auto it = refMap.cbegin(); it != refMap.cend(); ++it)
		EXPECT_TRUE(CStringUtils::StartsWith(it->first, L"refs/heads/"));

	refMap.clear();
	EXPECT_EQ(0, GitRevRefBrowser::GetGitRevRefMap(refMap, 1, err));
	EXPECT_STREQ(L"", err);
	EXPECT_EQ(6U, refMap.size());
	for (const auto& branch : { L"refs/heads/master", L"refs/heads/master2", L"refs/remotes/origin/master", L"refs/tags/all-files-signed", L"refs/tags/also-signed", L"refs/tags/normal-tag" })
		EXPECT_TRUE(refMap.find(branch) != refMap.end());

	refMap.clear();
	EXPECT_EQ(0, GitRevRefBrowser::GetGitRevRefMap(refMap, 2, err));
	EXPECT_STREQ(L"", err);
	EXPECT_EQ(6U, refMap.size());
	EXPECT_TRUE(refMap.find(L"refs/heads/master") == refMap.end());
	for (const auto& branch : { L"refs/heads/forconflict", L"refs/heads/signed-commit", L"refs/heads/simple-conflict", L"refs/heads/subdir/branch", L"refs/notes/commits", L"refs/stash" })
		EXPECT_TRUE(refMap.find(branch) != refMap.end());
}

TEST_P(GitRevRefBrowserCBasicGitWithTestRepoFixture, GetGitRevRefMap)
{
	GetGitRevRefMap();
}

TEST_P(GitRevRefBrowserCBasicGitWithTestRepoBareFixture, GetGitRevRefMap)
{
	GetGitRevRefMap();
}
