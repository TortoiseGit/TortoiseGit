// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2020-2021, 2023 - TortoiseGit

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
#include <stdafx.h>
#include "StagingOperations.h"
#include <regex>
#include "Git.h"

bool StagingOperations::IsWithinFileHeader(int line) const
{
	DiffLineTypes type = m_lines->GetLineType(line);
	if (m_lines->IsNoNewlineComment(line))
		return false;
	return (type == DiffLineTypes::COMMAND || type == DiffLineTypes::COMMENT || type == DiffLineTypes::HEADER);
}

// From (and including) given line, looks backwards for a hunk start line (@@xxxxxxxx@@),
// up to and including given topBoundaryLine. Returns -1 if no hunk start is found.
int StagingOperations::FindHunkStartBackwardsFrom(int line, int topBoundaryLine) const
{
	for (int i = line; i >= topBoundaryLine; --i)
	{
		if (m_lines->GetLineType(i) == DiffLineTypes::POSITION)
			return i;
	}
	return -1;
}

// From (and including) given line, looks forwards for a hunk start line (@@xxxxxxxx@@),
// up to and including given bottomBoundaryLine. Returns -1 if no hunk start is found.
int StagingOperations::FindHunkStartForwardsFrom(int line, int bottomBoundaryLine) const
{
	for (int i = line; i <= bottomBoundaryLine; ++i)
	{
		if (m_lines->GetLineType(i) == DiffLineTypes::POSITION)
			return i;
	}
	return -1;
}

// Try to find the last line within a hunk looking forwards from given line up to the end of the patch.
// Works by first looking backwards until a hunk start is found, up to and including given topBoundaryLine,
// then finds out the hunk's length from the counts in the hunk start pattern.
// Returns -1 if no hunk start is found.
// If the end of the patch is reached, returns the line number of the last line.
int StagingOperations::FindHunkEndForwardsFrom(int line, int topBoundaryLine) const
{
	int hunkStart = FindHunkStartBackwardsFrom(line, topBoundaryLine);
	if (hunkStart == -1)
		return -1;
	auto strHunkStart = m_lines->GetFullLineByLineNumber(hunkStart);

	int oldCount, newCount;
	if (!CDiffLinesForStaging::GetOldAndNewLinesCountFromHunk(strHunkStart, &oldCount, &newCount))
		return -1;

	return FindHunkEndGivenHunkStartAndCounts(hunkStart, oldCount, newCount);
}

// Given the line number of a hunk header and its old and new lines counts, iterates its lines
// to find out the line number of the last line within the hunk and returns it.
// If the end of the patch is reached, returns the line number of the last line.
// It will not work for files that were added or deleted, i.e., when either oldCount or
// newCount is 0, but that does not matter since the partial staging
// functionality is meant to be used only with *modified* files.
int StagingOperations::FindHunkEndGivenHunkStartAndCounts(int hunkStart, int oldCount, int newCount) const
{
	if (oldCount == 0 || newCount == 0)
		return -1; // Not applicable (file was added or deleted)
	int lastDocumentLine = m_lines->GetLastDocumentLine();
	int i = hunkStart + 1;
	for (; i <= lastDocumentLine; ++i)
	{
		DiffLineTypes type = m_lines->GetLineType(i);
		if (type == DiffLineTypes::DELETED)
			--oldCount;
		else if (type == DiffLineTypes::ADDED)
			--newCount;
		else if (type == DiffLineTypes::DEFAULT)
		{
			--oldCount;
			--newCount;
		}

		if (oldCount == 0 && newCount == 0)
		{
			if (i + 1 <= lastDocumentLine && m_lines->IsNoNewlineComment(i + 1))
				return i + 1;
			else
				return i;
		}
	}

	return -1; // corrupt diff
}

// From (and including) given line, looks backwards for a line that is neither a hunk header (@@xxxxx@@) nor a
// context/added/deleted line. From there, it goes on looking backwards for a "diff" line. A buffer is then returned
// containing those lines and any lines between them, including EOL characters. Returns nullptr if no such sequence is found.
std::string StagingOperations::FindFileHeaderBackwardsFrom(int line) const
{
	int i = line;
	for (; i > -1; --i)
	{
		DiffLineTypes type = m_lines->GetLineType(i);
		if (type != DiffLineTypes::POSITION && type != DiffLineTypes::DEFAULT && type != DiffLineTypes::ADDED && type != DiffLineTypes::DELETED && !m_lines->IsNoNewlineComment(i))
			break;
	}
	if (i == -1)
		return {};
	int fileHeaderLastLine = i;
	for (; i > -1; --i)
	{
		if (m_lines->GetLineType(i) == DiffLineTypes::COMMAND)
			break;
	}
	if (i == -1)
		return {};
	int fileHeaderFirstLine = i;
	return m_lines->GetFullTextOfLineRange(fileHeaderFirstLine, fileHeaderLastLine);
}

// According to the user selection, returns a buffer holding a temporary patch which must be written to a temporary
// file and applied to the index with git apply --cached (for staging) or git apply --cached -R (for unstaging).
// Even though this feature is intended for single-file diffs only, this code should also work for multi-file diffs.
std::string StagingOperations::CreatePatchBufferToStageOrUnstageSelectedHunks() const
{
	// Try to find a hunk backwards up to and including the first line in the patch
	int startline = FindHunkStartBackwardsFrom(m_lines->GetFirstLineNumberSelected(), 0);
	// If the selection starts before the first hunk in the patch, startline is now -1

	// If the selection starts within the headers between files, set startline = -1
	// so that the code below will go looking forwards instead
	if (IsWithinFileHeader(m_lines->GetFirstLineNumberSelected()))
		startline = -1;

	if (startline == -1)
	{
		// Try to find a hunk forwards up to and including the last line selected
		startline = FindHunkStartForwardsFrom(m_lines->GetFirstLineNumberSelected(), m_lines->GetLastLineNumberSelected());
		if (startline == -1)
			return {}; // No part of a hunk is selected, bail
	}
	int endline = FindHunkEndForwardsFrom(m_lines->GetLastLineNumberSelected(), startline);
	if (endline == -1)
		return {};

	if (endline <= startline) // this should never happen
		return {};
	auto hunksWithoutFirstFileHeader = m_lines->GetFullTextOfLineRange(startline, endline);
	auto firstFileHeader = FindFileHeaderBackwardsFrom(startline);

	std::string fullTempPatch;
	fullTempPatch.append(firstFileHeader);
	fullTempPatch.append(hunksWithoutFirstFileHeader);
	return fullTempPatch;
}

// According to the user selection, returns a buffer holding a temporary patch which must be written to a temporary
// file and applied to the index with git apply --cached (for staging) or git apply --cached -R (for unstaging).
// This will not work for multi-file diffs.
// This needs to take as parameter whether we're doing a staging or an unstaging, since the handling for those is different.
std::string StagingOperations::CreatePatchBufferToStageOrUnstageSelectedLines(StagingType stagingType) const
{
	// Try to find a hunk backwards up to and including the first line in the patch
	int firstHunkStartLine = FindHunkStartBackwardsFrom(m_lines->GetFirstLineNumberSelected(), 0);
	// If the selection starts before the first hunk in the patch, startline is now -1

	if (firstHunkStartLine == -1)
	{
		// Try to find a hunk forwards up to and including the last line selected
		firstHunkStartLine = FindHunkStartForwardsFrom(m_lines->GetFirstLineNumberSelected(), m_lines->GetLastLineNumberSelected());
		if (firstHunkStartLine == -1)
			return {}; // No part of a hunk is selected, bail
	}

	std::string fullTempPatch;
	std::string firstHunkWithoutStartLine;
	std::string lastHunkWithoutStartLine;

	auto strFirstHunkStartLine = m_lines->GetFullLineByLineNumber(firstHunkStartLine);
	int firstHunkOldCount, firstHunkNewCount;
	if (!CDiffLinesForStaging::GetOldAndNewLinesCountFromHunk(strFirstHunkStartLine, &firstHunkOldCount, &firstHunkNewCount))
		return {};
	int firstHunkLastLine = FindHunkEndGivenHunkStartAndCounts(firstHunkStartLine, firstHunkOldCount, firstHunkNewCount);
	if (firstHunkLastLine == -1)
		return {};

	int firstLineSelected = m_lines->GetFirstLineNumberSelected();
	int lastLineSelected = m_lines->GetLastLineNumberSelected();

	bool includeFirstHunkAtAll = ParseHunkOnEitherSelectionBoundary(firstHunkWithoutStartLine, firstHunkStartLine, firstHunkLastLine, firstLineSelected, lastLineSelected, &firstHunkOldCount, &firstHunkNewCount, stagingType);

	auto firstFileHeader = FindFileHeaderBackwardsFrom(firstHunkStartLine);
	fullTempPatch.append(firstFileHeader);

	// If no modified line is selected in the first (or the last) hunk, we must discard it entirely or else git would complain
	// about a corrupt patch (unless we passed --recount to git apply, but that could potentially cause other issues)
	if (includeFirstHunkAtAll)
	{
		auto strHunkStartLineChanged = ChangeOldAndNewLinesCount(std::string(strFirstHunkStartLine), firstHunkOldCount, firstHunkNewCount);

		fullTempPatch.append(strHunkStartLineChanged);
		fullTempPatch.append(firstHunkWithoutStartLine);
	}

	int lastHunkStartLine = FindHunkStartBackwardsFrom(lastLineSelected, firstHunkStartLine);// firstHunkLastLine + 1);
	if (lastHunkStartLine == -1)
		return {};

	// For line staging, we only support one file at a time, so we just assume the next hunk starts
	// at the next line from the end of the first hunk and don't bother looking for a file header again
	auto inBetweenLines = m_lines->GetFullTextOfLineRange(firstHunkLastLine + 1, lastHunkStartLine - 1);
	if (!inBetweenLines.empty())
		fullTempPatch.append(inBetweenLines);

	int lastHunkLastLine = FindHunkEndForwardsFrom(lastHunkStartLine, lastHunkStartLine);
	if (lastHunkLastLine == -1)
		return {};
	if (firstHunkStartLine == lastHunkStartLine)
	{
		if (includeFirstHunkAtAll)
			return fullTempPatch;
		return {};
	}

	auto strLastHunkStartLine = m_lines->GetFullLineByLineNumber(lastHunkStartLine);
	int lastHunkOldCount, lastHunkNewCount;
	if (!CDiffLinesForStaging::GetOldAndNewLinesCountFromHunk(strLastHunkStartLine, &lastHunkOldCount, &lastHunkNewCount))
		return {};

	bool includeLastHunkAtAll = ParseHunkOnEitherSelectionBoundary(lastHunkWithoutStartLine, lastHunkStartLine, lastHunkLastLine, firstLineSelected, lastLineSelected, &lastHunkOldCount, &lastHunkNewCount, stagingType);
	if (includeLastHunkAtAll)
	{
		auto strHunkStartLineChanged = ChangeOldAndNewLinesCount(std::string(strLastHunkStartLine), lastHunkOldCount, lastHunkNewCount);

		fullTempPatch.append(strHunkStartLineChanged);
		fullTempPatch.append(lastHunkWithoutStartLine);
	}

	if (!includeFirstHunkAtAll && inBetweenLines.empty() && !includeLastHunkAtAll)
		return {};

	return fullTempPatch;
}

// Takes a buffer containing the first line of a hunk (@@xxxxxx@@)
// Returns a new buffer with its old lines count and new lines count changed to the given ones.
std::string StagingOperations::ChangeOldAndNewLinesCount(const std::string& strHunkStart, int oldCount, int newCount) const
{
	std::string pattern = "^@@ -(\\d+?),(\\d+?) \\+(\\d+?),(\\d+?) @@";
	std::regex rx(pattern, std::regex_constants::ECMAScript);

	auto fmt = std::make_unique<char[]>(1024);
	sprintf_s(fmt.get(), 1024, "@@ -$1,%d +$3,%d @@", oldCount, newCount);
	return std::regex_replace(strHunkStart, rx, fmt.get());
}

// For line staging/unstaging.
// Writes to the given hunkWithoutStartLine all the lines of a hunk that need to be included in the temporary
// patch for staging/unstaging, according to the user selection. The starting @@xxxxx@@ is not written.
// The algorithm for determining which lines need to be included is (for staging):
// "-" lines outside the user selection are turned into context (" ") lines
// "+" lines outside the user selection are removed
// For unstaging, it's the same as above except that "-" and "+" are swapped:
// "-" lines outside the user selection are removed
// "+" lines outside the user selection are turned into context (" ") lines
// The given newCount and oldCount are modified accordingly.
// Returns true if at least one + or - line is within the user selection, false otherwise (meaning the hunk must be discarded entirely)
// This needs to take as parameter whether we're doing a staging or an unstaging, since the handling for those is different.
bool StagingOperations::ParseHunkOnEitherSelectionBoundary(std::string& hunkWithoutStartLine, int hunkStartLine, int hunkLastLine, int firstLineSelected, int lastLineSelected, int* oldCount, int* newCount, StagingType stagingType) const
{
	bool includeHunkAtAll = false;
	for (int i = hunkStartLine + 1; i <= hunkLastLine; ++i)
	{
		DiffLineTypes type = m_lines->GetLineType(i);
		auto strLine = m_lines->GetFullLineByLineNumber(i);
		if (type == DiffLineTypes::DEFAULT || type == DiffLineTypes::NO_NEWLINE_BOTHFILES)
			hunkWithoutStartLine.append(strLine);
		else if (type == DiffLineTypes::ADDED || type == DiffLineTypes::NO_NEWLINE_NEWFILE)
		{
			if (i < firstLineSelected || i > lastLineSelected) // outside the user selection
			{
				if (stagingType == StagingType::StageLines)
				{
					if (type == DiffLineTypes::ADDED) // hunk counts do not consider "\ No newline at end of file"
						--(*newCount);
				}
				else if (stagingType == StagingType::UnstageLines)
				{
					if (type == DiffLineTypes::ADDED) // hunk counts do not consider "\ No newline at end of file"
					{
						// Turn it into a context line
						hunkWithoutStartLine.append(" ");
						strLine.remove_prefix(1);
						++(*oldCount);
					}
					hunkWithoutStartLine.append(strLine);
				}
			}
			else
			{
				if (type == DiffLineTypes::ADDED)
					includeHunkAtAll = true;
				hunkWithoutStartLine.append(strLine);
			}
		}
		else if (type == DiffLineTypes::DELETED || type == DiffLineTypes::NO_NEWLINE_OLDFILE)
		{
			if (i < firstLineSelected || i > lastLineSelected) // outside the user selection
			{
				if (stagingType == StagingType::StageLines)
				{
					if (type == DiffLineTypes::DELETED) // hunk counts do not consider "\ No newline at end of file"
					{
						// Turn it into a context line
						hunkWithoutStartLine.append(" ");
						strLine.remove_prefix(1);
						++(*newCount);
					}
					hunkWithoutStartLine.append(strLine);
				}
				else if (stagingType == StagingType::UnstageLines)
				{
					if (type == DiffLineTypes::DELETED) // hunk counts do not consider "\ No newline at end of file"
						--(*oldCount);
				}
			}
			else
			{
				if (type == DiffLineTypes::DELETED)
					includeHunkAtAll = true;
				hunkWithoutStartLine.append(strLine);
			}
		}
	}
	return includeHunkAtAll;
}

// Creates a temporary file and writes to it the given buffer.
// Returns the path of the created file.
CString StagingOperations::WritePatchBufferToTemporaryFile(const std::string& data)
{
	CString tempFile = ::GetTempFile();
	if (tempFile.IsEmpty())
		return CString();

	FILE* fp = nullptr;
	_wfopen_s(&fp, tempFile, L"w+b");
	if (!fp)
		return CString();

	fwrite(data.c_str(), sizeof(char), data.length(), fp);
	fclose(fp);

	return tempFile;
}
