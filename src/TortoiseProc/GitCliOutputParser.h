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

#pragma once

struct EmittedLines
{
	std::string text;
	size_t erasePreviousLineWithLength = 0;
	bool limited = false;
};

class CGitCliOutputParser
{
public:
	CGitCliOutputParser() = default;
	CGitCliOutputParser(size_t limit)
		: m_limit(limit)
	{
	};
	CGitCliOutputParser(const CGitCliOutputParser&) = delete;
	CGitCliOutputParser& operator=(const CGitCliOutputParser&) = delete;

	EmittedLines Process(const std::string_view buffer);
	void Reset();

private:
	size_t m_limit = SIZE_T_MAX; // this is not intended as an exact limit, but a safeguard against memory exhaustion

	std::string m_currentLine;
	std::string m_pendingCR;

	bool m_pendingVisible = false;
	bool m_skipNextEmptyRemoteLF = false;

	void EraseVisiblePendingIfNeeded(EmittedLines& out);

	void DiscardPendingCR(EmittedLines& out);

	void MakePendingPermanent(EmittedLines& out);

	void HandleLine(EmittedLines& out, const std::string& line, bool endedWithCR);

	void AppendLine(EmittedLines& out, const std::string& line) const;
};
