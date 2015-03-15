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
#include "Git.h"
#include "StringUtils.h"
#include "RepositoryFixtures.h"

// For performance reason, turn LIBGIT off by default, 
INSTANTIATE_TEST_CASE_P(CGit, CBasicGitFixture, testing::Values(GIT_CLI, /*LIBGIT,*/ LIBGIT2, LIBGIT2_ALL));
INSTANTIATE_TEST_CASE_P(CGit, CBasicGitWithEmptyRepositoryFixture, testing::Values(GIT_CLI, /*LIBGIT,*/ LIBGIT2, LIBGIT2_ALL));
INSTANTIATE_TEST_CASE_P(CGit, CBasicGitWithEmptyBareRepositoryFixture, testing::Values(GIT_CLI, /*LIBGIT,*/ LIBGIT2, LIBGIT2_ALL));
INSTANTIATE_TEST_CASE_P(CGit, CBasicGitWithTestRepoFixture, testing::Values(GIT_CLI, /*LIBGIT,*/ LIBGIT2, LIBGIT2_ALL));
INSTANTIATE_TEST_CASE_P(CGit, CBasicGitWithTestRepoBareFixture, testing::Values(GIT_CLI, /*LIBGIT,*/ LIBGIT2, LIBGIT2_ALL));

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

TEST(CGit, RunGit_Error)
{
	CAutoTempDir tempdir;
	CGit cgit;
	cgit.m_CurrentDir = tempdir.GetTempDir();
	
	CString output;
	ASSERT_EQ(2, cgit.Run(_T("git-not-found.exe"), &output, CP_UTF8));
	ASSERT_TRUE(output.IsEmpty());

	output.Empty();
	ASSERT_EQ(128, cgit.Run(_T("git.exe add file.txt"), &output, CP_UTF8));
	ASSERT_TRUE(output.Find(_T("fatal: Not a git repository (or any of the parent directories): .git")) == 0);
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
	STRING_VECTOR list;
	EXPECT_EQ(0, m_Git.GetBranchList(list, nullptr));
	EXPECT_EQ(4, list.size());
	list.clear();
	EXPECT_EQ(0, m_Git.GetBranchList(list, nullptr, CGit::BRANCH_LOCAL_F));
	EXPECT_EQ(4, list.size());

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

	CString testFile = m_Git.m_CurrentDir + L"\\.git\\FETCH_HEAD";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)testFile, L"b9ef30183497cdad5c30b88d32dc1bed7951dfeb		branch 'master' of https://code.google.com/p/tortoisegit\n737878a4e2eabfa4fab580867c2b060c70999d31	not-for-merge	branch 'extend_hooks' of https://code.google.com/p/tortoisegit\n"));

	list.clear();
	EXPECT_EQ(0, m_Git.GetBranchList(list, nullptr));
	EXPECT_EQ(4, list.size());
	list.clear();
	EXPECT_EQ(0, m_Git.GetBranchList(list, nullptr, CGit::BRANCH_LOCAL_F));
	EXPECT_EQ(isBare ? 4 : 5, list.size());

	EXPECT_STREQ(_T("master"), m_Git.FixBranchName(_T("master")));
	EXPECT_STREQ(_T("non-existing"), m_Git.FixBranchName(_T("non-existing")));
	if (!isBare)
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
	if (!isBare)
	{
		branch = _T("FETCH_HEAD");
		EXPECT_STREQ(_T("b9ef30183497cdad5c30b88d32dc1bed7951dfeb"), m_Git.FixBranchName_Mod(branch));
		EXPECT_STREQ(_T("b9ef30183497cdad5c30b88d32dc1bed7951dfeb"), branch);
		EXPECT_EQ(0, m_Git.GetHash(hash, _T("FETCH_HEAD")));
		EXPECT_STREQ(_T("b9ef30183497cdad5c30b88d32dc1bed7951dfeb"), hash.ToString());
	}

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)testFile, L"737878a4e2eabfa4fab580867c2b060c70999d31	not-for-merge	branch 'extend_hooks' of https://code.google.com/p/tortoisegit\nb9ef30183497cdad5c30b88d32dc1bed7951dfeb		branch 'master' of https://code.google.com/p/tortoisegit\n"));

	list.clear();
	EXPECT_EQ(0, m_Git.GetBranchList(list, nullptr));
	EXPECT_EQ(4, list.size());
	list.clear();
	EXPECT_EQ(0, m_Git.GetBranchList(list, nullptr, CGit::BRANCH_LOCAL_F));
	EXPECT_EQ(isBare ? 4 : 5, list.size());

	if (!isBare)
	{
		EXPECT_STREQ(_T("b9ef30183497cdad5c30b88d32dc1bed7951dfeb"), m_Git.FixBranchName(_T("FETCH_HEAD")));
		branch = _T("FETCH_HEAD");
		EXPECT_STREQ(_T("b9ef30183497cdad5c30b88d32dc1bed7951dfeb"), m_Git.FixBranchName_Mod(branch));
		EXPECT_STREQ(_T("b9ef30183497cdad5c30b88d32dc1bed7951dfeb"), branch);
		// libgit2 fails here
		// EXPECT_EQ(0, m_Git.GetHash(hash, _T("FETCH_HEAD")));
		// EXPECT_STREQ(_T("b9ef30183497cdad5c30b88d32dc1bed7951dfeb"), hash.ToString());
	}
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
	EXPECT_STREQ(_T("49ecdfff36bfe2b9b499b33e5034f427e2fa54dd"), hash.ToString());
	EXPECT_EQ(0, m_Git.GetHash(hash, _T("HEAD~1")));
	EXPECT_STREQ(_T("35c91b4ae2f77f4f21a7aba56d3c473c705d89e6"), hash.ToString());
	EXPECT_EQ(0, m_Git.GetHash(hash, _T("ff1fbef1a54a9849afd4a5e94d2ca4d80d5b96c2")));
	EXPECT_STREQ(_T("ff1fbef1a54a9849afd4a5e94d2ca4d80d5b96c2"), hash.ToString());
	EXPECT_EQ(0, m_Git.GetHash(hash, _T("master")));
	EXPECT_STREQ(_T("49ecdfff36bfe2b9b499b33e5034f427e2fa54dd"), hash.ToString());
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
	ASSERT_EQ(4, branches.size());
	EXPECT_EQ(1, current);
	EXPECT_STREQ(_T("forconflict"), branches[0]);
	EXPECT_STREQ(_T("master"), branches[1]);
	EXPECT_STREQ(_T("simple-conflict"), branches[2]);
	EXPECT_STREQ(_T("subdir/branch"), branches[3]);

	branches.clear();
	current = -2;
	EXPECT_EQ(0, m_Git.GetBranchList(branches, &current, CGit::BRANCH_ALL));
	ASSERT_EQ(5, branches.size());
	EXPECT_EQ(1, current);
	EXPECT_STREQ(_T("forconflict"), branches[0]);
	EXPECT_STREQ(_T("master"), branches[1]);
	EXPECT_STREQ(_T("simple-conflict"), branches[2]);
	EXPECT_STREQ(_T("subdir/branch"), branches[3]);
	EXPECT_STREQ(_T("remotes/origin/master"), branches[4]);

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
	ASSERT_EQ(9, refs.size());
	EXPECT_STREQ(_T("refs/heads/forconflict"), refs[0]);
	EXPECT_STREQ(_T("refs/heads/master"), refs[1]);
	EXPECT_STREQ(_T("refs/heads/simple-conflict"), refs[2]);
	EXPECT_STREQ(_T("refs/heads/subdir/branch"), refs[3]);
	EXPECT_STREQ(_T("refs/remotes/origin/master"), refs[4]);
	EXPECT_STREQ(_T("refs/stash"), refs[5]);
	EXPECT_STREQ(_T("refs/tags/all-files-signed"), refs[6]);
	EXPECT_STREQ(_T("refs/tags/also-signed"), refs[7]);
	EXPECT_STREQ(_T("refs/tags/normal-tag"), refs[8]);

	MAP_HASH_NAME map;
	EXPECT_EQ(0, m_Git.GetMapHashToFriendName(map));
	if (testConfig == GIT_CLI)
		ASSERT_EQ(10, map.size()); // also contains the undereferenced tags with hashes
	else
		ASSERT_EQ(8, map.size());
	ASSERT_EQ(1, map[CGitHash(L"8d1ebbcc7eeb63af10ff8bcf7712afb9fcc90b8a")].size());
	EXPECT_STREQ(_T("refs/heads/subdir/branch"), map[CGitHash(L"8d1ebbcc7eeb63af10ff8bcf7712afb9fcc90b8a")][0]);
	ASSERT_EQ(1, map[CGitHash(L"18da7c332dcad0f37f9977d9176dce0b0c66f3eb")].size());
	EXPECT_STREQ(_T("refs/stash"), map[CGitHash(L"18da7c332dcad0f37f9977d9176dce0b0c66f3eb")][0]);
	ASSERT_EQ(1, map[CGitHash(L"c5b89de0335fd674e2e421ac4543098cb2f22cde")].size());
	EXPECT_STREQ(_T("refs/heads/simple-conflict"), map[CGitHash(L"c5b89de0335fd674e2e421ac4543098cb2f22cde")][0]);
	ASSERT_EQ(1, map[CGitHash(L"10385764a4d42d7428bbeb245015f8f338fc1e40")].size());
	EXPECT_STREQ(_T("refs/heads/forconflict"), map[CGitHash(L"10385764a4d42d7428bbeb245015f8f338fc1e40")][0]);
	ASSERT_EQ(2, map[CGitHash(L"49ecdfff36bfe2b9b499b33e5034f427e2fa54dd")].size());
	EXPECT_STREQ(_T("refs/heads/master"), map[CGitHash(L"49ecdfff36bfe2b9b499b33e5034f427e2fa54dd")][0]);
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
	EXPECT_EQ(5, branches.size());
	tags.clear();
	EXPECT_EQ(0, m_Git.GetTagList(tags));
	EXPECT_EQ(3, tags.size());
	refs.clear();
	EXPECT_EQ(0, m_Git.GetRefList(refs));
	EXPECT_EQ(9, refs.size());

	EXPECT_EQ(-1, m_Git.DeleteRef(_T("refs/heads/gibbednet")));
	branches.clear();
	EXPECT_EQ(0, m_Git.GetBranchList(branches, nullptr, CGit::BRANCH_ALL));
	EXPECT_EQ(5, branches.size());
	tags.clear();
	EXPECT_EQ(0, m_Git.GetTagList(tags));
	EXPECT_EQ(3, tags.size());
	refs.clear();
	EXPECT_EQ(0, m_Git.GetRefList(refs));
	EXPECT_EQ(9, refs.size());

	EXPECT_EQ(-1, m_Git.DeleteRef(_T("refs/remotes/origin/gibbednet")));
	branches.clear();
	EXPECT_EQ(0, m_Git.GetBranchList(branches, nullptr, CGit::BRANCH_ALL));
	EXPECT_EQ(5, branches.size());
	tags.clear();
	EXPECT_EQ(0, m_Git.GetTagList(tags));
	EXPECT_EQ(3, tags.size());
	refs.clear();
	EXPECT_EQ(0, m_Git.GetRefList(refs));
	EXPECT_EQ(9, refs.size());

	EXPECT_EQ(0, m_Git.DeleteRef(_T("refs/tags/normal-tag")));
	branches.clear();
	EXPECT_EQ(0, m_Git.GetBranchList(branches, nullptr, CGit::BRANCH_ALL));
	EXPECT_EQ(5, branches.size());
	tags.clear();
	EXPECT_EQ(0, m_Git.GetTagList(tags));
	EXPECT_EQ(2, tags.size());
	refs.clear();
	EXPECT_EQ(0, m_Git.GetRefList(refs));
	EXPECT_EQ(8, refs.size());

	EXPECT_EQ(0, m_Git.DeleteRef(_T("refs/tags/all-files-signed^{}")));
	branches.clear();
	EXPECT_EQ(0, m_Git.GetBranchList(branches, nullptr, CGit::BRANCH_ALL));
	EXPECT_EQ(5, branches.size());
	tags.clear();
	EXPECT_EQ(0, m_Git.GetTagList(tags));
	EXPECT_EQ(1, tags.size());
	refs.clear();
	EXPECT_EQ(0, m_Git.GetRefList(refs));
	EXPECT_EQ(7, refs.size());

	EXPECT_EQ(0, m_Git.DeleteRef(_T("refs/heads/subdir/branch")));
	branches.clear();
	EXPECT_EQ(0, m_Git.GetBranchList(branches, nullptr, CGit::BRANCH_ALL));
	EXPECT_EQ(4, branches.size());
	tags.clear();
	EXPECT_EQ(0, m_Git.GetTagList(tags));
	EXPECT_EQ(1, tags.size());
	refs.clear();
	EXPECT_EQ(0, m_Git.GetRefList(refs));
	EXPECT_EQ(6, refs.size());

	EXPECT_EQ(0, m_Git.DeleteRef(_T("refs/remotes/origin/master")));
	branches.clear();
	EXPECT_EQ(0, m_Git.GetBranchList(branches, nullptr, CGit::BRANCH_ALL));
	EXPECT_EQ(3, branches.size());
	tags.clear();
	EXPECT_EQ(0, m_Git.GetTagList(tags));
	EXPECT_EQ(1, tags.size());
	refs.clear();
	EXPECT_EQ(0, m_Git.GetRefList(refs));
	EXPECT_EQ(5, refs.size());
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
	ASSERT_EQ(4, branches.size());
	EXPECT_EQ(-2, current);
	EXPECT_STREQ(_T("forconflict"), branches[0]);
	EXPECT_STREQ(_T("master"), branches[1]);
	EXPECT_STREQ(_T("simple-conflict"), branches[2]);
	EXPECT_STREQ(_T("subdir/branch"), branches[3]);
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
}
