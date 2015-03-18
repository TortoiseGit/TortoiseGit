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
#include "StringUtils.h"

TEST(libgit2, Config)
{
	CAutoTempDir tempdir;
	CString testFile = tempdir.GetTempDir() + _T("\\config");
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile((LPCTSTR)testFile, L"[core]\nemail=dummy@example.com\ntrue=true\nfalse=false\n"));
	CAutoConfig config(true);
	EXPECT_EQ(0, git_config_add_file_ondisk(config, CUnicodeUtils::GetUTF8(testFile), GIT_CONFIG_LEVEL_LOCAL, 1));
	bool ret = false;
	EXPECT_EQ(0, config.GetBool(_T("core.true"), ret));
	EXPECT_EQ(true, ret);
	EXPECT_EQ(0, config.GetBool(_T("core.false"), ret));
	EXPECT_EQ(false, ret);
	EXPECT_EQ(-3, config.GetBool(_T("core.not-exist"), ret));
	CString value;
	EXPECT_EQ(0, config.GetString(_T("core.email"), value));
	EXPECT_STREQ(_T("dummy@example.com"), value);
}
