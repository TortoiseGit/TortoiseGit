// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN

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

#if defined(_MFC_VER)
// CSTRING is always available in an MFC build
#define CSTRING_AVAILABLE
#endif

/**
 * \ingroup Utils
 * helper class to handle path strings.
 */
class CPathUtils
{
public:
	static BOOL			MakeSureDirectoryPathExists(LPCTSTR path);
	static void			ConvertToBackslash(LPTSTR dest, LPCTSTR src, size_t len);
	/**
	 * Replaces escaped sequences with the corresponding characters in a string.
	 */
	static void Unescape(char * psz);

	/**
	 * Replaces non-URI chars with the corresponding escape sequences.
	 */
	static CStringA PathEscape(const CStringA& path);


#ifdef CSTRING_AVAILABLE
	/**
	 * returns the filename of a full path
	 */
	static CString GetFileNameFromPath(CString sPath);

	/**
	 * returns the file extension from a full path
	 */
	static CString GetFileExtFromPath(const CString& sPath);

	/**
	 * Returns the long pathname of a path which may be in 8.3 format.
	 */
	static CString GetLongPathname(const CString& path);

	/**
	 * Copies a file or a folder from \a srcPath to \a destpath, creating
	 * intermediate folders if necessary. If \a force is TRUE, then files
	 * are overwritten if they already exist.
	 * Folders are just created at the new location, no files in them get
	 * copied.
	 */
	static BOOL FileCopy(CString srcPath, CString destPath, BOOL force = TRUE);

	/**
	 * parses a string for a path or url. If no path or url is found,
	 * an empty string is returned.
	 * \remark if more than one path or url is inside the string, only
	 * the first one is returned.
	 */
	static CString ParsePathInString(const CString& Str);

	/**
	 * Returns the path to the installation folder, in our case the TortoiseSVN/bin folder.
	 * \remark the path returned has a trailing backslash
	 */
	static CString GetAppDirectory(HMODULE hMod = NULL);

	/**
	 * Returns the path to the installation parent folder, in our case the TortoiseSVN folder.
	 * \remark the path returned has a trailing backslash
	 */
	static CString GetAppParentDirectory(HMODULE hMod = NULL);

	/**
	 * Returns the path to the application data folder, in our case the %APPDATA%TortoiseSVN folder.
	 * \remark the path returned has a trailing backslash
	 */
	static CString GetAppDataDirectory();

	/**
	 * Replaces escaped sequences with the corresponding characters in a string.
	 */
	static CStringA PathUnescape(const CStringA& path);
	static CStringW PathUnescape(const CStringW& path);

	/**
	* Escapes regexp-specific chars.
	*/
	static CString PathPatternEscape(const CString& path);
	/**
	 * Unescapes regexp-specific chars.
	 */
	static CString PathPatternUnEscape(const CString& path);

	/**
	 * Returns the version string from the VERSION resource of a dll or exe.
	 * \param p_strDateiname path to the dll or exe
	 * \return the version string
	 */
	static CString GetVersionFromFile(const CString & p_strDateiname);


#endif
};
