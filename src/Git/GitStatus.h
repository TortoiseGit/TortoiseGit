// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2012 - TortoiseGit

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

#ifdef _MFC_VER
//#	include "SVNPrompt.h"
#endif
#include "TGitPath.h"

class CGitFileName;

#pragma warning (push,1)
typedef std::basic_string<wchar_t> wide_string;
#ifdef UNICODE
#	define stdstring wide_string
#else
#	define stdstring std::string
#endif
#pragma warning (pop)

#include "TGitPath.h"
#include "GitHash.h"

typedef enum type_git_wc_status_kind
{
	git_wc_status_none,
	git_wc_status_unversioned,
	git_wc_status_ignored,
	git_wc_status_normal,
	git_wc_status_external,
	git_wc_status_incomplete,
	git_wc_status_missing,
	git_wc_status_deleted,
	git_wc_status_replaced,
	git_wc_status_modified,
	git_wc_status_merged,
	git_wc_status_added,
	git_wc_status_conflicted,
	git_wc_status_obstructed,
	git_wc_status_unknown,

}git_wc_status_kind;

typedef enum
{
	git_depth_empty,
	git_depth_infinity,
	git_depth_unknown,
	git_depth_files,
	git_depth_immediates,
}git_depth_t;

#define GIT_REV_ZERO _T("0000000000000000000000000000000000000000")
#define GIT_INVALID_REVNUM _T("")
typedef CString git_revnum_t;

typedef struct git_wc_status2_t
{
	/** The status of the entries text. */
	git_wc_status_kind text_status;

	/** The status of the entries properties. */
	git_wc_status_kind prop_status;

	bool assumeValid;
	bool skipWorktree;
} git_wc_status2;

#define MAX_STATUS_STRING_LENGTH		256

typedef BOOL (*FIll_STATUS_CALLBACK)(const CString &path, git_wc_status_kind status, bool isDir, void *pdata, bool assumeValid, bool skipWorktree);

/**
 * \ingroup Git
 * Handles Subversion status of working copies.
 */
class GitStatus
{
public:

#define GIT_MODE_INDEX 0x1
#define GIT_MODE_HEAD 0x2
#define GIT_MODE_IGNORE 0x4
#define GIT_MODE_ALL (GIT_MODE_INDEX|GIT_MODE_HEAD|GIT_MODE_IGNORE)

	static int GetFileStatus(const CString &gitdir, const CString &path, git_wc_status_kind * status,BOOL IsFull=false, BOOL IsRecursive=false, BOOL isIgnore=true, FIll_STATUS_CALLBACK callback = NULL, void *pData = NULL, bool * assumeValid = NULL, bool * skipWorktree = NULL);
	static int GetDirStatus(const CString &gitdir,const CString &path,git_wc_status_kind * status,BOOL IsFull=false,  BOOL IsRecursive=false, BOOL isIgnore=true, FIll_STATUS_CALLBACK callback=NULL, void *pData=NULL);
	static int EnumDirStatus(const CString &gitdir,const CString &path,git_wc_status_kind * status,BOOL IsFull=false,  BOOL IsRecursive=false, BOOL isIgnore=true, FIll_STATUS_CALLBACK callback=NULL, void *pData=NULL);
	static int GetFileList(const CString &gitdir, const CString &path, std::vector<CGitFileName> &list);
	static bool IsGitReposChanged(const CString &gitdir, const CString &subpaths, int mode=GIT_MODE_ALL);
	static int LoadIgnoreFile(const CString &gitdir, const CString &subpaths);
	static int IsUnderVersionControl(const CString &gitdir, const CString &path, bool isDir,bool *isVersion);
	static int IsIgnore(const CString &gitdir, const CString &path, bool *isIgnore);
	static __int64 GetIndexFileTime(const CString &gitdir);
	static bool IsExistIndexLockFile(const CString &gitdir);

	static int GetHeadHash(const CString &gitdir, CGitHash &hash);

public:
	GitStatus();
	~GitStatus(void);


	/**
	 * Reads the Subversion status of the working copy entry. No
	 * recurse is done, even if the entry is a directory.
	 * If the status of the text and property part are different
	 * then the more important status is returned.
	 */
	static git_wc_status_kind GetAllStatus(const CTGitPath& path, git_depth_t depth = git_depth_empty, bool * assumeValid = NULL, bool * skipWorktree = NULL);

	/**
	 * Reads the Subversion status of the working copy entry and all its
	 * subitems. The resulting status is determined by using priorities for
	 * each status. The status with the highest priority is then returned.
	 * If the status of the text and property part are different then
	 * the more important status is returned.
	 */
	static git_wc_status_kind GetAllStatusRecursive(const CTGitPath& path);

	/**
	 * Returns the status which is more "important" of the two statuses specified.
	 * This is used for the "recursive" status functions on folders - i.e. which status
	 * should be returned for a folder which has several files with different statuses
	 * in it.
	 */
	static git_wc_status_kind GetMoreImportant(git_wc_status_kind status1, git_wc_status_kind status2);

	/**
	 * Checks if a status is "important", i.e. if the status indicates that the user should know about it.
	 * E.g. a "normal" status is not important, but "modified" is.
	 * \param status the status to check
	 */
	static BOOL IsImportant(git_wc_status_kind status) {return (GetMoreImportant(git_wc_status_added, status)==status);}

	/**
	 * Reads the Subversion text status of the working copy entry. No
	 * recurse is done, even if the entry is a directory.
	 * The result is stored in the public member variable status.
	 * Use this method if you need detailed information about a file/folder, not just the raw status (like "normal", "modified").
	 *
	 * \param path the pathname of the entry
	 * \param update true if the status should be updated with the repository. Default is false.
	 * \return If update is set to true the HEAD revision of the repository is returned. If update is false then -1 is returned.
	 * \remark If the return value is -2 then the status could not be obtained.
	 */
	void GetStatus(const CTGitPath& path, bool update = false, bool noignore = false, bool noexternals = false);

	/**
	 * Returns a string representation of a Subversion status.
	 * \param status the status enum
	 * \param string a string representation
	 */
	static void GetStatusString(git_wc_status_kind status, size_t buflen, TCHAR * string);
	static void GetStatusString(HINSTANCE hInst, git_wc_status_kind status, TCHAR * string, int size, WORD lang);

	/**
	 * This member variable hold the status of the last call to GetStatus().
	 */
	git_wc_status2_t *			status;				///< the status result of GetStatus()

#ifdef _MFC_VER
friend class Git;	// So that Git can get to our m_err
	/**
	 * Set a list of paths which will be considered when calling GetFirstFileStatus.
	 * If a filter is set, then GetFirstFileStatus/GetNextFileStatus will only return items which are in the filter list
	 */
	void SetFilter(const CTGitPathList& fileList);
	void ClearFilter();
#endif

private:
	git_wc_status_kind			m_allstatus;	///< used by GetAllStatus and GetAllStatusRecursive
	int							m_err;

	git_wc_status2_t			m_status;		// used for GetStatus

	/**
	 * Returns a numeric value indicating the importance of a status.
	 * A higher number indicates a more important status.
	 */
	static int GetStatusRanking(git_wc_status_kind status);

#pragma warning(push)
#pragma warning(disable: 4200)
	struct STRINGRESOURCEIMAGE
	{
		WORD nLength;
		WCHAR achString[];
	};
#pragma warning(pop)	// C4200

	static int LoadStringEx(HINSTANCE hInstance, UINT uID, LPTSTR lpBuffer, int nBufferMax, WORD wLanguage);
};

