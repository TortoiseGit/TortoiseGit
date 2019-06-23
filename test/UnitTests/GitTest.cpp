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
#include "Git.h"
#include "StringUtils.h"
#include "RepositoryFixtures.h"

// For performance reason, turn LIBGIT off by default,
INSTANTIATE_TEST_CASE_P(CGit, CBasicGitFixture, testing::Values(GIT_CLI, /*LIBGIT,*/ LIBGIT2, LIBGIT2_ALL));
INSTANTIATE_TEST_CASE_P(CGit, CBasicGitWithEmptyRepositoryFixture, testing::Values(GIT_CLI, /*LIBGIT,*/ LIBGIT2, LIBGIT2_ALL));
INSTANTIATE_TEST_CASE_P(CGit, CBasicGitWithEmptyBareRepositoryFixture, testing::Values(GIT_CLI, /*LIBGIT,*/ LIBGIT2, LIBGIT2_ALL));
INSTANTIATE_TEST_CASE_P(CGit, CBasicGitWithTestRepoFixture, testing::Values(GIT_CLI, LIBGIT, LIBGIT2, LIBGIT2_ALL));
INSTANTIATE_TEST_CASE_P(CGit, CBasicGitWithTestRepoBareFixture, testing::Values(GIT_CLI, LIBGIT, LIBGIT2, LIBGIT2_ALL));
INSTANTIATE_TEST_CASE_P(CGit, CBasicGitWithSubmoduleRepositoryFixture, testing::Values(GIT_CLI, /*LIBGIT,*/ LIBGIT2, LIBGIT2_ALL));

TEST(CGit, RunSet)
{
	CString output;
	CGit cgit;
	ASSERT_EQ(0, cgit.Run(L"cmd /c set", &output, CP_UTF8));
	ASSERT_FALSE(output.IsEmpty());
	ASSERT_TRUE(output.Find(L"windir")); // should be there on any MS OS ;)
}

TEST(CGit, RunGit)
{
	CString output;
	CGit cgit;
	ASSERT_EQ(0, cgit.Run(L"git --version", &output, CP_UTF8));
	ASSERT_FALSE(output.IsEmpty());
}

TEST(CGit, RunGit_BashPipe)
{
	CString tmpfile = GetTempFile();
	tmpfile.Replace(L'\\', L'/');
	ASSERT_TRUE(CStringUtils::WriteStringToTextFile(tmpfile, L"testing piping..."));
	SCOPE_EXIT{ ::DeleteFile(tmpfile); };
	CString pipefile = GetTempFile();
	pipefile.Replace(L'\\', L'/');
	CString pipecmd;
	pipecmd.Format(L"cat < %s", static_cast<LPCTSTR>(tmpfile));
	ASSERT_TRUE(CStringUtils::WriteStringToTextFile(pipefile, pipecmd));
	SCOPE_EXIT{ ::DeleteFile(pipefile); };
	CString output;
	CGit cgit;
	ASSERT_EQ(0, cgit.Run(L"bash.exe " + pipefile, &output, CP_UTF8));
	ASSERT_STREQ(L"testing piping...", output);
}

TEST(CGit, RunGit_Error)
{
	CAutoTempDir tempdir;
	CGit cgit;
	cgit.m_CurrentDir = tempdir.GetTempDir();

	CString output;
	EXPECT_NE(0, cgit.Run(L"git-not-found.exe", &output, CP_UTF8)); // Git for Windows returns 2, cygwin-hack returns 127
	//EXPECT_STREQ(L"", output); with cygwin-hack we get an error message from sh.exe

	output.Empty();
	EXPECT_EQ(128, cgit.Run(L"git.exe add file.txt", &output, CP_UTF8));
	EXPECT_TRUE(CStringUtils::StartsWithI(output, L"fatal: not a git repository (or any"));
}

TEST_P(CBasicGitWithTestRepoBareFixture, RunGit_AbsolutePath)
{
	CAutoTempDir tempdir;

	CString output;
	EXPECT_EQ(0, m_Git.Run(L"git archive -o " + tempdir.GetTempDir() + L"\\export.zip HEAD", &output, CP_UTF8));
	EXPECT_STREQ(L"", output);

	EXPECT_TRUE(PathFileExists(tempdir.GetTempDir() + L"\\export.zip"));
}

TEST(CGit, RunLogFile)
{
	CAutoTempDir tempdir;
	CString tmpfile = tempdir.GetTempDir() + L"\\output.txt";
	CString error;
	CGit cgit;
	ASSERT_EQ(0, cgit.RunLogFile(L"git --version", tmpfile, &error));
	EXPECT_STREQ(L"", error);
	CString fileContents;
	EXPECT_EQ(true, CStringUtils::ReadStringFromTextFile(tmpfile, fileContents));
	EXPECT_TRUE(CStringUtils::StartsWith(fileContents, L"git version "));
}

TEST(CGit, RunLogFile_Set)
{
	CAutoTempDir tempdir;
	CString tmpfile = tempdir.GetTempDir() + L"\\output.txt";
	CString error;
	CGit cgit;
	ASSERT_EQ(0, cgit.RunLogFile(L"cmd /c set", tmpfile, &error));
	EXPECT_STREQ(L"", error);
	CString fileContents;
	EXPECT_EQ(true, CStringUtils::ReadStringFromTextFile(tmpfile, fileContents));
	EXPECT_TRUE(fileContents.Find(L"windir")); // should be there on any MS OS ;)
}

TEST(CGit, RunLogFile_Error)
{
	CAutoTempDir tempdir;
	CString tmpfile = tempdir.GetTempDir() + L"\\output.txt";
	CString error;
	CGit cgit;
	cgit.m_CurrentDir = tempdir.GetTempDir();

	EXPECT_EQ(128, cgit.RunLogFile(L"git.exe add file.txt", tmpfile, &error));
	EXPECT_TRUE(CStringUtils::StartsWithI(error, L"fatal: not a git repository (or any"));
	__int64 size = -1;
	EXPECT_EQ(0, CGit::GetFileModifyTime(tmpfile, nullptr, nullptr, &size));
	EXPECT_EQ(0, size);
}

TEST(CGit, StringAppend)
{
	CGit::StringAppend(nullptr, static_cast<BYTE*>(nullptr)); // string may be null
	CString string = L"something";
	CGit::StringAppend(&string, static_cast<BYTE*>(nullptr), CP_UTF8, 0);
	EXPECT_STREQ(L"something", string);
	const BYTE somebytes[1] = { 0 };
	CGit::StringAppend(&string, somebytes, CP_UTF8, 0);
	EXPECT_STREQ(L"something", string);
	CGit::StringAppend(&string, somebytes);
	EXPECT_STREQ(L"something", string);
	const BYTE moreBytesUTFEight[] = { 0x68, 0x65, 0x6C, 0x6C, 0xC3, 0xB6, 0x0A, 0x00 };
	CGit::StringAppend(&string, moreBytesUTFEight, CP_UTF8, 3);
	EXPECT_STREQ(L"somethinghel", string);
	CGit::StringAppend(&string, moreBytesUTFEight + 3, CP_ACP, 1);
	EXPECT_STREQ(L"somethinghell", string);
	CGit::StringAppend(&string, moreBytesUTFEight);
	EXPECT_STREQ(L"somethinghellhellö\n", string);
	CGit::StringAppend(&string, moreBytesUTFEight, CP_UTF8, sizeof(moreBytesUTFEight));
	EXPECT_STREQ(L"somethinghellhellö\nhellö\n\0", string);
	CGit::StringAppend(&string, moreBytesUTFEight, CP_UTF8, 3);
	EXPECT_STREQ(L"somethinghellhellö\nhellö\n\0hel", string);
}

TEST(CGit, GetFileModifyTime)
{
	__int64 time = -1;
	bool isDir = false;
	__int64 size = -1;
	EXPECT_EQ(-1, CGit::GetFileModifyTime(L"does-not-exist.txt", &time, &isDir, &size));

	time = -1;
	isDir = false;
	size = -1;
	EXPECT_EQ(0, CGit::GetFileModifyTime(L"c:\\Windows", &time, &isDir, &size));
	EXPECT_TRUE(isDir);

	time = -1;
	isDir = false;
	size = -1;
	EXPECT_EQ(0, CGit::GetFileModifyTime(L"c:\\Windows\\", &time, &isDir, &size));
	EXPECT_TRUE(isDir);

	CAutoTempDir tempdir;
	CString testFile = tempdir.GetTempDir() + L"\\test.txt";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(testFile, L"this is testing fileöäü."));

	time = -1;
	EXPECT_EQ(0, CGit::GetFileModifyTime(testFile, &time));
	EXPECT_NE(-1, time);

	__int64 time2 = -1;
	isDir = false;
	size = -1;
	ULONGLONG ticks = GetTickCount64();
	EXPECT_EQ(0, CGit::GetFileModifyTime(testFile, &time2, &isDir, &size));
	EXPECT_EQ(time, time2);
	EXPECT_FALSE(isDir);
	EXPECT_EQ(27, size);

	Sleep(250);
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(testFile, L"this is testing fileöü."));
	__int64 time3 = -1;
	isDir = false;
	size = -1;
	EXPECT_EQ(0, CGit::GetFileModifyTime(testFile, &time3, &isDir, &size));
	EXPECT_NE(-1, time3);
	EXPECT_FALSE(isDir);
	EXPECT_EQ(25, size);
	EXPECT_GE(time3, time);
	EXPECT_LE(time3 - time, (__int64)(GetTickCount64() - ticks + 10) * 10000);
}

TEST(CGit, LoadTextFile)
{
	CAutoTempDir tempdir;

	CString msg = L"something--";
	EXPECT_FALSE(CGit::LoadTextFile(L"does-not-exist.txt", msg));
	EXPECT_STREQ(L"something--", msg);

	CString testFile = tempdir.GetTempDir() + L"\\test.txt";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(testFile, L"this is testing fileöäü."));
	EXPECT_TRUE(CGit::LoadTextFile(testFile, msg));
	EXPECT_STREQ(L"something--this is testing fileöäü.\n", msg);

	msg.Empty();
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(testFile, L"this is testing\nfileöäü."));
	EXPECT_TRUE(CGit::LoadTextFile(testFile, msg));
	EXPECT_STREQ(L"this is testing\nfileöäü.\n", msg);

	msg.Empty();
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(testFile, L"this is\r\ntesting\nfileöäü.\r\n\r\n"));
	EXPECT_TRUE(CGit::LoadTextFile(testFile, msg));
	EXPECT_STREQ(L"this is\ntesting\nfileöäü.\n", msg);
}

TEST(CGit, IsBranchNameValid)
{
	CGit cgit;
	EXPECT_TRUE(cgit.IsBranchNameValid(L"master"));
	EXPECT_TRUE(cgit.IsBranchNameValid(L"def/master"));
	EXPECT_FALSE(cgit.IsBranchNameValid(L"HEAD"));
	EXPECT_FALSE(cgit.IsBranchNameValid(L"-test"));
	EXPECT_FALSE(cgit.IsBranchNameValid(L"jfjf>ff"));
	EXPECT_FALSE(cgit.IsBranchNameValid(L"jf ff"));
	EXPECT_FALSE(cgit.IsBranchNameValid(L"jf~ff"));
}

TEST(CGit, StripRefName)
{
	EXPECT_STREQ(L"abc", CGit::StripRefName(L"abc"));
	EXPECT_STREQ(L"bcd", CGit::StripRefName(L"refs/bcd"));
	EXPECT_STREQ(L"cde", CGit::StripRefName(L"refs/heads/cde"));

	EXPECT_STREQ(L"notes/commits", CGit::StripRefName(L"refs/notes/commits"));
	EXPECT_STREQ(L"remotes/origin/abc", CGit::StripRefName(L"refs/remotes/origin/abc"));
	EXPECT_STREQ(L"tags/abc", CGit::StripRefName(L"refs/tags/abc"));
}

TEST(CGit, CombinePath)
{
	CGit cgit;
	cgit.m_CurrentDir = L"c:\\something";
	EXPECT_STREQ(L"c:\\something", cgit.CombinePath(L""));
	EXPECT_STREQ(L"c:\\something\\file.txt", cgit.CombinePath(L"file.txt"));
	EXPECT_STREQ(L"c:\\something\\sub\\file.txt", cgit.CombinePath(L"sub\\file.txt"));
	EXPECT_STREQ(L"c:\\something\\subdir\\file2.txt", cgit.CombinePath(CTGitPath(L"subdir/file2.txt")));
}

TEST(CGit, GetShortName)
{
	CGit::REF_TYPE type = CGit::UNKNOWN;
	EXPECT_STREQ(L"master", CGit::GetShortName(L"refs/heads/master", &type));
	EXPECT_EQ(CGit::LOCAL_BRANCH, type);

	type = CGit::UNKNOWN;
	EXPECT_STREQ(L"somedir/mastr", CGit::GetShortName(L"refs/heads/somedir/mastr", &type));
	EXPECT_EQ(CGit::LOCAL_BRANCH, type);

	type = CGit::BISECT_BAD; // do not use UNKNOWN here to make sure it gets set
	EXPECT_STREQ(L"svn/something", CGit::GetShortName(L"refs/svn/something", &type));
	EXPECT_EQ(CGit::UNKNOWN, type);

	type = CGit::UNKNOWN;
	EXPECT_STREQ(L"origin/master", CGit::GetShortName(L"refs/remotes/origin/master", &type));
	EXPECT_EQ(CGit::REMOTE_BRANCH, type);

	type = CGit::UNKNOWN;
	EXPECT_STREQ(L"origin/sub/master", CGit::GetShortName(L"refs/remotes/origin/sub/master", &type));
	EXPECT_EQ(CGit::REMOTE_BRANCH, type);

	type = CGit::UNKNOWN;
	EXPECT_STREQ(L"release1", CGit::GetShortName(L"refs/tags/release1", &type));
	EXPECT_EQ(CGit::TAG, type);

	type = CGit::UNKNOWN;
	EXPECT_STREQ(L"releases/v1", CGit::GetShortName(L"refs/tags/releases/v1", &type));
	EXPECT_EQ(CGit::TAG, type);

	type = CGit::UNKNOWN;
	EXPECT_STREQ(L"release2", CGit::GetShortName(L"refs/tags/release2^{}", &type));
	EXPECT_EQ(CGit::ANNOTATED_TAG, type);

	type = CGit::UNKNOWN;
	EXPECT_STREQ(L"releases/v2", CGit::GetShortName(L"refs/tags/releases/v2^{}", &type));
	EXPECT_EQ(CGit::ANNOTATED_TAG, type);

	type = CGit::UNKNOWN;
	EXPECT_STREQ(L"stash", CGit::GetShortName(L"refs/stash", &type));
	EXPECT_EQ(CGit::STASH, type);

	type = CGit::BISECT_BAD; // do not use UNKNOWN here to make sure it gets set
	EXPECT_STREQ(L"something", CGit::GetShortName(L"refs/something", &type));
	EXPECT_EQ(CGit::UNKNOWN, type);

	type = CGit::BISECT_BAD; // do not use UNKNOWN here to make sure it gets set
	EXPECT_STREQ(L"sth", CGit::GetShortName(L"sth", &type));
	EXPECT_EQ(CGit::UNKNOWN, type);

	type = CGit::UNKNOWN;
	EXPECT_STREQ(L"good", CGit::GetShortName(L"refs/bisect/good", &type));
	EXPECT_EQ(CGit::BISECT_GOOD, type);

	type = CGit::UNKNOWN;
	EXPECT_STREQ(L"good", CGit::GetShortName(L"refs/bisect/good-5809ac97a1115a8380b1d6bb304b62cd0b0fa9bb", &type));
	EXPECT_EQ(CGit::BISECT_GOOD, type);

	type = CGit::UNKNOWN;
	EXPECT_STREQ(L"bad", CGit::GetShortName(L"refs/bisect/bad", &type));
	EXPECT_EQ(CGit::BISECT_BAD, type);

	type = CGit::UNKNOWN;
	EXPECT_STREQ(L"bad", CGit::GetShortName(L"refs/bisect/bad-5809ac97a1115a8380b1d6bb304b62cd0b0fd9bb", &type));
	EXPECT_EQ(CGit::BISECT_BAD, type);

	type = CGit::UNKNOWN;
	EXPECT_STREQ(L"ab", CGit::GetShortName(L"refs/notes/ab", &type));
	EXPECT_EQ(CGit::NOTES, type);

	type = CGit::UNKNOWN;
	EXPECT_STREQ(L"a/b", CGit::GetShortName(L"refs/notes/a/b", &type));
	EXPECT_EQ(CGit::NOTES, type);
}

TEST(CGit, GetRepository)
{
	CAutoTempDir tempdir;
	CGit cgit;
	cgit.m_CurrentDir = tempdir.GetTempDir();

	CAutoRepository repo = cgit.GetGitRepository();
	EXPECT_FALSE(repo.IsValid());

	cgit.m_CurrentDir = tempdir.GetTempDir() + L"\\aöäüb";
	ASSERT_TRUE(CreateDirectory(cgit.m_CurrentDir, nullptr));

	CString output;
	EXPECT_EQ(0, cgit.Run(L"git.exe init", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);

	CAutoRepository repo2 = cgit.GetGitRepository(); // this tests GetGitRepository as well as m_Git.GetGitPathStringA
	EXPECT_TRUE(repo2.IsValid());

	cgit.m_CurrentDir = tempdir.GetTempDir() + L"\\aöäüb.git";
	ASSERT_TRUE(CreateDirectory(cgit.m_CurrentDir, nullptr));

	output.Empty();
	EXPECT_EQ(0, cgit.Run(L"git.exe init --bare", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);

	CAutoRepository repo3 = cgit.GetGitRepository(); // this tests GetGitRepository as well as m_Git.GetGitPathStringA
	EXPECT_TRUE(repo3.IsValid());
}

TEST_P(CBasicGitWithEmptyRepositoryFixture, IsInitRepos_GetInitAddList)
{
	EXPECT_STREQ(L"master", m_Git.GetCurrentBranch());

	CString output;
	CString testFile = m_Dir.GetTempDir() + L"\\test.txt";

	CTGitPathList addedFiles;

	EXPECT_TRUE(m_Git.IsInitRepos());
	EXPECT_EQ(0, m_Git.GetInitAddList(addedFiles));
	EXPECT_TRUE(addedFiles.IsEmpty());

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(testFile, L"this is testing file."));
	EXPECT_EQ(0, m_Git.GetInitAddList(addedFiles));
	EXPECT_TRUE(addedFiles.IsEmpty());
	EXPECT_EQ(0, m_Git.Run(L"git.exe add test.txt", &output, CP_UTF8));
	EXPECT_STREQ(L"", output);
	EXPECT_EQ(0, m_Git.GetInitAddList(addedFiles));
	ASSERT_EQ(1, addedFiles.GetCount());
	EXPECT_STREQ(L"test.txt", addedFiles[0].GetGitPathString());

	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe commit -m \"Add test.txt\"", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);

	EXPECT_FALSE(m_Git.IsInitRepos());

	EXPECT_STREQ(L"master", m_Git.GetCurrentBranch());
}

TEST_P(CBasicGitWithTestRepoFixture, IsInitRepos)
{
	EXPECT_FALSE(m_Git.IsInitRepos());

	CString output;
	EXPECT_EQ(0, m_Git.Run(L"git.exe checkout --orphan orphanic", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);

	EXPECT_TRUE(m_Git.IsInitRepos());
}

TEST_P(CBasicGitWithTestRepoFixture, HasWorkingTreeConflicts)
{
	CString output;
	EXPECT_EQ(0, m_Git.Run(L"git.exe reset --hard master", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);

	EXPECT_EQ(FALSE, m_Git.HasWorkingTreeConflicts());

	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe merge forconflict", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);
	EXPECT_EQ(FALSE, m_Git.HasWorkingTreeConflicts());

	output.Empty();
	EXPECT_EQ(1, m_Git.Run(L"git.exe merge simple-conflict", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);
	EXPECT_EQ(TRUE, m_Git.HasWorkingTreeConflicts());

	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe reset --hard master", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);

	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe checkout forconflict", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);

	output.Empty();
	EXPECT_EQ(1, m_Git.Run(L"git.exe merge simple-conflict", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);
	EXPECT_EQ(TRUE, m_Git.HasWorkingTreeConflicts());
}

TEST_P(CBasicGitWithTestRepoFixture, GetCurrentBranch)
{
	EXPECT_STREQ(L"master", m_Git.GetCurrentBranch());
	EXPECT_STREQ(L"master", m_Git.GetCurrentBranch(true));

	CString output;
	EXPECT_EQ(0, m_Git.Run(L"git.exe checkout simple-conflict", &output, CP_UTF8));
	EXPECT_STREQ(L"simple-conflict", m_Git.GetCurrentBranch());
	EXPECT_STREQ(L"simple-conflict", m_Git.GetCurrentBranch(true));

	EXPECT_EQ(0, m_Git.Run(L"git.exe checkout subdir/branch", &output, CP_UTF8));
	EXPECT_STREQ(L"subdir/branch", m_Git.GetCurrentBranch());
	EXPECT_STREQ(L"subdir/branch", m_Git.GetCurrentBranch(true));

	EXPECT_EQ(0, m_Git.Run(L"git.exe checkout 560deea87853158b22d0c0fd73f60a458d47838a", &output, CP_UTF8));
	EXPECT_STREQ(L"(no branch)", m_Git.GetCurrentBranch());
	EXPECT_STREQ(L"560deea87853158b22d0c0fd73f60a458d47838a", m_Git.GetCurrentBranch(true));

	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe checkout --orphan orphanic", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);

	EXPECT_STREQ(L"orphanic", m_Git.GetCurrentBranch());
	EXPECT_STREQ(L"orphanic", m_Git.GetCurrentBranch(true));
}

TEST_P(CBasicGitWithTestRepoBareFixture, GetCurrentBranch)
{
	EXPECT_STREQ(L"master", m_Git.GetCurrentBranch());
	EXPECT_STREQ(L"master", m_Git.GetCurrentBranch(true));
}

static void IsLocalBranch(CGit& m_Git)
{
	EXPECT_TRUE(m_Git.IsLocalBranch(L"master"));
	EXPECT_TRUE(m_Git.IsLocalBranch(L"subdir/branch"));

	EXPECT_FALSE(m_Git.IsLocalBranch(L"no_branch_in_repo"));

	EXPECT_FALSE(m_Git.IsLocalBranch(L"commits")); // notes/commits

	EXPECT_FALSE(m_Git.IsLocalBranch(L"stash"));

	EXPECT_FALSE(m_Git.IsLocalBranch(L"3686b9cf74f1a4ef96d6bfe736595ef9abf0fb8d"));

	// exist tags
	EXPECT_FALSE(m_Git.IsLocalBranch(L"normal-tag"));
	EXPECT_FALSE(m_Git.IsLocalBranch(L"also-signed"));
}

TEST_P(CBasicGitWithTestRepoFixture, IsLocalBranch)
{
	IsLocalBranch(m_Git);
}

TEST_P(CBasicGitWithTestRepoBareFixture, IsLocalBranch)
{
	IsLocalBranch(m_Git);
}

static void BranchTagExists_IsBranchTagNameUnique(CGit& m_Git)
{
	EXPECT_TRUE(m_Git.BranchTagExists(L"master", true));
	EXPECT_FALSE(m_Git.BranchTagExists(L"origin/master", true));
	EXPECT_FALSE(m_Git.BranchTagExists(L"normal-tag", true));
	EXPECT_FALSE(m_Git.BranchTagExists(L"also-signed", true));
	EXPECT_FALSE(m_Git.BranchTagExists(L"wuseldusel", true));

	EXPECT_FALSE(m_Git.BranchTagExists(L"master", false));
	EXPECT_TRUE(m_Git.BranchTagExists(L"normal-tag", false));
	EXPECT_TRUE(m_Git.BranchTagExists(L"also-signed", false));
	EXPECT_FALSE(m_Git.BranchTagExists(L"wuseldusel", false));

	EXPECT_TRUE(m_Git.IsBranchTagNameUnique(L"master"));
	EXPECT_TRUE(m_Git.IsBranchTagNameUnique(L"simpleconflict"));
	EXPECT_TRUE(m_Git.IsBranchTagNameUnique(L"normal-tag"));
	EXPECT_TRUE(m_Git.IsBranchTagNameUnique(L"also-signed"));
	EXPECT_TRUE(m_Git.IsBranchTagNameUnique(L"origin/master"));

	CString output;
	EXPECT_EQ(0, m_Git.Run(L"git.exe tag master HEAD~2", &output, CP_UTF8));
	EXPECT_STREQ(L"", output);

	EXPECT_EQ(0, m_Git.Run(L"git.exe branch normal-tag HEAD~2", &output, CP_UTF8));
	EXPECT_STREQ(L"", output);

	EXPECT_FALSE(m_Git.IsBranchTagNameUnique(L"master"));
	EXPECT_FALSE(m_Git.IsBranchTagNameUnique(L"normal-tag"));
	EXPECT_TRUE(m_Git.IsBranchTagNameUnique(L"also-signed"));
}

TEST_P(CBasicGitWithTestRepoFixture, BranchTagExists_IsBranchTagNameUnique)
{
	BranchTagExists_IsBranchTagNameUnique(m_Git);
}

TEST_P(CBasicGitWithTestRepoBareFixture, BranchTagExists_IsBranchTagNameUnique)
{
	BranchTagExists_IsBranchTagNameUnique(m_Git);
}

static void GetFullRefName(CGit& m_Git)
{
	EXPECT_STREQ(L"", m_Git.GetFullRefName(L"does_not_exist"));
	EXPECT_STREQ(L"refs/heads/master", m_Git.GetFullRefName(L"master"));
	EXPECT_STREQ(L"refs/remotes/origin/master", m_Git.GetFullRefName(L"origin/master"));
	EXPECT_STREQ(L"refs/tags/normal-tag", m_Git.GetFullRefName(L"normal-tag"));
	EXPECT_STREQ(L"refs/tags/also-signed", m_Git.GetFullRefName(L"also-signed"));

	CString output;
	EXPECT_EQ(0, m_Git.Run(L"git.exe tag master HEAD~2", &output, CP_UTF8));
	EXPECT_STREQ(L"", output);
	EXPECT_STREQ(L"", m_Git.GetFullRefName(L"master"));
	EXPECT_STREQ(L"refs/remotes/origin/master", m_Git.GetFullRefName(L"origin/master"));

	EXPECT_EQ(0, m_Git.Run(L"git.exe branch normal-tag HEAD~2", &output, CP_UTF8));
	EXPECT_STREQ(L"", output);
	EXPECT_STREQ(L"", m_Git.GetFullRefName(L"normal-tag"));

	EXPECT_EQ(0, m_Git.Run(L"git.exe branch origin/master HEAD~2", &output, CP_UTF8));
	EXPECT_STREQ(L"", output);
	EXPECT_STREQ(L"", m_Git.GetFullRefName(L"origin/master"));
}

TEST_P(CBasicGitWithTestRepoFixture, GetFullRefName)
{
	GetFullRefName(m_Git);

	CString output;
	EXPECT_EQ(0, m_Git.Run(L"git.exe checkout --orphan orphanic", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);
	EXPECT_STREQ(L"", m_Git.GetFullRefName(L"orphanic"));
}

TEST_P(CBasicGitWithTestRepoBareFixture, GetFullRefName)
{
	GetFullRefName(m_Git);
}

TEST_P(CBasicGitWithEmptyRepositoryFixture, GetRemoteTrackedBranch)
{
	CString remote, branch;
	m_Git.GetRemoteTrackedBranchForHEAD(remote, branch);
	EXPECT_STREQ(L"", remote);
	EXPECT_STREQ(L"", branch);

	m_Git.GetRemoteTrackedBranch(L"master", remote, branch);
	EXPECT_STREQ(L"", remote);
	EXPECT_STREQ(L"", branch);

	m_Git.GetRemoteTrackedBranch(L"non-existing", remote, branch);
	EXPECT_STREQ(L"", remote);
	EXPECT_STREQ(L"", branch);
}

static void GetRemoteTrackedBranch(CGit& m_Git)
{
	CString remote, branch;
	m_Git.GetRemoteTrackedBranchForHEAD(remote, branch);
	EXPECT_STREQ(L"origin", remote);
	EXPECT_STREQ(L"master", branch);

	remote.Empty();
	branch.Empty();
	m_Git.GetRemoteTrackedBranch(L"master", remote, branch);
	EXPECT_STREQ(L"origin", remote);
	EXPECT_STREQ(L"master", branch);

	remote.Empty();
	branch.Empty();
	m_Git.GetRemoteTrackedBranch(L"non-existing", remote, branch);
	EXPECT_STREQ(L"", remote);
	EXPECT_STREQ(L"", branch);
}

TEST_P(CBasicGitWithTestRepoFixture, GetRemoteTrackedBranch)
{
	GetRemoteTrackedBranch(m_Git);
}

TEST_P(CBasicGitWithTestRepoBareFixture, GetRemoteTrackedBranch)
{
	GetRemoteTrackedBranch(m_Git);
}

TEST_P(CBasicGitWithEmptyRepositoryFixture, GetRemotePushBranch)
{
	CString remote, branch;
	m_Git.GetRemotePushBranch(L"master", remote, branch);
	EXPECT_STREQ(L"", remote);
	EXPECT_STREQ(L"", branch);

	m_Git.GetRemotePushBranch(L"non-existing", remote, branch);
	EXPECT_STREQ(L"", remote);
	EXPECT_STREQ(L"", branch);
}

static void GetRemotePushBranch(CGit& m_Git)
{
	CString remote, branch;
	m_Git.GetRemotePushBranch(L"master", remote, branch);
	EXPECT_STREQ(L"origin", remote);
	EXPECT_STREQ(L"master", branch);

	remote.Empty();
	branch.Empty();
	m_Git.GetRemotePushBranch(L"non-existing", remote, branch);
	EXPECT_STREQ(L"", remote);
	EXPECT_STREQ(L"", branch);

	CAutoRepository repo(m_Git.GetGitRepository());
	ASSERT_TRUE(repo.IsValid());
	CAutoConfig config(repo);
	ASSERT_TRUE(config.IsValid());

	remote.Empty();
	branch.Empty();
	EXPECT_EQ(0, git_config_set_string(config, "remote.pushDefault", "originpush2"));
	m_Git.GetRemotePushBranch(L"master", remote, branch);
	EXPECT_STREQ(L"originpush2", remote);
	EXPECT_STREQ(L"master", branch);

	remote.Empty();
	branch.Empty();
	EXPECT_EQ(0, git_config_set_string(config, "branch.master.pushremote", "originpush3"));
	m_Git.GetRemotePushBranch(L"master", remote, branch);
	EXPECT_STREQ(L"originpush3", remote);
	EXPECT_STREQ(L"master", branch);

	remote.Empty();
	branch.Empty();
	EXPECT_EQ(0, git_config_set_string(config, "branch.master.pushbranch", "masterbranch2"));
	m_Git.GetRemotePushBranch(L"master", remote, branch);
	EXPECT_STREQ(L"originpush3", remote);
	EXPECT_STREQ(L"masterbranch2", branch);

	remote.Empty();
	branch.Empty();
	m_Git.GetRemotePushBranch(L"non-existing", remote, branch);
	EXPECT_STREQ(L"originpush2", remote);
	EXPECT_STREQ(L"", branch);
}

TEST_P(CBasicGitWithTestRepoFixture, GetRemotePushBranch)
{
	GetRemotePushBranch(m_Git);
}

TEST_P(CBasicGitWithTestRepoBareFixture, GetRemotePushBranch)
{
	GetRemotePushBranch(m_Git);
}

static void CanParseRev(CGit& m_Git)
{
	EXPECT_TRUE(m_Git.CanParseRev(L""));
	EXPECT_TRUE(m_Git.CanParseRev(L"HEAD"));
	EXPECT_TRUE(m_Git.CanParseRev(L"master"));
	EXPECT_TRUE(m_Git.CanParseRev(L"heads/master"));
	EXPECT_TRUE(m_Git.CanParseRev(L"refs/heads/master"));
	EXPECT_TRUE(m_Git.CanParseRev(L"master~1"));
	EXPECT_TRUE(m_Git.CanParseRev(L"master forconflict"));
	EXPECT_TRUE(m_Git.CanParseRev(L"origin/master..master"));
	EXPECT_TRUE(m_Git.CanParseRev(L"origin/master...master"));
	EXPECT_TRUE(m_Git.CanParseRev(L"49ecdfff36bfe2b9b499b33e5034f427e2fa54dd"));
	EXPECT_FALSE(m_Git.CanParseRev(L"non-existing"));
	EXPECT_TRUE(m_Git.CanParseRev(L"normal-tag"));
	EXPECT_TRUE(m_Git.CanParseRev(L"tags/normal-tag"));
	EXPECT_TRUE(m_Git.CanParseRev(L"refs/tags/normal-tag"));
	EXPECT_TRUE(m_Git.CanParseRev(L"all-files-signed"));
	EXPECT_TRUE(m_Git.CanParseRev(L"all-files-signed^{}"));

	EXPECT_FALSE(m_Git.CanParseRev(L"orphanic"));
}

TEST_P(CBasicGitWithTestRepoFixture, CanParseRev)
{
	CanParseRev(m_Git);

	CString output;
	EXPECT_EQ(0, m_Git.Run(L"git.exe checkout --orphan orphanic", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);
	EXPECT_FALSE(m_Git.CanParseRev(L""));
	EXPECT_FALSE(m_Git.CanParseRev(L"HEAD"));
	EXPECT_FALSE(m_Git.CanParseRev(L"orphanic"));
	EXPECT_TRUE(m_Git.CanParseRev(L"master"));
}

TEST_P(CBasicGitWithTestRepoBareFixture, CanParseRev)
{
	CanParseRev(m_Git);
}

static void FETCHHEAD(CGit& m_Git, bool isBare)
{
	CString repoDir = m_Git.m_CurrentDir;
	if (!isBare)
		repoDir += L"\\.git";

	STRING_VECTOR list;
	EXPECT_EQ(0, m_Git.GetBranchList(list, nullptr));
	EXPECT_EQ(6U, list.size());
	list.clear();
	EXPECT_EQ(0, m_Git.GetBranchList(list, nullptr, CGit::BRANCH_LOCAL_F));
	EXPECT_EQ(6U, list.size());

	EXPECT_STREQ(L"HEAD", m_Git.FixBranchName(L"HEAD"));
	EXPECT_STREQ(L"master", m_Git.FixBranchName(L"master"));
	EXPECT_STREQ(L"non-existing", m_Git.FixBranchName(L"non-existing"));
	CString branch = L"master";
	EXPECT_STREQ(L"master", m_Git.FixBranchName_Mod(branch));
	EXPECT_STREQ(L"master", branch);
	branch = L"non-existing";
	EXPECT_STREQ(L"non-existing", m_Git.FixBranchName_Mod(branch));
	EXPECT_STREQ(L"non-existing", branch);
	CGitHash hash;
	EXPECT_NE(0, m_Git.GetHash(hash, L"FETCH_HEAD"));

	CString testFile = repoDir + L"\\FETCH_HEAD";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(testFile, L"b9ef30183497cdad5c30b88d32dc1bed7951dfeb		branch 'master' of https://code.google.com/p/tortoisegit\n737878a4e2eabfa4fab580867c2b060c70999d31	not-for-merge	branch 'extend_hooks' of https://code.google.com/p/tortoisegit\n"));

	list.clear();
	EXPECT_EQ(0, m_Git.GetBranchList(list, nullptr));
	EXPECT_EQ(6U, list.size());
	list.clear();
	EXPECT_EQ(0, m_Git.GetBranchList(list, nullptr, CGit::BRANCH_LOCAL_F));
	EXPECT_EQ(7U, list.size());

	EXPECT_STREQ(L"master", m_Git.FixBranchName(L"master"));
	EXPECT_STREQ(L"non-existing", m_Git.FixBranchName(L"non-existing"));
	EXPECT_STREQ(L"b9ef30183497cdad5c30b88d32dc1bed7951dfeb", m_Git.FixBranchName(L"FETCH_HEAD"));
	branch = L"HEAD";
	EXPECT_STREQ(L"HEAD", m_Git.FixBranchName_Mod(branch));
	EXPECT_STREQ(L"HEAD", branch);
	branch = L"master";
	EXPECT_STREQ(L"master", m_Git.FixBranchName_Mod(branch));
	EXPECT_STREQ(L"master", branch);
	branch = L"non-existing";
	EXPECT_STREQ(L"non-existing", m_Git.FixBranchName_Mod(branch));
	EXPECT_STREQ(L"non-existing", branch);
	branch = L"FETCH_HEAD";
	EXPECT_STREQ(L"b9ef30183497cdad5c30b88d32dc1bed7951dfeb", m_Git.FixBranchName_Mod(branch));
	EXPECT_STREQ(L"b9ef30183497cdad5c30b88d32dc1bed7951dfeb", branch);
	EXPECT_EQ(0, m_Git.GetHash(hash, L"FETCH_HEAD"));
	EXPECT_STREQ(L"b9ef30183497cdad5c30b88d32dc1bed7951dfeb", hash.ToString());

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(testFile, L"737878a4e2eabfa4fab580867c2b060c70999d31	not-for-merge	branch 'extend_hooks' of https://code.google.com/p/tortoisegit\nb9ef30183497cdad5c30b88d32dc1bed7951dfeb		branch 'master' of https://code.google.com/p/tortoisegit\n"));

	list.clear();
	EXPECT_EQ(0, m_Git.GetBranchList(list, nullptr));
	EXPECT_EQ(6U, list.size());
	list.clear();
	EXPECT_EQ(0, m_Git.GetBranchList(list, nullptr, CGit::BRANCH_LOCAL_F));
	EXPECT_EQ(7U, list.size());

	EXPECT_STREQ(L"b9ef30183497cdad5c30b88d32dc1bed7951dfeb", m_Git.FixBranchName(L"FETCH_HEAD"));
	branch = L"FETCH_HEAD";
	EXPECT_STREQ(L"b9ef30183497cdad5c30b88d32dc1bed7951dfeb", m_Git.FixBranchName_Mod(branch));
	EXPECT_STREQ(L"b9ef30183497cdad5c30b88d32dc1bed7951dfeb", branch);
	// libgit2 fails here
	//EXPECT_EQ(0, m_Git.GetHash(hash, L"FETCH_HEAD"));
	//EXPECT_STREQ(L"b9ef30183497cdad5c30b88d32dc1bed7951dfeb", hash.ToString());
}

TEST_P(CBasicGitWithTestRepoFixture, FETCHHEAD)
{
	FETCHHEAD(m_Git, false);
}

TEST_P(CBasicGitWithTestRepoBareFixture, FETCHHEAD)
{
	FETCHHEAD(m_Git, true);
}

TEST_P(CBasicGitWithTestRepoFixture, IsFastForward)
{
	CGitHash commonAncestor;
	EXPECT_TRUE(m_Git.IsFastForward(L"origin/master", L"master", &commonAncestor));
	EXPECT_STREQ(L"a9d53b535cb49640a6099860ac4999f5a0857b91", commonAncestor.ToString());

	EXPECT_FALSE(m_Git.IsFastForward(L"simple-conflict", L"master", &commonAncestor));
	EXPECT_STREQ(L"b02add66f48814a73aa2f0876d6bbc8662d6a9a8", commonAncestor.ToString());
}

static void GetHash(CGit& m_Git)
{
	CGitHash hash;
	EXPECT_EQ(0, m_Git.GetHash(hash, L"HEAD"));
	EXPECT_STREQ(L"7c3cbfe13a929d2291a574dca45e4fd2d2ac1aa6", hash.ToString());
	EXPECT_EQ(0, m_Git.GetHash(hash, L"HEAD~1"));
	EXPECT_STREQ(L"1fc3c9688e27596d8717b54f2939dc951568f6cb", hash.ToString());
	EXPECT_EQ(0, m_Git.GetHash(hash, L"ff1fbef1a54a9849afd4a5e94d2ca4d80d5b96c2"));
	EXPECT_STREQ(L"ff1fbef1a54a9849afd4a5e94d2ca4d80d5b96c2", hash.ToString());
	EXPECT_EQ(0, m_Git.GetHash(hash, L"master"));
	EXPECT_STREQ(L"7c3cbfe13a929d2291a574dca45e4fd2d2ac1aa6", hash.ToString());
	EXPECT_EQ(0, m_Git.GetHash(hash, L"origin/master"));
	EXPECT_STREQ(L"a9d53b535cb49640a6099860ac4999f5a0857b91", hash.ToString());
	EXPECT_EQ(0, m_Git.GetHash(hash, L"49ecdfff36bfe2b9b499b33e5034f427e2fa54dd"));
	EXPECT_STREQ(L"49ecdfff36bfe2b9b499b33e5034f427e2fa54dd", hash.ToString());
	EXPECT_EQ(0, m_Git.GetHash(hash, L"normal-tag"));
	EXPECT_STREQ(L"b9ef30183497cdad5c30b88d32dc1bed7951dfeb", hash.ToString());
	EXPECT_EQ(0, m_Git.GetHash(hash, L"all-files-signed"));
	EXPECT_STREQ(L"ab555b2776c6b700ad93848d0dd050e7d08be779", hash.ToString()); // maybe we need automatically to dereference it
	EXPECT_EQ(0, m_Git.GetHash(hash, L"all-files-signed^{}"));
	EXPECT_STREQ(L"313a41bc88a527289c87d7531802ab484715974f", hash.ToString());

	EXPECT_NE(0, m_Git.GetHash(hash, L"non-existing"));
}

TEST_P(CBasicGitWithTestRepoFixture, GetHash)
{
	GetHash(m_Git);
}

TEST_P(CBasicGitWithTestRepoBareFixture, GetHash)
{
	GetHash(m_Git);
}

TEST_P(CBasicGitWithEmptyRepositoryFixture, GetHash_EmptyRepo)
{
	CGitHash hash;
	EXPECT_EQ(0, m_Git.GetHash(hash, L"HEAD"));
	EXPECT_TRUE(hash.IsEmpty());
}

TEST_P(CBasicGitWithEmptyRepositoryFixture, GetEmptyBranchesTagsRefs)
{
	STRING_VECTOR branches;
	int current = -2;
	EXPECT_EQ(0, m_Git.GetBranchList(branches, &current));
	EXPECT_TRUE(branches.empty());
	EXPECT_EQ(-2, current); // not touched

	EXPECT_EQ(0, m_Git.GetBranchList(branches, &current, CGit::BRANCH_ALL));
	EXPECT_TRUE(branches.empty());
	EXPECT_EQ(-2, current); // not touched

	STRING_VECTOR tags;
	EXPECT_EQ(0, m_Git.GetTagList(tags));
	EXPECT_TRUE(tags.empty());

	STRING_VECTOR refs;
	EXPECT_EQ(0, m_Git.GetRefList(refs));
	EXPECT_TRUE(refs.empty());

	MAP_HASH_NAME map;
	EXPECT_EQ(0, m_Git.GetMapHashToFriendName(map));
	EXPECT_TRUE(map.empty());

	STRING_VECTOR remotes;
	EXPECT_EQ(0, m_Git.GetRemoteList(remotes));
	EXPECT_TRUE(remotes.empty());
}

static void GetBranchesTagsRefs(CGit& m_Git, config testConfig)
{
	STRING_VECTOR branches;
	int current = -2;
	EXPECT_EQ(0, m_Git.GetBranchList(branches, &current));
	ASSERT_EQ(6U, branches.size());
	EXPECT_EQ(1, current);
	EXPECT_STREQ(L"forconflict", branches[0]);
	EXPECT_STREQ(L"master", branches[1]);
	EXPECT_STREQ(L"master2", branches[2]);
	EXPECT_STREQ(L"signed-commit", branches[3]);
	EXPECT_STREQ(L"simple-conflict", branches[4]);
	EXPECT_STREQ(L"subdir/branch", branches[5]);

	branches.clear();
	current = -2;
	EXPECT_EQ(0, m_Git.GetBranchList(branches, &current, CGit::BRANCH_ALL));
	ASSERT_EQ(7U, branches.size());
	EXPECT_EQ(1, current);
	EXPECT_STREQ(L"forconflict", branches[0]);
	EXPECT_STREQ(L"master", branches[1]);
	EXPECT_STREQ(L"master2", branches[2]);
	EXPECT_STREQ(L"signed-commit", branches[3]);
	EXPECT_STREQ(L"simple-conflict", branches[4]);
	EXPECT_STREQ(L"subdir/branch", branches[5]);
	EXPECT_STREQ(L"remotes/origin/master", branches[6]);

	branches.clear();
	current = -2;
	EXPECT_EQ(0, m_Git.GetBranchList(branches, &current, CGit::BRANCH_ALL, true));
	ASSERT_EQ(6U, branches.size());
	EXPECT_EQ(-2, current); // not touched
	EXPECT_STREQ(L"forconflict", branches[0]);
	EXPECT_STREQ(L"master2", branches[1]);
	EXPECT_STREQ(L"signed-commit", branches[2]);
	EXPECT_STREQ(L"simple-conflict", branches[3]);
	EXPECT_STREQ(L"subdir/branch", branches[4]);
	EXPECT_STREQ(L"remotes/origin/master", branches[5]);

	branches.clear();
	EXPECT_EQ(0, m_Git.GetBranchList(branches, nullptr, CGit::BRANCH_ALL, true));
	ASSERT_EQ(6U, branches.size());
	EXPECT_STREQ(L"forconflict", branches[0]);
	EXPECT_STREQ(L"master2", branches[1]);
	EXPECT_STREQ(L"signed-commit", branches[2]);
	EXPECT_STREQ(L"simple-conflict", branches[3]);
	EXPECT_STREQ(L"subdir/branch", branches[4]);
	EXPECT_STREQ(L"remotes/origin/master", branches[5]);

	branches.clear();
	current = -2;
	EXPECT_EQ(0, m_Git.GetBranchList(branches, &current, CGit::BRANCH_REMOTE));
	ASSERT_EQ(1U, branches.size());
	EXPECT_EQ(-2, current); // not touched
	EXPECT_STREQ(L"remotes/origin/master", branches[0]);

	STRING_VECTOR tags;
	EXPECT_EQ(0, m_Git.GetTagList(tags));
	ASSERT_EQ(3U, tags.size());
	EXPECT_STREQ(L"all-files-signed", tags[0]);
	EXPECT_STREQ(L"also-signed", tags[1]);
	EXPECT_STREQ(L"normal-tag", tags[2]);

	STRING_VECTOR refs;
	EXPECT_EQ(0, m_Git.GetRefList(refs));
	ASSERT_EQ(12U, refs.size());
	EXPECT_STREQ(L"refs/heads/forconflict", refs[0]);
	EXPECT_STREQ(L"refs/heads/master", refs[1]);
	EXPECT_STREQ(L"refs/heads/master2", refs[2]);
	EXPECT_STREQ(L"refs/heads/signed-commit", refs[3]);
	EXPECT_STREQ(L"refs/heads/simple-conflict", refs[4]);
	EXPECT_STREQ(L"refs/heads/subdir/branch", refs[5]);
	EXPECT_STREQ(L"refs/notes/commits", refs[6]);
	EXPECT_STREQ(L"refs/remotes/origin/master", refs[7]);
	EXPECT_STREQ(L"refs/stash", refs[8]);
	EXPECT_STREQ(L"refs/tags/all-files-signed", refs[9]);
	EXPECT_STREQ(L"refs/tags/also-signed", refs[10]);
	EXPECT_STREQ(L"refs/tags/normal-tag", refs[11]);

	MAP_HASH_NAME map;
	EXPECT_EQ(0, m_Git.GetMapHashToFriendName(map));
	if (testConfig == GIT_CLI || testConfig == LIBGIT)
		ASSERT_EQ(13U, map.size()); // also contains the undereferenced tags with hashes
	else
		ASSERT_EQ(11U, map.size());

	ASSERT_EQ(1U, map[CGitHash::FromHexStr(L"7c3cbfe13a929d2291a574dca45e4fd2d2ac1aa6")].size());
	EXPECT_STREQ(L"refs/heads/master", map[CGitHash::FromHexStr(L"7c3cbfe13a929d2291a574dca45e4fd2d2ac1aa6")][0]);
	ASSERT_EQ(1U, map[CGitHash::FromHexStr(L"4c5c93d2a0b368bc4570d5ec02ab03b9c4334d44")].size());
	EXPECT_STREQ(L"refs/heads/signed-commit", map[CGitHash::FromHexStr(L"4c5c93d2a0b368bc4570d5ec02ab03b9c4334d44")][0]);
	ASSERT_EQ(1U, map[CGitHash::FromHexStr(L"31ff87c86e9f6d3853e438cb151043f30f09029a")].size());
	EXPECT_STREQ(L"refs/heads/subdir/branch", map[CGitHash::FromHexStr(L"31ff87c86e9f6d3853e438cb151043f30f09029a")][0]);
	ASSERT_EQ(1U, map[CGitHash::FromHexStr(L"5e702e1712aa6f8cd8e0328a87be006f3a923710")].size());
	EXPECT_STREQ(L"refs/notes/commits", map[CGitHash::FromHexStr(L"5e702e1712aa6f8cd8e0328a87be006f3a923710")][0]);
	ASSERT_EQ(1U, map[CGitHash::FromHexStr(L"18da7c332dcad0f37f9977d9176dce0b0c66f3eb")].size());
	EXPECT_STREQ(L"refs/stash", map[CGitHash::FromHexStr(L"18da7c332dcad0f37f9977d9176dce0b0c66f3eb")][0]);
	ASSERT_EQ(1U, map[CGitHash::FromHexStr(L"c5b89de0335fd674e2e421ac4543098cb2f22cde")].size());
	EXPECT_STREQ(L"refs/heads/simple-conflict", map[CGitHash::FromHexStr(L"c5b89de0335fd674e2e421ac4543098cb2f22cde")][0]);
	ASSERT_EQ(1U, map[CGitHash::FromHexStr(L"10385764a4d42d7428bbeb245015f8f338fc1e40")].size());
	EXPECT_STREQ(L"refs/heads/forconflict", map[CGitHash::FromHexStr(L"10385764a4d42d7428bbeb245015f8f338fc1e40")][0]);
	ASSERT_EQ(2U, map[CGitHash::FromHexStr(L"49ecdfff36bfe2b9b499b33e5034f427e2fa54dd")].size());
	EXPECT_STREQ(L"refs/heads/master2", map[CGitHash::FromHexStr(L"49ecdfff36bfe2b9b499b33e5034f427e2fa54dd")][0]);
	EXPECT_STREQ(L"refs/tags/also-signed^{}", map[CGitHash::FromHexStr(L"49ecdfff36bfe2b9b499b33e5034f427e2fa54dd")][1]);
	ASSERT_EQ(1U, map[CGitHash::FromHexStr(L"b9ef30183497cdad5c30b88d32dc1bed7951dfeb")].size());//
	EXPECT_STREQ(L"refs/tags/normal-tag", map[CGitHash::FromHexStr(L"b9ef30183497cdad5c30b88d32dc1bed7951dfeb")][0]);
	ASSERT_EQ(1U, map[CGitHash::FromHexStr(L"a9d53b535cb49640a6099860ac4999f5a0857b91")].size());
	EXPECT_STREQ(L"refs/remotes/origin/master", map[CGitHash::FromHexStr(L"a9d53b535cb49640a6099860ac4999f5a0857b91")][0]);
	ASSERT_EQ(1U, map[CGitHash::FromHexStr(L"313a41bc88a527289c87d7531802ab484715974f")].size());
	EXPECT_STREQ(L"refs/tags/all-files-signed^{}", map[CGitHash::FromHexStr(L"313a41bc88a527289c87d7531802ab484715974f")][0]);

	STRING_VECTOR remotes;
	EXPECT_EQ(0, m_Git.GetRemoteList(remotes));
	ASSERT_EQ(1U, remotes.size());
	EXPECT_STREQ(L"origin", remotes[0]);

	EXPECT_EQ(-1, m_Git.DeleteRef(L"refs/tags/gibbednet"));
	branches.clear();
	EXPECT_EQ(0, m_Git.GetBranchList(branches, nullptr, CGit::BRANCH_ALL));
	EXPECT_EQ(7U, branches.size());
	tags.clear();
	EXPECT_EQ(0, m_Git.GetTagList(tags));
	EXPECT_EQ(3U, tags.size());
	refs.clear();
	EXPECT_EQ(0, m_Git.GetRefList(refs));
	EXPECT_EQ(12U, refs.size());

	EXPECT_EQ(-1, m_Git.DeleteRef(L"refs/heads/gibbednet"));
	branches.clear();
	EXPECT_EQ(0, m_Git.GetBranchList(branches, nullptr, CGit::BRANCH_ALL));
	EXPECT_EQ(7U, branches.size());
	tags.clear();
	EXPECT_EQ(0, m_Git.GetTagList(tags));
	EXPECT_EQ(3U, tags.size());
	refs.clear();
	EXPECT_EQ(0, m_Git.GetRefList(refs));
	EXPECT_EQ(12U, refs.size());

	EXPECT_EQ(-1, m_Git.DeleteRef(L"refs/remotes/origin/gibbednet"));
	branches.clear();
	EXPECT_EQ(0, m_Git.GetBranchList(branches, nullptr, CGit::BRANCH_ALL));
	EXPECT_EQ(7U, branches.size());
	tags.clear();
	EXPECT_EQ(0, m_Git.GetTagList(tags));
	EXPECT_EQ(3U, tags.size());
	refs.clear();
	EXPECT_EQ(0, m_Git.GetRefList(refs));
	EXPECT_EQ(12U, refs.size());

	EXPECT_EQ(0, m_Git.DeleteRef(L"refs/tags/normal-tag"));
	branches.clear();
	EXPECT_EQ(0, m_Git.GetBranchList(branches, nullptr, CGit::BRANCH_ALL));
	EXPECT_EQ(7U, branches.size());
	tags.clear();
	EXPECT_EQ(0, m_Git.GetTagList(tags));
	EXPECT_EQ(2U, tags.size());
	refs.clear();
	EXPECT_EQ(0, m_Git.GetRefList(refs));
	EXPECT_EQ(11U, refs.size());

	EXPECT_EQ(0, m_Git.DeleteRef(L"refs/tags/all-files-signed^{}"));
	branches.clear();
	EXPECT_EQ(0, m_Git.GetBranchList(branches, nullptr, CGit::BRANCH_ALL));
	EXPECT_EQ(7U, branches.size());
	tags.clear();
	EXPECT_EQ(0, m_Git.GetTagList(tags));
	EXPECT_EQ(1U, tags.size());
	refs.clear();
	EXPECT_EQ(0, m_Git.GetRefList(refs));
	EXPECT_EQ(10U, refs.size());

	EXPECT_EQ(0, m_Git.DeleteRef(L"refs/heads/subdir/branch"));
	branches.clear();
	EXPECT_EQ(0, m_Git.GetBranchList(branches, nullptr, CGit::BRANCH_ALL));
	EXPECT_EQ(6U, branches.size());
	tags.clear();
	EXPECT_EQ(0, m_Git.GetTagList(tags));
	EXPECT_EQ(1U, tags.size());
	refs.clear();
	EXPECT_EQ(0, m_Git.GetRefList(refs));
	EXPECT_EQ(9U, refs.size());

	EXPECT_EQ(0, m_Git.DeleteRef(L"refs/remotes/origin/master"));
	branches.clear();
	EXPECT_EQ(0, m_Git.GetBranchList(branches, nullptr, CGit::BRANCH_ALL));
	EXPECT_EQ(5U, branches.size());
	tags.clear();
	EXPECT_EQ(0, m_Git.GetTagList(tags));
	EXPECT_EQ(1U, tags.size());
	refs.clear();
	EXPECT_EQ(0, m_Git.GetRefList(refs));
	EXPECT_EQ(8U, refs.size());
}

TEST_P(CBasicGitWithTestRepoFixture, GetBranchesTagsRefs)
{
	GetBranchesTagsRefs(m_Git, GetParam());
}

TEST_P(CBasicGitWithTestRepoBareFixture, GetBranchesTagsRefs)
{
	GetBranchesTagsRefs(m_Git, GetParam());
}

TEST_P(CBasicGitWithTestRepoFixture, GetBranchList_orphan)
{
	CString output;
	EXPECT_EQ(0, m_Git.Run(L"git.exe checkout --orphan orphanic", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);

	STRING_VECTOR branches;
	int current = -2;
	EXPECT_EQ(0, m_Git.GetBranchList(branches, &current));
	ASSERT_EQ(6U, branches.size());
	EXPECT_EQ(-2, current);
	EXPECT_STREQ(L"forconflict", branches[0]);
	EXPECT_STREQ(L"master", branches[1]);
	EXPECT_STREQ(L"master2", branches[2]);
	EXPECT_STREQ(L"signed-commit", branches[3]);
	EXPECT_STREQ(L"simple-conflict", branches[4]);
	EXPECT_STREQ(L"subdir/branch", branches[5]);
}

TEST_P(CBasicGitWithTestRepoFixture, GetBranchList_detachedhead)
{
	CString output;
	EXPECT_EQ(0, m_Git.Run(L"git.exe checkout a9d53b535cb49640a6099860ac4999f5a0857b91", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);

	STRING_VECTOR branches;
	int current = -2;
	EXPECT_EQ(0, m_Git.GetBranchList(branches, &current));
	ASSERT_EQ(6U, branches.size());
	EXPECT_EQ(-2, current);
	EXPECT_STREQ(L"forconflict", branches[0]);
	EXPECT_STREQ(L"master", branches[1]);
	EXPECT_STREQ(L"master2", branches[2]);
	EXPECT_STREQ(L"signed-commit", branches[3]);
	EXPECT_STREQ(L"simple-conflict", branches[4]);
	EXPECT_STREQ(L"subdir/branch", branches[5]);

	branches.clear();
	current = -2;
	EXPECT_EQ(0, m_Git.GetBranchList(branches, &current, CGit::BRANCH_LOCAL, true));
	ASSERT_EQ(6U, branches.size());
	EXPECT_EQ(-2, current);
	EXPECT_STREQ(L"forconflict", branches[0]);
	EXPECT_STREQ(L"master", branches[1]);
	EXPECT_STREQ(L"master2", branches[2]);
	EXPECT_STREQ(L"signed-commit", branches[3]);
	EXPECT_STREQ(L"simple-conflict", branches[4]);
	EXPECT_STREQ(L"subdir/branch", branches[5]);

	// cygwin fails here
	if (CGit::ms_bCygwinGit)
		return;

	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe checkout -b (HEAD a9d53b535cb49640a6099860ac4999f5a0857b91", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);

	branches.clear();
	current = -2;
	EXPECT_EQ(0, m_Git.GetBranchList(branches, &current));
	ASSERT_EQ(7U, branches.size());
	EXPECT_EQ(0, current);
	EXPECT_STREQ(L"(HEAD", branches[0]);
	EXPECT_STREQ(L"forconflict", branches[1]);
	EXPECT_STREQ(L"master", branches[2]);
	EXPECT_STREQ(L"master2", branches[3]);
	EXPECT_STREQ(L"signed-commit", branches[4]);
	EXPECT_STREQ(L"simple-conflict", branches[5]);
	EXPECT_STREQ(L"subdir/branch", branches[6]);
}

TEST_P(CBasicGitWithEmptyBareRepositoryFixture, GetEmptyBranchesTagsRefs)
{
	EXPECT_STREQ(L"master", m_Git.GetCurrentBranch());

	STRING_VECTOR branches;
	int current = -2;
	EXPECT_EQ(0, m_Git.GetBranchList(branches, &current));
	EXPECT_TRUE(branches.empty());
	EXPECT_EQ(-2, current); // not touched

	EXPECT_EQ(0, m_Git.GetBranchList(branches, &current, CGit::BRANCH_ALL));
	EXPECT_TRUE(branches.empty());
	EXPECT_EQ(-2, current); // not touched

	STRING_VECTOR tags;
	EXPECT_EQ(0, m_Git.GetTagList(tags));
	EXPECT_TRUE(tags.empty());

	STRING_VECTOR refs;
	EXPECT_EQ(0, m_Git.GetRefList(refs));
	EXPECT_TRUE(refs.empty());

	MAP_HASH_NAME map;
	EXPECT_EQ(0, m_Git.GetMapHashToFriendName(map));
	EXPECT_TRUE(map.empty());

	STRING_VECTOR remotes;
	EXPECT_EQ(0, m_Git.GetRemoteList(remotes));
	EXPECT_TRUE(remotes.empty());
}

TEST_P(CBasicGitWithEmptyRepositoryFixture, CheckCleanWorkTree)
{
	// this test is known to fail with cygwin and also not enabled by default
	if (GetParam() == LIBGIT2_ALL && CGit::ms_bCygwinGit)
		return;

	CString output;
	CString testFile = m_Dir.GetTempDir() + L"\\test.txt";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(testFile, L"this is testing file."));
	EXPECT_EQ(0, m_Git.Run(L"git.exe add test.txt", &output, CP_UTF8));
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe commit -m \"Add test.txt\"", &output, CP_UTF8));
	// repo with 1 versioned file
	EXPECT_STRNE(L"", output);
	EXPECT_TRUE(m_Git.CheckCleanWorkTree());
	EXPECT_TRUE(m_Git.CheckCleanWorkTree(true));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(testFile, L"Overwriting this testing file."));
	// repo with 1 modified versioned file
	EXPECT_FALSE(m_Git.CheckCleanWorkTree());
	EXPECT_FALSE(m_Git.CheckCleanWorkTree(true));

	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe add test.txt", &output, CP_UTF8));
	// repo with 1 modified versioned and staged file
	EXPECT_STREQ(L"", output);
	EXPECT_FALSE(m_Git.CheckCleanWorkTree());
	EXPECT_TRUE(m_Git.CheckCleanWorkTree(true));

	EXPECT_EQ(0, m_Git.Run(L"git.exe commit -m \"Modified test.txt\"", &output, CP_UTF8));
	testFile = m_Dir.GetTempDir() + L"\\test2.txt";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(testFile, L"this is ANOTHER testing file."));
	EXPECT_TRUE(m_Git.CheckCleanWorkTree());
	EXPECT_TRUE(m_Git.CheckCleanWorkTree(true));

	EXPECT_EQ(0, m_Git.Run(L"git.exe checkout --orphan orphanic", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);
	EXPECT_FALSE(m_Git.CheckCleanWorkTree());
	EXPECT_FALSE(m_Git.CheckCleanWorkTree(true));
}

TEST(CGit, CEnvironment)
{
	CEnvironment env;
	wchar_t** basePtr = env;
	ASSERT_TRUE(basePtr);
	EXPECT_FALSE(*basePtr);
	EXPECT_TRUE(env.empty());
	env.SetEnv(L"not-found", nullptr);
	EXPECT_FALSE(static_cast<wchar_t*>(env));
	EXPECT_STREQ(L"", env.GetEnv(L"test"));
	env.SetEnv(L"key1", L"value1");
	EXPECT_STREQ(L"value1", env.GetEnv(L"key1"));
	EXPECT_STREQ(L"value1", env.GetEnv(L"kEy1")); // check case insensitivity
	EXPECT_TRUE(*basePtr);
	EXPECT_EQ(static_cast<wchar_t*>(env), *basePtr);
	EXPECT_FALSE(env.empty());
	EXPECT_STREQ(L"value1", env.GetEnv(L"key1"));
	env.SetEnv(L"key1", nullptr); // delete first
	EXPECT_FALSE(*basePtr);
	EXPECT_EQ(static_cast<wchar_t*>(env), *basePtr);
	EXPECT_TRUE(env.empty());
	env.SetEnv(L"key1", L"value1");
	EXPECT_STREQ(L"value1", env.GetEnv(L"key1"));
	EXPECT_TRUE(*basePtr);
	EXPECT_EQ(static_cast<wchar_t*>(env), *basePtr);
	EXPECT_FALSE(env.empty());
	env.SetEnv(L"key2", L"value2");
	EXPECT_STREQ(L"value1", env.GetEnv(L"key1"));
	EXPECT_STREQ(L"value2", env.GetEnv(L"key2"));
	EXPECT_EQ(static_cast<wchar_t*>(env), *basePtr);
	env.SetEnv(L"not-found", nullptr);
	EXPECT_STREQ(L"value1", env.GetEnv(L"key1"));
	EXPECT_STREQ(L"value2", env.GetEnv(L"key2"));
	env.SetEnv(L"key2", nullptr); // delete last
	EXPECT_STREQ(L"value1", env.GetEnv(L"key1"));
	EXPECT_STREQ(L"", env.GetEnv(L"key2"));
	env.SetEnv(L"key3", L"value3");
	EXPECT_STREQ(L"value1", env.GetEnv(L"key1"));
	EXPECT_STREQ(L"value3", env.GetEnv(L"key3"));
	env.SetEnv(L"key4", L"value4");
	env.SetEnv(L"value3", nullptr); // delete middle
	EXPECT_STREQ(L"value1", env.GetEnv(L"key1"));
	EXPECT_STREQ(L"value4", env.GetEnv(L"key4"));
	env.SetEnv(L"key5", L"value5");
	EXPECT_STREQ(L"value1", env.GetEnv(L"key1"));
	EXPECT_STREQ(L"value4", env.GetEnv(L"key4"));
	EXPECT_STREQ(L"value5", env.GetEnv(L"key5"));
	env.SetEnv(L"key4", L"value4a");
	EXPECT_STREQ(L"value1", env.GetEnv(L"key1"));
	EXPECT_STREQ(L"value4a", env.GetEnv(L"key4"));
	EXPECT_STREQ(L"value5", env.GetEnv(L"key5"));
	env.SetEnv(L"key5", L"value5a");
	EXPECT_STREQ(L"value1", env.GetEnv(L"key1"));
	EXPECT_STREQ(L"value4a", env.GetEnv(L"key4"));
	EXPECT_STREQ(L"value5a", env.GetEnv(L"key5"));
#pragma warning(push)
#pragma warning(disable: 4996)
	CString windir = _wgetenv(L"windir");
#pragma warning(pop)
	env.CopyProcessEnvironment();
	EXPECT_STREQ(windir, env.GetEnv(L"windir"));
	EXPECT_STREQ(L"value1", env.GetEnv(L"key1"));
	EXPECT_STREQ(L"value4a", env.GetEnv(L"key4"));
	EXPECT_STREQ(L"value5a", env.GetEnv(L"key5"));
	env.clear();
	EXPECT_FALSE(*basePtr);
	EXPECT_TRUE(env.empty());
	EXPECT_STREQ(L"", env.GetEnv(L"key4"));
	env.CopyProcessEnvironment();
	EXPECT_STREQ(windir, env.GetEnv(L"windir"));
	EXPECT_TRUE(*basePtr);

	// make sure baseptr always points to current values
	EXPECT_EQ(static_cast<wchar_t*>(env), *basePtr);

	env.clear();
	CString path = L"c:\\windows;c:\\windows\\system32";
	env.SetEnv(L"PATH", path);
	env.AddToPath(L"");
	EXPECT_STREQ(path, env.GetEnv(L"PATH"));
	env.AddToPath(L"c:\\windows");
	EXPECT_STREQ(path, env.GetEnv(L"PATH"));
	env.AddToPath(L"c:\\windows\\");
	EXPECT_STREQ(path, env.GetEnv(L"PATH"));
	env.AddToPath(L"c:\\windows\\system32");
	EXPECT_STREQ(path, env.GetEnv(L"PATH"));
	env.AddToPath(L"c:\\windows\\system32\\");
	EXPECT_STREQ(path, env.GetEnv(L"PATH"));
	path += L";c:\\windows\\system";
	env.AddToPath(L"c:\\windows\\system");
	EXPECT_STREQ(path, env.GetEnv(L"PATH"));
	path += L";c:\\test";
	env.AddToPath(L"c:\\test\\");
	EXPECT_STREQ(path, env.GetEnv(L"PATH"));
	env.AddToPath(L"c:\\test\\");
	EXPECT_STREQ(path, env.GetEnv(L"PATH"));
	env.AddToPath(L"c:\\test");
	EXPECT_STREQ(path, env.GetEnv(L"PATH"));
	path = L"c:\\windows;c:\\windows\\system32;";
	env.SetEnv(L"PATH", path);
	env.AddToPath(L"");
	EXPECT_STREQ(path, env.GetEnv(L"PATH"));
	env.AddToPath(L"c:\\test");
	path += L"c:\\test";
	EXPECT_STREQ(path, env.GetEnv(L"PATH"));

	// also test copy constructor
	CEnvironment env2(env);
	EXPECT_EQ(static_cast<wchar_t*>(env2), *static_cast<wchar_t**>(env2));
	EXPECT_NE(static_cast<wchar_t*>(env2), *basePtr);

	// also test assignment operation
	CEnvironment env3a;
	basePtr = env3a;
	env3a.SetEnv(L"something", L"else");
	CEnvironment env3b;
	env3b = env3a;
	EXPECT_FALSE(env3b.empty());
	EXPECT_EQ(static_cast<wchar_t**>(env3a), basePtr);
	EXPECT_EQ(static_cast<wchar_t*>(env3a), *basePtr);
	EXPECT_NE(static_cast<wchar_t*>(env3b), *basePtr);
}

static void GetOneFile(CGit& m_Git)
{
	CString tmpFile = GetTempFile();
	EXPECT_EQ(0, m_Git.GetOneFile(L"b9ef30183497cdad5c30b88d32dc1bed7951dfeb", CTGitPath(L"utf8-nobom.txt"), tmpFile));
	CString fileContents;
	EXPECT_EQ(true, CStringUtils::ReadStringFromTextFile(tmpFile, fileContents));
	struct _stat32 stat_buf = { 0 };
	EXPECT_EQ(0, _wstat32(tmpFile, &stat_buf));
	EXPECT_EQ(139, stat_buf.st_size);
	EXPECT_EQ(108, fileContents.GetLength());
	EXPECT_STREQ(L"ä#äf34ööcöäß€9875oe\r\nfgdjkglsfdg\r\nöäöü45g\r\nfdgi&§$%&hfdsgä\r\nä#äf34öööäß€9875oe\r\nöäcüpfgmfdg\r\n€fgfdsg\r\n45\r\näü", fileContents);
	::DeleteFile(tmpFile);
}

TEST_P(CBasicGitWithTestRepoFixture, GetOneFile)
{
	GetOneFile(m_Git);

	// clean&smudge filters are not available for GetOneFile without libigt2
	if (GetParam() == GIT_CLI || GetParam() == LIBGIT)
		return;

	CString cleanFilterFilename = m_Git.m_CurrentDir + L"\\clean_filter_openssl";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(cleanFilterFilename, L"#!/bin/bash\nopenssl version | grep -q 1\\\\.0\nif [[ $? = 0 ]]; then\n\topenssl enc -base64 -aes-256-ecb -S FEEDDEADBEEF -k PASS_FIXED\n\techo 1.0>openssl10\nelse\n\topenssl enc -base64 -pbkdf2 -aes-256-ecb -S FEEDDEADBEEFFEED -k PASS_FIXED\nfi\n"));
	CString smudgeFilterFilename = m_Git.m_CurrentDir + L"\\smudge_filter_openssl";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(smudgeFilterFilename, L"#!/bin/bash\nopenssl version | grep -q 1\\\\.0\nif [[ $? = 0 ]]; then\n\topenssl enc -d -base64 -aes-256-ecb -k PASS_FIXED\nelse\n\topenssl enc -d -base64 -pbkdf2 -aes-256-ecb -k PASS_FIXED\nfi\n"));

	CAutoRepository repo(m_Git.GetGitRepository());
	ASSERT_TRUE(repo.IsValid());
	CAutoConfig config(repo);
	ASSERT_TRUE(config.IsValid());
	CStringA path = CUnicodeUtils::GetUTF8(m_Git.m_CurrentDir);
	path.Replace('\\', '/');
	EXPECT_EQ(0, git_config_set_string(config, "filter.openssl.clean", path + "/clean_filter_openssl"));
	EXPECT_EQ(0, git_config_set_string(config, "filter.openssl.smudge", path + "/smudge_filter_openssl"));
	EXPECT_EQ(0, git_config_set_bool(config, "filter.openssl.required", 1));

	CString attributesFile = m_Git.m_CurrentDir + L"\\.gitattributes";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(attributesFile, L"*.enc filter=openssl\n"));

	CString encryptedFileOne = m_Git.m_CurrentDir + L"\\1.enc";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(encryptedFileOne, L"This should be encrypted...\nAnd decrypted on the fly\n"));

	CString encryptedFileTwo = m_Git.m_CurrentDir + L"\\2.enc";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(encryptedFileTwo, L"This should also be encrypted...\nAnd also decrypted on the fly\n"));

	CString output;
	EXPECT_EQ(0, m_Git.Run(L"git.exe add 1.enc", &output, CP_UTF8));
	if (!g_Git.ms_bCygwinGit) // on AppVeyor with the VS2017 image we get a warning: "WARNING: can't open config file: /usr/local/ssl/openssl.cnf"
		EXPECT_STREQ(L"", output);

	CAutoIndex index;
	ASSERT_EQ(0, git_repository_index(index.GetPointer(), repo));
	EXPECT_EQ(0, git_index_add_bypath(index, "2.enc"));
	EXPECT_EQ(0, git_index_write(index));

	EXPECT_EQ(0, m_Git.Run(L"git.exe commit -m \"Message\"", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);

	CString fileContents;
	CString tmpFile = GetTempFile();
	EXPECT_EQ(0, m_Git.GetOneFile(L"HEAD", CTGitPath(L"1.enc"), tmpFile));
	EXPECT_EQ(true, CStringUtils::ReadStringFromTextFile(tmpFile, fileContents));
	EXPECT_STREQ(L"This should be encrypted...\nAnd decrypted on the fly\n", fileContents);
	::DeleteFile(tmpFile);

	fileContents.Empty();
	tmpFile = GetTempFile();
	EXPECT_EQ(0, m_Git.GetOneFile(L"HEAD", CTGitPath(L"2.enc"), tmpFile));
	EXPECT_EQ(true, CStringUtils::ReadStringFromTextFile(tmpFile, fileContents));
	EXPECT_STREQ(L"This should also be encrypted...\nAnd also decrypted on the fly\n", fileContents);
	::DeleteFile(tmpFile);

	EXPECT_TRUE(::DeleteFile(attributesFile));

	bool openssl10 = PathFileExists(L"openssl10");

	fileContents.Empty();
	tmpFile = GetTempFile();
	EXPECT_EQ(0, m_Git.GetOneFile(L"HEAD", CTGitPath(L"1.enc"), tmpFile));
	EXPECT_EQ(true, CStringUtils::ReadStringFromTextFile(tmpFile, fileContents));
	fileContents.Replace(L"\r\n", L"\n");
	EXPECT_TRUE(CStringUtils::StartsWith(fileContents, L"U2FsdGVkX1/+7d6tvu"));
	if (openssl10)
		EXPECT_STREQ(L"U2FsdGVkX1/+7d6tvu8AABwbE+Xy7U4l5boTKjIgUkYHONqmYHD+0e6k35MgtUGx\ns11nq1QuKeFCW5wFWNSj1WcHg2n4W59xfnB7RkSSIDQ=\n", fileContents);
	else
		EXPECT_STREQ(L"U2FsdGVkX1/+7d6tvu/+7UgE7iA1vUXeIPVzXx1ef6pAZjq/p481dZp1oCyRa/ur\nzgcQLgv/OrJfYMWXxWXQRp2uWGnntih9NrvOTlSN440=\n", fileContents);
	::DeleteFile(tmpFile);

	fileContents.Empty();
	tmpFile = GetTempFile();
	EXPECT_EQ(0, m_Git.GetOneFile(L"HEAD", CTGitPath(L"2.enc"), tmpFile));
	EXPECT_EQ(true, CStringUtils::ReadStringFromTextFile(tmpFile, fileContents));
	fileContents.Replace(L"\r\n", L"\n");
	EXPECT_TRUE(CStringUtils::StartsWith(fileContents, L"U2FsdGVkX1/+7d6tvu"));
	if (openssl10)
		EXPECT_STREQ(L"U2FsdGVkX1/+7d6tvu8AAIDDx8qi/l0qzkSMsS2YLt8tYK1oWzj8+o78fXH0/tlO\nCRVrKqTvh9eUFklY8QFYfZfj01zBkFat+4zrW+1rV4Q=\n", fileContents);
	else
		EXPECT_STREQ(L"U2FsdGVkX1/+7d6tvu/+7XAtdjFg6XFOvt0SWT9/LdWG8J1pLET464/4A3jusIMK\nuCP1hvKsnuGcQv3KtoJbRU3KAFarZIrNEC1mHofQPFs=\n", fileContents);
	::DeleteFile(tmpFile);
}

TEST_P(CBasicGitWithTestRepoBareFixture, GetOneFile)
{
	GetOneFile(m_Git);
}

static void GetBranchDescriptions(CGit& m_Git)
{
	MAP_STRING_STRING descriptions;
	EXPECT_EQ(0, m_Git.GetBranchDescriptions(descriptions));
	EXPECT_EQ(0U, descriptions.size());

	g_Git.SetConfigValue(L"branch.master.description", L"test");
	g_Git.SetConfigValue(L"branch.subdir/branch.description", L"multi\nline");

	EXPECT_EQ(0, m_Git.GetBranchDescriptions(descriptions));
	ASSERT_EQ(2U, descriptions.size());
	EXPECT_STREQ(L"test", descriptions[L"master"]);
	EXPECT_STREQ(L"multi\nline", descriptions[L"subdir/branch"]);
}

TEST_P(CBasicGitWithEmptyRepositoryFixture, GetBranchDescriptions)
{
	GetBranchDescriptions(m_Git);
}

TEST_P(CBasicGitWithEmptyBareRepositoryFixture, GetBranchDescriptions)
{
	GetBranchDescriptions(m_Git);
}

TEST_P(CBasicGitWithTestRepoFixture, Config)
{
	EXPECT_STREQ(L"", m_Git.GetConfigValue(L"not-found"));
	EXPECT_STREQ(L"default", m_Git.GetConfigValue(L"not-found", L"default"));

	EXPECT_STREQ(L"false", m_Git.GetConfigValue(L"core.bare"));
	EXPECT_STREQ(L"false", m_Git.GetConfigValue(L"core.bare", L"default-value")); // value exist, so default does not match
	EXPECT_STREQ(L"true", m_Git.GetConfigValue(L"core.ignorecase"));
	EXPECT_STREQ(L"0", m_Git.GetConfigValue(L"core.repositoryformatversion"));
	EXPECT_STREQ(L"https://example.com/git/testing", m_Git.GetConfigValue(L"remote.origin.url"));

	EXPECT_EQ(false, m_Git.GetConfigValueBool(L"not-found"));
	EXPECT_EQ(true, m_Git.GetConfigValueBool(L"not-found", true));
	EXPECT_EQ(false, m_Git.GetConfigValueBool(L"core.bare"));
	EXPECT_EQ(false, m_Git.GetConfigValueBool(L"core.bare", true)); // value exist, so default does not match
	EXPECT_EQ(false, m_Git.GetConfigValueBool(L"core.repositoryformatversion"));
	EXPECT_EQ(false, m_Git.GetConfigValueBool(L"remote.origin.url"));
	EXPECT_EQ(true, m_Git.GetConfigValueBool(L"core.ignorecase"));

	CString values[] = { L"", L" ", L"ending-with-space ", L" starting with-space", L"test1", L"some\\backslashes\\in\\it", L"with \" doublequote", L"with backslash before \\\" doublequote", L"with'quote", L"multi\nline", L"no-multi\\nline", L"new line at end\n" };
	for (int i = 0; i < _countof(values); ++i)
	{
		CString key;
		key.Format(L"re-read.test%d", i);
		EXPECT_EQ(0, m_Git.SetConfigValue(key, values[i]));
		EXPECT_STREQ(values[i], m_Git.GetConfigValue(key));
	}

	m_Git.SetConfigValue(L"booltest.true1", L"1");
	m_Git.SetConfigValue(L"booltest.true2", L"100");
	m_Git.SetConfigValue(L"booltest.true3", L"-2");
	m_Git.SetConfigValue(L"booltest.true4", L"yes");
	m_Git.SetConfigValue(L"booltest.true5", L"yEs");
	m_Git.SetConfigValue(L"booltest.true6", L"true");
	m_Git.SetConfigValue(L"booltest.true7", L"on");
	for (int i = 1; i <= 7; ++i)
	{
		CString key;
		key.Format(L"booltest.true%d", i);
		EXPECT_EQ(true, m_Git.GetConfigValueBool(key));
	}
	m_Git.SetConfigValue(L"booltest.false1", L"0");
	EXPECT_EQ(false, m_Git.GetConfigValueBool(L"booltest.false1"));
	m_Git.SetConfigValue(L"booltest.false2", L"");
	EXPECT_EQ(false, m_Git.GetConfigValueBool(L"booltest.false2"));

	EXPECT_EQ(0, m_Git.GetConfigValueInt32(L"does-not-exist"));
	EXPECT_EQ(15, m_Git.GetConfigValueInt32(L"does-not-exist", 15));
	EXPECT_EQ(0, m_Git.GetConfigValueInt32(L"core.repositoryformatversion"));
	EXPECT_EQ(0, m_Git.GetConfigValueInt32(L"core.repositoryformatversion", 42)); // value exist, so default should not be returned
	EXPECT_EQ(1, m_Git.GetConfigValueInt32(L"booltest.true1"));
	EXPECT_EQ(100, m_Git.GetConfigValueInt32(L"booltest.true2"));
	EXPECT_EQ(-2, m_Git.GetConfigValueInt32(L"booltest.true3"));
	EXPECT_EQ(0, m_Git.GetConfigValueInt32(L"booltest.true4"));
	EXPECT_EQ(42, m_Git.GetConfigValueInt32(L"booltest.true4", 42));
	EXPECT_EQ(0, m_Git.GetConfigValueInt32(L"booltest.true8"));
	EXPECT_EQ(42, m_Git.GetConfigValueInt32(L"booltest.true8", 42));

	EXPECT_NE(0, m_Git.UnsetConfigValue(L"does-not-exist"));
	EXPECT_STREQ(L"false", m_Git.GetConfigValue(L"core.bare"));
	EXPECT_STREQ(L"true", m_Git.GetConfigValue(L"core.ignorecase"));
	EXPECT_EQ(0, m_Git.UnsetConfigValue(L"core.bare"));
	EXPECT_STREQ(L"default", m_Git.GetConfigValue(L"core.bare", L"default"));
	EXPECT_STREQ(L"true", m_Git.GetConfigValue(L"core.ignorecase"));

	CString gitConfig = m_Git.m_CurrentDir + L"\\.git\\config";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(gitConfig, L"[booltest]\nistrue"));
	EXPECT_EQ(true, m_Git.GetConfigValueBool(L"booltest.istrue"));

	// test includes from %HOME% specified as ~/
	EXPECT_STREQ(L"not-found", g_Git.GetConfigValue(L"test.fromincluded", L"not-found"));
	EXPECT_EQ(0, m_Git.SetConfigValue(L"include.path", L"~/a-path-that-should-not-exist.gconfig"));
	EXPECT_STREQ(L"~/a-path-that-should-not-exist.gconfig", g_Git.GetConfigValue(L"include.path", L"not-found"));
	CString testFile = g_Git.GetHomeDirectory() + L"\\a-path-that-should-not-exist.gconfig";
	ASSERT_FALSE(PathFileExists(testFile)); // make sure we don't override a file by mistake ;)
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(testFile, L"[test]\nfromincluded=yeah-this-is-included\n"));
	EXPECT_STREQ(L"yeah-this-is-included", g_Git.GetConfigValue(L"test.fromincluded", L"not-found"));
	EXPECT_TRUE(::DeleteFile(testFile));
}

TEST_P(CBasicGitWithTestRepoFixture, GetWorkingTreeChanges)
{
	if (GetParam() != 0)
		return;

	// adding ansi2.txt (as a copy of ansi.txt) produces a warning
	m_Git.SetConfigValue(L"core.autocrlf", L"false");

	CString output;
	EXPECT_EQ(0, m_Git.Run(L"git.exe reset --hard master", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);

	CTGitPathList filter(CTGitPath(L"copy"));

	// no changes
	CTGitPathList list;
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	EXPECT_TRUE(list.IsEmpty());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, nullptr));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(L"ascii.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter));
	EXPECT_TRUE(list.IsEmpty());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter, false));
	ASSERT_EQ(0, list.GetCount());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter, true));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(L"ascii.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());

	// untracked file
	CString testFile = m_Git.m_CurrentDir + L"\\untracked-file.txt";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(testFile, L"*.enc filter=openssl\n"));
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	EXPECT_TRUE(list.IsEmpty());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, nullptr));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(L"ascii.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter));
	EXPECT_TRUE(list.IsEmpty());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter, false));
	ASSERT_EQ(0, list.GetCount());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter, true));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(L"ascii.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());

	// untracked file in sub-directory
	testFile = m_Git.m_CurrentDir + L"\\copy\\untracked-file.txt";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(testFile, L"*.enc filter=openssl\n"));
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	EXPECT_TRUE(list.IsEmpty());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, nullptr));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(L"ascii.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter));
	EXPECT_TRUE(list.IsEmpty());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter, false));
	EXPECT_TRUE(list.IsEmpty());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter, true));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(L"ascii.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());

	// modified file in sub-directory
	testFile = m_Git.m_CurrentDir + L"\\copy\\utf8-nobom.txt";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(testFile, L"*.enc filter=openssl\n"));
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(L"copy/utf8-nobom.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, nullptr));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(L"ascii.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	EXPECT_STREQ(L"copy/utf8-nobom.txt", list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);
	EXPECT_FALSE(list[1].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(L"copy/utf8-nobom.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter, false));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(L"copy/utf8-nobom.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter, true));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(L"ascii.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	EXPECT_STREQ(L"copy/utf8-nobom.txt", list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);
	EXPECT_FALSE(list[1].IsDirectory());

	// two modified files, one in root and one in sub-directory
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe reset --hard master", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);
	testFile = m_Git.m_CurrentDir + L"\\utf8-bom.txt";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(testFile, L"*.enc filter=openssl\n"));
	testFile = m_Git.m_CurrentDir + L"\\copy\\utf8-nobom.txt";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(testFile, L"*.enc filter=openssl\n"));
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(L"copy/utf8-nobom.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	EXPECT_STREQ(L"utf8-bom.txt", list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);
	EXPECT_FALSE(list[1].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, nullptr));
	ASSERT_EQ(3, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(L"ascii.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	EXPECT_STREQ(L"copy/utf8-nobom.txt", list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);
	EXPECT_FALSE(list[1].IsDirectory());
	EXPECT_STREQ(L"utf8-bom.txt", list[2].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[2].m_Action);
	EXPECT_FALSE(list[2].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(L"copy/utf8-nobom.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter, false));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(L"copy/utf8-nobom.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter, true));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(L"ascii.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	EXPECT_STREQ(L"copy/utf8-nobom.txt", list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);
	EXPECT_FALSE(list[1].IsDirectory());

	// Staged modified file
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe reset --hard master", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);
	testFile = m_Git.m_CurrentDir + L"\\utf8-nobom.txt";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(testFile, L"*.enc filter=openssl\n"));
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe add utf8-nobom.txt", &output, CP_UTF8));
	EXPECT_STREQ(L"", output);
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(L"utf8-nobom.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, nullptr));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(L"ascii.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	EXPECT_STREQ(L"utf8-nobom.txt", list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);
	EXPECT_FALSE(list[1].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter, false));
	ASSERT_EQ(0, list.GetCount());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter, true));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(L"utf8-nobom.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter, false));
	ASSERT_EQ(0, list.GetCount());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter, true));
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(L"ascii.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	EXPECT_STREQ(L"utf8-nobom.txt", list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);
	EXPECT_FALSE(list[1].IsDirectory());

	// Staged modified file in subfolder
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe reset --hard master", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);
	testFile = m_Git.m_CurrentDir + L"\\copy\\utf8-nobom.txt";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(testFile, L"*.enc filter=openssl\n"));
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe add copy/utf8-nobom.txt", &output, CP_UTF8));
	EXPECT_STREQ(L"", output);
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(L"copy/utf8-nobom.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, nullptr));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(L"ascii.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	EXPECT_STREQ(L"copy/utf8-nobom.txt", list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);
	EXPECT_FALSE(list[1].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(L"copy/utf8-nobom.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter, false));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(L"copy/utf8-nobom.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter, true));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(L"ascii.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	EXPECT_STREQ(L"copy/utf8-nobom.txt", list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);
	EXPECT_FALSE(list[1].IsDirectory());

	// Modified file modified after staging
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe reset --hard master", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);
	testFile = m_Git.m_CurrentDir + L"\\copy\\utf8-nobom.txt";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(testFile, L"*.enc filter=openssl\n"));
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe add copy/utf8-nobom.txt", &output, CP_UTF8));
	EXPECT_STREQ(L"", output);
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(testFile, L"now with different content after staging"));
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(L"copy/utf8-nobom.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, nullptr));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(L"ascii.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	EXPECT_STREQ(L"copy/utf8-nobom.txt", list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);
	EXPECT_FALSE(list[1].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(L"copy/utf8-nobom.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter, false));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(L"copy/utf8-nobom.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter, true));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(L"ascii.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	EXPECT_STREQ(L"copy/utf8-nobom.txt", list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);
	EXPECT_FALSE(list[1].IsDirectory());

	// Missing file
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe reset --hard master", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);
	EXPECT_TRUE(::DeleteFile(m_Dir.GetTempDir()+L"\\copy\\ansi.txt"));
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list.GetAction());
	EXPECT_STREQ(L"copy/ansi.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED | CTGitPath::LOGACTIONS_MISSING, list[0].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, nullptr));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED | CTGitPath::LOGACTIONS_DELETED, list.GetAction());
	EXPECT_STREQ(L"ascii.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_STREQ(L"copy/ansi.txt", list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED | CTGitPath::LOGACTIONS_MISSING, list[1].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list.GetAction());
	EXPECT_STREQ(L"copy/ansi.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED | CTGitPath::LOGACTIONS_MISSING, list[0].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter, false));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list.GetAction());
	EXPECT_STREQ(L"copy/ansi.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED | CTGitPath::LOGACTIONS_MISSING, list[0].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter, true));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED | CTGitPath::LOGACTIONS_DELETED, list.GetAction());
	EXPECT_STREQ(L"ascii.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_STREQ(L"copy/ansi.txt", list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED | CTGitPath::LOGACTIONS_MISSING, list[1].m_Action);

	// deleted file, also deleted in index
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe reset --hard master", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe rm copy/ansi.txt", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list.GetAction());
	EXPECT_STREQ(L"copy/ansi.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list[0].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, nullptr));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED | CTGitPath::LOGACTIONS_DELETED, list.GetAction());
	EXPECT_STREQ(L"ascii.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_STREQ(L"copy/ansi.txt", list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list[1].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list.GetAction());
	EXPECT_STREQ(L"copy/ansi.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list[0].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter, false));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list.GetAction());
	EXPECT_STREQ(L"copy/ansi.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list[0].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter, true));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED | CTGitPath::LOGACTIONS_DELETED, list.GetAction());
	EXPECT_STREQ(L"ascii.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_STREQ(L"copy/ansi.txt", list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list[1].m_Action);

	// file deleted in index, but still on disk
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe reset --hard master", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe rm --cached copy/ansi.txt", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list.GetAction());
	EXPECT_STREQ(L"copy/ansi.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, nullptr));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED | CTGitPath::LOGACTIONS_DELETED, list.GetAction());
	EXPECT_STREQ(L"ascii.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	EXPECT_STREQ(L"copy/ansi.txt", list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list[1].m_Action);
	EXPECT_FALSE(list[1].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list.GetAction());
	EXPECT_STREQ(L"copy/ansi.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter, false));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list.GetAction());
	EXPECT_STREQ(L"copy/ansi.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter, true));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED | CTGitPath::LOGACTIONS_DELETED, list.GetAction());
	EXPECT_STREQ(L"ascii.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	EXPECT_STREQ(L"copy/ansi.txt", list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list[1].m_Action);
	EXPECT_FALSE(list[1].IsDirectory());

	// file deleted in index, but still on disk, but modified
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe reset --hard master", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe rm --cached copy/ansi.txt", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);
	testFile = m_Git.m_CurrentDir + L"\\copy\\ansi.txt";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(testFile, L"*.enc filter=openssl\n"));
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list.GetAction());
	EXPECT_STREQ(L"copy/ansi.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, nullptr));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED | CTGitPath::LOGACTIONS_DELETED, list.GetAction());
	EXPECT_STREQ(L"ascii.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	EXPECT_STREQ(L"copy/ansi.txt", list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list[1].m_Action);
	EXPECT_FALSE(list[1].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list.GetAction());
	EXPECT_STREQ(L"copy/ansi.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter, false));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list.GetAction());
	EXPECT_STREQ(L"copy/ansi.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter, true));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED | CTGitPath::LOGACTIONS_DELETED, list.GetAction());
	EXPECT_STREQ(L"ascii.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	EXPECT_STREQ(L"copy/ansi.txt", list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list[1].m_Action);
	EXPECT_FALSE(list[1].IsDirectory());

	// renamed file in same folder (root)
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe reset --hard master", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe mv ansi.txt ansi2.txt", &output, CP_UTF8));
	EXPECT_STREQ(L"", output);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED, list.GetAction());
	EXPECT_STREQ(L"ansi2.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, nullptr));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED | CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(L"ansi2.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	EXPECT_STREQ(L"ascii.txt", list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);
	EXPECT_FALSE(list[1].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter, false));
	ASSERT_EQ(0, list.GetCount());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter, true));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED, list.GetAction());
	EXPECT_STREQ(L"ansi2.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter, false));
	ASSERT_EQ(0, list.GetCount());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter, true));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED | CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(L"ansi2.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	EXPECT_STREQ(L"ascii.txt", list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);
	EXPECT_FALSE(list[1].IsDirectory());

	// renamed file in same folder (subfolder)
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe reset --hard master", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe mv copy/ansi.txt copy/ansi2.txt", &output, CP_UTF8));
	EXPECT_STREQ(L"", output);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED, list.GetAction());
	EXPECT_STREQ(L"copy/ansi2.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, nullptr));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED | CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(L"ascii.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	EXPECT_STREQ(L"copy/ansi2.txt", list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED, list[1].m_Action);
	EXPECT_FALSE(list[1].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter, false));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED, list.GetAction());
	EXPECT_STREQ(L"copy/ansi2.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter, true));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED, list.GetAction());
	EXPECT_STREQ(L"copy/ansi2.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter, false));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED, list.GetAction());
	EXPECT_STREQ(L"copy/ansi2.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter, true));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED | CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(L"ascii.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	EXPECT_STREQ(L"copy/ansi2.txt", list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED, list[1].m_Action);
	EXPECT_FALSE(list[1].IsDirectory());

	// added and staged new file
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe reset --hard master", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);
	testFile = m_Git.m_CurrentDir + L"\\copy\\test-file.txt";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(testFile, L"*.enc filter=openssl\n"));
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe add copy/test-file.txt", &output, CP_UTF8));
	EXPECT_STREQ(L"", output);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list.GetAction());
	EXPECT_STREQ(L"copy/test-file.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, nullptr));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED | CTGitPath::LOGACTIONS_ADDED, list.GetAction());
	EXPECT_STREQ(L"ascii.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	EXPECT_STREQ(L"copy/test-file.txt", list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list[1].m_Action);
	EXPECT_FALSE(list[1].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list.GetAction());
	EXPECT_STREQ(L"copy/test-file.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter, false));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list.GetAction());
	EXPECT_STREQ(L"copy/test-file.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter, true));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED | CTGitPath::LOGACTIONS_ADDED, list.GetAction());
	EXPECT_STREQ(L"ascii.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	EXPECT_STREQ(L"copy/test-file.txt", list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list[1].m_Action);
	EXPECT_FALSE(list[1].IsDirectory());

	// file copied and staged
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe reset --hard master", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);
	testFile = m_Git.m_CurrentDir + L"\\ansi.txt";
	EXPECT_TRUE(CopyFile(m_Git.m_CurrentDir + L"\\ansi.txt", m_Git.m_CurrentDir + L"\\ansi2.txt", TRUE));
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe add ansi2.txt", &output, CP_UTF8));
	EXPECT_STREQ(L"", output);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list.GetAction());
	EXPECT_STREQ(L"ansi2.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, nullptr));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED | CTGitPath::LOGACTIONS_ADDED, list.GetAction());
	EXPECT_STREQ(L"ansi2.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	EXPECT_STREQ(L"ascii.txt", list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);
	EXPECT_FALSE(list[1].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter, false));
	ASSERT_EQ(0, list.GetCount());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter, true));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list.GetAction());
	EXPECT_STREQ(L"ansi2.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter, false));
	ASSERT_EQ(0, list.GetCount());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter, true));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED | CTGitPath::LOGACTIONS_ADDED, list.GetAction());
	EXPECT_STREQ(L"ansi2.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	EXPECT_STREQ(L"ascii.txt", list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);
	EXPECT_FALSE(list[1].IsDirectory());

	// file renamed + moved to sub-folder
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe reset --hard master", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe mv ansi.txt copy/ansi2.txt", &output, CP_UTF8));
	EXPECT_STREQ(L"", output);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED, list.GetAction());
	EXPECT_STREQ(L"copy/ansi2.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, nullptr));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED | CTGitPath::LOGACTIONS_REPLACED, list.GetAction());
	EXPECT_STREQ(L"ascii.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	EXPECT_STREQ(L"copy/ansi2.txt", list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED, list[1].m_Action);
	EXPECT_FALSE(list[1].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter, false)); // TODO: due to filter restrictions we cannot detect the rename here
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list.GetAction());
	EXPECT_STREQ(L"copy/ansi2.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list[0].m_Action); // TODO
	EXPECT_FALSE(list[0].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter, true));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED | CTGitPath::LOGACTIONS_ADDED, list.GetAction());
	EXPECT_STREQ(L"copy/ansi2.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED | CTGitPath::LOGACTIONS_ADDED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter, false)); // TODO: due to filter restrictions we cannot detect the rename here
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list.GetAction());
	EXPECT_STREQ(L"copy/ansi2.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list[0].m_Action); // TODO
	EXPECT_FALSE(list[0].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter, true));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED | CTGitPath::LOGACTIONS_REPLACED | CTGitPath::LOGACTIONS_ADDED, list.GetAction());
	EXPECT_STREQ(L"ascii.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	EXPECT_STREQ(L"copy/ansi2.txt", list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED | CTGitPath::LOGACTIONS_ADDED, list[1].m_Action); // TODO
	EXPECT_FALSE(list[1].IsDirectory());

	// conflicting files
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe reset --hard master", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe merge forconflict", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);
	output.Empty();
	EXPECT_EQ(1, m_Git.Run(L"git.exe merge simple-conflict", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED | CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(L"ansi.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED | CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, nullptr));
	ASSERT_EQ(7, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED | CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(L"ansi.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED | CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	EXPECT_STREQ(L"utf16-be-bom.txt", list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);
	EXPECT_FALSE(list[1].IsDirectory());
	EXPECT_STREQ(L"utf16-be-nobom.txt", list[2].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[2].m_Action);
	EXPECT_FALSE(list[2].IsDirectory());
	EXPECT_STREQ(L"utf16-le-bom.txt", list[3].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[3].m_Action);
	EXPECT_FALSE(list[3].IsDirectory());
	EXPECT_STREQ(L"utf16-le-nobom.txt", list[4].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[4].m_Action);
	EXPECT_FALSE(list[4].IsDirectory());
	EXPECT_STREQ(L"utf8-bom.txt", list[5].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[5].m_Action);
	EXPECT_FALSE(list[5].IsDirectory());
	EXPECT_STREQ(L"utf8-nobom.txt", list[6].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[6].m_Action);
	EXPECT_FALSE(list[6].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter, false));
	ASSERT_EQ(0, list.GetCount());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter, true));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED, list.GetAction());
	EXPECT_STREQ(L"ansi.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter, false));
	ASSERT_EQ(0, list.GetCount());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter, true));
	ASSERT_EQ(7, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED | CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(L"ansi.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	EXPECT_STREQ(L"utf16-be-bom.txt", list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);
	EXPECT_FALSE(list[1].IsDirectory());
	EXPECT_STREQ(L"utf16-be-nobom.txt", list[2].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[2].m_Action);
	EXPECT_FALSE(list[2].IsDirectory());
	EXPECT_STREQ(L"utf16-le-bom.txt", list[3].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[3].m_Action);
	EXPECT_FALSE(list[3].IsDirectory());
	EXPECT_STREQ(L"utf16-le-nobom.txt", list[4].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[4].m_Action);
	EXPECT_FALSE(list[4].IsDirectory());
	EXPECT_STREQ(L"utf8-bom.txt", list[5].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[5].m_Action);
	EXPECT_FALSE(list[5].IsDirectory());
	EXPECT_STREQ(L"utf8-nobom.txt", list[6].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[6].m_Action);
	EXPECT_FALSE(list[6].IsDirectory());
}

TEST_P(CBasicGitWithTestRepoFixture, GetWorkingTreeChanges_DeleteModifyConflict_DeletedRemotely)
{
	if (GetParam() != 0)
		return;

	CString output;
	EXPECT_EQ(0, m_Git.Run(L"git.exe reset --hard master", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);

	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe checkout forconflict", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe rm ansi.txt", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe commit -m \"Prepare conflict case\"", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe checkout simple-conflict", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);
	output.Empty();
	EXPECT_EQ(1, m_Git.Run(L"git.exe merge forconflict", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);

	CTGitPathList filter(CTGitPath(L"copy"));

	CTGitPathList list;
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	ASSERT_EQ(7, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED | CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(L"ansi.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED | CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	EXPECT_STREQ(L"utf16-be-bom.txt", list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);
	EXPECT_FALSE(list[1].IsDirectory());
	EXPECT_STREQ(L"utf16-be-nobom.txt", list[2].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[2].m_Action);
	EXPECT_FALSE(list[2].IsDirectory());
	EXPECT_STREQ(L"utf16-le-bom.txt", list[3].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[3].m_Action);
	EXPECT_FALSE(list[3].IsDirectory());
	EXPECT_STREQ(L"utf16-le-nobom.txt", list[4].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[4].m_Action);
	EXPECT_FALSE(list[4].IsDirectory());
	EXPECT_STREQ(L"utf8-bom.txt", list[5].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[5].m_Action);
	EXPECT_FALSE(list[5].IsDirectory());
	EXPECT_STREQ(L"utf8-nobom.txt", list[6].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[6].m_Action);
	EXPECT_FALSE(list[6].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, nullptr));
	ASSERT_EQ(7, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED | CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(L"ansi.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED | CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	EXPECT_STREQ(L"utf16-be-bom.txt", list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);
	EXPECT_FALSE(list[1].IsDirectory());
	EXPECT_STREQ(L"utf16-be-nobom.txt", list[2].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[2].m_Action);
	EXPECT_FALSE(list[2].IsDirectory());
	EXPECT_STREQ(L"utf16-le-bom.txt", list[3].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[3].m_Action);
	EXPECT_FALSE(list[3].IsDirectory());
	EXPECT_STREQ(L"utf16-le-nobom.txt", list[4].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[4].m_Action);
	EXPECT_FALSE(list[4].IsDirectory());
	EXPECT_STREQ(L"utf8-bom.txt", list[5].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[5].m_Action);
	EXPECT_FALSE(list[5].IsDirectory());
	EXPECT_STREQ(L"utf8-nobom.txt", list[6].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[6].m_Action);
	EXPECT_FALSE(list[6].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter, false));
	ASSERT_EQ(0, list.GetCount());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter, true));
	ASSERT_EQ(7, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED | CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(L"ansi.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	EXPECT_STREQ(L"utf16-be-bom.txt", list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);
	EXPECT_FALSE(list[1].IsDirectory());
	EXPECT_STREQ(L"utf16-be-nobom.txt", list[2].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[2].m_Action);
	EXPECT_FALSE(list[2].IsDirectory());
	EXPECT_STREQ(L"utf16-le-bom.txt", list[3].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[3].m_Action);
	EXPECT_FALSE(list[3].IsDirectory());
	EXPECT_STREQ(L"utf16-le-nobom.txt", list[4].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[4].m_Action);
	EXPECT_FALSE(list[4].IsDirectory());
	EXPECT_STREQ(L"utf8-bom.txt", list[5].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[5].m_Action);
	EXPECT_FALSE(list[5].IsDirectory());
	EXPECT_STREQ(L"utf8-nobom.txt", list[6].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[6].m_Action);
	EXPECT_FALSE(list[6].IsDirectory());
}

TEST_P(CBasicGitWithTestRepoFixture, GetWorkingTreeChanges_DeleteModifyConflict_DeletedLocally)
{
	if (GetParam() != 0)
		return;

	CString output;
	EXPECT_EQ(0, m_Git.Run(L"git.exe reset --hard master", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);

	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe checkout forconflict", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe rm ansi.txt", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe commit -m \"Prepare conflict case\"", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);
	output.Empty();
	EXPECT_EQ(1, m_Git.Run(L"git.exe merge simple-conflict", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);

	CTGitPathList filter(CTGitPath(L"copy"));

	CTGitPathList list;
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED | CTGitPath::LOGACTIONS_ADDED, list.GetAction());
	EXPECT_STREQ(L"ansi.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED | CTGitPath::LOGACTIONS_ADDED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, nullptr));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED | CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(L"ansi.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED | CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter, false));
	ASSERT_EQ(0, list.GetCount());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter, true));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED, list.GetAction());
	EXPECT_STREQ(L"ansi.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
}

TEST_P(CBasicGitWithEmptyRepositoryFixture, GetWorkingTreeChanges)
{
	if (GetParam() != 0)
		return;

	CTGitPathList list;
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	EXPECT_TRUE(list.IsEmpty());

	CString testFile = m_Dir.GetTempDir() + L"\\test.txt";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(testFile, L"this is testing file."));
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	EXPECT_TRUE(list.IsEmpty());
	CString output;
	EXPECT_EQ(0, m_Git.Run(L"git.exe add test.txt", &output, CP_UTF8));
	EXPECT_STREQ(L"", output);
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	ASSERT_EQ(1, list.GetCount());
	// EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list.GetAction()); // we do not care here for the list action, as its only used in GitLogListBase and there we re-calculate it in AsyncDiffThread
	EXPECT_STREQ(L"test.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());

	CTGitPathList filter(CTGitPath(L"copy"));
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter));
	ASSERT_EQ(1, list.GetCount());
	// EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list.GetAction()); // we do not care here for the list action, as its only used in GitLogListBase and there we re-calculate it in AsyncDiffThread
	EXPECT_STREQ(L"test.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());

	list.Clear();
	EXPECT_TRUE(::CreateDirectory(m_Dir.GetTempDir() + L"\\copy", nullptr));
	testFile = m_Dir.GetTempDir() + L"\\copy\\test2.txt";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(testFile, L"this is another testing file."));
	EXPECT_EQ(0, m_Git.Run(L"git.exe add copy/test2.txt", &output, CP_UTF8));
	EXPECT_STREQ(L"", output);
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter));
	ASSERT_EQ(2, list.GetCount());
	// EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list.GetAction()); // we do not care here for the list action, as its only used in GitLogListBase and there we re-calculate it in AsyncDiffThread
	EXPECT_STREQ(L"copy/test2.txt", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
	EXPECT_STREQ(L"test.txt", list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list[1].m_Action);
	EXPECT_FALSE(list[1].IsDirectory());
}

static int DoSubmodule(const CString& cmd, CGit& git, const CString& submoduleDir, CString& output)
{
	output.Empty();
	CString old = git.m_CurrentDir;
	SCOPE_EXIT { git.m_CurrentDir = old; };
	git.m_CurrentDir = submoduleDir;
	return git.Run(cmd, &output, CP_UTF8);
}

TEST_P(CBasicGitWithSubmoduleRepositoryFixture, GetWorkingTreeChanges_Submodules)
{
	if (GetParam() != 0)
		return;

	CTGitPathList list;

	CString resourcesDir;
	ASSERT_TRUE(GetResourcesDir(resourcesDir));
	CString submoduleDir = m_Dir.GetTempDir() + L"\\submodule";
	ASSERT_TRUE(CreateDirectory(submoduleDir, nullptr));
	ASSERT_TRUE(CreateDirectory(submoduleDir + L"\\.git", nullptr));
	CString repoDir = resourcesDir + L"\\git-repo1";
	CopyRecursively(repoDir, submoduleDir + L"\\.git");

	CString output;
	EXPECT_EQ(0, m_Git.Run(L"git.exe reset --hard master", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);

	// test for clean repo which contains an unrelated git submodule
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	EXPECT_TRUE(list.IsEmpty());

	// test for added, uncommitted submodule
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe add submodule", &output, CP_UTF8));
	//EXPECT_STREQ(L"", output); // Git 2.14 starts to print a warning here
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	ASSERT_EQ(1, list.GetCount());
	// EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list.GetAction()); // we do not care here for the list action, as its only used in GitLogListBase and there we re-calculate it in AsyncDiffThread
	EXPECT_STREQ(L"submodule", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list[0].m_Action);
	EXPECT_TRUE(list[0].IsDirectory());

	// cleanup
	CAutoTempDir::DeleteDirectoryRecursive(submoduleDir);
	submoduleDir = m_Dir.GetTempDir() + L"\\something";

	// test for unchanged submodule
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe reset --hard branch1", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	ASSERT_EQ(0, list.GetCount());

	// test for unchanged submodule with checkout out files
	output.Empty();
	ASSERT_TRUE(CreateDirectory(submoduleDir + L"\\.git", nullptr));
	CopyRecursively(repoDir, submoduleDir + L"\\.git");
	EXPECT_EQ(0, DoSubmodule(L"git.exe reset --hard 49ecdfff36bfe2b9b499b33e5034f427e2fa54dd", m_Git, submoduleDir, output));
	EXPECT_STRNE(L"", output);
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	ASSERT_EQ(0, list.GetCount());

	// test for changed submodule
	EXPECT_EQ(0, DoSubmodule(L"git.exe reset --hard HEAD~1", m_Git, submoduleDir, output));
	EXPECT_STRNE(L"", output);
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_STREQ(L"something", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_TRUE(list[0].IsDirectory());

	// test for modify-delete conflict (deleted remote)
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe checkout --force branch1", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);
	output.Empty();
	EXPECT_EQ(1, m_Git.Run(L"git.exe merge deleted", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_STREQ(L"something", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED | CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_TRUE(list[0].IsDirectory());

	// test for modify-delete conflict (deleted locally)
	output.Empty();
	CAutoTempDir::DeleteDirectoryRecursive(submoduleDir);
	EXPECT_EQ(0, m_Git.Run(L"git.exe checkout --force deleted", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);
	output.Empty();
	EXPECT_EQ(1, m_Git.Run(L"git.exe merge branch1", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_STREQ(L"something", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED | CTGitPath::LOGACTIONS_MISSING, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory()); // neither file nor directory is in filesystem

	// test for merge conflict submodule/file (local submodule, remote file)
	output.Empty();
	CAutoTempDir::DeleteDirectoryRecursive(submoduleDir);
	DeleteFile(submoduleDir);
	EXPECT_EQ(0, m_Git.Run(L"git.exe checkout --force branch1", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);
	output.Empty();
	EXPECT_EQ(1, m_Git.Run(L"git.exe merge file", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_STREQ(L"something", list[0].GetGitPathString());
	if (m_Git.ms_bCygwinGit)
	{
		EXPECT_TRUE(output.Find(L"error: failed to create path") > 0);
		EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED, list[0].m_Action);
		EXPECT_TRUE(list[0].IsDirectory()); // folder is in filesystem
	}
	else
	{
		EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED | CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
		EXPECT_FALSE(list[0].IsDirectory()); // file is in filesystem
	}

	// test for merge conflict submodule/file (remote submodule, local file)
	output.Empty();
	CAutoTempDir::DeleteDirectoryRecursive(submoduleDir);
	DeleteFile(submoduleDir);
	EXPECT_EQ(0, m_Git.Run(L"git.exe checkout --force file", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);
	output.Empty();
	EXPECT_EQ(1, m_Git.Run(L"git.exe merge branch1", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_STREQ(L"something", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED | CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory()); // file is in filesystem

	// test for simple merge conflict
	DeleteFile(submoduleDir);
	CAutoTempDir::DeleteDirectoryRecursive(submoduleDir); // delete .git folder so that we get a simple merge conflict!
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe checkout --force branch1", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);
	output.Empty();
	EXPECT_EQ(1, m_Git.Run(L"git.exe merge branch2", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_STREQ(L"something", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED | CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_TRUE(list[0].IsDirectory());

	// test for submodule to file
	DeleteFile(submoduleDir);
	CAutoTempDir::DeleteDirectoryRecursive(submoduleDir);
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe checkout --force branch1", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe rm -rf something", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);
	ASSERT_TRUE(CStringUtils::WriteStringToTextFile(submoduleDir, L"something"));
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe add something", &output, CP_UTF8));
	EXPECT_STREQ(L"", output);
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_STREQ(L"something", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());

	// test for file to submodule
	DeleteFile(submoduleDir);
	CAutoTempDir::DeleteDirectoryRecursive(submoduleDir);
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe checkout --force file", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe rm something", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);
	ASSERT_TRUE(CreateDirectory(submoduleDir, nullptr));
	ASSERT_TRUE(CreateDirectory(submoduleDir + L"\\.git", nullptr));
	CopyRecursively(repoDir, submoduleDir + L"\\.git");
	EXPECT_EQ(0, DoSubmodule(L"git.exe reset --hard 49ecdfff36bfe2b9b499b33e5034f427e2fa54dd", m_Git, submoduleDir, output));
	EXPECT_STRNE(L"", output);
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe add something", &output, CP_UTF8));
	//EXPECT_STREQ(L"", output); // Git 2.14 starts to print a warning here
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_STREQ(L"something", list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_FALSE(list[0].IsDirectory());
}

TEST_P(CBasicGitWithTestRepoFixture, GetWorkingTreeChanges_RefreshGitIndex)
{
	if (GetParam() != 0)
		return;

	// adding ansi2.txt (as a copy of ansi.txt) produces a warning
	m_Git.SetConfigValue(L"core.autocrlf", L"false");

	CString output;
	EXPECT_EQ(0, m_Git.Run(L"git.exe reset --hard master", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);

	CTGitPathList list;
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	EXPECT_TRUE(list.IsEmpty());

	// touch file
	HANDLE handle = CreateFile(m_Git.m_CurrentDir + L"\\ascii.txt", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	ASSERT_NE(handle, INVALID_HANDLE_VALUE);
	FILETIME ft;
	EXPECT_EQ(TRUE, GetFileTime(handle, nullptr, nullptr, &ft));
	ft.dwLowDateTime -= 1000 * 60 * 1000;
	EXPECT_NE(0, SetFileTime(handle, nullptr, nullptr, &ft));
	CloseHandle(handle);

	// START: this is the undesired behavior
	// this test is just there so we notice when this change somehow
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	if (m_Git.ms_bCygwinGit || m_Git.ms_bMsys2Git)
		EXPECT_EQ(0, list.GetCount());
	else
		EXPECT_EQ(1, list.GetCount());
	list.Clear();
	// END: this is the undesired behavior

	m_Git.RefreshGitIndex(); // without this GetWorkingTreeChanges might report this file as modified (if no msys or cygwin hack is on)
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	EXPECT_EQ(0, list.GetCount());
}

TEST_P(CBasicGitWithTestRepoFixture, GetBisectTerms)
{
	if (m_Git.ms_bCygwinGit)
		return;

	CString good, bad;
	CString output;

	EXPECT_EQ(0, m_Git.Run(L"git.exe bisect start", &output, CP_UTF8));
	m_Git.GetBisectTerms(&good, &bad);
	EXPECT_STREQ(L"good", good);
	EXPECT_STREQ(L"bad", bad);

	good.Empty();
	bad.Empty();
	m_Git.GetBisectTerms(&good, &bad);
	EXPECT_STREQ(L"good", good);
	EXPECT_STREQ(L"bad", bad);

	EXPECT_EQ(0, m_Git.Run(L"git.exe bisect reset", &output, CP_UTF8));

	if (m_Git.GetGitVersion(nullptr, nullptr) < 0x02070000)
		return;

	EXPECT_EQ(0, m_Git.Run(L"git.exe bisect start --term-good=original --term-bad=changed", &output, CP_UTF8));
	m_Git.GetBisectTerms(&good, &bad);
	EXPECT_STREQ(L"original", good);
	EXPECT_STREQ(L"changed", bad);

	EXPECT_EQ(0, m_Git.Run(L"git.exe bisect reset", &output, CP_UTF8));
}

TEST_P(CBasicGitWithTestRepoFixture, GetRefsCommitIsOn)
{
	STRING_VECTOR list;
	EXPECT_EQ(0, m_Git.GetRefsCommitIsOn(list, CGitHash::FromHexStr(L"7c3cbfe13a929d2291a574dca45e4fd2d2ac1aa6"), false, false));
	EXPECT_TRUE(list.empty());

	list.clear();
	EXPECT_EQ(0, m_Git.GetRefsCommitIsOn(list, CGitHash::FromHexStr(L"7c3cbfe13a929d2291a574dca45e4fd2d2ac1aa6"), true, true));
	ASSERT_EQ(1U, list.size());
	EXPECT_STREQ(L"refs/heads/master", list[0]);

	list.clear();
	EXPECT_EQ(0, m_Git.GetRefsCommitIsOn(list, CGitHash::FromHexStr(L"1fc3c9688e27596d8717b54f2939dc951568f6cb"), true, true));
	ASSERT_EQ(1U, list.size());
	EXPECT_STREQ(L"refs/heads/master", list[0]);

	list.clear();
	EXPECT_EQ(0, m_Git.GetRefsCommitIsOn(list, CGitHash::FromHexStr(L"1fc3c9688e27596d8717b54f2939dc951568f6cb"), false, true));
	ASSERT_EQ(1U, list.size());
	EXPECT_STREQ(L"refs/heads/master", list[0]);

	list.clear();
	EXPECT_EQ(0, m_Git.GetRefsCommitIsOn(list, CGitHash::FromHexStr(L"1fc3c9688e27596d8717b54f2939dc951568f6cb"), true, true, CGit::BRANCH_REMOTE));
	EXPECT_TRUE(list.empty());

	list.clear();
	EXPECT_EQ(0, m_Git.GetRefsCommitIsOn(list, CGitHash::FromHexStr(L"1fc3c9688e27596d8717b54f2939dc951568f6cb"), false, true, CGit::BRANCH_ALL));
	ASSERT_EQ(1U, list.size());
	EXPECT_STREQ(L"refs/heads/master", list[0]);

	list.clear();
	EXPECT_EQ(0, m_Git.GetRefsCommitIsOn(list, CGitHash::FromHexStr(L"1fc3c9688e27596d8717b54f2939dc951568f6cb"), true, false));
	EXPECT_TRUE(list.empty());

	list.clear();
	EXPECT_EQ(0, m_Git.GetRefsCommitIsOn(list, CGitHash::FromHexStr(L"b9ef30183497cdad5c30b88d32dc1bed7951dfeb"), true, false));
	ASSERT_EQ(2U, list.size());
	EXPECT_STREQ(L"refs/tags/also-signed", list[0]);
	EXPECT_STREQ(L"refs/tags/normal-tag", list[1]);

	list.clear();
	EXPECT_EQ(0, m_Git.GetRefsCommitIsOn(list, CGitHash::FromHexStr(L"35c91b4ae2f77f4f21a7aba56d3c473c705d89e6"), true, true));
	ASSERT_EQ(3U, list.size());
	EXPECT_STREQ(L"refs/heads/master", list[0]);
	EXPECT_STREQ(L"refs/heads/master2", list[1]);
	EXPECT_STREQ(L"refs/tags/also-signed", list[2]);

	list.clear();
	EXPECT_EQ(0, m_Git.GetRefsCommitIsOn(list, CGitHash::FromHexStr(L"313a41bc88a527289c87d7531802ab484715974f"), false, true));
	ASSERT_EQ(6U, list.size());
	EXPECT_STREQ(L"refs/heads/forconflict", list[0]);
	EXPECT_STREQ(L"refs/heads/master", list[1]);
	EXPECT_STREQ(L"refs/heads/master2", list[2]);
	EXPECT_STREQ(L"refs/heads/signed-commit", list[3]);
	EXPECT_STREQ(L"refs/heads/simple-conflict", list[4]);
	EXPECT_STREQ(L"refs/heads/subdir/branch", list[5]);

	list.clear();
	EXPECT_EQ(0, m_Git.GetRefsCommitIsOn(list, CGitHash::FromHexStr(L"313a41bc88a527289c87d7531802ab484715974f"), false, true, CGit::BRANCH_REMOTE));
	ASSERT_EQ(1U, list.size());
	EXPECT_STREQ(L"refs/remotes/origin/master", list[0]);

	list.clear();
	EXPECT_EQ(0, m_Git.GetRefsCommitIsOn(list, CGitHash::FromHexStr(L"313a41bc88a527289c87d7531802ab484715974f"), false, true, CGit::BRANCH_LOCAL));
	ASSERT_EQ(6U, list.size());
	EXPECT_STREQ(L"refs/heads/forconflict", list[0]);
	EXPECT_STREQ(L"refs/heads/master", list[1]);
	EXPECT_STREQ(L"refs/heads/master2", list[2]);
	EXPECT_STREQ(L"refs/heads/signed-commit", list[3]);
	EXPECT_STREQ(L"refs/heads/simple-conflict", list[4]);
	EXPECT_STREQ(L"refs/heads/subdir/branch", list[5]);

	list.clear();
	EXPECT_EQ(0, m_Git.GetRefsCommitIsOn(list, CGitHash::FromHexStr(L"313a41bc88a527289c87d7531802ab484715974f"), false, true, CGit::BRANCH_ALL));
	ASSERT_EQ(7U, list.size());
	EXPECT_STREQ(L"refs/heads/forconflict", list[0]);
	EXPECT_STREQ(L"refs/remotes/origin/master", list[6]);

	// test for symbolic refs
	CString adminDir;
	ASSERT_TRUE(GitAdminDir::GetAdminDirPath(g_Git.m_CurrentDir, adminDir));
	adminDir += L"refs\\remotes\\origin\\HEAD";
	ASSERT_TRUE(CStringUtils::WriteStringToTextFile(adminDir, L"ref: refs/remotes/origin/master"));
	list.clear();
	EXPECT_EQ(0, m_Git.GetRefsCommitIsOn(list, CGitHash::FromHexStr(L"313a41bc88a527289c87d7531802ab484715974f"), false, true, CGit::BRANCH_ALL));
	ASSERT_EQ(8U, list.size());
	EXPECT_STREQ(L"refs/heads/forconflict", list[0]);
	EXPECT_STREQ(L"refs/remotes/origin/HEAD", list[6]);
	EXPECT_STREQ(L"refs/remotes/origin/master", list[7]);
}

TEST_P(CBasicGitWithTestRepoFixture, GetUnifiedDiff)
{
	CString tmpfile = m_Dir.GetTempDir() + L"\\output.txt";
	EXPECT_EQ(0, m_Git.GetUnifiedDiff(CTGitPath(L""), L"b02add66f48814a73aa2f0876d6bbc8662d6a9a8", L"b9ef30183497cdad5c30b88d32dc1bed7951dfeb", tmpfile, false, false, -1, false));
	CString fileContents;
	EXPECT_EQ(true, CStringUtils::ReadStringFromTextFile(tmpfile, fileContents));
	EXPECT_STREQ(L" utf8-nobom.txt | 4 ++--\n 1 file changed, 2 insertions(+), 2 deletions(-)\n\ndiff --git a/utf8-nobom.txt b/utf8-nobom.txt\nindex ffa0d50..c225b3f 100644\n--- a/utf8-nobom.txt\n+++ b/utf8-nobom.txt\n@@ -1,9 +1,9 @@\n-ä#äf34öööäß€9875oe\r\n+ä#äf34ööcöäß€9875oe\r\n fgdjkglsfdg\r\n öäöü45g\r\n fdgi&§$%&hfdsgä\r\n ä#äf34öööäß€9875oe\r\n-öäüpfgmfdg\r\n+öäcüpfgmfdg\r\n €fgfdsg\r\n 45\r\n äü\n\\ No newline at end of file\n", fileContents);
}

static void GetGitNotes(CGit& m_Git, config testConfig)
{
	if (testConfig != LIBGIT2_ALL)
		return;

	CString notes;
	EXPECT_EQ(0, m_Git.GetGitNotes(CGitHash::FromHexStr(L"1fc3c9688e27596d8717b54f2939dc951568f6cb"), notes));
	EXPECT_STREQ(L"A note here!\n", notes);

	EXPECT_EQ(0, m_Git.GetGitNotes(CGitHash::FromHexStr(L"1ce788330fd3a306c8ad37654063ceee13a7f172"), notes));
	EXPECT_STREQ(L"", notes);
}

TEST_P(CBasicGitWithTestRepoFixture, GetGitNotes)
{
	GetGitNotes(m_Git, GetParam());
}

TEST_P(CBasicGitWithTestRepoBareFixture, GetGitNotes)
{
	GetGitNotes(m_Git, GetParam());
}

TEST_P(CBasicGitWithTestRepoFixture, GuessRefForHash)
{
	CString ref;
	// HEAD
	EXPECT_EQ(0, m_Git.GuessRefForHash(ref, CGitHash::FromHexStr(L"7c3cbfe13a929d2291a574dca45e4fd2d2ac1aa6")));
	EXPECT_STREQ(L"master", ref);

	// local branch
	EXPECT_EQ(0, m_Git.GuessRefForHash(ref, CGitHash::FromHexStr(L"10385764a4d42d7428bbeb245015f8f338fc1e40")));
	EXPECT_STREQ(L"forconflict", ref);

	// stash
	EXPECT_EQ(0, m_Git.GuessRefForHash(ref, CGitHash::FromHexStr(L"18da7c332dcad0f37f9977d9176dce0b0c66f3eb")));
	EXPECT_STREQ(CString(L"18da7c332dcad0f37f9977d9176dce0b0c66f3eb").Left(m_Git.GetShortHASHLength()), ref);

	// tag only
	EXPECT_EQ(0, m_Git.GuessRefForHash(ref, CGitHash::FromHexStr(L"b9ef30183497cdad5c30b88d32dc1bed7951dfeb")));
	EXPECT_STREQ(L"normal-tag", ref);

	// local branch & tag
	EXPECT_EQ(0, m_Git.GuessRefForHash(ref, CGitHash::FromHexStr(L"49ecdfff36bfe2b9b499b33e5034f427e2fa54dd")));
	EXPECT_STREQ(L"master2", ref);

	// remote branch
	EXPECT_EQ(0, m_Git.GuessRefForHash(ref, CGitHash::FromHexStr(L"a9d53b535cb49640a6099860ac4999f5a0857b91")));
	EXPECT_STREQ(L"origin/master", ref);

	// no ref
	EXPECT_EQ(0, m_Git.GuessRefForHash(ref, CGitHash::FromHexStr(L"1ce788330fd3a306c8ad37654063ceee13a7f172")));
	EXPECT_STREQ(CString(L"1ce788330fd3a306c8ad37654063ceee13a7f172").Left(m_Git.GetShortHASHLength()), ref);
}

TEST_P(CBasicGitWithTestRepoFixture, IsResultingCommitBecomeEmpty)
{
	// all files are missing, thus we resulting commit won't be empty
	EXPECT_EQ(FALSE, m_Git.IsResultingCommitBecomeEmpty());
	EXPECT_EQ(FALSE, m_Git.IsResultingCommitBecomeEmpty(true));

	CString output;
	EXPECT_EQ(0, m_Git.Run(L"git.exe reset --hard master", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);

	// now we have no staged stuff
	EXPECT_EQ(TRUE, m_Git.IsResultingCommitBecomeEmpty());
	EXPECT_EQ(FALSE, m_Git.IsResultingCommitBecomeEmpty(true));

	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe commit --allow-empty -m \"Add test.txt\"", &output, CP_UTF8));
	EXPECT_STRNE(L"", output);

	// empty commit and no staged files
	EXPECT_EQ(TRUE, m_Git.IsResultingCommitBecomeEmpty());
	EXPECT_EQ(TRUE, m_Git.IsResultingCommitBecomeEmpty(true));

	// create a file and stage it, thus, resulting commits won't be empty
	CString testFile = m_Dir.GetTempDir() + L"\\test.txt";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(testFile, L"this is testing file."));
	EXPECT_EQ(TRUE, m_Git.IsResultingCommitBecomeEmpty());
	EXPECT_EQ(TRUE, m_Git.IsResultingCommitBecomeEmpty(true));
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(L"git.exe add test.txt", &output, CP_UTF8));
	EXPECT_STREQ(L"", output);

	EXPECT_EQ(FALSE, m_Git.IsResultingCommitBecomeEmpty());
	EXPECT_EQ(FALSE, m_Git.IsResultingCommitBecomeEmpty(true));
}
