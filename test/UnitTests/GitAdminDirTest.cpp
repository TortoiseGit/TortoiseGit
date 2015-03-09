// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2015 - TortoiseGit
// Copyright (C) 2003-2008 - TortoiseSVN

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
#include "GitAdminDir.h"
#include "StringUtils.h"

TEST(CGitAdminDir, IsBareRepo)
{
	CAutoTempDir tmpDir;

	EXPECT_FALSE(GitAdminDir::IsBareRepo(tmpDir.GetTempDir()));

	CAutoRepository repo = nullptr;
	ASSERT_TRUE(git_repository_init(repo.GetPointer(), CUnicodeUtils::GetUTF8(tmpDir.GetTempDir()), true) == 0);

	EXPECT_TRUE(GitAdminDir::IsBareRepo(tmpDir.GetTempDir()));
}

TEST(CGitAdminDir, IsBareRepo_normalRepo)
{
	CAutoTempDir tmpDir;

	EXPECT_FALSE(GitAdminDir::IsBareRepo(tmpDir.GetTempDir()));

	CAutoRepository repo = nullptr;
	ASSERT_TRUE(git_repository_init(repo.GetPointer(), CUnicodeUtils::GetUTF8(tmpDir.GetTempDir()), false) == 0);

	EXPECT_FALSE(GitAdminDir::IsBareRepo(tmpDir.GetTempDir()));

	EXPECT_FALSE(GitAdminDir::IsBareRepo(tmpDir.GetTempDir() + _T("\\.git")));

	EXPECT_FALSE(GitAdminDir::IsBareRepo(tmpDir.GetTempDir() + _T("\\.git\\objects")));
}

TEST(CGitAdminDir, IsBareRepo_ReferencedRepo)
{
	CAutoTempDir tmpDir;

	CString gitFile = tmpDir.GetTempDir() + L"\\.git";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)gitFile, L"gitdir: dontcare"));

	EXPECT_FALSE(GitAdminDir::IsBareRepo(tmpDir.GetTempDir()));
}

TEST(CGitAdminDir, IsAdminDirPath)
{
	CAutoTempDir tmpDir;

	EXPECT_FALSE(GitAdminDir::IsAdminDirPath(tmpDir.GetTempDir()));

	CAutoRepository repo = nullptr;
	ASSERT_TRUE(git_repository_init(repo.GetPointer(), CUnicodeUtils::GetUTF8(tmpDir.GetTempDir()), false) == 0);

	EXPECT_FALSE(GitAdminDir::IsAdminDirPath(tmpDir.GetTempDir()));

	EXPECT_TRUE(GitAdminDir::IsAdminDirPath(tmpDir.GetTempDir() + L"\\.git"));

	EXPECT_TRUE(GitAdminDir::IsAdminDirPath(tmpDir.GetTempDir() + L"\\.git\\config"));

	EXPECT_TRUE(GitAdminDir::IsAdminDirPath(tmpDir.GetTempDir() + L"\\.git\\objects"));

	EXPECT_FALSE(GitAdminDir::IsAdminDirPath(tmpDir.GetTempDir() + L"\\.gitmodules"));

	ASSERT_TRUE(CreateDirectory(tmpDir.GetTempDir() + L"\\.gitted", nullptr));
	EXPECT_FALSE(GitAdminDir::IsAdminDirPath(tmpDir.GetTempDir() + L"\\.gitted"));
}

TEST(CGitAdminDir, IsAdminDirPath_BareRepo)
{
	CAutoTempDir tmpDir;

	EXPECT_FALSE(GitAdminDir::IsAdminDirPath(tmpDir.GetTempDir()));

	CAutoRepository repo = nullptr;
	ASSERT_TRUE(git_repository_init(repo.GetPointer(), CUnicodeUtils::GetUTF8(tmpDir.GetTempDir()), true) == 0);

	EXPECT_FALSE(GitAdminDir::IsAdminDirPath(tmpDir.GetTempDir()));
}

TEST(CGitAdminDir, GetAdminDirPath_BareRepo)
{
	CAutoTempDir tmpDir;

	CString adminDir;
	EXPECT_FALSE(GitAdminDir::GetAdminDirPath(tmpDir.GetTempDir(), adminDir));
	EXPECT_TRUE(adminDir.IsEmpty());

	CAutoRepository repo = nullptr;
	ASSERT_TRUE(git_repository_init(repo.GetPointer(), CUnicodeUtils::GetUTF8(tmpDir.GetTempDir()), true) == 0);

	EXPECT_TRUE(GitAdminDir::GetAdminDirPath(tmpDir.GetTempDir(), adminDir));
	EXPECT_STREQ(tmpDir.GetTempDir() + _T("\\"), adminDir);

	adminDir.Empty();
	EXPECT_FALSE(GitAdminDir::GetAdminDirPath(tmpDir.GetTempDir() + L"\\objects", adminDir));
	EXPECT_TRUE(adminDir.IsEmpty());
}

TEST(CGitAdminDir, GetAdminDirPath_ReferencedRepo)
{
	CAutoTempDir tmpDir;

	CString gitFile = tmpDir.GetTempDir() + L"\\.git";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)gitFile, L""));

	CString adminDir;
	EXPECT_FALSE(GitAdminDir::GetAdminDirPath(tmpDir.GetTempDir(), adminDir));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)gitFile, L"gitdir: dontcare\n"));

	EXPECT_TRUE(GitAdminDir::GetAdminDirPath(tmpDir.GetTempDir(), adminDir));
	EXPECT_STREQ(L"dontcare\\", adminDir);

	ASSERT_TRUE(CreateDirectory(tmpDir.GetTempDir() + L"\\anotherdir", nullptr));
	gitFile = tmpDir.GetTempDir() + L"\\anotherdir\\.git";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)gitFile, L"gitdir: ../something\n"));

	EXPECT_TRUE(GitAdminDir::GetAdminDirPath(tmpDir.GetTempDir() + L"\\anotherdir", adminDir));
	EXPECT_STREQ(tmpDir.GetTempDir() + L"\\something\\", adminDir);

	ASSERT_TRUE(CreateDirectory(tmpDir.GetTempDir() + L"\\\x6587", nullptr));
	gitFile = tmpDir.GetTempDir() + L"\\\x6587\\.git";

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)gitFile, L"gitdir: ../\x4e2d\n"));

	EXPECT_TRUE(GitAdminDir::GetAdminDirPath(tmpDir.GetTempDir() + L"\\\x6587", adminDir));
	EXPECT_STREQ(tmpDir.GetTempDir() + L"\\\x4e2d\\", adminDir);
}

TEST(CGitAdminDir, HasAdminDir)
{
	CAutoTempDir tmpDir;

	EXPECT_FALSE(GitAdminDir::HasAdminDir(tmpDir.GetTempDir()));

	CString gitFile = tmpDir.GetTempDir() + L"\\.gitted";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)gitFile, L"something"));

	EXPECT_FALSE(GitAdminDir::HasAdminDir(tmpDir.GetTempDir()));

	CAutoRepository repo = nullptr;
	ASSERT_TRUE(git_repository_init(repo.GetPointer(), CUnicodeUtils::GetUTF8(tmpDir.GetTempDir()), false) == 0);

	CString repoRoot;
	EXPECT_TRUE(GitAdminDir::HasAdminDir(tmpDir.GetTempDir(), &repoRoot));
	EXPECT_STREQ(tmpDir.GetTempDir(), repoRoot);

	EXPECT_FALSE(GitAdminDir::HasAdminDir(tmpDir.GetTempDir() + _T("\\.git"), &repoRoot));
	
	repoRoot.Empty();
	EXPECT_TRUE(GitAdminDir::HasAdminDir(tmpDir.GetTempDir() + _T("\\.gitmodules"), &repoRoot));
	EXPECT_STREQ(tmpDir.GetTempDir(), repoRoot);

	repoRoot.Empty();
	EXPECT_TRUE(GitAdminDir::HasAdminDir(tmpDir.GetTempDir() + _T("\\something"), &repoRoot));
	EXPECT_STREQ(tmpDir.GetTempDir(), repoRoot);

	ASSERT_TRUE(CreateDirectory(tmpDir.GetTempDir() + L"\\anotherdir", nullptr));
	repoRoot.Empty();
	EXPECT_TRUE(GitAdminDir::HasAdminDir(tmpDir.GetTempDir() + _T("\\anotherdir"), &repoRoot));
	EXPECT_STREQ(tmpDir.GetTempDir(), repoRoot);

	CAutoRepository subrepo = nullptr;
	ASSERT_TRUE(git_repository_init(subrepo.GetPointer(), CUnicodeUtils::GetUTF8(tmpDir.GetTempDir() + _T("\\anotherdir")), false) == 0);

	repoRoot.Empty();
	EXPECT_TRUE(GitAdminDir::HasAdminDir(tmpDir.GetTempDir() + _T("\\anotherdir"), &repoRoot));
	EXPECT_STREQ(tmpDir.GetTempDir() + _T("\\anotherdir"), repoRoot);

	repoRoot.Empty();
	EXPECT_TRUE(GitAdminDir::HasAdminDir(tmpDir.GetTempDir() + _T("\\anotherdir"), true, &repoRoot));
	EXPECT_STREQ(tmpDir.GetTempDir() + _T("\\anotherdir"), repoRoot);

	repoRoot.Empty();
	EXPECT_TRUE(GitAdminDir::HasAdminDir(tmpDir.GetTempDir() + _T("\\anotherdir"), false, &repoRoot));
	EXPECT_STREQ(tmpDir.GetTempDir(), repoRoot);
}

TEST(CGitAdminDir, HasAdminDir_bareRepo)
{
	CAutoTempDir tmpDir;

	EXPECT_FALSE(GitAdminDir::HasAdminDir(tmpDir.GetTempDir()));

	CAutoRepository repo = nullptr;
	ASSERT_TRUE(git_repository_init(repo.GetPointer(), CUnicodeUtils::GetUTF8(tmpDir.GetTempDir()), true) == 0);

	EXPECT_FALSE(GitAdminDir::HasAdminDir(tmpDir.GetTempDir()));

	EXPECT_FALSE(GitAdminDir::IsBareRepo(tmpDir.GetTempDir() + _T("\\objects")));
}

TEST(CGitAdminDir, HasAdminDir_ReferencedRepo)
{
	CAutoTempDir tmpDir;

	CString gitFile = tmpDir.GetTempDir() + L"\\.git";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)gitFile, L"gitdir: dontcare"));

	CString repoRoot;
	EXPECT_TRUE(GitAdminDir::HasAdminDir(tmpDir.GetTempDir(), &repoRoot));
	EXPECT_STREQ(tmpDir.GetTempDir(), repoRoot);
}
