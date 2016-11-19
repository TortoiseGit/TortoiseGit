// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2010 - TortoiseSVN
// Copyright (C) 2015-2016 - TortoiseGit

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

#ifdef UNICODE
#define wcswildcmp wcswildcmp
#else
#define wcswildcmp strwildcmp
#endif

/**
 * \ingroup Utils
 * Performs a wild card compare of two strings.
 * \param wild the wild card string
 * \param string the string to compare the wild card to
 * \return TRUE if the wild card matches the string, 0 otherwise
 * \par example
 * \code
 * if (strwildcmp("bl?hblah.*", "bliblah.jpeg"))
 *  printf("success\n");
 * else
 *  printf("not found\n");
 * if (strwildcmp("bl?hblah.*", "blabblah.jpeg"))
 *  printf("success\n");
 * else
 *  printf("not found\n");
 * \endcode
 * The output of the above code would be:
 * \code
 * success
 * not found
 * \endcode
 */
int strwildcmp(const char * wild, const char * string);
int wcswildcmp(const wchar_t * wild, const wchar_t * string);


/**
 * \ingroup Utils
 * string helper functions
 */
class CStringUtils
{
public:
	CStringUtils() = delete;
#ifdef _MFC_VER

	/**
	 * Removes all '&' chars from a string.
	 */
	static void RemoveAccelerators(CString& text);

	/**
	 * Returns the accellerator used in the string or \0
	 */
	static TCHAR GetAccellerator(const CString& text);

	/**
	 * Writes an ASCII CString to the clipboard in CF_TEXT format
	 */
	static bool WriteAsciiStringToClipboard(const CStringA& sClipdata, LCID lcid, HWND hOwningWnd = nullptr);
	/**
	 * Writes a String to the clipboard in both CF_UNICODETEXT and CF_TEXT format
	 */
	static bool WriteAsciiStringToClipboard(const CStringW& sClipdata, HWND hOwningWnd = nullptr);

	/**
	* Writes an ASCII CString to the clipboard in TGIT_UNIFIEDDIFF format, which is basically the patch file
	* as a ASCII string.
	*/
	static bool WriteDiffToClipboard(const CStringA& sClipdata, HWND hOwningWnd = nullptr);

	/**
	 * Reads the string \text from the file \path in utf8 encoding.
	 */
	static bool ReadStringFromTextFile(const CString& path, CString& text);

#endif
#if defined(CSTRING_AVAILABLE) || defined(_MFC_VER)
	static BOOL WildCardMatch(const CString& wildcard, const CString& string);
	static CString LinesWrap(const CString& longstring, int limit = 80, bool bCompactPaths = false);
	static CString WordWrap(const CString& longstring, int limit, bool bCompactPaths, bool bForceWrap, int tabSize);
	/**
	 * Find and return the number n of starting characters equal between
	 * \ref lhs and \ref rhs. (max n: lhs.Left(n) == rhs.Left(n))
	 */
	static int GetMatchingLength (const CString& lhs, const CString& rhs);

	/**
	 * Optimizing wrapper around CompareNoCase.
	 */
	static int FastCompareNoCase (const CStringW& lhs, const CStringW& rhs);

	static void ParseEmailAddress(CString mailaddress, CString& parsedAddress, CString* parsedName = nullptr);

	static bool IsPlainReadableASCII(const CString& text);

	static bool StartsWith(const wchar_t* heystack, const CString& needle);
	static bool StartsWithI(const wchar_t* heystack, const CString& needle);
	static bool WriteStringToTextFile(LPCTSTR path, LPCTSTR text, bool bUTF8 = true);
	static bool EndsWith(const CString& heystack, const wchar_t* needle);
	static bool EndsWith(const CString& heystack, const wchar_t needle);
	static bool EndsWithI(const CString& heystack, const wchar_t* needle);
#endif
	static bool StartsWith(const wchar_t* heystack, const wchar_t* needle);
	static bool StartsWith(const char* heystack, const char* needle);

	/**
	 * Writes the string \text to the file \path, either in utf16 or utf8 encoding,
	 * depending on the \c bUTF8 param.
	 */
	static bool WriteStringToTextFile(const std::wstring& path, const std::wstring& text, bool bUTF8 = true);

	/**
	 * Replace all pipe (|) character in the string with a nullptr character. Used
	 * for passing into Win32 functions that require such representation
	 */
	static void PipesToNulls(TCHAR* buffer, size_t length);
	static void PipesToNulls(TCHAR* buffer);
};

