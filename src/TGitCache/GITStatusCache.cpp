// TortoiseGit - a Windows shell extension for easy version control

// External Cache Copyright (C) 2005-2006,2008,2010,2014 - TortoiseSVN
// Copyright (C) 2008-2015 - TortoiseGit

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
#include "GitStatus.h"
#include "GitStatusCache.h"
#include "CacheInterface.h"
#include <ShlObj.h>
#include "PathUtils.h"

//////////////////////////////////////////////////////////////////////////

#define BLOCK_PATH_DEFAULT_TIMEOUT	600		// 10 minutes
#define BLOCK_PATH_MAX_TIMEOUT		1200	// 20 minutes

#define CACHEDISKVERSION 2

#ifdef _WIN64
#define STATUSCACHEFILENAME L"cache64"
#else
#define STATUSCACHEFILENAME L"cache"
#endif

CGitStatusCache* CGitStatusCache::m_pInstance;

CGitStatusCache& CGitStatusCache::Instance()
{
	ATLASSERT(m_pInstance != NULL);
	return *m_pInstance;
}

void CGitStatusCache::Create()
{
	ATLASSERT(m_pInstance == NULL);
	m_pInstance = new CGitStatusCache;

	m_pInstance->watcher.SetFolderCrawler(&m_pInstance->m_folderCrawler);

	if (!CRegStdDWORD(_T("Software\\TortoiseGit\\CacheSave"), TRUE))
		return;

#define LOADVALUEFROMFILE(x) if (fread(&x, sizeof(x), 1, pFile)!=1) goto exit;
#define LOADVALUEFROMFILE2(x) if (fread(&x, sizeof(x), 1, pFile)!=1) goto error;
	unsigned int value = (unsigned int)-1;
	// find the location of the cache
	CString path = CPathUtils::GetLocalAppDataDirectory();
	CString path2;
	if (!path.IsEmpty())
	{
		path += STATUSCACHEFILENAME;
		// in case the cache file is corrupt, we could crash while
		// reading it! To prevent crashing every time once that happens,
		// we make a copy of the cache file and use that copy to read from.
		// if that copy is corrupt, the original file won't exist anymore
		// and the second time we start up and try to read the file,
		// it's not there anymore and we start from scratch without a crash.
		path2 = path;
		path2 += _T("2");
		DeleteFile(path2);
		CopyFile(path, path2, FALSE);
		DeleteFile(path);
		CAutoFILE pFile = _tfsopen(path2, _T("rb"), _SH_DENYWR);
		if (pFile)
		{
			try
			{
				LOADVALUEFROMFILE(value);
				if (value != CACHEDISKVERSION)
				{
					goto error;
				}
				int mapsize = 0;
				LOADVALUEFROMFILE(mapsize);
				for (int i=0; i<mapsize; ++i)
				{
					LOADVALUEFROMFILE2(value);
					if (value > MAX_PATH)
						goto error;
					if (value)
					{
						CString sKey;
						if (fread(sKey.GetBuffer(value+1), sizeof(TCHAR), value, pFile)!=value)
						{
							sKey.ReleaseBuffer(0);
							goto error;
						}
						sKey.ReleaseBuffer(value);
						auto cacheddir = std::make_unique<CCachedDirectory>();
						if (!cacheddir.get() || !cacheddir->LoadFromDisk(pFile))
						{
							cacheddir.reset();
							goto error;
						}
						CTGitPath KeyPath = CTGitPath(sKey);
						if (m_pInstance->IsPathAllowed(KeyPath))
						{
							// only add the path to the watch list if it is versioned
							if ((cacheddir->GetCurrentFullStatus() != git_wc_status_unversioned)&&(cacheddir->GetCurrentFullStatus() != git_wc_status_none))
								m_pInstance->watcher.AddPath(KeyPath, false);

							m_pInstance->m_directoryCache[KeyPath] = cacheddir.release();

							// do *not* add the paths for crawling!
							// because crawled paths will trigger a shell
							// notification, which makes the desktop flash constantly
							// until the whole first time crawling is over
							// m_pInstance->AddFolderForCrawling(KeyPath);
						}
					}
				}
			}
			catch (CAtlException)
			{
				goto error;
			}
		}
	}
exit:
	DeleteFile(path2);
	m_pInstance->watcher.ClearInfoMap();
	CTraceToOutputDebugString::Instance()(__FUNCTION__ ": cache loaded from disk successfully!\n");
	return;
error:
	DeleteFile(path2);
	m_pInstance->watcher.ClearInfoMap();
	Destroy();
	m_pInstance = new CGitStatusCache;
	CTraceToOutputDebugString::Instance()(__FUNCTION__ ": cache not loaded from disk\n");
}

bool CGitStatusCache::SaveCache()
{
	if (!CRegStdDWORD(_T("Software\\TortoiseGit\\CacheSave"), TRUE))
		return false;

#define WRITEVALUETOFILE(x) if (fwrite(&x, sizeof(x), 1, pFile)!=1) goto error;
	unsigned int value = 0;
	// save the cache to disk
	// find a location to write the cache to
	CString path = CPathUtils::GetLocalAppDataDirectory();
	if (!path.IsEmpty())
	{
		path += STATUSCACHEFILENAME;
		CAutoFILE pFile = _tfsopen(path, _T("wb"), SH_DENYRW);
		if (pFile)
		{
			value = CACHEDISKVERSION;
			WRITEVALUETOFILE(value);
			value = (int)m_pInstance->m_directoryCache.size();
			WRITEVALUETOFILE(value);
			for (auto I = m_pInstance->m_directoryCache.cbegin(); I != m_pInstance->m_directoryCache.cend(); ++I)
			{
				if (I->second == NULL)
				{
					value = 0;
					WRITEVALUETOFILE(value);
					continue;
				}
				const CString& key = I->first.GetWinPathString();
				value = key.GetLength();
				WRITEVALUETOFILE(value);
				if (value)
				{
					if (fwrite((LPCTSTR)key, sizeof(TCHAR), value, pFile)!=value)
						goto error;
					if (!I->second->SaveToDisk(pFile))
						goto error;
				}
			}
		}
	}
	CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) _T(": cache saved to disk at %s\n"), (LPCTSTR)path);
	return true;
error:
	Destroy();
	DeleteFile(path);
	return false;
}

void CGitStatusCache::Destroy()
{
	if (m_pInstance)
	{
		m_pInstance->Stop();
		Sleep(100);
		delete m_pInstance;
		m_pInstance = NULL;
	}
}

void CGitStatusCache::Stop()
{
//	m_svnHelp.Cancel(true);
	watcher.Stop();
	m_folderCrawler.Stop();
	m_shellUpdater.Stop();
}

void CGitStatusCache::Init()
{
	m_folderCrawler.Initialise();
	m_shellUpdater.Initialise();
}

CGitStatusCache::CGitStatusCache(void)
{
	#define forever DWORD(-1)
	AutoLocker lock(m_NoWatchPathCritSec);
	KNOWNFOLDERID folderids[] = { FOLDERID_Cookies, FOLDERID_History, FOLDERID_InternetCache, FOLDERID_Windows, FOLDERID_CDBurning, FOLDERID_Fonts, FOLDERID_RecycleBinFolder }; //FOLDERID_SearchHistory
	for (KNOWNFOLDERID folderid : folderids)
		m_NoWatchPaths[CTGitPath(GetSpecialFolder(folderid))] = forever;
	m_bClearMemory = false;
	m_mostRecentExpiresAt = 0;
}

CGitStatusCache::~CGitStatusCache(void)
{
	CAutoWriteLock writeLock(m_guard);
	ClearCache();
}

void CGitStatusCache::Refresh()
{
	m_shellCache.ForceRefresh();
//	m_pInstance->m_svnHelp.ReloadConfig();
	if (!m_pInstance->m_directoryCache.empty())
	{
		auto I = m_pInstance->m_directoryCache.cbegin();
		for (/* no init */; I != m_pInstance->m_directoryCache.cend(); ++I)
		{
			if (m_shellCache.IsPathAllowed(I->first.GetWinPath()))
				I->second->RefreshMostImportant();
			else
			{
				CGitStatusCache::Instance().RemoveCacheForPath(I->first);
				I = m_pInstance->m_directoryCache.cbegin();
				if (I == m_pInstance->m_directoryCache.cend())
					break;
			}
		}
	}
}

bool CGitStatusCache::IsPathGood(const CTGitPath& path)
{
	AutoLocker lock(m_NoWatchPathCritSec);
	for (auto it = m_NoWatchPaths.cbegin(); it != m_NoWatchPaths.cend(); ++it)
	{
		// the ticks check is necessary here, because RemoveTimedoutBlocks is only called within the FolderCrawler loop
		// and we might miss update calls
		// TODO: maybe we also need to check if index.lock and HEAD.lock still exists after a specific timeout (if we miss update notifications for these files too often)
		if (GetTickCount64() < it->second && it->first.IsAncestorOf(path))
		{
			CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) _T(": path not good: %s\n"), it->first.GetWinPath());
			return false;
		}
	}
	return true;
}

bool CGitStatusCache::BlockPath(const CTGitPath& path, ULONGLONG timeout /* = 0 */)
{
	if (timeout == 0)
		timeout = BLOCK_PATH_DEFAULT_TIMEOUT;

	if (timeout > BLOCK_PATH_MAX_TIMEOUT)
		timeout = BLOCK_PATH_MAX_TIMEOUT;

	timeout = GetTickCount64() + (timeout * 1000);	// timeout is in seconds, but we need the milliseconds

	AutoLocker lock(m_NoWatchPathCritSec);
	m_NoWatchPaths[path.GetDirectory()] = timeout;

	return true;
}

bool CGitStatusCache::UnBlockPath(const CTGitPath& path)
{
	bool ret = false;
	AutoLocker lock(m_NoWatchPathCritSec);
	std::map<CTGitPath, ULONGLONG>::iterator it = m_NoWatchPaths.find(path.GetDirectory());
	if (it != m_NoWatchPaths.end())
	{
		CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) _T(": path removed from no good: %s\n"), it->first.GetWinPath());
		m_NoWatchPaths.erase(it);
		ret = true;
	}
	AddFolderForCrawling(path.GetDirectory());

	return ret;
}

bool CGitStatusCache::RemoveTimedoutBlocks()
{
	bool ret = false;
	ULONGLONG currentTicks = GetTickCount64();
	AutoLocker lock(m_NoWatchPathCritSec);
	std::vector<CTGitPath> toRemove;
	for (auto it = m_NoWatchPaths.cbegin(); it != m_NoWatchPaths.cend(); ++it)
	{
		if (currentTicks > it->second)
		{
			toRemove.push_back(it->first);
		}
	}
	if (!toRemove.empty())
	{
		for (auto it = toRemove.cbegin(); it != toRemove.cend(); ++it)
		{
			ret = ret || UnBlockPath(*it);
		}
	}

	return ret;
}

void CGitStatusCache::UpdateShell(const CTGitPath& path)
{
	if (path.IsEquivalentToWithoutCase(m_mostRecentPath))
		m_mostRecentExpiresAt = 0;
	m_shellUpdater.AddPathForUpdate(path);
}

void CGitStatusCache::ClearCache()
{
	CAutoWriteLock writeLock(m_guard);
	for (CCachedDirectory::CachedDirMap::iterator I = m_directoryCache.begin(); I != m_directoryCache.end(); ++I)
	{
		delete I->second;
		I->second = NULL;
	}
	m_directoryCache.clear();
}

bool CGitStatusCache::RemoveCacheForDirectory(CCachedDirectory * cdir)
{
	if (cdir == NULL)
		return false;

	typedef std::map<CTGitPath, git_wc_status_kind>  ChildDirStatus;
	CAutoWriteLock writeLock(m_guard);
	if (!cdir->m_childDirectories.empty())
	{
		ChildDirStatus::iterator it = cdir->m_childDirectories.begin();
		for (; it != cdir->m_childDirectories.end(); )
		{
			CCachedDirectory * childdir = CGitStatusCache::Instance().GetDirectoryCacheEntryNoCreate(it->first);
			if ((childdir) && (!cdir->m_directoryPath.IsEquivalentTo(childdir->m_directoryPath)) && (cdir->m_directoryPath.GetFileOrDirectoryName() != L".."))
				RemoveCacheForDirectory(childdir);
			cdir->m_childDirectories.erase(it->first);
			it = cdir->m_childDirectories.begin();
		}
	}
	cdir->m_childDirectories.clear();
	m_directoryCache.erase(cdir->m_directoryPath);

	// we could have entries versioned and/or stored in our cache which are
	// children of the specified directory, but not in the m_childDirectories
	// member
	CCachedDirectory::ItDir itMap = m_directoryCache.lower_bound(cdir->m_directoryPath);
	do
	{
		if (itMap != m_directoryCache.end())
		{
			if (cdir->m_directoryPath.IsAncestorOf(itMap->first))
			{
				// just in case (see TortoiseSVN issue #255)
				if (itMap->second == cdir)
				{
					m_directoryCache.erase(itMap);
				}
				else
					RemoveCacheForDirectory(itMap->second);
			}
		}
		itMap = m_directoryCache.lower_bound(cdir->m_directoryPath);
	} while (itMap != m_directoryCache.end() && cdir->m_directoryPath.IsAncestorOf(itMap->first));

	CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) _T(": removed from cache %s\n"), cdir->m_directoryPath.GetWinPath());
	delete cdir;
	return true;
}

void CGitStatusCache::RemoveCacheForPath(const CTGitPath& path)
{
	// Stop the crawler starting on a new folder
	CCrawlInhibitor crawlInhibit(&m_folderCrawler);
	CCachedDirectory::ItDir itMap;
	CCachedDirectory * dirtoremove = NULL;

	itMap = m_directoryCache.find(path);
	if ((itMap != m_directoryCache.end())&&(itMap->second))
		dirtoremove = itMap->second;
	if (dirtoremove == NULL)
		return;
	ATLASSERT(path.IsEquivalentToWithoutCase(dirtoremove->m_directoryPath));
	RemoveCacheForDirectory(dirtoremove);
}

CCachedDirectory * CGitStatusCache::GetDirectoryCacheEntry(const CTGitPath& path, bool isAddToWatch)
{
	ATLASSERT(path.IsDirectory() || !PathFileExists(path.GetWinPath()));


	CCachedDirectory::ItDir itMap;
	itMap = m_directoryCache.find(path);
	if ((itMap != m_directoryCache.end())&&(itMap->second))
	{
		// We've found this directory in the cache
		return itMap->second;
	}
	else
	{
		// if the CCachedDirectory is NULL but the path is in our cache,
		// that means that path got invalidated and needs to be treated
		// as if it never was in our cache. So we remove the last remains
		// from the cache and start from scratch.

		CAutoWriteLock writeLock(m_guard);
		// Since above there's a small chance that before we can upgrade to
		// writer state some other thread gained writer state and changed
		// the data, we have to recreate the iterator here again.
		itMap = m_directoryCache.find(path);
		if (itMap!=m_directoryCache.end())
		{
			delete itMap->second;
			m_directoryCache.erase(itMap);
		}
		// We don't know anything about this directory yet - lets add it to our cache
		// but only if it exists!
		if (path.Exists() && m_shellCache.IsPathAllowed(path.GetWinPath()) && !GitAdminDir::IsAdminDirPath(path.GetWinPath()))
		{
			// some notifications are for files which got removed/moved around.
			// In such cases, the CTGitPath::IsDirectory() will return true (it assumes a directory if
			// the path doesn't exist). Which means we can get here with a path to a file
			// instead of a directory.
			// Since we're here most likely called from the crawler thread, the file could exist
			// again. If that's the case, just do nothing
			if (path.IsDirectory()||(!path.Exists()))
			{
				CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) _T(": adding %s to our cache\n"), path.GetWinPath());
				CCachedDirectory * newcdir = new CCachedDirectory(path);
				if (newcdir)
				{
					CCachedDirectory * cdir = m_directoryCache.insert(m_directoryCache.lower_bound(path), std::make_pair(path, newcdir))->second;
					CString gitdir;
					if ((!path.IsEmpty())&&(path.HasAdminDir(&gitdir))&&isAddToWatch)
					{
						/* Just watch version path */
						watcher.AddPath(gitdir);
						watcher.AddPath(path);
					}
					return cdir;
				}
				m_bClearMemory = true;
			}
		}
		return NULL;
	}
}

CCachedDirectory * CGitStatusCache::GetDirectoryCacheEntryNoCreate(const CTGitPath& path)
{
	ATLASSERT(path.IsDirectory() || !PathFileExists(path.GetWinPath()));

	CCachedDirectory::ItDir itMap;
	itMap = m_directoryCache.find(path);
	if(itMap != m_directoryCache.end())
	{
		// We've found this directory in the cache
		return itMap->second;
	}
	return NULL;
}

/* Fetch is true, means fetch status from */
/* Fetch is false, means fetch status from cache */
CStatusCacheEntry CGitStatusCache::GetStatusForPath(const CTGitPath& path, DWORD flags,  bool bFetch /* = true */)
{
	bool bRecursive = !!(flags & TGITCACHE_FLAGS_RECUSIVE_STATUS);

	// Check a very short-lived 'mini-cache' of the last thing we were asked for.
	LONGLONG now = (LONGLONG)GetTickCount64();
	if(now-m_mostRecentExpiresAt < 0)
	{
		if(path.IsEquivalentToWithoutCase(m_mostRecentPath))
		{
			return m_mostRecentStatus;
		}
	}
	{
		AutoLocker lock(m_critSec);
		m_mostRecentPath = path;
		m_mostRecentExpiresAt = now + 1000;
	}

	if (IsPathGood(path))
	{
		// Stop the crawler starting on a new folder while we're doing this much more important task...
		// Please note, that this may be a second "lock" used concurrently to the one in RemoveCacheForPath().
		CCrawlInhibitor crawlInhibit(&m_folderCrawler);

		CTGitPath dirpath = path.GetContainingDirectory();
		if ((dirpath.IsEmpty()) || (!m_shellCache.IsPathAllowed(dirpath.GetWinPath())))
			dirpath = path.GetDirectory();
		CCachedDirectory * cachedDir = GetDirectoryCacheEntry(dirpath);
		if (cachedDir != NULL)
		{
			//CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) _T(": GetStatusForMember %d\n"), bFetch);
			CStatusCacheEntry entry = cachedDir->GetStatusForMember(path, bRecursive, bFetch);
			{
				AutoLocker lock(m_critSec);
				m_mostRecentStatus = entry;
				return m_mostRecentStatus;
			}
		}
	}
	else
	{
		// path is blocked for some reason: return the cached status if we have one
		// we do here only a cache search, absolutely no disk access is allowed!
		CCachedDirectory::ItDir itMap = m_directoryCache.find(path.GetDirectory());
		if ((itMap != m_directoryCache.end())&&(itMap->second))
		{
			if (path.IsDirectory())
			{
				CStatusCacheEntry entry = itMap->second->GetOwnStatus(false);
				AutoLocker lock(m_critSec);
				m_mostRecentStatus = entry;
				return m_mostRecentStatus;
			}
			else
			{
				// We've found this directory in the cache
				CCachedDirectory * cachedDir = itMap->second;
				CStatusCacheEntry entry = cachedDir->GetCacheStatusForMember(path);
				{
					AutoLocker lock(m_critSec);
					m_mostRecentStatus = entry;
					return m_mostRecentStatus;
				}
			}
		}
	}
	AutoLocker lock(m_critSec);
	CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) _T(": ignored no good path %s\n"), path.GetWinPath());
	m_mostRecentStatus = CStatusCacheEntry();
	if (m_shellCache.ShowExcludedAsNormal() && path.IsDirectory() && m_shellCache.HasGITAdminDir(path.GetWinPath(), true))
	{
		CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) _T(": force status %s\n"), path.GetWinPath());
		m_mostRecentStatus.ForceStatus(git_wc_status_normal);
	}
	return m_mostRecentStatus;
}

void CGitStatusCache::AddFolderForCrawling(const CTGitPath& path)
{
	m_folderCrawler.AddDirectoryForUpdate(path);
}

void CGitStatusCache::CloseWatcherHandles(HANDLE hFile)
{
	CTGitPath path = watcher.CloseInfoMap(hFile);
	if (!path.IsEmpty())
		m_folderCrawler.BlockPath(path);
	CGitStatusCache::Instance().m_GitStatus.ReleasePathsRecursively(path.GetWinPathString());
}

void CGitStatusCache::CloseWatcherHandles(const CTGitPath& path)
{
	watcher.CloseHandlesForPath(path);
	m_folderCrawler.ReleasePathForUpdate(path);
	CGitStatusCache::Instance().m_GitStatus.ReleasePathsRecursively(path.GetWinPathString());
}

CString CGitStatusCache::GetSpecialFolder(REFKNOWNFOLDERID rfid)
{
	PWSTR pszPath = nullptr;
	if (SHGetKnownFolderPath(rfid, KF_FLAG_CREATE, nullptr, &pszPath) != S_OK)
		return CString();

	CString path = pszPath;
	CoTaskMemFree(pszPath);
	return path;
}
