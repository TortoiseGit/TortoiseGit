// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2016-2019 - TortoiseGit

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
#include "Git.h"
#include "StringUtils.h"
#include "gitdll.h"

TEST(libgit, BrokenConfig)
{
	CAutoTempDir tempdir;
	g_Git.m_CurrentDir = tempdir.GetTempDir();
	g_Git.m_IsGitDllInited = false;
	g_Git.m_CurrentDir = g_Git.m_CurrentDir;
	g_Git.m_IsUseGitDLL = true;
	g_Git.m_IsUseLibGit2 = false;
	g_Git.m_IsUseLibGit2_mask = 0;
	// libgit relies on CWD being set to working tree
	SetCurrentDirectory(g_Git.m_CurrentDir);

	CString output;
	EXPECT_EQ(0, g_Git.Run(L"git.exe init", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);
	CString testFile = tempdir.GetTempDir() + L"\\.git\\config";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(testFile, L"[push]\ndefault=something-that-is-invalid\n"));

	EXPECT_THROW(g_Git.CheckAndInitDll(), const char*);
}

TEST(libgit, Mailmap)
{
	CAutoTempDir tempdir;
	g_Git.m_CurrentDir = tempdir.GetTempDir();
	// libgit relies on CWD being set to working tree
	SetCurrentDirectory(g_Git.m_CurrentDir);

	GIT_MAILMAP mailmap = reinterpret_cast<void*>(0x12345678);
	git_read_mailmap(&mailmap);
	EXPECT_EQ(nullptr, mailmap);

	CString mailmapFile = tempdir.GetTempDir() + L"\\.mailmap";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(mailmapFile, L""));

	mailmap = reinterpret_cast<void*>(0x12345678);
	git_read_mailmap(&mailmap);
	EXPECT_EQ(nullptr, mailmap);

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(mailmapFile, L"Sven Strickroth <sven@tortoisegit.org>"));
	git_read_mailmap(&mailmap);
	EXPECT_NE(nullptr, mailmap);
	const char* email1 = nullptr;
	const char* author1 = nullptr;
	EXPECT_EQ(-1, git_lookup_mailmap(mailmap, &email1, &author1, "email@cs-ware.de", nullptr, [](void*) { return "Sven S."; }));
	EXPECT_EQ(0, git_lookup_mailmap(mailmap, &email1, &author1, "sven@tortoisegit.org", nullptr, [](void*) { return "Sven S."; }));
	EXPECT_EQ(nullptr, email1);
	EXPECT_STREQ("Sven Strickroth", author1);

	email1 = nullptr;
	author1 = nullptr;
	EXPECT_EQ(0, git_lookup_mailmap(mailmap, &email1, &author1, "Sven@tortoisegit.org", nullptr, [](void*) { return "Sven S."; }));
	EXPECT_EQ(nullptr, email1);
	EXPECT_STREQ("Sven Strickroth", author1);

	git_free_mailmap(mailmap);
	CString content;
	for (auto& entry : { L"", L"1", L"2", L"A", L"4", L"5", L"b", L"7" })
		content.AppendFormat(L"Sven%s Strickroth <sven%s@tortoisegit.org> <email%s@cs-ware.de>\n", entry, entry, entry);
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(mailmapFile, content));
	git_read_mailmap(&mailmap);
	EXPECT_NE(nullptr, mailmap);
	email1 = nullptr;
	author1 = nullptr;
	EXPECT_EQ(-1, git_lookup_mailmap(mailmap, &email1, &author1, "sven@tortoisegit.org", nullptr, [](void*) { return "Sven S."; }));
	EXPECT_EQ(-1, git_lookup_mailmap(mailmap, &email1, &author1, "aaa@tortoisegit.org", nullptr, [](void*) { return "Sven S."; }));
	EXPECT_EQ(-1, git_lookup_mailmap(mailmap, &email1, &author1, "zzz@tortoisegit.org", nullptr, [](void*) { return "Sven S."; }));
	for (auto& entry : { "", "1", "2", "A", "4", "5", "b", "7" })
	{
		CStringA maillookup, mail, name;
		maillookup.Format("email%s@cs-ware.de", entry);
		mail.Format("sven%s@tortoisegit.org", entry);
		name.Format("Sven%s Strickroth", entry);
		email1 = nullptr;
		author1 = nullptr;
		EXPECT_EQ(0, git_lookup_mailmap(mailmap, &email1, &author1, maillookup, nullptr, [](void*) { return "Sven S."; }));
		EXPECT_STREQ(mail, email1);
		EXPECT_STREQ(name, author1);
	}

	email1 = nullptr;
	author1 = nullptr;
	EXPECT_EQ(0, git_lookup_mailmap(mailmap, &email1, &author1, "email@cs-ware.de", nullptr, [](void*) { return "Sven Strickroth"; }));
	EXPECT_STREQ("sven@tortoisegit.org", email1);
	EXPECT_STREQ("Sven Strickroth", author1);

	git_free_mailmap(mailmap);
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(mailmapFile, L"<sven@tortoisegit.org> <email@cs-ware.de>\nSven S. <sven@tortoisegit.org> Sven Strickroth <email@cs-ware.de>"));
	git_read_mailmap(&mailmap);
	EXPECT_NE(nullptr, mailmap);
	email1 = nullptr;
	author1 = nullptr;
	EXPECT_EQ(-1, git_lookup_mailmap(mailmap, &email1, &author1, "sven@tortoisegit.org", nullptr, [](void*) { return "Sven S."; }));
	EXPECT_EQ(-1, git_lookup_mailmap(mailmap, &email1, &author1, "aaa@tortoisegit.org", nullptr, [](void*) { return "Sven S."; }));
	EXPECT_EQ(-1, git_lookup_mailmap(mailmap, &email1, &author1, "zzz@tortoisegit.org", nullptr, [](void*) { return "Sven S."; }));
	EXPECT_EQ(0, git_lookup_mailmap(mailmap, &email1, &author1, "email@cs-ware.de", nullptr, [](void*) { return "Sven S."; }));
	EXPECT_STREQ("sven@tortoisegit.org", email1);
	EXPECT_STREQ(nullptr, author1);
	email1 = nullptr;
	author1 = nullptr;
	EXPECT_EQ(0, git_lookup_mailmap(mailmap, &email1, &author1, "email@cs-ware.de", nullptr, [](void*) { return "Sven Strickroth"; }));
	EXPECT_STREQ("sven@tortoisegit.org", email1);
	EXPECT_STREQ("Sven S.", author1);
}

TEST(libgit, MkDir)
{
	CAutoTempDir tempdir;
	CString subdir = tempdir.GetTempDir() + L"\\abc";

	EXPECT_FALSE(PathFileExists(subdir));
	EXPECT_EQ(0, git_mkdir(CUnicodeUtils::GetUTF8(subdir)));
	EXPECT_TRUE(PathFileExists(subdir));
	EXPECT_TRUE(PathIsDirectory(subdir));
	EXPECT_EQ(-1, git_mkdir(CUnicodeUtils::GetUTF8(subdir)));
}

TEST(libgit, RefreshIndex)
{
	CAutoTempDir tempdir;
	g_Git.m_CurrentDir = tempdir.GetTempDir();
	g_Git.m_bInitialized = false;
	g_Git.m_IsGitDllInited = false;
	g_Git.m_IsUseGitDLL = true;
	g_Git.m_IsUseLibGit2 = false;
	g_Git.m_IsUseLibGit2_mask = 0;

	// libgit relies on CWD being set to working tree
	SetCurrentDirectory(g_Git.m_CurrentDir);

	git_repository_init_options options = GIT_REPOSITORY_INIT_OPTIONS_INIT;
	options.flags = GIT_REPOSITORY_INIT_MKPATH | GIT_REPOSITORY_INIT_EXTERNAL_TEMPLATE;
	CAutoRepository repo;
	ASSERT_EQ(0, git_repository_init_ext(repo.GetPointer(), CUnicodeUtils::GetUTF8(tempdir.GetTempDir()), &options));
	CAutoConfig config(repo);
	ASSERT_TRUE(config.IsValid());
	CStringA path = CUnicodeUtils::GetUTF8(g_Git.m_CurrentDir);
	path.Replace('\\', '/');
	EXPECT_EQ(0, git_config_set_string(config, "filter.openssl.clean", path + "/clean_filter_openssl"));
	EXPECT_EQ(0, git_config_set_string(config, "filter.openssl.smudge", path + "/smudge_filter_openssl"));
	EXPECT_EQ(0, git_config_set_bool(config, "filter.openssl.required", 1));
	CString cleanFilterFilename = g_Git.m_CurrentDir + L"\\clean_filter_openssl";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(cleanFilterFilename, L"#!/bin/bash\nopenssl version | grep -q 1\\\\.0\nif [[ $? = 0 ]]; then\n\topenssl enc -base64 -aes-256-ecb -S FEEDDEADBEEF -k PASS_FIXED\nelse\n\topenssl enc -base64 -pbkdf2 -aes-256-ecb -S FEEDDEADBEEFFEED -k PASS_FIXED\nfi\n"));
	CString smudgeFilterFilename = g_Git.m_CurrentDir + L"\\smudge_filter_openssl";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(smudgeFilterFilename, L"#!/bin/bash\nopenssl version | grep -q 1\\\\.0\nif [[ $? = 0 ]]; then\n\topenssl enc -d -base64 -aes-256-ecb -k PASS_FIXED\nelse\n\topenssl enc -d -base64 -pbkdf2 -aes-256-ecb -k PASS_FIXED\nfi\n"));
	EXPECT_EQ(0, git_config_set_string(config, "filter.test.clean", path + "/clean_filter_openssl"));
	EXPECT_EQ(0, git_config_set_string(config, "filter.test.smudge", path + "/smudge_filter_openssl"));
	EXPECT_EQ(0, git_config_set_string(config, "filter.test.process", path + "/clean_filter_openssl"));
	EXPECT_EQ(0, git_config_set_bool(config, "filter.test.required", 1));

	// need to make sure sh.exe is on PATH
	g_Git.CheckMsysGitDir();
	size_t size;
	_wgetenv_s(&size, nullptr, 0, L"PATH");
	ASSERT_LT(0U, size);
	auto oldEnv = std::make_unique<wchar_t[]>(size);
	ASSERT_TRUE(oldEnv);
	_wgetenv_s(&size, oldEnv.get(), size, L"PATH");
	_wputenv_s(L"PATH", g_Git.m_Environment.GetEnv(L"PATH"));
	SCOPE_EXIT { _wputenv_s(L"PATH", oldEnv.get()); };
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(g_Git.m_CurrentDir + L"\\somefile.txt", L"some content"));

	g_Git.RefreshGitIndex();

	CString output;
	EXPECT_EQ(0, g_Git.Run(L"git.exe add somefile.txt", &output, CP_UTF8));
	EXPECT_STREQ(L"", output);

	g_Git.RefreshGitIndex();

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(g_Git.m_CurrentDir + L"\\somefile.txt", L"some other content"));

	g_Git.RefreshGitIndex();

	output.Empty();
	EXPECT_EQ(0, g_Git.Run(L"git.exe add somefile.txt", &output, CP_UTF8));
	EXPECT_STREQ(L"", output);

	g_Git.RefreshGitIndex();

	// now check with external command filters defined
	CString attributesFile = g_Git.m_CurrentDir + L"\\.gitattributes";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(attributesFile, L"*.enc filter=openssl\n"));

	CString encryptedFileOne = g_Git.m_CurrentDir + L"\\1.enc";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(encryptedFileOne, L"This should be encrypted...\nAnd decrypted on the fly\n"));

	output.Empty();
	EXPECT_EQ(0, g_Git.Run(L"git.exe add 1.enc", &output, CP_UTF8));
	if (!g_Git.ms_bCygwinGit) // on AppVeyor with the VS2017 image we get a warning: "WARNING: can't open config file: /usr/local/ssl/openssl.cnf"
		EXPECT_STREQ(L"", output);

	WIN32_FILE_ATTRIBUTE_DATA fdata;
	GetFileAttributesEx(g_Git.m_CurrentDir + L"\\.git\\index", GetFileExInfoStandard, &fdata);

	g_Git.RefreshGitIndex();

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(g_Git.m_CurrentDir + L"\\1.enc", L"some other content"));

	g_Git.RefreshGitIndex();

	// need racy timestamp
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(g_Git.m_CurrentDir + L"\\1.enc", L"somE other content"));
	{
		CAutoGeneralHandle handle = ::CreateFile(g_Git.m_CurrentDir + L"\\1.enc", FILE_WRITE_ATTRIBUTES, 0, nullptr, 0, 0, nullptr);
		SetFileTime(handle, &fdata.ftCreationTime, &fdata.ftLastAccessTime, &fdata.ftLastWriteTime);
	}

	g_Git.RefreshGitIndex();

	// now check with external command filters with multi-filter (process) defined
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(attributesFile, L"*.enc filter=test\n"));

	g_Git.RefreshGitIndex();

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(g_Git.m_CurrentDir + L"\\1.enc", L"somE other conTentsome other conTentsome other conTen"));

	g_Git.RefreshGitIndex();

	// need racy timestamp
	GetFileAttributesEx(g_Git.m_CurrentDir + L"\\.git\\index", GetFileExInfoStandard, &fdata);
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(g_Git.m_CurrentDir + L"\\1.enc", L"some other conTentsome other conTentsome other conTen"));
	{
		CAutoGeneralHandle handle = ::CreateFile(g_Git.m_CurrentDir + L"\\1.enc", FILE_WRITE_ATTRIBUTES, 0, nullptr, 0, 0, nullptr);
		SetFileTime(handle, &fdata.ftCreationTime, &fdata.ftLastAccessTime, &fdata.ftLastWriteTime);
	}

	g_Git.RefreshGitIndex();
}

TEST(libgit, IncludeIf)
{
	CAutoTempDir tempdir;
	g_Git.m_bInitialized = false;
	g_Git.m_IsGitDllInited = false;
	g_Git.m_IsUseGitDLL = true;
	g_Git.m_IsUseLibGit2 = false;
	g_Git.m_IsUseLibGit2_mask = 0;

	// .git dir
	CString repoDir = tempdir.GetTempDir() + L"\\RepoWithAInPath";
	g_Git.m_CurrentDir = repoDir;
	EXPECT_TRUE(CreateDirectory(repoDir, nullptr));
	// libgit relies on CWD being set to working tree
	EXPECT_TRUE(SetCurrentDirectory(repoDir));

	git_repository_init_options options = GIT_REPOSITORY_INIT_OPTIONS_INIT;
	options.flags = GIT_REPOSITORY_INIT_EXTERNAL_TEMPLATE;
	CAutoRepository repo;
	ASSERT_EQ(0, git_repository_init_ext(repo.GetPointer(), CUnicodeUtils::GetUTF8(repoDir), &options));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(repoDir + L"\\.git\\config", L"[core]\n	repositoryformatversion = 0\n	filemode = false\n	bare = false\n	logallrefupdates = true\n	symlinks = false\n	ignorecase = true\n	hideDotFiles = dotGitOnly\n[something]\n	thevalue = jap\n[includeIf \"gitdir:RepoWithAInPath/**\"]\n	path = configA\n[includeIf \"gitdir:RepoWithBInPath/**\"]\n	path = configB\n"));
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(repoDir + L"\\.git\\configA", L"[somethinga]\n	thevalue = jop\n"));
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(repoDir + L"\\.git\\configB", L"[somethingb]\n	thevalue = jup\n"));

	EXPECT_STREQ(L"jap", g_Git.GetConfigValue(L"something.thevalue"));
	EXPECT_STREQ(L"jop",g_Git.GetConfigValue(L"somethinga.thevalue"));
	EXPECT_STREQ(L"", g_Git.GetConfigValue(L"somethingb.thevalue"));

	// .git file
	g_Git.m_bInitialized = false;
	g_Git.m_IsGitDllInited = false;
	g_Git.m_IsUseGitDLL = true;
	repoDir = tempdir.GetTempDir() + L"\\RepoWithBInPath";
	g_Git.m_CurrentDir = repoDir;
	EXPECT_TRUE(CreateDirectory(repoDir, nullptr));
	// libgit relies on CWD being set to working tree
	EXPECT_TRUE(SetCurrentDirectory(repoDir));
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(repoDir + L"\\.git", L"gitdir: ../RepoWithAInPath/.git\n"));

	EXPECT_STREQ(L"jap", g_Git.GetConfigValue(L"something.thevalue"));
	EXPECT_STREQ(L"jop", g_Git.GetConfigValue(L"somethinga.thevalue"));
	EXPECT_STREQ(L"", g_Git.GetConfigValue(L"somethingb.thevalue"));
}

TEST(libgit, StoreUninitializedRepositoryConfig)
{
	CAutoTempDir tempdir;
	g_Git.m_CurrentDir = tempdir.GetTempDir();
	g_Git.m_IsGitDllInited = false;
	g_Git.m_CurrentDir = g_Git.m_CurrentDir;
	g_Git.m_IsUseGitDLL = true;
	g_Git.m_IsUseLibGit2 = false;
	g_Git.m_IsUseLibGit2_mask = 0;
	// libgit relies on CWD being set to working tree
	SetCurrentDirectory(g_Git.m_CurrentDir);

	// clear any leftovers in caches
	EXPECT_THROW(g_Git.CheckAndInitDll(), const char*);

	EXPECT_THROW(get_set_config("something", "else", CONFIG_LOCAL), const char*);
}
