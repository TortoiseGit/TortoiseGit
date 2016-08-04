// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2015-2016 - TortoiseGit

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

TEST(CGit, RunSet)
{
	CString output;
	CGit cgit;
	ASSERT_EQ(0, cgit.Run(_T("cmd /c set"), &output, CP_UTF8));
	ASSERT_FALSE(output.IsEmpty());
	ASSERT_TRUE(output.Find(_T("windir"))); // should be there on any MS OS ;)
}

TEST(CGit, RunGit)
{
	CString output;
	CGit cgit;
	ASSERT_EQ(0, cgit.Run(_T("git --version"), &output, CP_UTF8));
	ASSERT_FALSE(output.IsEmpty());
}

TEST(CGit, RunGit_BashPipe)
{
	CString tmpfile = GetTempFile();
	tmpfile.Replace(L"\\", L"/");
	ASSERT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)tmpfile, L"testing piping..."));
	SCOPE_EXIT{ ::DeleteFile(tmpfile); };
	CString pipefile = GetTempFile();
	pipefile.Replace(L"\\", L"/");
	CString pipecmd;
	pipecmd.Format(L"cat < %s", (LPCTSTR)tmpfile);
	ASSERT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)pipefile, (LPCTSTR)pipecmd));
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
	EXPECT_NE(0, cgit.Run(_T("git-not-found.exe"), &output, CP_UTF8)); // Git for Windows returns 2, cygwin-hack returns 127
	//EXPECT_TRUE(output.IsEmpty()); with cygwin-hack we get an error message from sh.exe

	output.Empty();
	EXPECT_EQ(128, cgit.Run(_T("git.exe add file.txt"), &output, CP_UTF8));
	EXPECT_TRUE(output.Find(_T("fatal: Not a git repository (or any")) == 0);
}

TEST_P(CBasicGitWithTestRepoBareFixture, RunGit_AbsolutePath)
{
	CAutoTempDir tempdir;

	CString output;
	EXPECT_EQ(0, m_Git.Run(_T("git archive -o ") + tempdir.GetTempDir() + _T("\\export.zip HEAD"), &output, CP_UTF8));
	EXPECT_TRUE(output.IsEmpty());

	EXPECT_TRUE(PathFileExists(tempdir.GetTempDir() + _T("\\export.zip")));
}

TEST(CGit, StringAppend)
{
	CGit::StringAppend(nullptr, nullptr); // string may be null
	CString string = _T("something");
	CGit::StringAppend(&string, nullptr, CP_UTF8, 0);
	EXPECT_STREQ(_T("something"), string);
	const BYTE somebytes[1] = { 0 };
	CGit::StringAppend(&string, somebytes, CP_UTF8, 0);
	EXPECT_STREQ(_T("something"), string);
	CGit::StringAppend(&string, somebytes);
	EXPECT_STREQ(_T("something"), string);
	const BYTE moreBytesUTFEight[] = { 0x68, 0x65, 0x6C, 0x6C, 0xC3, 0xB6, 0x0A, 0x00 };
	CGit::StringAppend(&string, moreBytesUTFEight, CP_UTF8, 3);
	EXPECT_STREQ(_T("somethinghel"), string);
	CGit::StringAppend(&string, moreBytesUTFEight + 3, CP_ACP, 1);
	EXPECT_STREQ(_T("somethinghell"), string);
	CGit::StringAppend(&string, moreBytesUTFEight);
	EXPECT_STREQ(_T("somethinghellhellö\n"), string);
	CGit::StringAppend(&string, moreBytesUTFEight, CP_UTF8, sizeof(moreBytesUTFEight));
	EXPECT_STREQ(_T("somethinghellhellö\nhellö\n\0"), string);
	CGit::StringAppend(&string, moreBytesUTFEight, CP_UTF8, 3);
	EXPECT_STREQ(_T("somethinghellhellö\nhellö\n\0hel"), string);
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
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)testFile, L"this is testing fileöäü."));

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

	Sleep(1200);
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)testFile, L"this is testing fileöü."));
	__int64 time3 = -1;
	isDir = false;
	size = -1;
	EXPECT_EQ(0, CGit::GetFileModifyTime(testFile, &time3, &isDir, &size));
	EXPECT_NE(-1, time3);
	EXPECT_FALSE(isDir);
	EXPECT_EQ(25, size);
	EXPECT_TRUE(time3 >= time);
	EXPECT_TRUE(time3 - time <= 1 + (__int64)(GetTickCount64() - ticks) / 1000);
}

TEST(CGit, LoadTextFile)
{
	CAutoTempDir tempdir;

	CString msg = L"something--";
	EXPECT_FALSE(CGit::LoadTextFile(L"does-not-exist.txt", msg));
	EXPECT_STREQ(L"something--", msg);

	CString testFile = tempdir.GetTempDir() + L"\\test.txt";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)testFile, L"this is testing fileöäü."));
	EXPECT_TRUE(CGit::LoadTextFile(testFile, msg));
	EXPECT_STREQ(L"something--this is testing fileöäü.\n", msg);

	msg.Empty();
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)testFile, L"this is testing\nfileöäü."));
	EXPECT_TRUE(CGit::LoadTextFile(testFile, msg));
	EXPECT_STREQ(L"this is testing\nfileöäü.\n", msg);

	msg.Empty();
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)testFile, L"this is\r\ntesting\nfileöäü.\r\n\r\n"));
	EXPECT_TRUE(CGit::LoadTextFile(testFile, msg));
	EXPECT_STREQ(L"this is\ntesting\nfileöäü.\n", msg);
}

TEST(CGit, IsBranchNameValid)
{
	CGit cgit;
	EXPECT_TRUE(cgit.IsBranchNameValid(_T("master")));
	EXPECT_TRUE(cgit.IsBranchNameValid(_T("def/master"))); 
	EXPECT_FALSE(cgit.IsBranchNameValid(_T("-test")));
	EXPECT_FALSE(cgit.IsBranchNameValid(_T("jfjf>ff")));
	EXPECT_FALSE(cgit.IsBranchNameValid(_T("jf ff")));
	EXPECT_FALSE(cgit.IsBranchNameValid(_T("jf~ff")));
}

TEST(CGit, StripRefName)
{
	EXPECT_STREQ(_T("abc"), CGit::StripRefName(_T("abc")));
	EXPECT_STREQ(_T("bcd"), CGit::StripRefName(_T("refs/bcd")));
	EXPECT_STREQ(_T("cde"), CGit::StripRefName(_T("refs/heads/cde")));
}

TEST(CGit, CombinePath)
{
	CGit cgit;
	cgit.m_CurrentDir = _T("c:\\something");
	EXPECT_STREQ(_T("c:\\something"), cgit.CombinePath(_T("")));
	EXPECT_STREQ(_T("c:\\something\\file.txt"), cgit.CombinePath(_T("file.txt")));
	EXPECT_STREQ(_T("c:\\something\\sub\\file.txt"), cgit.CombinePath(_T("sub\\file.txt")));
	EXPECT_STREQ(_T("c:\\something\\subdir\\file2.txt"), cgit.CombinePath(CTGitPath(_T("subdir/file2.txt"))));
}

TEST(CGit, GetShortName)
{
	CGit::REF_TYPE type = CGit::UNKNOWN;
	EXPECT_STREQ(_T("master"), CGit::GetShortName(_T("refs/heads/master"), &type));
	EXPECT_EQ(CGit::LOCAL_BRANCH, type);

	type = CGit::UNKNOWN;
	EXPECT_STREQ(_T("somedir/mastr"), CGit::GetShortName(_T("refs/heads/somedir/mastr"), &type));
	EXPECT_EQ(CGit::LOCAL_BRANCH, type);

	type = CGit::BISECT_BAD; // do not use UNKNOWN here to make sure it gets set
	EXPECT_STREQ(_T("svn/something"), CGit::GetShortName(_T("refs/svn/something"), &type));
	EXPECT_EQ(CGit::UNKNOWN, type);

	type = CGit::UNKNOWN;
	EXPECT_STREQ(_T("origin/master"), CGit::GetShortName(_T("refs/remotes/origin/master"), &type));
	EXPECT_EQ(CGit::REMOTE_BRANCH, type);

	type = CGit::UNKNOWN;
	EXPECT_STREQ(_T("origin/sub/master"), CGit::GetShortName(_T("refs/remotes/origin/sub/master"), &type));
	EXPECT_EQ(CGit::REMOTE_BRANCH, type);

	type = CGit::UNKNOWN;
	EXPECT_STREQ(_T("release1"), CGit::GetShortName(_T("refs/tags/release1"), &type));
	EXPECT_EQ(CGit::TAG, type);

	type = CGit::UNKNOWN;
	EXPECT_STREQ(_T("releases/v1"), CGit::GetShortName(_T("refs/tags/releases/v1"), &type));
	EXPECT_EQ(CGit::TAG, type);

	type = CGit::UNKNOWN;
	EXPECT_STREQ(_T("release2"), CGit::GetShortName(_T("refs/tags/release2^{}"), &type));
	EXPECT_EQ(CGit::ANNOTATED_TAG, type);

	type = CGit::UNKNOWN;
	EXPECT_STREQ(_T("releases/v2"), CGit::GetShortName(_T("refs/tags/releases/v2^{}"), &type));
	EXPECT_EQ(CGit::ANNOTATED_TAG, type);

	type = CGit::UNKNOWN;
	EXPECT_STREQ(_T("stash"), CGit::GetShortName(_T("refs/stash"), &type));
	EXPECT_EQ(CGit::STASH, type);

	type = CGit::BISECT_BAD; // do not use UNKNOWN here to make sure it gets set
	EXPECT_STREQ(_T("something"), CGit::GetShortName(_T("refs/something"), &type));
	EXPECT_EQ(CGit::UNKNOWN, type);

	type = CGit::BISECT_BAD; // do not use UNKNOWN here to make sure it gets set
	EXPECT_STREQ(_T("sth"), CGit::GetShortName(_T("sth"), &type));
	EXPECT_EQ(CGit::UNKNOWN, type);

	type = CGit::UNKNOWN;
	EXPECT_STREQ(_T("good"), CGit::GetShortName(_T("refs/bisect/good"), &type));
	EXPECT_EQ(CGit::BISECT_GOOD, type);

	type = CGit::UNKNOWN;
	EXPECT_STREQ(_T("good"), CGit::GetShortName(_T("refs/bisect/good-5809ac97a1115a8380b1d6bb304b62cd0b0fa9bb"), &type));
	EXPECT_EQ(CGit::BISECT_GOOD, type);

	type = CGit::UNKNOWN;
	EXPECT_STREQ(_T("bad"), CGit::GetShortName(_T("refs/bisect/bad"), &type));
	EXPECT_EQ(CGit::BISECT_BAD, type);

	type = CGit::UNKNOWN;
	EXPECT_STREQ(_T("bad"), CGit::GetShortName(_T("refs/bisect/bad-5809ac97a1115a8380b1d6bb304b62cd0b0fd9bb"), &type));
	EXPECT_EQ(CGit::BISECT_BAD, type);

	type = CGit::UNKNOWN;
	EXPECT_STREQ(_T("ab"), CGit::GetShortName(_T("refs/notes/ab"), &type));
	EXPECT_EQ(CGit::NOTES, type);

	type = CGit::UNKNOWN;
	EXPECT_STREQ(_T("a/b"), CGit::GetShortName(_T("refs/notes/a/b"), &type));
	EXPECT_EQ(CGit::NOTES, type);
}

TEST(CGit, GetRepository)
{
	CAutoTempDir tempdir;
	CGit cgit;
	cgit.m_CurrentDir = tempdir.GetTempDir();

	CAutoRepository repo = cgit.GetGitRepository();
	EXPECT_FALSE(repo.IsValid());

	cgit.m_CurrentDir = tempdir.GetTempDir() + _T("\\aöäüb");
	ASSERT_TRUE(CreateDirectory(cgit.m_CurrentDir, nullptr));

	CString output;
	EXPECT_EQ(0, cgit.Run(_T("git.exe init"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());

	CAutoRepository repo2 = cgit.GetGitRepository(); // this tests GetGitRepository as well as m_Git.GetGitPathStringA
	EXPECT_TRUE(repo2.IsValid());

	cgit.m_CurrentDir = tempdir.GetTempDir() + _T("\\aöäüb.git");
	ASSERT_TRUE(CreateDirectory(cgit.m_CurrentDir, nullptr));

	output.Empty();
	EXPECT_EQ(0, cgit.Run(_T("git.exe init --bare"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());

	CAutoRepository repo3 = cgit.GetGitRepository(); // this tests GetGitRepository as well as m_Git.GetGitPathStringA
	EXPECT_TRUE(repo3.IsValid());
}

TEST_P(CBasicGitWithEmptyRepositoryFixture, IsInitRepos_GetInitAddList)
{
	EXPECT_STREQ(_T("master"), m_Git.GetCurrentBranch());

	CString output;
	CString testFile = m_Dir.GetTempDir() + L"\\test.txt";

	CTGitPathList addedFiles;

	EXPECT_TRUE(m_Git.IsInitRepos());
	EXPECT_EQ(0, m_Git.GetInitAddList(addedFiles));
	EXPECT_TRUE(addedFiles.IsEmpty());

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)testFile, L"this is testing file."));
	EXPECT_EQ(0, m_Git.GetInitAddList(addedFiles));
	EXPECT_TRUE(addedFiles.IsEmpty());
	EXPECT_EQ(0, m_Git.Run(_T("git.exe add test.txt"), &output, CP_UTF8));
	EXPECT_TRUE(output.IsEmpty());
	EXPECT_EQ(0, m_Git.GetInitAddList(addedFiles));
	ASSERT_EQ(1, addedFiles.GetCount());
	EXPECT_STREQ(_T("test.txt"), addedFiles[0].GetGitPathString());

	output.Empty();
	EXPECT_EQ(0, m_Git.Run(_T("git.exe commit -m \"Add test.txt\""), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());

	EXPECT_FALSE(m_Git.IsInitRepos());

	EXPECT_STREQ(_T("master"), m_Git.GetCurrentBranch());
}

TEST_P(CBasicGitWithTestRepoFixture, IsInitRepos)
{
	EXPECT_FALSE(m_Git.IsInitRepos());

	CString output;
	EXPECT_EQ(0, m_Git.Run(_T("git.exe checkout --orphan orphanic"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());

	EXPECT_TRUE(m_Git.IsInitRepos());
}

TEST_P(CBasicGitWithTestRepoFixture, HasWorkingTreeConflicts)
{
	CString output;
	EXPECT_EQ(0, m_Git.Run(_T("git.exe reset --hard master"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());

	EXPECT_EQ(FALSE, m_Git.HasWorkingTreeConflicts());

	output.Empty();
	EXPECT_EQ(0, m_Git.Run(_T("git.exe merge forconflict"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());
	EXPECT_EQ(FALSE, m_Git.HasWorkingTreeConflicts());

	output.Empty();
	EXPECT_EQ(1, m_Git.Run(_T("git.exe merge simple-conflict"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());
	EXPECT_EQ(TRUE, m_Git.HasWorkingTreeConflicts());

	output.Empty();
	EXPECT_EQ(0, m_Git.Run(_T("git.exe reset --hard master"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());

	output.Empty();
	EXPECT_EQ(0, m_Git.Run(_T("git.exe checkout forconflict"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());

	output.Empty();
	EXPECT_EQ(1, m_Git.Run(_T("git.exe merge simple-conflict"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());
	EXPECT_EQ(TRUE, m_Git.HasWorkingTreeConflicts());
}

TEST_P(CBasicGitWithTestRepoFixture, GetCurrentBranch)
{
	EXPECT_STREQ(_T("master"), m_Git.GetCurrentBranch());
	EXPECT_STREQ(_T("master"), m_Git.GetCurrentBranch(true));

	CString output;
	EXPECT_EQ(0, m_Git.Run(_T("git.exe checkout simple-conflict"), &output, CP_UTF8));
	EXPECT_STREQ(_T("simple-conflict"), m_Git.GetCurrentBranch());
	EXPECT_STREQ(_T("simple-conflict"), m_Git.GetCurrentBranch(true));

	EXPECT_EQ(0, m_Git.Run(_T("git.exe checkout subdir/branch"), &output, CP_UTF8));
	EXPECT_STREQ(_T("subdir/branch"), m_Git.GetCurrentBranch());
	EXPECT_STREQ(_T("subdir/branch"), m_Git.GetCurrentBranch(true));

	EXPECT_EQ(0, m_Git.Run(_T("git.exe checkout 560deea87853158b22d0c0fd73f60a458d47838a"), &output, CP_UTF8));
	EXPECT_STREQ(_T("(no branch)"), m_Git.GetCurrentBranch());
	EXPECT_STREQ(_T("560deea87853158b22d0c0fd73f60a458d47838a"), m_Git.GetCurrentBranch(true));

	output.Empty();
	EXPECT_EQ(0, m_Git.Run(_T("git.exe checkout --orphan orphanic"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());

	EXPECT_STREQ(_T("orphanic"), m_Git.GetCurrentBranch());
	EXPECT_STREQ(_T("orphanic"), m_Git.GetCurrentBranch(true));
}

TEST_P(CBasicGitWithTestRepoBareFixture, GetCurrentBranch)
{
	EXPECT_STREQ(_T("master"), m_Git.GetCurrentBranch());
	EXPECT_STREQ(_T("master"), m_Git.GetCurrentBranch(true));
}

static void BranchTagExists_IsBranchTagNameUnique(CGit& m_Git)
{
	EXPECT_TRUE(m_Git.BranchTagExists(_T("master"), true));
	EXPECT_FALSE(m_Git.BranchTagExists(_T("origin/master"), true));
	EXPECT_FALSE(m_Git.BranchTagExists(_T("normal-tag"), true));
	EXPECT_FALSE(m_Git.BranchTagExists(_T("also-signed"), true));
	EXPECT_FALSE(m_Git.BranchTagExists(_T("wuseldusel"), true));

	EXPECT_FALSE(m_Git.BranchTagExists(_T("master"), false));
	EXPECT_TRUE(m_Git.BranchTagExists(_T("normal-tag"), false));
	EXPECT_TRUE(m_Git.BranchTagExists(_T("also-signed"), false));
	EXPECT_FALSE(m_Git.BranchTagExists(_T("wuseldusel"), false));

	EXPECT_TRUE(m_Git.IsBranchTagNameUnique(_T("master")));
	EXPECT_TRUE(m_Git.IsBranchTagNameUnique(_T("simpleconflict")));
	EXPECT_TRUE(m_Git.IsBranchTagNameUnique(_T("normal-tag")));
	EXPECT_TRUE(m_Git.IsBranchTagNameUnique(_T("also-signed")));
	EXPECT_TRUE(m_Git.IsBranchTagNameUnique(_T("origin/master")));

	CString output;
	EXPECT_EQ(0, m_Git.Run(_T("git.exe tag master HEAD~2"), &output, CP_UTF8));
	EXPECT_TRUE(output.IsEmpty());

	EXPECT_EQ(0, m_Git.Run(_T("git.exe branch normal-tag HEAD~2"), &output, CP_UTF8));
	EXPECT_TRUE(output.IsEmpty());

	EXPECT_FALSE(m_Git.IsBranchTagNameUnique(_T("master")));
	EXPECT_FALSE(m_Git.IsBranchTagNameUnique(_T("normal-tag")));
	EXPECT_TRUE(m_Git.IsBranchTagNameUnique(_T("also-signed")));
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
	EXPECT_STREQ(_T(""), m_Git.GetFullRefName(_T("does_not_exist")));
	EXPECT_STREQ(_T("refs/heads/master"), m_Git.GetFullRefName(_T("master")));
	EXPECT_STREQ(_T("refs/remotes/origin/master"), m_Git.GetFullRefName(_T("origin/master")));
	EXPECT_STREQ(_T("refs/tags/normal-tag"), m_Git.GetFullRefName(_T("normal-tag")));
	EXPECT_STREQ(_T("refs/tags/also-signed"), m_Git.GetFullRefName(_T("also-signed")));

	CString output;
	EXPECT_EQ(0, m_Git.Run(_T("git.exe tag master HEAD~2"), &output, CP_UTF8));
	EXPECT_TRUE(output.IsEmpty());
	EXPECT_STREQ(_T(""), m_Git.GetFullRefName(_T("master")));
	EXPECT_STREQ(_T("refs/remotes/origin/master"), m_Git.GetFullRefName(_T("origin/master")));

	EXPECT_EQ(0, m_Git.Run(_T("git.exe branch normal-tag HEAD~2"), &output, CP_UTF8));
	EXPECT_TRUE(output.IsEmpty());
	EXPECT_STREQ(_T(""), m_Git.GetFullRefName(_T("normal-tag")));

	EXPECT_EQ(0, m_Git.Run(_T("git.exe branch origin/master HEAD~2"), &output, CP_UTF8));
	EXPECT_TRUE(output.IsEmpty());
	EXPECT_STREQ(_T(""), m_Git.GetFullRefName(_T("origin/master")));
}

TEST_P(CBasicGitWithTestRepoFixture, GetFullRefName)
{
	GetFullRefName(m_Git);

	CString output;
	EXPECT_EQ(0, m_Git.Run(_T("git.exe checkout --orphan orphanic"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());
	EXPECT_STREQ(_T(""), m_Git.GetFullRefName(_T("orphanic")));
}

TEST_P(CBasicGitWithTestRepoBareFixture, GetFullRefName)
{
	GetFullRefName(m_Git);
}

TEST_P(CBasicGitWithEmptyRepositoryFixture, GetRemoteTrackedBranch)
{
	CString remote, branch;
	m_Git.GetRemoteTrackedBranchForHEAD(remote, branch);
	EXPECT_TRUE(remote.IsEmpty());
	EXPECT_TRUE(branch.IsEmpty());

	m_Git.GetRemoteTrackedBranch(_T("master"), remote, branch);
	EXPECT_TRUE(remote.IsEmpty());
	EXPECT_TRUE(branch.IsEmpty());

	m_Git.GetRemoteTrackedBranch(_T("non-existing"), remote, branch);
	EXPECT_TRUE(remote.IsEmpty());
	EXPECT_TRUE(branch.IsEmpty());
}

static void GetRemoteTrackedBranch(CGit& m_Git)
{
	CString remote, branch;
	m_Git.GetRemoteTrackedBranchForHEAD(remote, branch);
	EXPECT_STREQ(_T("origin"), remote);
	EXPECT_STREQ(_T("master"), branch);

	remote.Empty();
	branch.Empty();
	m_Git.GetRemoteTrackedBranch(_T("master"), remote, branch);
	EXPECT_STREQ(_T("origin"), remote);
	EXPECT_STREQ(_T("master"), branch);

	remote.Empty();
	branch.Empty();
	m_Git.GetRemoteTrackedBranch(_T("non-existing"), remote, branch);
	EXPECT_TRUE(remote.IsEmpty());
	EXPECT_TRUE(branch.IsEmpty());
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
	m_Git.GetRemotePushBranch(_T("master"), remote, branch);
	EXPECT_TRUE(remote.IsEmpty());
	EXPECT_TRUE(branch.IsEmpty());

	m_Git.GetRemotePushBranch(_T("non-existing"), remote, branch);
	EXPECT_TRUE(remote.IsEmpty());
	EXPECT_TRUE(branch.IsEmpty());
}

static void GetRemotePushBranch(CGit& m_Git)
{
	CString remote, branch;
	m_Git.GetRemotePushBranch(_T("master"), remote, branch);
	EXPECT_STREQ(_T("origin"), remote);
	EXPECT_STREQ(_T("master"), branch);

	remote.Empty();
	branch.Empty();
	m_Git.GetRemotePushBranch(_T("non-existing"), remote, branch);
	EXPECT_TRUE(remote.IsEmpty());
	EXPECT_TRUE(branch.IsEmpty());

	CAutoRepository repo(m_Git.GetGitRepository());
	ASSERT_TRUE(repo.IsValid());
	CAutoConfig config(repo);
	ASSERT_TRUE(config.IsValid());

	remote.Empty();
	branch.Empty();
	EXPECT_EQ(0, git_config_set_string(config, "remote.pushDefault", "originpush2"));
	m_Git.GetRemotePushBranch(_T("master"), remote, branch);
	EXPECT_STREQ(_T("originpush2"), remote);
	EXPECT_STREQ(_T("master"), branch);

	remote.Empty();
	branch.Empty();
	EXPECT_EQ(0, git_config_set_string(config, "branch.master.pushremote", "originpush3"));
	m_Git.GetRemotePushBranch(_T("master"), remote, branch);
	EXPECT_STREQ(_T("originpush3"), remote);
	EXPECT_STREQ(_T("master"), branch);

	remote.Empty();
	branch.Empty();
	EXPECT_EQ(0, git_config_set_string(config, "branch.master.pushbranch", "masterbranch2"));
	m_Git.GetRemotePushBranch(_T("master"), remote, branch);
	EXPECT_STREQ(_T("originpush3"), remote);
	EXPECT_STREQ(_T("masterbranch2"), branch);

	remote.Empty();
	branch.Empty();
	m_Git.GetRemotePushBranch(_T("non-existing"), remote, branch);
	EXPECT_STREQ(_T("originpush2"), remote);
	EXPECT_TRUE(branch.IsEmpty());
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
	EXPECT_TRUE(m_Git.CanParseRev(_T("")));
	EXPECT_TRUE(m_Git.CanParseRev(_T("HEAD")));
	EXPECT_TRUE(m_Git.CanParseRev(_T("master")));
	EXPECT_TRUE(m_Git.CanParseRev(_T("heads/master")));
	EXPECT_TRUE(m_Git.CanParseRev(_T("refs/heads/master")));
	EXPECT_TRUE(m_Git.CanParseRev(_T("master~1")));
	EXPECT_TRUE(m_Git.CanParseRev(_T("master forconflict")));
	EXPECT_TRUE(m_Git.CanParseRev(_T("origin/master..master")));
	EXPECT_TRUE(m_Git.CanParseRev(_T("origin/master...master")));
	EXPECT_TRUE(m_Git.CanParseRev(_T("49ecdfff36bfe2b9b499b33e5034f427e2fa54dd")));
	EXPECT_FALSE(m_Git.CanParseRev(_T("non-existing")));
	EXPECT_TRUE(m_Git.CanParseRev(_T("normal-tag")));
	EXPECT_TRUE(m_Git.CanParseRev(_T("tags/normal-tag")));
	EXPECT_TRUE(m_Git.CanParseRev(_T("refs/tags/normal-tag")));
	EXPECT_TRUE(m_Git.CanParseRev(_T("all-files-signed")));
	EXPECT_TRUE(m_Git.CanParseRev(_T("all-files-signed^{}")));

	EXPECT_FALSE(m_Git.CanParseRev(_T("orphanic")));
}

TEST_P(CBasicGitWithTestRepoFixture, CanParseRev)
{
	CanParseRev(m_Git);

	CString output;
	EXPECT_EQ(0, m_Git.Run(_T("git.exe checkout --orphan orphanic"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());
	EXPECT_FALSE(m_Git.CanParseRev(_T("")));
	EXPECT_FALSE(m_Git.CanParseRev(_T("HEAD")));
	EXPECT_FALSE(m_Git.CanParseRev(_T("orphanic")));
	EXPECT_TRUE(m_Git.CanParseRev(_T("master")));
}

TEST_P(CBasicGitWithTestRepoBareFixture, CanParseRev)
{
	CanParseRev(m_Git);
}

static void FETCHHEAD(CGit& m_Git, bool isBare)
{
	CString repoDir = m_Git.m_CurrentDir;
	if (!isBare)
		repoDir += _T("\\.git");

	STRING_VECTOR list;
	EXPECT_EQ(0, m_Git.GetBranchList(list, nullptr));
	EXPECT_EQ(5, list.size());
	list.clear();
	EXPECT_EQ(0, m_Git.GetBranchList(list, nullptr, CGit::BRANCH_LOCAL_F));
	EXPECT_EQ(5, list.size());

	EXPECT_STREQ(_T("HEAD"), m_Git.FixBranchName(_T("HEAD")));
	EXPECT_STREQ(_T("master"), m_Git.FixBranchName(_T("master")));
	EXPECT_STREQ(_T("non-existing"), m_Git.FixBranchName(_T("non-existing")));
	CString branch = _T("master");
	EXPECT_STREQ(_T("master"), m_Git.FixBranchName_Mod(branch));
	EXPECT_STREQ(_T("master"), branch);
	branch = _T("non-existing");
	EXPECT_STREQ(_T("non-existing"), m_Git.FixBranchName_Mod(branch));
	EXPECT_STREQ(_T("non-existing"), branch);
	CGitHash hash;
	EXPECT_NE(0, m_Git.GetHash(hash, _T("FETCH_HEAD")));

	CString testFile = repoDir + L"\\FETCH_HEAD";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)testFile, L"b9ef30183497cdad5c30b88d32dc1bed7951dfeb		branch 'master' of https://code.google.com/p/tortoisegit\n737878a4e2eabfa4fab580867c2b060c70999d31	not-for-merge	branch 'extend_hooks' of https://code.google.com/p/tortoisegit\n"));

	list.clear();
	EXPECT_EQ(0, m_Git.GetBranchList(list, nullptr));
	EXPECT_EQ(5, list.size());
	list.clear();
	EXPECT_EQ(0, m_Git.GetBranchList(list, nullptr, CGit::BRANCH_LOCAL_F));
	EXPECT_EQ(6, list.size());

	EXPECT_STREQ(_T("master"), m_Git.FixBranchName(_T("master")));
	EXPECT_STREQ(_T("non-existing"), m_Git.FixBranchName(_T("non-existing")));
	EXPECT_STREQ(_T("b9ef30183497cdad5c30b88d32dc1bed7951dfeb"), m_Git.FixBranchName(_T("FETCH_HEAD")));
	branch = _T("HEAD");
	EXPECT_STREQ(_T("HEAD"), m_Git.FixBranchName_Mod(branch));
	EXPECT_STREQ(_T("HEAD"), branch);
	branch = _T("master");
	EXPECT_STREQ(_T("master"), m_Git.FixBranchName_Mod(branch));
	EXPECT_STREQ(_T("master"), branch);
	branch = _T("non-existing");
	EXPECT_STREQ(_T("non-existing"), m_Git.FixBranchName_Mod(branch));
	EXPECT_STREQ(_T("non-existing"), branch);
	branch = _T("FETCH_HEAD");
	EXPECT_STREQ(_T("b9ef30183497cdad5c30b88d32dc1bed7951dfeb"), m_Git.FixBranchName_Mod(branch));
	EXPECT_STREQ(_T("b9ef30183497cdad5c30b88d32dc1bed7951dfeb"), branch);
	EXPECT_EQ(0, m_Git.GetHash(hash, _T("FETCH_HEAD")));
	EXPECT_STREQ(_T("b9ef30183497cdad5c30b88d32dc1bed7951dfeb"), hash.ToString());

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)testFile, L"737878a4e2eabfa4fab580867c2b060c70999d31	not-for-merge	branch 'extend_hooks' of https://code.google.com/p/tortoisegit\nb9ef30183497cdad5c30b88d32dc1bed7951dfeb		branch 'master' of https://code.google.com/p/tortoisegit\n"));

	list.clear();
	EXPECT_EQ(0, m_Git.GetBranchList(list, nullptr));
	EXPECT_EQ(5, list.size());
	list.clear();
	EXPECT_EQ(0, m_Git.GetBranchList(list, nullptr, CGit::BRANCH_LOCAL_F));
	EXPECT_EQ(6, list.size());

	EXPECT_STREQ(_T("b9ef30183497cdad5c30b88d32dc1bed7951dfeb"), m_Git.FixBranchName(_T("FETCH_HEAD")));
	branch = _T("FETCH_HEAD");
	EXPECT_STREQ(_T("b9ef30183497cdad5c30b88d32dc1bed7951dfeb"), m_Git.FixBranchName_Mod(branch));
	EXPECT_STREQ(_T("b9ef30183497cdad5c30b88d32dc1bed7951dfeb"), branch);
	// libgit2 fails here
	//EXPECT_EQ(0, m_Git.GetHash(hash, _T("FETCH_HEAD")));
	//EXPECT_STREQ(_T("b9ef30183497cdad5c30b88d32dc1bed7951dfeb"), hash.ToString());
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
	EXPECT_TRUE(m_Git.IsFastForward(_T("origin/master"), _T("master"), &commonAncestor));
	EXPECT_STREQ(_T("a9d53b535cb49640a6099860ac4999f5a0857b91"), commonAncestor.ToString());

	EXPECT_FALSE(m_Git.IsFastForward(_T("simple-conflict"), _T("master"), &commonAncestor));
	EXPECT_STREQ(_T("b02add66f48814a73aa2f0876d6bbc8662d6a9a8"), commonAncestor.ToString());
}

static void GetHash(CGit& m_Git)
{
	CGitHash hash;
	EXPECT_EQ(0, m_Git.GetHash(hash, _T("HEAD")));
	EXPECT_STREQ(_T("7c3cbfe13a929d2291a574dca45e4fd2d2ac1aa6"), hash.ToString());
	EXPECT_EQ(0, m_Git.GetHash(hash, _T("HEAD~1")));
	EXPECT_STREQ(_T("1fc3c9688e27596d8717b54f2939dc951568f6cb"), hash.ToString());
	EXPECT_EQ(0, m_Git.GetHash(hash, _T("ff1fbef1a54a9849afd4a5e94d2ca4d80d5b96c2")));
	EXPECT_STREQ(_T("ff1fbef1a54a9849afd4a5e94d2ca4d80d5b96c2"), hash.ToString());
	EXPECT_EQ(0, m_Git.GetHash(hash, _T("master")));
	EXPECT_STREQ(_T("7c3cbfe13a929d2291a574dca45e4fd2d2ac1aa6"), hash.ToString());
	EXPECT_EQ(0, m_Git.GetHash(hash, _T("origin/master")));
	EXPECT_STREQ(_T("a9d53b535cb49640a6099860ac4999f5a0857b91"), hash.ToString());
	EXPECT_EQ(0, m_Git.GetHash(hash, _T("49ecdfff36bfe2b9b499b33e5034f427e2fa54dd")));
	EXPECT_STREQ(_T("49ecdfff36bfe2b9b499b33e5034f427e2fa54dd"), hash.ToString());
	EXPECT_EQ(0, m_Git.GetHash(hash, _T("normal-tag")));
	EXPECT_STREQ(_T("b9ef30183497cdad5c30b88d32dc1bed7951dfeb"), hash.ToString());
	EXPECT_EQ(0, m_Git.GetHash(hash, _T("all-files-signed")));
	EXPECT_STREQ(_T("ab555b2776c6b700ad93848d0dd050e7d08be779"), hash.ToString()); // maybe we need automatically to dereference it
	EXPECT_EQ(0, m_Git.GetHash(hash, _T("all-files-signed^{}")));
	EXPECT_STREQ(_T("313a41bc88a527289c87d7531802ab484715974f"), hash.ToString());

	EXPECT_NE(0, m_Git.GetHash(hash, _T("non-existing")));
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
	EXPECT_EQ(0, m_Git.GetHash(hash, _T("HEAD")));
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
	ASSERT_EQ(5, branches.size());
	EXPECT_EQ(1, current);
	EXPECT_STREQ(_T("forconflict"), branches[0]);
	EXPECT_STREQ(_T("master"), branches[1]);
	EXPECT_STREQ(_T("master2"), branches[2]);
	EXPECT_STREQ(_T("simple-conflict"), branches[3]);
	EXPECT_STREQ(_T("subdir/branch"), branches[4]);

	branches.clear();
	current = -2;
	EXPECT_EQ(0, m_Git.GetBranchList(branches, &current, CGit::BRANCH_ALL));
	ASSERT_EQ(6, branches.size());
	EXPECT_EQ(1, current);
	EXPECT_STREQ(_T("forconflict"), branches[0]);
	EXPECT_STREQ(_T("master"), branches[1]);
	EXPECT_STREQ(_T("master2"), branches[2]);
	EXPECT_STREQ(_T("simple-conflict"), branches[3]);
	EXPECT_STREQ(_T("subdir/branch"), branches[4]);
	EXPECT_STREQ(_T("remotes/origin/master"), branches[5]);

	branches.clear();
	current = -2;
	EXPECT_EQ(0, m_Git.GetBranchList(branches, &current, CGit::BRANCH_REMOTE));
	ASSERT_EQ(1, branches.size());
	EXPECT_EQ(-2, current); // not touched
	EXPECT_STREQ(_T("remotes/origin/master"), branches[0]);

	STRING_VECTOR tags;
	EXPECT_EQ(0, m_Git.GetTagList(tags));
	ASSERT_EQ(3, tags.size());
	EXPECT_STREQ(_T("all-files-signed"), tags[0]);
	EXPECT_STREQ(_T("also-signed"), tags[1]);
	EXPECT_STREQ(_T("normal-tag"), tags[2]);

	STRING_VECTOR refs;
	EXPECT_EQ(0, m_Git.GetRefList(refs));
	ASSERT_EQ(11, refs.size());
	EXPECT_STREQ(_T("refs/heads/forconflict"), refs[0]);
	EXPECT_STREQ(_T("refs/heads/master"), refs[1]);
	EXPECT_STREQ(_T("refs/heads/master2"), refs[2]);
	EXPECT_STREQ(_T("refs/heads/simple-conflict"), refs[3]);
	EXPECT_STREQ(_T("refs/heads/subdir/branch"), refs[4]);
	EXPECT_STREQ(_T("refs/notes/commits"), refs[5]);
	EXPECT_STREQ(_T("refs/remotes/origin/master"), refs[6]);
	EXPECT_STREQ(_T("refs/stash"), refs[7]);
	EXPECT_STREQ(_T("refs/tags/all-files-signed"), refs[8]);
	EXPECT_STREQ(_T("refs/tags/also-signed"), refs[9]);
	EXPECT_STREQ(_T("refs/tags/normal-tag"), refs[10]);

	MAP_HASH_NAME map;
	EXPECT_EQ(0, m_Git.GetMapHashToFriendName(map));
	if (testConfig == GIT_CLI || testConfig == LIBGIT)
		ASSERT_EQ(12, map.size()); // also contains the undereferenced tags with hashes
	else
		ASSERT_EQ(10, map.size());

	ASSERT_EQ(1, map[CGitHash(L"7c3cbfe13a929d2291a574dca45e4fd2d2ac1aa6")].size());
	EXPECT_STREQ(_T("refs/heads/master"), map[CGitHash(L"7c3cbfe13a929d2291a574dca45e4fd2d2ac1aa6")][0]);
	ASSERT_EQ(1, map[CGitHash(L"4c5c93d2a0b368bc4570d5ec02ab03b9c4334d44")].size());
	EXPECT_STREQ(_T("refs/heads/subdir/branch"), map[CGitHash(L"4c5c93d2a0b368bc4570d5ec02ab03b9c4334d44")][0]);
	ASSERT_EQ(1, map[CGitHash(L"5e702e1712aa6f8cd8e0328a87be006f3a923710")].size());
	EXPECT_STREQ(_T("refs/notes/commits"), map[CGitHash(L"5e702e1712aa6f8cd8e0328a87be006f3a923710")][0]);
	ASSERT_EQ(1, map[CGitHash(L"18da7c332dcad0f37f9977d9176dce0b0c66f3eb")].size());
	EXPECT_STREQ(_T("refs/stash"), map[CGitHash(L"18da7c332dcad0f37f9977d9176dce0b0c66f3eb")][0]);
	ASSERT_EQ(1, map[CGitHash(L"c5b89de0335fd674e2e421ac4543098cb2f22cde")].size());
	EXPECT_STREQ(_T("refs/heads/simple-conflict"), map[CGitHash(L"c5b89de0335fd674e2e421ac4543098cb2f22cde")][0]);
	ASSERT_EQ(1, map[CGitHash(L"10385764a4d42d7428bbeb245015f8f338fc1e40")].size());
	EXPECT_STREQ(_T("refs/heads/forconflict"), map[CGitHash(L"10385764a4d42d7428bbeb245015f8f338fc1e40")][0]);
	ASSERT_EQ(2, map[CGitHash(L"49ecdfff36bfe2b9b499b33e5034f427e2fa54dd")].size());
	EXPECT_STREQ(_T("refs/heads/master2"), map[CGitHash(L"49ecdfff36bfe2b9b499b33e5034f427e2fa54dd")][0]);
	EXPECT_STREQ(_T("refs/tags/also-signed^{}"), map[CGitHash(L"49ecdfff36bfe2b9b499b33e5034f427e2fa54dd")][1]);
	ASSERT_EQ(1, map[CGitHash(L"b9ef30183497cdad5c30b88d32dc1bed7951dfeb")].size());//
	EXPECT_STREQ(_T("refs/tags/normal-tag"), map[CGitHash(L"b9ef30183497cdad5c30b88d32dc1bed7951dfeb")][0]);
	ASSERT_EQ(1, map[CGitHash(L"a9d53b535cb49640a6099860ac4999f5a0857b91")].size());
	EXPECT_STREQ(_T("refs/remotes/origin/master"), map[CGitHash(L"a9d53b535cb49640a6099860ac4999f5a0857b91")][0]);
	ASSERT_EQ(1, map[CGitHash(L"313a41bc88a527289c87d7531802ab484715974f")].size());
	EXPECT_STREQ(_T("refs/tags/all-files-signed^{}"), map[CGitHash(L"313a41bc88a527289c87d7531802ab484715974f")][0]);

	STRING_VECTOR remotes;
	EXPECT_EQ(0, m_Git.GetRemoteList(remotes));
	ASSERT_EQ(1, remotes.size());
	EXPECT_STREQ(_T("origin"), remotes[0]);

	EXPECT_EQ(-1, m_Git.DeleteRef(_T("refs/tags/gibbednet")));
	branches.clear();
	EXPECT_EQ(0, m_Git.GetBranchList(branches, nullptr, CGit::BRANCH_ALL));
	EXPECT_EQ(6, branches.size());
	tags.clear();
	EXPECT_EQ(0, m_Git.GetTagList(tags));
	EXPECT_EQ(3, tags.size());
	refs.clear();
	EXPECT_EQ(0, m_Git.GetRefList(refs));
	EXPECT_EQ(11, refs.size());

	EXPECT_EQ(-1, m_Git.DeleteRef(_T("refs/heads/gibbednet")));
	branches.clear();
	EXPECT_EQ(0, m_Git.GetBranchList(branches, nullptr, CGit::BRANCH_ALL));
	EXPECT_EQ(6, branches.size());
	tags.clear();
	EXPECT_EQ(0, m_Git.GetTagList(tags));
	EXPECT_EQ(3, tags.size());
	refs.clear();
	EXPECT_EQ(0, m_Git.GetRefList(refs));
	EXPECT_EQ(11, refs.size());

	EXPECT_EQ(-1, m_Git.DeleteRef(_T("refs/remotes/origin/gibbednet")));
	branches.clear();
	EXPECT_EQ(0, m_Git.GetBranchList(branches, nullptr, CGit::BRANCH_ALL));
	EXPECT_EQ(6, branches.size());
	tags.clear();
	EXPECT_EQ(0, m_Git.GetTagList(tags));
	EXPECT_EQ(3, tags.size());
	refs.clear();
	EXPECT_EQ(0, m_Git.GetRefList(refs));
	EXPECT_EQ(11, refs.size());

	EXPECT_EQ(0, m_Git.DeleteRef(_T("refs/tags/normal-tag")));
	branches.clear();
	EXPECT_EQ(0, m_Git.GetBranchList(branches, nullptr, CGit::BRANCH_ALL));
	EXPECT_EQ(6, branches.size());
	tags.clear();
	EXPECT_EQ(0, m_Git.GetTagList(tags));
	EXPECT_EQ(2, tags.size());
	refs.clear();
	EXPECT_EQ(0, m_Git.GetRefList(refs));
	EXPECT_EQ(10, refs.size());

	EXPECT_EQ(0, m_Git.DeleteRef(_T("refs/tags/all-files-signed^{}")));
	branches.clear();
	EXPECT_EQ(0, m_Git.GetBranchList(branches, nullptr, CGit::BRANCH_ALL));
	EXPECT_EQ(6, branches.size());
	tags.clear();
	EXPECT_EQ(0, m_Git.GetTagList(tags));
	EXPECT_EQ(1, tags.size());
	refs.clear();
	EXPECT_EQ(0, m_Git.GetRefList(refs));
	EXPECT_EQ(9, refs.size());

	EXPECT_EQ(0, m_Git.DeleteRef(_T("refs/heads/subdir/branch")));
	branches.clear();
	EXPECT_EQ(0, m_Git.GetBranchList(branches, nullptr, CGit::BRANCH_ALL));
	EXPECT_EQ(5, branches.size());
	tags.clear();
	EXPECT_EQ(0, m_Git.GetTagList(tags));
	EXPECT_EQ(1, tags.size());
	refs.clear();
	EXPECT_EQ(0, m_Git.GetRefList(refs));
	EXPECT_EQ(8, refs.size());

	EXPECT_EQ(0, m_Git.DeleteRef(_T("refs/remotes/origin/master")));
	branches.clear();
	EXPECT_EQ(0, m_Git.GetBranchList(branches, nullptr, CGit::BRANCH_ALL));
	EXPECT_EQ(4, branches.size());
	tags.clear();
	EXPECT_EQ(0, m_Git.GetTagList(tags));
	EXPECT_EQ(1, tags.size());
	refs.clear();
	EXPECT_EQ(0, m_Git.GetRefList(refs));
	EXPECT_EQ(7, refs.size());
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
	EXPECT_EQ(0, m_Git.Run(_T("git.exe checkout --orphan orphanic"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());

	STRING_VECTOR branches;
	int current = -2;
	EXPECT_EQ(0, m_Git.GetBranchList(branches, &current));
	ASSERT_EQ(5, branches.size());
	EXPECT_EQ(-2, current);
	EXPECT_STREQ(_T("forconflict"), branches[0]);
	EXPECT_STREQ(_T("master"), branches[1]);
	EXPECT_STREQ(_T("master2"), branches[2]);
	EXPECT_STREQ(_T("simple-conflict"), branches[3]);
	EXPECT_STREQ(_T("subdir/branch"), branches[4]);
}

TEST_P(CBasicGitWithTestRepoFixture, GetBranchList_detachedhead)
{
	CString output;
	EXPECT_EQ(0, m_Git.Run(_T("git.exe checkout a9d53b535cb49640a6099860ac4999f5a0857b91"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());

	STRING_VECTOR branches;
	int current = -2;
	EXPECT_EQ(0, m_Git.GetBranchList(branches, &current));
	ASSERT_EQ(5, branches.size());
	EXPECT_EQ(-2, current);
	EXPECT_STREQ(_T("forconflict"), branches[0]);
	EXPECT_STREQ(_T("master"), branches[1]);
	EXPECT_STREQ(_T("master2"), branches[2]);
	EXPECT_STREQ(_T("simple-conflict"), branches[3]);
	EXPECT_STREQ(_T("subdir/branch"), branches[4]);

	// cygwin fails here
	if (CGit::ms_bCygwinGit)
		return;

	output.Empty();
	EXPECT_EQ(0, m_Git.Run(_T("git.exe checkout -b (HEAD a9d53b535cb49640a6099860ac4999f5a0857b91"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());

	branches.clear();
	current = -2;
	EXPECT_EQ(0, m_Git.GetBranchList(branches, &current));
	ASSERT_EQ(6, branches.size());
	EXPECT_EQ(0, current);
	EXPECT_STREQ(_T("(HEAD"), branches[0]);
	EXPECT_STREQ(_T("forconflict"), branches[1]);
	EXPECT_STREQ(_T("master"), branches[2]);
	EXPECT_STREQ(_T("master2"), branches[3]);
	EXPECT_STREQ(_T("simple-conflict"), branches[4]);
	EXPECT_STREQ(_T("subdir/branch"), branches[5]);
}

TEST_P(CBasicGitWithEmptyBareRepositoryFixture, GetEmptyBranchesTagsRefs)
{
	EXPECT_STREQ(_T("master"), m_Git.GetCurrentBranch());

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
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)testFile, L"this is testing file."));
	EXPECT_EQ(0, m_Git.Run(_T("git.exe add test.txt"), &output, CP_UTF8));
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(_T("git.exe commit -m \"Add test.txt\""), &output, CP_UTF8));
	// repo with 1 versioned file
	EXPECT_FALSE(output.IsEmpty());
	EXPECT_TRUE(m_Git.CheckCleanWorkTree());
	EXPECT_TRUE(m_Git.CheckCleanWorkTree(true));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)testFile, L"Overwriting this testing file."));
	// repo with 1 modified versioned file
	EXPECT_FALSE(m_Git.CheckCleanWorkTree());
	EXPECT_FALSE(m_Git.CheckCleanWorkTree(true));

	output.Empty();
	EXPECT_EQ(0, m_Git.Run(_T("git.exe add test.txt"), &output, CP_UTF8));
	// repo with 1 modified versioned and staged file
	EXPECT_TRUE(output.IsEmpty());
	EXPECT_FALSE(m_Git.CheckCleanWorkTree());
	EXPECT_TRUE(m_Git.CheckCleanWorkTree(true));

	EXPECT_EQ(0, m_Git.Run(_T("git.exe commit -m \"Modified test.txt\""), &output, CP_UTF8));
	testFile = m_Dir.GetTempDir() + L"\\test2.txt";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)testFile, L"this is ANOTHER testing file."));
	EXPECT_TRUE(m_Git.CheckCleanWorkTree());
	EXPECT_TRUE(m_Git.CheckCleanWorkTree(true));

	EXPECT_EQ(0, m_Git.Run(_T("git.exe checkout --orphan orphanic"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());
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
	env.SetEnv(_T("not-found"), nullptr);
	EXPECT_FALSE(static_cast<wchar_t*>(env));
	EXPECT_STREQ(_T(""), env.GetEnv(L"test"));
	env.SetEnv(L"key1", L"value1");
	EXPECT_STREQ(_T("value1"), env.GetEnv(L"key1"));
	EXPECT_STREQ(_T("value1"), env.GetEnv(L"kEy1")); // check case insensitivity
	EXPECT_TRUE(*basePtr);
	EXPECT_EQ(static_cast<wchar_t*>(env), *basePtr);
	EXPECT_FALSE(env.empty());
	EXPECT_STREQ(_T("value1"), env.GetEnv(L"key1"));
	env.SetEnv(L"key1", nullptr); // delete first
	EXPECT_FALSE(*basePtr);
	EXPECT_EQ(static_cast<wchar_t*>(env), *basePtr);
	EXPECT_TRUE(env.empty());
	env.SetEnv(L"key1", L"value1");
	EXPECT_STREQ(_T("value1"), env.GetEnv(L"key1"));
	EXPECT_TRUE(*basePtr);
	EXPECT_EQ(static_cast<wchar_t*>(env), *basePtr);
	EXPECT_FALSE(env.empty());
	env.SetEnv(L"key2", L"value2");
	EXPECT_STREQ(_T("value1"), env.GetEnv(L"key1"));
	EXPECT_STREQ(_T("value2"), env.GetEnv(L"key2"));
	EXPECT_EQ(static_cast<wchar_t*>(env), *basePtr);
	env.SetEnv(_T("not-found"), nullptr);
	EXPECT_STREQ(_T("value1"), env.GetEnv(L"key1"));
	EXPECT_STREQ(_T("value2"), env.GetEnv(L"key2"));
	env.SetEnv(_T("key2"), nullptr); // delete last
	EXPECT_STREQ(_T("value1"), env.GetEnv(L"key1"));
	EXPECT_STREQ(_T(""), env.GetEnv(L"key2"));
	env.SetEnv(L"key3", L"value3");
	EXPECT_STREQ(_T("value1"), env.GetEnv(L"key1"));
	EXPECT_STREQ(_T("value3"), env.GetEnv(L"key3"));
	env.SetEnv(L"key4", L"value4");
	env.SetEnv(_T("value3"), nullptr); // delete middle
	EXPECT_STREQ(_T("value1"), env.GetEnv(L"key1"));
	EXPECT_STREQ(_T("value4"), env.GetEnv(L"key4"));
	env.SetEnv(L"key5", L"value5");
	EXPECT_STREQ(_T("value1"), env.GetEnv(L"key1"));
	EXPECT_STREQ(_T("value4"), env.GetEnv(L"key4"));
	EXPECT_STREQ(_T("value5"), env.GetEnv(L"key5"));
	env.SetEnv(L"key4", L"value4a");
	EXPECT_STREQ(_T("value1"), env.GetEnv(L"key1"));
	EXPECT_STREQ(_T("value4a"), env.GetEnv(L"key4"));
	EXPECT_STREQ(_T("value5"), env.GetEnv(L"key5"));
	env.SetEnv(L"key5", L"value5a");
	EXPECT_STREQ(_T("value1"), env.GetEnv(L"key1"));
	EXPECT_STREQ(_T("value4a"), env.GetEnv(L"key4"));
	EXPECT_STREQ(_T("value5a"), env.GetEnv(L"key5"));
#pragma warning(push)
#pragma warning(disable: 4996)
	CString windir = _wgetenv(L"windir");
#pragma warning(pop)
	env.CopyProcessEnvironment();
	EXPECT_STREQ(windir, env.GetEnv(L"windir"));
	EXPECT_STREQ(_T("value1"), env.GetEnv(L"key1"));
	EXPECT_STREQ(_T("value4a"), env.GetEnv(L"key4"));
	EXPECT_STREQ(_T("value5a"), env.GetEnv(L"key5"));
	env.clear();
	EXPECT_FALSE(*basePtr);
	EXPECT_TRUE(env.empty());
	EXPECT_STREQ(_T(""), env.GetEnv(L"key4"));
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
	EXPECT_STREQ(_T("ä#äf34ööcöäß€9875oe\r\nfgdjkglsfdg\r\nöäöü45g\r\nfdgi&§$%&hfdsgä\r\nä#äf34öööäß€9875oe\r\nöäcüpfgmfdg\r\n€fgfdsg\r\n45\r\näü"), fileContents);
	::DeleteFile(tmpFile);
}

TEST_P(CBasicGitWithTestRepoFixture, GetOneFile)
{
	GetOneFile(m_Git);

	// clean&smudge filters are not available for GetOneFile without libigt2
	if (GetParam() == GIT_CLI || GetParam() == LIBGIT)
		return;

	CString cleanFilterFilename = m_Git.m_CurrentDir + L"\\clean_filter_openssl";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)cleanFilterFilename, L"#!/bin/bash\nopenssl enc -base64 -aes-256-ecb -S FEEDDEADBEEF -k PASS_FIXED"));
	CString smudgeFilterFilename = m_Git.m_CurrentDir + L"\\smudge_filter_openssl";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)smudgeFilterFilename, L"#!/bin/bash\nopenssl enc -d -base64 -aes-256-ecb -k PASS_FIXED"));

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
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)attributesFile, L"*.enc filter=openssl\n"));

	CString encryptedFileOne = m_Git.m_CurrentDir + L"\\1.enc";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)encryptedFileOne, L"This should be encrypted...\nAnd decrypted on the fly\n"));

	CString encryptedFileTwo = m_Git.m_CurrentDir + L"\\2.enc";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)encryptedFileTwo, L"This should also be encrypted...\nAnd also decrypted on the fly\n"));

	CString output;
	EXPECT_EQ(0, m_Git.Run(_T("git.exe add 1.enc"), &output, CP_UTF8));
	EXPECT_TRUE(output.IsEmpty());

	CAutoIndex index;
	ASSERT_EQ(0, git_repository_index(index.GetPointer(), repo));
	EXPECT_EQ(0, git_index_add_bypath(index, "2.enc"));
	EXPECT_EQ(0, git_index_write(index));

	EXPECT_EQ(0, m_Git.Run(_T("git.exe commit -m \"Message\""), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());

	CString fileContents;
	CString tmpFile = GetTempFile();
	EXPECT_EQ(0, m_Git.GetOneFile(L"HEAD", CTGitPath(L"1.enc"), tmpFile));
	EXPECT_EQ(true, CStringUtils::ReadStringFromTextFile(tmpFile, fileContents));
	EXPECT_STREQ(_T("This should be encrypted...\nAnd decrypted on the fly\n"), fileContents);
	::DeleteFile(tmpFile);

	fileContents.Empty();
	tmpFile = GetTempFile();
	EXPECT_EQ(0, m_Git.GetOneFile(L"HEAD", CTGitPath(L"2.enc"), tmpFile));
	EXPECT_EQ(true, CStringUtils::ReadStringFromTextFile(tmpFile, fileContents));
	EXPECT_STREQ(_T("This should also be encrypted...\nAnd also decrypted on the fly\n"), fileContents);
	::DeleteFile(tmpFile);

	EXPECT_TRUE(::DeleteFile(attributesFile));

	fileContents.Empty();
	tmpFile = GetTempFile();
	EXPECT_EQ(0, m_Git.GetOneFile(L"HEAD", CTGitPath(L"1.enc"), tmpFile));
	EXPECT_EQ(true, CStringUtils::ReadStringFromTextFile(tmpFile, fileContents));
	EXPECT_STREQ(_T("U2FsdGVkX1/+7d6tvu8AABwbE+Xy7U4l5boTKjIgUkYHONqmYHD+0e6k35MgtUGx\ns11nq1QuKeFCW5wFWNSj1WcHg2n4W59xfnB7RkSSIDQ=\n"), fileContents);
	::DeleteFile(tmpFile);

	fileContents.Empty();
	tmpFile = GetTempFile();
	EXPECT_EQ(0, m_Git.GetOneFile(L"HEAD", CTGitPath(L"2.enc"), tmpFile));
	EXPECT_EQ(true, CStringUtils::ReadStringFromTextFile(tmpFile, fileContents));
	EXPECT_STREQ(_T("U2FsdGVkX1/+7d6tvu8AAIDDx8qi/l0qzkSMsS2YLt8tYK1oWzj8+o78fXH0/tlO\nCRVrKqTvh9eUFklY8QFYfZfj01zBkFat+4zrW+1rV4Q=\n"), fileContents);
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
	EXPECT_EQ(0, descriptions.size());

	g_Git.SetConfigValue(_T("branch.master.description"), _T("test"));
	g_Git.SetConfigValue(_T("branch.subdir/branch.description"), _T("multi\nline"));

	EXPECT_EQ(0, m_Git.GetBranchDescriptions(descriptions));
	ASSERT_EQ(2, descriptions.size());
	EXPECT_STREQ(_T("test"), descriptions[L"master"]);
	EXPECT_STREQ(_T("multi\nline"), descriptions[L"subdir/branch"]);
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
	EXPECT_STREQ(_T(""), m_Git.GetConfigValue(_T("not-found")));
	EXPECT_STREQ(_T("default"), m_Git.GetConfigValue(_T("not-found"), _T("default")));

	EXPECT_STREQ(_T("false"), m_Git.GetConfigValue(_T("core.bare")));
	EXPECT_STREQ(_T("false"), m_Git.GetConfigValue(_T("core.bare"), _T("default-value"))); // value exist, so default does not match
	EXPECT_STREQ(_T("true"), m_Git.GetConfigValue(_T("core.ignorecase")));
	EXPECT_STREQ(_T("0"), m_Git.GetConfigValue(_T("core.repositoryformatversion")));
	EXPECT_STREQ(_T("https://example.com/git/testing"), m_Git.GetConfigValue(_T("remote.origin.url")));

	EXPECT_EQ(false, m_Git.GetConfigValueBool(_T("not-found")));
	EXPECT_EQ(true, m_Git.GetConfigValueBool(_T("not-found"), true));
	EXPECT_EQ(false, m_Git.GetConfigValueBool(_T("core.bare")));
	EXPECT_EQ(false, m_Git.GetConfigValueBool(_T("core.bare"), true)); // value exist, so default does not match
	EXPECT_EQ(false, m_Git.GetConfigValueBool(_T("core.repositoryformatversion")));
	EXPECT_EQ(false, m_Git.GetConfigValueBool(_T("remote.origin.url")));
	EXPECT_EQ(true, m_Git.GetConfigValueBool(_T("core.ignorecase")));

	CString values[] = { _T(""), _T(" "), _T("ending-with-space "), _T(" starting with-space"), _T("test1"), _T("some\\backslashes\\in\\it"), _T("with \" doublequote"), _T("with backslash before \\\" doublequote"), _T("with'quote"), _T("multi\nline"), _T("no-multi\\nline"), _T("new line at end\n") };
	for (int i = 0; i < _countof(values); ++i)
	{
		CString key;
		key.Format(_T("re-read.test%d"), i);
		EXPECT_EQ(0, m_Git.SetConfigValue(key, values[i]));
		EXPECT_STREQ(values[i], m_Git.GetConfigValue(key));
	}

	m_Git.SetConfigValue(_T("booltest.true1"), _T("1"));
	m_Git.SetConfigValue(_T("booltest.true2"), _T("100"));
	m_Git.SetConfigValue(_T("booltest.true3"), _T("-2"));
	m_Git.SetConfigValue(_T("booltest.true4"), _T("yes"));
	m_Git.SetConfigValue(_T("booltest.true5"), _T("yEs"));
	m_Git.SetConfigValue(_T("booltest.true6"), _T("true"));
	m_Git.SetConfigValue(_T("booltest.true7"), _T("on"));
	for (int i = 1; i <= 7; ++i)
	{
		CString key;
		key.Format(_T("booltest.true%d"), i);
		EXPECT_EQ(true, m_Git.GetConfigValueBool(key));
	}
	m_Git.SetConfigValue(_T("booltest.false1"), _T("0"));
	EXPECT_EQ(false, m_Git.GetConfigValueBool(_T("booltest.false1")));
	m_Git.SetConfigValue(_T("booltest.false2"), _T(""));
	EXPECT_EQ(false, m_Git.GetConfigValueBool(_T("booltest.false2")));

	EXPECT_EQ(0, m_Git.GetConfigValueInt32(_T("does-not-exist")));
	EXPECT_EQ(15, m_Git.GetConfigValueInt32(_T("does-not-exist"), 15));
	EXPECT_EQ(0, m_Git.GetConfigValueInt32(_T("core.repositoryformatversion")));
	EXPECT_EQ(0, m_Git.GetConfigValueInt32(_T("core.repositoryformatversion"), 42)); // value exist, so default should not be returned
	EXPECT_EQ(1, m_Git.GetConfigValueInt32(_T("booltest.true1")));
	EXPECT_EQ(100, m_Git.GetConfigValueInt32(_T("booltest.true2")));
	EXPECT_EQ(-2, m_Git.GetConfigValueInt32(_T("booltest.true3")));
	EXPECT_EQ(0, m_Git.GetConfigValueInt32(_T("booltest.true4")));
	EXPECT_EQ(42, m_Git.GetConfigValueInt32(_T("booltest.true4"), 42));
	EXPECT_EQ(0, m_Git.GetConfigValueInt32(_T("booltest.true8")));
	EXPECT_EQ(42, m_Git.GetConfigValueInt32(_T("booltest.true8"), 42));

	EXPECT_NE(0, m_Git.UnsetConfigValue(_T("does-not-exist")));
	EXPECT_STREQ(_T("false"), m_Git.GetConfigValue(_T("core.bare")));
	EXPECT_STREQ(_T("true"), m_Git.GetConfigValue(_T("core.ignorecase")));
	EXPECT_EQ(0, m_Git.UnsetConfigValue(_T("core.bare")));
	EXPECT_STREQ(_T("default"), m_Git.GetConfigValue(_T("core.bare"), _T("default")));
	EXPECT_STREQ(_T("true"), m_Git.GetConfigValue(_T("core.ignorecase")));

	CString gitConfig = m_Git.m_CurrentDir + L"\\.git\\config";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)gitConfig, L"[booltest]\nistrue"));
	EXPECT_EQ(true, m_Git.GetConfigValueBool(L"booltest.istrue"));

	// test includes from %HOME% specified as ~/
	EXPECT_STREQ(L"not-found", g_Git.GetConfigValue(L"test.fromincluded", L"not-found"));
	EXPECT_EQ(0, m_Git.SetConfigValue(L"include.path", L"~/a-path-that-should-not-exist.gconfig"));
	EXPECT_STREQ(L"~/a-path-that-should-not-exist.gconfig", g_Git.GetConfigValue(L"include.path", L"not-found"));
	CString testFile = g_Git.GetHomeDirectory() + _T("\\a-path-that-should-not-exist.gconfig");
	ASSERT_FALSE(PathFileExists(testFile)); // make sure we don't override a file by mistake ;)
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)testFile, L"[test]\nfromincluded=yeah-this-is-included\n"));
	EXPECT_STREQ(L"yeah-this-is-included", g_Git.GetConfigValue(L"test.fromincluded", L"not-found"));
	EXPECT_TRUE(::DeleteFile(testFile));
}

TEST_P(CBasicGitWithTestRepoFixture, GetWorkingTreeChanges)
{
	if (GetParam() != 0)
		return;

	// adding ansi2.txt (as a copy of ansi.txt) produces a warning
	m_Git.SetConfigValue(_T("core.autocrlf"), _T("false"));

	CString output;
	EXPECT_EQ(0, m_Git.Run(_T("git.exe reset --hard master"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());

	CTGitPathList filter(CTGitPath(_T("copy")));

	// no changes
	CTGitPathList list;
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	EXPECT_TRUE(list.IsEmpty());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, nullptr));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(_T("ascii.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter));
	EXPECT_TRUE(list.IsEmpty());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(_T("ascii.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);

	// untracked file
	CString testFile = m_Git.m_CurrentDir + L"\\untracked-file.txt";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)testFile, L"*.enc filter=openssl\n"));
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	EXPECT_TRUE(list.IsEmpty());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, nullptr));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(_T("ascii.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter));
	EXPECT_TRUE(list.IsEmpty());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(_T("ascii.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);

	// untracked file in sub-directory
	testFile = m_Git.m_CurrentDir + L"\\copy\\untracked-file.txt";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)testFile, L"*.enc filter=openssl\n"));
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	EXPECT_TRUE(list.IsEmpty());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, nullptr));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(_T("ascii.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter));
	EXPECT_TRUE(list.IsEmpty());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(_T("ascii.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);

	// modified file in sub-directory
	testFile = m_Git.m_CurrentDir + L"\\copy\\utf8-nobom.txt";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)testFile, L"*.enc filter=openssl\n"));
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(_T("copy/utf8-nobom.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, nullptr));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(_T("ascii.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_STREQ(_T("copy/utf8-nobom.txt"), list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(_T("copy/utf8-nobom.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(_T("ascii.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_STREQ(_T("copy/utf8-nobom.txt"), list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);

	// two modified files, one in root and one in sub-directory
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(_T("git.exe reset --hard master"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());
	testFile = m_Git.m_CurrentDir + L"\\utf8-bom.txt";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)testFile, L"*.enc filter=openssl\n"));
	testFile = m_Git.m_CurrentDir + L"\\copy\\utf8-nobom.txt";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)testFile, L"*.enc filter=openssl\n"));
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(_T("copy/utf8-nobom.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_STREQ(_T("utf8-bom.txt"), list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, nullptr));
	ASSERT_EQ(3, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(_T("ascii.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_STREQ(_T("copy/utf8-nobom.txt"), list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);
	EXPECT_STREQ(_T("utf8-bom.txt"), list[2].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[2].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(_T("copy/utf8-nobom.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(_T("ascii.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_STREQ(_T("copy/utf8-nobom.txt"), list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);

	// Staged modified file
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(_T("git.exe reset --hard master"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());
	testFile = m_Git.m_CurrentDir + L"\\utf8-nobom.txt";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)testFile, L"*.enc filter=openssl\n"));
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(_T("git.exe add utf8-nobom.txt"), &output, CP_UTF8));
	EXPECT_TRUE(output.IsEmpty());
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(_T("utf8-nobom.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, nullptr));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(_T("ascii.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_STREQ(_T("utf8-nobom.txt"), list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(_T("utf8-nobom.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(_T("ascii.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_STREQ(_T("utf8-nobom.txt"), list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);

	// Staged modified file in subfolder
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(_T("git.exe reset --hard master"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());
	testFile = m_Git.m_CurrentDir + L"\\copy\\utf8-nobom.txt";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)testFile, L"*.enc filter=openssl\n"));
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(_T("git.exe add copy/utf8-nobom.txt"), &output, CP_UTF8));
	EXPECT_TRUE(output.IsEmpty());
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(_T("copy/utf8-nobom.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, nullptr));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(_T("ascii.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_STREQ(_T("copy/utf8-nobom.txt"), list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(_T("copy/utf8-nobom.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(_T("ascii.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_STREQ(_T("copy/utf8-nobom.txt"), list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);

	// Modified file modified after staging
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(_T("git.exe reset --hard master"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());
	testFile = m_Git.m_CurrentDir + L"\\copy\\utf8-nobom.txt";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)testFile, L"*.enc filter=openssl\n"));
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(_T("git.exe add copy/utf8-nobom.txt"), &output, CP_UTF8));
	EXPECT_TRUE(output.IsEmpty());
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)testFile, L"now with different content after staging"));
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(_T("copy/utf8-nobom.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, nullptr));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(_T("ascii.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_STREQ(_T("copy/utf8-nobom.txt"), list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(_T("copy/utf8-nobom.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(_T("ascii.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_STREQ(_T("copy/utf8-nobom.txt"), list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);

	// Missing file
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(_T("git.exe reset --hard master"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());
	EXPECT_TRUE(::DeleteFile(m_Dir.GetTempDir()+_T("\\copy\\ansi.txt")));
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list.GetAction());
	EXPECT_STREQ(_T("copy/ansi.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED | CTGitPath::LOGACTIONS_MISSING, list[0].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, nullptr));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED | CTGitPath::LOGACTIONS_DELETED, list.GetAction());
	EXPECT_STREQ(_T("ascii.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_STREQ(_T("copy/ansi.txt"), list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED | CTGitPath::LOGACTIONS_MISSING, list[1].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list.GetAction());
	EXPECT_STREQ(_T("copy/ansi.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED | CTGitPath::LOGACTIONS_MISSING, list[0].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED | CTGitPath::LOGACTIONS_DELETED, list.GetAction());
	EXPECT_STREQ(_T("ascii.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_STREQ(_T("copy/ansi.txt"), list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED | CTGitPath::LOGACTIONS_MISSING, list[1].m_Action);

	// deleted file, also deleted in index
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(_T("git.exe reset --hard master"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(_T("git.exe rm copy/ansi.txt"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list.GetAction());
	EXPECT_STREQ(_T("copy/ansi.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list[0].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, nullptr));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED | CTGitPath::LOGACTIONS_DELETED, list.GetAction());
	EXPECT_STREQ(_T("ascii.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_STREQ(_T("copy/ansi.txt"), list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list[1].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list.GetAction());
	EXPECT_STREQ(_T("copy/ansi.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list[0].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED | CTGitPath::LOGACTIONS_DELETED, list.GetAction());
	EXPECT_STREQ(_T("ascii.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_STREQ(_T("copy/ansi.txt"), list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list[1].m_Action);

	// file deleted in index, but still on disk
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(_T("git.exe reset --hard master"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(_T("git.exe rm --cached copy/ansi.txt"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list.GetAction());
	EXPECT_STREQ(_T("copy/ansi.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list[0].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, nullptr));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED | CTGitPath::LOGACTIONS_DELETED, list.GetAction());
	EXPECT_STREQ(_T("ascii.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_STREQ(_T("copy/ansi.txt"), list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list[1].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list.GetAction());
	EXPECT_STREQ(_T("copy/ansi.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list[0].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED | CTGitPath::LOGACTIONS_DELETED, list.GetAction());
	EXPECT_STREQ(_T("ascii.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_STREQ(_T("copy/ansi.txt"), list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list[1].m_Action);

	// file deleted in index, but still on disk, but modified
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(_T("git.exe reset --hard master"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(_T("git.exe rm --cached copy/ansi.txt"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());
	testFile = m_Git.m_CurrentDir + L"\\copy\\ansi.txt";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)testFile, L"*.enc filter=openssl\n"));
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list.GetAction());
	EXPECT_STREQ(_T("copy/ansi.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list[0].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, nullptr));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED | CTGitPath::LOGACTIONS_DELETED, list.GetAction());
	EXPECT_STREQ(_T("ascii.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_STREQ(_T("copy/ansi.txt"), list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list[1].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list.GetAction());
	EXPECT_STREQ(_T("copy/ansi.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list[0].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED | CTGitPath::LOGACTIONS_DELETED, list.GetAction());
	EXPECT_STREQ(_T("ascii.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_STREQ(_T("copy/ansi.txt"), list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, list[1].m_Action);

	// renamed file in same folder
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(_T("git.exe reset --hard master"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(_T("git.exe mv ansi.txt ansi2.txt"), &output, CP_UTF8));
	EXPECT_TRUE(output.IsEmpty());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED, list.GetAction());
	EXPECT_STREQ(_T("ansi2.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED, list[0].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, nullptr));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED | CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(_T("ansi2.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED, list[0].m_Action);
	EXPECT_STREQ(_T("ascii.txt"), list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED, list.GetAction());
	EXPECT_STREQ(_T("ansi2.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED, list[0].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED | CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(_T("ansi2.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED, list[0].m_Action);
	EXPECT_STREQ(_T("ascii.txt"), list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);

	// added and staged new file
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(_T("git.exe reset --hard master"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());
	testFile = m_Git.m_CurrentDir + L"\\copy\\test-file.txt";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)testFile, L"*.enc filter=openssl\n"));
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(_T("git.exe add copy/test-file.txt"), &output, CP_UTF8));
	EXPECT_TRUE(output.IsEmpty());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list.GetAction());
	EXPECT_STREQ(_T("copy/test-file.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list[0].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, nullptr));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED | CTGitPath::LOGACTIONS_ADDED, list.GetAction());
	EXPECT_STREQ(_T("ascii.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_STREQ(_T("copy/test-file.txt"), list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list[1].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list.GetAction());
	EXPECT_STREQ(_T("copy/test-file.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list[0].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED | CTGitPath::LOGACTIONS_ADDED, list.GetAction());
	EXPECT_STREQ(_T("ascii.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_STREQ(_T("copy/test-file.txt"), list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list[1].m_Action);

	// file copied and staged
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(_T("git.exe reset --hard master"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());
	testFile = m_Git.m_CurrentDir + L"\\ansi.txt";
	EXPECT_TRUE(CopyFile(m_Git.m_CurrentDir + L"\\ansi.txt", m_Git.m_CurrentDir + L"\\ansi2.txt", TRUE));
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(_T("git.exe add ansi2.txt"), &output, CP_UTF8));
	EXPECT_TRUE(output.IsEmpty());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list.GetAction());
	EXPECT_STREQ(_T("ansi2.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list[0].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, nullptr));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED | CTGitPath::LOGACTIONS_ADDED, list.GetAction());
	EXPECT_STREQ(_T("ansi2.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list[0].m_Action);
	EXPECT_STREQ(_T("ascii.txt"), list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list.GetAction());
	EXPECT_STREQ(_T("ansi2.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list[0].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED | CTGitPath::LOGACTIONS_ADDED, list.GetAction());
	EXPECT_STREQ(_T("ansi2.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list[0].m_Action);
	EXPECT_STREQ(_T("ascii.txt"), list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);

	// file renamed + moved to sub-folder
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(_T("git.exe reset --hard master"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(_T("git.exe mv ansi.txt copy/ansi2.txt"), &output, CP_UTF8));
	EXPECT_TRUE(output.IsEmpty());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED, list.GetAction());
	EXPECT_STREQ(_T("copy/ansi2.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED, list[0].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, nullptr));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED | CTGitPath::LOGACTIONS_REPLACED, list.GetAction());
	EXPECT_STREQ(_T("ascii.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_STREQ(_T("copy/ansi2.txt"), list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED, list[1].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED | CTGitPath::LOGACTIONS_ADDED, list.GetAction());
	EXPECT_STREQ(_T("copy/ansi2.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED | CTGitPath::LOGACTIONS_ADDED, list[0].m_Action); // TODO
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter));
	ASSERT_EQ(2, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED | CTGitPath::LOGACTIONS_REPLACED | CTGitPath::LOGACTIONS_ADDED, list.GetAction());
	EXPECT_STREQ(_T("ascii.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_STREQ(_T("copy/ansi2.txt"), list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED | CTGitPath::LOGACTIONS_ADDED, list[1].m_Action); // TODO

	// conflicting files
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(_T("git.exe reset --hard master"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(_T("git.exe merge forconflict"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());
	output.Empty();
	EXPECT_EQ(1, m_Git.Run(_T("git.exe merge simple-conflict"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED | CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(_T("ansi.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED | CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, nullptr));
	ASSERT_EQ(7, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED | CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(_T("ansi.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED | CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_STREQ(_T("utf16-be-bom.txt"), list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);
	EXPECT_STREQ(_T("utf16-be-nobom.txt"), list[2].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[2].m_Action);
	EXPECT_STREQ(_T("utf16-le-bom.txt"), list[3].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[3].m_Action);
	EXPECT_STREQ(_T("utf16-le-nobom.txt"), list[4].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[4].m_Action);
	EXPECT_STREQ(_T("utf8-bom.txt"), list[5].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[5].m_Action);
	EXPECT_STREQ(_T("utf8-nobom.txt"), list[6].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[6].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED, list.GetAction());
	EXPECT_STREQ(_T("ansi.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED, list[0].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, &filter));
	ASSERT_EQ(7, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED | CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(_T("ansi.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED, list[0].m_Action);
	EXPECT_STREQ(_T("utf16-be-bom.txt"), list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);
	EXPECT_STREQ(_T("utf16-be-nobom.txt"), list[2].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[2].m_Action);
	EXPECT_STREQ(_T("utf16-le-bom.txt"), list[3].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[3].m_Action);
	EXPECT_STREQ(_T("utf16-le-nobom.txt"), list[4].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[4].m_Action);
	EXPECT_STREQ(_T("utf8-bom.txt"), list[5].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[5].m_Action);
	EXPECT_STREQ(_T("utf8-nobom.txt"), list[6].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[6].m_Action);
}

TEST_P(CBasicGitWithTestRepoFixture, GetWorkingTreeChanges_DeleteModifyConflict_DeletedRemotely)
{
	if (GetParam() != 0)
		return;

	CString output;
	EXPECT_EQ(0, m_Git.Run(_T("git.exe reset --hard master"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());

	output.Empty();
	EXPECT_EQ(0, m_Git.Run(_T("git.exe checkout forconflict"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(_T("git.exe rm ansi.txt"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(_T("git.exe commit -m \"Prepare conflict case\""), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(_T("git.exe checkout simple-conflict"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());
	output.Empty();
	EXPECT_EQ(1, m_Git.Run(_T("git.exe merge forconflict"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());

	CTGitPathList filter(CTGitPath(_T("copy")));

	CTGitPathList list;
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	ASSERT_EQ(7, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED | CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(_T("ansi.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED | CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_STREQ(_T("utf16-be-bom.txt"), list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);
	EXPECT_STREQ(_T("utf16-be-nobom.txt"), list[2].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[2].m_Action);
	EXPECT_STREQ(_T("utf16-le-bom.txt"), list[3].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[3].m_Action);
	EXPECT_STREQ(_T("utf16-le-nobom.txt"), list[4].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[4].m_Action);
	EXPECT_STREQ(_T("utf8-bom.txt"), list[5].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[5].m_Action);
	EXPECT_STREQ(_T("utf8-nobom.txt"), list[6].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[6].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, nullptr));
	ASSERT_EQ(7, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED | CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(_T("ansi.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED | CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	EXPECT_STREQ(_T("utf16-be-bom.txt"), list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);
	EXPECT_STREQ(_T("utf16-be-nobom.txt"), list[2].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[2].m_Action);
	EXPECT_STREQ(_T("utf16-le-bom.txt"), list[3].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[3].m_Action);
	EXPECT_STREQ(_T("utf16-le-nobom.txt"), list[4].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[4].m_Action);
	EXPECT_STREQ(_T("utf8-bom.txt"), list[5].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[5].m_Action);
	EXPECT_STREQ(_T("utf8-nobom.txt"), list[6].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[6].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter));
	ASSERT_EQ(7, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED | CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(_T("ansi.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED, list[0].m_Action);
	EXPECT_STREQ(_T("utf16-be-bom.txt"), list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[1].m_Action);
	EXPECT_STREQ(_T("utf16-be-nobom.txt"), list[2].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[2].m_Action);
	EXPECT_STREQ(_T("utf16-le-bom.txt"), list[3].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[3].m_Action);
	EXPECT_STREQ(_T("utf16-le-nobom.txt"), list[4].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[4].m_Action);
	EXPECT_STREQ(_T("utf8-bom.txt"), list[5].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[5].m_Action);
	EXPECT_STREQ(_T("utf8-nobom.txt"), list[6].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, list[6].m_Action);
}

TEST_P(CBasicGitWithTestRepoFixture, GetWorkingTreeChanges_DeleteModifyConflict_DeletedLocally)
{
	if (GetParam() != 0)
		return;

	CString output;
	EXPECT_EQ(0, m_Git.Run(_T("git.exe reset --hard master"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());

	output.Empty();
	EXPECT_EQ(0, m_Git.Run(_T("git.exe checkout forconflict"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(_T("git.exe rm ansi.txt"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());
	output.Empty();
	EXPECT_EQ(0, m_Git.Run(_T("git.exe commit -m \"Prepare conflict case\""), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());
	output.Empty();
	EXPECT_EQ(1, m_Git.Run(_T("git.exe merge simple-conflict"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());

	CTGitPathList filter(CTGitPath(_T("copy")));

	CTGitPathList list;
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED | CTGitPath::LOGACTIONS_ADDED, list.GetAction());
	EXPECT_STREQ(_T("ansi.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED | CTGitPath::LOGACTIONS_ADDED, list[0].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, true, nullptr));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED | CTGitPath::LOGACTIONS_MODIFIED, list.GetAction());
	EXPECT_STREQ(_T("ansi.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED | CTGitPath::LOGACTIONS_MODIFIED, list[0].m_Action);
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter));
	ASSERT_EQ(1, list.GetCount());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED, list.GetAction());
	EXPECT_STREQ(_T("ansi.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_UNMERGED, list[0].m_Action);
}

TEST_P(CBasicGitWithEmptyRepositoryFixture, GetWorkingTreeChanges)
{
	if (GetParam() != 0)
		return;

	CTGitPathList list;
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	EXPECT_TRUE(list.IsEmpty());

	CString testFile = m_Dir.GetTempDir() + L"\\test.txt";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)testFile, L"this is testing file."));
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	EXPECT_TRUE(list.IsEmpty());
	CString output;
	EXPECT_EQ(0, m_Git.Run(_T("git.exe add test.txt"), &output, CP_UTF8));
	EXPECT_TRUE(output.IsEmpty());
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, nullptr));
	ASSERT_EQ(1, list.GetCount());
	// EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list.GetAction()); // we do not care here for the list action, as its only used in GitLogListBase and there we re-calculate it in AsyncDiffThread
	EXPECT_STREQ(_T("test.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list[0].m_Action);

	CTGitPathList filter(CTGitPath(_T("copy")));
	list.Clear();
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter));
	ASSERT_EQ(1, list.GetCount());
	// EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list.GetAction()); // we do not care here for the list action, as its only used in GitLogListBase and there we re-calculate it in AsyncDiffThread
	EXPECT_STREQ(_T("test.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list[0].m_Action);

	list.Clear();
	EXPECT_TRUE(::CreateDirectory(m_Dir.GetTempDir() + L"\\copy", nullptr));
	testFile = m_Dir.GetTempDir() + L"\\copy\\test2.txt";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)testFile, L"this is another testing file."));
	EXPECT_EQ(0, m_Git.Run(_T("git.exe add copy/test2.txt"), &output, CP_UTF8));
	EXPECT_TRUE(output.IsEmpty());
	EXPECT_EQ(0, m_Git.GetWorkingTreeChanges(list, false, &filter));
	ASSERT_EQ(2, list.GetCount());
	// EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list.GetAction()); // we do not care here for the list action, as its only used in GitLogListBase and there we re-calculate it in AsyncDiffThread
	EXPECT_STREQ(_T("copy/test2.txt"), list[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list[0].m_Action);
	EXPECT_STREQ(_T("test.txt"), list[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, list[1].m_Action);
}

TEST_P(CBasicGitWithTestRepoFixture, GetWorkingTreeChanges_RefreshGitIndex)
{
	if (GetParam() != 0)
		return;

	// adding ansi2.txt (as a copy of ansi.txt) produces a warning
	m_Git.SetConfigValue(_T("core.autocrlf"), _T("false"));

	CString output;
	EXPECT_EQ(0, m_Git.Run(_T("git.exe reset --hard master"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());

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

	EXPECT_EQ(0, m_Git.Run(_T("git.exe bisect start"), &output, CP_UTF8));
	m_Git.GetBisectTerms(&good, &bad);
	EXPECT_STREQ(_T("good"), good);
	EXPECT_STREQ(_T("bad"), bad);

	good.Empty();
	bad.Empty();
	m_Git.GetBisectTerms(&good, &bad);
	EXPECT_STREQ(_T("good"), good);
	EXPECT_STREQ(_T("bad"), bad);

	EXPECT_EQ(0, m_Git.Run(_T("git.exe bisect reset"), &output, CP_UTF8));

	if (m_Git.GetGitVersion(nullptr, nullptr) < 0x02070000)
		return;

	EXPECT_EQ(0, m_Git.Run(_T("git.exe bisect start --term-good=original --term-bad=changed"), &output, CP_UTF8));
	m_Git.GetBisectTerms(&good, &bad);
	EXPECT_STREQ(_T("original"), good);
	EXPECT_STREQ(_T("changed"), bad);

	EXPECT_EQ(0, m_Git.Run(_T("git.exe bisect reset"), &output, CP_UTF8));
}

TEST_P(CBasicGitWithTestRepoFixture, GetRefsCommitIsOn)
{
	STRING_VECTOR list;
	EXPECT_EQ(0, m_Git.GetRefsCommitIsOn(list, CGitHash(L"7c3cbfe13a929d2291a574dca45e4fd2d2ac1aa6"), false, false));
	EXPECT_TRUE(list.empty());

	list.clear();
	EXPECT_EQ(0, m_Git.GetRefsCommitIsOn(list, CGitHash(L"7c3cbfe13a929d2291a574dca45e4fd2d2ac1aa6"), true, true));
	ASSERT_EQ(1, list.size());
	EXPECT_STREQ(L"refs/heads/master", list[0]);
	
	list.clear();
	EXPECT_EQ(0, m_Git.GetRefsCommitIsOn(list, CGitHash(L"1fc3c9688e27596d8717b54f2939dc951568f6cb"), true, true));
	ASSERT_EQ(1, list.size());
	EXPECT_STREQ(L"refs/heads/master", list[0]);

	list.clear();
	EXPECT_EQ(0, m_Git.GetRefsCommitIsOn(list, CGitHash(L"1fc3c9688e27596d8717b54f2939dc951568f6cb"), false, true));
	ASSERT_EQ(1, list.size());
	EXPECT_STREQ(L"refs/heads/master", list[0]);

	list.clear();
	EXPECT_EQ(0, m_Git.GetRefsCommitIsOn(list, CGitHash(L"1fc3c9688e27596d8717b54f2939dc951568f6cb"), true, true, CGit::BRANCH_REMOTE));
	EXPECT_TRUE(list.empty());

	list.clear();
	EXPECT_EQ(0, m_Git.GetRefsCommitIsOn(list, CGitHash(L"1fc3c9688e27596d8717b54f2939dc951568f6cb"), false, true, CGit::BRANCH_ALL));
	ASSERT_EQ(1, list.size());
	EXPECT_STREQ(L"refs/heads/master", list[0]);

	list.clear();
	EXPECT_EQ(0, m_Git.GetRefsCommitIsOn(list, CGitHash(L"1fc3c9688e27596d8717b54f2939dc951568f6cb"), true, false));
	EXPECT_TRUE(list.empty());

	list.clear();
	EXPECT_EQ(0, m_Git.GetRefsCommitIsOn(list, CGitHash(L"b9ef30183497cdad5c30b88d32dc1bed7951dfeb"), true, false));
	ASSERT_EQ(2, list.size());
	EXPECT_STREQ(L"refs/tags/also-signed", list[0]);
	EXPECT_STREQ(L"refs/tags/normal-tag", list[1]);

	list.clear();
	EXPECT_EQ(0, m_Git.GetRefsCommitIsOn(list, CGitHash(L"35c91b4ae2f77f4f21a7aba56d3c473c705d89e6"), true, true));
	ASSERT_EQ(3, list.size());
	EXPECT_STREQ(L"refs/heads/master", list[0]);
	EXPECT_STREQ(L"refs/heads/master2", list[1]);
	EXPECT_STREQ(L"refs/tags/also-signed", list[2]);

	list.clear();
	EXPECT_EQ(0, m_Git.GetRefsCommitIsOn(list, CGitHash(L"313a41bc88a527289c87d7531802ab484715974f"), false, true));
	ASSERT_EQ(5, list.size());
	EXPECT_STREQ(L"refs/heads/forconflict", list[0]);
	EXPECT_STREQ(L"refs/heads/master", list[1]);
	EXPECT_STREQ(L"refs/heads/master2", list[2]);
	EXPECT_STREQ(L"refs/heads/simple-conflict", list[3]);
	EXPECT_STREQ(L"refs/heads/subdir/branch", list[4]);

	list.clear();
	EXPECT_EQ(0, m_Git.GetRefsCommitIsOn(list, CGitHash(L"313a41bc88a527289c87d7531802ab484715974f"), false, true, CGit::BRANCH_REMOTE));
	ASSERT_EQ(1, list.size());
	EXPECT_STREQ(L"refs/remotes/origin/master", list[0]);

	list.clear();
	EXPECT_EQ(0, m_Git.GetRefsCommitIsOn(list, CGitHash(L"313a41bc88a527289c87d7531802ab484715974f"), false, true, CGit::BRANCH_LOCAL));
	ASSERT_EQ(5, list.size());
	EXPECT_STREQ(L"refs/heads/forconflict", list[0]);
	EXPECT_STREQ(L"refs/heads/master", list[1]);
	EXPECT_STREQ(L"refs/heads/master2", list[2]);
	EXPECT_STREQ(L"refs/heads/simple-conflict", list[3]);
	EXPECT_STREQ(L"refs/heads/subdir/branch", list[4]);

	list.clear();
	EXPECT_EQ(0, m_Git.GetRefsCommitIsOn(list, CGitHash(L"313a41bc88a527289c87d7531802ab484715974f"), false, true, CGit::BRANCH_ALL));
	ASSERT_EQ(6, list.size());
	EXPECT_STREQ(L"refs/heads/forconflict", list[0]);
	EXPECT_STREQ(L"refs/remotes/origin/master", list[5]);
}
