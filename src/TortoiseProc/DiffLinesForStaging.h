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
#pragma once

enum class DiffLineTypes
{
	ADDED,
	COMMAND,
	COMMENT,
	DEFAULT,
	DELETED,
	HEADER,
	NO_NEWLINE_OLDFILE, // "\ No newline at end of file"
	NO_NEWLINE_NEWFILE,
	NO_NEWLINE_BOTHFILES,
	POSITION
};

struct DiffLineForStaging
{
	DiffLineForStaging(std::string_view line, DiffLineTypes theType)
		: sLine(line)
		, type(theType)
	{
	}

	std::string_view sLine; // Includes EOL
	DiffLineTypes type;
};

// Stores a copy of a patch loaded in Patch View Dialog, as a vector of lines (which also stores the type of each line).
// Also stores information about the selection made by the user. Handles line text and line type retrieval.
// Intended usage:
// When the user invokes the partial staging/unstaging functionality in the Patch View Dialog, it creates an instance
// of this class and then pass the instance to StagingOperations, which will handle the staging/unstaging operations.
class CDiffLinesForStaging
{
private:
	std::vector<DiffLineForStaging> m_linevec;
	int m_firstLineSelected;
	int m_lastLineSelected;

public:
	CDiffLinesForStaging(const char* text, int numLines, int firstLineSelected, int lastLineSelected);

	int GetFirstLineNumberSelected() const;
	int GetLastLineNumberSelected() const;
	std::string_view GetFullLineByLineNumber(int line) const;
	std::string GetFullTextOfSelectedLines() const;
	std::string GetFullTextOfLineRange(int startline, int endline) const;
	int GetLastDocumentLine() const;
	DiffLineTypes GetLineType(int line) const;
	bool IsNoNewlineComment(int line) const;

	static bool GetOldAndNewLinesCountFromHunk(std::string_view hunk, int* oldCount, int* newCount, bool allowSingleLine = false);

#ifdef GOOGLETEST_INCLUDE_GTEST_GTEST_H_
public:
	const auto& GetLineVec() const { return m_linevec; };
#endif
};
