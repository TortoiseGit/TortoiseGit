// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2016 - TortoiseGit

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
	EXPECT_EQ(0, g_Git.Run(_T("git.exe init"), &output, CP_UTF8));
	EXPECT_FALSE(output.IsEmpty());
	CString testFile = tempdir.GetTempDir() + _T("\\.git\\config");
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)testFile, L"[push]\ndefault=something-that-is-invalid\n"));

	EXPECT_THROW(g_Git.CheckAndInitDll(), const char*);
}
