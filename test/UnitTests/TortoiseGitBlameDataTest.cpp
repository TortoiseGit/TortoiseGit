// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2026 - TortoiseGit

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
#include "TortoiseGitBlameData.h"
#include "RepositoryFixtures.h"

class CTortoiseGitBlameDataRepositoryFixture : public CBasicGitWithTestRepoFixture
{
public:
	CTortoiseGitBlameDataRepositoryFixture() : CBasicGitWithTestRepoFixture(L"blame") {};
};

INSTANTIATE_TEST_SUITE_P(CTortoiseGitBlameData, CTortoiseGitBlameDataRepositoryFixture, testing::Values(LIBGIT));

namespace
{
	constexpr auto TEST_COMMIT = L"106b2e870129ed29a3213fb3c8ddf089ec9a9a82";
	const std::vector<const char*> kBlameContentTwoNewLines = { "dsad", "sad", "ds\xC3\xB6""d", "dssdddd", "dd", "sdas", "", "" };
	const std::vector<const char*> kBlameContentTwoNewLinesUtf16 = { "dsad", "sad", "ds\xC3\xB6""d", "dssdddd", "dd", "sdas", "", "", "" };

	BYTE_VECTOR RunBlame(const wchar_t* revision, const wchar_t* file)
	{
		CString cmd;
		cmd.Format(L"git.exe blame -p %s -- \"%s\"", revision, file);

		BYTE_VECTOR blameData;
		BYTE_VECTOR err;
		EXPECT_EQ(0, g_Git.Run(cmd, &blameData, &err));
		return blameData;
	}

	CTortoiseGitBlameData ParseBlame(const BYTE_VECTOR& blameData)
	{
		CGitHashMap hashMap;
		CTortoiseGitBlameData data;
		data.ParseBlameOutput(blameData, hashMap, 0, false);
		return data;
	}

	void ExpectLines(const CTortoiseGitBlameData& data, const std::vector<const char*>& expectedLines)
	{
		ASSERT_EQ(expectedLines.size(), data.GetNumberOfLines());
		for (size_t i = 0; i < expectedLines.size(); ++i)
			EXPECT_STREQ(expectedLines[i], data.GetUtf8Line(i));
	}
}

TEST_P(CTortoiseGitBlameDataRepositoryFixture, EmptyFile)
{
	const auto blameData = RunBlame(L"HEAD", L"empty.txt");
	auto data = ParseBlame(blameData);

	EXPECT_EQ(0u, data.GetNumberOfLines());
	EXPECT_EQ(CP_UTF8, data.UpdateEncoding(CP_UTF8));
}

TEST_P(CTortoiseGitBlameDataRepositoryFixture, EmptyFileUtf8Bom)
{
	const auto blameData = RunBlame(L"HEAD", L"empty-utf8-bom.txt");
	auto data = ParseBlame(blameData);

	ASSERT_EQ(1u, data.GetNumberOfLines());
	EXPECT_EQ(CP_UTF8, data.UpdateEncoding());
	EXPECT_STREQ("", data.GetUtf8Line(0));
}

TEST_P(CTortoiseGitBlameDataRepositoryFixture, EmptyFileUtf16BeBom)
{
	const auto blameData = RunBlame(L"HEAD", L"empty-utf16-be-bom.txt");
	auto data = ParseBlame(blameData);

	ASSERT_EQ(1u, data.GetNumberOfLines());
	EXPECT_EQ(1201, data.UpdateEncoding());
	EXPECT_STREQ("", data.GetUtf8Line(0));
}

TEST_P(CTortoiseGitBlameDataRepositoryFixture, EmptyFileUtf16LeBom)
{
	const auto blameData = RunBlame(L"HEAD", L"empty-utf16-le-bom.txt");
	auto data = ParseBlame(blameData);

	ASSERT_EQ(1u, data.GetNumberOfLines());
	EXPECT_EQ(1200, data.UpdateEncoding());
	EXPECT_STREQ("", data.GetUtf8Line(0));
}

TEST_P(CTortoiseGitBlameDataRepositoryFixture, Utf8FileTwoNewLinesAtEnd)
{
	const auto blameData = RunBlame(TEST_COMMIT, L"utf8-no-bom.txt");
	auto data = ParseBlame(blameData);

	EXPECT_EQ(CP_UTF8, data.UpdateEncoding());
	ExpectLines(data, kBlameContentTwoNewLines);
}

TEST_P(CTortoiseGitBlameDataRepositoryFixture, Utf8FileTwoNewLinesAtEndBom)
{
	const auto blameData = RunBlame(TEST_COMMIT, L"utf8-bom.txt");
	auto data = ParseBlame(blameData);

	EXPECT_EQ(CP_UTF8, data.UpdateEncoding());
	ExpectLines(data, kBlameContentTwoNewLines);
}

TEST_P(CTortoiseGitBlameDataRepositoryFixture, Utf16BeFileTwoNewLinesAtEnd)
{
	const auto blameData = RunBlame(TEST_COMMIT, L"utf16-be-no-bom.txt");
	auto data = ParseBlame(blameData);

	EXPECT_EQ(1201, data.UpdateEncoding());
	ExpectLines(data, kBlameContentTwoNewLines);
}

TEST_P(CTortoiseGitBlameDataRepositoryFixture, Utf16BeFileTwoNewLinesAtEndBom)
{
	const auto blameData = RunBlame(TEST_COMMIT, L"utf16-be-bom.txt");
	auto data = ParseBlame(blameData);

	EXPECT_EQ(1201, data.UpdateEncoding());
	ExpectLines(data, kBlameContentTwoNewLines);
}
TEST_P(CTortoiseGitBlameDataRepositoryFixture, Utf16LeFileTwoNewLinesAtEnd)
{
	const auto blameData = RunBlame(TEST_COMMIT, L"utf16-le-no-bom.txt");
	auto data = ParseBlame(blameData);

	EXPECT_EQ(1200, data.UpdateEncoding());
	ExpectLines(data, kBlameContentTwoNewLinesUtf16);
}

TEST_P(CTortoiseGitBlameDataRepositoryFixture, Utf16LeFileTwoNewLinesAtEndBom)
{
	const auto blameData = RunBlame(TEST_COMMIT, L"utf16-le-bom.txt");
	auto data = ParseBlame(blameData);

	EXPECT_EQ(1200, data.UpdateEncoding());
	ExpectLines(data, kBlameContentTwoNewLinesUtf16);
}

TEST_P(CTortoiseGitBlameDataRepositoryFixture, OEM850FileTwoNewLinesAtEnd)
{
	const auto blameData = RunBlame(TEST_COMMIT, L"oem850.txt");
	auto data = ParseBlame(blameData);

	EXPECT_EQ(static_cast<int>(GetACP()), data.UpdateEncoding());

	EXPECT_EQ(850, data.UpdateEncoding(850));
	ExpectLines(data, kBlameContentTwoNewLines);
}
