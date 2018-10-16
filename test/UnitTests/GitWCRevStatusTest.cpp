// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2018 - TortoiseGit

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
#include "StringUtils.h"
#include "status.h"

class GitWCRevStatusCBasicGitWithTestRepoFixture : public CBasicGitWithTestRepoFixture
{
};

class GitWCRevStatusCBasicGitWithEmptyRepositoryFixture : public CBasicGitWithEmptyRepositoryFixture
{
};

class GitWCRevStatusCBasicGitWithTestBareRepositoryFixture : public CBasicGitWithTestRepoBareFixture
{
};

INSTANTIATE_TEST_CASE_P(GitWCRevStatus, GitWCRevStatusCBasicGitWithTestRepoFixture, testing::Values(LIBGIT2));
INSTANTIATE_TEST_CASE_P(GitWCRevStatus, GitWCRevStatusCBasicGitWithEmptyRepositoryFixture, testing::Values(LIBGIT2));
INSTANTIATE_TEST_CASE_P(GitWCRevStatus, GitWCRevStatusCBasicGitWithTestBareRepositoryFixture, testing::Values(LIBGIT2));

TEST(GitWCRevStatus, NotExists)
{
	GitWCRev_t GitStat;
	EXPECT_EQ(ERR_NOWC, GetStatus(L"C:\\Windows\\does-not-exist", GitStat));
}

TEST(GitWCRevStatus, NoWorkingTree)
{
	GitWCRev_t GitStat;
	EXPECT_EQ(ERR_NOWC, GetStatus(L"C:\\Windows", GitStat));
}

TEST(GitWCRevStatus, EmptyBareRepo)
{
	CAutoTempDir tempdir;

	git_repository_init_options options = GIT_REPOSITORY_INIT_OPTIONS_INIT;
	options.flags = GIT_REPOSITORY_INIT_MKPATH | GIT_REPOSITORY_INIT_BARE | GIT_REPOSITORY_INIT_NO_DOTGIT_DIR;
	CAutoRepository repo;
	ASSERT_EQ(0, git_repository_init_ext(repo.GetPointer(), CUnicodeUtils::GetUTF8(tempdir.GetTempDir()), &options));

	GitWCRev_t GitStat;
	EXPECT_EQ(ERR_NOWC, GetStatus(tempdir.GetTempDir(), GitStat));
}

TEST_P(GitWCRevStatusCBasicGitWithTestBareRepositoryFixture, BareRepo)
{
	GitWCRev_t GitStat;
	EXPECT_EQ(ERR_NOWC, GetStatus(m_Dir.GetTempDir(), GitStat));
}

TEST_P(GitWCRevStatusCBasicGitWithEmptyRepositoryFixture, EmptyRepo)
{
	GitWCRev_t GitStat;
	EXPECT_EQ(0, GetStatus(m_Dir.GetTempDir(), GitStat));
	EXPECT_TRUE(GitStat.bIsGitItem);
	EXPECT_TRUE(GitStat.bIsUnborn);
	EXPECT_STREQ("master", GitStat.CurrentBranch.c_str());
	EXPECT_FALSE(GitStat.bHasSubmodule);
	EXPECT_FALSE(GitStat.HasMods);
	EXPECT_FALSE(GitStat.HasUnversioned);
	EXPECT_EQ(0u, GitStat.NumCommits);
	EXPECT_FALSE(GitStat.bHasSubmoduleMods);
	EXPECT_FALSE(GitStat.bHasSubmoduleNewCommits);
	EXPECT_FALSE(GitStat.bHasSubmoduleUnversioned);
	EXPECT_FALSE(GitStat.bIsTagged);
	EXPECT_STREQ(GIT_REV_ZERO_C, GitStat.HeadHashReadable);
	EXPECT_STREQ("", GitStat.HeadAuthor.c_str());
}

TEST_P(GitWCRevStatusCBasicGitWithEmptyRepositoryFixture, EmptyRepoFile)
{
	CString file = m_Dir.GetTempDir() + L"\\somefile.txt";

	GitWCRev_t GitStat;
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(file, L"something\n"));
	EXPECT_EQ(0, GetStatus(m_Dir.GetTempDir(), GitStat));
	EXPECT_TRUE(GitStat.bIsGitItem);
	EXPECT_TRUE(GitStat.bIsUnborn);
	EXPECT_STREQ("master", GitStat.CurrentBranch.c_str());
	EXPECT_FALSE(GitStat.bHasSubmodule);
	EXPECT_FALSE(GitStat.HasMods);
	EXPECT_TRUE(GitStat.HasUnversioned);
	EXPECT_EQ(0u, GitStat.NumCommits);
	EXPECT_FALSE(GitStat.bHasSubmoduleMods);
	EXPECT_FALSE(GitStat.bHasSubmoduleNewCommits);
	EXPECT_FALSE(GitStat.bHasSubmoduleUnversioned);
	EXPECT_FALSE(GitStat.bIsTagged);
	EXPECT_STREQ(GIT_REV_ZERO_C, GitStat.HeadHashReadable);
	EXPECT_STREQ("", GitStat.HeadAuthor.c_str());

	GitWCRev_t GitStat2;
	EXPECT_EQ(0, GetStatus(file, GitStat2));
	EXPECT_FALSE(GitStat2.bIsGitItem);
	EXPECT_TRUE(GitStat2.bIsUnborn);
	EXPECT_STREQ("master", GitStat2.CurrentBranch.c_str());
	EXPECT_FALSE(GitStat2.bHasSubmodule);
	EXPECT_FALSE(GitStat2.HasMods);
	EXPECT_TRUE(GitStat2.HasUnversioned);
	EXPECT_EQ(0u, GitStat2.NumCommits);
	EXPECT_FALSE(GitStat2.bHasSubmoduleMods);
	EXPECT_FALSE(GitStat2.bHasSubmoduleNewCommits);
	EXPECT_FALSE(GitStat2.bHasSubmoduleUnversioned);
	EXPECT_FALSE(GitStat2.bIsTagged);
	EXPECT_STREQ(GIT_REV_ZERO_C, GitStat2.HeadHashReadable);
	EXPECT_STREQ("", GitStat2.HeadAuthor.c_str());
}

TEST_P(GitWCRevStatusCBasicGitWithEmptyRepositoryFixture, EmptyRepoStagedFile)
{
	CString file = m_Dir.GetTempDir() + L"\\somefile.txt";

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(file, L"something\n"));

	CAutoRepository repo;
	ASSERT_EQ(0, git_repository_open(repo.GetPointer(), CUnicodeUtils::GetUTF8(m_Dir.GetTempDir())));
	CAutoIndex index;
	ASSERT_EQ(0, git_repository_index(index.GetPointer(), repo));
	ASSERT_EQ(0, git_index_add_bypath(index, "somefile.txt"));
	ASSERT_EQ(0, git_index_write(index));

	GitWCRev_t GitStat3;
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(file, L"something\n"));
	EXPECT_EQ(0, GetStatus(m_Dir.GetTempDir(), GitStat3));
	EXPECT_TRUE(GitStat3.bIsGitItem);
	EXPECT_TRUE(GitStat3.bIsUnborn);
	EXPECT_STREQ("master", GitStat3.CurrentBranch.c_str());
	EXPECT_FALSE(GitStat3.bHasSubmodule);
	EXPECT_FALSE(GitStat3.HasMods);
	EXPECT_TRUE(GitStat3.HasUnversioned);
	EXPECT_EQ(0u, GitStat3.NumCommits);
	EXPECT_FALSE(GitStat3.bHasSubmoduleMods);
	EXPECT_FALSE(GitStat3.bHasSubmoduleNewCommits);
	EXPECT_FALSE(GitStat3.bHasSubmoduleUnversioned);
	EXPECT_FALSE(GitStat3.bIsTagged);
	EXPECT_STREQ(GIT_REV_ZERO_C, GitStat3.HeadHashReadable);
	EXPECT_STREQ("", GitStat3.HeadAuthor.c_str());

	GitWCRev_t GitStat4;
	EXPECT_EQ(0, GetStatus(file, GitStat4));
	EXPECT_FALSE(GitStat4.bIsGitItem);
	EXPECT_TRUE(GitStat4.bIsUnborn);
	EXPECT_STREQ("master", GitStat4.CurrentBranch.c_str());
	EXPECT_FALSE(GitStat4.bHasSubmodule);
	EXPECT_FALSE(GitStat4.HasMods);
	EXPECT_TRUE(GitStat4.HasUnversioned);
	EXPECT_EQ(0u, GitStat4.NumCommits);
	EXPECT_FALSE(GitStat4.bHasSubmoduleMods);
	EXPECT_FALSE(GitStat4.bHasSubmoduleNewCommits);
	EXPECT_FALSE(GitStat4.bHasSubmoduleUnversioned);
	EXPECT_FALSE(GitStat4.bIsTagged);
	EXPECT_STREQ(GIT_REV_ZERO_C, GitStat4.HeadHashReadable);
	EXPECT_STREQ("", GitStat4.HeadAuthor.c_str());
}

TEST_P(GitWCRevStatusCBasicGitWithEmptyRepositoryFixture, EmptyRepoFolder)
{
	CString folder = m_Dir.GetTempDir() + L"\\folder";
	GitWCRev_t GitStat;
	ASSERT_TRUE(CreateDirectory(folder, nullptr));
	EXPECT_EQ(0, GetStatus(m_Dir.GetTempDir(), GitStat));
	EXPECT_TRUE(GitStat.bIsGitItem);
	EXPECT_TRUE(GitStat.bIsUnborn);
	EXPECT_STREQ("master", GitStat.CurrentBranch.c_str());
	EXPECT_FALSE(GitStat.bHasSubmodule);
	EXPECT_FALSE(GitStat.HasMods);
	EXPECT_FALSE(GitStat.HasUnversioned);
	EXPECT_EQ(0u, GitStat.NumCommits);
	EXPECT_FALSE(GitStat.bHasSubmoduleMods);
	EXPECT_FALSE(GitStat.bHasSubmoduleNewCommits);
	EXPECT_FALSE(GitStat.bHasSubmoduleUnversioned);
	EXPECT_FALSE(GitStat.bIsTagged);
	EXPECT_STREQ(GIT_REV_ZERO_C, GitStat.HeadHashReadable);
	EXPECT_STREQ("", GitStat.HeadAuthor.c_str());

	GitWCRev_t GitStat2;
	EXPECT_EQ(0, GetStatus(folder, GitStat2));
	EXPECT_FALSE(GitStat2.bIsGitItem);
	EXPECT_TRUE(GitStat2.bIsUnborn);
	EXPECT_STREQ("master", GitStat2.CurrentBranch.c_str());
	EXPECT_FALSE(GitStat2.bHasSubmodule);
	EXPECT_FALSE(GitStat2.HasMods);
	EXPECT_FALSE(GitStat2.HasUnversioned);
	EXPECT_EQ(0u, GitStat2.NumCommits);
	EXPECT_FALSE(GitStat2.bHasSubmoduleMods);
	EXPECT_FALSE(GitStat2.bHasSubmoduleNewCommits);
	EXPECT_FALSE(GitStat2.bHasSubmoduleUnversioned);
	EXPECT_FALSE(GitStat2.bIsTagged);
	EXPECT_STREQ(GIT_REV_ZERO_C, GitStat2.HeadHashReadable);
	EXPECT_STREQ("", GitStat2.HeadAuthor.c_str());
}

TEST_P(GitWCRevStatusCBasicGitWithEmptyRepositoryFixture, EmptyRepoFolderFile)
{
	CString folder = m_Dir.GetTempDir() + L"\\folder";

	GitWCRev_t GitStat;
	ASSERT_TRUE(CreateDirectory(folder, nullptr));
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(folder + L"\\somefile.txt", L"something\n"));
	EXPECT_EQ(0, GetStatus(m_Dir.GetTempDir(), GitStat));
	EXPECT_TRUE(GitStat.bIsGitItem);
	EXPECT_TRUE(GitStat.bIsUnborn);
	EXPECT_STREQ("master", GitStat.CurrentBranch.c_str());
	EXPECT_FALSE(GitStat.bHasSubmodule);
	EXPECT_FALSE(GitStat.HasMods);
	EXPECT_TRUE(GitStat.HasUnversioned);
	EXPECT_EQ(0u, GitStat.NumCommits);
	EXPECT_FALSE(GitStat.bHasSubmoduleMods);
	EXPECT_FALSE(GitStat.bHasSubmoduleNewCommits);
	EXPECT_FALSE(GitStat.bHasSubmoduleUnversioned);
	EXPECT_FALSE(GitStat.bIsTagged);
	EXPECT_STREQ(GIT_REV_ZERO_C, GitStat.HeadHashReadable);
	EXPECT_STREQ("", GitStat.HeadAuthor.c_str());

	GitWCRev_t GitStat2;
	EXPECT_EQ(0, GetStatus(folder, GitStat2));
	EXPECT_FALSE(GitStat2.bIsGitItem);
	EXPECT_TRUE(GitStat2.bIsUnborn);
	EXPECT_STREQ("master", GitStat2.CurrentBranch.c_str());
	EXPECT_FALSE(GitStat2.bHasSubmodule);
	EXPECT_FALSE(GitStat2.HasMods);
	EXPECT_TRUE(GitStat2.HasUnversioned);
	EXPECT_EQ(0u, GitStat2.NumCommits);
	EXPECT_FALSE(GitStat2.bHasSubmoduleMods);
	EXPECT_FALSE(GitStat2.bHasSubmoduleNewCommits);
	EXPECT_FALSE(GitStat2.bHasSubmoduleUnversioned);
	EXPECT_FALSE(GitStat2.bIsTagged);
	EXPECT_STREQ(GIT_REV_ZERO_C, GitStat2.HeadHashReadable);
	EXPECT_STREQ("", GitStat2.HeadAuthor.c_str());

	GitWCRev_t GitStat3;
	EXPECT_EQ(0, GetStatus(folder + L"\\somefile.txt", GitStat3));
	EXPECT_FALSE(GitStat3.bIsGitItem);
	EXPECT_TRUE(GitStat3.bIsUnborn);
	EXPECT_STREQ("master", GitStat3.CurrentBranch.c_str());
	EXPECT_FALSE(GitStat3.bHasSubmodule);
	EXPECT_FALSE(GitStat3.HasMods);
	EXPECT_TRUE(GitStat3.HasUnversioned);
	EXPECT_EQ(0u, GitStat3.NumCommits);
	EXPECT_FALSE(GitStat3.bHasSubmoduleMods);
	EXPECT_FALSE(GitStat3.bHasSubmoduleNewCommits);
	EXPECT_FALSE(GitStat3.bHasSubmoduleUnversioned);
	EXPECT_FALSE(GitStat3.bIsTagged);
	EXPECT_STREQ(GIT_REV_ZERO_C, GitStat3.HeadHashReadable);
	EXPECT_STREQ("", GitStat3.HeadAuthor.c_str());

	CString folder2 = folder + L"2";
	ASSERT_TRUE(CreateDirectory(folder2, nullptr));
	GitWCRev_t GitStat4;
	EXPECT_EQ(0, GetStatus(folder2, GitStat4));
	EXPECT_FALSE(GitStat4.bIsGitItem);
	EXPECT_TRUE(GitStat4.bIsUnborn);
	EXPECT_STREQ("master", GitStat4.CurrentBranch.c_str());
	EXPECT_FALSE(GitStat4.bHasSubmodule);
	EXPECT_FALSE(GitStat4.HasMods);
	EXPECT_FALSE(GitStat4.HasUnversioned);
	EXPECT_EQ(0u, GitStat4.NumCommits);
	EXPECT_FALSE(GitStat4.bHasSubmoduleMods);
	EXPECT_FALSE(GitStat4.bHasSubmoduleNewCommits);
	EXPECT_FALSE(GitStat4.bHasSubmoduleUnversioned);
	EXPECT_FALSE(GitStat4.bIsTagged);
	EXPECT_STREQ(GIT_REV_ZERO_C, GitStat4.HeadHashReadable);
	EXPECT_STREQ("", GitStat4.HeadAuthor.c_str());
}

TEST_P(GitWCRevStatusCBasicGitWithTestRepoFixture, Basic)
{
	GitWCRev_t GitStat;
	EXPECT_EQ(0, GetStatus(m_Dir.GetTempDir(), GitStat));
	EXPECT_TRUE(GitStat.bIsGitItem);
	EXPECT_FALSE(GitStat.bIsUnborn);
	EXPECT_STREQ("master", GitStat.CurrentBranch.c_str());
	EXPECT_FALSE(GitStat.bHasSubmodule);
	EXPECT_TRUE(GitStat.HasMods);
	EXPECT_FALSE(GitStat.HasUnversioned);
	EXPECT_EQ(12u, GitStat.NumCommits);
	EXPECT_FALSE(GitStat.bHasSubmoduleMods);
	EXPECT_FALSE(GitStat.bHasSubmoduleNewCommits);
	EXPECT_FALSE(GitStat.bHasSubmoduleUnversioned);
	EXPECT_FALSE(GitStat.bIsTagged);
	EXPECT_STREQ("7c3cbfe13a929d2291a574dca45e4fd2d2ac1aa6", GitStat.HeadHashReadable);
	EXPECT_STREQ("Sven Strickroth", GitStat.HeadAuthor.c_str());
	EXPECT_STREQ("email@cs-ware.de", GitStat.HeadEmail.c_str());

	CString output;
	EXPECT_EQ(0, m_Git.Run(L"git.exe checkout -f master2", &output, nullptr, CP_UTF8));
	EXPECT_STREQ(L"", output);

	GitWCRev_t GitStat2;
	EXPECT_EQ(0, GetStatus(m_Dir.GetTempDir(), GitStat2));
	EXPECT_TRUE(GitStat2.bIsGitItem);
	EXPECT_FALSE(GitStat2.bIsUnborn);
	EXPECT_STREQ("master2", GitStat2.CurrentBranch.c_str());
	EXPECT_FALSE(GitStat2.bHasSubmodule);
	EXPECT_FALSE(GitStat2.HasMods);
	EXPECT_FALSE(GitStat2.HasUnversioned);
	EXPECT_EQ(10u, GitStat2.NumCommits);
	EXPECT_FALSE(GitStat2.bHasSubmoduleMods);
	EXPECT_FALSE(GitStat2.bHasSubmoduleNewCommits);
	EXPECT_FALSE(GitStat2.bHasSubmoduleUnversioned);
	EXPECT_TRUE(GitStat2.bIsTagged);
	EXPECT_STREQ("49ecdfff36bfe2b9b499b33e5034f427e2fa54dd", GitStat2.HeadHashReadable);
	EXPECT_STREQ("Sven Strickroth", GitStat2.HeadAuthor.c_str());
	EXPECT_STREQ("email@cs-ware.de", GitStat2.HeadEmail.c_str());

	EXPECT_TRUE(DeleteFile(m_Dir.GetTempDir() + L"\\utf8-bom.txt"));

	GitWCRev_t GitStat3;
	EXPECT_EQ(0, GetStatus(m_Dir.GetTempDir() + L"\\ansi.txt", GitStat3));
	EXPECT_TRUE(GitStat3.bIsGitItem);
	EXPECT_FALSE(GitStat3.bIsUnborn);
	EXPECT_STREQ("master2", GitStat3.CurrentBranch.c_str());
	EXPECT_FALSE(GitStat3.bHasSubmodule);
	EXPECT_FALSE(GitStat3.HasMods);
	EXPECT_FALSE(GitStat3.HasUnversioned);
	EXPECT_EQ(10u, GitStat3.NumCommits);
	EXPECT_FALSE(GitStat3.bHasSubmoduleMods);
	EXPECT_FALSE(GitStat3.bHasSubmoduleNewCommits);
	EXPECT_FALSE(GitStat3.bHasSubmoduleUnversioned);
	EXPECT_TRUE(GitStat3.bIsTagged);
	EXPECT_STREQ("49ecdfff36bfe2b9b499b33e5034f427e2fa54dd", GitStat3.HeadHashReadable);
	EXPECT_STREQ("Sven Strickroth", GitStat3.HeadAuthor.c_str());
	EXPECT_STREQ("email@cs-ware.de", GitStat3.HeadEmail.c_str());

	GitWCRev_t GitStat4;
	EXPECT_EQ(0, GetStatus(m_Dir.GetTempDir(), GitStat4));
	EXPECT_TRUE(GitStat4.bIsGitItem);
	EXPECT_FALSE(GitStat4.bIsUnborn);
	EXPECT_STREQ("master2", GitStat4.CurrentBranch.c_str());
	EXPECT_FALSE(GitStat4.bHasSubmodule);
	EXPECT_TRUE(GitStat4.HasMods); // missing file: utf8-bom.txt
	EXPECT_FALSE(GitStat4.HasUnversioned);
	EXPECT_EQ(10u, GitStat4.NumCommits);
	EXPECT_FALSE(GitStat4.bHasSubmoduleMods);
	EXPECT_FALSE(GitStat4.bHasSubmoduleNewCommits);
	EXPECT_FALSE(GitStat4.bHasSubmoduleUnversioned);
	EXPECT_TRUE(GitStat4.bIsTagged);
	EXPECT_STREQ("49ecdfff36bfe2b9b499b33e5034f427e2fa54dd", GitStat4.HeadHashReadable);
	EXPECT_STREQ("Sven Strickroth", GitStat4.HeadAuthor.c_str());
	EXPECT_STREQ("email@cs-ware.de", GitStat4.HeadEmail.c_str());

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(m_Dir.GetTempDir() + L"\\copy\\ba.txt", L"something\n"));
	GitWCRev_t GitStat5;
	EXPECT_EQ(0, GetStatus(m_Dir.GetTempDir() + L"\\copy", GitStat5));
	EXPECT_FALSE(GitStat5.bIsGitItem);
	EXPECT_FALSE(GitStat5.bIsUnborn);
	EXPECT_STREQ("master2", GitStat5.CurrentBranch.c_str());
	EXPECT_FALSE(GitStat5.bHasSubmodule);
	EXPECT_FALSE(GitStat5.HasMods);
	EXPECT_TRUE(GitStat5.HasUnversioned);
	EXPECT_EQ(10u, GitStat5.NumCommits);
	EXPECT_FALSE(GitStat5.bHasSubmoduleMods);
	EXPECT_FALSE(GitStat5.bHasSubmoduleNewCommits);
	EXPECT_FALSE(GitStat5.bHasSubmoduleUnversioned);
	EXPECT_TRUE(GitStat5.bIsTagged);
	EXPECT_STREQ("49ecdfff36bfe2b9b499b33e5034f427e2fa54dd", GitStat5.HeadHashReadable);
	EXPECT_STREQ("Sven Strickroth", GitStat5.HeadAuthor.c_str());
	EXPECT_STREQ("email@cs-ware.de", GitStat5.HeadEmail.c_str());

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(m_Dir.GetTempDir() + L"\\copy\\aaa.txt", L"something\n"));
	GitWCRev_t GitStat6;
	EXPECT_EQ(0, GetStatus(m_Dir.GetTempDir() + L"\\copy", GitStat6));
	EXPECT_FALSE(GitStat6.bIsGitItem);
	EXPECT_FALSE(GitStat6.bIsUnborn);
	EXPECT_STREQ("master2", GitStat6.CurrentBranch.c_str());
	EXPECT_FALSE(GitStat6.bHasSubmodule);
	EXPECT_FALSE(GitStat6.HasMods);
	EXPECT_TRUE(GitStat6.HasUnversioned);
	EXPECT_EQ(10u, GitStat6.NumCommits);
	EXPECT_FALSE(GitStat6.bHasSubmoduleMods);
	EXPECT_FALSE(GitStat6.bHasSubmoduleNewCommits);
	EXPECT_FALSE(GitStat6.bHasSubmoduleUnversioned);
	EXPECT_TRUE(GitStat6.bIsTagged);
	EXPECT_STREQ("49ecdfff36bfe2b9b499b33e5034f427e2fa54dd", GitStat6.HeadHashReadable);
	EXPECT_STREQ("Sven Strickroth", GitStat6.HeadAuthor.c_str());
	EXPECT_STREQ("email@cs-ware.de", GitStat6.HeadEmail.c_str());
}

TEST_P(GitWCRevStatusCBasicGitWithTestRepoFixture, GitWCRevignore)
{
	CString output;
	EXPECT_EQ(0, m_Git.Run(L"git.exe checkout -f master", &output, nullptr, CP_UTF8));
	EXPECT_STRNE(L"", output);

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(m_Dir.GetTempDir() + L"\\.GitWCRevignore", L"\n"));

	GitWCRev_t GitStat;
	EXPECT_EQ(0, GetStatus(m_Dir.GetTempDir(), GitStat));
	EXPECT_TRUE(GitStat.bIsGitItem);
	EXPECT_FALSE(GitStat.bIsUnborn);
	EXPECT_STREQ("master", GitStat.CurrentBranch.c_str());
	EXPECT_FALSE(GitStat.bHasSubmodule);
	EXPECT_FALSE(GitStat.HasMods);
	EXPECT_TRUE(GitStat.HasUnversioned);
	EXPECT_EQ(12u, GitStat.NumCommits);
	EXPECT_FALSE(GitStat.bHasSubmoduleMods);
	EXPECT_FALSE(GitStat.bHasSubmoduleNewCommits);
	EXPECT_FALSE(GitStat.bHasSubmoduleUnversioned);
	EXPECT_FALSE(GitStat.bIsTagged);
	EXPECT_STREQ("7c3cbfe13a929d2291a574dca45e4fd2d2ac1aa6", GitStat.HeadHashReadable);
	EXPECT_STREQ("Sven Strickroth", GitStat.HeadAuthor.c_str());
	EXPECT_STREQ("email@cs-ware.de", GitStat.HeadEmail.c_str());

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(m_Dir.GetTempDir() + L"\\.GitWCRevignore", L".GitWCRevignore\n"));
	GitWCRev_t GitStat2;
	EXPECT_EQ(0, GetStatus(m_Dir.GetTempDir(), GitStat2));
	EXPECT_TRUE(GitStat2.bIsGitItem);
	EXPECT_FALSE(GitStat2.bIsUnborn);
	EXPECT_STREQ("master", GitStat2.CurrentBranch.c_str());
	EXPECT_FALSE(GitStat2.bHasSubmodule);
	EXPECT_FALSE(GitStat2.HasMods);
	EXPECT_FALSE(GitStat2.HasUnversioned);
	EXPECT_EQ(12u, GitStat2.NumCommits);
	EXPECT_FALSE(GitStat2.bHasSubmoduleMods);
	EXPECT_FALSE(GitStat2.bHasSubmoduleNewCommits);
	EXPECT_FALSE(GitStat2.bHasSubmoduleUnversioned);
	EXPECT_FALSE(GitStat2.bIsTagged);
	EXPECT_STREQ("7c3cbfe13a929d2291a574dca45e4fd2d2ac1aa6", GitStat2.HeadHashReadable);
	EXPECT_STREQ("Sven Strickroth", GitStat2.HeadAuthor.c_str());
	EXPECT_STREQ("email@cs-ware.de", GitStat2.HeadEmail.c_str());

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(m_Dir.GetTempDir() + L"\\utf8-bom.txt", L"modified!"));
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(m_Dir.GetTempDir() + L"\\.GitWCRevignore", L".GitWCRevignore\nutf8-bom.txt\n"));

	GitWCRev_t GitStat3;
	EXPECT_EQ(0, GetStatus(m_Dir.GetTempDir(), GitStat3));
	EXPECT_TRUE(GitStat3.bIsGitItem);
	EXPECT_FALSE(GitStat3.bIsUnborn);
	EXPECT_STREQ("master", GitStat3.CurrentBranch.c_str());
	EXPECT_FALSE(GitStat3.bHasSubmodule);
	EXPECT_FALSE(GitStat3.HasMods);
	EXPECT_FALSE(GitStat3.HasUnversioned);
	EXPECT_EQ(12u, GitStat3.NumCommits);
	EXPECT_FALSE(GitStat3.bHasSubmoduleMods);
	EXPECT_FALSE(GitStat3.bHasSubmoduleNewCommits);
	EXPECT_FALSE(GitStat3.bHasSubmoduleUnversioned);
	EXPECT_FALSE(GitStat3.bIsTagged);
	EXPECT_STREQ("7c3cbfe13a929d2291a574dca45e4fd2d2ac1aa6", GitStat3.HeadHashReadable);
	EXPECT_STREQ("Sven Strickroth", GitStat3.HeadAuthor.c_str());
	EXPECT_STREQ("email@cs-ware.de", GitStat3.HeadEmail.c_str());

	GitWCRev_t GitStat4;
	EXPECT_EQ(0, GetStatus(m_Dir.GetTempDir() + L"\\utf8-bom.txt", GitStat4));
	EXPECT_TRUE(GitStat4.bIsGitItem);
	EXPECT_FALSE(GitStat4.bIsUnborn);
	EXPECT_STREQ("master", GitStat4.CurrentBranch.c_str());
	EXPECT_FALSE(GitStat4.bHasSubmodule);
	EXPECT_TRUE(GitStat4.HasMods); // requesting a specific file which is also ignored, the unignoring has precedence!
	EXPECT_FALSE(GitStat4.HasUnversioned);
	EXPECT_EQ(12u, GitStat4.NumCommits);
	EXPECT_FALSE(GitStat4.bHasSubmoduleMods);
	EXPECT_FALSE(GitStat4.bHasSubmoduleNewCommits);
	EXPECT_FALSE(GitStat4.bHasSubmoduleUnversioned);
	EXPECT_FALSE(GitStat4.bIsTagged);
	EXPECT_STREQ("7c3cbfe13a929d2291a574dca45e4fd2d2ac1aa6", GitStat4.HeadHashReadable);
	EXPECT_STREQ("Sven Strickroth", GitStat4.HeadAuthor.c_str());
	EXPECT_STREQ("email@cs-ware.de", GitStat4.HeadEmail.c_str());

	GitWCRev_t GitStat5;
	EXPECT_EQ(0, GetStatus(m_Dir.GetTempDir(), GitStat5));
	EXPECT_TRUE(GitStat5.bIsGitItem);
	EXPECT_FALSE(GitStat5.bIsUnborn);
	EXPECT_STREQ("master", GitStat5.CurrentBranch.c_str());
	EXPECT_FALSE(GitStat5.bHasSubmodule);
	EXPECT_FALSE(GitStat5.HasMods); // ignore changes in modified file
	EXPECT_FALSE(GitStat5.HasUnversioned);
	EXPECT_EQ(12u, GitStat5.NumCommits);
	EXPECT_FALSE(GitStat5.bHasSubmoduleMods);
	EXPECT_FALSE(GitStat5.bHasSubmoduleNewCommits);
	EXPECT_FALSE(GitStat5.bHasSubmoduleUnversioned);
	EXPECT_FALSE(GitStat5.bIsTagged);
	EXPECT_STREQ("7c3cbfe13a929d2291a574dca45e4fd2d2ac1aa6", GitStat5.HeadHashReadable);
	EXPECT_STREQ("Sven Strickroth", GitStat5.HeadAuthor.c_str());
	EXPECT_STREQ("email@cs-ware.de", GitStat5.HeadEmail.c_str());

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(m_Dir.GetTempDir() + L"\\ansi.txt", L"modified!"));
	GitWCRev_t GitStat6;
	EXPECT_EQ(0, GetStatus(m_Dir.GetTempDir(), GitStat6));
	EXPECT_TRUE(GitStat6.bIsGitItem);
	EXPECT_FALSE(GitStat6.bIsUnborn);
	EXPECT_STREQ("master", GitStat6.CurrentBranch.c_str());
	EXPECT_FALSE(GitStat6.bHasSubmodule);
	EXPECT_TRUE(GitStat6.HasMods);
	EXPECT_FALSE(GitStat6.HasUnversioned);
	EXPECT_EQ(12u, GitStat6.NumCommits);
	EXPECT_FALSE(GitStat6.bHasSubmoduleMods);
	EXPECT_FALSE(GitStat6.bHasSubmoduleNewCommits);
	EXPECT_FALSE(GitStat6.bHasSubmoduleUnversioned);
	EXPECT_FALSE(GitStat6.bIsTagged);
	EXPECT_STREQ("7c3cbfe13a929d2291a574dca45e4fd2d2ac1aa6", GitStat6.HeadHashReadable);
	EXPECT_STREQ("Sven Strickroth", GitStat6.HeadAuthor.c_str());
	EXPECT_STREQ("email@cs-ware.de", GitStat6.HeadEmail.c_str());
}

TEST_P(GitWCRevStatusCBasicGitWithTestRepoFixture, PathNormalize)
{
	CString output;
	EXPECT_EQ(0, m_Git.Run(L"git.exe checkout -f master", &output, nullptr, CP_UTF8));
	EXPECT_STRNE(L"", output);

	GitWCRev_t GitStat;
	EXPECT_EQ(0, GetStatusUnCleanPath(m_Dir.GetTempDir() + L"\\copy\\.\\ansi.txt", GitStat));
	EXPECT_TRUE(GitStat.bIsGitItem);
	EXPECT_FALSE(GitStat.bIsUnborn);
	EXPECT_STREQ("master", GitStat.CurrentBranch.c_str());
	EXPECT_FALSE(GitStat.bHasSubmodule);
	EXPECT_FALSE(GitStat.HasMods);
	EXPECT_FALSE(GitStat.HasUnversioned);
	EXPECT_EQ(12u, GitStat.NumCommits);
	EXPECT_FALSE(GitStat.bHasSubmoduleMods);
	EXPECT_FALSE(GitStat.bHasSubmoduleNewCommits);
	EXPECT_FALSE(GitStat.bHasSubmoduleUnversioned);
	EXPECT_FALSE(GitStat.bIsTagged);
	EXPECT_STREQ("7c3cbfe13a929d2291a574dca45e4fd2d2ac1aa6", GitStat.HeadHashReadable);
	EXPECT_STREQ("Sven Strickroth", GitStat.HeadAuthor.c_str());
	EXPECT_STREQ("email@cs-ware.de", GitStat.HeadEmail.c_str());
}
