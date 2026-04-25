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
#include "GitCliOutputParser.h"

constexpr std::string_view GIT_REMOTE_PREFIX = "remote: ";

static constexpr bool IsEmptyRemoteLine(const std::string& line)
{
	return line == GIT_REMOTE_PREFIX;
}

static constexpr bool IsRemoteLine(const std::string& line)
{
	return line.starts_with(GIT_REMOTE_PREFIX);
}

EmittedLines CGitCliOutputParser::Process(const std::string_view buffer)
{
	EmittedLines out;
	bool foundCr = false;
	for (const char ch : buffer)
	{
		if (ch == '\r' || ch == '\n')
		{
			HandleLine(out, m_currentLine, ch == '\r');
			if (ch == '\r')
				foundCr = true;
			m_currentLine.clear();
			if (out.limited)
				break; // do not add info on overall length limitation here as the richedit may be longer due to line breaks; break is enough as we do not get called any more later on
			continue;
		}

		m_currentLine.push_back(ch);
	}

	if (!m_pendingCR.empty() && foundCr)
	{
		// Do not make a skipped empty remote CR visible.
		// It is only a temporary placeholder after a previous remote: LF/CR sequence
		// and may still be replaced by the next real remote line.
		if (!(m_skipNextEmptyRemoteLF && IsEmptyRemoteLine(m_pendingCR)))
		{
			out.text.append(m_pendingCR);
			m_pendingVisible = true;
		}
	}

	return out;
}

void CGitCliOutputParser::Reset()
{
	m_currentLine.clear();
	m_pendingCR.clear();
	m_pendingVisible = false;
	m_skipNextEmptyRemoteLF = false;
}

void CGitCliOutputParser::AppendLine(EmittedLines& out, const std::string& line) const
{
	if (out.text.size() > m_limit)
	{
		out.limited = true;
		return;
	}
	out.text += line;
	out.text += '\n';
}

static std::string OverlayConsoleLine(const std::string& oldLine, const std::string& newLine)
{
	if (newLine.size() >= oldLine.size())
		return newLine;

	std::string result = oldLine;
	std::copy(newLine.begin(), newLine.end(), result.begin());
	return result;
}

void CGitCliOutputParser::EraseVisiblePendingIfNeeded(EmittedLines& out)
{
	if (!m_pendingVisible)
		return;

	out.erasePreviousLineWithLength = m_pendingCR.size();
	m_pendingVisible = false;
}

void CGitCliOutputParser::DiscardPendingCR(EmittedLines& out)
{
	EraseVisiblePendingIfNeeded(out);
	m_pendingCR.clear();
}

void CGitCliOutputParser::MakePendingPermanent(EmittedLines& out)
{
	if (m_pendingCR.empty())
		return;

	EraseVisiblePendingIfNeeded(out);
	AppendLine(out, m_pendingCR);
	m_pendingCR.clear();
}

void CGitCliOutputParser::HandleLine(EmittedLines& out, const std::string& line, bool endedWithCR)
{
	const bool remoteLine = IsRemoteLine(line);
	const bool pendingRemoteLine = IsRemoteLine(m_pendingCR);
	const bool emptyRemote = remoteLine && IsEmptyRemoteLine(line);

	if (endedWithCR)
	{
		if (m_pendingCR.empty())
		{
			m_pendingCR = line;
			return;
		}

		if (pendingRemoteLine && remoteLine)
		{
			const bool preserveSkip = m_skipNextEmptyRemoteLF && emptyRemote;
			if (!preserveSkip)
				m_skipNextEmptyRemoteLF = false;

			if (!emptyRemote)
				DiscardPendingCR(out);
			else if (!m_skipNextEmptyRemoteLF)
				MakePendingPermanent(out);

			m_pendingCR = line;

			return;
		}

		// Normal console CR behavior
		EraseVisiblePendingIfNeeded(out);
		m_pendingCR = OverlayConsoleLine(m_pendingCR, line);
		return;
	}

	// LF line without pending CR
	if (m_pendingCR.empty())
	{
		if (!m_skipNextEmptyRemoteLF || !emptyRemote)
			AppendLine(out, line);

		m_skipNextEmptyRemoteLF = false;
		return;
	}

	if (pendingRemoteLine && remoteLine)
	{
		m_skipNextEmptyRemoteLF = emptyRemote;

		if (emptyRemote && !IsEmptyRemoteLine(m_pendingCR))
		{
			MakePendingPermanent(out);
			return;
		}

		DiscardPendingCR(out);
		AppendLine(out, line);
		return;
	}

	// Normal console LF behavior after CR
	EraseVisiblePendingIfNeeded(out);
	if (out.text.size() > m_limit)
	{
		out.limited = true;
		m_pendingCR.clear();
		m_skipNextEmptyRemoteLF = false;
		return;
	}
	if (line.empty())
		out.text.append(m_pendingCR);
	else
		out.text.append(OverlayConsoleLine(m_pendingCR, line));

	if (!endedWithCR)
		out.text.push_back('\n');

	m_pendingCR.clear();
	m_skipNextEmptyRemoteLF = false;
}
