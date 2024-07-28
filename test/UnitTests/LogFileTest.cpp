// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2024 - TortoiseGit

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
#include "LogFile.h"

TEST(CLogFile, Empty)
{
	CString tmpfile = GetTempFile();
	ASSERT_STRNE(L"", tmpfile);
	SCOPE_EXIT{ ::DeleteFile(tmpfile); };
	CLogFile logFile{L"someRepo", 4000};
	ASSERT_TRUE(logFile.Open(tmpfile));
	EXPECT_TRUE(logFile.Close());
	EXPECT_EQ(0, CTGitPath(tmpfile).GetFileSize());
}

TEST(CLogFile, NoTruncate)
{
	CString tmpfile = GetTempFile();
	ASSERT_STRNE(L"", tmpfile);
	ASSERT_TRUE(CStringUtils::WriteStringToTextFile(tmpfile, L"1"));
	SCOPE_EXIT{ ::DeleteFile(tmpfile); };
	CLogFile logFile{ L"someRepo", 5 };
	ASSERT_TRUE(logFile.Open(tmpfile));
	EXPECT_TRUE(logFile.Close());
	CString text;
	ASSERT_TRUE(CStringUtils::ReadStringFromTextFile(tmpfile, text));
	EXPECT_STREQ(text, L"1");
}

TEST(CLogFile, TruncateToFive)
{
	CString tmpfile = GetTempFile();
	ASSERT_STRNE(L"", tmpfile);
	ASSERT_TRUE(CStringUtils::WriteStringToTextFile(tmpfile, L"1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n"));
	SCOPE_EXIT{ ::DeleteFile(tmpfile); };
	CLogFile logFile{ L"someRepo", 5 };
	ASSERT_TRUE(logFile.Open(tmpfile));
	EXPECT_TRUE(logFile.Close());
	CString text;
	ASSERT_TRUE(CStringUtils::ReadStringFromTextFile(tmpfile, text));
	EXPECT_STREQ(text, L"6\n7\n8\n9\n10\n");
}

TEST(CLogFile, TruncateToOne)
{
	CString tmpfile = GetTempFile();
	ASSERT_STRNE(L"", tmpfile);
	ASSERT_TRUE(CStringUtils::WriteStringToTextFile(tmpfile, L"1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n"));
	SCOPE_EXIT{ ::DeleteFile(tmpfile); };
	CLogFile logFile{ L"someRepo", 1 };
	ASSERT_TRUE(logFile.Open(tmpfile));
	EXPECT_TRUE(logFile.Close());
	CString text;
	ASSERT_TRUE(CStringUtils::ReadStringFromTextFile(tmpfile, text));
	EXPECT_STREQ(text, L"10\n");
}

TEST(CLogFile, TruncateToOneEmpty)
{
	CString tmpfile = GetTempFile();
	ASSERT_STRNE(L"", tmpfile);
	ASSERT_TRUE(CStringUtils::WriteStringToTextFile(tmpfile, L"1\n2\n3\n4\n5\n6\n7\n8\n9\n\n"));
	SCOPE_EXIT{ ::DeleteFile(tmpfile); };
	CLogFile logFile{ L"someRepo", 1 };
	ASSERT_TRUE(logFile.Open(tmpfile));
	EXPECT_TRUE(logFile.Close());
	CString text;
	ASSERT_TRUE(CStringUtils::ReadStringFromTextFile(tmpfile, text));
	EXPECT_STREQ(text, L"\n");
}

TEST(CLogFile, TruncateLarge)
{
	CString tmpfile = GetTempFile();
	ASSERT_STRNE(L"", tmpfile);
	ASSERT_TRUE(CStringUtils::WriteStringToTextFile(tmpfile, L"1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n"));
	SCOPE_EXIT{ ::DeleteFile(tmpfile); };
	CLogFile logFile{ L"someRepo", 5000 };
	ASSERT_TRUE(logFile.Open(tmpfile));
	EXPECT_TRUE(logFile.Close());
	CString text;
	ASSERT_TRUE(CStringUtils::ReadStringFromTextFile(tmpfile, text));
	EXPECT_STREQ(text, L"1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n");
}

TEST(CLogFile, LoggingDisabled)
{
	CString tmpfile = GetTempFile();
	ASSERT_STRNE(L"", tmpfile);
	ASSERT_TRUE(CStringUtils::WriteStringToTextFile(tmpfile, L"1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n"));
	SCOPE_EXIT{ ::DeleteFile(tmpfile); };
	CLogFile logFile{ L"someRepo", 0 };
	ASSERT_FALSE(logFile.Open(tmpfile));
}

TEST(CLogFile, Append)
{
	CString tmpfile = GetTempFile();
	ASSERT_STRNE(L"", tmpfile);
	SCOPE_EXIT{ ::DeleteFile(tmpfile); };
	CLogFile logFile{ L"someRepo", 2 };
	ASSERT_TRUE(logFile.Open(tmpfile));
	logFile.AddLine(L"1");
	logFile.AddLine(L"2");
	logFile.AddLine(L"3");
	EXPECT_TRUE(logFile.Close());
	CString text;
	ASSERT_TRUE(CStringUtils::ReadStringFromTextFile(tmpfile, text));
	EXPECT_STREQ(text, L"1\r\n2\r\n3\r\n");
}
