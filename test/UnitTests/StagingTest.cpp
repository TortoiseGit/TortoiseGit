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
	EXPECT_EQ(op.CreatePatchBufferToStageOrUnstageSelectedHunks(), nullptr);
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
	EXPECT_EQ(op.CreatePatchBufferToStageOrUnstageSelectedLines(StagingType::StageLines), nullptr);
}

inline void TestLineUnstagingNullptr(const char* buf, int numLines, int firstLineSelected, int lastLineSelected)
{
	CDiffLinesForStaging lines(buf, numLines, firstLineSelected, lastLineSelected);
	StagingOperations op(&lines);
	EXPECT_EQ(op.CreatePatchBufferToStageOrUnstageSelectedLines(StagingType::UnstageLines), nullptr);
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
		EXPECT_EQ(strcmp(base.GetLineVec()[match.first].sLine.get(), temp.GetLineVec()[match.second].sLine.get()), 0); // lines are identical?
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
		EXPECT_EQ(base.GetLineVec()[match.first].size, temp.GetLineVec()[match.second].size);
		EXPECT_TRUE(base.GetLineVec()[match.first].size >= 2);
		// lines are identical ignoring the first character?
		EXPECT_EQ(strcmp(base.GetLineVec()[match.first].sLine.get() + 1, temp.GetLineVec()[match.second].sLine.get() + 1), 0);
		EXPECT_EQ(base.GetLineVec()[match.first].type, DiffLineTypes::ADDED);
		EXPECT_EQ(temp.GetLineVec()[match.second].type, DiffLineTypes::DEFAULT);
	}
}

inline void ExpectOldLineTurnedIntoContext(const CDiffLinesForStaging& base, const CDiffLinesForStaging& temp, const std::map<int, int>& linemap)
{
	for (auto match : linemap)
	{
		EXPECT_EQ(base.GetLineVec()[match.first].size, temp.GetLineVec()[match.second].size);
		EXPECT_TRUE(base.GetLineVec()[match.first].size >= 2);
		// lines are identical ignoring the first character?
		EXPECT_EQ(strcmp(base.GetLineVec()[match.first].sLine.get() + 1, temp.GetLineVec()[match.second].sLine.get() + 1), 0);
		EXPECT_EQ(base.GetLineVec()[match.first].type, DiffLineTypes::DELETED);
		EXPECT_EQ(temp.GetLineVec()[match.second].type, DiffLineTypes::DEFAULT);
	}
}

inline void ExpectPositionLineCountsChanged(const CDiffLinesForStaging& base, const CDiffLinesForStaging& temp,
	int basePositionLine, int tempPositionLine, int new_oldCount, int new_newCount)
{
	EXPECT_EQ(temp.GetLineVec()[tempPositionLine].type, DiffLineTypes::POSITION);
	StagingOperations op(&base);
	auto changedbuf = op.ChangeOldAndNewLinesCount(base.GetLineVec()[basePositionLine].sLine.get(), new_oldCount, new_newCount);
	EXPECT_EQ(strcmp(changedbuf.get(), temp.GetLineVec()[tempPositionLine].sLine.get()), 0);
}

TEST(StagingOperations, PatchBuffer)
{
	auto buf = ReadResourcePatchToBuffer(L"\\patches\\partial-staging.patch");
	const int numLines = 105;

	// nothing useful selected (file header)
	for (int i = 0; i <= 3; ++i)
		for (int j = i; j <= 3; ++j)
			TestHunkStagingNullptr(buf.get(), numLines, i, j);

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
		for (int j = i; j <= 7; ++j)
			TestLineStagingNullptr(buf.get(), numLines, i, j);

	for (int i = 10; i <= 14; ++i)
		for (int j = i; j <= 14; ++j)
			TestLineStagingNullptr(buf.get(), numLines, i, j);

	for (int i = 101; i <= 104; ++i)
		for (int j = i; j <= 104; ++j)
			TestLineStagingNullptr(buf.get(), numLines, i, j);

	// nothing useful selected (only context lines from neighboring hunks)
	for (int i = 30; i <= 36; ++i)
		for (int j = i; j <= 36; ++j)
			TestLineStagingNullptr(buf.get(), numLines, i, j);

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
	CDiffLinesForStaging temp(tempbuf.get(), 23, 0, 0); // the temporary patch should have 23 lines, 0 and 0 don't matter here
	ExpectLineRangeIncludedAsIs(base, temp, 0, 0, 15); // first 15 lines
	ExpectOldLineTurnedIntoContext(base, temp, { {15,15},{19,18} });
	ExpectLineIncludedAsIs(base, temp, { {17,16},{18,17},{21,19},{22,20},{23,21},{104,22} });

	// Two hunks at the end of the file, both with at least one - or + line selected
	base = CDiffLinesForStaging(buf.get(), numLines, 84, 99); // user selected lines 85 to 100 and clicked line staging
	op = StagingOperations(&base);
	tempbuf = op.CreatePatchBufferToStageOrUnstageSelectedLines(StagingType::StageLines);
	temp = CDiffLinesForStaging(tempbuf.get(), 29, 0, 0); // the temporary patch should have 29 lines, 0 and 0 don't matter here
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
	temp = CDiffLinesForStaging(tempbuf.get(), 31, 0, 0); // the temporary patch should have 31 lines, 0 and 0 don't matter here
	ExpectLineRangeIncludedAsIs(base, temp, 0, 0, 4); // first 4 lines (file header)
	ExpectLineRangeIncludedAsIs(base, temp, 33, 4, 26);
	ExpectLineIncludedAsIs(base, temp, { {104,30} });

	// Two hunks with some of their modified lines selected
	base = CDiffLinesForStaging(buf.get(), numLines, 64, 74); // user selected lines 65 to 75 and clicked line unstaging
	op = StagingOperations(&base);
	tempbuf = op.CreatePatchBufferToStageOrUnstageSelectedLines(StagingType::UnstageLines);
	temp = CDiffLinesForStaging(tempbuf.get(), 24, 0, 0); // the temporary patch should have 24 lines, 0 and 0 don't matter here
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
		for (int j = i; j < numLines; ++j)
		{
			TestHunkStagingNullptr(buf.get(), numLines, i, j);
			TestLineStagingNullptr(buf.get(), numLines, i, j);
			TestLineUnstagingNullptr(buf.get(), numLines, i, j);
		}
}

TEST(StagingOperations, DeletedFile)
{
	auto buf = ReadResourcePatchToBuffer(L"\\patches\\git-diff.patch");
	const int numLines = 179;

	for (int i = 6; i < 21; ++i) // covers some of the lines regarding the deleted file and also part of the diff headers
		for (int j = i; j < 21; ++j)
		{
			TestHunkStagingNullptr(buf.get(), numLines, i, j);
			TestLineStagingNullptr(buf.get(), numLines, i, j);
			TestLineUnstagingNullptr(buf.get(), numLines, i, j);
		}
}

TEST(StagingOperations, AddedFile)
{
	auto buf = ReadResourcePatchToBuffer(L"\\patches\\git-diff.patch");
	const int numLines = 179;

	for (int i = 166; i < 179; ++i) // covers the lines regarding the added file and also its headers
		for (int j = i; j < 179; ++j)
		{
			TestHunkStagingNullptr(buf.get(), numLines, i, j);
			TestLineStagingNullptr(buf.get(), numLines, i, j);
			TestLineUnstagingNullptr(buf.get(), numLines, i, j);
		}
}
