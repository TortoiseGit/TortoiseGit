// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2011 - TortoiseGit
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
#include "gittype.h"

#define PARENT_MASK   0xFFFFFF
#define MERGE_MASK	(0x1000000)

class CTGitPath
{
public:
	CTGitPath(void);
	~CTGitPath(void);
	CTGitPath(const CString& sUnknownPath);
	int m_Stage;
	int m_ParentNo;
public:
#pragma warning(push)
#pragma warning(disable: 4480)	// nonstandard extension used: specifying underlying type for enum 'enum'
	enum : unsigned int
	{
		LOGACTIONS_ADDED	= 0x00000001,
		LOGACTIONS_MODIFIED	= 0x00000002,
		LOGACTIONS_REPLACED	= 0x00000004,
		LOGACTIONS_DELETED	= 0x00000008,
		LOGACTIONS_UNMERGED = 0x00000010,
		LOGACTIONS_CACHE	= 0x00000020,
		LOGACTIONS_COPY		= 0x00000040,
		LOGACTIONS_MERGED   = 0x00000080,
		LOGACTIONS_FORWORD  = 0x00000100,
		LOGACTIONS_UNVER	= 0x80000000,
		LOGACTIONS_IGNORE	= 0x40000000,
		//LOGACTIONS_CONFLICT = 0x20000000,

		// For log filter only
		LOGACTIONS_HIDE		= 0x20000000,
		LOGACTIONS_GRAY		= 0x10000000,

		// For Rebase only
		LOGACTIONS_REBASE_CURRENT = 0x08000000,
		LOGACTIONS_REBASE_PICK	  = 0x04000000,
		LOGACTIONS_REBASE_SQUASH  = 0x02000000,
		LOGACTIONS_REBASE_EDIT	  = 0x01000000,
		LOGACTIONS_REBASE_DONE	  = 0x00800000,
		LOGACTIONS_REBASE_SKIP    = 0x00400000,
		LOGACTIONS_REBASE_MASK	  = 0x0FC00000,
		LOGACTIONS_REBASE_MODE_MASK=0x07C00000,
	};
#pragma warning(pop)

	CString m_StatAdd;
	CString m_StatDel;
	unsigned int		m_Action;
	bool    m_Checked;
	int	ParserAction(BYTE action);
	CString GetActionName();
	static CString GetActionName(int action);
	/**
	 * Set the path as an UTF8 string with forward slashes
	 */
	void SetFromGit(const char* pPath);
	void SetFromGit(const char* pPath, bool bIsDirectory);
	void SetFromGit(const TCHAR* pPath, bool bIsDirectory);
	void SetFromGit(const CString& sPath,CString *OldPath=NULL);

	/**
	 * Set the path as UNICODE with backslashes
	 */
	void SetFromWin(LPCTSTR pPath);
	void SetFromWin(const CString& sPath);
	void SetFromWin(LPCTSTR pPath, bool bIsDirectory);
	void SetFromWin(const CString& sPath, bool bIsDirectory);
	/**
	 * Set the path from an unknown source.
	 */
	void SetFromUnknown(const CString& sPath);
	/**
	 * Returns the path in Windows format, i.e. with backslashes
	 */
	LPCTSTR GetWinPath() const;
	/**
	 * Returns the path in Windows format, i.e. with backslashes
	 */
	const CString& GetWinPathString() const;
	/**
	 * Returns the path with forward slashes.
	 */
	const CString& GetGitPathString() const;

	const CString& GetGitOldPathString() const;
	/**
	 * Returns the path completely prepared to be fed the the Git APIs
	 * It will be in UTF8, with URLs escaped, if necessary
	 */
//	const char* GetGitApiPath(apr_pool_t *pool) const;
	/**
	 * Returns the path for showing in an UI.
	 *
	 * URL's are returned with forward slashes, unescaped if necessary
	 * Paths are returned with backward slashes
	 */
	const CString& GetUIPathString() const;
	/**
	 * Checks if the path is an URL.
	 */
	bool IsUrl() const;
	/**
	 * Returns true if the path points to a directory
	 */
	bool IsDirectory() const;

	CTGitPath GetSubPath(const CTGitPath &root);

	/**
	 * Returns the directory. If the path points to a directory, then the path
	 * is returned unchanged. If the path points to a file, the path to the
	 * parent directory is returned.
	 */
	CTGitPath GetDirectory() const;
	/**
	* Returns the the directory which contains the item the path refers to.
	* If the path is a directory, then this returns the directory above it.
	* If the path is to a file, then this returns the directory which contains the path
	* parent directory is returned.
	*/
	CTGitPath GetContainingDirectory() const;
	/**
	 * Get the 'root path' (e.g. "c:\") - Used to pass to GetDriveType
	 */
	CString GetRootPathString() const;
	/**
	 * Returns the filename part of the full path.
	 * \remark don't call this for directories.
	 */
	CString GetFilename() const;
	CString GetBaseFilename() const;
	/**
	 * Returns the item's name without the full path.
	 */
	CString GetFileOrDirectoryName() const;
	/**
	 * Returns the item's name without the full path, unescaped if necessary.
	 */
	CString GetUIFileOrDirectoryName() const;
	/**
	 * Returns the file extension, including the dot.
	 * \remark Returns an empty string for directories
	 */
	CString GetFileExtension() const;

	bool IsEmpty() const;
	void Reset();
	/**
	 * Checks if two paths are equal. The slashes are taken care of.
	 */
	bool IsEquivalentTo(const CTGitPath& rhs) const;
	bool IsEquivalentToWithoutCase(const CTGitPath& rhs) const;
	bool operator==(const CTGitPath& x) const {return IsEquivalentTo(x);}

	/**
	 * Checks if \c possibleDescendant is a child of this path.
	 */
	bool IsAncestorOf(const CTGitPath& possibleDescendant) const;
	/**
	 * Get a string representing the file path, optionally with a base
	 * section stripped off the front
	 * Returns a string with fwdslash paths
	 */
	CString GetDisplayString(const CTGitPath* pOptionalBasePath = NULL) const;
	/**
	 * Compares two paths. Slash format is irrelevant.
	 */
	static int Compare(const CTGitPath& left, const CTGitPath& right);

	/** As PredLeftLessThanRight, but for checking if paths are equivalent
	 */
	static bool PredLeftEquivalentToRight(const CTGitPath& left, const CTGitPath& right);

	/** Checks if the left path is pointing to the same working copy path as the right.
	 * The same wc path means the paths are equivalent once all the admin dir path parts
	 * are removed. This is used in the TGitCache crawler to filter out all the 'duplicate'
	 * paths to crawl.
	 */
	static bool PredLeftSameWCPathAsRight(const CTGitPath& left, const CTGitPath& right);

	static bool CheckChild(const CTGitPath &parent, const CTGitPath& child);

	/**
	 * appends a string to this path.
	 *\remark - missing slashes are not added - this is just a string concatenation, but with
	 * preservation of the proper caching behavior.
	 * If you want to join a file- or directory-name onto the path, you should use AppendPathString
	 */
	void AppendRawString(const CString& sAppend);

	/**
	* appends a part of a path to this path.
	*\remark - missing slashes are dealt with properly. Don't use this to append a file extension, for example
	*
	*/
	void AppendPathString(const CString& sAppend);

	/**
	 * Get the file modification time - returns zero for files which don't exist
	 * Returns a FILETIME structure cast to an __int64, for easy comparisons
	 */
	__int64 GetLastWriteTime() const;

	bool IsReadOnly() const;

	/**
	 * Checks if the path really exists.
	 */
	bool Exists() const;

	/**
	 * Deletes the file/folder
	 * \param bTrash if true, uses the Windows trash bin when deleting.
	 */
	bool Delete(bool bTrash) const;

	/**
	 * Checks if a Subversion admin directory is present. For files, the check
	 * is done in the same directory. For folders, it checks if the folder itself
	 * contains an admin directory.
	 */
	bool HasAdminDir() const;
	bool HasAdminDir(CString *ProjectTopDir) const;
	bool HasSubmodules() const;
	bool HasGitSVNDir() const;
	bool IsBisectActive() const;
	bool IsMergeActive() const;
	bool HasStashDir() const;
	bool HasRebaseApply() const;

	bool IsWCRoot() const;

	int  GetAdminDirMask() const;

	/**
	 * Checks if the path point to or below a Subversion admin directory (.Git).
	 */
	bool IsAdminDir() const;

	void SetCustomData(LPARAM lp) {m_customData = lp;}
	LPARAM GetCustomData() {return m_customData;}

	/**
	 * Checks if the path or URL is valid on Windows.
	 * A path is valid if conforms to the specs in the windows API.
	 * An URL is valid if the path checked out from it is valid
	 * on windows. That means an URL which is valid according to the WWW specs
	 * isn't necessarily valid as a windows path (e.g. http://myserver.com/repos/file:name
	 * is a valid URL, but the path is illegal on windows ("file:name" is illegal), so
	 * this function would return \c false for that URL).
	 */
	bool IsValidOnWindows() const;

	/**
	 * Checks to see if the path or URL represents one of the special directories
	 * (branches, tags, or trunk).
	 */
	bool IsSpecialDirectory() const;
private:
	// All these functions are const, and all the data
	// is mutable, in order that the hidden caching operations
	// can be carried out on a const CTGitPath object, which is what's
	// likely to be passed between functions
	// The public 'SetFromxxx' functions are not const, and so the proper
	// const-correctness semantics are preserved
	void SetFwdslashPath(const CString& sPath) const;
	void SetBackslashPath(const CString& sPath) const;
	void SetUTF8FwdslashPath(const CString& sPath) const;
	void EnsureBackslashPathSet() const;
	void EnsureFwdslashPathSet() const;
	/**
	 * Checks if two path strings are equal. No conversion of slashes is done!
	 * \remark for slash-independent comparison, use IsEquivalentTo()
	 */
	static bool ArePathStringsEqual(const CString& sP1, const CString& sP2);
	static bool ArePathStringsEqualWithCase(const CString& sP1, const CString& sP2);

	/**
	 * Adds the required trailing slash to local root paths such as 'C:'
	 */
	void SanitizeRootPath(CString& sPath, bool bIsForwardPath) const;

	void UpdateAttributes() const;



private:
	mutable CString m_sBackslashPath;
	mutable CString m_sFwdslashPath;
	mutable CString m_sUIPath;
	mutable	CStringA m_sUTF8FwdslashPath;
	mutable CStringA m_sUTF8FwdslashPathEscaped;
	mutable CString m_sProjectRoot;

	//used for rename case
	mutable CString m_sOldBackslashPath;
	mutable CString m_sOldFwdslashPath;

	// Have we yet determined if this is a directory or not?
	mutable bool m_bDirectoryKnown;
	mutable bool m_bIsDirectory;
	mutable bool m_bLastWriteTimeKnown;
	mutable bool m_bURLKnown;
	mutable bool m_bIsURL;
	mutable __int64 m_lastWriteTime;
	mutable bool m_bIsReadOnly;
	mutable bool m_bHasAdminDirKnown;
	mutable bool m_bHasAdminDir;
	mutable bool m_bIsValidOnWindowsKnown;
	mutable bool m_bIsValidOnWindows;
	mutable bool m_bIsAdminDirKnown;
	mutable bool m_bIsAdminDir;
	mutable bool m_bIsWCRootKnown;
	mutable bool m_bIsWCRoot;
	mutable bool m_bExists;
	mutable bool m_bExistsKnown;
	mutable LPARAM m_customData;
	mutable bool m_bIsSpecialDirectoryKnown;
	mutable bool m_bIsSpecialDirectory;

	friend bool operator<(const CTGitPath& left, const CTGitPath& right);
};
/**
 * Compares two paths and return true if left is earlier in sort order than right
 * (Uses CTGitPath::Compare logic, but is suitable for std::sort and similar)
 */
 bool operator<(const CTGitPath& left, const CTGitPath& right);


//////////////////////////////////////////////////////////////////////////

/**
 * \ingroup Utils
 * This class represents a list of paths
 */
class CTGitPathList
{
public:
	CTGitPathList();
	// A constructor which allows a path list to be easily built with one initial entry in
	explicit CTGitPathList(const CTGitPath& firstEntry);
	int m_Action;

public:
	void AddPath(const CTGitPath& newPath);
	bool LoadFromFile(const CTGitPath& filename);
	bool WriteToFile(const CString& sFilename, bool bANSI = false) const;
	CTGitPath * LookForGitPath(CString path);
	int	ParserFromLog(BYTE_VECTOR &log, bool parseDeletes = false);
	int ParserFromLsFile(BYTE_VECTOR &out,bool staged=true);
	int FillUnRev(unsigned int Action,CTGitPathList *list=NULL);
	int GetAction();
	/**
	 * Load from the path argument string, when the 'path' parameter is used
	 * This is a list of paths, with '*' between them
	 */
	void LoadFromAsteriskSeparatedString(const CString& sPathString);
	CString CreateAsteriskSeparatedString() const;

	int GetCount() const;
	void Clear();
	const CTGitPath& operator[](INT_PTR index) const;
	bool AreAllPathsFiles() const;
	bool AreAllPathsFilesInOneDirectory() const;
	CTGitPath GetCommonDirectory() const;
	CTGitPath GetCommonRoot() const;
	void SortByPathname(bool bReverse = false);
	/**
	 * Delete all the files in the list, then clear the list.
	 * \param bTrash if true, the items are deleted using the Windows trash bin
	 */
	void DeleteAllFiles(bool bTrash, bool bFilesOnly = true);
	static bool DeleteViaShell(LPCTSTR path, bool useTrashbin);
	/** Remove duplicate entries from the list (sorts the list as a side-effect */
	void RemoveDuplicates();
	/** Removes all paths which are on or in a Subversion admin directory */
	void RemoveAdminPaths();
	void RemovePath(const CTGitPath& path);
	void RemoveItem(CTGitPath &path);
	/**
	 * Removes all child items and leaves only the top folders. Useful if you
	 * create the list to remove them (i.e. if you remove a parent folder, the
	 * child files and folders don't have to be deleted anymore)
	 */
	void RemoveChildren();

	/** Checks if two CTGitPathLists are the same */
	bool IsEqual(const CTGitPathList& list);

	/** Convert into the Git API parameter format */
//	apr_array_header_t * MakePathArray (apr_pool_t *pool) const;

	typedef std::vector<CTGitPath> PathVector;
	PathVector m_paths;
	// If the list contains just files in one directory, then
	// this contains the directory name
	mutable CTGitPath m_commonBaseDirectory;
};
