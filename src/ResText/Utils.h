// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2007, 2012, 2015 - TortoiseSVN

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
#pragma once
#include <string>
#include <algorithm>
#include <functional>

/**
 * \ingroup ResText
 * static helper methods for ResText.
 */
class CUtils
{
public:
	CUtils(void);
	~CUtils(void);
	static void StringExtend(LPTSTR str);
	static void StringCollapse(LPTSTR str);
	static void Error();
	static void SearchReplace(std::wstring& str, const std::wstring& toreplace, const std::wstring& replacewith);
};


// trim from start
inline std::string &ltrim(std::string &s)
{
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(isspace))));
	return s;
}

// trim from end
inline std::string &rtrim(std::string &s)
{
	s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(isspace))).base(), s.end());
	return s;
}

// trim from both ends
inline std::string &trim(std::string &s)
{
	return ltrim(rtrim(s));
}

// trim from start
inline std::wstring &ltrim(std::wstring &s)
{
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<wint_t, int>(iswspace))));
	return s;
}

// trim from end
inline std::wstring &rtrim(std::wstring &s)
{
	s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<wint_t, int>(iswspace))).base(), s.end());
	return s;
}

// trim from both ends
inline std::wstring &trim(std::wstring &s)
{
	return ltrim(rtrim(s));
}
