// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2008,2011 - TortoiseSVN
// Copyright (C) 2008-2011 - TortoiseGit

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

#include "stdafx.h"
#include "ShellExt.h"
#include "GitFolderStatus.h"
#include "UnicodeUtils.h"
#include "..\TGitCache\CacheInterface.h"
#include "Git.h"
//#include "GitGlobal.h"
#include "gitindex.h"

extern ShellCache g_ShellCache;

//extern CGitIndexFileMap g_IndexFileMap;

// get / auto-alloc a string "copy"

const char* StringPool::GetString (const char* value)
{
	// special case: NULL pointer

	if (value == NULL)
	{
		return emptyString;
	}

	// do we already have a string with the desired value?

	pool_type::const_iterator iter = pool.find (value);
	if (iter != pool.end())
	{
		// yes -> return it
		return *iter;
	}

	// no -> add one

	const char* newString =  _strdup (value);
	if (newString)
	{
		pool.insert (newString);
	}
	else
		return emptyString;

	// .. and return it

	return newString;
}

// clear internal pool

void StringPool::clear()
{
	// delete all strings

	for (pool_type::iterator iter = pool.begin(), end = pool.end(); iter != end; ++iter)
	{
		free((void*)*iter);
	}

	// remove pointers from pool

	pool.clear();
}

CTGitPath	GitFolderStatus::folderpath;


GitFolderStatus::GitFolderStatus(void)
{
	m_TimeStamp = 0;
	emptyString[0] = 0;
	invalidstatus.author = emptyString;
	invalidstatus.askedcounter = -1;
	invalidstatus.status = git_wc_status_none;
	invalidstatus.url = emptyString;
//	invalidstatus.rev = -1;
	invalidstatus.owner = emptyString;
	invalidstatus.needslock = false;
	invalidstatus.tree_conflict = false;
	m_nCounter = 0;
	dirstatus = NULL;
	sCacheKey.reserve(MAX_PATH);

	//rootpool = svn_pool_create (NULL);
	g_Git.SetCurrentDir(_T(""));
	m_hInvalidationEvent = CreateEvent(NULL, FALSE, FALSE, _T("TortoiseGitCacheInvalidationEvent"));
}

GitFolderStatus::~GitFolderStatus(void)
{
	//svn_pool_destroy(rootpool);
}

const FileStatusCacheEntry * GitFolderStatus::BuildCache(const CTGitPath& filepath, const CString& sProjectRoot, BOOL bIsFolder, BOOL bDirectFolder)
{
//	svn_client_ctx_t *			localctx;
//	apr_hash_t *				statushash;
//	apr_pool_t *				pool;
	//git_error_t *				err = NULL; // If svn_client_status comes out through catch(...), err would else be unassigned
	git_error_t err = 0;

	//dont' build the cache if an instance of TortoiseProc is running
	//since this could interfere with svn commands running (concurrent
	//access of the .git directory).
	if (g_ShellCache.BlockStatus())
	{
		CAutoGeneralHandle TGitMutex = ::CreateMutex(NULL, FALSE, _T("TortoiseGitProc.exe"));
		if (TGitMutex != NULL)
		{
			if (::GetLastError() == ERROR_ALREADY_EXISTS)
			{
				return &invalidstatus;
			}
		}
	}

//	pool = svn_pool_create (rootpool);				// create the memory pool

	ClearCache();
//	svn_error_clear(svn_client_create_context(&localctx, pool));
	// set up the configuration
	// Note: I know this is an 'expensive' call, but without this, ignores
	// done in the global ignore pattern won't show up.
//	if (g_ShellCache.ShowIgnoredOverlay())
//;//		svn_error_clear(svn_config_get_config (&(localctx->config), g_pConfigDir, pool));

	// strings pools are unused, now -> we may clear them

	authors.clear();
	urls.clear();
	owners.clear();

	if (bIsFolder)
	{
		if (bDirectFolder)
		{
			// NOTE: see not in GetFullStatus about project inside another project, we should only get here when
			//       that occurs, and this is not correctly handled yet

			// initialize record members
//			dirstat.rev = -1;
			dirstat.status = git_wc_status_none;
			dirstat.author = authors.GetString(NULL);
			dirstat.url = urls.GetString(NULL);
			dirstat.owner = owners.GetString(NULL);
			dirstat.askedcounter = GITFOLDERSTATUS_CACHETIMES;
			dirstat.needslock = false;
			dirstat.tree_conflict = false;

			dirstatus = NULL;
//			statushash = apr_hash_make(pool);
//			git_revnum_t youngest = GIT_INVALID_REVNUM;
//			git_opt_revision_t rev;
//			rev.kind = git_opt_revision_unspecified;
			try
			{
				folderpath = filepath;

				/*err = svn_client_status4 (&youngest,
					filepath.GetDirectory().GetSVNApiPath(pool),
					&rev,
					findfolderstatus,
					this,
					svn_depth_empty,//depth
					TRUE,		//getall
					FALSE,		//update
					TRUE,		//noignore
					FALSE,		//ignore externals
					NULL,
					localctx,
					pool);*/
			}
			catch ( ... )
			{
				dirstatus = NULL;
			}


			if (dirstatus)
			{
/*				if (dirstatus->entry)
				{
					dirstat.author = authors.GetString (dirstatus->entry->cmt_author);
					dirstat.url = authors.GetString (dirstatus->entry->url);
					dirstat.rev = dirstatus->entry->cmt_rev;
					dirstat.owner = owners.GetString(dirstatus->entry->lock_owner);
				}*/
				dirstat.status = GitStatus::GetMoreImportant(dirstatus->text_status, dirstatus->prop_status);
//				dirstat.tree_conflict = dirstatus->tree_conflict != NULL;
			}
			m_cache[filepath.GetWinPath()] = dirstat;
			m_TimeStamp = GetTickCount();
//			svn_error_clear(err);
//			svn_pool_destroy (pool);				//free allocated memory
			return &dirstat;
		}
	} // if (bIsFolder)

	m_nCounter = 0;

	//Fill in the cache with
	//all files inside the same folder as the asked file/folder is
	//since subversion can do this in one step
//	localctx->auth_baton = NULL;

//	statushash = apr_hash_make(pool);
//	git_revnum_t youngest = GIT_INVALID_REVNUM;
//	git_opt_revision_t rev;
//	rev.kind = git_opt_revision_unspecified;

	git_wc_status_kind status;
	int t1,t2;
	t2=t1=0;
	try
	{
		git_depth_t depth = git_depth_infinity;

		if (g_ShellCache.GetCacheType() == ShellCache::dll)
		{
			depth = git_depth_empty;
		}

		t1 = ::GetCurrentTime();
		status = m_GitStatus.GetAllStatus(filepath, depth);
		t2 = ::GetCurrentTime();
	}
	catch ( ... )
	{
	}

	ATLTRACE2(_T("building cache for %s - time %d\n"), filepath.GetWinPath(), t2 -t1);

	// Error present if function is not under version control
	if (err != NULL)
	{
//		svn_error_clear(err);
//		svn_pool_destroy (pool);				//free allocated memory
		return &invalidstatus;
	}

//	svn_error_clear(err);
//	svn_pool_destroy (pool);				//free allocated memory
	m_TimeStamp = GetTickCount();
	FileStatusCacheEntry * ret = NULL;

	if (_tcslen(filepath.GetWinPath())==3)
		ret = &m_cache[(LPCTSTR)filepath.GetWinPathString().Left(2)];
	else
		ret = &m_cache[filepath.GetWinPath()];

	//memset(ret, 0, sizeof(FileStatusCacheEntry));
	ret->status = status;

	m_mostRecentPath = filepath;
	m_mostRecentStatus = ret;

#if 0
	FileStatusMap::const_iterator iter;
	if ((iter = m_cache.find(filepath.GetWinPath())) != m_cache.end())
	{
		ret = &iter->second;
		m_mostRecentPath = filepath;
		m_mostRecentStatus = ret;
	}
	else
	{
		// for SUBST'ed drives, Subversion doesn't return a path with a backslash
		// e.g. G:\ but only G: when fetching the status. So search for that
		// path too before giving up.
		// This is especially true when right-clicking directly on a SUBST'ed
		// drive to get the context menu
		if (_tcslen(filepath.GetWinPath())==3)
		{
			if ((iter = m_cache.find((LPCTSTR)filepath.GetWinPathString().Left(2))) != m_cache.end())
			{
				ret = &iter->second;
				m_mostRecentPath = filepath;
				m_mostRecentStatus = ret;
			}
		}
	}
#endif
	if (ret)
		return ret;
	return &invalidstatus;
}

DWORD GitFolderStatus::GetTimeoutValue()
{
	DWORD timeout = GITFOLDERSTATUS_CACHETIMEOUT;
	DWORD factor = m_cache.size()/200;
	if (factor==0)
		factor = 1;
	return factor*timeout;
}

const FileStatusCacheEntry * GitFolderStatus::GetFullStatus(const CTGitPath& filepath, BOOL bIsFolder, BOOL bColumnProvider)
{
	const FileStatusCacheEntry * ret = NULL;

	CString sProjectRoot;
	BOOL bHasAdminDir = g_ShellCache.HasGITAdminDir(filepath.GetWinPath(), bIsFolder, &sProjectRoot);

	//no overlay for unversioned folders
	if ((!bColumnProvider)&&(!bHasAdminDir))
		return &invalidstatus;
	//for the SVNStatus column, we have to check the cache to see
	//if it's not just unversioned but ignored
	ret = GetCachedItem(filepath);
	if ((ret)&&(ret->status == git_wc_status_unversioned)&&(bIsFolder)&&(bHasAdminDir))
	{
		// an 'unversioned' folder, but with an ADMIN dir --> nested layout!
		// NOTE: this could be a sub-project in git, or just some standalone project inside of another, either way a TODO
		ret = BuildCache(filepath, sProjectRoot, bIsFolder, TRUE);
		if (ret)
			return ret;
		else
			return &invalidstatus;
	}
	if (ret)
		return ret;

	//if it's not in the cache and has no admin dir, then we assume
	//it's not ignored too
	if ((bColumnProvider)&&(!bHasAdminDir))
		return &invalidstatus;
	ret = BuildCache(filepath, sProjectRoot, bIsFolder);
	if (ret)
		return ret;
	else
		return &invalidstatus;
}

const FileStatusCacheEntry * GitFolderStatus::GetCachedItem(const CTGitPath& filepath)
{
	sCacheKey.assign(filepath.GetWinPath());
	FileStatusMap::const_iterator iter;
	const FileStatusCacheEntry *retVal;

	if(m_mostRecentPath.IsEquivalentTo(CTGitPath(sCacheKey.c_str())))
	{
		// We've hit the same result as we were asked for last time
		ATLTRACE2(_T("fast cache hit for %s\n"), filepath);
		retVal = m_mostRecentStatus;
	}
	else if ((iter = m_cache.find(sCacheKey)) != m_cache.end())
	{
		ATLTRACE2(_T("cache found for %s\n"), filepath);
		retVal = &iter->second;
		m_mostRecentStatus = retVal;
		m_mostRecentPath = CTGitPath(sCacheKey.c_str());
	}
	else
	{
		retVal = NULL;
	}

	if(retVal != NULL)
	{
		// We found something in a cache - check that the cache is not timed-out or force-invalidated
		DWORD now = GetTickCount();

		if ((now >= m_TimeStamp)&&((now - m_TimeStamp) > GetTimeoutValue()))
		{
			// Cache is timed-out
			ATLTRACE("Cache timed-out\n");
			ClearCache();
			retVal = NULL;
		}
		else if(WaitForSingleObject(m_hInvalidationEvent, 0) == WAIT_OBJECT_0)
		{
			// TortoiseProc has just done something which has invalidated the cache
			ATLTRACE("Cache invalidated\n");
			ClearCache();
			retVal = NULL;
		}
		return retVal;
	}
	return NULL;
}

BOOL GitFolderStatus::fillstatusmap(const struct wgFile_s *pFile, void *pUserData)
{
	GitFolderStatus *Stat = (GitFolderStatus*)pUserData;

	FileStatusMap &cache = Stat->m_cache;
	FileStatusCacheEntry s;
	s.needslock = false;
	s.tree_conflict = false;

	s.author = Stat->authors.GetString(NULL);
	s.url = Stat->urls.GetString(NULL);
	if (pFile->sha1)
		s.rev = ConvertHashToRevnum(pFile->sha1);
	s.owner = Stat->owners.GetString(NULL);

	s.status = git_wc_status_none;

	//s.status = GitStatus::GetMoreImportant(s.status, status->text_status);
	//s.status = GitStatus::GetMoreImportant(s.status, status->prop_status);
	s.status = GitStatusFromWingit(pFile->nStatus);

	// TODO ?: s.blaha = pFile->nStage

	//s.lock = status->repos_lock;
	//s.tree_conflict = (status->tree_conflict != NULL);

	s.askedcounter = GITFOLDERSTATUS_CACHETIMES;
	stdstring str;
	if (pFile->sFileName)
	{
		str = pFile->sFileName;//CUnicodeUtils::StdGetUnicode(pFile->sFileName);
		std::replace(str.begin(), str.end(), '/', '\\');
//MessageBox(NULL, str.c_str(), _T(""), MB_OK);
	}
	else
		str = _T(" ");
	cache[str] = s;

	return FALSE;
}

void GitFolderStatus::fillstatusmap_idx(CString &path,git_wc_status_kind status,void *pUserData)
{
	GitFolderStatus *Stat = (GitFolderStatus*)pUserData;

	FileStatusMap &cache = Stat->m_cache;
	FileStatusCacheEntry s;
	s.needslock = false;
	s.tree_conflict = false;

	s.author = Stat->authors.GetString(NULL);
	s.url = Stat->urls.GetString(NULL);
//	s.rev = -1;
	s.owner = Stat->owners.GetString(NULL);

	s.status = status;

	//s.status = GitStatus::GetMoreImportant(s.status, status->text_status);
	//s.status = GitStatus::GetMoreImportant(s.status, status->prop_status);
	//s.status = GitStatusFromWingit(pFile->nStatus);

	// TODO ?: s.blaha = pFile->nStage

	//s.lock = status->repos_lock;
	//s.tree_conflict = (status->tree_conflict != NULL);

	s.askedcounter = GITFOLDERSTATUS_CACHETIMES;
	//stdstring str;
	//if (pFile->sFileName)
	//{
	//	str = CUnicodeUtils::StdGetUnicode(pFile->sFileName);
	//	std::replace(str.begin(), str.end(), '/', '\\');
//MessageBox(NULL, str.c_str(), _T(""), MB_OK);
	//}
	//else
	//	str = _T(" ");
	if( path.Right(1) == _T("\\"))
	{
		path=path.Left(path.GetLength()-1);
	}
	stdstring str;
	str=path;
	cache[str] = s;

	return;
}

#if 0
git_error_t* GitFolderStatus::fillstatusmap(void * baton, const char * path, git_wc_status2_t * status, apr_pool_t * /*pool*/)
{
	GitFolderStatus * Stat = (GitFolderStatus *)baton;
	FileStatusMap * cache = &Stat->m_cache;
	FileStatusCacheEntry s;
	s.needslock = false;
	s.tree_conflict = false;
	if ((status)&&(status->entry))
	{
		s.author = Stat->authors.GetString(status->entry->cmt_author);
		s.url = Stat->urls.GetString(status->entry->url);
		s.rev = status->entry->cmt_rev;
		s.owner = Stat->owners.GetString(status->entry->lock_owner);
		if (status->entry->present_props)
			s.needslock = strstr(status->entry->present_props, "svn:needs-lock") ? true : false;
	}
	else
	{
		s.author = Stat->authors.GetString(NULL);
		s.url = Stat->urls.GetString(NULL);
		s.rev = -1;
		s.owner = Stat->owners.GetString(NULL);
	}
	s.status = git_wc_status_none;
	if (status)
	{
		s.status = GitStatus::GetMoreImportant(s.status, status->text_status);
		s.status = GitStatus::GetMoreImportant(s.status, status->prop_status);
		s.lock = status->repos_lock;
		s.tree_conflict = (status->tree_conflict != NULL);
	}
	s.askedcounter = GITFOLDERSTATUS_CACHETIMES;
	stdstring str;
	if (path)
	{
		str = CUnicodeUtils::StdGetUnicode(path);
		std::replace(str.begin(), str.end(), '/', '\\');
	}
	else
		str = _T(" ");
	(*cache)[str] = s;

	return GIT_NO_ERROR;
}

git_error_t* GitFolderStatus::findfolderstatus(void * baton, const char * path, git_wc_status2_t * status, apr_pool_t * /*pool*/)
{
	GitFolderStatus * Stat = (GitFolderStatus *)baton;
	if ((Stat)&&(Stat->folderpath.IsEquivalentTo(CTGitPath(CString(path)))))
	{
		Stat->dirstatus = status;
	}

	return GIT_NO_ERROR;
}
#endif

void GitFolderStatus::ClearCache()
{
	m_cache.clear();
	m_mostRecentStatus = NULL;
	m_mostRecentPath.Reset();
	// If we're about to rebuild the cache, there's no point hanging on to
	// an event which tells us that it's invalid
	ResetEvent(m_hInvalidationEvent);
}
