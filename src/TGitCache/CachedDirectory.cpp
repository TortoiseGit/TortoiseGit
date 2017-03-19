// TortoiseGit - a Windows shell extension for easy version control

// External Cache Copyright (C) 2005-2008 - TortoiseSVN
// Copyright (C) 2008-2017 - TortoiseGit

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
#include "CachedDirectory.h"
#include "GitAdminDir.h"
#include "GitStatusCache.h"
#include "PathUtils.h"
#include "GitStatus.h"
#include <set>

CCachedDirectory::CCachedDirectory(void)
	: m_currentFullStatus(git_wc_status_none)
	, m_mostImportantFileStatus(git_wc_status_none)
	, m_bRecursive(true)
{
}

CCachedDirectory::~CCachedDirectory(void)
{
}

CCachedDirectory::CCachedDirectory(const CTGitPath& directoryPath)
	: CCachedDirectory()
{
	ATLASSERT(directoryPath.IsDirectory() || !PathFileExists(directoryPath.GetWinPath()));

	directoryPath.HasAdminDir(); // make sure HasAdminDir is always initialized
	m_directoryPath = directoryPath;
	m_directoryPath.GetGitPathString(); // make sure git path string is set
}

BOOL CCachedDirectory::SaveToDisk(FILE * pFile)
{
	AutoLocker lock(m_critSec);
#define WRITEVALUETOFILE(x) if (fwrite(&x, sizeof(x), 1, pFile)!=1) return false;

	unsigned int value = GIT_CACHE_VERSION;
	WRITEVALUETOFILE(value);	// 'version' of this save-format
	value = (int)m_entryCache.size();
	WRITEVALUETOFILE(value);	// size of the cache map
	// now iterate through the maps and save every entry.
	for (const auto& entry : m_entryCache)
	{
		const CString& key = entry.first;
		value = key.GetLength();
		WRITEVALUETOFILE(value);
		if (value)
		{
			if (fwrite((LPCTSTR)key, sizeof(TCHAR), value, pFile)!=value)
				return false;
			if (!entry.second.SaveToDisk(pFile))
				return false;
		}
	}
	value = (int)m_childDirectories.size();
	WRITEVALUETOFILE(value);
	for (const auto& entry : m_childDirectories)
	{
		const CString& path = entry.first;
		value = path.GetLength();
		WRITEVALUETOFILE(value);
		if (value)
		{
			if (fwrite((LPCTSTR)path, sizeof(TCHAR), value, pFile)!=value)
				return false;
			git_wc_status_kind status = entry.second;
			WRITEVALUETOFILE(status);
		}
	}
//	WRITEVALUETOFILE(m_propsFileTime);
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
		if (value != GIT_CACHE_VERSION)
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
				// only read non empty keys (just needed for transition from old TGit clients)
				if (!sKey.IsEmpty())
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
				m_childDirectories[sPath] = status;
			}
		}
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
			// make sure paths do not end with backslash (just needed for transition from old TGit clients)
			if (sPath.GetLength() > 3 && sPath[sPath.GetLength() - 1] == L'\\')
				sPath.TrimRight(L'\\');
			m_directoryPath.SetFromWin(sPath);
			m_directoryPath.GetGitPathString(); // make sure git path string is set
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


CStatusCacheEntry CCachedDirectory::GetStatusFromCache(const CTGitPath& path, bool bRecursive)
{
	if(path.IsDirectory())
	{
		// We don't have directory status in our cache
		// Ask the directory if it knows its own status
		CCachedDirectory * dirEntry = CGitStatusCache::Instance().GetDirectoryCacheEntry(path);
		if( dirEntry)
		{
			if (dirEntry->IsOwnStatusValid())
				return dirEntry->GetOwnStatus(bRecursive);
			else
			{
				/* cache have outof date, need crawl again*/

				/*AutoLocker lock(dirEntry->m_critSec);
				ChildDirStatus::const_iterator it;
				for(it = dirEntry->m_childDirectories.begin(); it != dirEntry->m_childDirectories.end(); ++it)
				{
					CGitStatusCache::Instance().AddFolderForCrawling(it->first);
				}*/

				CGitStatusCache::Instance().AddFolderForCrawling(path);

				/*Return old status during crawling*/
				return dirEntry->GetOwnStatus(bRecursive);
			}
		}
		else
		{
			CGitStatusCache::Instance().AddFolderForCrawling(path);
		}
		return CStatusCacheEntry();
	}
	else
	{
		//All file ignored if under ignore directory
		if (m_ownStatus.GetEffectiveStatus() == git_wc_status_ignored)
			return CStatusCacheEntry(git_wc_status_ignored);
		if (m_ownStatus.GetEffectiveStatus() == git_wc_status_unversioned)
			return CStatusCacheEntry(git_wc_status_unversioned);

		// Look up a file in our own cache
		AutoLocker lock(m_critSec);
		CString strCacheKey = GetCacheKey(path);
		CacheEntryMap::iterator itMap = m_entryCache.find(strCacheKey);
		if(itMap != m_entryCache.end())
		{
			// We've hit the cache - check for timeout
			if (!itMap->second.HasExpired((LONGLONG)GetTickCount64()))
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

		CGitStatusCache::Instance().AddFolderForCrawling(path.GetContainingDirectory());
		return CStatusCacheEntry();
	}
}

CStatusCacheEntry CCachedDirectory::GetStatusFromGit(const CTGitPath &path, const CString& sProjectRoot, bool isSelf)
{
	CString subpaths;
	CString s = path.GetGitPathString();
	if (s.GetLength() > sProjectRoot.GetLength())
	{
		if (s[sProjectRoot.GetLength()] == L'/')
			subpaths = s.Right(s.GetLength() - sProjectRoot.GetLength() - 1);
		else
			subpaths = s.Right(s.GetLength() - sProjectRoot.GetLength());
	}

	GitStatus *pGitStatus = &CGitStatusCache::Instance().m_GitStatus;
	UNREFERENCED_PARAMETER(pGitStatus);

	bool isVersion =true;
	pGitStatus->IsUnderVersionControl(sProjectRoot, subpaths, path.IsDirectory(), &isVersion);
	if(!isVersion)
	{	//untracked file
		bool isDir = path.IsDirectory();
		bool isIgnoreFileChanged = pGitStatus->CheckAndUpdateIgnoreFiles(sProjectRoot, subpaths, isDir);

		if (isDir)
		{
			CCachedDirectory * dirEntry = CGitStatusCache::Instance().GetDirectoryCacheEntry(path,
											false); /* we needn't watch untracked directory*/

			if(dirEntry)
			{
				AutoLocker lock(dirEntry->m_critSec);

				git_wc_status_kind dirstatus = dirEntry->GetCurrentFullStatus() ;
				if (CGitStatusCache::Instance().IsUnversionedAsModified() || dirstatus == git_wc_status_none || dirstatus >= git_wc_status_normal || isIgnoreFileChanged)
				{/* status have not initialized*/
					bool isignore = pGitStatus->IsIgnored(sProjectRoot, subpaths, isDir);

					if (!isignore && CGitStatusCache::Instance().IsUnversionedAsModified())
					{
						dirEntry->EnumFiles(path, sProjectRoot, subpaths, isSelf);
						dirEntry->UpdateCurrentStatus();
						return CStatusCacheEntry(dirEntry->GetCurrentFullStatus());
					}

					git_wc_status2_t status2;
					status2.text_status = status2.prop_status =
						(isignore? git_wc_status_ignored:git_wc_status_unversioned);

					// we do not know anything about files here, all we know is that there are not versioned files in this dir
					dirEntry->m_mostImportantFileStatus = git_wc_status_none;
					dirEntry->m_ownStatus.SetKind(git_node_dir);
					dirEntry->m_ownStatus.SetStatus(&status2);
					dirEntry->m_currentFullStatus = status2.text_status;
				}
				return dirEntry->m_ownStatus;
			}

		}
		else /* path is file */
		{
			AutoLocker lock(m_critSec);
			CString strCacheKey = GetCacheKey(path);

			if (strCacheKey.IsEmpty())
				return CStatusCacheEntry();

			CacheEntryMap::iterator itMap = m_entryCache.find(strCacheKey);
			if(itMap == m_entryCache.end() || isIgnoreFileChanged)
			{
				git_wc_status2_t status2;
				bool isignore = pGitStatus->IsIgnored(sProjectRoot, subpaths, isDir);
				status2.text_status = status2.prop_status =
					(isignore? git_wc_status_ignored:git_wc_status_unversioned);
				AddEntry(path, &status2);
				return m_entryCache[strCacheKey];
			}
			else
			{
				return itMap->second;
			}
		}
		return CStatusCacheEntry();

	}
	else
	{
		EnumFiles(path, sProjectRoot, subpaths, isSelf);
		UpdateCurrentStatus();
		if (!path.IsDirectory())
			return GetCacheStatusForMember(path);
		return CStatusCacheEntry(m_ownStatus);
	}
}

/// bFetch is true, fetch all status, call by crawl.
/// bFetch is false, get cache status, return quickly.

CStatusCacheEntry CCachedDirectory::GetStatusForMember(const CTGitPath& path, bool bRecursive,  bool bFetch /* = true */)
{
	CString sProjectRoot;
	bool bIsVersionedPath;

	bool bRequestForSelf = false;
	if(path.IsEquivalentToWithoutCase(m_directoryPath))
	{
		bRequestForSelf = true;
		AutoLocker lock(m_critSec);
		// HasAdminDir might modify m_directoryPath, so we need to do it synchronized
		bIsVersionedPath = m_directoryPath.HasAdminDir(&sProjectRoot);
	}
	else
		bIsVersionedPath = path.HasAdminDir(&sProjectRoot);

	// In all most circumstances, we ask for the status of a member of this directory.
	ATLASSERT(m_directoryPath.IsEquivalentToWithoutCase(path.GetContainingDirectory()) || bRequestForSelf);

	//If is not version control path
	if( !bIsVersionedPath)
	{
		CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": %s is not underversion control\n", path.GetWinPath());
		return CStatusCacheEntry();
	}

	// We've not got this item in the cache - let's add it
	// We never bother asking SVN for the status of just one file, always for its containing directory

	if (GitAdminDir::IsAdminDirPath(path.GetWinPathString()))
	{
		// We're being asked for the status of an .git directory
		// It's not worth asking for this
		return CStatusCacheEntry();
	}


	if(bFetch)
	{
		return GetStatusFromGit(path, sProjectRoot, bRequestForSelf);
	}
	else
	{
		return GetStatusFromCache(path, bRecursive);
	}
}

CStatusCacheEntry CCachedDirectory::GetCacheStatusForMember(const CTGitPath& path)
{
	// no disk access!
	AutoLocker lock(m_critSec);
	CacheEntryMap::iterator itMap = m_entryCache.find(GetCacheKey(path));
	if(itMap != m_entryCache.end())
		return itMap->second;

	return CStatusCacheEntry();
}

int CCachedDirectory::EnumFiles(const CTGitPath& path, CString sProjectRoot, const CString& sSubPath, bool isSelf)
{
	CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": EnumFiles %s\n", path.GetWinPath());

	// strip "\" at the end, otherwise cache lookups for drives do not work correctly
	sProjectRoot.TrimRight(L'\\');

	GitStatus *pStatus = &CGitStatusCache::Instance().m_GitStatus;
	UNREFERENCED_PARAMETER(pStatus);
	git_wc_status_kind status = git_wc_status_none;

	if (!path.IsDirectory())
	{
		bool assumeValid = false;
		bool skipWorktree = false;
		pStatus->GetFileStatus(sProjectRoot, sSubPath, &status, TRUE, false, true, GetStatusCallback, this, &assumeValid, &skipWorktree);
		if (status < m_mostImportantFileStatus)
			RefreshMostImportant();
	}
	else
	{
		if (isSelf)
		{
			AutoLocker lock(m_critSec);
			// clear subdirectory status cache
			m_childDirectories.clear();
			// build new files status cache
			m_entryCache_tmp.clear();
		}

		m_mostImportantFileStatus = git_wc_status_none;
		pStatus->EnumDirStatus(sProjectRoot, sSubPath, &status, TRUE, false, true, GetStatusCallback, this);
		m_mostImportantFileStatus = GitStatus::GetMoreImportant(m_mostImportantFileStatus, status);

		if (isSelf)
		{
			AutoLocker lock(m_critSec);
			// use a tmp files status cache so that we can still use the old cached values
			// for deciding whether we have to issue a shell notify
			m_entryCache = std::move(m_entryCache_tmp);
			m_entryCache_tmp.clear();
		}

		// need to set/construct m_ownStatus (only unversioned and normal are valid values)
		m_ownStatus = git_wc_status_unversioned;
		m_ownStatus.SetKind(git_node_dir);
		if (m_mostImportantFileStatus > git_wc_status_unversioned)
		{
			git_wc_status2_t status2;
			status2.text_status = status2.prop_status = git_wc_status_normal;
			m_ownStatus.SetStatus(&status2);
		}
		else
		{
			if (::PathFileExists(m_directoryPath.GetWinPathString() + L"\\.git")) {
				git_wc_status2_t status2;
				status2.text_status = status2.prop_status = git_wc_status_normal;
				m_ownStatus.SetStatus(&status2);
			}
			else
			{
				git_wc_status2_t status2;
				status2.text_status = status2.prop_status = CalculateRecursiveStatus();
				m_ownStatus.SetStatus(&status2);
			}
		}
	}

	return 0;
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
			if ((childDir->GetCurrentFullStatus() != git_wc_status_missing) || !pGitStatus || (pGitStatus->text_status != git_wc_status_unversioned))
			{
				if(pGitStatus)
				{
					if(childDir->GetCurrentFullStatus() != GitStatus::GetMoreImportant(pGitStatus->prop_status, pGitStatus->text_status))
					{
						CGitStatusCache::Instance().UpdateShell(path);
						//CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": shell update for %s\n", path.GetWinPath());
						childDir->m_ownStatus.SetKind(git_node_dir);
						childDir->m_ownStatus.SetStatus(pGitStatus);
					}
				}
			}
			childDir->m_ownStatus.SetKind(git_node_dir);


		}
	}
	else
	{
		CCachedDirectory * childDir = CGitStatusCache::Instance().GetDirectoryCacheEntry(path.GetContainingDirectory());
		bool bNotified = false;

		if(!childDir)
			return ;

		AutoLocker lock2(childDir->m_critSec);
		CString cachekey = GetCacheKey(path);
		CacheEntryMap::iterator entry_it = childDir->m_entryCache.lower_bound(cachekey);
		if (entry_it != childDir->m_entryCache.end() && entry_it->first == cachekey)
		{
			if (pGitStatus)
			{
				if (entry_it->second.GetEffectiveStatus() > git_wc_status_none &&
					entry_it->second.GetEffectiveStatus() != GitStatus::GetMoreImportant(pGitStatus->prop_status, pGitStatus->text_status)
				)
				{
					bNotified =true;
				}
			}

		}
		else
		{
			entry_it = childDir->m_entryCache.insert(entry_it, std::make_pair(cachekey, CStatusCacheEntry()));
			bNotified = true;

		}
		entry_it->second = CStatusCacheEntry(pGitStatus, path.GetLastWriteTime(), path.IsReadOnly(), validuntil);
		// TEMP(?): git status doesn't not have "entry" that contains node type, so manually set as file
		entry_it->second.SetKind(git_node_file);

		childDir->m_entryCache_tmp[cachekey] = entry_it->second;

		if(bNotified)
		{
			CGitStatusCache::Instance().UpdateShell(path);
			//CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": shell update for %s\n", path.GetWinPath());
		}

		//CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Path Entry Add %s %s %s %d\n", path.GetWinPath(), cachekey, m_directoryPath.GetWinPath(), pGitStatus->text_status);
	}
}

CString
CCachedDirectory::GetCacheKey(const CTGitPath& path)
{
	// All we put into the cache as a key is just the end portion of the pathname
	// There's no point storing the path of the containing directory for every item
	return path.GetWinPathString().Mid(m_directoryPath.GetWinPathString().GetLength()).TrimLeft(L'\\');
}

CString
CCachedDirectory::GetFullPathString(const CString& cacheKey)
{
	CString fullpath(m_directoryPath.GetWinPathString());
	fullpath += L'\\';
	fullpath += cacheKey;
	return fullpath;
}

BOOL CCachedDirectory::GetStatusCallback(const CString & path, git_wc_status_kind status,bool isDir, void *, bool assumeValid, bool skipWorktree)
{
	git_wc_status2_t _status;
	git_wc_status2_t *status2 = &_status;

	status2->prop_status = status2->text_status = status;
	status2->assumeValid = assumeValid;
	status2->skipWorktree = skipWorktree;

	CTGitPath gitPath(path);

	CCachedDirectory *pThis = CGitStatusCache::Instance().GetDirectoryCacheEntry(gitPath.GetContainingDirectory());

	if (!pThis)
		return FALSE;

//	if(status->entry)
	{
		if (isDir)
		{	/*gitpath is directory*/
			//if ( !gitPath.IsEquivalentToWithoutCase(pThis->m_directoryPath) )
			{
				if (!gitPath.Exists())
				{
					CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Miss dir %s \n", gitPath.GetWinPath());
					pThis->m_mostImportantFileStatus = GitStatus::GetMoreImportant(pThis->m_mostImportantFileStatus, git_wc_status_deleted);
				}

				if ( status <  git_wc_status_normal)
				{
					if (::PathFileExists(path + L"\\.git"))
					{ // this is submodule
						CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": skip submodule %s\n", (LPCTSTR)path);
						return FALSE;
					}
				}
				if (pThis->m_bRecursive)
				{
					// Add any versioned directory, which is not our 'self' entry, to the list for having its status updated
//OutputDebugStringA("AddFolderCrawl: ");OutputDebugStringW(svnPath.GetWinPathString());OutputDebugStringA("\r\n");
					if (status >= git_wc_status_normal || (CGitStatusCache::Instance().IsUnversionedAsModified() && status == git_wc_status_unversioned))
						CGitStatusCache::Instance().AddFolderForCrawling(gitPath);
				}

				// Make sure we know about this child directory
				// This initial status value is likely to be overwritten from below at some point
				git_wc_status_kind s = GitStatus::GetMoreImportant(status2->text_status, status2->prop_status);

				// folders must not be displayed as added or deleted only as modified
				if (s == git_wc_status_deleted || s == git_wc_status_added)
					s = git_wc_status_modified;

				CCachedDirectory * cdir = CGitStatusCache::Instance().GetDirectoryCacheEntryNoCreate(gitPath);
				if (cdir)
				{
					// This child directory is already in our cache!
					// So ask this dir about its recursive status
					git_wc_status_kind st = GitStatus::GetMoreImportant(s, cdir->GetCurrentFullStatus());

					// only propagate real status of submodules to parent repo if enabled
					if (!CGitStatusCache::Instance().IsRecurseSubmodules())
					{
						CString root1, root2;
						bool pThisIsVersioned;
						{
							AutoLocker lock(pThis->m_critSec);
							pThisIsVersioned = pThis->m_directoryPath.HasAdminDir(&root1);
						}
						AutoLocker lock(cdir->m_critSec);
						if (pThisIsVersioned && cdir->m_directoryPath.HasAdminDir(&root2) && !CPathUtils::ArePathStringsEqualWithCase(root1, root2))
							st = s;
					}
					AutoLocker lock(pThis->m_critSec);
					pThis->m_childDirectories[gitPath.GetWinPathString()] = st;
					CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": call 1 Update dir %s %d\n", gitPath.GetWinPath(), st);
				}
				else
				{
					// the child directory is not in the cache. Create a new entry for it in the cache which is
					// initially 'unversioned'. But we added that directory to the crawling list above, which
					// means the cache will be updated soon.
					CGitStatusCache::Instance().GetDirectoryCacheEntry(gitPath);

					AutoLocker lock(pThis->m_critSec);
					pThis->m_childDirectories[gitPath.GetWinPathString()] = s;
					CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": call 2 Update dir %s %d\n", gitPath.GetWinPath(), s);
				}
			}
		}
		else /* gitpath is file*/
		{
			// Keep track of the most important status of all the files in this directory
			// Don't include subdirectories in this figure, because they need to provide their
			// own 'most important' value
			pThis->m_mostImportantFileStatus = GitStatus::GetMoreImportant(pThis->m_mostImportantFileStatus, status2->text_status);
			pThis->m_mostImportantFileStatus = GitStatus::GetMoreImportant(pThis->m_mostImportantFileStatus, status2->prop_status);
			if ((status2->text_status == git_wc_status_unversioned) && (CGitStatusCache::Instance().IsUnversionedAsModified()))
			{
				// treat unversioned files as modified
				if (pThis->m_mostImportantFileStatus != git_wc_status_added)
					pThis->m_mostImportantFileStatus = GitStatus::GetMoreImportant(pThis->m_mostImportantFileStatus, git_wc_status_modified);
			}
		}
	}

	pThis->AddEntry(gitPath, status2);

	return FALSE;
}

bool
CCachedDirectory::IsOwnStatusValid() const
{
	return m_ownStatus.HasBeenSet() && !m_ownStatus.HasExpired(GetTickCount64());
}

void CCachedDirectory::Invalidate()
{
	m_ownStatus.Invalidate();
}

git_wc_status_kind CCachedDirectory::CalculateRecursiveStatus()
{
	// Combine our OWN folder status with the most important of our *FILES'* status.
	git_wc_status_kind retVal = GitStatus::GetMoreImportant(m_mostImportantFileStatus, m_ownStatus.GetEffectiveStatus());

	// folders can only be none, unversioned, normal, modified, and conflicted
	if (retVal == git_wc_status_deleted || retVal == git_wc_status_added)
		retVal = git_wc_status_modified;

	// Now combine all our child-directorie's status
	AutoLocker lock(m_critSec);
	ChildDirStatus::const_iterator it;
	for(it = m_childDirectories.begin(); it != m_childDirectories.end(); ++it)
	{
		retVal = GitStatus::GetMoreImportant(retVal, it->second);
	}

	return retVal;
}

// Update our composite status and deal with things if it's changed
void CCachedDirectory::UpdateCurrentStatus()
{
	git_wc_status_kind newStatus = CalculateRecursiveStatus();
	CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": UpdateCurrentStatus %s new:%d old: %d\n",
		m_directoryPath.GetWinPath(),
		newStatus, m_currentFullStatus);

	if (newStatus != m_currentFullStatus && m_ownStatus.IsDirectory())
	{
		m_currentFullStatus = newStatus;

		// Our status has changed - tell the shell
		CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Dir %s, status change from %d to %d\n", m_directoryPath.GetWinPath(), m_currentFullStatus, newStatus);
		CGitStatusCache::Instance().UpdateShell(m_directoryPath);
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
		// just version controled directory need to cache.
		CString root1, root2;
		if (parentPath.HasAdminDir(&root1) && (CGitStatusCache::Instance().IsRecurseSubmodules() || m_directoryPath.HasAdminDir(&root2) && CPathUtils::ArePathStringsEqualWithCase(root1, root2)))
		{
			CCachedDirectory * cachedDir = CGitStatusCache::Instance().GetDirectoryCacheEntry(parentPath);
			if (cachedDir)
				cachedDir->UpdateChildDirectoryStatus(m_directoryPath, m_currentFullStatus);
		}
	}
}

// Receive a notification from a child that its status has changed
void CCachedDirectory::UpdateChildDirectoryStatus(const CTGitPath& childDir, git_wc_status_kind childStatus)
{
	git_wc_status_kind currentStatus = git_wc_status_none;
	{
		AutoLocker lock(m_critSec);
		currentStatus = m_childDirectories[childDir.GetWinPathString()];
	}
	if ((currentStatus != childStatus)||(!IsOwnStatusValid()))
	{
		{
			AutoLocker lock(m_critSec);
			m_childDirectories[childDir.GetWinPathString()] = childStatus;
		}
		UpdateCurrentStatus();
	}
}

CStatusCacheEntry CCachedDirectory::GetOwnStatus(bool bRecursive)
{
	// Don't return recursive status if we're unversioned ourselves.
	if(bRecursive && m_ownStatus.IsDirectory() && m_ownStatus.GetEffectiveStatus() != git_wc_status_ignored)
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
	ULONGLONG now = GetTickCount64();
	if (m_entryCache.empty())
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
					if (m_entryCache.empty())
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
	AutoLocker lock(m_critSec);
	CacheEntryMap::iterator itMembers;
	git_wc_status_kind newStatus = git_wc_status_unversioned;
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
		CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": status change of path %s\n", m_directoryPath.GetWinPath());
		CGitStatusCache::Instance().UpdateShell(m_directoryPath);
	}
	m_mostImportantFileStatus = newStatus;
}
