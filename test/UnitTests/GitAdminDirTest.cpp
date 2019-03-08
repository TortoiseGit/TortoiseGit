// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2015-2017, 2019 - TortoiseGit
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

	CAutoRepository repo;
	ASSERT_TRUE(git_repository_init(repo.GetPointer(), CUnicodeUtils::GetUTF8(tmpDir.GetTempDir()), true) == 0);

	EXPECT_TRUE(GitAdminDir::IsBareRepo(tmpDir.GetTempDir()));
}

TEST(CGitAdminDir, IsBareRepo_normalRepo)
{
	CAutoTempDir tmpDir;

	EXPECT_FALSE(GitAdminDir::IsBareRepo(tmpDir.GetTempDir()));

	CAutoRepository repo;
	ASSERT_TRUE(git_repository_init(repo.GetPointer(), CUnicodeUtils::GetUTF8(tmpDir.GetTempDir()), false) == 0);

	EXPECT_FALSE(GitAdminDir::IsBareRepo(tmpDir.GetTempDir()));

	EXPECT_FALSE(GitAdminDir::IsBareRepo(tmpDir.GetTempDir() + L"\\.git"));

	EXPECT_FALSE(GitAdminDir::IsBareRepo(tmpDir.GetTempDir() + L"\\.git\\objects"));
}

TEST(CGitAdminDir, IsBareRepo_ReferencedRepo)
{
	CAutoTempDir tmpDir;

	CString gitFile = tmpDir.GetTempDir() + L"\\.git";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(gitFile, L"gitdir: dontcare"));

	EXPECT_FALSE(GitAdminDir::IsBareRepo(tmpDir.GetTempDir()));
}

TEST(CGitAdminDir, IsAdminDirPath)
{
	CAutoTempDir tmpDir;

	EXPECT_FALSE(GitAdminDir::IsAdminDirPath(tmpDir.GetTempDir()));

	CAutoRepository repo;
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

	CAutoRepository repo;
	ASSERT_TRUE(git_repository_init(repo.GetPointer(), CUnicodeUtils::GetUTF8(tmpDir.GetTempDir()), true) == 0);

	EXPECT_FALSE(GitAdminDir::IsAdminDirPath(tmpDir.GetTempDir()));
}

TEST(CGitAdminDir, GetAdminDirPath_BareRepo)
{
	CAutoTempDir tmpDir;

	CString adminDir;
	EXPECT_FALSE(GitAdminDir::GetAdminDirPath(tmpDir.GetTempDir(), adminDir));
	EXPECT_STREQ(L"", adminDir);

	CAutoRepository repo;
	ASSERT_TRUE(git_repository_init(repo.GetPointer(), CUnicodeUtils::GetUTF8(tmpDir.GetTempDir()), true) == 0);

	EXPECT_TRUE(GitAdminDir::GetAdminDirPath(tmpDir.GetTempDir(), adminDir));
	EXPECT_STREQ(tmpDir.GetTempDir() + L'\\', adminDir);

	adminDir.Empty();
	EXPECT_FALSE(GitAdminDir::GetAdminDirPath(tmpDir.GetTempDir() + L"\\objects", adminDir));
	EXPECT_STREQ(L"", adminDir);
}

TEST(CGitAdminDir, GetAdminDirPath_ReferencedRepo)
{
	CAutoTempDir tmpDir;

	CString gitFile = tmpDir.GetTempDir() + L"\\.git";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(gitFile, L""));

	CString adminDir;
	EXPECT_FALSE(GitAdminDir::GetAdminDirPath(tmpDir.GetTempDir(), adminDir));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(gitFile, L"gitdir: dontcare\n"));

	EXPECT_TRUE(GitAdminDir::GetAdminDirPath(tmpDir.GetTempDir(), adminDir));
	EXPECT_STREQ(tmpDir.GetTempDir() + L"\\dontcare\\", adminDir);

	ASSERT_TRUE(CreateDirectory(tmpDir.GetTempDir() + L"\\anotherdir", nullptr));
	gitFile = tmpDir.GetTempDir() + L"\\anotherdir\\.git";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(gitFile, L"gitdir: ../something\n"));

	EXPECT_TRUE(GitAdminDir::GetAdminDirPath(tmpDir.GetTempDir() + L"\\anotherdir", adminDir));
	EXPECT_STREQ(tmpDir.GetTempDir() + L"\\something\\", adminDir);

	ASSERT_TRUE(CreateDirectory(tmpDir.GetTempDir() + L"\\\x6587", nullptr));
	gitFile = tmpDir.GetTempDir() + L"\\\x6587\\.git";

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(gitFile, L"gitdir: ../\x4e2d\n"));

	EXPECT_TRUE(GitAdminDir::GetAdminDirPath(tmpDir.GetTempDir() + L"\\\x6587", adminDir));
	EXPECT_STREQ(tmpDir.GetTempDir() + L"\\\x4e2d\\", adminDir);
}

TEST(CGitAdminDir, HasAdminDir)
{
	CAutoTempDir tmpDir;

	EXPECT_FALSE(GitAdminDir::HasAdminDir(tmpDir.GetTempDir()));

	CString gitFile = tmpDir.GetTempDir() + L"\\.gitted";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(gitFile, L"something"));

	EXPECT_FALSE(GitAdminDir::HasAdminDir(tmpDir.GetTempDir()));

	CAutoRepository repo;
	ASSERT_TRUE(git_repository_init(repo.GetPointer(), CUnicodeUtils::GetUTF8(tmpDir.GetTempDir()), false) == 0);

	CString repoRoot;
	EXPECT_TRUE(GitAdminDir::HasAdminDir(tmpDir.GetTempDir(), &repoRoot));
	EXPECT_STREQ(tmpDir.GetTempDir(), repoRoot);

	EXPECT_FALSE(GitAdminDir::HasAdminDir(tmpDir.GetTempDir() + L"\\.git", &repoRoot));

	repoRoot.Empty();
	EXPECT_TRUE(GitAdminDir::HasAdminDir(tmpDir.GetTempDir() + L"\\.gitmodules", &repoRoot));
	EXPECT_STREQ(tmpDir.GetTempDir(), repoRoot);

	repoRoot.Empty();
	EXPECT_TRUE(GitAdminDir::HasAdminDir(tmpDir.GetTempDir() + L"\\something", &repoRoot));
	EXPECT_STREQ(tmpDir.GetTempDir(), repoRoot);

	ASSERT_TRUE(CreateDirectory(tmpDir.GetTempDir() + L"\\anotherdir", nullptr));
	repoRoot.Empty();
	EXPECT_TRUE(GitAdminDir::HasAdminDir(tmpDir.GetTempDir() + L"\\anotherdir", &repoRoot));
	EXPECT_STREQ(tmpDir.GetTempDir(), repoRoot);

	CAutoRepository subrepo;
	ASSERT_TRUE(git_repository_init(subrepo.GetPointer(), CUnicodeUtils::GetUTF8(tmpDir.GetTempDir() + L"\\anotherdir"), false) == 0);

	repoRoot.Empty();
	EXPECT_TRUE(GitAdminDir::HasAdminDir(tmpDir.GetTempDir() + L"\\anotherdir", &repoRoot));
	EXPECT_STREQ(tmpDir.GetTempDir() + L"\\anotherdir", repoRoot);

	repoRoot.Empty();
	EXPECT_TRUE(GitAdminDir::HasAdminDir(tmpDir.GetTempDir() + L"\\anotherdir", true, &repoRoot));
	EXPECT_STREQ(tmpDir.GetTempDir() + L"\\anotherdir", repoRoot);

	repoRoot.Empty();
	EXPECT_TRUE(GitAdminDir::HasAdminDir(tmpDir.GetTempDir() + L"\\anotherdir", false, &repoRoot));
	EXPECT_STREQ(tmpDir.GetTempDir(), repoRoot);
}

TEST(CGitAdminDir, HasAdminDir_bareRepo)
{
	CAutoTempDir tmpDir;

	EXPECT_FALSE(GitAdminDir::HasAdminDir(tmpDir.GetTempDir()));

	CAutoRepository repo;
	ASSERT_TRUE(git_repository_init(repo.GetPointer(), CUnicodeUtils::GetUTF8(tmpDir.GetTempDir()), true) == 0);

	EXPECT_FALSE(GitAdminDir::HasAdminDir(tmpDir.GetTempDir()));

	EXPECT_FALSE(GitAdminDir::IsBareRepo(tmpDir.GetTempDir() + L"\\objects"));
}

TEST(CGitAdminDir, HasAdminDir_ReferencedRepo)
{
	CAutoTempDir tmpDir;

	CString barePath = tmpDir.GetTempDir() + L"\\bare.git";
	ASSERT_TRUE(CreateDirectory(barePath, nullptr));
	CAutoRepository repo;
	ASSERT_TRUE(git_repository_init(repo.GetPointer(), CUnicodeUtils::GetUTF8(barePath), true) == 0);

	CString wtPath = tmpDir.GetTempDir() + L"\\wt";
	ASSERT_TRUE(CreateDirectory(wtPath, nullptr));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(wtPath + L"\\.git", L"gitdir: ../bare.git"));

	CString repoRoot;
	EXPECT_TRUE(GitAdminDir::HasAdminDir(wtPath, &repoRoot));
	EXPECT_STREQ(wtPath, repoRoot);
}

TEST(CGitAdminDir, HasAdminDir_ReferencedRepo2)
{
	CAutoTempDir tmpDir;

	CAutoRepository repo;
	ASSERT_TRUE(git_repository_init(repo.GetPointer(), CUnicodeUtils::GetUTF8(tmpDir.GetTempDir()), false) == 0);

	EXPECT_TRUE(MoveFile(tmpDir.GetTempDir() + L"\\.git", tmpDir.GetTempDir() + L"\\_git"));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(tmpDir.GetTempDir() + L"\\.git", L"gitdir: _git"));

	CString repoRoot;
	EXPECT_TRUE(GitAdminDir::HasAdminDir(tmpDir.GetTempDir(), &repoRoot));
	EXPECT_STREQ(tmpDir.GetTempDir(), repoRoot);
}

TEST(CGitAdminDir, HasAdminDir_InvalidRepo)
{
	CAutoTempDir tmpDir;

	EXPECT_TRUE(CreateDirectory(tmpDir.GetTempDir() + L"\\.git", nullptr));

	CString repoRoot;
	EXPECT_FALSE(GitAdminDir::HasAdminDir(tmpDir.GetTempDir(), &repoRoot));
}

TEST(CGitAdminDir, HasAdminDir_InvalidRepoFile)
{
	CAutoTempDir tmpDir;

	CString gitFile = tmpDir.GetTempDir() + L"\\.git";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(gitFile, L"gitdir: dontcare"));

	CString repoRoot;
	EXPECT_FALSE(GitAdminDir::HasAdminDir(tmpDir.GetTempDir(), &repoRoot));
}

TEST(CGitAdminDir, IsWorkingTreeOrBareRepo)
{
	CAutoTempDir tmpDir;

	EXPECT_FALSE(GitAdminDir::IsWorkingTreeOrBareRepo(tmpDir.GetTempDir()));

	CAutoRepository repo;
	ASSERT_TRUE(git_repository_init(repo.GetPointer(), CUnicodeUtils::GetUTF8(tmpDir.GetTempDir()), true) == 0);

	EXPECT_TRUE(GitAdminDir::IsWorkingTreeOrBareRepo(tmpDir.GetTempDir()));
}

TEST(CGitAdminDir, IsWorkingTreeOrBareRepo_normalRepo)
{
	CAutoTempDir tmpDir;

	EXPECT_FALSE(GitAdminDir::IsWorkingTreeOrBareRepo(tmpDir.GetTempDir()));

	CAutoRepository repo;
	ASSERT_TRUE(git_repository_init(repo.GetPointer(), CUnicodeUtils::GetUTF8(tmpDir.GetTempDir()), false) == 0);

	EXPECT_TRUE(GitAdminDir::IsWorkingTreeOrBareRepo(tmpDir.GetTempDir()));

	EXPECT_FALSE(GitAdminDir::IsWorkingTreeOrBareRepo(tmpDir.GetTempDir() + L"\\.git"));

	EXPECT_FALSE(GitAdminDir::IsWorkingTreeOrBareRepo(tmpDir.GetTempDir() + L"\\.git\\objects"));
}

TEST(CGitAdminDir, IsWorkingTreeOrBareRepo_ReferencedRepo)
{
	CAutoTempDir tmpDir;

	CString barePath = tmpDir.GetTempDir() + L"\\bare.git";
	ASSERT_TRUE(CreateDirectory(barePath, nullptr));
	CAutoRepository repo;
	ASSERT_TRUE(git_repository_init(repo.GetPointer(), CUnicodeUtils::GetUTF8(barePath), true) == 0);

	CString wtPath = tmpDir.GetTempDir() + L"\\wt";
	ASSERT_TRUE(CreateDirectory(wtPath, nullptr));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(wtPath + L"\\.git", L"gitdir: ../bare.git"));

	EXPECT_TRUE(GitAdminDir::IsWorkingTreeOrBareRepo(wtPath));
}

TEST(CGitAdminDir, IsWorkingTreeOrBareRepo_ReferencedRepo2)
{
	CAutoTempDir tmpDir;

	CAutoRepository repo;
	ASSERT_TRUE(git_repository_init(repo.GetPointer(), CUnicodeUtils::GetUTF8(tmpDir.GetTempDir()), false) == 0);

	EXPECT_TRUE(MoveFile(tmpDir.GetTempDir() + L"\\.git", tmpDir.GetTempDir() + L"\\_git"));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(tmpDir.GetTempDir() + L"\\.git", L"gitdir: _git"));

	EXPECT_TRUE(GitAdminDir::IsWorkingTreeOrBareRepo(tmpDir.GetTempDir()));
}

TEST(CGitAdminDir, IsWorkingTreeOrBareRepo_InvalidRepo)
{
	CAutoTempDir tmpDir;

	EXPECT_TRUE(CreateDirectory(tmpDir.GetTempDir() + L"\\.git", nullptr));

	EXPECT_FALSE(GitAdminDir::IsWorkingTreeOrBareRepo(tmpDir.GetTempDir()));
}

TEST(CGitAdminDir, IsWorkingTreeOrBareRepo_InvalidRepoFile)
{
	CAutoTempDir tmpDir;

	CString gitFile = tmpDir.GetTempDir() + L"\\.git";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(gitFile, L"gitdir: dontcare"));

	EXPECT_FALSE(GitAdminDir::IsWorkingTreeOrBareRepo(tmpDir.GetTempDir()));
}

TEST(CGitAdminDir, ReadGitLink)
{
	CAutoTempDir tmpDir;

	CString gitFile = tmpDir.GetTempDir() + L"\\.git";
	EXPECT_STREQ(L"", GitAdminDir::ReadGitLink(L"C:\\somerepo", gitFile));

	EXPECT_TRUE(::CreateDirectory(gitFile, nullptr));
	EXPECT_STREQ(L"", GitAdminDir::ReadGitLink(L"C:\\somerepo", gitFile));
	EXPECT_TRUE(::RemoveDirectory(gitFile));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(gitFile, L"broken"));
	EXPECT_STREQ(L"", GitAdminDir::ReadGitLink(L"C:\\somerepo", gitFile));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(gitFile, L"gitdir: dontcare"));
	EXPECT_STREQ(L"C:\\somerepo\\dontcare", GitAdminDir::ReadGitLink(L"C:\\somerepo", gitFile));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(gitFile, L"gitdir: ./dontcare"));
	EXPECT_STREQ(L"C:\\somerepo\\dontcare", GitAdminDir::ReadGitLink(L"C:\\somerepo", gitFile));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(gitFile, L"gitdir: .dontcare"));
	EXPECT_STREQ(L"C:\\somerepo\\.dontcare", GitAdminDir::ReadGitLink(L"C:\\somerepo", gitFile));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(gitFile, L"gitdir: ..dontcare"));
	EXPECT_STREQ(L"C:\\somerepo\\..dontcare", GitAdminDir::ReadGitLink(L"C:\\somerepo", gitFile));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(gitFile, L"gitdir: ../.git/modules/dontcare"));
	EXPECT_STREQ(L"C:\\.git\\modules\\dontcare", GitAdminDir::ReadGitLink(L"C:\\somerepo", gitFile));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(gitFile, L"gitdir: c:\\someotherrepo\\.git\\modules\\bla"));
	EXPECT_STREQ(L"c:\\someotherrepo\\.git\\modules\\bla", GitAdminDir::ReadGitLink(L"C:\\somerepo", gitFile));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(gitFile, L"gitdir: \u6280\u672F\u6587\u6863")); // cf. issue #2453
	EXPECT_STREQ(L"C:\\somerepo\\\u6280\u672F\u6587\u6863", GitAdminDir::ReadGitLink(L"C:\\somerepo", gitFile));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(gitFile, L"gitdir: \u6280\u672F\u6587\u6863\n")); // cf. issue #2453
	EXPECT_STREQ(L"C:\\somerepo\\\u6280\u672F\u6587\u6863", GitAdminDir::ReadGitLink(L"C:\\somerepo", gitFile));
}

TEST(CGitAdminDir, GetSuperProjectRoot)
{
	CAutoTempDir tmpDir;

	EXPECT_STREQ(L"", GitAdminDir::GetSuperProjectRoot(tmpDir.GetTempDir()));

	EXPECT_TRUE(::CreateDirectory(tmpDir.GetTempDir() + L"\\.git", nullptr));
	EXPECT_STREQ(L"", GitAdminDir::GetSuperProjectRoot(tmpDir.GetTempDir()));

	EXPECT_TRUE(::CreateDirectory(tmpDir.GetTempDir() + L"\\subdir", nullptr));
	EXPECT_STREQ(L"", GitAdminDir::GetSuperProjectRoot(tmpDir.GetTempDir()));

	CString gitmodules = tmpDir.GetTempDir() + L"\\.gitmodules";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(gitmodules, L"something"));
	EXPECT_STREQ(tmpDir.GetTempDir(), GitAdminDir::GetSuperProjectRoot(tmpDir.GetTempDir()));
	EXPECT_STREQ(tmpDir.GetTempDir(), GitAdminDir::GetSuperProjectRoot(tmpDir.GetTempDir() + L"\\subdir"));

	// make subdir a nested repository
	EXPECT_TRUE(::CreateDirectory(tmpDir.GetTempDir() + L"\\subdir\\.git", nullptr));
	EXPECT_STREQ(L"", GitAdminDir::GetSuperProjectRoot(tmpDir.GetTempDir() + L"\\subdir"));

	EXPECT_TRUE(::DeleteFile(gitmodules));
	EXPECT_STREQ(L"", GitAdminDir::GetSuperProjectRoot(tmpDir.GetTempDir() + L"\\subdir"));

	gitmodules = tmpDir.GetTempDir() + L"\\subdir\\.gitmodules";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(gitmodules, L"something"));
	EXPECT_STREQ(tmpDir.GetTempDir() + L"\\subdir", GitAdminDir::GetSuperProjectRoot(tmpDir.GetTempDir() + L"\\subdir"));
}
