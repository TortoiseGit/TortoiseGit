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

enum config
{
	LIBGIT2_ALL,
	LIBGIT2,
	LIBGIT,
	GIT_CLI,
};

enum check_str_empty
{
	EXPECT_STR_EMPTY,
	EXPECT_STR_NOT_EMPTY,
	NO_CHECK,
};

class CBasicGitFixture : public ::testing::TestWithParam<config>
{
protected:
	virtual void SetUp()
	{
		switch (GetParam())
		{
		case LIBGIT2_ALL:
			m_Git.m_IsUseLibGit2 = true;
			m_Git.m_IsUseLibGit2_mask = 0xffffffff;
			m_Git.m_IsUseGitDLL = false;
			break;
		case LIBGIT2:
			m_Git.m_IsUseLibGit2 = true;
			m_Git.m_IsUseLibGit2_mask = DEFAULT_USE_LIBGIT2_MASK;
			m_Git.m_IsUseGitDLL = false;
			break;
		case LIBGIT:
			m_Git.m_IsUseLibGit2 = false;
			m_Git.m_IsUseLibGit2_mask = 0;
			m_Git.m_IsUseGitDLL = true;
			break;
		case GIT_CLI:
		default:
			m_Git.m_IsUseLibGit2 = false;
			m_Git.m_IsUseLibGit2_mask = 0;
			m_Git.m_IsUseGitDLL = false;
		}
		m_Git.m_CurrentDir = m_Dir.GetTempDir();
	}

public:
	CGit m_Git;
	CAutoTempDir m_Dir;
	testing::AssertionResult RunGitCmdOrWarn(const char* pCmd, const char* /*pCheckOutputEmpty*/, const CString& cmd, check_str_empty checkOutputEmpty)
	{
		CString output;
		int ret = m_Git.Run(cmd, &output, CP_UTF8);
		switch (checkOutputEmpty)
		{
		case EXPECT_STR_EMPTY:
			EXPECT_TRUE(output.IsEmpty());
			break;
		case EXPECT_STR_NOT_EMPTY:
			EXPECT_FALSE(output.IsEmpty());
			break;
		}
		if (ret == 0)
			return testing::AssertionSuccess();
		testing::Message msg;
		msg << pCmd << "\n*** Git Command Error ***" << CStringA(output);
		return testing::AssertionFailure(msg);
	}
};

class CBasicGitWithEmptyRepositoryFixture : public CBasicGitFixture
{
protected:
	virtual void SetUp()
	{
		CBasicGitFixture::SetUp();
		EXPECT_PRED_FORMAT2(RunGitCmdOrWarn, _T("git.exe init"), EXPECT_STR_NOT_EMPTY);
	}
};

class CBasicGitWithEmptyBareRepositoryFixture : public CBasicGitFixture
{
protected:
	virtual void SetUp()
	{
		CBasicGitFixture::SetUp();
		EXPECT_PRED_FORMAT2(RunGitCmdOrWarn, _T("git.exe init --bare"), EXPECT_STR_NOT_EMPTY);
	}
};

TEST(CGit, RunSet)
{
	CString output;
	CGit cgit;
	ASSERT_EQ(0, cgit.Run(_T("cmd /c set"), &output, CP_UTF8));
	ASSERT_FALSE(output.IsEmpty());
	ASSERT_TRUE(output.Find(_T("windir"))); // should be there on any MS OS ;)
}

// For performance reason, turn LIBGIT off by default, 
INSTANTIATE_TEST_CASE_P(CGit, CBasicGitFixture, testing::Values(GIT_CLI, /*LIBGIT,*/ LIBGIT2, LIBGIT2_ALL));
INSTANTIATE_TEST_CASE_P(CGit, CBasicGitWithEmptyRepositoryFixture, testing::Values(GIT_CLI, /*LIBGIT,*/ LIBGIT2, LIBGIT2_ALL));

TEST_P(CBasicGitFixture, IsInitRepos)
{
	EXPECT_TRUE(m_Git.IsInitRepos());
}

TEST_P(CBasicGitWithEmptyRepositoryFixture, IsInitRepos)
{
	CString testFile = m_Dir.GetTempDir() + L"\\test.txt";

	EXPECT_TRUE(m_Git.IsInitRepos());

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)testFile, L"this is testing file."));
	EXPECT_PRED_FORMAT2(RunGitCmdOrWarn, _T("git.exe add test.txt"), EXPECT_STR_EMPTY);
	EXPECT_PRED_FORMAT2(RunGitCmdOrWarn, _T("git.exe commit -m \"Add test.txt\""), EXPECT_STR_NOT_EMPTY);

	EXPECT_FALSE(m_Git.IsInitRepos());
}

TEST_P(CBasicGitWithEmptyRepositoryFixture, CheckCleanWorkTree)
{
	CString testFile = m_Dir.GetTempDir() + L"\\test.txt";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)testFile, L"this is testing file."));
	EXPECT_PRED_FORMAT2(RunGitCmdOrWarn, _T("git.exe add test.txt"), EXPECT_STR_EMPTY);
	EXPECT_PRED_FORMAT2(RunGitCmdOrWarn, _T("git.exe commit -m \"Add test.txt\""), EXPECT_STR_NOT_EMPTY);
	// repo with 1 versioned file
	EXPECT_TRUE(m_Git.CheckCleanWorkTree());
	EXPECT_TRUE(m_Git.CheckCleanWorkTree(true));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)testFile, L"Overwriting this testing file."));
	// repo with 1 modified versioned file
	EXPECT_FALSE(m_Git.CheckCleanWorkTree());
	EXPECT_FALSE(m_Git.CheckCleanWorkTree(true));

	EXPECT_PRED_FORMAT2(RunGitCmdOrWarn, _T("git.exe add test.txt"), EXPECT_STR_EMPTY);
	// repo with 1 modified versioned and staged file
	EXPECT_FALSE(m_Git.CheckCleanWorkTree());
	EXPECT_TRUE(m_Git.CheckCleanWorkTree(true));

	EXPECT_PRED_FORMAT2(RunGitCmdOrWarn, _T("git.exe commit -m \"Modified test.txt\""), EXPECT_STR_NOT_EMPTY);
	testFile = m_Dir.GetTempDir() + L"\\test2.txt";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)testFile, L"this is ANOTHER testing file."));
	EXPECT_TRUE(m_Git.CheckCleanWorkTree());
	EXPECT_TRUE(m_Git.CheckCleanWorkTree(true));
}
