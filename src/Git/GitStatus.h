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
typedef int tgit_error_t;

typedef struct git_wc_entry_t
{
	// url in repository
	const char *url;

	TCHAR cmt_rev[41];
} git_wc_entry_t;


typedef struct git_wc_status2_t
{
	/** The status of the entries text. */
	git_wc_status_kind text_status;

	/** The status of the entries properties. */
	git_wc_status_kind prop_status;

	//git_wc_entry_t *entry;
} git_wc_status2;

#define MAX_STATUS_STRING_LENGTH		256


/////////////////////////////////////////////////////////////////////
// WINGIT API (replaced by commandline tool, but defs and data types kept so old code still works)

// Flags for wgEnumFiles
enum WGENUMFILEFLAGS
{
	WGEFF_NoRecurse		= (1<<0),	// only enumerate files directly in the specified path
	WGEFF_FullPath		= (1<<1),	// enumerated filenames are specified with full path (instead of relative to proj root)
	WGEFF_DirStatusDelta= (1<<2),	// include directories, in enumeration, that have a recursive status != WGFS_Normal (may have a slightly better performance than WGEFF_DirStatusAll)
	WGEFF_DirStatusAll	= (1<<3),	// include directories, in enumeration, with recursive status
	WGEFF_EmptyAsNormal	= (1<<4),	// report sub-directories, with no versioned files, as WGFS_Normal instead of WGFS_Empty
	WGEFF_SingleFile	= (1<<5)	// indicates that the status of a single file or dir, specified by pszSubPath, is wanted
};

// File status
enum WGFILESTATUS
{
	WGFS_Normal,
	WGFS_Modified,
	WGFS_Staged,
	WGFS_Added,
	WGFS_Conflicted,
	WGFS_Deleted,

	WGFS_Ignored = -1,
	WGFS_Unversioned = -2,
	WGFS_Empty = -3,
	WGFS_Unknown = -4
};

// File flags
enum WGFILEFLAGS
{
	WGFF_Directory		= (1<<0)	// enumerated file is a directory
};

struct wgFile_s
{
	LPCTSTR sFileName;				// filename or directory relative to project root (using forward slashes)
	int nStatus;					// the WGFILESTATUS of the file
	int nFlags;						// a combination of WGFILEFLAGS

	const BYTE* sha1;				// points to the BYTE[20] sha1 (NULL for directories, WGFF_Directory)
};

// Application-defined callback function for wgEnumFiles, returns TRUE to abort enumeration
// NOTE: do NOT store the pFile pointer or any pointers in wgFile_s for later use, the data is only valid for a single callback call
typedef BOOL (__cdecl WGENUMFILECB)(const struct wgFile_s *pFile, void *pUserData);

//
/////////////////////////////////////////////////////////////////////


// convert wingit.dll status to git_wc_status_kind
inline static git_wc_status_kind GitStatusFromWingit(int nStatus)
{
	switch (nStatus)
	{
	case WGFS_Normal: return git_wc_status_normal;
	case WGFS_Modified: return git_wc_status_modified;
	case WGFS_Staged: return git_wc_status_merged;
	case WGFS_Added: return git_wc_status_added;
	case WGFS_Conflicted: return git_wc_status_conflicted;
	case WGFS_Deleted: return git_wc_status_deleted;

	case WGFS_Ignored: return git_wc_status_ignored;
	case WGFS_Unversioned: return git_wc_status_unversioned;
	case WGFS_Empty: return git_wc_status_unversioned;
	}

	return git_wc_status_none;
}

// convert 20 byte sha1 hash to the git_revnum_t type
inline static git_revnum_t ConvertHashToRevnum(const BYTE *sha1)
{
	if (!sha1)
		return GIT_INVALID_REVNUM;

	char s[41];
	char *p = s;
	for (int i=0; i<20; i++)
	{
#pragma warning(push)
#pragma warning(disable: 4996)
		sprintf(p, "%02x", (UINT)*sha1);
#pragma warning(pop)
		p += 2;
		sha1++;
	}

	return CString(s);
}

typedef BOOL (*FIll_STATUS_CALLBACK)(const CString &path,git_wc_status_kind status,bool isDir, void *pdata);

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

	static int GetFileStatus(const CString &gitdir,const CString &path,git_wc_status_kind * status,BOOL IsFull=false, BOOL IsRecursive=false, BOOL isIgnore=true, FIll_STATUS_CALLBACK callback=NULL,void *pData=NULL);
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
	GitStatus(bool * pbCanceled = NULL);
	~GitStatus(void);


	/**
	 * Reads the Subversion status of the working copy entry. No
	 * recurse is done, even if the entry is a directory.
	 * If the status of the text and property part are different
	 * then the more important status is returned.
	 */
	static git_wc_status_kind GetAllStatus(const CTGitPath& path, git_depth_t depth = git_depth_empty);

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
	git_revnum_t GetStatus(const CTGitPath& path, bool update = false, bool noignore = false, bool noexternals = false);

	/**
	 * Returns a string representation of a Subversion status.
	 * \param status the status enum
	 * \param string a string representation
	 */
	static void GetStatusString(git_wc_status_kind status, size_t buflen, TCHAR * string);
	static void GetStatusString(HINSTANCE hInst, git_wc_status_kind status, TCHAR * string, int size, WORD lang);

	/**
	 * Returns the string representation of a depth.
	 */
#ifdef _MFC_VER
	static CString GetDepthString(git_depth_t depth);
#endif
	static void GetDepthString(HINSTANCE hInst, git_depth_t depth, TCHAR * string, int size, WORD lang);

	/**
	 * Returns the status of the first file of the given path. Use GetNextFileStatus() to obtain
	 * the status of the next file in the list.
	 * \param path the path of the folder from where the status list should be obtained
	 * \param retPath the path of the file for which the status was returned
	 * \param update set this to true if you want the status to be updated with the repository (needs network access)
	 * \param recurse true to fetch the status recursively
	 * \param bNoIgnore true to not fetch the ignored files
	 * \param bNoExternals true to not fetch the status of included Git:externals
	 * \return the status
	 */
	git_wc_status2_t * GetFirstFileStatus(const CTGitPath& path, CTGitPath& retPath, bool update = false, git_depth_t depth = git_depth_infinity, bool bNoIgnore = true, bool bNoExternals = false);
	unsigned int GetFileCount() const {return /*apr_hash_count(m_statushash);*/0;}
	unsigned int GetVersionedCount() const;
	/**
	 * Returns the status of the next file in the file list. If no more files are in the list then NULL is returned.
	 * See GetFirstFileStatus() for details.
	 */
	git_wc_status2_t * GetNextFileStatus(CTGitPath& retPath);
	/**
	 * Checks if a path is an external folder.
	 * This is necessary since Subversion returns two entries for external folders: one with the status Git_wc_status_external
	 * and one with the 'real' status of that folder. GetFirstFileStatus() and GetNextFileStatus() only return the 'real'
	 * status, so with this method it's possible to check if the status also is Git_wc_status_external.
	 */
	bool IsExternal(const CTGitPath& path) const;
	/**
	 * Checks if a path is in an external folder.
	 */
	bool IsInExternal(const CTGitPath& path) const;

	/**
	 * Clears the memory pool.
	 */
	void ClearPool();

	/**
	 * This member variable hold the status of the last call to GetStatus().
	 */
	git_wc_status2_t *			status;				///< the status result of GetStatus()

	git_revnum_t				headrev;			///< the head revision fetched with GetFirstStatus()

	bool *						m_pbCanceled;
#ifdef _MFC_VER
friend class Git;	// So that Git can get to our m_err
	/**
	 * Returns the last error message as a CString object.
	 */
	CString GetLastErrorMsg() const;

	/**
	 * Set a list of paths which will be considered when calling GetFirstFileStatus.
	 * If a filter is set, then GetFirstFileStatus/GetNextFileStatus will only return items which are in the filter list
	 */
	void SetFilter(const CTGitPathList& fileList);
	void ClearFilter();

#else
	/**
	 * Returns the last error message as a CString object.
	 */
	stdstring GetLastErrorMsg() const;
#endif


protected:
//	apr_pool_t *				m_pool;			///< the memory pool
private:
	typedef struct sort_item
	{
		const void *key;
//		apr_ssize_t klen;
		void *value;
	} sort_item;

	typedef struct hashbaton_t
	{
		GitStatus*		pThis;
//		apr_hash_t *	hash;
//		apr_hash_t *	exthash;
	} hash_baton_t;

//	git_client_ctx_t * 			ctx;
	git_wc_status_kind			m_allstatus;	///< used by GetAllStatus and GetAllStatusRecursive
//	git_error_t *				m_err;			///< Subversion error baton
	tgit_error_t							m_err;

	git_wc_status2_t			m_status;		// used for GetStatus

#ifdef _MFC_VER
//	GitPrompt					m_prompt;
#endif

	/**
	 * Returns a numeric value indicating the importance of a status.
	 * A higher number indicates a more important status.
	 */
	static int GetStatusRanking(git_wc_status_kind status);

	/**
	 * Callback function which collects the raw status from a Git_client_status() function call
	 */
	//static git_error_t * getallstatus (void *baton, const char *path, git_wc_status2_t *status, apr_pool_t *pool);
	static BOOL getallstatus(const struct wgFile_s *pFile, void *pUserData);
	static BOOL getstatus(const struct wgFile_s *pFile, void *pUserData);

	/**
	 * Callback function which stores the raw status from a Git_client_status() function call
	 * in a hash table.
	 */
//	static git_error_t * getstatushash (void *baton, const char *path, git_wc_status2_t *status, apr_pool_t *pool);

	/**
	 * helper function to sort a hash to an array
	 */
//	static apr_array_header_t * sort_hash (apr_hash_t *ht, int (*comparison_func) (const sort_item *,
//										const sort_item *), apr_pool_t *pool);

	/**
	 * Callback function used by qsort() which does the comparison of two elements
	 */
	static int __cdecl sort_compare_items_as_paths (const sort_item *a, const sort_item *b);

	//for GetFirstFileStatus and GetNextFileStatus
//	apr_hash_t *				m_statushash;
//	apr_array_header_t *		m_statusarray;
	unsigned int				m_statushashindex;
//	apr_hash_t *				m_externalhash;

#pragma warning(push)
#pragma warning(disable: 4200)
	struct STRINGRESOURCEIMAGE
	{
		WORD nLength;
		WCHAR achString[];
	};
#pragma warning(pop)	// C4200

	static int LoadStringEx(HINSTANCE hInstance, UINT uID, LPTSTR lpBuffer, int nBufferMax, WORD wLanguage);
	static tgit_error_t* cancel(void *baton);

	// A sorted list of filenames (in Git format, in lowercase)
	// when this list is set, we only pick-up files during a GetStatus which are found in this list
	typedef std::vector<std::string> StdStrAVector;
	StdStrAVector m_filterFileList;
};

