// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2016-2018 - TortoiseGit
// Copyright (C) 2003-2008, 2013-2014 - TortoiseSVN

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
	CPathUtils() = delete;
	static BOOL			MakeSureDirectoryPathExists(LPCTSTR path);
	static void			ConvertToBackslash(LPTSTR dest, LPCTSTR src, size_t len);

#ifdef CSTRING_AVAILABLE
	inline static void	ConvertToBackslash(CString& path);

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
	static CString GetAppDirectory(HMODULE hMod = nullptr);

	/**
	 * Returns the path to the installation parent folder, in our case the TortoiseSVN folder.
	 * \remark the path returned has a trailing backslash
	 */
	static CString GetAppParentDirectory(HMODULE hMod = nullptr);

	static CString GetDocumentsDirectory();
	static CString GetProgramsDirectory();

	/**
	 * Returns the path to the application data folder, in our case the %APPDATA%TortoiseSVN folder.
	 * \remark the path returned has a trailing backslash
	 */
	static CString GetAppDataDirectory();
	static CString GetLocalAppDataDirectory();

	/**
	 * Removes any of the following namespace prefixes from a path, if found: "\??\", "\\?\", "\\?\UNC\".
	 */
	static void DropPathPrefixes(CString& path);

	static int ReadLink(LPCTSTR filename, CStringA* target = nullptr);

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
	 * \param p_strFilename path to the dll or exe
	 * \return the version string
	 */
	static CString GetVersionFromFile(const CString & p_strFilename);

	/**
	 * Ensures that the path ends with a folder separator.
	 * If the delimiter already exists, no additional delimiter will be added.
	 * \param path to ensure
	 */
	static void EnsureTrailingPathDelimiter(CString& path);

	/**
	 * Returns a path guaranteeing that a valid path delimiter follows.
	 * If the delimiter already exists, no additional delimiter will be added.
	 * \param path to ensure
	 * \return path including path delimiter
	 */
	static CString BuildPathWithPathDelimiter(const CString& path);

	/**
	 * Trims a possible included trailing folder separator from the provided path.
	 * \param path to trim
	 */
	static void TrimTrailingPathDelimiter(CString& path);

	/**
	 * ExpandFileName converts the relative file name into a fully qualified path name.
	 * ExpandFileName does not verify that the resulting fully qualified path name
	 * refers to an existing file, or even that the resulting path exists.
	 * \param path to expand
	 * \return fully qualified path name
	 */
	static CString CPathUtils::ExpandFileName(const CString& path);

	/**
	 * This method will make a path comparable to another path.
	 * It will do the following:
	 * 1.) Modify all characters in the path to be lower case
	 * 2.) Account for ..\'s and .\'s that may occur in the middle of the path and remove them
	 * 3.) Expand a path that has DOS 8.3 file/folder names
	 * 4.) Remove the trailing path delimiter at the end
	 * The function does not account for symlinks at this point in time.
	 * \param path to normalize
	 * \return normalized path
	 */
	static CString CPathUtils::NormalizePath(const CString& path);

	/**
	 * Compares two paths and returns true if they are logically the same path.
	 * The function does not account for symlinks at this point in time.
	 * \param path1 to compare
	 * \param path2 to compare
	 * \return true if they are the same path
	 */
	static bool CPathUtils::IsSamePath(const CString& path1, const CString& path2);

	/**
	 * Checks if two path strings are equal. No conversion of slashes is done!
	 * \remark for slash-independent comparison, use IsEquivalentTo()
	 */
	static bool ArePathStringsEqual(const CString& sP1, const CString& sP2);
	static bool ArePathStringsEqualWithCase(const CString& sP1, const CString& sP2);

	static CString GetCopyrightForSelf();

	/**
	 * Sets the last-write-time of the file to the current time
	 */
	static bool Touch(const CString& path);
#endif
};
