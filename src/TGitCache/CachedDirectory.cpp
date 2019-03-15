// TortoiseGit - a Windows shell extension for easy version control

// External Cache Copyright (C) 2005-2008 - TortoiseSVN
// Copyright (C) 2008-2019 - TortoiseGit

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
	m_directoryPath.UpdateCase();
	m_directoryPath.GetGitPathString(); // make sure git path string is set
}

BOOL CCachedDirectory::SaveToDisk(FILE * pFile)
{
	AutoLocker lock(m_critSec);
#define WRITEVALUETOFILE(x) if (fwrite(&x, sizeof(x), 1, pFile)!=1) return false;

	unsigned int value = GIT_CACHE_VERSION;
	WRITEVALUETOFILE(value);	// 'version' of this save-format
	value = static_cast<int>(m_entryCache.size());
	WRITEVALUETOFILE(value);	// size of the cache map
	// now iterate through the maps and save every entry.
	for (const auto& entry : m_entryCache)
	{
		const CString& key = entry.first;
		value = key.GetLength();
		WRITEVALUETOFILE(value);
		if (value)
		{
			if (fwrite(static_cast<LPCTSTR>(key), sizeof(TCHAR), value, pFile)!=value)
				return false;
			if (!entry.second.SaveToDisk(pFile))
				return false;
		}
	}
	value = static_cast<int>(m_childDirectories.size());
	WRITEVALUETOFILE(value);
	for (const auto& entry : m_childDirectories)
	{
		const CString& path = entry.first;
		value = path.GetLength();
		WRITEVALUETOFILE(value);
		if (value)
		{
			if (fwrite(static_cast<LPCTSTR>(path), sizeof(TCHAR), value, pFile)!=value)
				return false;
			git_wc_status_kind status = entry.second;
			WRITEVALUETOFILE(status);
		}
	}
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

				CGitStatusCache::Instance().AddFolderForCrawling(path);
				// also ask parent in case me might have missed some watcher update requests for our parent
				CTGitPath parentPath = path.GetContainingDirectory();
				if (!parentPath.IsEmpty())
				{
					auto parentEntry = CGitStatusCache::Instance().GetDirectoryCacheEntry(parentPath);
					if (parentEntry && parentEntry->GetCurrentFullStatus() > git_wc_status_unversioned)
						CGitStatusCache::Instance().AddFolderForCrawling(parentPath);
				}

				/*Return old status during crawling*/
				return dirEntry->GetOwnStatus(bRecursive);
			}
		}
		else
		{
			CGitStatusCache::Instance().AddFolderForCrawling(path);
			// also ask parent in case me might have missed some watcher update requests for our parent
			CTGitPath parentPath = path.GetContainingDirectory();
			if (!parentPath.IsEmpty())
			{
				auto parentEntry = CGitStatusCache::Instance().GetDirectoryCacheEntry(parentPath);
				if (parentEntry && parentEntry->GetCurrentFullStatus() > git_wc_status_unversioned)
					CGitStatusCache::Instance().AddFolderForCrawling(parentPath);
			}
		}
		return CStatusCacheEntry();
	}
	else
	{
		//All file ignored if under ignore directory
		if (m_ownStatus.GetEffectiveStatus() == git_wc_status_ignored)
			return CStatusCacheEntry(git_wc_status_ignored);

		// Look up a file in our own cache
		AutoLocker lock(m_critSec);
		CString strCacheKey = GetCacheKey(path);
		CacheEntryMap::iterator itMap = m_entryCache.find(strCacheKey);
		if(itMap != m_entryCache.end())
		{
			// We've hit the cache - check for timeout
			if (!itMap->second.HasExpired(static_cast<LONGLONG>(GetTickCount64())))
			{
				if (itMap->second.GetEffectiveStatus() == git_wc_status_ignored || itMap->second.GetEffectiveStatus() == git_wc_status_unversioned || itMap->second.DoesFileTimeMatch(path.GetLastWriteTime()))
				{
					// Note: the filetime matches after a modified has been committed too.
					// So in that case, we would return a wrong status (e.g. 'modified' instead
					// of 'normal') here.
					return itMap->second;
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

	if (EnumFiles(path, sProjectRoot, subpaths, isSelf))
	{
		// there was an error
		m_ownStatus = git_wc_status_none;
		m_currentFullStatus = git_wc_status_none;
		m_mostImportantFileStatus = git_wc_status_none;
		{
			AutoLocker lock(m_critSec);
			for (auto it = m_childDirectories.cbegin(); it != m_childDirectories.cend(); ++it)
				CGitStatusCache::Instance().AddFolderForCrawling(it->first);
			m_childDirectories.clear();
			m_entryCache.clear();
		}
		UpdateCurrentStatus();
		// make sure that this status times out soon.
		CGitStatusCache::Instance().m_folderCrawler.BlockPath(m_directoryPath, 20);
		CGitStatusCache::Instance().AddFolderForCrawling(m_directoryPath);
		return CStatusCacheEntry();
	}
	UpdateCurrentStatus();
	if (!path.IsDirectory())
		return GetCacheStatusForMember(path);
	return CStatusCacheEntry(m_ownStatus);
}

/// bFetch is true, fetch all status, call by crawl.
/// bFetch is false, get cache status, return quickly.

CStatusCacheEntry CCachedDirectory::GetStatusForMember(const CTGitPath& path, bool bRecursive,  bool bFetch /* = true */)
{
	bool bRequestForSelf = false;
	if(path.IsEquivalentToWithoutCase(m_directoryPath))
		bRequestForSelf = true;

	// In all most circumstances, we ask for the status of a member of this directory.
	ATLASSERT(m_directoryPath.IsEquivalentToWithoutCase(path.GetContainingDirectory()) || bRequestForSelf);

	if (GitAdminDir::IsAdminDirPath(path.GetWinPathString()))
	{
		// We're being asked for the status of an .git directory
		// It's not worth asking for this
		return CStatusCacheEntry();
	}

	if (bFetch)
	{
		CString sProjectRoot;
		{
			AutoLocker lock(m_critSec);
			// HasAdminDir(..., true) might modify m_directoryPath, so we need to do it synchronized (also write access to m_childDirectories, ... requires it)
			bool isVersioned = m_directoryPath.HasAdminDir(&sProjectRoot, true);
			if (!isVersioned && (bRequestForSelf || !path.IsDirectory()))
			{
				// shortcut if path is not versioned
				m_ownStatus = git_wc_status_none;
				m_mostImportantFileStatus = git_wc_status_none;
				for (auto it = m_childDirectories.cbegin(); it != m_childDirectories.cend(); ++it)
					CGitStatusCache::Instance().AddFolderForCrawling(it->first);
				m_childDirectories.clear();
				m_entryCache.clear();
				UpdateCurrentStatus();
				CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": %s is not underversion control\n", path.GetWinPath());
				return CStatusCacheEntry();
			}

		}
		if (!bRequestForSelf && path.IsDirectory() && !path.HasAdminDir(&sProjectRoot))
		{
			CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": %s is not underversion control\n", path.GetWinPath());
			return CStatusCacheEntry();
		}

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

	if (!path.IsDirectory())
	{
		git_wc_status2_t status = { git_wc_status_none, false, false };
		if (pStatus->GetFileStatus(sProjectRoot, sSubPath, status, TRUE, true))
		{
			// we could not get the status of a file, try whole directory after a short delay if the whole directory was not already crawled with an error
			if (m_currentFullStatus == git_wc_status_none)
				return -1;
			CGitStatusCache::Instance().m_folderCrawler.BlockPath(m_directoryPath, 20);
			CGitStatusCache::Instance().AddFolderForCrawling(m_directoryPath);
			return 0;
		}
		GetStatusCallback(path.GetWinPathString(), &status, false, path.GetLastWriteTime(true), this);
		RefreshMostImportant(false);
	}
	else
	{
		if (isSelf)
		{
			AutoLocker lock(m_critSec);
			// clear subdirectory status cache
			m_childDirectories_tmp.clear();
			// build new files status cache
			m_entryCache_tmp.clear();
		}

		m_mostImportantFileStatus = git_wc_status_none;
		git_wc_status_kind folderstatus = git_wc_status_unversioned;
		if (pStatus->EnumDirStatus(sProjectRoot, sSubPath, &folderstatus, GetStatusCallback, this))
			return -1;

		if (isSelf)
		{
			AutoLocker lock(m_critSec);
			// use a tmp files status cache so that we can still use the old cached values
			// for deciding whether we have to issue a shell notify
			m_entryCache = std::move(m_entryCache_tmp);
			m_childDirectories = std::move(m_childDirectories_tmp);
		}

		RefreshMostImportant(false);
		// need to update m_ownStatus
		m_ownStatus = folderstatus;
	}

	return 0;
}
void
CCachedDirectory::AddEntry(const CTGitPath& path, const git_wc_status2_t* pGitStatus, __int64 lastwritetime)
{
	if (!path.IsDirectory())
	{
		AutoLocker lock(m_critSec);
		CString cachekey = GetCacheKey(path);
		CacheEntryMap::iterator entry_it = m_entryCache.lower_bound(cachekey);
		if (entry_it != m_entryCache.end() && entry_it->first == cachekey)
		{
			if (pGitStatus)
			{
				if (entry_it->second.GetEffectiveStatus() > git_wc_status_none && entry_it->second.GetEffectiveStatus() != pGitStatus->status)
				{
					CGitStatusCache::Instance().UpdateShell(path);
					CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": shell update for %s\n", path.GetWinPath());
				}
			}
		}
		else
			entry_it = m_entryCache.insert(entry_it, std::make_pair(cachekey, CStatusCacheEntry()));
		entry_it->second = CStatusCacheEntry(pGitStatus, lastwritetime);
		m_entryCache_tmp[cachekey] = entry_it->second;
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

BOOL CCachedDirectory::GetStatusCallback(const CString& path, const git_wc_status2_t* pGitStatus, bool isDir, __int64 lastwritetime, void* baton)
{
	CTGitPath gitPath(path, isDir);

	auto pThis = reinterpret_cast<CCachedDirectory*>(baton);

	{
		if (isDir)
		{	/*gitpath is directory*/
			ATLASSERT(!gitPath.IsEquivalentToWithoutCase(pThis->m_directoryPath)); // this method does not get called four ourself
			//if ( !gitPath.IsEquivalentToWithoutCase(pThis->m_directoryPath) )
			{
				if (pThis->m_bRecursive)
				{
					// Add any versioned directory, which is not our 'self' entry, to the list for having its status updated
					if (pGitStatus->status >= git_wc_status_normal || (CGitStatusCache::Instance().IsUnversionedAsModified() && pGitStatus->status == git_wc_status_unversioned))
						CGitStatusCache::Instance().AddFolderForCrawling(gitPath);
				}

				// deleted subfolders are reported as modified whereas deleted submodules are reported as deleted
				if (pGitStatus->status == git_wc_status_deleted || pGitStatus->status == git_wc_status_modified)
				{
					pThis->SetChildStatus(gitPath.GetWinPathString(), pGitStatus->status);
					return FALSE;
				}

				// Make sure we know about this child directory
				// and keep the last known status so that we can use this
				// to check whether we need to refresh explorer
				pThis->KeepChildStatus(gitPath.GetWinPathString());
			}
		}
	}

	pThis->AddEntry(gitPath, pGitStatus, lastwritetime);

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

	// Now combine all our child-directorie's status
	AutoLocker lock(m_critSec);
	ChildDirStatus::const_iterator it;
	for(it = m_childDirectories.begin(); it != m_childDirectories.end(); ++it)
	{
		retVal = GitStatus::GetMoreImportant(retVal, it->second);
	}

	// folders can only be none, unversioned, normal, modified, and conflicted
	GitStatus::AdjustFolderStatus(retVal);

	if (retVal == git_wc_status_ignored && m_ownStatus.GetEffectiveStatus() != git_wc_status_ignored) // hack to show folders which have only ignored files inside but are not ignored themself
		retVal = git_wc_status_unversioned;

	return retVal;
}

// Update our composite status and deal with things if it's changed
void CCachedDirectory::UpdateCurrentStatus()
{
	git_wc_status_kind newStatus = CalculateRecursiveStatus();
	CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": UpdateCurrentStatus %s new:%d old: %d\n",
		m_directoryPath.GetWinPath(),
		newStatus, m_currentFullStatus);

	if (newStatus != m_currentFullStatus && IsOwnStatusValid())
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
		m_childDirectories_tmp[childDir.GetWinPathString()] = childStatus;
	}
	if ((currentStatus != childStatus)||(!IsOwnStatusValid()))
	{
		SetChildStatus(childDir.GetWinPathString(), childStatus);
		UpdateCurrentStatus();
	}
}

void CCachedDirectory::KeepChildStatus(const CString& childDir)
{
	AutoLocker lock(m_critSec);
	auto it = m_childDirectories.find(childDir);
	if (it != m_childDirectories.cend())
	{
		// if a submodule was deleted, we must not keep the deleted status if it re-appears - the deleted status cannot be reset otherwise
		// ATM only missing submodules are reported as deleted, so that this check only performed for submodules which were deleted
		if (it->second == git_wc_status_deleted)
		{
			CTGitPath child(childDir);
			CString root1, root2;
			if (child.HasAdminDir(&root1) && m_directoryPath.HasAdminDir(&root2) && !CPathUtils::ArePathStringsEqualWithCase(root1, root2))
				return;
		}
		m_childDirectories_tmp[childDir] = it->second;
	}
}

void CCachedDirectory::SetChildStatus(const CString& childDir, git_wc_status_kind childStatus)
{
	AutoLocker lock(m_critSec);
	m_childDirectories[childDir] = childStatus;
	m_childDirectories_tmp[childDir] = childStatus;
}

CStatusCacheEntry CCachedDirectory::GetOwnStatus(bool bRecursive)
{
	// Don't return recursive status if we're unversioned ourselves.
	if (bRecursive && m_ownStatus.GetEffectiveStatus() > git_wc_status_none)
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
	{
		AutoLocker lock(m_critSec);
		m_directoryPath.UpdateCase();
	}

	// Make sure that our own status is up-to-date
	GetStatusForMember(m_directoryPath,bRecursive);

	/*
	 * TSVNCache here checks whether m_entryCache is still up2date with the filesystem and refreshes all child directories.
	 * In the current TGitCache implementation, however, the file status is always fetched from git when GetStatusForMember is called from here (with fetch=true).
	 * Therefore, it is not necessary check whether the cache is still up to date since we just updated it.
	 */
}

void CCachedDirectory::RefreshMostImportant(bool bUpdateShell /* = true */)
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
	if (bUpdateShell && newStatus != m_mostImportantFileStatus)
	{
		CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": status change of path %s\n", m_directoryPath.GetWinPath());
		CGitStatusCache::Instance().UpdateShell(m_directoryPath);
	}
	m_mostImportantFileStatus = newStatus;
}
