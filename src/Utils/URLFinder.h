// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2023 - TortoiseGit
// Copyright (C) 2003-2008, 2013, 2018, 2020 - TortoiseSVN

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

#include <Shlwapi.h>
#include "StringUtils.h"

template <typename T>
bool isalnumTmpl(T ch)
{
	if constexpr (std::is_same_v<T, wchar_t>)
		return iswalnum(ch);
	else
		return isalnum(ch);
}

template <typename T>
bool IsValidURLChar(T ch)
{
	static_assert('_' == L'_', "This method expects that char and wchar_t literals are comparable for ASCII characters");
	return isalnumTmpl(ch) ||
		ch == '_' || ch == '/' || ch == ';' || ch == '?' || ch == '&' || ch == '=' ||
		ch == '%' || ch == ':' || ch == '.' || ch == '#' || ch == '-' || ch == '+' ||
		ch == '|' || ch == '>' || ch == '<' || ch == '!' || ch == '@' || ch == '~';
}

template <typename T>
bool PathIsURLTmpl(const T& sText)
{
	if constexpr (std::is_same_v<T, CString>)
		return ::PathIsURLW(sText);
	else
		return ::PathIsURLA(sText);
}

template <typename T>
bool IsUrlOrEmail(const T& sText)
{
	if (!PathIsURLTmpl(sText))
	{
		auto atpos = sText.Find('@');
		if (atpos <= 0)
			return false;
		if (sText.Find('.', atpos) <= atpos + 1) // a dot must follow after the @, but not directly after it
			return false;
		if (sText.Find(':', atpos) < 0) // do not detect git@example.com:something as an email address
			return true;
		return false;
	}
	static const T prefixes[] = { T("http://"), T("https://"), T("git://"), T("ftp://"), T("file://"), T("mailto:") };
	for (auto& prefix : prefixes)
	{
		if (CStringUtils::StartsWith(sText, prefix) && sText.GetLength() != prefix.GetLength())
			return true;
	}
	return false;
}

template <typename T, typename AdvanceCharacterFunc, typename FoundRangeCallback>
void FindURLMatches(const T& msg, AdvanceCharacterFunc advanceCharacter, FoundRangeCallback callback)
{
	static_assert(std::is_convertible_v<AdvanceCharacterFunc, std::function<void(const T&, int&)>>, "Wrong signature for AdvanceCharacterFunc!");
	static_assert(std::is_convertible_v<FoundRangeCallback, std::function<void(int, int)>>, "Wrong signature for FoundRangeCallback!");
	const int len = msg.GetLength();
	int starturl = -1;

	for (int i = 0; i <= len; advanceCharacter(msg, i))
	{
		if ((i < len) && IsValidURLChar(msg[i]))
		{
			if (starturl < 0)
				starturl = i;
			continue;
		}

		if (starturl < 0)
			continue;

		bool strip = true;
		if (msg[starturl] == '<' && i < len) // try to detect and do not strip URLs put within <>
		{
			while (starturl <= i && msg[starturl] == '<') // strip leading '<'
				++starturl;
			strip = false;
			i = starturl;
			while (i < len && msg[i] != '\r' && msg[i] != '\n' && msg[i] != '>') // find first '>' or new line after resetting i to start position
				advanceCharacter(msg, i);
		}

		int skipTrailing = 0;
		while (strip && i - skipTrailing - 1 > starturl && (msg[i - skipTrailing - 1] == '.' || msg[i - skipTrailing - 1] == '-' || msg[i - skipTrailing - 1] == '?' || msg[i - skipTrailing - 1] == ';' || msg[i - skipTrailing - 1] == ':' || msg[i - skipTrailing - 1] == '>' || msg[i - skipTrailing - 1] == '<' || msg[i - skipTrailing - 1] == '!'))
			++skipTrailing;

		if (!IsUrlOrEmail(msg.Mid(starturl, i - starturl - skipTrailing)))
		{
			starturl = -1;
			continue;
		}

		callback(starturl, i - skipTrailing);

		starturl = -1;
	}
}
