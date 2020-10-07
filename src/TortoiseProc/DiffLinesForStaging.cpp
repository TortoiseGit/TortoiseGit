// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2016, 2019-2021 - TortoiseGit
// Copyright (C) 2007, 2009-2013 - TortoiseSVN

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
#include "DiffLinesForStaging.h"
#include <regex>

CDiffLinesForStaging::CDiffLinesForStaging(const char* text, int numLines, int firstLineSelected, int lastLineSelected)
{
	const char* ptr = text;
	size_t last_i = 0; // beginning of the last line processed
	int state = 0;
	int oldCount = 0, newCount = 0;
	bool fileWasAdded = false, fileWasDeleted = false;
	DiffLineTypes lastType = DiffLineTypes::DEFAULT; // type of the last line processed, necessary to handle "No newline at end of file" correctly

	for (size_t i = 0; ;)
	{
		int eol_len = 0;
		if (!ptr[i])
			break;
		if (ptr[i] == '\r' && ptr[i + 1] == '\n' ||
			ptr[i] == '\n' && ptr[i + 1] == '\r')
			eol_len = 2;
		else if (ptr[i] == '\r' || ptr[i] == '\n')
			eol_len = 1;
		else
		{
			++i;
			continue;
		}

		size_t linebuflen = i - last_i + eol_len + 1;
		auto line = std::make_unique<char[]>(linebuflen);
		strncpy_s(line.get(), linebuflen, text + last_i, linebuflen - 1);
		last_i = i + eol_len;
		i += eol_len;

		DiffLineTypes type = DiffLineTypes::DEFAULT;
		bool exitLoop = false;
		// This algorithm is based off CPatch::ParsePatchFile
		while (!exitLoop)
		{
			exitLoop = true;
			switch (state)
			{
			case 0:
				if (strncmp(line.get(), "diff ", 5) == 0)
				{
					type = DiffLineTypes::COMMAND;
					state = 1;
				}
				break;
			case 1:
				if (strncmp(line.get(), "--- ", 4) == 0)
				{
					type = DiffLineTypes::HEADER;
					state = 2;
				}
				else
					type = DiffLineTypes::COMMENT;
				break;
			case 2:
				if (strncmp(line.get(), "+++ ", 4) == 0)
				{
					type = DiffLineTypes::HEADER;
					state = 3;
				}
				break;
			case 3:
				if (strncmp(line.get(), "@@ ", 3) == 0)
				{
					if (GetOldAndNewLinesCountFromHunk(line.get(), &oldCount, &newCount, true))
					{
						type = DiffLineTypes::POSITION;
						fileWasAdded = oldCount == 0;
						fileWasDeleted = newCount == 0;
						state = 4;
					}
				}
				break;
			case 4:
				if (strncmp(line.get(), "+", 1) == 0)
				{
					--newCount;
					type = DiffLineTypes::ADDED;
				}
				else if (strncmp(line.get(), "-", 1) == 0)
				{
					--oldCount;
					type = DiffLineTypes::DELETED;
				}
				else if (strncmp(line.get(), " ", 1) == 0)
				{
					--oldCount;
					--newCount;
					type = DiffLineTypes::DEFAULT;
				}
				// Regardless of locales, a "\ No newline at end of file" will always begin with "\ " and 10 is a sane minimum length to look for
				else if (linebuflen - 1 >= 10 && strncmp(line.get(), "\\ ", 2) == 0)
				{
					if (fileWasAdded)
						type = DiffLineTypes::NO_NEWLINE_NEWFILE;
					else if (fileWasDeleted)
						type = DiffLineTypes::NO_NEWLINE_OLDFILE;
					else if (oldCount == 0 && newCount > 0)
						type = DiffLineTypes::NO_NEWLINE_OLDFILE;
					else if (newCount == 0 && oldCount > 0)
						type = DiffLineTypes::NO_NEWLINE_NEWFILE;
					else if (oldCount == 0 && newCount == 0)
					{
						if (lastType == DiffLineTypes::ADDED)
							type = DiffLineTypes::NO_NEWLINE_NEWFILE;
						else if (lastType == DiffLineTypes::DELETED)
							type = DiffLineTypes::NO_NEWLINE_OLDFILE;
						else if (lastType == DiffLineTypes::DEFAULT)
							type = DiffLineTypes::NO_NEWLINE_BOTHFILES;
					}
				}
				else if (strncmp(line.get(), "@@ ", 3) == 0)
				{
					state = 3;
					exitLoop = false;
				}
				else
				{
					state = 0;
					exitLoop = false;
				}
			} // switch (state)
		} // while (!exitLoop)
		lastType = type;
		m_linevec.emplace_back(std::move(line), linebuflen - 1, type);
	} // for (int i = 0; ;)
	m_linevec.emplace_back("", 0, DiffLineTypes::DEFAULT); // Scintilla considers an empty document to have 1 line, so add an extra line here
	VERIFY(m_linevec.size() == static_cast<size_t>(numLines));
	m_firstLineSelected = firstLineSelected;
	m_lastLineSelected = lastLineSelected;
}

int CDiffLinesForStaging::GetFirstLineNumberSelected() const
{
	return m_firstLineSelected;
}

int CDiffLinesForStaging::GetLastLineNumberSelected() const
{
	return m_lastLineSelected;
}

// Includes EOL characters of all lines
std::unique_ptr<char[]> CDiffLinesForStaging::GetFullTextOfSelectedLines() const
{
	return GetFullTextOfLineRange(GetFirstLineNumberSelected(), GetLastLineNumberSelected());
}

// Includes EOL characters of all lines
std::unique_ptr<char[]> CDiffLinesForStaging::GetFullTextOfLineRange(int startline, int endline) const
{
	if (endline < startline)
		return nullptr;
	int size = 0;
	for (int i = startline; i <= endline; ++i)
		size += m_linevec.at(i).size;
	std::unique_ptr<char[]> ret = std::make_unique<char[]>(size + 1);
	for (int i = startline; i <= endline; ++i)
		strcat_s(ret.get(), size + 1, m_linevec.at(i).sLine.get());
	return ret;
}

DiffLineTypes CDiffLinesForStaging::GetLineType(int line) const
{
	return m_linevec.at(line).type;
}

bool CDiffLinesForStaging::IsNoNewlineComment(int line) const
{
	auto type = m_linevec.at(line).type;
	return type == DiffLineTypes::NO_NEWLINE_OLDFILE || type == DiffLineTypes::NO_NEWLINE_NEWFILE || type == DiffLineTypes::NO_NEWLINE_BOTHFILES;
}

// Includes EOL characters
std::unique_ptr<char[]> CDiffLinesForStaging::GetFullLineByLineNumber(int line) const
{
	int size = m_linevec.at(line).size + 1;
	std::unique_ptr<char[]> ret = std::make_unique<char[]>(size);
	strcpy_s(ret.get(), size, m_linevec.at(line).sLine.get());
	return ret;
}

int CDiffLinesForStaging::GetLastDocumentLine() const
{
	return static_cast<int>(m_linevec.size() - 1);
}

int CDiffLinesForStaging::GetDocumentLength() const
{
	size_t linesCount = m_linevec.size();
	int documentLength = 0;
	for (size_t i = 0; i < linesCount; ++i)
		documentLength += m_linevec.at(i).size;
	return documentLength;
}

// Takes a buffer containing the first line of a hunk (@@xxxxxx@@)
// Parses it to extract its old lines count and new lines count and passes them back to the given oldCount and newCount.
// Returns true if the line matches the expected format, false otherwise (what should never happen).
// If allowSingleLine is false, returns false for hunks missing the start line number in one or both sides (e.g. @@ -x +y,z @@)
bool CDiffLinesForStaging::GetOldAndNewLinesCountFromHunk(const char* strHunkStart, int* oldCount, int* newCount, bool allowSingleLine)
{
	std::string pattern = allowSingleLine ? "^@@ -(?:\\d+?,)?(\\d+?) \\+(?:\\d+?,)?(\\d+?) @@" : "^@@ -\\d+?,(\\d+?) \\+\\d+?,(\\d+?) @@";
	std::regex rx(pattern, std::regex_constants::ECMAScript);
	std::smatch match;

	std::string rmatch = std::string(static_cast<LPCSTR>(strHunkStart));
	if (!std::regex_search(rmatch, match, rx) || match.size() != 3) // this should never happen
		return false;
	*oldCount = StrToIntA(match[1].str().c_str());
	*newCount = StrToIntA(match[2].str().c_str());
	return true;
}
