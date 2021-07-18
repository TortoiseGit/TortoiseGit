// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2020-2021 - TortoiseGit

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
#include "RepositoryFixtures.h"
#include "StagingOperations.h"

inline std::unique_ptr<char[]> ReadResourcePatchToBuffer(LPCTSTR resourceFile)
{
	CString resourceDir;
	GetResourcesDir(resourceDir);

	CFile file;
	file.Open(resourceDir + resourceFile, CFile::modeRead | CFile::typeBinary);
	auto filelen = file.GetLength();
	auto buf = std::make_unique<char[]>(static_cast<size_t>(filelen) + 1);
	file.Read(buf.get(), static_cast<UINT>(filelen));
	file.Close();
	return buf;
}

inline void TestLineType(const CDiffLinesForStaging& lines, const std::vector<int>& lineNums, DiffLineTypes type)
{
	for (auto lineNum : lineNums)
		EXPECT_EQ(lines.GetLineVec()[lineNum].type, type);
}

// Convenience function
inline void TestLineRangeType(const CDiffLinesForStaging& lines, int firstLine, int count, DiffLineTypes type)
{
	std::vector<int> lineNums;
	for (int i = 0; i < count; ++i)
		lineNums.emplace_back(firstLine + i);
	TestLineType(lines, lineNums, type);
}

TEST(CDiffLinesForStaging, LineTypes)
{
	auto buf = ReadResourcePatchToBuffer(L"\\patches\\partial-staging.patch");
	const int numLines = 105;
	CDiffLinesForStaging lines(buf.get(), numLines, 0, 0); // 0 and 0 don't matter here; numLines is sanity-checked by the constructor itself

	TestLineType(lines, { 0 }, DiffLineTypes::COMMAND);
	TestLineType(lines, { 1 }, DiffLineTypes::COMMENT);
	TestLineType(lines, { 2, 3 }, DiffLineTypes::HEADER);
	TestLineType(lines, { 4, 24, 33, 42, 59, 70, 79, 95 }, DiffLineTypes::POSITION);
	TestLineRangeType(lines, 5, 3, DiffLineTypes::DEFAULT);
	TestLineRangeType(lines, 10, 5, DiffLineTypes::DEFAULT);
	TestLineRangeType(lines, 17, 2, DiffLineTypes::DEFAULT);
	TestLineRangeType(lines, 21, 3, DiffLineTypes::DEFAULT);
	TestLineRangeType(lines, 25, 3, DiffLineTypes::DEFAULT);
	TestLineRangeType(lines, 30, 3, DiffLineTypes::DEFAULT);
	TestLineRangeType(lines, 34, 3, DiffLineTypes::DEFAULT);
	TestLineRangeType(lines, 39, 3, DiffLineTypes::DEFAULT);
	TestLineRangeType(lines, 43, 3, DiffLineTypes::DEFAULT);
	TestLineRangeType(lines, 50, 1, DiffLineTypes::DEFAULT);
	TestLineRangeType(lines, 53, 1, DiffLineTypes::DEFAULT);
	TestLineRangeType(lines, 56, 3, DiffLineTypes::DEFAULT);
	TestLineRangeType(lines, 60, 3, DiffLineTypes::DEFAULT);
	TestLineRangeType(lines, 67, 3, DiffLineTypes::DEFAULT);
	TestLineRangeType(lines, 71, 3, DiffLineTypes::DEFAULT);
	TestLineRangeType(lines, 76, 3, DiffLineTypes::DEFAULT);
	TestLineRangeType(lines, 80, 3, DiffLineTypes::DEFAULT);
	TestLineRangeType(lines, 85, 2, DiffLineTypes::DEFAULT);
	TestLineRangeType(lines, 89, 1, DiffLineTypes::DEFAULT);
	TestLineRangeType(lines, 92, 3, DiffLineTypes::DEFAULT);
	TestLineRangeType(lines, 96, 3, DiffLineTypes::DEFAULT);
	TestLineRangeType(lines, 101, 4, DiffLineTypes::DEFAULT);
	TestLineType(lines, {8, 9, 15, 19, 28, 37, 46, 47, 51, 54, 63, 64, 74, 83, 87, 90, 99}, DiffLineTypes::DELETED);
	TestLineType(lines, {16, 20, 29, 38, 48, 49, 52, 55, 65, 66, 75, 84, 88, 91, 100}, DiffLineTypes::ADDED);
}

TEST(CDiffLinesForStaging, LineTypesFoolParser)
{
	auto buf = ReadResourcePatchToBuffer(L"\\patches\\fool-parser.patch");
	const int numLines = 14;
	CDiffLinesForStaging lines(buf.get(), numLines, 0, 0); // 0 and 0 don't matter here; numLines is sanity-checked by the constructor itself

	TestLineType(lines, { 0 }, DiffLineTypes::COMMAND);
	TestLineType(lines, { 1 }, DiffLineTypes::COMMENT);
	TestLineType(lines, { 2, 3 }, DiffLineTypes::HEADER);
	TestLineType(lines, { 4 }, DiffLineTypes::POSITION);
	TestLineType(lines, { 5, 6, 9, 10, 11, 13 }, DiffLineTypes::DEFAULT);
	TestLineType(lines, { 7 }, DiffLineTypes::DELETED);
	TestLineType(lines, { 8 }, DiffLineTypes::ADDED);
	TestLineType(lines, { 12 }, DiffLineTypes::NO_NEWLINE_BOTHFILES);
}

TEST(CDiffLinesForStaging, LineTypesNoNewlineAdded)
{
	auto buf = ReadResourcePatchToBuffer(L"\\patches\\newline-at-end-of-file-added.patch");
	const int numLines = 11;
	CDiffLinesForStaging lines(buf.get(), numLines, 0, 0); // 0 and 0 don't matter here; numLines is sanity-checked by the constructor itself

	TestLineType(lines, { 0 }, DiffLineTypes::COMMAND);
	TestLineType(lines, { 1 }, DiffLineTypes::COMMENT);
	TestLineType(lines, { 2, 3 }, DiffLineTypes::HEADER);
	TestLineType(lines, { 4 }, DiffLineTypes::POSITION);
	TestLineType(lines, { 5, 6, 10 }, DiffLineTypes::DEFAULT);
	TestLineType(lines, { 7 }, DiffLineTypes::DELETED);
	TestLineType(lines, { 8 }, DiffLineTypes::NO_NEWLINE_OLDFILE);
	TestLineType(lines, { 9 }, DiffLineTypes::ADDED);
}

TEST(CDiffLinesForStaging, LineTypesNoNewlineDeleted)
{
	auto buf = ReadResourcePatchToBuffer(L"\\patches\\newline-at-end-of-file-deleted.patch");
	const int numLines = 11;
	CDiffLinesForStaging lines(buf.get(), numLines, 0, 0); // 0 and 0 don't matter here; numLines is sanity-checked by the constructor itself

	TestLineType(lines, { 0 }, DiffLineTypes::COMMAND);
	TestLineType(lines, { 1 }, DiffLineTypes::COMMENT);
	TestLineType(lines, { 2, 3 }, DiffLineTypes::HEADER);
	TestLineType(lines, { 4 }, DiffLineTypes::POSITION);
	TestLineType(lines, { 5, 6, 10 }, DiffLineTypes::DEFAULT);
	TestLineType(lines, { 7 }, DiffLineTypes::DELETED);
	TestLineType(lines, { 8 }, DiffLineTypes::ADDED);
	TestLineType(lines, { 9 }, DiffLineTypes::NO_NEWLINE_NEWFILE);
}

TEST(CDiffLinesForStaging, LineTypesNoNewlineLastLineModified)
{
	auto buf = ReadResourcePatchToBuffer(L"\\patches\\newline-at-end-of-file-last-line-modified.patch");
	const int numLines = 12;
	CDiffLinesForStaging lines(buf.get(), numLines, 0, 0); // 0 and 0 don't matter here; numLines is sanity-checked by the constructor itself

	TestLineType(lines, { 0 }, DiffLineTypes::COMMAND);
	TestLineType(lines, { 1 }, DiffLineTypes::COMMENT);
	TestLineType(lines, { 2, 3 }, DiffLineTypes::HEADER);
	TestLineType(lines, { 4 }, DiffLineTypes::POSITION);
	TestLineType(lines, { 5, 6, 11 }, DiffLineTypes::DEFAULT);
	TestLineType(lines, { 7 }, DiffLineTypes::DELETED);
	TestLineType(lines, { 8 }, DiffLineTypes::NO_NEWLINE_OLDFILE);
	TestLineType(lines, { 9 }, DiffLineTypes::ADDED);
	TestLineType(lines, { 10 }, DiffLineTypes::NO_NEWLINE_NEWFILE);
}

TEST(CDiffLinesForStaging, LineTypesNoNewlineOtherLineModified)
{
	auto buf = ReadResourcePatchToBuffer(L"\\patches\\newline-at-end-of-file-other-line-modified.patch");
	const int numLines = 11;
	CDiffLinesForStaging lines(buf.get(), numLines, 0, 0); // 0 and 0 don't matter here; numLines is sanity-checked by the constructor itself

	TestLineType(lines, { 0 }, DiffLineTypes::COMMAND);
	TestLineType(lines, { 1 }, DiffLineTypes::COMMENT);
	TestLineType(lines, { 2, 3 }, DiffLineTypes::HEADER);
	TestLineType(lines, { 4 }, DiffLineTypes::POSITION);
	TestLineType(lines, { 5, 8, 10 }, DiffLineTypes::DEFAULT);
	TestLineType(lines, { 6 }, DiffLineTypes::DELETED);
	TestLineType(lines, { 7 }, DiffLineTypes::ADDED);
	TestLineType(lines, { 9 }, DiffLineTypes::NO_NEWLINE_BOTHFILES);
}

inline void TestHunkStagingNullptr(const char* buf, int numLines, int firstLineSelected, int lastLineSelected)
{
	CDiffLinesForStaging lines(buf, numLines, firstLineSelected, lastLineSelected);
	StagingOperations op(&lines);
	EXPECT_STREQ(op.CreatePatchBufferToStageOrUnstageSelectedHunks().c_str(), "");
}

inline void TestHunkStagingException(const char* buf, int numLines, int firstLineSelected, int lastLineSelected)
{
	CDiffLinesForStaging lines(buf, numLines, firstLineSelected, lastLineSelected);
	StagingOperations op(&lines);
	EXPECT_ANY_THROW(op.CreatePatchBufferToStageOrUnstageSelectedHunks());
}

inline void TestLineStagingNullptr(const char* buf, int numLines, int firstLineSelected, int lastLineSelected)
{
	CDiffLinesForStaging lines(buf, numLines, firstLineSelected, lastLineSelected);
	StagingOperations op(&lines);
	EXPECT_STREQ(op.CreatePatchBufferToStageOrUnstageSelectedLines(StagingType::StageLines).c_str(), "");
}

inline void TestLineUnstagingNullptr(const char* buf, int numLines, int firstLineSelected, int lastLineSelected)
{
	CDiffLinesForStaging lines(buf, numLines, firstLineSelected, lastLineSelected);
	StagingOperations op(&lines);
	EXPECT_STREQ(op.CreatePatchBufferToStageOrUnstageSelectedLines(StagingType::UnstageLines).c_str(), "");
}

inline void TestLineStagingException(const char* buf, int numLines, int firstLineSelected, int lastLineSelected)
{
	CDiffLinesForStaging lines(buf, numLines, firstLineSelected, lastLineSelected);
	StagingOperations op(&lines);
	EXPECT_ANY_THROW(op.CreatePatchBufferToStageOrUnstageSelectedLines(StagingType::StageLines));
}

// When the user selects lines in the partial staging window and invokes the partial staging functionality,
// there are 5 mutually exclusive possibilities for what will happen to each line in the diff shown on the window:
// 1) Line (of any type) is discarded, i.e., not included in the temporary patch
// 2) Line (of any type) is included "as is" in the temporary patch
// 3) A + line is turned into context line in the temporary patch
// 4) A - line is turned into context line in the temporary patch
// 5) A @@ line has its counts changed and is included in the temporary patch
// Each line in the temporary patch corresponds to one and only one of these possibilities, except 1) of course.
// The functions below test possibilities 2, 3, 4 and 5.
// Since the expected number of lines in the temporary patch is passed to the constructor of CDiffLinesForStaging, which sanity-checks it,
// discarded lines (possibility 1) are implicitly tested, as long as all the lines from the temporary patch are passed to the appropriate test.

inline void ExpectLineIncludedAsIs(const CDiffLinesForStaging& base, const CDiffLinesForStaging& temp, const std::map<int, int>& linemap)
{
	for (auto match : linemap)
	{
		EXPECT_EQ(strcmp(base.GetLineVec()[match.first].sLine.c_str(), temp.GetLineVec()[match.second].sLine.c_str()), 0); // lines are identical?
		EXPECT_EQ(base.GetLineVec()[match.first].type, temp.GetLineVec()[match.second].type); // for good measure
	}
}

// Convenience function
inline void ExpectLineRangeIncludedAsIs(const CDiffLinesForStaging& base, const CDiffLinesForStaging& temp, int baseFirstLine, int tempFirstLine, int count)
{
	std::map<int, int> linemap;
	for (int i = 0; i < count; ++i)
		linemap.emplace(baseFirstLine + i, tempFirstLine + i);
	ExpectLineIncludedAsIs(base, temp, linemap);
}

inline void ExpectNewLineTurnedIntoContext(const CDiffLinesForStaging& base, const CDiffLinesForStaging& temp, const std::map<int, int>& linemap)
{
	for (auto match : linemap)
	{
		EXPECT_EQ(base.GetLineVec()[match.first].sLine.length(), temp.GetLineVec()[match.second].sLine.length());
		EXPECT_TRUE(base.GetLineVec()[match.first].sLine.length() >= 2);
		// lines are identical ignoring the first character?
		EXPECT_EQ(strcmp(base.GetLineVec()[match.first].sLine.c_str() + 1, temp.GetLineVec()[match.second].sLine.c_str() + 1), 0);
		EXPECT_EQ(base.GetLineVec()[match.first].type, DiffLineTypes::ADDED);
		EXPECT_EQ(temp.GetLineVec()[match.second].type, DiffLineTypes::DEFAULT);
	}
}

inline void ExpectOldLineTurnedIntoContext(const CDiffLinesForStaging& base, const CDiffLinesForStaging& temp, const std::map<int, int>& linemap)
{
	for (auto match : linemap)
	{
		EXPECT_EQ(base.GetLineVec()[match.first].sLine.length(), temp.GetLineVec()[match.second].sLine.length());
		EXPECT_TRUE(base.GetLineVec()[match.first].sLine.length() >= 2);
		// lines are identical ignoring the first character?
		EXPECT_EQ(strcmp(base.GetLineVec()[match.first].sLine.c_str() + 1, temp.GetLineVec()[match.second].sLine.c_str() + 1), 0);
		EXPECT_EQ(base.GetLineVec()[match.first].type, DiffLineTypes::DELETED);
		EXPECT_EQ(temp.GetLineVec()[match.second].type, DiffLineTypes::DEFAULT);
	}
}

inline void ExpectPositionLineCountsChanged(const CDiffLinesForStaging& base, const CDiffLinesForStaging& temp, int basePositionLine, int tempPositionLine, int new_oldCount, int new_newCount)
{
	EXPECT_EQ(temp.GetLineVec()[tempPositionLine].type, DiffLineTypes::POSITION);
	StagingOperations op(&base);
	auto changedbuf = op.ChangeOldAndNewLinesCount(base.GetLineVec()[basePositionLine].sLine, new_oldCount, new_newCount);
	EXPECT_EQ(strcmp(changedbuf.c_str(), temp.GetLineVec()[tempPositionLine].sLine.c_str()), 0);
}

TEST(StagingOperations, PatchBuffer)
{
	auto buf = ReadResourcePatchToBuffer(L"\\patches\\partial-staging.patch");
	const int numLines = 105;

	// nothing useful selected (file header)
	for (int i = 0; i <= 3; ++i)
	{
		for (int j = i; j <= 3; ++j)
			TestHunkStagingNullptr(buf.get(), numLines, i, j);
	}

	// firstLineSelected > lastLineSelected (should never happen)
	TestHunkStagingNullptr(buf.get(), numLines, 1, 0);

	// out-of-bounds (should never happen)
	TestHunkStagingException(buf.get(), numLines, 0, numLines);
	TestHunkStagingException(buf.get(), numLines, numLines, 0);
	TestHunkStagingException(buf.get(), numLines, numLines, numLines);
	TestHunkStagingException(buf.get(), numLines, numLines - 1, numLines);
	TestHunkStagingException(buf.get(), numLines, numLines, numLines - 1);

	// nothing useful selected (file header and/or only context lines)
	for (int i = 0; i <= 7; ++i)
	{
		for (int j = i; j <= 7; ++j)
			TestLineStagingNullptr(buf.get(), numLines, i, j);
	}

	for (int i = 10; i <= 14; ++i)
	{
		for (int j = i; j <= 14; ++j)
			TestLineStagingNullptr(buf.get(), numLines, i, j);
	}

	for (int i = 101; i <= 104; ++i)
	{
		for (int j = i; j <= 104; ++j)
			TestLineStagingNullptr(buf.get(), numLines, i, j);
	}

	// nothing useful selected (only context lines from neighboring hunks)
	for (int i = 30; i <= 36; ++i)
	{
		for (int j = i; j <= 36; ++j)
			TestLineStagingNullptr(buf.get(), numLines, i, j);
	}

	// nothing useful selected (last line)
	TestLineStagingNullptr(buf.get(), numLines, numLines - 1, numLines - 1);

	// firstLineSelected > lastLineSelected (should never happen)
	TestLineStagingNullptr(buf.get(), numLines, 1, 0);

	// out-of-bounds (should never happen)
	TestLineStagingException(buf.get(), numLines, 0, numLines);
	TestLineStagingException(buf.get(), numLines, numLines, 0);
	TestLineStagingException(buf.get(), numLines, numLines, numLines);
	TestLineStagingException(buf.get(), numLines, numLines - 1, numLines);
	TestLineStagingException(buf.get(), numLines, numLines, numLines - 1);

	// The first hunk with some of its modified lines selected
	CDiffLinesForStaging base(buf.get(), numLines, 7, 11); // user selected lines 8 to 12 and clicked line staging
	StagingOperations op(&base);
	auto tempbuf = op.CreatePatchBufferToStageOrUnstageSelectedLines(StagingType::StageLines);
	CDiffLinesForStaging temp(tempbuf.c_str(), 23, 0, 0); // the temporary patch should have 23 lines, 0 and 0 don't matter here
	ExpectLineRangeIncludedAsIs(base, temp, 0, 0, 15); // first 15 lines
	ExpectOldLineTurnedIntoContext(base, temp, { {15,15},{19,18} });
	ExpectLineIncludedAsIs(base, temp, { {17,16},{18,17},{21,19},{22,20},{23,21},{104,22} });

	// Two hunks at the end of the file, both with at least one - or + line selected
	base = CDiffLinesForStaging(buf.get(), numLines, 84, 99); // user selected lines 85 to 100 and clicked line staging
	op = StagingOperations(&base);
	tempbuf = op.CreatePatchBufferToStageOrUnstageSelectedLines(StagingType::StageLines);
	temp = CDiffLinesForStaging(tempbuf.c_str(), 29, 0, 0); // the temporary patch should have 29 lines, 0 and 0 don't matter here
	ExpectLineRangeIncludedAsIs(base, temp, 0, 0, 4); // first 4 lines (file header)
	ExpectPositionLineCountsChanged(base, temp, 79, 4, 12, 13); // @@ -336,12 +334,12  ->  @@ -336,12 +334,13
	ExpectLineRangeIncludedAsIs(base, temp, 80, 5, 3);
	ExpectOldLineTurnedIntoContext(base, temp, { {83,8} });
	ExpectLineRangeIncludedAsIs(base, temp, 84, 9, 11);
	ExpectPositionLineCountsChanged(base, temp, 95, 20, 7, 6); // @@ -362,7 +360,7  ->  @@ -362,7 +360,6
	ExpectLineRangeIncludedAsIs(base, temp, 96, 21, 4);
	ExpectLineRangeIncludedAsIs(base, temp, 101, 25, 4);

	// Four hunks. The two hunks at either end have no - or + lines selected and so must be discarded
	base = CDiffLinesForStaging(buf.get(), numLines, 30, 62); // user selected lines 31 to 63 and clicked line unstaging
	op = StagingOperations(&base);
	tempbuf = op.CreatePatchBufferToStageOrUnstageSelectedLines(StagingType::UnstageLines);
	temp = CDiffLinesForStaging(tempbuf.c_str(), 31, 0, 0); // the temporary patch should have 31 lines, 0 and 0 don't matter here
	ExpectLineRangeIncludedAsIs(base, temp, 0, 0, 4); // first 4 lines (file header)
	ExpectLineRangeIncludedAsIs(base, temp, 33, 4, 26);
	ExpectLineIncludedAsIs(base, temp, { {104,30} });

	// Two hunks with some of their modified lines selected
	base = CDiffLinesForStaging(buf.get(), numLines, 64, 74); // user selected lines 65 to 75 and clicked line unstaging
	op = StagingOperations(&base);
	tempbuf = op.CreatePatchBufferToStageOrUnstageSelectedLines(StagingType::UnstageLines);
	temp = CDiffLinesForStaging(tempbuf.c_str(), 24, 0, 0); // the temporary patch should have 24 lines, 0 and 0 don't matter here
	ExpectLineRangeIncludedAsIs(base, temp, 0, 0, 4); // first 4 lines (file header)
	ExpectPositionLineCountsChanged(base, temp, 59, 4, 7, 8); // @@ -122,8 +120,8  ->  @@ -122,7 +120,8 
	ExpectLineRangeIncludedAsIs(base, temp, 60, 5, 3);
	ExpectLineRangeIncludedAsIs(base, temp, 64, 8, 6);
	ExpectPositionLineCountsChanged(base, temp, 70, 14, 8, 7); // @@ -132,7 +130,7  ->  @@ -132,8 +130,7
	ExpectLineRangeIncludedAsIs(base, temp, 71, 15, 4);
	ExpectNewLineTurnedIntoContext(base, temp, { {75,19} });
	ExpectLineRangeIncludedAsIs(base, temp, 76, 20, 3);
	ExpectLineIncludedAsIs(base, temp, { {104,23} });
}

TEST(StagingOperations, SingleLine)
{
	auto buf = ReadResourcePatchToBuffer(L"\\patches\\git-diff-single-line-change.patch");
	const int numLines = 8;

	for (int i = 0; i < numLines; ++i)
	{
		for (int j = i; j < numLines; ++j)
		{
			TestHunkStagingNullptr(buf.get(), numLines, i, j);
			TestLineStagingNullptr(buf.get(), numLines, i, j);
			TestLineUnstagingNullptr(buf.get(), numLines, i, j);
		}
	}
}

TEST(StagingOperations, DeletedFile)
{
	auto buf = ReadResourcePatchToBuffer(L"\\patches\\git-diff.patch");
	const int numLines = 179;

	for (int i = 6; i < 21; ++i) // covers some of the lines regarding the deleted file and also part of the diff headers
	{
		for (int j = i; j < 21; ++j)
		{
			TestHunkStagingNullptr(buf.get(), numLines, i, j);
			TestLineStagingNullptr(buf.get(), numLines, i, j);
			TestLineUnstagingNullptr(buf.get(), numLines, i, j);
		}
	}
}

TEST(StagingOperations, AddedFile)
{
	auto buf = ReadResourcePatchToBuffer(L"\\patches\\git-diff.patch");
	const int numLines = 179;

	for (int i = 166; i < 179; ++i) // covers the lines regarding the added file and also its headers
	{
		for (int j = i; j < 179; ++j)
		{
			TestHunkStagingNullptr(buf.get(), numLines, i, j);
			TestLineStagingNullptr(buf.get(), numLines, i, j);
			TestLineUnstagingNullptr(buf.get(), numLines, i, j);
		}
	}
}

class CBasicGitWithPartialStagingRepositoryFixture : public CBasicGitWithTestRepoFixture
{
public:
	CBasicGitWithPartialStagingRepositoryFixture() : CBasicGitWithTestRepoFixture(L"git-partial-staging-repo") {};
};

INSTANTIATE_TEST_SUITE_P(PartialStaging, CBasicGitWithPartialStagingRepositoryFixture, testing::Values(GIT_CLI));


TEST_P(CBasicGitWithPartialStagingRepositoryFixture, NoNewlineAdded)
{
	auto buf = ReadResourcePatchToBuffer(L"\\patches\\newline-at-end-of-file-added.patch");
	const int numLines = 11;

	for (int i = 7; i <= 7; ++i) // - line alone or with \ No newline
	{
		for (int j = i; j <= 8; ++j)
		{
			CDiffLinesForStaging base(buf.get(), numLines, i, j);
			StagingOperations op(&base);
			auto tempbuf = op.CreatePatchBufferToStageOrUnstageSelectedLines(StagingType::StageLines);
			CDiffLinesForStaging temp(tempbuf.c_str(), 10, 0, 0); // the temporary patch should have 10 lines, 0 and 0 don't matter here
			ExpectLineRangeIncludedAsIs(base, temp, 0, 0, 4); // first 4 lines (file header)
			ExpectPositionLineCountsChanged(base, temp, 4, 4, 3, 2); // @@ -1,3 +1,3 @@  ->  @@ -1,3 +1,2 @@
			ExpectLineRangeIncludedAsIs(base, temp, 5, 5, 4);
			ExpectLineIncludedAsIs(base, temp, { {10, 9} });

			CString cmd, out;
			EXPECT_EQ(0, g_Git.ApplyPatchToIndex(static_cast<LPCWSTR>(StagingOperations::WritePatchBufferToTemporaryFile(tempbuf)), &out));
			EXPECT_STREQ(L"", out);
			cmd.Format(L"git.exe reset --hard HEAD");
			EXPECT_EQ(0, g_Git.Run(cmd, &out, CP_UTF8));
		}
	}

	// \ No newline alone
	TestLineStagingNullptr(buf.get(), numLines, 8, 8);

	// This will stage a "3" and newline being concatenated onto the old "3" without newline  (3 no newline -> 33 newline)
	for (auto& pair : std::initializer_list<std::pair<int, int>>{ {8,9}, {8,10}, {9,9}, {9,10} }) // + line alone or with \ No newline and/or last (empty) line
	{
		CDiffLinesForStaging base(buf.get(), numLines, pair.first, pair.second);
		StagingOperations op(&base);
		auto tempbuf = op.CreatePatchBufferToStageOrUnstageSelectedLines(StagingType::StageLines);
		CDiffLinesForStaging temp(tempbuf.c_str(), 11, 0, 0); // the temporary patch should have 11 lines, 0 and 0 don't matter here
		ExpectLineRangeIncludedAsIs(base, temp, 0, 0, 4); // first 4 lines (file header)
		ExpectPositionLineCountsChanged(base, temp, 4, 4, 3, 4); // @@ -1,3 +1,3 @@  ->  @@ -1,3 +1,4 @@
		ExpectLineRangeIncludedAsIs(base, temp, 5, 5, 2);
		ExpectOldLineTurnedIntoContext(base, temp, { {7, 7} });
		ExpectLineRangeIncludedAsIs(base, temp, 8, 8, 3);

		CString cmd, out;
		EXPECT_EQ(0, g_Git.ApplyPatchToIndex(static_cast<LPCWSTR>(StagingOperations::WritePatchBufferToTemporaryFile(tempbuf)), &out));
		EXPECT_STREQ(L"", out);
		cmd.Format(L"git.exe reset --hard HEAD");
		EXPECT_EQ(0, g_Git.Run(cmd, &out, CP_UTF8));
	}

	// This will stage a newline and unstage it back
	for (auto& pair : std::initializer_list<std::pair<int, int>>{ {6,9}, {6,10}, {7,9}, {7,10} }) // both - and + lines, with or without one neighboring context line
	{
		CDiffLinesForStaging base(buf.get(), numLines, pair.first, pair.second);
		StagingOperations op(&base);
		auto tempbuf = op.CreatePatchBufferToStageOrUnstageSelectedLines(StagingType::StageLines);
		CDiffLinesForStaging temp(tempbuf.c_str(), 11, 0, 0); // the temporary patch should have 11 lines, 0 and 0 don't matter here
		ExpectLineRangeIncludedAsIs(base, temp, 0, 0, 11); // full patch

		CString cmd, out;
		EXPECT_EQ(0, g_Git.ApplyPatchToIndex(static_cast<LPCWSTR>(StagingOperations::WritePatchBufferToTemporaryFile(tempbuf)), &out));
		EXPECT_STREQ(L"", out);

		tempbuf = op.CreatePatchBufferToStageOrUnstageSelectedLines(StagingType::UnstageLines);
		temp = CDiffLinesForStaging(tempbuf.c_str(), 11, 0, 0); // the temporary patch should have 11 lines, 0 and 0 don't matter here
		ExpectLineRangeIncludedAsIs(base, temp, 0, 0, 11); // full patch

		EXPECT_EQ(0, g_Git.ApplyPatchToIndexReverse(static_cast<LPCWSTR>(StagingOperations::WritePatchBufferToTemporaryFile(tempbuf)), &out));
		EXPECT_STREQ(L"", out);

		cmd.Format(L"git.exe reset --hard HEAD");
		EXPECT_EQ(0, g_Git.Run(cmd, &out, CP_UTF8));
	}
}

TEST_P(CBasicGitWithPartialStagingRepositoryFixture, NoNewlineLastLineModified)
{
	auto buf = ReadResourcePatchToBuffer(L"\\patches\\newline-at-end-of-file-last-line-modified.patch");
	const int numLines = 12;

	for (int i = 7; i <= 7; i++) // - line alone or with \ No newline
	{
		for (int j = i; j <= 8; j++)
		{
			CDiffLinesForStaging base(buf.get(), numLines, i, j);
			StagingOperations op(&base);
			auto tempbuf = op.CreatePatchBufferToStageOrUnstageSelectedLines(StagingType::StageLines);
			CDiffLinesForStaging temp(tempbuf.c_str(), 10, 0, 0); // the temporary patch should have 10 lines, 0 and 0 don't matter here
			ExpectLineRangeIncludedAsIs(base, temp, 0, 0, 4); // first 4 lines (file header)
			ExpectPositionLineCountsChanged(base, temp, 4, 4, 3, 2); // @@ -1,3 +1,3 @@  ->  @@ -1,3 +1,2 @@
			ExpectLineRangeIncludedAsIs(base, temp, 5, 5, 4);
			ExpectLineIncludedAsIs(base, temp, { {11, 9} });

			CString cmd, out;
			EXPECT_EQ(0, g_Git.ApplyPatchToIndex(static_cast<LPCWSTR>(StagingOperations::WritePatchBufferToTemporaryFile(tempbuf)), &out));
			EXPECT_STREQ(L"", out);
			cmd.Format(L"git.exe reset --hard HEAD");
			EXPECT_EQ(0, g_Git.Run(cmd, &out, CP_UTF8));
		}
	}

	// \ No newline alone
	TestLineStagingNullptr(buf.get(), numLines, 8, 8);
	TestLineStagingNullptr(buf.get(), numLines, 10, 10);

	// This will stage a "34" without newline being concatenated onto the old "3" without newline  (3 no newline -> 334 no newline)
	for (auto& pair : std::initializer_list<std::pair<int, int>>{ {8,10}, {8,11}, {9,10}, {9,11} }) // + line with its \ No newline and with/without other \ No newline and last (empty) line
	{
		CDiffLinesForStaging base(buf.get(), numLines, pair.first, pair.second);
		StagingOperations op(&base);
		auto tempbuf = op.CreatePatchBufferToStageOrUnstageSelectedLines(StagingType::StageLines);
		CDiffLinesForStaging temp(tempbuf.c_str(), 12, 0, 0); // the temporary patch should have 12 lines, 0 and 0 don't matter here
		ExpectLineRangeIncludedAsIs(base, temp, 0, 0, 4); // first 4 lines (file header)
		ExpectPositionLineCountsChanged(base, temp, 4, 4, 3, 4); // @@ -1,3 +1,3 @@  ->  @@ -1,3 +1,4 @@
		ExpectLineRangeIncludedAsIs(base, temp, 5, 5, 2);
		ExpectOldLineTurnedIntoContext(base, temp, { {7, 7} });
		ExpectLineRangeIncludedAsIs(base, temp, 8, 8, 4);

		CString cmd, out;
		EXPECT_EQ(0, g_Git.ApplyPatchToIndex(static_cast<LPCWSTR>(StagingOperations::WritePatchBufferToTemporaryFile(tempbuf)), &out));
		EXPECT_STREQ(L"", out);
		cmd.Format(L"git.exe reset --hard HEAD");
		EXPECT_EQ(0, g_Git.Run(cmd, &out, CP_UTF8));
	}

	// This will stage a "34" with newline being concatenated onto the old "3" without newline  (3 no newline -> 334 newline)
	for (auto& pair : std::initializer_list<std::pair<int, int>>{ {8,9}, {9,9} }) // + line without its \ No newline and with/without other \ No newline
	{
		CDiffLinesForStaging base(buf.get(), numLines, pair.first, pair.second);
		StagingOperations op(&base);
		auto tempbuf = op.CreatePatchBufferToStageOrUnstageSelectedLines(StagingType::StageLines);
		CDiffLinesForStaging temp(tempbuf.c_str(), 11, 0, 0); // the temporary patch should have 11 lines, 0 and 0 don't matter here
		ExpectLineRangeIncludedAsIs(base, temp, 0, 0, 4); // first 4 lines (file header)
		ExpectPositionLineCountsChanged(base, temp, 4, 4, 3, 4); // @@ -1,3 +1,3 @@  ->  @@ -1,3 +1,4 @@
		ExpectLineRangeIncludedAsIs(base, temp, 5, 5, 2);
		ExpectOldLineTurnedIntoContext(base, temp, { {7, 7} });
		ExpectLineRangeIncludedAsIs(base, temp, 8, 8, 2);
		ExpectLineIncludedAsIs(base, temp, { {11, 10} });

		CString cmd, out;
		EXPECT_EQ(0, g_Git.ApplyPatchToIndex(static_cast<LPCWSTR>(StagingOperations::WritePatchBufferToTemporaryFile(tempbuf)), &out));
		EXPECT_STREQ(L"", out);
		cmd.Format(L"git.exe reset --hard HEAD");
		EXPECT_EQ(0, g_Git.Run(cmd, &out, CP_UTF8));
	}

	// This will stage a modification in the last line and unstage it back
	for (auto& pair : std::initializer_list<std::pair<int, int>>{ {6,10}, {6,11}, {7,10}, {7,11} }) // both - and + lines, with or without one neighboring context line
	{
		CDiffLinesForStaging base(buf.get(), numLines, pair.first, pair.second);
		StagingOperations op(&base);
		auto tempbuf = op.CreatePatchBufferToStageOrUnstageSelectedLines(StagingType::StageLines);
		CDiffLinesForStaging temp(tempbuf.c_str(), 12, 0, 0); // the temporary patch should have 12 lines, 0 and 0 don't matter here
		ExpectLineRangeIncludedAsIs(base, temp, 0, 0, 12); // full patch

		CString cmd, out;
		EXPECT_EQ(0, g_Git.ApplyPatchToIndex(static_cast<LPCWSTR>(StagingOperations::WritePatchBufferToTemporaryFile(tempbuf)), &out));
		EXPECT_STREQ(L"", out);

		tempbuf = op.CreatePatchBufferToStageOrUnstageSelectedLines(StagingType::UnstageLines);
		temp = CDiffLinesForStaging(tempbuf.c_str(), 12, 0, 0); // the temporary patch should have 11 lines, 0 and 0 don't matter here
		ExpectLineRangeIncludedAsIs(base, temp, 0, 0, 12); // full patch

		EXPECT_EQ(0, g_Git.ApplyPatchToIndexReverse(static_cast<LPCWSTR>(StagingOperations::WritePatchBufferToTemporaryFile(tempbuf)), &out));
		EXPECT_STREQ(L"", out);

		cmd.Format(L"git.exe reset --hard HEAD");
		EXPECT_EQ(0, g_Git.Run(cmd, &out, CP_UTF8));
	}
}

TEST_P(CBasicGitWithPartialStagingRepositoryFixture, NoNewlineOtherLineModified)
{
	// Stage the - line alone
	{
		auto buf = ReadResourcePatchToBuffer(L"\\patches\\newline-at-end-of-file-other-line-modified.patch");
		const int numLines = 11;

		CDiffLinesForStaging base(buf.get(), numLines, 6, 6);
		StagingOperations op(&base);
		auto tempbuf = op.CreatePatchBufferToStageOrUnstageSelectedLines(StagingType::StageLines);
		CDiffLinesForStaging temp(tempbuf.c_str(), 10, 0, 0); // the temporary patch should have 10 lines, 0 and 0 don't matter here
		ExpectLineRangeIncludedAsIs(base, temp, 0, 0, 4); // first 4 lines (file header)
		ExpectPositionLineCountsChanged(base, temp, 4, 4, 3, 2); // @@ -1,3 +1,3 @@  ->  @@ -1,3 +1,2 @@
		ExpectLineRangeIncludedAsIs(base, temp, 5, 5, 2);
		ExpectLineRangeIncludedAsIs(base, temp, 8, 7, 3);

		CString out;
		EXPECT_EQ(0, g_Git.ApplyPatchToIndex(static_cast<LPCWSTR>(StagingOperations::WritePatchBufferToTemporaryFile(tempbuf)), &out));
		EXPECT_STREQ(L"", out);

		// \ No newline alone
		TestLineStagingNullptr(buf.get(), numLines, 9, 9);
	}
	
	// Unstage it back
	{
		auto buf = ReadResourcePatchToBuffer(L"\\patches\\newline-at-end-of-file-other-line-modified2.patch");
		const int numLines = 10;

		CDiffLinesForStaging base(buf.get(), numLines, 6, 6);
		StagingOperations op(&base);
		auto tempbuf = op.CreatePatchBufferToStageOrUnstageSelectedLines(StagingType::UnstageLines);
		CDiffLinesForStaging temp(tempbuf.c_str(), 10, 0, 0); // the temporary patch should have 10 lines, 0 and 0 don't matter here
		ExpectLineRangeIncludedAsIs(base, temp, 0, 0, 10); // entire patch

		CString out;
		EXPECT_EQ(0, g_Git.ApplyPatchToIndexReverse(static_cast<LPCWSTR>(StagingOperations::WritePatchBufferToTemporaryFile(tempbuf)), &out));
		EXPECT_STREQ(L"", out);
	}

	// Stage the + line alone, with or without context lines below it
	{
		auto buf = ReadResourcePatchToBuffer(L"\\patches\\newline-at-end-of-file-other-line-modified.patch");
		const int numLines = 11;

		for (int i = 7; i <= 10; i++)
		{
			CDiffLinesForStaging base(buf.get(), numLines, 7, i);
			StagingOperations op(&base);
			auto tempbuf = op.CreatePatchBufferToStageOrUnstageSelectedLines(StagingType::StageLines);
			CDiffLinesForStaging temp(tempbuf.c_str(), 11, 0, 0); // the temporary patch should have 11 lines, 0 and 0 don't matter here
			ExpectLineRangeIncludedAsIs(base, temp, 0, 0, 4); // first 4 lines (file header)
			ExpectPositionLineCountsChanged(base, temp, 4, 4, 3, 4); // @@ -1,3 +1,3 @@  ->  @@ -1,3 +1,4 @@
			ExpectLineIncludedAsIs(base, temp, { {5, 5} });
			ExpectOldLineTurnedIntoContext(base, temp, { {6, 6} });
			ExpectLineRangeIncludedAsIs(base, temp, 7, 7, 4);

			CString out;
			EXPECT_EQ(0, g_Git.ApplyPatchToIndex(static_cast<LPCWSTR>(StagingOperations::WritePatchBufferToTemporaryFile(tempbuf)), &out));
			EXPECT_STREQ(L"", out);

			// Unstage it back
			EXPECT_EQ(0, g_Git.ApplyPatchToIndexReverse(static_cast<LPCWSTR>(StagingOperations::WritePatchBufferToTemporaryFile(tempbuf)), &out));
			EXPECT_STREQ(L"", out);
		}
	}
}
