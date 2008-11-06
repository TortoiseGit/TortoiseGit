#pragma once

#ifdef _MFC_VER
#	include "SVNPrompt.h"
#endif
#include "TGitPath.h"

#pragma warning (push,1)
typedef std::basic_string<wchar_t> wide_string;
#ifdef UNICODE
#	define stdstring wide_string
#else
#	define stdstring std::string
#endif
#pragma warning (pop)

#include "TGitPath.h"

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

}git_wc_status_kind;

typedef enum
{
	git_depth_empty,
	git_depth_infinity,
	git_depth_unknown,
	git_depth_files,
	git_depth_immediates,
}git_depth_t;



typedef CString git_revnum_t;
typedef int git_wc_status2_t;
typedef int git_error_t;

#define MAX_STATUS_STRING_LENGTH		256

/**
 * \ingroup Git
 * Handles Subversion status of working copies.
 */
class GitStatus
{
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
	static CString GetDepthString(Git_depth_t depth);
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
	git_error_t *				m_err;			///< Subversion error baton

#ifdef _MFC_VER
	GitPrompt					m_prompt;
#endif

	/**
	 * Returns a numeric value indicating the importance of a status. 
	 * A higher number indicates a more important status.
	 */
	static int GetStatusRanking(git_wc_status_kind status);

	/**
	 * Callback function which collects the raw status from a Git_client_status() function call
	 */
//	static git_error_t * getallstatus (void *baton, const char *path, git_wc_status2_t *status, apr_pool_t *pool);

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
	static git_error_t* cancel(void *baton);

	// A sorted list of filenames (in Git format, in lowercase) 
	// when this list is set, we only pick-up files during a GetStatus which are found in this list
	typedef std::vector<std::string> StdStrAVector;
	StdStrAVector m_filterFileList;
};


	