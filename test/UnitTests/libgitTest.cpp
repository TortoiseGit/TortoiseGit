// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2016-2017 - TortoiseGit

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

	GIT_MAILMAP mailmap = (void*)0x12345678;
	git_read_mailmap(&mailmap);
	EXPECT_EQ(nullptr, mailmap);

	CString mailmapFile = tempdir.GetTempDir() + L"\\.mailmap";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(mailmapFile, L""));

	mailmap = (void*)0x12345678;
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
