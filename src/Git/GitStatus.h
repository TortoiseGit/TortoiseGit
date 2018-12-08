// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2018 - TortoiseGit

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
#include "TGitPath.h"
#include "PathUtils.h"

class CGitFileName;

#include "GitHash.h"

typedef enum type_git_wc_status_kind
{
	git_wc_status_none,
	git_wc_status_unversioned,
	git_wc_status_ignored,
	git_wc_status_normal,
	git_wc_status_deleted,
	git_wc_status_modified,
	git_wc_status_added,
	git_wc_status_conflicted,
	git_wc_status_unknown, // should be last, see TGitCache/CacheInterface.h
}git_wc_status_kind;

typedef struct git_wc_status2_t
{
	git_wc_status_kind status;

	bool assumeValid;
	bool skipWorktree;
} git_wc_status2;

#define MAX_STATUS_STRING_LENGTH		256

typedef BOOL (*FILL_STATUS_CALLBACK)(const CString& path, const git_wc_status2_t* status, bool isDir, __int64 lastwritetime, void* baton);

static CString CombinePath(const CString& part1, const CString& part2)
{
	CString path(part1);
	path += L'\\';
	path += part2;
	return path;
}

static CString CombinePath(const CString& part1, const CString& part2, const CString& part3)
{
	CString path(part1);
	path += L'\\';
	path += part2;
	CPathUtils::EnsureTrailingPathDelimiter(path);
	path += part3;
	return path;
}

/**
 * \ingroup Git
 * Handles git status of working copies.
 */
class GitStatus
{
public:

	static int GetFileStatus(const CString& gitdir, CString path, git_wc_status2_t& status, BOOL IsFull = FALSE, BOOL isIgnore = TRUE, bool update = true);
	static int GetDirStatus(const CString& gitdir, const CString& path, git_wc_status_kind* status, BOOL IsFull = false, BOOL IsRecursive = false, BOOL isIgnore = true);
	static int EnumDirStatus(const CString& gitdir, const CString& path, git_wc_status_kind* dirstatus, FILL_STATUS_CALLBACK callback, void* pData);
	static int GetFileList(const CString& path, std::vector<CGitFileName>& list, bool& isRepoRoot, bool ignoreCase);
	static bool IsExistIndexLockFile(CString gitdir);
	static bool ReleasePath(const CString &gitdir);
	static bool ReleasePathsRecursively(const CString &rootpath);

	GitStatus();

	/**
	 * Reads the git status of the working copy entry. No
	 * recurse is done, even if the entry is a directory.
	 * If the status of the text and property part are different
	 * then the more important status is returned.
	 */
	static int GetAllStatus(const CTGitPath& path, bool bIsRecursive, git_wc_status2_t& status);

	/**
	 * Returns the status which is more "important" of the two statuses specified.
	 * This is used for the "recursive" status functions on folders - i.e. which status
	 * should be returned for a folder which has several files with different statuses
	 * in it.
	 */
	static git_wc_status_kind GetMoreImportant(git_wc_status_kind status1, git_wc_status_kind status2);

	static void AdjustFolderStatus(git_wc_status_kind& status);

	/**
	 * Reads the git text status of the working copy entry. No
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
	 * This member variable hold the status of the last call to GetStatus().
	 */
	git_wc_status2_t *			status;				///< the status result of GetStatus()

private:
	git_wc_status2_t			m_status;		// used for GetStatus

	/**
	 * Returns a numeric value indicating the importance of a status.
	 * A higher number indicates a more important status.
	 */
	static int GetStatusRanking(git_wc_status_kind status);
};
