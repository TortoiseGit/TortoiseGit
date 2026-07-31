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
#include <format>

constexpr size_t MAX_LINE_LENGTH = 8 * 1024; // 8 KiB
constexpr size_t MAX_BUFFER_SIZE = 150 * 1024 * 1024; // 150 MiB
constexpr std::string_view GIT_REMOTE_PREFIX = "remote: ";

static std::optional<COLORREF> GetAnsiColor(int index)
{
	static constexpr COLORREF colors[] = {
		RGB(0, 0, 0), RGB(205, 49, 49), RGB(13, 188, 121), RGB(229, 229, 16),
		RGB(36, 114, 200), RGB(188, 63, 188), RGB(17, 168, 205), RGB(229, 229, 229),
		RGB(102, 102, 102), RGB(241, 76, 76), RGB(35, 209, 139), RGB(245, 245, 67),
		RGB(59, 142, 234), RGB(214, 112, 214), RGB(41, 184, 219), RGB(255, 255, 255),
	};
	if (index < 0 || index > 255)
		return {};
	if (index < 16)
		return colors[index];
	if (index < 232)
	{
		index -= 16;
		const auto component = [](int value) { return value == 0 ? 0 : 55 + value * 40; };
		return RGB(component(index / 36), component(index / 6 % 6), component(index % 6));
	}
	const int gray = 8 + (index - 232) * 10;
	return RGB(gray, gray, gray);
}

void CAnsiEscapeParser::AppendChar(std::vector<AnsiTextRun>& runs, wchar_t ch) const
{
	if (runs.empty() || runs.back().style != m_style)
		runs.push_back({ {}, m_style });
	runs.back().text.AppendChar(ch);
}

void CAnsiEscapeParser::ApplySgr(const CString& parameters)
{
	std::vector<int> codes;
	int value = 0;
	bool hasValue = false;
	for (int i = 0; i < parameters.GetLength(); ++i)
	{
		const wchar_t ch = parameters[i];
		if (ch >= L'0' && ch <= L'9')
		{
			value = std::min(value * 10 + ch - L'0', 256);
			hasValue = true;
			continue;
		}
		if (ch != L';')
			return;
		codes.push_back(hasValue ? value : 0);
		value = 0;
		hasValue = false;
	}
	codes.push_back(hasValue ? value : 0);

	for (size_t i = 0; i < codes.size(); ++i)
	{
		const int code = codes[i];
		if (code == 0)
			m_style = {};
		else if (code == 1)
			m_style.bold = true;
		else if (code == 3)
			m_style.italic = true;
		else if (code == 4 || code == 21)
			m_style.underline = true;
		else if (code == 7)
			m_style.inverse = true;
		else if (code == 9)
			m_style.strikeout = true;
		else if (code == 22)
			m_style.bold = false;
		else if (code == 23)
			m_style.italic = false;
		else if (code == 24)
			m_style.underline = false;
		else if (code == 27)
			m_style.inverse = false;
		else if (code == 29)
			m_style.strikeout = false;
		else if (code >= 30 && code <= 37)
			m_style.foreground = GetAnsiColor(code - 30);
		else if (code == 39)
			m_style.foreground.reset();
		else if (code >= 40 && code <= 47)
			m_style.background = GetAnsiColor(code - 40);
		else if (code == 49)
			m_style.background.reset();
		else if (code >= 90 && code <= 97)
			m_style.foreground = GetAnsiColor(code - 90 + 8);
		else if (code >= 100 && code <= 107)
			m_style.background = GetAnsiColor(code - 100 + 8);
		else if ((code == 38 || code == 48) && i + 2 < codes.size() && codes[i + 1] == 5)
		{
			if (auto color = GetAnsiColor(codes[i + 2]))
				(code == 38 ? m_style.foreground : m_style.background) = color;
			i += 2;
		}
		else if ((code == 38 || code == 48) && i + 4 < codes.size() && codes[i + 1] == 2)
		{
			const int red = codes[i + 2];
			const int green = codes[i + 3];
			const int blue = codes[i + 4];
			if (red >= 0 && red <= 255 && green >= 0 && green <= 255 && blue >= 0 && blue <= 255)
				(code == 38 ? m_style.foreground : m_style.background) = RGB(red, green, blue);
			i += 4;
		}
	}
}

std::vector<AnsiTextRun> CAnsiEscapeParser::Parse(const CString& text)
{
	const CString input = m_pendingEscape + text;
	m_pendingEscape.Empty();
	std::vector<AnsiTextRun> runs;

	for (int i = 0; i < input.GetLength();)
	{
		if (input[i] != L'\033')
		{
			AppendChar(runs, input[i++]);
			continue;
		}

		if (i + 1 >= input.GetLength())
		{
			m_pendingEscape = input.Mid(i);
			break;
		}

		if (input[i + 1] == L'[')
		{
			int end = i + 2;
			while (end < input.GetLength() && input[end] >= 0x30 && input[end] <= 0x3f)
				++end;
			while (end < input.GetLength() && input[end] >= 0x20 && input[end] <= 0x2f)
				++end;
			if (end >= input.GetLength())
			{
				m_pendingEscape = input.Mid(i);
				break;
			}
			if (input[end] == L'm')
				ApplySgr(input.Mid(i + 2, end - i - 2));
			i = input[end] >= 0x40 && input[end] <= 0x7e ? end + 1 : end;
			continue;
		}

		if (input[i + 1] == L']')
		{
			int end = i + 2;
			while (end < input.GetLength() && input[end] != L'\a' && !(input[end] == L'\033' && end + 1 < input.GetLength() && input[end + 1] == L'\\'))
				++end;
			if (end >= input.GetLength())
			{
				m_pendingEscape = input.Mid(i);
				break;
			}
			i = end + (input[end] == L'\a' ? 1 : 2);
			continue;
		}

		int end = i + 1;
		while (end < input.GetLength() && input[end] >= 0x20 && input[end] <= 0x2f)
			++end;
		if (end >= input.GetLength())
		{
			m_pendingEscape = input.Mid(i);
			break;
		}
		i = input[end] >= 0x30 && input[end] <= 0x7e ? end + 1 : end;
	}
	return runs;
}

void CAnsiEscapeParser::Reset()
{
	m_pendingEscape.Empty();
	m_style = {};
}

static constexpr bool IsEmptyRemoteLine(const std::string& line)
{
	return line == GIT_REMOTE_PREFIX;
}

static constexpr bool IsRemoteLine(const std::string& line)
{
	return line.starts_with(GIT_REMOTE_PREFIX);
}

void CGitCliOutputParser::AppendChunk(const std::string_view chunk)
{
	if (m_dropMode)
		return;

	CComCritSecLock<CComCriticalSection> lock{ m_critSec };
	for (char ch : chunk)
	{
		if (ch == '\0')
			ch = '\n';

		if (ch == '\r' || ch == '\n')
		{
			m_inputBuffer.push_back(ch);
			m_inputBufferCurrentLineLength = 0;
			m_inputBufferSkippingTruncatedLine = false;
			continue;
		}
		if (m_inputBufferSkippingTruncatedLine)
			continue;
		if (m_inputBufferCurrentLineLength >= MAX_LINE_LENGTH)
		{
			m_inputBufferSkippingTruncatedLine = true;
			m_inputBuffer.append(std::format("... [line truncated at {} KiB]", MAX_LINE_LENGTH / 1024));
			continue;
		}
		++m_inputBufferCurrentLineLength;
		m_inputBuffer.push_back(ch);
	}

	if (m_inputBuffer.size() > MAX_BUFFER_SIZE) // just a safeguard with an arbitrary large limit, we should never get there as in another thread we're processing the buffer in short intervals
	{
		m_inputBuffer.append(std::format("\n\n[Buffer truncated at about {} MiB to prevent resource exhaustion]\n", MAX_BUFFER_SIZE / 1024 / 1024));
		m_dropMode = true;
	}
}

void CGitCliOutputParser::ActivateDropMode()
{
	CComCritSecLock<CComCriticalSection> lock{ m_critSec };
	m_dropMode = true;
	m_inputBuffer.clear();
}

EmittedLines CGitCliOutputParser::ProcessPending()
{
	std::string buffer;
	{
		CComCritSecLock<CComCriticalSection> lock{ m_critSec };
		if (m_inputBuffer.empty())
			return {};

		buffer.swap(m_inputBuffer);
	}
	return Process(buffer);
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
	m_inputBuffer.clear();
	m_inputBufferSkippingTruncatedLine = false;
	m_inputBufferCurrentLineLength = 0;
	m_dropMode = false;
	m_ansiEscapeParser.Reset();
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
