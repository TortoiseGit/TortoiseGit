// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2013, 2016, 2020 - TortoiseGit
// Copyright (C) 2003-2007 - TortoiseSVN

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

#include <string>

/**
 * \ingroup Utils
 * Class to convert strings from/to UTF8 and UTF16.
 */
class CUnicodeUtils
{
public:
	CUnicodeUtils() = delete;
#if defined(_MFC_VER) || defined(CSTRING_AVAILABLE)
	static inline CStringA GetUTF8(const CStringW& string) { return GetMulti(string, CP_UTF8); }
	static CStringA GetMulti(const CStringW& string, int acp);
	static inline CString GetUnicode(const CStringA& string, int acp = CP_UTF8) { return GetUnicodeLength(string, string.GetLength(), acp); };
	static inline CString GetUnicode(const char* string, int acp = CP_UTF8) { return GetUnicodeLength(string, static_cast<int>(strlen(string)), acp); };
	static CString GetUnicodeLength(const char* string, int len, int acp = CP_UTF8);
	static int GetCPCode(const CString & codename);
#endif

	static std::string StdGetUTF8(const std::wstring& wide);
	static std::wstring StdGetUnicode(const std::string& multibyte);
};

/* only used in TortoiseGitShell\ContextMenu.h */
std::string WideToMultibyte(const std::wstring& wide);
std::wstring MultibyteToWide(const std::string& multibyte);

int LoadStringEx(HINSTANCE hInstance, UINT uID, LPTSTR lpBuffer, int nBufferMax, WORD wLanguage);
