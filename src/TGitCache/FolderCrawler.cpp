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
#include ".\foldercrawler.h"
#include "GitStatusCache.h"
#include "registry.h"
#include "TSVNCache.h"
#include "shlobj.h"


CFolderCrawler::CFolderCrawler(void)
{
	m_hWakeEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
	m_hTerminationEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
	m_hThread = INVALID_HANDLE_VALUE;
	m_lCrawlInhibitSet = 0;
	m_crawlHoldoffReleasesAt = (long)GetTickCount();
	m_bRun = false;
	m_bPathsAddedSinceLastCrawl = false;
	m_bItemsAddedSinceLastCrawl = false;
}

CFolderCrawler::~CFolderCrawler(void)
{
	Stop();
}

void CFolderCrawler::Stop()
{
	m_bRun = false;
	if (m_hTerminationEvent != INVALID_HANDLE_VALUE)
	{
		SetEvent(m_hTerminationEvent);
		if(WaitForSingleObject(m_hThread, 4000) != WAIT_OBJECT_0)
		{
			ATLTRACE("Error terminating crawler thread\n");
		}
		CloseHandle(m_hThread);
		m_hThread = INVALID_HANDLE_VALUE;
		CloseHandle(m_hTerminationEvent);
		m_hTerminationEvent = INVALID_HANDLE_VALUE;
		CloseHandle(m_hWakeEvent);
		m_hWakeEvent = INVALID_HANDLE_VALUE;
	}
}

void CFolderCrawler::Initialise()
{
	// Don't call Initialize more than once
	ATLASSERT(m_hThread == INVALID_HANDLE_VALUE);

	// Just start the worker thread. 
	// It will wait for event being signaled.
	// If m_hWakeEvent is already signaled the worker thread 
	// will behave properly (with normal priority at worst).

	m_bRun = true;
	unsigned int threadId;
	m_hThread = (HANDLE)_beginthreadex(NULL,0,ThreadEntry,this,0,&threadId);
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_IDLE);
}

void CFolderCrawler::AddDirectoryForUpdate(const CTGitPath& path)
{
	if (!CGitStatusCache::Instance().IsPathGood(path))
		return;
	{
		AutoLocker lock(m_critSec);
		m_foldersToUpdate.push_back(path);
		m_foldersToUpdate.back().SetCustomData(GetTickCount()+10);
		ATLASSERT(path.IsDirectory() || !path.Exists());
		// set this flag while we are sync'ed 
		// with the worker thread
		m_bItemsAddedSinceLastCrawl = true;
	}
	if (SetHoldoff())
		SetEvent(m_hWakeEvent);
}

void CFolderCrawler::AddPathForUpdate(const CTGitPath& path)
{
	if (!CGitStatusCache::Instance().IsPathGood(path))
		return;
	{
		AutoLocker lock(m_critSec);
		m_pathsToUpdate.push_back(path);
		m_pathsToUpdate.back().SetCustomData(GetTickCount()+1000);
		m_bPathsAddedSinceLastCrawl = true;
	}
	if (SetHoldoff())
		SetEvent(m_hWakeEvent);
}

unsigned int CFolderCrawler::ThreadEntry(void* pContext)
{
	((CFolderCrawler*)pContext)->WorkerThread();
	return 0;
}

void CFolderCrawler::WorkerThread()
{
	HANDLE hWaitHandles[2];
	hWaitHandles[0] = m_hTerminationEvent;	
	hWaitHandles[1] = m_hWakeEvent;
	CTGitPath workingPath;
	bool bFirstRunAfterWakeup = false;
	DWORD currentTicks = 0;

	// Quick check if we're on Vista
	OSVERSIONINFOEX inf;
	SecureZeroMemory(&inf, sizeof(OSVERSIONINFOEX));
	inf.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	GetVersionEx((OSVERSIONINFO *)&inf);
	WORD fullver = MAKEWORD(inf.dwMinorVersion, inf.dwMajorVersion);

	for(;;)
	{
		bool bRecursive = !!(DWORD)CRegStdWORD(_T("Software\\TortoiseGit\\RecursiveOverlay"), TRUE);

		if (fullver >= 0x0600)
		{
			SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_END);
		}
		DWORD waitResult = WaitForMultipleObjects(sizeof(hWaitHandles)/sizeof(hWaitHandles[0]), hWaitHandles, FALSE, INFINITE);
		
		// exit event/working loop if the first event (m_hTerminationEvent)
		// has been signaled or if one of the events has been abandoned
		// (i.e. ~CFolderCrawler() is being executed)
		if(m_bRun == false || waitResult == WAIT_OBJECT_0 || waitResult == WAIT_ABANDONED_0 || waitResult == WAIT_ABANDONED_0+1)
		{
			// Termination event
			break;
		}

		if (fullver >= 0x0600)
		{
			SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_BEGIN);
		}

		// If we get here, we've been woken up by something being added to the queue.
		// However, it's important that we don't do our crawling while
		// the shell is still asking for items
		// 
		bFirstRunAfterWakeup = true;
		for(;;)
		{
			if (!m_bRun)
				break;
			// Any locks today?
			if (CGitStatusCache::Instance().m_bClearMemory)
			{
				CGitStatusCache::Instance().WaitToWrite();
				CGitStatusCache::Instance().ClearCache();
				CGitStatusCache::Instance().Done();
				CGitStatusCache::Instance().m_bClearMemory = false;
			}
			if(m_lCrawlInhibitSet > 0)
			{
				// We're in crawl hold-off 
				ATLTRACE("Crawl hold-off\n");
				Sleep(50);
				continue;
			}
			if (bFirstRunAfterWakeup)
			{
				Sleep(2000);
				bFirstRunAfterWakeup = false;
				continue;
			}
			if ((m_blockReleasesAt < GetTickCount())&&(!m_blockedPath.IsEmpty()))
			{
				ATLTRACE(_T("stop blocking path %s\n"), m_blockedPath.GetWinPath());
				m_blockedPath.Reset();
			}
	
			if ((m_foldersToUpdate.empty())&&(m_pathsToUpdate.empty()))
			{
				// Nothing left to do 
				break;
			}
			currentTicks = GetTickCount();
			if (!m_pathsToUpdate.empty())
			{
				{
					AutoLocker lock(m_critSec);

					if (m_bPathsAddedSinceLastCrawl)
					{
						// The queue has changed - it's worth sorting and de-duping
						std::sort(m_pathsToUpdate.begin(), m_pathsToUpdate.end());
						m_pathsToUpdate.erase(std::unique(m_pathsToUpdate.begin(), m_pathsToUpdate.end(), &CTGitPath::PredLeftSameWCPathAsRight), m_pathsToUpdate.end());
						m_bPathsAddedSinceLastCrawl = false;
					}
					workingPath = m_pathsToUpdate.front();
					if ((DWORD(workingPath.GetCustomData()) < currentTicks)||(DWORD(workingPath.GetCustomData()) > (currentTicks + 200000)))
						m_pathsToUpdate.pop_front();
					else
					{
						// since we always sort the whole list, we risk adding tons of new paths to m_pathsToUpdate
						// until the last one in the sorted list finally times out.
						// to avoid that, we go through the list again and crawl the first one which is valid
						// to crawl. That way, we still reduce the size of the list.
						if (m_pathsToUpdate.size() > 10)
							ATLTRACE("attention: the list of paths to update is now %ld big!\n", m_pathsToUpdate.size());
						for (std::deque<CTGitPath>::iterator it = m_pathsToUpdate.begin(); it != m_pathsToUpdate.end(); ++it)
						{
							workingPath = *it;
							if ((DWORD(workingPath.GetCustomData()) < currentTicks)||(DWORD(workingPath.GetCustomData()) > (currentTicks + 200000)))
							{
								m_pathsToUpdate.erase(it);
								break;
							}
						}
					}
				}
				if (DWORD(workingPath.GetCustomData()) >= currentTicks)
				{
					Sleep(50);
					continue;
				}
				if ((!m_blockedPath.IsEmpty())&&(m_blockedPath.IsAncestorOf(workingPath)))
					continue;
				// don't crawl paths that are excluded
				if (!CGitStatusCache::Instance().IsPathAllowed(workingPath))
					continue;
				// check if the changed path is inside an .svn folder
				if ((workingPath.HasAdminDir()&&workingPath.IsDirectory())||workingPath.IsAdminDir())
				{
					// we don't crawl for paths changed in a tmp folder inside an .svn folder.
					// Because we also get notifications for those even if we just ask for the status!
					// And changes there don't affect the file status at all, so it's safe
					// to ignore notifications on those paths.
					if (workingPath.IsAdminDir())
					{
						CString lowerpath = workingPath.GetWinPathString();
						lowerpath.MakeLower();
						if (lowerpath.Find(_T("\\tmp\\"))>0)
							continue;
						if (lowerpath.Find(_T("\\tmp")) == (lowerpath.GetLength()-4))
							continue;
						if (lowerpath.Find(_T("\\log"))>0)
							continue;
						// Here's a little problem:
						// the lock file is also created for fetching the status
						// and not just when committing.
						// If we could find out why the lock file was changed
						// we could decide to crawl the folder again or not.
						// But for now, we have to crawl the parent folder
						// no matter what.

						//if (lowerpath.Find(_T("\\lock"))>0)
						//	continue;
					}
					else if (!workingPath.Exists())
					{
						CGitStatusCache::Instance().WaitToWrite();
						CGitStatusCache::Instance().RemoveCacheForPath(workingPath);
						CGitStatusCache::Instance().Done();
						continue;
					}

					do 
					{
						workingPath = workingPath.GetContainingDirectory();	
					} while(workingPath.IsAdminDir());

					ATLTRACE(_T("Invalidating and refreshing folder: %s\n"), workingPath.GetWinPath());
					{
						AutoLocker print(critSec);
						_stprintf_s(szCurrentCrawledPath[nCurrentCrawledpathIndex], MAX_CRAWLEDPATHSLEN, _T("Invalidating and refreshing folder: %s"), workingPath.GetWinPath());
						nCurrentCrawledpathIndex++;
						if (nCurrentCrawledpathIndex >= MAX_CRAWLEDPATHS)
							nCurrentCrawledpathIndex = 0;
					}
					InvalidateRect(hWnd, NULL, FALSE);
					CGitStatusCache::Instance().WaitToRead();
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
							if ((status != git_wc_status_normal)&&(pCachedDir->GetCurrentFullStatus() != status))
							{
								CGitStatusCache::Instance().UpdateShell(workingPath);
								ATLTRACE(_T("shell update in crawler for %s\n"), workingPath.GetWinPath());
							}
						}
						else
						{
							CGitStatusCache::Instance().Done();
							CGitStatusCache::Instance().WaitToWrite();
							CGitStatusCache::Instance().RemoveCacheForPath(workingPath);
						}
					}
					CGitStatusCache::Instance().Done();
					//In case that svn_client_stat() modified a file and we got
					//a notification about that in the directory watcher,
					//remove that here again - this is to prevent an endless loop
					AutoLocker lock(m_critSec);
					m_pathsToUpdate.erase(std::remove(m_pathsToUpdate.begin(), m_pathsToUpdate.end(), workingPath), m_pathsToUpdate.end());
				}
				else if (workingPath.HasAdminDir())
				{
					if (!workingPath.Exists())
					{
						CGitStatusCache::Instance().WaitToWrite();
						CGitStatusCache::Instance().RemoveCacheForPath(workingPath);
						CGitStatusCache::Instance().Done();
						continue;
					}
					if (!workingPath.Exists())
						continue;
					ATLTRACE(_T("Updating path: %s\n"), workingPath.GetWinPath());
					{
						AutoLocker print(critSec);
						_stprintf_s(szCurrentCrawledPath[nCurrentCrawledpathIndex], MAX_CRAWLEDPATHSLEN, _T("Updating path: %s"), workingPath.GetWinPath());
						nCurrentCrawledpathIndex++;
						if (nCurrentCrawledpathIndex >= MAX_CRAWLEDPATHS)
							nCurrentCrawledpathIndex = 0;
					}
					InvalidateRect(hWnd, NULL, FALSE);
					// HasAdminDir() already checks if the path points to a dir
					DWORD flags = TSVNCACHE_FLAGS_FOLDERISKNOWN;
					flags |= (workingPath.IsDirectory() ? TSVNCACHE_FLAGS_ISFOLDER : 0);
					flags |= (bRecursive ? TSVNCACHE_FLAGS_RECUSIVE_STATUS : 0);
					CGitStatusCache::Instance().WaitToRead();
					// Invalidate the cache of folders manually. The cache of files is invalidated
					// automatically if the status is asked for it and the file times don't match
					// anymore, so we don't need to manually invalidate those.
					if (workingPath.IsDirectory())
					{
						CCachedDirectory * cachedDir = CGitStatusCache::Instance().GetDirectoryCacheEntry(workingPath);
						if (cachedDir)
							cachedDir->Invalidate();
					}
					CStatusCacheEntry ce = CGitStatusCache::Instance().GetStatusForPath(workingPath, flags);
					if (ce.GetEffectiveStatus() > git_wc_status_unversioned)
					{
						CGitStatusCache::Instance().UpdateShell(workingPath);
						ATLTRACE(_T("shell update in folder crawler for %s\n"), workingPath.GetWinPath());
					}
					CGitStatusCache::Instance().Done();
					AutoLocker lock(m_critSec);
					m_pathsToUpdate.erase(std::remove(m_pathsToUpdate.begin(), m_pathsToUpdate.end(), workingPath), m_pathsToUpdate.end());
				}
				else
				{
					if (!workingPath.Exists())
					{
						CGitStatusCache::Instance().WaitToWrite();
						CGitStatusCache::Instance().RemoveCacheForPath(workingPath);
						CGitStatusCache::Instance().Done();
					}
				}
			}
			else if (!m_foldersToUpdate.empty())
			{
				{
					AutoLocker lock(m_critSec);

					if (m_bItemsAddedSinceLastCrawl)
					{
						// The queue has changed - it's worth sorting and de-duping
						std::sort(m_foldersToUpdate.begin(), m_foldersToUpdate.end());
						m_foldersToUpdate.erase(std::unique(m_foldersToUpdate.begin(), m_foldersToUpdate.end(), &CTGitPath::PredLeftEquivalentToRight), m_foldersToUpdate.end());
						m_bItemsAddedSinceLastCrawl = false;
					}
					// create a new CTGitPath object to make sure the cached flags are requested again.
					// without this, a missing file/folder is still treated as missing even if it is available
					// now when crawling.
					workingPath = CTGitPath(m_foldersToUpdate.front().GetWinPath());
					workingPath.SetCustomData(m_foldersToUpdate.front().GetCustomData());
					if ((DWORD(workingPath.GetCustomData()) < currentTicks)||(DWORD(workingPath.GetCustomData()) > (currentTicks + 200000)))
						m_foldersToUpdate.pop_front();
					else
					{
						// since we always sort the whole list, we risk adding tons of new paths to m_pathsToUpdate
						// until the last one in the sorted list finally times out.
						// to avoid that, we go through the list again and crawl the first one which is valid
						// to crawl. That way, we still reduce the size of the list.
						if (m_foldersToUpdate.size() > 10)
							ATLTRACE("attention: the list of folders to update is now %ld big!\n", m_foldersToUpdate.size());
						for (std::deque<CTGitPath>::iterator it = m_foldersToUpdate.begin(); it != m_foldersToUpdate.end(); ++it)
						{
							workingPath = *it;
							if ((DWORD(workingPath.GetCustomData()) < currentTicks)||(DWORD(workingPath.GetCustomData()) > (currentTicks + 200000)))
							{
								m_foldersToUpdate.erase(it);
								break;
							}
						}
					}
				}
				if (DWORD(workingPath.GetCustomData()) >= currentTicks)
				{
					Sleep(50);
					continue;
				}
				if ((!m_blockedPath.IsEmpty())&&(m_blockedPath.IsAncestorOf(workingPath)))
					continue;
				if (!CGitStatusCache::Instance().IsPathAllowed(workingPath))
					continue;

				ATLTRACE(_T("Crawling folder: %s\n"), workingPath.GetWinPath());
				{
					AutoLocker print(critSec);
					_stprintf_s(szCurrentCrawledPath[nCurrentCrawledpathIndex], MAX_CRAWLEDPATHSLEN, _T("Crawling folder: %s"), workingPath.GetWinPath());
					nCurrentCrawledpathIndex++;
					if (nCurrentCrawledpathIndex >= MAX_CRAWLEDPATHS)
						nCurrentCrawledpathIndex = 0;
				}
				InvalidateRect(hWnd, NULL, FALSE);
				CGitStatusCache::Instance().WaitToRead();
				// Now, we need to visit this folder, to make sure that we know its 'most important' status
				CCachedDirectory * cachedDir = CGitStatusCache::Instance().GetDirectoryCacheEntry(workingPath.GetDirectory());
				// check if the path is monitored by the watcher. If it isn't, then we have to invalidate the cache
				// for that path and add it to the watcher.
				if (!CGitStatusCache::Instance().IsPathWatched(workingPath))
				{
					if (workingPath.HasAdminDir())
						CGitStatusCache::Instance().AddPathToWatch(workingPath);
					if (cachedDir)
						cachedDir->Invalidate();
					else
					{
						CGitStatusCache::Instance().Done();
						CGitStatusCache::Instance().WaitToWrite();
						CGitStatusCache::Instance().RemoveCacheForPath(workingPath);
					}
				}
				if (cachedDir)
					cachedDir->RefreshStatus(bRecursive);

				// While refreshing the status, we could get another crawl request for the same folder.
				// This can happen if the crawled folder has a lower status than one of the child folders
				// (recursively). To avoid double crawlings, remove such a crawl request here
				AutoLocker lock(m_critSec);
				if (m_bItemsAddedSinceLastCrawl)
				{
					if (m_foldersToUpdate.back().IsEquivalentToWithoutCase(workingPath))
					{
						m_foldersToUpdate.pop_back();
						m_bItemsAddedSinceLastCrawl = false;
					}
				}
				CGitStatusCache::Instance().Done();
			}
		}
	}
	_endthread();
}

bool CFolderCrawler::SetHoldoff(DWORD milliseconds /* = 100*/)
{
	long tick = (long)GetTickCount();
	bool ret = ((tick - m_crawlHoldoffReleasesAt) > 0);
	m_crawlHoldoffReleasesAt = tick + milliseconds;
	return ret;
}

void CFolderCrawler::BlockPath(const CTGitPath& path, DWORD ticks)
{
	ATLTRACE(_T("block path %s from being crawled\n"), path.GetWinPath());
	m_blockedPath = path;
	if (ticks == 0)
		m_blockReleasesAt = GetTickCount()+10000;
	else
		m_blockReleasesAt = GetTickCount()+ticks;
}
