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
#include "StringUtils.h"
#include "git2/sys/repository.h"
#include "Git.h"

TEST(libgit2, Config)
{
	CAutoTempDir tempdir;
	CString testFile = tempdir.GetTempDir() + L"\\config";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(testFile, L"[core]\nemail=dummy@example.com\ntrue=true\nfalse=false\n"));
	CAutoConfig config(true);
	EXPECT_EQ(0, git_config_add_file_ondisk(config, CUnicodeUtils::GetUTF8(testFile), GIT_CONFIG_LEVEL_LOCAL, nullptr, 1));
	bool ret = false;
	EXPECT_EQ(0, config.GetBool(L"core.true", ret));
	EXPECT_EQ(true, ret);
	EXPECT_EQ(0, config.GetBool(L"core.false", ret));
	EXPECT_EQ(false, ret);
	EXPECT_EQ(-3, config.GetBool(L"core.not-exist", ret));
	CString value;
	EXPECT_EQ(0, config.GetString(L"core.email", value));
	EXPECT_STREQ(L"dummy@example.com", value);
}

// the following test ís similar to a test in libgit2, keep it to prevent surprises
TEST(libgit2, ConfigSnaphot)
{
	CAutoTempDir tmpDir;

	CAutoRepository repo;
	ASSERT_TRUE(git_repository_init(repo.GetPointer(), CUnicodeUtils::GetUTF8(tmpDir.GetTempDir()), false) == 0);
	CString configFile = tmpDir.GetTempDir() + L"\\.git\\config";
	CString text;
	ASSERT_TRUE(CStringUtils::ReadStringFromTextFile(configFile, text));
	text += L"[includeIf \"onbranch:master\"]\n\tpath = master.txt\n";
	text += L"[includeIf \"onbranch:other\"]\n\tpath = other.txt\n";
	text += L"[include]\n\tpath = include.txt\n";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(configFile, text));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(tmpDir.GetTempDir() + L"\\.git\\master.txt", L"[blup]\n\tbran = master\nmaster\n"));
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(tmpDir.GetTempDir() + L"\\.git\\other.txt", L"[blup]\n\tbran = other\nother\n"));
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(tmpDir.GetTempDir() + L"\\.git\\include.txt", L"[blup]\n\tfoo = bar\n"));

	CAutoConfig config;
	EXPECT_EQ(0, git_repository_config_snapshot(config.GetPointer(), repo));
	repo.Free();

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(tmpDir.GetTempDir() + L"\\.git\\include.txt", L"[blup]\n\tfoo = changedafterwards\n"));

	bool bResult = false;
	EXPECT_EQ(-3, config.GetBool(L"blup.doesnotexist", bResult));
	EXPECT_EQ(-3, config.GetBool(L"blup.other", bResult));
	EXPECT_EQ(0, config.GetBool(L"blup.master", bResult));
	EXPECT_TRUE(bResult);

	CString sResult;
	EXPECT_EQ(0, config.GetString(L"blup.bran", sResult));
	EXPECT_STREQ(L"master", sResult);
	sResult.Empty();
	EXPECT_EQ(0, config.GetString(L"blup.foo", sResult));
	EXPECT_STREQ(L"bar", sResult);
}

// check whether a snapshotted config can be used in git_repository and then working on the repo using hashfile as TGitCache does in GitIndex.cpp
TEST(libgit2, ConfigSnaphotRepoHashFile)
{
	CAutoTempDir tmpDir;

	CAutoRepository repo;
	ASSERT_TRUE(git_repository_init(repo.GetPointer(), CUnicodeUtils::GetUTF8(tmpDir.GetTempDir()), false) == 0);
	CString configFile = tmpDir.GetTempDir() + L"\\.git\\config";
	CString text;
	ASSERT_TRUE(CStringUtils::ReadStringFromTextFile(configFile, text));
	text += L"[user]\n\tuser = Someone\n\temail = some@one.de\n";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(configFile, text));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(tmpDir.GetTempDir() + L"\\test.txt", L"something"));
	git_oid oid;
	EXPECT_EQ(0, git_repository_hashfile(&oid, repo, "test.txt", GIT_OBJECT_BLOB, nullptr));
	EXPECT_STREQ(L"a459bc245bdbc45e1bca99e7fe61731da5c48da4", CGitHash(oid).ToString());

	CAutoConfig config;
	EXPECT_EQ(0, git_repository_config_snapshot(config.GetPointer(), repo));
	git_repository_set_config(repo, config);

	g_Git.m_CurrentDir = tmpDir.GetTempDir();
	CString output;
	EXPECT_EQ(0, g_Git.Run(L"git.exe add test.txt", &output, CP_UTF8));
	EXPECT_STREQ(L"", output);
	output.Empty();
	EXPECT_EQ(0, g_Git.Run(L"git.exe commit -m \"Add test.txt\"", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(tmpDir.GetTempDir() + L"\\test.txt", L"something else"));

	git_oid oid2;
	EXPECT_EQ(0, git_repository_hashfile(&oid2, repo, "test.txt", GIT_OBJECT_BLOB, nullptr));
	EXPECT_STREQ(L"339f0be6c0f5257a3279fcefc45373e805b7ebc3", CGitHash(oid2).ToString());

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(tmpDir.GetTempDir() + L"\\test.txt", L"something"));

	git_oid oid3;
	EXPECT_EQ(0, git_repository_hashfile(&oid3, repo, "test.txt", GIT_OBJECT_BLOB, nullptr));
	EXPECT_STREQ(L"a459bc245bdbc45e1bca99e7fe61731da5c48da4", CGitHash(oid3).ToString());
}
