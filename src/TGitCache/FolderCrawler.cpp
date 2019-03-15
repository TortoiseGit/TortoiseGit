// TortoiseGit - a Windows shell extension for easy version control

// External Cache Copyright (C) 2005-2008,2011,2014 - TortoiseSVN
// Copyright (C) 2008-2014, 2016-2019 - TortoiseGit

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
#include "FolderCrawler.h"
#include "GitStatusCache.h"
#include "registry.h"
#include "TGitCache.h"
#include <ShlObj.h>

CFolderCrawler::CFolderCrawler(void)
{
	m_hWakeEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	m_hTerminationEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
	m_lCrawlInhibitSet = 0;
	m_crawlHoldoffReleasesAt = static_cast<LONGLONG>(GetTickCount64());
	m_bRun = false;
	m_bPathsAddedSinceLastCrawl = false;
	m_bItemsAddedSinceLastCrawl = false;
	m_blockReleasesAt = 0;
}

CFolderCrawler::~CFolderCrawler(void)
{
	Stop();
}

void CFolderCrawler::Stop()
{
	m_bRun = false;
	if (m_hTerminationEvent)
	{
		SetEvent(m_hTerminationEvent);
		if (m_hThread.IsValid() && WaitForSingleObject(m_hThread, 4000) != WAIT_OBJECT_0)
			CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Error terminating crawler thread\n");
	}
	m_hThread.CloseHandle();
	m_hTerminationEvent.CloseHandle();
	m_hWakeEvent.CloseHandle();
}

void CFolderCrawler::Initialise()
{
	// Don't call Initialize more than once
	ATLASSERT(!m_hThread);

	// Just start the worker thread.
	// It will wait for event being signaled.
	// If m_hWakeEvent is already signaled the worker thread
	// will behave properly (with normal priority at worst).

	m_bRun = true;
	unsigned int threadId;
	m_hThread = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, ThreadEntry, this, 0, &threadId));
	SetThreadPriority(m_hThread, THREAD_PRIORITY_BELOW_NORMAL);
}

void CFolderCrawler::AddDirectoryForUpdate(const CTGitPath& path)
{
	/* Index file changing*/
	if (GitStatus::IsExistIndexLockFile(path.GetWinPathString()))
		return;

	if (!CGitStatusCache::Instance().IsPathGood(path))
		return;
	{
		CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": AddDirectoryForUpdate %s\n", path.GetWinPath());

		AutoLocker lock(m_critSec);

		m_foldersToUpdate.Push(path);

		//ATLASSERT(path.IsDirectory() || !path.Exists());
		// set this flag while we are sync'ed
		// with the worker thread
		m_bItemsAddedSinceLastCrawl = true;
	}
	//if (SetHoldoff())
		SetEvent(m_hWakeEvent);
}

void CFolderCrawler::AddPathForUpdate(const CTGitPath& path)
{
	/* Index file changing*/
	if (GitStatus::IsExistIndexLockFile(path.GetWinPathString()))
		return;

	{
		AutoLocker lock(m_critSec);

		m_pathsToUpdate.Push(path);
		m_bPathsAddedSinceLastCrawl = true;
	}
	//if (SetHoldoff())
		SetEvent(m_hWakeEvent);
}

void CFolderCrawler::ReleasePathForUpdate(const CTGitPath& path)
{
	CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": ReleasePathForUpdate %s\n", path.GetWinPath());
	AutoLocker lock(m_critSec);

	m_pathsToRelease.Push(path);
	SetEvent(m_hWakeEvent);
}

unsigned int CFolderCrawler::ThreadEntry(void* pContext)
{
	reinterpret_cast<CFolderCrawler*>(pContext)->WorkerThread();
	return 0;
}

void CFolderCrawler::WorkerThread()
{
	HANDLE hWaitHandles[2];
	hWaitHandles[0] = m_hTerminationEvent;
	hWaitHandles[1] = m_hWakeEvent;
	CTGitPath workingPath;
	ULONGLONG currentTicks = 0;

	for(;;)
	{
		bool bRecursive = !!static_cast<DWORD>(CRegStdDWORD(L"Software\\TortoiseGit\\RecursiveOverlay", TRUE));

		SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_END);

		DWORD waitResult = WaitForMultipleObjects(_countof(hWaitHandles), hWaitHandles, FALSE, INFINITE);

		// exit event/working loop if the first event (m_hTerminationEvent)
		// has been signaled or if one of the events has been abandoned
		// (i.e. ~CFolderCrawler() is being executed)
		if(m_bRun == false || waitResult == WAIT_OBJECT_0 || waitResult == WAIT_ABANDONED_0 || waitResult == WAIT_ABANDONED_0+1)
		{
			// Termination event
			break;
		}

		SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_BEGIN);

		// If we get here, we've been woken up by something being added to the queue.
		// However, it's important that we don't do our crawling while
		// the shell is still asking for items
		bool bFirstRunAfterWakeup = true;
		for(;;)
		{
			if (!m_bRun)
				break;
			// Any locks today?
			if (CGitStatusCache::Instance().m_bClearMemory)
			{
				CAutoWriteLock writeLock(CGitStatusCache::Instance().GetGuard());
				CGitStatusCache::Instance().ClearCache();
				CGitStatusCache::Instance().m_bClearMemory = false;
			}
			if(m_lCrawlInhibitSet > 0)
			{
				// We're in crawl hold-off
				CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Crawl hold-off\n");
				Sleep(50);
				continue;
			}
			if (bFirstRunAfterWakeup)
			{
				Sleep(20);
				CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Crawl bFirstRunAfterWakeup\n");
				bFirstRunAfterWakeup = false;
				continue;
			}
			if ((m_blockReleasesAt < GetTickCount64()) && (!m_blockedPath.IsEmpty()))
			{
				CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Crawl stop blocking path %s\n", m_blockedPath.GetWinPath());
				m_blockedPath.Reset();
			}
			CGitStatusCache::Instance().RemoveTimedoutBlocks();

			while (!m_pathsToRelease.empty())
			{
				AutoLocker lock(m_critSec);
				CTGitPath path = m_pathsToRelease.Pop();
				GitStatus::ReleasePath(path.GetWinPathString());
			}

			if (m_foldersToUpdate.empty() && m_pathsToUpdate.empty())
			{
				// Nothing left to do
				break;
			}
			currentTicks = GetTickCount64();
			if (!m_pathsToUpdate.empty())
			{
				{
					AutoLocker lock(m_critSec);

					m_bPathsAddedSinceLastCrawl = false;

					workingPath = m_pathsToUpdate.Pop();
					if ((!m_blockedPath.IsEmpty()) && (m_blockedPath.IsAncestorOf(workingPath)))
					{
						// move the path to the end of the list
						m_pathsToUpdate.Push(workingPath);
						if (m_pathsToUpdate.size() < 3)
							Sleep(50);
						continue;
					}
				}

				// don't crawl paths that are excluded
				if (!CGitStatusCache::Instance().IsPathAllowed(workingPath))
					continue;
				// check if the changed path is inside an .git folder
				CString projectroot;
				if ((workingPath.HasAdminDir(&projectroot)&&workingPath.IsDirectory()) || workingPath.IsAdminDir())
				{
					// we don't crawl for paths changed in a tmp folder inside an .git folder.
					// Because we also get notifications for those even if we just ask for the status!
					// And changes there don't affect the file status at all, so it's safe
					// to ignore notifications on those paths.
					if (workingPath.IsAdminDir())
					{
						// TODO: add git specific filters here. is there really any change besides index file in .git
						//       that is relevant for overlays?
						/*CString lowerpath = workingPath.GetWinPathString();
						lowerpath.MakeLower();
						if (lowerpath.Find(L"\\tmp\\") > 0)
							continue;
						if (CStringUtils::EndsWith(lowerpath, L"\\tmp"))
							continue;
						if (lowerpath.Find(L"\\log") > 0)
							continue;*/
						// Here's a little problem:
						// the lock file is also created for fetching the status
						// and not just when committing.
						// If we could find out why the lock file was changed
						// we could decide to crawl the folder again or not.
						// But for now, we have to crawl the parent folder
						// no matter what.

						//if (lowerpath.Find(L"\\lock") > 0)
						//	continue;
						// only go back to wc root if we are in .git-dir
						do
						{
							workingPath = workingPath.GetContainingDirectory();
						} while(workingPath.IsAdminDir());
					}
					else if (!workingPath.Exists())
					{
						CAutoWriteLock writeLock(CGitStatusCache::Instance().GetGuard());
						CGitStatusCache::Instance().RemoveCacheForPath(workingPath);
						continue;
					}

					if (!CGitStatusCache::Instance().IsPathGood(workingPath))
					{
						AutoLocker lock(m_critSec);
						// move the path, the root of the repository, to the end of the list
						if (projectroot.IsEmpty())
							m_pathsToUpdate.Push(workingPath);
						else
							m_pathsToUpdate.Push(CTGitPath(projectroot));
						if (m_pathsToUpdate.size() < 3)
							Sleep(50);
						continue;
					}

					CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Invalidating and refreshing folder: %s\n", workingPath.GetWinPath());
					{
						AutoLocker print(critSec);
						_sntprintf_s(szCurrentCrawledPath[nCurrentCrawledpathIndex], MAX_CRAWLEDPATHSLEN, _TRUNCATE, L"Invalidating and refreshing folder: %s", workingPath.GetWinPath());
						++nCurrentCrawledpathIndex;
						if (nCurrentCrawledpathIndex >= MAX_CRAWLEDPATHS)
							nCurrentCrawledpathIndex = 0;
					}
					InvalidateRect(hWndHidden, nullptr, FALSE);
					{
						CAutoReadLock readLock(CGitStatusCache::Instance().GetGuard());
						// Invalidate the cache of this folder, to make sure its status is fetched again.
						CCachedDirectory * pCachedDir = CGitStatusCache::Instance().GetDirectoryCacheEntry(workingPath);
						if (pCachedDir)
						{
							git_wc_status_kind status = pCachedDir->GetCurrentFullStatus();
							pCachedDir->Invalidate();
							if (workingPath.Exists())
							{
								pCachedDir->RefreshStatus(bRecursive);
								// if the previous status wasn't normal and now it is, then
								// send a notification too.
								// We do this here because GetCurrentFullStatus() doesn't send
								// notifications for 'normal' status - if it would, we'd get tons
								// of notifications when crawling a working copy not yet in the cache.
								if ((status != git_wc_status_normal) && (pCachedDir->GetCurrentFullStatus() != status))
								{
									CGitStatusCache::Instance().UpdateShell(workingPath);
									CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": shell update in crawler for %s\n", workingPath.GetWinPath());
								}
							}
							else
							{
								CAutoWriteLock writeLock(CGitStatusCache::Instance().GetGuard());
								CGitStatusCache::Instance().RemoveCacheForPath(workingPath);
							}
						}
					}
					//In case that svn_client_stat() modified a file and we got
					//a notification about that in the directory watcher,
					//remove that here again - this is to prevent an endless loop
					AutoLocker lock(m_critSec);
					m_pathsToUpdate.erase(workingPath);
				}
				else if (workingPath.HasAdminDir())
				{
					if (!workingPath.Exists())
					{
						CAutoWriteLock writeLock(CGitStatusCache::Instance().GetGuard());
						CGitStatusCache::Instance().RemoveCacheForPath(workingPath);
						if (!workingPath.GetContainingDirectory().Exists())
							continue;
						else
							workingPath = workingPath.GetContainingDirectory();
					}
					CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Updating path: %s\n", workingPath.GetWinPath());
					{
						AutoLocker print(critSec);
						_sntprintf_s(szCurrentCrawledPath[nCurrentCrawledpathIndex], MAX_CRAWLEDPATHSLEN, _TRUNCATE, L"Updating path: %s", workingPath.GetWinPath());
						++nCurrentCrawledpathIndex;
						if (nCurrentCrawledpathIndex >= MAX_CRAWLEDPATHS)
							nCurrentCrawledpathIndex = 0;
					}
					InvalidateRect(hWndHidden, nullptr, FALSE);
					{
						CAutoReadLock readLock(CGitStatusCache::Instance().GetGuard());
						// Invalidate the cache of folders manually. The cache of files is invalidated
						// automatically if the status is asked for it and the file times don't match
						// anymore, so we don't need to manually invalidate those.
						CCachedDirectory* cachedDir = CGitStatusCache::Instance().GetDirectoryCacheEntry(workingPath.GetDirectory());
						if (cachedDir && workingPath.IsDirectory())
							cachedDir->Invalidate();
						if (cachedDir && cachedDir->GetStatusForMember(workingPath, bRecursive).GetEffectiveStatus() > git_wc_status_unversioned)
							CGitStatusCache::Instance().UpdateShell(workingPath);
					}
					AutoLocker lock(m_critSec);
					m_pathsToUpdate.erase(workingPath);
				}
				else
				{
					if (!workingPath.Exists())
					{
						CAutoWriteLock writeLock(CGitStatusCache::Instance().GetGuard());
						CGitStatusCache::Instance().RemoveCacheForPath(workingPath);
					}
				}
			}
			if (!m_foldersToUpdate.empty())
			{
				{
					AutoLocker lock(m_critSec);
					m_bItemsAddedSinceLastCrawl = false;

					// create a new CTGitPath object to make sure the cached flags are requested again.
					// without this, a missing file/folder is still treated as missing even if it is available
					// now when crawling.
					workingPath = CTGitPath(m_foldersToUpdate.Pop().GetWinPath());

					if ((!m_blockedPath.IsEmpty())&&(m_blockedPath.IsAncestorOf(workingPath)))
					{
						// move the path to the end of the list
						m_foldersToUpdate.Push(workingPath);
						if (m_foldersToUpdate.size() < 3)
							Sleep(50);
						continue;
					}
				}
				if ((!m_blockedPath.IsEmpty())&&(m_blockedPath.IsAncestorOf(workingPath)))
					continue;
				if (!CGitStatusCache::Instance().IsPathAllowed(workingPath))
					continue;
				if (!CGitStatusCache::Instance().IsPathGood(workingPath))
					continue;

				CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Crawling folder: %s\n", workingPath.GetWinPath());
				{
					AutoLocker print(critSec);
					_sntprintf_s(szCurrentCrawledPath[nCurrentCrawledpathIndex], MAX_CRAWLEDPATHSLEN, _TRUNCATE, L"Crawling folder: %s", workingPath.GetWinPath());
					++nCurrentCrawledpathIndex;
					if (nCurrentCrawledpathIndex >= MAX_CRAWLEDPATHS)
						nCurrentCrawledpathIndex = 0;
				}
				InvalidateRect(hWndHidden, nullptr, FALSE);
				{
					CAutoReadLock readLock(CGitStatusCache::Instance().GetGuard());
					// Now, we need to visit this folder, to make sure that we know its 'most important' status
					CCachedDirectory * cachedDir = CGitStatusCache::Instance().GetDirectoryCacheEntry(workingPath.GetDirectory());
					// check if the path is monitored by the watcher. If it isn't, then we have to invalidate the cache
					// for that path and add it to the watcher.
					if (!CGitStatusCache::Instance().IsPathWatched(workingPath))
					{
						if (workingPath.HasAdminDir())
						{
							CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Add watch path %s\n", workingPath.GetWinPath());
							CGitStatusCache::Instance().AddPathToWatch(workingPath);
						}
						if (cachedDir)
							cachedDir->Invalidate();
						else
						{
							CAutoWriteLock writeLock(CGitStatusCache::Instance().GetGuard());
							CGitStatusCache::Instance().RemoveCacheForPath(workingPath);
							// now cacheDir is invalid because it got deleted in the RemoveCacheForPath() call above.
							cachedDir = nullptr;
						}
					}
					if (cachedDir)
						cachedDir->RefreshStatus(bRecursive);
				}

				// While refreshing the status, we could get another crawl request for the same folder.
				// This can happen if the crawled folder has a lower status than one of the child folders
				// (recursively). To avoid double crawlings, remove such a crawl request here
				AutoLocker lock(m_critSec);
				if (m_bItemsAddedSinceLastCrawl)
				{
					m_foldersToUpdate.erase(workingPath);
				}
			}
		}
	}
	_endthread();
}

bool CFolderCrawler::SetHoldoff(DWORD milliseconds /* = 100*/)
{
	LONGLONG tick = static_cast<LONGLONG>(GetTickCount64());
	bool ret = ((tick - m_crawlHoldoffReleasesAt) > 0);
	m_crawlHoldoffReleasesAt = tick + milliseconds;
	return ret;
}

void CFolderCrawler::BlockPath(const CTGitPath& path, DWORD ticks)
{
	AutoLocker lock(m_critSec);
	CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": block path %s from being crawled\n", path.GetWinPath());
	m_blockedPath = path;
	if (ticks == 0)
		m_blockReleasesAt = GetTickCount64() + 10000;
	else
		m_blockReleasesAt = GetTickCount64() + ticks;
}
