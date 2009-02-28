// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseGit

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

#include "stdafx.h"
#ifdef _TORTOISESHELL
#include "ShellExt.h"
#else
#include "registry.h"
#endif
//#include "resource.h"
#include "..\TortoiseShell\resource.h"
//#include "git_config.h"
#include "GitStatus.h"
#include "UnicodeUtils.h"
//#include "GitGlobal.h"
//#include "GitHelpers.h"
#ifdef _MFC_VER
//#	include "Git.h"
//#	include "MessageBox.h"
//#	include "registry.h"
//#	include "TGitPath.h"
//#	include "PathUtils.h"
#endif
#include "git.h"
#include "gitindex.h"

CGitIndexFileMap g_IndexFileMap;

GitStatus::GitStatus(bool * pbCanceled)
	: status(NULL)
{
#if 0
	m_pool = git_pool_create (NULL);
	
	git_error_clear(git_client_create_context(&ctx, m_pool));
	
	if (pbCanceled)
	{
		ctx->cancel_func = cancel;
		ctx->cancel_baton = pbCanceled;
	}

#ifdef _MFC_VER
	git_error_clear(git_config_ensure(NULL, m_pool));
	
	// set up authentication
	m_prompt.Init(m_pool, ctx);

	// set up the configuration
	m_err = git_config_get_config (&(ctx->config), g_pConfigDir, m_pool);

	if (m_err)
	{
		::MessageBox(NULL, this->GetLastErrorMsg(), _T("TortoiseGit"), MB_ICONERROR);
		git_error_clear(m_err);
		git_pool_destroy (m_pool);					// free the allocated memory
		exit(-1);
	}

	// set up the Git_SSH param
	CString tgit_ssh = CRegString(_T("Software\\TortoiseGit\\SSH"));
	if (tgit_ssh.IsEmpty())
		tgit_ssh = CPathUtils::GetAppDirectory() + _T("TortoisePlink.exe");
	tgit_ssh.Replace('\\', '/');
	if (!tgit_ssh.IsEmpty())
	{
		git_config_t * cfg = (git_config_t *)apr_hash_get ((apr_hash_t *)ctx->config, Git_CONFIG_CATEGORY_CONFIG,
			APR_HASH_KEY_STRING);
		git_config_set(cfg, Git_CONFIG_SECTION_TUNNELS, "ssh", CUnicodeUtils::GetUTF8(tgit_ssh));
	}
#else
	git_error_clear(git_config_ensure(NULL, m_pool));

	// set up the configuration
	m_err = git_config_get_config (&(ctx->config), g_pConfigDir, m_pool);

#endif
#endif
}

GitStatus::~GitStatus(void)
{
#if 0
	git_error_clear(m_err);
	git_pool_destroy (m_pool);					// free the allocated memory
#endif
}

void GitStatus::ClearPool()
{
#if 0
	git_pool_clear(m_pool);
#endif
}

#ifdef _MFC_VER
CString GitStatus::GetLastErrorMsg() const
{
//	return Git::GetErrorString(m_err);
	return CString("");
}
#else
stdstring GitStatus::GetLastErrorMsg() const
{

	stdstring msg;
#if 0
	char errbuf[256];

	if (m_err != NULL)
	{
		git_error_t * ErrPtr = m_err;
		if (ErrPtr->message)
		{
			msg = CUnicodeUtils::StdGetUnicode(ErrPtr->message);
		}
		else
		{
			/* Is this a Subversion-specific error code? */
			if ((ErrPtr->apr_err > APR_OS_START_USEERR)
				&& (ErrPtr->apr_err <= APR_OS_START_CANONERR))
				msg = CUnicodeUtils::StdGetUnicode(git_strerror (ErrPtr->apr_err, errbuf, sizeof (errbuf)));
			/* Otherwise, this must be an APR error code. */
			else
			{
				git_error_t *temp_err = NULL;
				const char * err_string = NULL;
				temp_err = git_utf_cstring_to_utf8(&err_string, apr_strerror (ErrPtr->apr_err, errbuf, sizeof (errbuf)-1), ErrPtr->pool);
				if (temp_err)
				{
					git_error_clear (temp_err);
					msg = _T("Can't recode error string from APR");
				}
				else
				{
					msg = CUnicodeUtils::StdGetUnicode(err_string);
				}
			}

		}

		while (ErrPtr->child)
		{
			ErrPtr = ErrPtr->child;
			msg += _T("\n");
			if (ErrPtr->message)
			{
				msg += CUnicodeUtils::StdGetUnicode(ErrPtr->message);
			}
			else
			{
				/* Is this a Subversion-specific error code? */
				if ((ErrPtr->apr_err > APR_OS_START_USEERR)
					&& (ErrPtr->apr_err <= APR_OS_START_CANONERR))
					msg += CUnicodeUtils::StdGetUnicode(git_strerror (ErrPtr->apr_err, errbuf, sizeof (errbuf)));
				/* Otherwise, this must be an APR error code. */
				else
				{
					git_error_t *temp_err = NULL;
					const char * err_string = NULL;
					temp_err = git_utf_cstring_to_utf8(&err_string, apr_strerror (ErrPtr->apr_err, errbuf, sizeof (errbuf)-1), ErrPtr->pool);
					if (temp_err)
					{
						git_error_clear (temp_err);
						msg += _T("Can't recode error string from APR");
					}
					else
					{
						msg += CUnicodeUtils::StdGetUnicode(err_string);
					}
				}

			}
		}
		return msg;
	} // if (m_err != NULL)
#endif
	return msg;
}
#endif

// static method
git_wc_status_kind GitStatus::GetAllStatus(const CTGitPath& path, git_depth_t depth)
{
	git_wc_status_kind			statuskind;
//	git_client_ctx_t * 			ctx;
	
//	apr_pool_t *				pool;
//	git_error_t *				err;
	BOOL						err;
	BOOL						isDir;
	CString						sProjectRoot;

	isDir = path.IsDirectory();
	if (!path.HasAdminDir(&sProjectRoot))
		return git_wc_status_none;

//	pool = git_pool_create (NULL);				// create the memory pool

//	git_error_clear(git_client_create_context(&ctx, pool));

//	git_revnum_t youngest = Git_INVALID_REVNUM;
//	git_opt_revision_t rev;
//	rev.kind = git_opt_revision_unspecified;
	statuskind = git_wc_status_none;

	const BOOL bIsRecursive = (depth == git_depth_infinity || depth == git_depth_unknown); // taken from SVN source

#ifdef _TORTOISESHELL
	if (g_ShellCache.GetCacheType() == ShellCache::dll)
#else
	if ((DWORD)CRegStdWORD(_T("Software\\TortoiseGit\\CacheType"), GetSystemMetrics(SM_REMOTESESSION) ? 2 : 1) == 2)
#endif
	{
		// gitindex.h based status

		CString sSubPath;
		CString s = path.GetWinPathString();
		if (s.GetLength() > sProjectRoot.GetLength())
		{
			sSubPath = CStringA(s.Right(s.GetLength() - sProjectRoot.GetLength() - 1/*otherwise it gets initial slash*/));
		}

		err = g_IndexFileMap.GetFileStatus(sProjectRoot,sSubPath,&statuskind);
	}
	else
	{
		LPCTSTR lpszSubPath = NULL;
		CString sSubPath;
		CString s = path.GetWinPathString();
		if (s.GetLength() > sProjectRoot.GetLength())
		{
			sSubPath = s.Right(s.GetLength() - sProjectRoot.GetLength() - 1/*otherwise it gets initial slash*/);
			lpszSubPath = sSubPath;
		}

#if 1
		// when recursion enabled, let wingit determine the recursive status for folders instead of enumerating all files here
		UINT nFlags = WGEFF_SingleFile;
		if (!bIsRecursive)
			nFlags |= WGEFF_NoRecurse;
		if (!lpszSubPath)
			// report root dir as normal (otherwise it could be considered git_wc_status_unversioned, which would be wrong?)
			nFlags |= WGEFF_EmptyAsNormal;
#else
		// enumerate all files, recursively if requested
		UINT nFlags = 0;
		if (!bIsRecursive)
			nFlags |= WGEFF_NoRecurse;
#endif

		err = !wgEnumFiles(sProjectRoot, lpszSubPath, nFlags, &getallstatus, &statuskind);

		/*err = git_client_status4 (&youngest,
							path.GetSVNApiPath(pool),
							&rev,
							getallstatus,
							&statuskind,
							depth,
							TRUE,		//getall
							FALSE,		//update
							TRUE,		//noignore
							FALSE,		//ignore externals
							NULL,
							ctx,
							pool);*/
	}

	// Error present
	if (err != NULL)
	{
//		git_error_clear(err);
//		git_pool_destroy (pool);				//free allocated memory
		return git_wc_status_none;	
	}

//	git_pool_destroy (pool);				//free allocated memory

	return statuskind;
}

// static method
git_wc_status_kind GitStatus::GetAllStatusRecursive(const CTGitPath& path)
{
	return GetAllStatus(path, git_depth_infinity);
}

// static method
git_wc_status_kind GitStatus::GetMoreImportant(git_wc_status_kind status1, git_wc_status_kind status2)
{
	if (GetStatusRanking(status1) >= GetStatusRanking(status2))
		return status1;
	return status2;
}
// static private method
int GitStatus::GetStatusRanking(git_wc_status_kind status)
{
	switch (status)
	{
		case git_wc_status_none:
			return 0;
		case git_wc_status_unversioned:
			return 1;
		case git_wc_status_ignored:
			return 2;
		case git_wc_status_incomplete:
			return 4;
		case git_wc_status_normal:
		case git_wc_status_external:
			return 5;
		case git_wc_status_added:
			return 6;
		case git_wc_status_missing:
			return 7;
		case git_wc_status_deleted:
			return 8;
		case git_wc_status_replaced:
			return 9;
		case git_wc_status_modified:
			return 10;
		case git_wc_status_merged:
			return 11;
		case git_wc_status_conflicted:
			return 12;
		case git_wc_status_obstructed:
			return 13;
	}
	return 0;
}

git_revnum_t GitStatus::GetStatus(const CTGitPath& path, bool update /* = false */, bool noignore /* = false */, bool noexternals /* = false */)
{
	// NOTE: unlike the SVN version this one does not cache the enumerated files, because in practice no code in all of
	//       Tortoise uses this, all places that call GetStatus create a temp GitStatus object which gets destroyed right
	//       after the call again

//	apr_hash_t *				statushash;
//	apr_hash_t *				exthash;
//	apr_array_header_t *		statusarray;
//	const sort_item*			item;
	
//	git_error_clear(m_err);
//	statushash = apr_hash_make(m_pool);
//	exthash = apr_hash_make(m_pool);
	git_revnum_t youngest = GIT_INVALID_REVNUM;
//	git_opt_revision_t rev;
//	rev.kind = git_opt_revision_unspecified;

	CString sProjectRoot;
	if ( !path.HasAdminDir(&sProjectRoot) )
		return youngest;

	struct hashbaton_t hashbaton;
//	hashbaton.hash = statushash;
//	hashbaton.exthash = exthash;
	hashbaton.pThis = this;

#ifdef _TORTOISESHELL
	if (g_ShellCache.GetCacheType() == ShellCache::dll)
#else
	if ((DWORD)CRegStdWORD(_T("Software\\TortoiseGit\\CacheType"), GetSystemMetrics(SM_REMOTESESSION) ? 2 : 1) == 2)
#endif
	{
		// gitindex.h based status

		CString sSubPath;
		CString s = path.GetWinPathString();
		if (s.GetLength() > sProjectRoot.GetLength())
		{
			sSubPath = CString(s.Right(s.GetLength() - sProjectRoot.GetLength() - 1/*otherwise it gets initial slash*/));
		}

		m_status.prop_status = m_status.text_status = git_wc_status_none;

		m_err = g_IndexFileMap.GetFileStatus(sProjectRoot,sSubPath,&m_status.text_status);
	}
	else
	{
		LPCTSTR lpszSubPath = NULL;
		CString sSubPath;
		CString s = path.GetWinPathString();
		if (s.GetLength() > sProjectRoot.GetLength())
		{
			sSubPath = s.Right(s.GetLength() - sProjectRoot.GetLength() - 1/*otherwise it gets initial slash*/);
			lpszSubPath = sSubPath;
		}

		// when recursion enabled, let wingit determine the recursive status for folders instead of enumerating all files here
		UINT nFlags = WGEFF_SingleFile | WGEFF_NoRecurse;
		if (!lpszSubPath)
			// report root dir as normal (otherwise it could be considered git_wc_status_unversioned, which would be wrong?)
			nFlags |= WGEFF_EmptyAsNormal;

		m_status.prop_status = m_status.text_status = git_wc_status_none;

		// NOTE: currently wgEnumFiles will not enumerate file if it isn't versioned (so status will be git_wc_status_none)
		m_err = !wgEnumFiles(sProjectRoot, lpszSubPath, nFlags, &getstatus, &m_status);

		/*m_err = git_client_status4 (&youngest,
							path.GetGitApiPath(m_pool),
							&rev,
							getstatushash,
							&hashbaton,
							git_depth_empty,		//depth
							TRUE,		//getall
							update,		//update
							noignore,		//noignore
							noexternals,
							NULL,
							ctx,
							m_pool);*/
	}

	// Error present if function is not under version control
	if (m_err) /*|| (apr_hash_count(statushash) == 0)*/
	{
		status = NULL;
//		return -2;	
		return GIT_INVALID_REVNUM;
	}

	// Convert the unordered hash to an ordered, sorted array
	/*statusarray = sort_hash (statushash,
							  sort_compare_items_as_paths,
							  m_pool);*/

	// only the first entry is needed (no recurse)
//	item = &APR_ARRAY_IDX (statusarray, 0, const sort_item);
	
//	status = (git_wc_status2_t *) item->value;
	status = &m_status;

	if (update)
	{
		// done to match TSVN functionality of this function (not sure if any code uses the reutrn val)
		// if TGit does not need this, then change the return type of function
		youngest = g_Git.GetHash(CString(_T("HEAD")));
	}

	return youngest;
}

git_wc_status2_t * GitStatus::GetFirstFileStatus(const CTGitPath& path, CTGitPath& retPath, bool update, git_depth_t depth, bool bNoIgnore /* = true */, bool bNoExternals /* = false */)
{
	static git_wc_status2 st;
/*
	m_fileCache.Reset();

	m_fileCache.Init( CStringA( path.GetWinPathString().GetString() ) );
MessageBox(NULL, path.GetWinPathString(), _T("GetFirstFile"), MB_OK);
	m_fileCache.m_pFileIter = m_fileCache.m_pFiles;
	st.text_status = git_wc_status_none;

	if (m_fileCache.m_pFileIter)
	{
		switch(m_fileCache.m_pFileIter->nStatus)
		{
		case WGFS_Normal: st.text_status = git_wc_status_normal; break;
		case WGFS_Modified: st.text_status = git_wc_status_modified; break;
		case WGFS_Deleted: st.text_status = git_wc_status_deleted; break;
		}

		//retPath.SetFromGit((const char*)item->key);

		m_fileCache.m_pFileIter = m_fileCache.m_pFileIter->pNext;
	}

	return &st;
*/
#if 0
	const sort_item*			item;

	git_error_clear(m_err);
	m_statushash = apr_hash_make(m_pool);
	m_externalhash = apr_hash_make(m_pool);
	headrev = Git_INVALID_REVNUM;
	git_opt_revision_t rev;
	rev.kind = git_opt_revision_unspecified;
	struct hashbaton_t hashbaton;
	hashbaton.hash = m_statushash;
	hashbaton.exthash = m_externalhash;
	hashbaton.pThis = this;
	m_statushashindex = 0;
	m_err = git_client_status4 (&headrev,
							path.GetGitApiPath(m_pool),
							&rev,
							getstatushash,
							&hashbaton,
							depth,
							TRUE,		//getall
							update,		//update
							bNoIgnore,	//noignore
							bNoExternals,		//noexternals
							NULL,
							ctx,
							m_pool);


	// Error present if function is not under version control
	if ((m_err != NULL) || (apr_hash_count(m_statushash) == 0))
	{
		return NULL;	
	}

	// Convert the unordered hash to an ordered, sorted array
	m_statusarray = sort_hash (m_statushash,
								sort_compare_items_as_paths,
								m_pool);

	// only the first entry is needed (no recurse)
	m_statushashindex = 0;
	item = &APR_ARRAY_IDX (m_statusarray, m_statushashindex, const sort_item);
	retPath.SetFromGit((const char*)item->key);
	return (git_wc_status2_t *) item->value;
#endif

	return 0;
}

unsigned int GitStatus::GetVersionedCount() const
{
//	return /**/m_fileCache.GetFileCount();

	unsigned int count = 0;
#if 0
	const sort_item* item;
	for (unsigned int i=0; i<apr_hash_count(m_statushash); ++i)
	{
		item = &APR_ARRAY_IDX(m_statusarray, i, const sort_item);
		if (item)
		{
			if (GitStatus::GetMoreImportant(((git_wc_status_t *)item->value)->text_status, git_wc_status_ignored)!=git_wc_status_ignored)
				count++;				
		}
	}
#endif
	return count;
}

git_wc_status2_t * GitStatus::GetNextFileStatus(CTGitPath& retPath)
{
	static git_wc_status2 st;

	st.text_status = git_wc_status_none;

	/*if (m_fileCache.m_pFileIter)
	{
		switch(m_fileCache.m_pFileIter->nStatus)
		{
		case WGFS_Normal: st.text_status = git_wc_status_normal; break;
		case WGFS_Modified: st.text_status = git_wc_status_modified; break;
		case WGFS_Deleted: st.text_status = git_wc_status_deleted; break;
		}

		m_fileCache.m_pFileIter = m_fileCache.m_pFileIter->pNext;
	}*/

	return &st;

#if 0
	const sort_item*			item;

	if ((m_statushashindex+1) >= apr_hash_count(m_statushash))
		return NULL;
	m_statushashindex++;

	item = &APR_ARRAY_IDX (m_statusarray, m_statushashindex, const sort_item);
	retPath.SetFromGit((const char*)item->key);
	return (git_wc_status2_t *) item->value;
#endif
	return 0;
}

bool GitStatus::IsExternal(const CTGitPath& path) const
{
#if 0
	if (apr_hash_get(m_externalhash, path.GetGitApiPath(m_pool), APR_HASH_KEY_STRING))
		return true;
#endif
	return false;
}

bool GitStatus::IsInExternal(const CTGitPath& path) const
{
#if 0
	if (apr_hash_count(m_statushash) == 0)
		return false;

	GitPool localpool(m_pool);
	apr_hash_index_t *hi;
	const char* key;
	for (hi = apr_hash_first(localpool, m_externalhash); hi; hi = apr_hash_next(hi)) 
	{
		apr_hash_this(hi, (const void**)&key, NULL, NULL);
		if (key)
		{
			if (CTGitPath(CUnicodeUtils::GetUnicode(key)).IsAncestorOf(path))
				return true;
		}
	}
#endif
	return false;
}


void GitStatus::GetStatusString(git_wc_status_kind status, size_t buflen, TCHAR * string)
{
	TCHAR * buf;
	switch (status)
	{
		case git_wc_status_none:
			buf = _T("none\0");
			break;
		case git_wc_status_unversioned:
			buf = _T("unversioned\0");
			break;
		case git_wc_status_normal:
			buf = _T("normal\0");
			break;
		case git_wc_status_added:
			buf = _T("added\0");
			break;
		case git_wc_status_missing:
			buf = _T("missing\0");
			break;
		case git_wc_status_deleted:
			buf = _T("deleted\0");
			break;
		case git_wc_status_replaced:
			buf = _T("replaced\0");
			break;
		case git_wc_status_modified:
			buf = _T("modified\0");
			break;
		case git_wc_status_merged:
			buf = _T("merged\0");
			break;
		case git_wc_status_conflicted:
			buf = _T("conflicted\0");
			break;
		case git_wc_status_obstructed:
			buf = _T("obstructed\0");
			break;
		case git_wc_status_ignored:
			buf = _T("ignored");
			break;
		case git_wc_status_external:
			buf = _T("external");
			break;
		case git_wc_status_incomplete:
			buf = _T("incomplete\0");
			break;
		default:
			buf = _T("\0");
			break;
	}
	_stprintf_s(string, buflen, _T("%s"), buf);
}

void GitStatus::GetStatusString(HINSTANCE hInst, git_wc_status_kind status, TCHAR * string, int size, WORD lang)
{
	switch (status)
	{
		case git_wc_status_none:
			LoadStringEx(hInst, IDS_STATUSNONE, string, size, lang);
			break;
		case git_wc_status_unversioned:
			LoadStringEx(hInst, IDS_STATUSUNVERSIONED, string, size, lang);
			break;
		case git_wc_status_normal:
			LoadStringEx(hInst, IDS_STATUSNORMAL, string, size, lang);
			break;
		case git_wc_status_added:
			LoadStringEx(hInst, IDS_STATUSADDED, string, size, lang);
			break;
		case git_wc_status_missing:
			LoadStringEx(hInst, IDS_STATUSABSENT, string, size, lang);
			break;
		case git_wc_status_deleted:
			LoadStringEx(hInst, IDS_STATUSDELETED, string, size, lang);
			break;
		case git_wc_status_replaced:
			LoadStringEx(hInst, IDS_STATUSREPLACED, string, size, lang);
			break;
		case git_wc_status_modified:
			LoadStringEx(hInst, IDS_STATUSMODIFIED, string, size, lang);
			break;
		case git_wc_status_merged:
			LoadStringEx(hInst, IDS_STATUSMERGED, string, size, lang);
			break;
		case git_wc_status_conflicted:
			LoadStringEx(hInst, IDS_STATUSCONFLICTED, string, size, lang);
			break;
		case git_wc_status_ignored:
			LoadStringEx(hInst, IDS_STATUSIGNORED, string, size, lang);
			break;
		case git_wc_status_obstructed:
			LoadStringEx(hInst, IDS_STATUSOBSTRUCTED, string, size, lang);
			break;
		case git_wc_status_external:
			LoadStringEx(hInst, IDS_STATUSEXTERNAL, string, size, lang);
			break;
		case git_wc_status_incomplete:
			LoadStringEx(hInst, IDS_STATUSINCOMPLETE, string, size, lang);
			break;
		default:
			LoadStringEx(hInst, IDS_STATUSNONE, string, size, lang);
			break;
	}
}

#ifdef _MFC_VER
CString GitStatus::GetDepthString(git_depth_t depth)
{
#if 0
	CString sDepth;
	switch (depth)
	{
	case git_depth_unknown:
		sDepth.LoadString(IDS_Git_DEPTH_UNKNOWN);
		break;
	case git_depth_empty:
		sDepth.LoadString(IDS_Git_DEPTH_EMPTY);
		break;
	case git_depth_files:
		sDepth.LoadString(IDS_Git_DEPTH_FILES);
		break;
	case git_depth_immediates:
		sDepth.LoadString(IDS_Git_DEPTH_IMMEDIATE);
		break;
	case git_depth_infinity:
		sDepth.LoadString(IDS_Git_DEPTH_INFINITE);
		break;
	}
	return sDepth;
#endif
	return CString("");
}
#endif

void GitStatus::GetDepthString(HINSTANCE hInst, git_depth_t depth, TCHAR * string, int size, WORD lang)
{
#if 0
	switch (depth)
	{
	case git_depth_unknown:
		LoadStringEx(hInst, IDS_SVN_DEPTH_UNKNOWN, string, size, lang);
		break;
	case git_depth_empty:
		LoadStringEx(hInst, IDS_SVN_DEPTH_EMPTY, string, size, lang);
		break;
	case git_depth_files:
		LoadStringEx(hInst, IDS_SVN_DEPTH_FILES, string, size, lang);
		break;
	case git_depth_immediates:
		LoadStringEx(hInst, IDS_SVN_DEPTH_IMMEDIATE, string, size, lang);
		break;
	case git_depth_infinity:
		LoadStringEx(hInst, IDS_SVN_DEPTH_INFINITE, string, size, lang);
		break;
	}
#endif
}


int GitStatus::LoadStringEx(HINSTANCE hInstance, UINT uID, LPTSTR lpBuffer, int nBufferMax, WORD wLanguage)
{
	const STRINGRESOURCEIMAGE* pImage;
	const STRINGRESOURCEIMAGE* pImageEnd;
	ULONG nResourceSize;
	HGLOBAL hGlobal;
	UINT iIndex;
	int ret;

	HRSRC hResource =  FindResourceEx(hInstance, RT_STRING, MAKEINTRESOURCE(((uID>>4)+1)), wLanguage);
	if (!hResource)
	{
		// try the default language before giving up!
		hResource = FindResource(hInstance, MAKEINTRESOURCE(((uID>>4)+1)), RT_STRING);
		if (!hResource)
			return 0;
	}
	hGlobal = LoadResource(hInstance, hResource);
	if (!hGlobal)
		return 0;
	pImage = (const STRINGRESOURCEIMAGE*)::LockResource(hGlobal);
	if(!pImage)
		return 0;

	nResourceSize = ::SizeofResource(hInstance, hResource);
	pImageEnd = (const STRINGRESOURCEIMAGE*)(LPBYTE(pImage)+nResourceSize);
	iIndex = uID&0x000f;

	while ((iIndex > 0) && (pImage < pImageEnd))
	{
		pImage = (const STRINGRESOURCEIMAGE*)(LPBYTE(pImage)+(sizeof(STRINGRESOURCEIMAGE)+(pImage->nLength*sizeof(WCHAR))));
		iIndex--;
	}
	if (pImage >= pImageEnd)
		return 0;
	if (pImage->nLength == 0)
		return 0;

	ret = pImage->nLength;
	if (pImage->nLength > nBufferMax)
	{
		wcsncpy_s(lpBuffer, nBufferMax, pImage->achString, pImage->nLength-1);
		lpBuffer[nBufferMax-1] = 0;
	}
	else
	{
		wcsncpy_s(lpBuffer, nBufferMax, pImage->achString, pImage->nLength);
		lpBuffer[ret] = 0;
	}
	return ret;
}

BOOL GitStatus::getallstatus(const struct wgFile_s *pFile, void *pUserData)
{
	git_wc_status_kind * s = (git_wc_status_kind *)pUserData;
	*s = GitStatus::GetMoreImportant(*s, GitStatusFromWingit(pFile->nStatus));
	return FALSE;
}

BOOL GitStatus::getstatus(const struct wgFile_s *pFile, void *pUserData)
{
	git_wc_status2_t * s = (git_wc_status2_t*)pUserData;
	s->prop_status = s->text_status = GitStatus::GetMoreImportant(s->prop_status, GitStatusFromWingit(pFile->nStatus));
	return FALSE;
}

#if 0
git_error_t * GitStatus::getallstatus(void * baton, const char * /*path*/, git_wc_status2_t * status, apr_pool_t * /*pool*/)
{
	git_wc_status_kind * s = (git_wc_status_kind *)baton;
	*s = GitStatus::GetMoreImportant(*s, status->text_status);
	*s = GitStatus::GetMoreImportant(*s, status->prop_status);
	return Git_NO_ERROR;
}
#endif

#if 0
git_error_t * GitStatus::getstatushash(void * baton, const char * path, git_wc_status2_t * status, apr_pool_t * /*pool*/)
{
	hashbaton_t * hash = (hashbaton_t *)baton;
	const StdStrAVector& filterList = hash->pThis->m_filterFileList;
	if (status->text_status == git_wc_status_external)
	{
		apr_hash_set (hash->exthash, apr_pstrdup(hash->pThis->m_pool, path), APR_HASH_KEY_STRING, (const void*)1);
		return Git_NO_ERROR;
	}
	if(filterList.size() > 0)
	{
		// We have a filter active - we're only interested in files which are in 
		// the filter  
		if(!binary_search(filterList.begin(), filterList.end(), path))
		{
			// This item is not in the filter - don't store it
			return Git_NO_ERROR;
		}
	}
	git_wc_status2_t * statuscopy = git_wc_dup_status2 (status, hash->pThis->m_pool);
	apr_hash_set (hash->hash, apr_pstrdup(hash->pThis->m_pool, path), APR_HASH_KEY_STRING, statuscopy);
	return Git_NO_ERROR;
}

apr_array_header_t * GitStatus::sort_hash (apr_hash_t *ht,
										int (*comparison_func) (const GitStatus::sort_item *, const GitStatus::sort_item *),
										apr_pool_t *pool)
{
	apr_hash_index_t *hi;
	apr_array_header_t *ary;

	/* allocate an array with only one element to begin with. */
	ary = apr_array_make (pool, 1, sizeof(sort_item));

	/* loop over hash table and push all keys into the array */
	for (hi = apr_hash_first (pool, ht); hi; hi = apr_hash_next (hi))
	{
		sort_item *item = (sort_item*)apr_array_push (ary);

		apr_hash_this (hi, &item->key, &item->klen, &item->value);
	}

	/* now quick sort the array.  */
	qsort (ary->elts, ary->nelts, ary->elt_size,
		(int (*)(const void *, const void *))comparison_func);

	return ary;
}

int GitStatus::sort_compare_items_as_paths (const sort_item *a, const sort_item *b)
{
	const char *astr, *bstr;

	astr = (const char*)a->key;
	bstr = (const char*)b->key;
	return git_path_compare_paths (astr, bstr);
}
#endif

git_error_t* GitStatus::cancel(void *baton)
{
#if 0
	volatile bool * canceled = (bool *)baton;
	if (*canceled)
	{
		CString temp;
		temp.LoadString(IDS_Git_USERCANCELLED);
		return git_error_create(Git_ERR_CANCELLED, NULL, CUnicodeUtils::GetUTF8(temp));
	}
	return Git_NO_ERROR;
#endif 
	return 0;
}

#ifdef _MFC_VER

// Set-up a filter to restrict the files which will have their status stored by a get-status
void GitStatus::SetFilter(const CTGitPathList& fileList)
{
	m_filterFileList.clear();
	for(int fileIndex = 0; fileIndex < fileList.GetCount(); fileIndex++)
	{
//		m_filterFileList.push_back(fileList[fileIndex].GetGitApiPath(m_pool));
	}
	// Sort the list so that we can do binary searches
	std::sort(m_filterFileList.begin(), m_filterFileList.end());
}

void GitStatus::ClearFilter()
{
	m_filterFileList.clear();
}

#endif // _MFC_VER

