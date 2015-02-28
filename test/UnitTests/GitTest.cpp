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

TEST(CGit, RunSet)
{
	CString output;
	CGit cgit;
	ASSERT_EQ(0, cgit.Run(_T("cmd /c set"), &output, CP_UTF8));
	ASSERT_FALSE(output.IsEmpty());
	ASSERT_TRUE(output.Find(_T("windir"))); // should be there on any MS OS ;)
}

TEST(CGit, IsInitRepos_Git)
{
	CAutoTempDir tmpDir;

	CGit cgit;
	cgit.m_IsUseLibGit2 = false;
	cgit.m_CurrentDir = tmpDir.GetTempDir();

	ASSERT_TRUE(cgit.IsInitRepos());

	CString output;
	EXPECT_EQ(0, cgit.Run(_T("git.exe init"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());

	EXPECT_TRUE(cgit.IsInitRepos());
}

TEST(CGit, IsInitRepos_Libgit2)
{
	CAutoTempDir tmpDir;

	CGit cgit;
	cgit.m_IsUseLibGit2 = true;
	cgit.m_CurrentDir = tmpDir.GetTempDir();

	ASSERT_TRUE(cgit.IsInitRepos());

	CString output;
	EXPECT_EQ(0, cgit.Run(_T("git.exe init"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());

	EXPECT_TRUE(cgit.IsInitRepos());
}
