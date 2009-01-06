// TortoiseSVN - a Windows shell extension for easy version control

// External Cache Copyright (C) 2005-2008 - TortoiseSVN

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
#include "StdAfx.h"
#include ".\cacheddirectory.h"
//#include "SVNHelpers.h"
#include "GitStatusCache.h"
#include "GitStatus.h"
#include <set>

CCachedDirectory::CCachedDirectory(void)
{
	m_entriesFileTime = 0;
	m_propsFileTime = 0;
	m_currentStatusFetchingPathTicks = 0;
	m_bCurrentFullStatusValid = false;
	m_currentFullStatus = m_mostImportantFileStatus = git_wc_status_none;
	m_bRecursive = true;
}

CCachedDirectory::~CCachedDirectory(void)
{
}

CCachedDirectory::CCachedDirectory(const CTGitPath& directoryPath)
{
	ATLASSERT(directoryPath.IsDirectory() || !PathFileExists(directoryPath.GetWinPath()));

	m_directoryPath = directoryPath;
	m_entriesFileTime = 0;
	m_propsFileTime = 0;
	m_currentStatusFetchingPathTicks = 0;
	m_bCurrentFullStatusValid = false;
	m_currentFullStatus = m_mostImportantFileStatus = git_wc_status_none;
	m_bRecursive = true;
}

BOOL CCachedDirectory::SaveToDisk(FILE * pFile)
{
	AutoLocker lock(m_critSec);
#define WRITEVALUETOFILE(x) if (fwrite(&x, sizeof(x), 1, pFile)!=1) return false;

	unsigned int value = 1;
	WRITEVALUETOFILE(value);	// 'version' of this save-format
	value = (int)m_entryCache.size();
	WRITEVALUETOFILE(value);	// size of the cache map
	// now iterate through the maps and save every entry.
	for (CacheEntryMap::iterator I = m_entryCache.begin(); I != m_entryCache.end(); ++I)
	{
		const CString& key = I->first;
		value = key.GetLength();
		WRITEVALUETOFILE(value);
		if (value)
		{
			if (fwrite((LPCTSTR)key, sizeof(TCHAR), value, pFile)!=value)
				return false;
			if (!I->second.SaveToDisk(pFile))
				return false;
		}
	}
	value = (int)m_childDirectories.size();
	WRITEVALUETOFILE(value);
	for (ChildDirStatus::iterator I = m_childDirectories.begin(); I != m_childDirectories.end(); ++I)
	{
		const CString& path = I->first.GetWinPathString();
		value = path.GetLength();
		WRITEVALUETOFILE(value);
		if (value)
		{
			if (fwrite((LPCTSTR)path, sizeof(TCHAR), value, pFile)!=value)
				return false;
			git_wc_status_kind status = I->second;
			WRITEVALUETOFILE(status);
		}
	}
	WRITEVALUETOFILE(m_entriesFileTime);
	WRITEVALUETOFILE(m_propsFileTime);
	value = m_directoryPath.GetWinPathString().GetLength();
	WRITEVALUETOFILE(value);
	if (value)
	{
		if (fwrite(m_directoryPath.GetWinPath(), sizeof(TCHAR), value, pFile)!=value)
			return false;
	}
	if (!m_ownStatus.SaveToDisk(pFile))
		return false;
	WRITEVALUETOFILE(m_currentFullStatus);
	WRITEVALUETOFILE(m_mostImportantFileStatus);
	return true;
}

BOOL CCachedDirectory::LoadFromDisk(FILE * pFile)
{
	AutoLocker lock(m_critSec);
#define LOADVALUEFROMFILE(x) if (fread(&x, sizeof(x), 1, pFile)!=1) return false;
	try
	{
		unsigned int value = 0;
		LOADVALUEFROMFILE(value);
		if (value != 1)
			return false;		// not the correct version
		int mapsize = 0;
		LOADVALUEFROMFILE(mapsize);
		for (int i=0; i<mapsize; ++i)
		{
			LOADVALUEFROMFILE(value);
			if (value > MAX_PATH)
				return false;
			if (value)
			{
				CString sKey;
				if (fread(sKey.GetBuffer(value+1), sizeof(TCHAR), value, pFile)!=value)
				{
					sKey.ReleaseBuffer(0);
					return false;
				}
				sKey.ReleaseBuffer(value);
				CStatusCacheEntry entry;
				if (!entry.LoadFromDisk(pFile))
					return false;
				m_entryCache[sKey] = entry;
			}
		}
		LOADVALUEFROMFILE(mapsize);
		for (int i=0; i<mapsize; ++i)
		{
			LOADVALUEFROMFILE(value);
			if (value > MAX_PATH)
				return false;
			if (value)
			{
				CString sPath;
				if (fread(sPath.GetBuffer(value), sizeof(TCHAR), value, pFile)!=value)
				{
					sPath.ReleaseBuffer(0);
					return false;
				}
				sPath.ReleaseBuffer(value);
				git_wc_status_kind status;
				LOADVALUEFROMFILE(status);
				m_childDirectories[CTGitPath(sPath)] = status;
			}
		}
		LOADVALUEFROMFILE(m_entriesFileTime);
		LOADVALUEFROMFILE(m_propsFileTime);
		LOADVALUEFROMFILE(value);
		if (value > MAX_PATH)
			return false;
		if (value)
		{
			CString sPath;
			if (fread(sPath.GetBuffer(value+1), sizeof(TCHAR), value, pFile)!=value)
			{
				sPath.ReleaseBuffer(0);
				return false;
			}
			sPath.ReleaseBuffer(value);
			m_directoryPath.SetFromWin(sPath);
		}
		if (!m_ownStatus.LoadFromDisk(pFile))
			return false;

		LOADVALUEFROMFILE(m_currentFullStatus);
		LOADVALUEFROMFILE(m_mostImportantFileStatus);
	}
	catch ( CAtlException )
	{
		return false;
	}
	return true;

}

CStatusCacheEntry CCachedDirectory::GetStatusForMember(const CTGitPath& path, bool bRecursive,  bool bFetch /* = true */)
{
	CString strCacheKey;
	bool bThisDirectoryIsUnversioned = false;
	bool bRequestForSelf = false;
	if(path.IsEquivalentToWithoutCase(m_directoryPath))
	{
		bRequestForSelf = true;
	}

	// In all most circumstances, we ask for the status of a member of this directory.
	ATLASSERT(m_directoryPath.IsEquivalentToWithoutCase(path.GetContainingDirectory()) || bRequestForSelf);

	// Check if the entries file has been changed
	CTGitPath entriesFilePath(m_directoryPath);
	CTGitPath propsDirPath(m_directoryPath);
	if (g_GitAdminDir.IsVSNETHackActive())
	{
		entriesFilePath.AppendPathString(g_GitAdminDir.GetVSNETAdminDirName() + _T("\\entries"));
		propsDirPath.AppendPathString(g_GitAdminDir.GetVSNETAdminDirName() + _T("\\dir-props"));
	}
	else
	{
		entriesFilePath.AppendPathString(g_GitAdminDir.GetAdminDirName() + _T("\\entries"));
		propsDirPath.AppendPathString(g_GitAdminDir.GetAdminDirName() + _T("\\dir-props"));
	}
	if ( (m_entriesFileTime == entriesFilePath.GetLastWriteTime()) && ((entriesFilePath.GetLastWriteTime() == 0) || (m_propsFileTime == propsDirPath.GetLastWriteTime())) )
	{
		m_entriesFileTime = entriesFilePath.GetLastWriteTime();
		if (m_entriesFileTime)
			m_propsFileTime = propsDirPath.GetLastWriteTime();

		if(m_entriesFileTime == 0)
		{
			// We are a folder which is not in a working copy
			bThisDirectoryIsUnversioned = true;
			m_ownStatus.SetStatus(NULL);

			// If a user removes the .svn directory, we get here with m_entryCache
			// not being empty, but still us being unversioned
			if (!m_entryCache.empty())
			{
				m_entryCache.clear();
			}
			ATLASSERT(m_entryCache.empty());
			
			// However, a member *DIRECTORY* might be the top of WC
			// so we need to ask them to get their own status
			if(!path.IsDirectory())
			{
				if ((PathFileExists(path.GetWinPath()))||(bRequestForSelf))
					return CStatusCacheEntry();
				// the entry doesn't exist anymore! 
				// but we can't remove it from the cache here:
				// the GetStatusForMember() method is called only with a read
				// lock and not a write lock!
				// So mark it for crawling, and let the crawler remove it
				// later
				CGitStatusCache::Instance().AddFolderForCrawling(path.GetContainingDirectory());
				return CStatusCacheEntry();
			}
			else
			{
				// If we're in the special case of a directory being asked for its own status
				// and this directory is unversioned, then we should just return that here
				if(bRequestForSelf)
				{
					return CStatusCacheEntry();
				}
			}
		}

		if(path.IsDirectory())
		{
			// We don't have directory status in our cache
			// Ask the directory if it knows its own status
			CCachedDirectory * dirEntry = CGitStatusCache::Instance().GetDirectoryCacheEntry(path);
			if ((dirEntry)&&(dirEntry->IsOwnStatusValid()))
			{
				// To keep recursive status up to date, we'll request that children are all crawled again
				// This will be very quick if nothings changed, because it will all be cache hits
				if (bRecursive)
				{
					AutoLocker lock(dirEntry->m_critSec);
					ChildDirStatus::const_iterator it;
					for(it = dirEntry->m_childDirectories.begin(); it != dirEntry->m_childDirectories.end(); ++it)
					{
						CGitStatusCache::Instance().AddFolderForCrawling(it->first);
					}
				}

				return dirEntry->GetOwnStatus(bRecursive);
			}
		}
		else
		{
			{
				// if we currently are fetching the status of the directory
				// we want the status for, we just return an empty entry here
				// and don't wait for that fetching to finish.
				// That's because fetching the status can take a *really* long
				// time (e.g. if a commit is also in progress on that same
				// directory), and we don't want to make the explorer appear
				// to hang.
				AutoLocker pathlock(m_critSecPath);
				if ((!bFetch)&&(!m_currentStatusFetchingPath.IsEmpty()))
				{
					if ((m_currentStatusFetchingPath.IsAncestorOf(path))&&((m_currentStatusFetchingPathTicks + 1000)<GetTickCount()))
					{
						ATLTRACE(_T("returning empty status (status fetch in progress) for %s\n"), path.GetWinPath());
						m_currentFullStatus = m_mostImportantFileStatus = git_wc_status_none;
						return CStatusCacheEntry();
					}
				}
			}
			// Look up a file in our own cache
			AutoLocker lock(m_critSec);
			strCacheKey = GetCacheKey(path);
			CacheEntryMap::iterator itMap = m_entryCache.find(strCacheKey);
			if(itMap != m_entryCache.end())
			{
				// We've hit the cache - check for timeout
				if(!itMap->second.HasExpired((long)GetTickCount()))
				{
					if(itMap->second.DoesFileTimeMatch(path.GetLastWriteTime()))
					{
						if ((itMap->second.GetEffectiveStatus()!=git_wc_status_missing)||(!PathFileExists(path.GetWinPath())))
						{
							// Note: the filetime matches after a modified has been committed too.
							// So in that case, we would return a wrong status (e.g. 'modified' instead
							// of 'normal') here.
							return itMap->second;
						}
					}
				}
			}
		}
	}
	else
	{
		AutoLocker pathlock(m_critSecPath);
		if ((!bFetch)&&(!m_currentStatusFetchingPath.IsEmpty()))
		{
			if ((m_currentStatusFetchingPath.IsAncestorOf(path))&&((m_currentStatusFetchingPathTicks + 1000)<GetTickCount()))
			{
				ATLTRACE(_T("returning empty status (status fetch in progress) for %s\n"), path.GetWinPath());
				m_currentFullStatus = m_mostImportantFileStatus = git_wc_status_none;
				return CStatusCacheEntry();
			}
		}
		// if we're fetching the status for the explorer,
		// we don't refresh the status but use the one
		// we already have (to save time and make the explorer
		// more responsive in stress conditions).
		// We leave the refreshing to the crawler.
		if ((!bFetch)&&(m_entriesFileTime))
		{
			CGitStatusCache::Instance().AddFolderForCrawling(path.GetDirectory());
			return CStatusCacheEntry();
		}
		AutoLocker lock(m_critSec);
		m_entriesFileTime = entriesFilePath.GetLastWriteTime();
		m_propsFileTime = propsDirPath.GetLastWriteTime();
		m_entryCache.clear();
		strCacheKey = GetCacheKey(path);
	}

//	svn_opt_revision_t revision;
//	revision.kind = svn_opt_revision_unspecified;

	// We've not got this item in the cache - let's add it
	// We never bother asking SVN for the status of just one file, always for its containing directory

	if (g_GitAdminDir.IsAdminDirPath(path.GetWinPathString()))
	{
		// We're being asked for the status of an .SVN directory
		// It's not worth asking for this
		return CStatusCacheEntry();
	}

	{
		{
			AutoLocker pathlock(m_critSecPath);
			if ((!bFetch)&&(!m_currentStatusFetchingPath.IsEmpty()))
			{
				if ((m_currentStatusFetchingPath.IsAncestorOf(path))&&((m_currentStatusFetchingPathTicks + 1000)<GetTickCount()))
				{
					ATLTRACE(_T("returning empty status (status fetch in progress) for %s\n"), path.GetWinPath());
					m_currentFullStatus = m_mostImportantFileStatus = git_wc_status_none;
					return CStatusCacheEntry();
				}
			}
		}
//		SVNPool subPool(CGitStatusCache::Instance().m_svnHelp.Pool());
		{
			AutoLocker lock(m_critSec);
			m_mostImportantFileStatus = git_wc_status_none;
			m_childDirectories.clear();
			m_entryCache.clear();
			m_ownStatus.SetStatus(NULL);
			m_bRecursive = bRecursive;
		}
		if(!bThisDirectoryIsUnversioned)
		{
			{
				AutoLocker pathlock(m_critSecPath);
				m_currentStatusFetchingPath = m_directoryPath;
				m_currentStatusFetchingPathTicks = GetTickCount();
			}
			ATLTRACE(_T("svn_cli_stat for '%s' (req %s)\n"), m_directoryPath.GetWinPath(), path.GetWinPath());
			git_error_t* pErr;
#if 0
			 = svn_client_status4 (
				NULL,
				m_directoryPath.GetSVNApiPath(subPool),
				&revision,
				GetStatusCallback,
				this,
				svn_depth_immediates,
				TRUE,									//getall
				FALSE,
				TRUE,									//noignore
				FALSE,									//ignore externals
				NULL,									//changelists
				CGitStatusCache::Instance().m_svnHelp.ClientContext(),
				subPool
				);
#endif
			{
				AutoLocker pathlock(m_critSecPath);
				m_currentStatusFetchingPath.Reset();
			}
			ATLTRACE(_T("svn_cli_stat finished for '%s'\n"), m_directoryPath.GetWinPath(), path.GetWinPath());
			if(pErr)
			{
				// Handle an error
				// The most likely error on a folder is that it's not part of a WC
				// In most circumstances, this will have been caught earlier,
				// but in some situations, we'll get this error.
				// If we allow ourselves to fall on through, then folders will be asked
				// for their own status, and will set themselves as unversioned, for the 
				// benefit of future requests
//				ATLTRACE("svn_cli_stat err: '%s'\n", pErr->message);
//				svn_error_clear(pErr);
				// No assert here! Since we _can_ get here, an assertion is not an option!
				// Reasons to get here: 
				// - renaming a folder with many sub folders --> results in "not a working copy" if the revert
				//   happens between our checks and the svn_client_status() call.
				// - reverting a move/copy --> results in "not a working copy" (as above)
				if (!m_directoryPath.HasAdminDir())
				{
					m_currentFullStatus = m_mostImportantFileStatus = git_wc_status_none;
					return CStatusCacheEntry();
				}
				else
				{
					ATLTRACE("svn_cli_stat error, assume none status\n");
					// Since we only assume a none status here due to svn_client_status()
					// returning an error, make sure that this status times out soon.
					CGitStatusCache::Instance().m_folderCrawler.BlockPath(m_directoryPath, 2000);
					CGitStatusCache::Instance().AddFolderForCrawling(m_directoryPath);
					return CStatusCacheEntry();
				}
			}
		}
		else
		{
			ATLTRACE("Skipped SVN status for unversioned folder\n");
		}
	}
	// Now that we've refreshed our SVN status, we can see if it's 
	// changed the 'most important' status value for this directory.
	// If it has, then we should tell our parent
	UpdateCurrentStatus();

	if (path.IsDirectory())
	{
		CCachedDirectory * dirEntry = CGitStatusCache::Instance().GetDirectoryCacheEntry(path);
		if ((dirEntry)&&(dirEntry->IsOwnStatusValid()))
		{
			CGitStatusCache::Instance().AddFolderForCrawling(path);
			return dirEntry->GetOwnStatus(bRecursive);
		}

		// If the status *still* isn't valid here, it means that 
		// the current directory is unversioned, and we shall need to ask its children for info about themselves
		if (dirEntry)
			return dirEntry->GetStatusForMember(path,bRecursive);
		CGitStatusCache::Instance().AddFolderForCrawling(path);
		return CStatusCacheEntry();
	}
	else
	{
		CacheEntryMap::iterator itMap = m_entryCache.find(strCacheKey);
		if(itMap != m_entryCache.end())
		{
			return itMap->second;
		}
	}

	AddEntry(path, NULL);
	return CStatusCacheEntry();
}

void 
CCachedDirectory::AddEntry(const CTGitPath& path, const git_wc_status2_t* pGitStatus, DWORD validuntil /* = 0*/)
{
	AutoLocker lock(m_critSec);
	if(path.IsDirectory())
	{
		CCachedDirectory * childDir = CGitStatusCache::Instance().GetDirectoryCacheEntry(path);
		if (childDir)
		{
			if ((childDir->GetCurrentFullStatus() != git_wc_status_missing)||(pGitStatus==NULL)||(pGitStatus->text_status != git_wc_status_unversioned))
				childDir->m_ownStatus.SetStatus(pGitStatus);
//			childDir->m_ownStatus.SetKind(svn_node_dir);
		}
	}
	else
	{
		CString cachekey = GetCacheKey(path);
		CacheEntryMap::iterator entry_it = m_entryCache.lower_bound(cachekey);
		if (entry_it != m_entryCache.end() && entry_it->first == cachekey)
		{
			if (pGitStatus)
			{
				if (entry_it->second.GetEffectiveStatus() > git_wc_status_none &&
					entry_it->second.GetEffectiveStatus() != GitStatus::GetMoreImportant(pGitStatus->prop_status, pGitStatus->text_status))
				{
					CGitStatusCache::Instance().UpdateShell(path);
					ATLTRACE(_T("shell update for %s\n"), path.GetWinPath());
				}
			}
		}
		else
		{
			entry_it = m_entryCache.insert(entry_it, std::make_pair(cachekey, CStatusCacheEntry()));
		}
		entry_it->second = CStatusCacheEntry(pGitStatus, path.GetLastWriteTime(), path.IsReadOnly(), validuntil);
	}
}


CString 
CCachedDirectory::GetCacheKey(const CTGitPath& path)
{
	// All we put into the cache as a key is just the end portion of the pathname
	// There's no point storing the path of the containing directory for every item
	return path.GetWinPathString().Mid(m_directoryPath.GetWinPathString().GetLength());
}

CString 
CCachedDirectory::GetFullPathString(const CString& cacheKey)
{
	return m_directoryPath.GetWinPathString() + _T("\\") + cacheKey;
}

git_error_t * CCachedDirectory::GetStatusCallback(void *baton, const char *path, git_wc_status2_t *status)
{
	CCachedDirectory* pThis = (CCachedDirectory*)baton;

	if (path == NULL)
		return 0;
		
	CTGitPath svnPath;

#if 0
	if(status->entry)
	{
		if ((status->text_status != git_wc_status_none)&&(status->text_status != git_wc_status_missing))
			svnPath.SetFromSVN(path, (status->entry->kind == svn_node_dir));
		else
			svnPath.SetFromSVN(path);

		if(svnPath.IsDirectory())
		{
			if(!svnPath.IsEquivalentToWithoutCase(pThis->m_directoryPath))
			{
				if (pThis->m_bRecursive)
				{
					// Add any versioned directory, which is not our 'self' entry, to the list for having its status updated
					CGitStatusCache::Instance().AddFolderForCrawling(svnPath);
				}

				// Make sure we know about this child directory
				// This initial status value is likely to be overwritten from below at some point
				git_wc_status_kind s = GitStatus::GetMoreImportant(status->text_status, status->prop_status);
				CCachedDirectory * cdir = CGitStatusCache::Instance().GetDirectoryCacheEntryNoCreate(svnPath);
				if (cdir)
				{
					// This child directory is already in our cache!
					// So ask this dir about its recursive status
					git_wc_status_kind st = GitStatus::GetMoreImportant(s, cdir->GetCurrentFullStatus());
					AutoLocker lock(pThis->m_critSec);
					pThis->m_childDirectories[svnPath] = st;
				}
				else
				{
					// the child directory is not in the cache. Create a new entry for it in the cache which is
					// initially 'unversioned'. But we added that directory to the crawling list above, which
					// means the cache will be updated soon.
					CGitStatusCache::Instance().GetDirectoryCacheEntry(svnPath);
					AutoLocker lock(pThis->m_critSec);
					pThis->m_childDirectories[svnPath] = s;
				}
			}
		}
		else
		{
			// Keep track of the most important status of all the files in this directory
			// Don't include subdirectories in this figure, because they need to provide their 
			// own 'most important' value
			pThis->m_mostImportantFileStatus = GitStatus::GetMoreImportant(pThis->m_mostImportantFileStatus, status->text_status);
			pThis->m_mostImportantFileStatus = GitStatus::GetMoreImportant(pThis->m_mostImportantFileStatus, status->prop_status);
			if (((status->text_status == git_wc_status_unversioned)||(status->text_status == git_wc_status_none))
				&&(CGitStatusCache::Instance().IsUnversionedAsModified()))
			{
				// treat unversioned files as modified
				if (pThis->m_mostImportantFileStatus != git_wc_status_added)
					pThis->m_mostImportantFileStatus = GitStatus::GetMoreImportant(pThis->m_mostImportantFileStatus, git_wc_status_modified);
			}
		}
	}
	else
	{
		svnPath.SetFromSVN(path);
		// Subversion returns no 'entry' field for versioned folders if they're
		// part of another working copy (nested layouts).
		// So we have to make sure that such an 'unversioned' folder really
		// is unversioned.
		if (((status->text_status == git_wc_status_unversioned)||(status->text_status == git_wc_status_missing))&&(!svnPath.IsEquivalentToWithoutCase(pThis->m_directoryPath))&&(svnPath.IsDirectory()))
		{
			if (svnPath.HasAdminDir())
			{
				CGitStatusCache::Instance().AddFolderForCrawling(svnPath);
				// Mark the directory as 'versioned' (status 'normal' for now).
				// This initial value will be overwritten from below some time later
				{
					AutoLocker lock(pThis->m_critSec);
					pThis->m_childDirectories[svnPath] = git_wc_status_normal;
				}
				// Make sure the entry is also in the cache
				CGitStatusCache::Instance().GetDirectoryCacheEntry(svnPath);
				// also mark the status in the status object as normal
				status->text_status = git_wc_status_normal;
			}
		}
		else if (status->text_status == git_wc_status_external)
		{
			CGitStatusCache::Instance().AddFolderForCrawling(svnPath);
			// Mark the directory as 'versioned' (status 'normal' for now).
			// This initial value will be overwritten from below some time later
			{
				AutoLocker lock(pThis->m_critSec);
				pThis->m_childDirectories[svnPath] = git_wc_status_normal;
			}
			// we have added a directory to the child-directory list of this
			// directory. We now must make sure that this directory also has
			// an entry in the cache.
			CGitStatusCache::Instance().GetDirectoryCacheEntry(svnPath);
			// also mark the status in the status object as normal
			status->text_status = git_wc_status_normal;
		}
		else
		{
			if (svnPath.IsDirectory())
			{
				AutoLocker lock(pThis->m_critSec);
				pThis->m_childDirectories[svnPath] = GitStatus::GetMoreImportant(status->text_status, status->prop_status);
			}
			else if ((CGitStatusCache::Instance().IsUnversionedAsModified())&&(status->text_status != git_wc_status_missing))
			{
				// make this unversioned item change the most important status of this
				// folder to modified if it doesn't already have another status
				if (pThis->m_mostImportantFileStatus != git_wc_status_added)
					pThis->m_mostImportantFileStatus = GitStatus::GetMoreImportant(pThis->m_mostImportantFileStatus, git_wc_status_modified);
			}
		}
	}
#endif
	pThis->AddEntry(svnPath, status);

	return 0;
}

bool 
CCachedDirectory::IsOwnStatusValid() const
{
	return m_ownStatus.HasBeenSet() && 
		   !m_ownStatus.HasExpired(GetTickCount()) &&
		   // 'external' isn't a valid status. That just
		   // means the folder is not part of the current working
		   // copy but it still has its own 'real' status
		   m_ownStatus.GetEffectiveStatus()!=git_wc_status_external &&
		   m_ownStatus.IsKindKnown();
}

void CCachedDirectory::Invalidate()
{
	m_ownStatus.Invalidate();
}

git_wc_status_kind CCachedDirectory::CalculateRecursiveStatus()
{
	// Combine our OWN folder status with the most important of our *FILES'* status.
	git_wc_status_kind retVal = GitStatus::GetMoreImportant(m_mostImportantFileStatus, m_ownStatus.GetEffectiveStatus());

	if ((retVal != git_wc_status_modified)&&(retVal != m_ownStatus.GetEffectiveStatus()))
	{
		if ((retVal == git_wc_status_added)||(retVal == git_wc_status_deleted)||(retVal == git_wc_status_missing))
			retVal = git_wc_status_modified;
	}

	// Now combine all our child-directorie's status
	
	AutoLocker lock(m_critSec);
	ChildDirStatus::const_iterator it;
	for(it = m_childDirectories.begin(); it != m_childDirectories.end(); ++it)
	{
		retVal = GitStatus::GetMoreImportant(retVal, it->second);
		if ((retVal != git_wc_status_modified)&&(retVal != m_ownStatus.GetEffectiveStatus()))
		{
			if ((retVal == git_wc_status_added)||(retVal == git_wc_status_deleted)||(retVal == git_wc_status_missing))
				retVal = git_wc_status_modified;
		}
	}
	
	return retVal;
}

// Update our composite status and deal with things if it's changed
void CCachedDirectory::UpdateCurrentStatus()
{
	git_wc_status_kind newStatus = CalculateRecursiveStatus();

	if ((newStatus != m_currentFullStatus)&&(m_ownStatus.IsVersioned()))
	{
		if ((m_currentFullStatus != git_wc_status_none)&&(m_ownStatus.GetEffectiveStatus() != git_wc_status_missing))
		{
			// Our status has changed - tell the shell
			ATLTRACE(_T("Dir %s, status change from %d to %d, send shell notification\n"), m_directoryPath.GetWinPath(), m_currentFullStatus, newStatus);		
			CGitStatusCache::Instance().UpdateShell(m_directoryPath);
		}
		if (m_ownStatus.GetEffectiveStatus() != git_wc_status_missing)
			m_currentFullStatus = newStatus;
		else
			m_currentFullStatus = git_wc_status_missing;
	}
	// And tell our parent, if we've got one...
	// we tell our parent *always* about our status, even if it hasn't
	// changed. This is to make sure that the parent has really our current
	// status - the parent can decide itself if our status has changed
	// or not.
	CTGitPath parentPath = m_directoryPath.GetContainingDirectory();
	if(!parentPath.IsEmpty())
	{
		// We have a parent
		CCachedDirectory * cachedDir = CGitStatusCache::Instance().GetDirectoryCacheEntry(parentPath);
		if (cachedDir)
			cachedDir->UpdateChildDirectoryStatus(m_directoryPath, m_currentFullStatus);
	}
}


// Receive a notification from a child that its status has changed
void CCachedDirectory::UpdateChildDirectoryStatus(const CTGitPath& childDir, git_wc_status_kind childStatus)
{
	git_wc_status_kind currentStatus = git_wc_status_none;
	{
		AutoLocker lock(m_critSec);
		currentStatus = m_childDirectories[childDir];
	}
	if ((currentStatus != childStatus)||(!IsOwnStatusValid()))
	{
		{
			AutoLocker lock(m_critSec);
			m_childDirectories[childDir] = childStatus;
		}
		UpdateCurrentStatus();
	}
}

CStatusCacheEntry CCachedDirectory::GetOwnStatus(bool bRecursive)
{
	// Don't return recursive status if we're unversioned ourselves.
	if(bRecursive && m_ownStatus.GetEffectiveStatus() > git_wc_status_unversioned)
	{
		CStatusCacheEntry recursiveStatus(m_ownStatus);
		UpdateCurrentStatus();
		recursiveStatus.ForceStatus(m_currentFullStatus);
		return recursiveStatus;				
	}
	else
	{
		return m_ownStatus;
	}
}

void CCachedDirectory::RefreshStatus(bool bRecursive)
{
	// Make sure that our own status is up-to-date
	GetStatusForMember(m_directoryPath,bRecursive);

	AutoLocker lock(m_critSec);
	// We also need to check if all our file members have the right date on them
	CacheEntryMap::iterator itMembers;
	std::set<CTGitPath> refreshedpaths;
	DWORD now = GetTickCount();
	if (m_entryCache.size() == 0)
		return;
	for (itMembers = m_entryCache.begin(); itMembers != m_entryCache.end(); ++itMembers)
	{
		if (itMembers->first)
		{
			CTGitPath filePath(m_directoryPath);
			filePath.AppendPathString(itMembers->first);
			std::set<CTGitPath>::iterator refr_it;
			if ((!filePath.IsEquivalentToWithoutCase(m_directoryPath))&&
				(((refr_it = refreshedpaths.lower_bound(filePath)) == refreshedpaths.end()) || !filePath.IsEquivalentToWithoutCase(*refr_it)))
			{
				if ((itMembers->second.HasExpired(now))||(!itMembers->second.DoesFileTimeMatch(filePath.GetLastWriteTime())))
				{
					lock.Unlock();
					// We need to request this item as well
					GetStatusForMember(filePath,bRecursive);
					// GetStatusForMember now has recreated the m_entryCache map.
					// So start the loop again, but add this path to the refreshed paths set
					// to make sure we don't refresh this path again. This is to make sure
					// that we don't end up in an endless loop.
					lock.Lock();
					refreshedpaths.insert(refr_it, filePath);
					itMembers = m_entryCache.begin();
					if (m_entryCache.size()==0)
						return;
					continue;
				}
				else if ((bRecursive)&&(itMembers->second.IsDirectory()))
				{
					// crawl all sub folders too! Otherwise a change deep inside the
					// tree which has changed won't get propagated up the tree.
					CGitStatusCache::Instance().AddFolderForCrawling(filePath);
				}
			}
		}
	}
}

void CCachedDirectory::RefreshMostImportant()
{
	CacheEntryMap::iterator itMembers;
	git_wc_status_kind newStatus = m_ownStatus.GetEffectiveStatus();
	for (itMembers = m_entryCache.begin(); itMembers != m_entryCache.end(); ++itMembers)
	{
		newStatus = GitStatus::GetMoreImportant(newStatus, itMembers->second.GetEffectiveStatus());
		if (((itMembers->second.GetEffectiveStatus() == git_wc_status_unversioned)||(itMembers->second.GetEffectiveStatus() == git_wc_status_none))
			&&(CGitStatusCache::Instance().IsUnversionedAsModified()))
		{
			// treat unversioned files as modified
			if (newStatus != git_wc_status_added)
				newStatus = GitStatus::GetMoreImportant(newStatus, git_wc_status_modified);
		}
	}
	if (newStatus != m_mostImportantFileStatus)
	{
		ATLTRACE(_T("status change of path %s\n"), m_directoryPath.GetWinPath());
		CGitStatusCache::Instance().UpdateShell(m_directoryPath);
	}
	m_mostImportantFileStatus = newStatus;
}
