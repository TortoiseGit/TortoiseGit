﻿// TortoiseGit - a Windows shell extension for easy version control

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

#pragma once
#include "Git.h"
#include "StringUtils.h"
#include "PathUtils.h"
#include "DirFileEnum.h"

enum config
{
	LIBGIT2_ALL,
	LIBGIT2,
	LIBGIT,
	GIT_CLI,
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
			m_Git.m_IsUseLibGit2 = false;
			m_Git.m_IsUseLibGit2_mask = 0;
			m_Git.m_IsUseGitDLL = false;
		}
		m_Git.m_CurrentDir = m_Dir.GetTempDir();
		// some methods rely on g_Git, so set values there as well
		g_Git.m_IsGitDllInited = false;
		g_Git.m_CurrentDir = m_Git.m_CurrentDir;
		g_Git.m_IsUseGitDLL = m_Git.m_IsUseGitDLL;
		g_Git.m_IsUseLibGit2 = m_Git.m_IsUseLibGit2;
		g_Git.m_IsUseLibGit2_mask = m_Git.m_IsUseLibGit2_mask;
		// libgit relies on CWD being set to working tree
		SetCurrentDirectory(m_Git.m_CurrentDir);
	}

public:
	CGit m_Git;
	CAutoTempDir m_Dir;
};

class CBasicGitWithTestRepoFixture : public CBasicGitFixture
{
protected:
	CBasicGitWithTestRepoFixture()
	{
		prefix = _T("\\.git");
	}

	virtual void SetUp()
	{
		CBasicGitFixture::SetUp();
		CString resourcesDir = CPathUtils::GetAppDirectory() + _T("\\resources");
		if (!PathIsDirectory(resourcesDir))
		{
			resourcesDir = CPathUtils::GetAppDirectory() + _T("\\..\\..\\..\\test\\UnitTests\\resources");
			ASSERT_TRUE(PathIsDirectory(resourcesDir));
		}
		EXPECT_TRUE(CreateDirectory(m_Dir.GetTempDir() + _T("\\.git"), nullptr));
		CString repoDir = resourcesDir + _T("\\git-repo1");
		CDirFileEnum finder(repoDir);
		bool isDir;
		CString filepath;
		while (finder.NextFile(filepath, &isDir))
		{
			CString relpath = filepath.Mid(repoDir.GetLength());
			if (isDir)
				EXPECT_TRUE(CreateDirectory(m_Dir.GetTempDir() + prefix + relpath, nullptr));
			else
				EXPECT_TRUE(CopyFile(filepath, m_Dir.GetTempDir() + prefix + relpath, false));
		}
	}
	CString prefix;
};

class CBasicGitWithTestRepoBareFixture : public CBasicGitWithTestRepoFixture
{
protected:
	virtual void SetUp()
	{
		prefix.Empty();
		CBasicGitWithTestRepoFixture::SetUp();

		DeleteFile(m_Dir.GetTempDir() + _T("\\index"));
		CString configFile = m_Dir.GetTempDir() + _T("\\config");
		CString text;
		ASSERT_TRUE(CStringUtils::ReadStringFromTextFile(configFile, text));
		EXPECT_EQ(1, text.Replace(_T("bare = false"), _T("bare = true")));
		EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)configFile, (LPCTSTR)text));
	}
};

class CBasicGitWithEmptyRepositoryFixture : public CBasicGitFixture
{
protected:
	virtual void SetUp()
	{
		CBasicGitFixture::SetUp();
		CString output;
		EXPECT_EQ(0, m_Git.Run(_T("git.exe init"), &output, CP_UTF8));
		EXPECT_FALSE(output.IsEmpty());
	}
};

class CBasicGitWithEmptyBareRepositoryFixture : public CBasicGitFixture
{
protected:
	virtual void SetUp()
	{
		CBasicGitFixture::SetUp();
		CString output;
		EXPECT_EQ(0, m_Git.Run(_T("git.exe init --bare"), &output, CP_UTF8));
		EXPECT_FALSE(output.IsEmpty());
	}
};
