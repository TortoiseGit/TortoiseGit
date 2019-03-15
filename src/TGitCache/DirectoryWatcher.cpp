// TortoiseGit - a Windows shell extension for easy version control

// External Cache Copyright (C) 2005-2008, 2011-2012 - TortoiseSVN
// Copyright (C) 2008-2017, 2019 - TortoiseGit

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
#include <Dbt.h>
#include "GitStatusCache.h"
#include "DirectoryWatcher.h"
#include "GitIndex.h"
#include "SmartHandle.h"

#include <list>

extern HWND hWndHidden;
extern CGitAdminDirMap g_AdminDirMap;

CDirectoryWatcher::CDirectoryWatcher(void)
	: m_bRunning(TRUE)
	, m_bCleaned(FALSE)
	, m_FolderCrawler(nullptr)
	, blockTickCount(0)
{
	// enable the required privileges for this process

	LPCTSTR arPrivelegeNames[] = {  SE_BACKUP_NAME,
									SE_RESTORE_NAME,
									SE_CHANGE_NOTIFY_NAME
								 };

	for (int i=0; i<(sizeof(arPrivelegeNames)/sizeof(LPCTSTR)); ++i)
	{
		CAutoGeneralHandle hToken;
		if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, hToken.GetPointer()))
		{
			TOKEN_PRIVILEGES tp = { 1 };

			if (LookupPrivilegeValue(nullptr, arPrivelegeNames[i], &tp.Privileges[0].Luid))
			{
				tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

				AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), nullptr, nullptr);
			}
		}
	}

	unsigned int threadId = 0;
	m_hThread = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, ThreadEntry, this, 0, &threadId));
}

CDirectoryWatcher::~CDirectoryWatcher(void)
{
	Stop();
	AutoLocker lock(m_critSec);
	ClearInfoMap();
	CleanupWatchInfo();
}

void CDirectoryWatcher::CloseCompletionPort()
{
	m_hCompPort.CloseHandle();
}

void CDirectoryWatcher::ScheduleForDeletion (CDirWatchInfo* info)
{
	infoToDelete.push_back (info);
}

void CDirectoryWatcher::CleanupWatchInfo()
{
	AutoLocker lock(m_critSec);
	InterlockedExchange(&m_bCleaned, TRUE);
	while (!infoToDelete.empty())
	{
		CDirWatchInfo* info = infoToDelete.back();
		infoToDelete.pop_back();
		delete info;
	}
}

void CDirectoryWatcher::Stop()
{
	InterlockedExchange(&m_bRunning, FALSE);
	CloseWatchHandles();
	WaitForSingleObject(m_hThread, 4000);
	m_hThread.CloseHandle();
}

void CDirectoryWatcher::SetFolderCrawler(CFolderCrawler * crawler)
{
	m_FolderCrawler = crawler;
}

bool CDirectoryWatcher::RemovePathAndChildren(const CTGitPath& path)
{
	bool bRemoved = false;
	AutoLocker lock(m_critSec);
repeat:
	for (int i=0; i<watchedPaths.GetCount(); ++i)
	{
		if (path.IsAncestorOf(watchedPaths[i]))
		{
			watchedPaths.RemovePath(watchedPaths[i]);
			bRemoved = true;
			goto repeat;
		}
	}
	return bRemoved;
}

void CDirectoryWatcher::BlockPath(const CTGitPath& path)
{
	blockedPath = path;
	// block the path from being watched for 4 seconds
	blockTickCount = GetTickCount64() + 4000;
	CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Blocking path: %s\n", path.GetWinPath());
}

bool CDirectoryWatcher::AddPath(const CTGitPath& path, bool bCloseInfoMap)
{
	if (!CGitStatusCache::Instance().IsPathAllowed(path))
		return false;
	if ((!blockedPath.IsEmpty())&&(blockedPath.IsAncestorOf(path)))
	{
		if (GetTickCount64() < blockTickCount)
		{
			CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Path %s prevented from being watched\n", path.GetWinPath());
			return false;
		}
	}

	// ignore the recycle bin
	PTSTR pFound = StrStrI(path.GetWinPath(), L":\\RECYCLER");
	if (pFound)
	{
		if ((*(pFound + 10) == '\0') || (*(pFound + 10) == '\\'))
			return false;
	}
	pFound = StrStrI(path.GetWinPath(), L":\\$Recycle.Bin");
	if (pFound)
	{
		if ((*(pFound + 14) == '\0') || (*(pFound + 14) == '\\'))
			return false;
	}

	AutoLocker lock(m_critSec);
	for (int i=0; i<watchedPaths.GetCount(); ++i)
	{
		if (watchedPaths[i].IsAncestorOf(path))
			return false;		// already watched (recursively)
	}

	// now check if with the new path we might have a new root
	CTGitPath newroot;
	for (int i=0; i<watchedPaths.GetCount(); ++i)
	{
		const CString& watched = watchedPaths[i].GetWinPathString();
		const CString& sPath = path.GetWinPathString();
		int minlen = min(sPath.GetLength(), watched.GetLength());
		int len = 0;
		for (len = 0; len < minlen; ++len)
		{
			if (watched.GetAt(len) != sPath.GetAt(len))
			{
				if ((len > 1)&&(len < minlen))
				{
					if (sPath.GetAt(len)=='\\')
						newroot = CTGitPath(sPath.Left(len));
					else if (watched.GetAt(len)=='\\')
						newroot = CTGitPath(watched.Left(len));
				}
				break;
			}
		}
		if (len == minlen)
		{
			if (sPath.GetLength() == minlen)
			{
				if (watched.GetLength() > minlen)
				{
					if (watched.GetAt(len)=='\\')
						newroot = path;
					else if (sPath.GetLength() == 3 && sPath[1] == ':')
						newroot = path;
				}
			}
			else
			{
				if (sPath.GetLength() > minlen)
				{
					if (sPath.GetAt(len)=='\\')
						newroot = CTGitPath(watched);
					else if (watched.GetLength() == 3 && watched[1] == ':')
						newroot = CTGitPath(watched);
				}
			}
		}
	}
	if (!newroot.IsEmpty() && newroot.HasAdminDir())
	{
		CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": add path to watch %s\n", newroot.GetWinPath());
		watchedPaths.AddPath(newroot);
		watchedPaths.RemoveChildren();
		if (bCloseInfoMap)
			ClearInfoMap();

		return true;
	}
	if (!path.HasAdminDir())
	{
		CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Path %s prevented from being watched: not versioned\n", path.GetWinPath());
		return false;
	}
	CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": add path to watch %s\n", path.GetWinPath());
	watchedPaths.AddPath(path);
	if (bCloseInfoMap)
		ClearInfoMap();

	return true;
}

bool CDirectoryWatcher::IsPathWatched(const CTGitPath& path)
{
	AutoLocker lock(m_critSec);
	for (int i=0; i<watchedPaths.GetCount(); ++i)
	{
		if (watchedPaths[i].IsAncestorOf(path))
			return true;
	}
	return false;
}

unsigned int CDirectoryWatcher::ThreadEntry(void* pContext)
{
	reinterpret_cast<CDirectoryWatcher*>(pContext)->WorkerThread();
	return 0;
}

void CDirectoryWatcher::WorkerThread()
{
	DWORD numBytes;
	CDirWatchInfo* pdi = nullptr;
	LPOVERLAPPED lpOverlapped;
	WCHAR buf[READ_DIR_CHANGE_BUFFER_SIZE] = {0};
	WCHAR* pFound = nullptr;
	while (m_bRunning)
	{
		CleanupWatchInfo();
		if (!watchedPaths.IsEmpty())
		{
			// Any incoming notifications?

			pdi = nullptr;
			numBytes = 0;
			InterlockedExchange(&m_bCleaned, FALSE);
			if ((!m_hCompPort)
				|| !GetQueuedCompletionStatus(m_hCompPort,
											  &numBytes,
											  (PULONG_PTR) &pdi,
											  &lpOverlapped,
											  600000 /*10 minutes*/))
			{
				// No. Still trying?

				if (!m_bRunning)
					return;

				CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": restarting watcher\n");
				m_hCompPort.CloseHandle();

				// We must sync the whole section because other threads may
				// receive "AddPath" calls that will delete the completion
				// port *while* we are adding references to it .

				AutoLocker lock(m_critSec);

				// Clear the list of watched objects and recreate that list.
				// This will also delete the old completion port

				ClearInfoMap();
				CleanupWatchInfo();

				for (int i=0; i<watchedPaths.GetCount(); ++i)
				{
					CTGitPath watchedPath = watchedPaths[i];

					CAutoFile hDir = CreateFile(watchedPath.GetWinPath(),
											FILE_LIST_DIRECTORY,
											FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
											nullptr, //security attributes
											OPEN_EXISTING,
											FILE_FLAG_BACKUP_SEMANTICS | //required privileges: SE_BACKUP_NAME and SE_RESTORE_NAME.
											FILE_FLAG_OVERLAPPED,
											nullptr);
					if (!hDir)
					{
						// this could happen if a watched folder has been removed/renamed
						CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": CreateFile failed. Can't watch directory %s\n", watchedPaths[i].GetWinPath());
						watchedPaths.RemovePath(watchedPath);
						break;
					}

					DEV_BROADCAST_HANDLE NotificationFilter = { 0 };
					NotificationFilter.dbch_size = sizeof(DEV_BROADCAST_HANDLE);
					NotificationFilter.dbch_devicetype = DBT_DEVTYP_HANDLE;
					NotificationFilter.dbch_handle = hDir;
					// RegisterDeviceNotification sends a message to the UI thread:
					// make sure we *can* send it and that the UI thread isn't waiting on a lock
					int numPaths = watchedPaths.GetCount();
					size_t numWatch = watchInfoMap.size();
					lock.Unlock();
					NotificationFilter.dbch_hdevnotify = RegisterDeviceNotification(hWndHidden, &NotificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE);
					lock.Lock();
					// since we released the lock to prevent a deadlock with the UI thread,
					// it could happen that new paths were added to watch, or another thread
					// could have cleared our info map.
					// if that happened, we have to restart watching all paths again.
					if ((numPaths != watchedPaths.GetCount()) || (numWatch != watchInfoMap.size()))
					{
						ClearInfoMap();
						CleanupWatchInfo();
						Sleep(200);
						break;
					}

					CDirWatchInfo * pDirInfo = new CDirWatchInfo(hDir.Detach(), watchedPath);// the new CDirWatchInfo object owns the handle now
					pDirInfo->m_hDevNotify = NotificationFilter.dbch_hdevnotify;


					HANDLE port = CreateIoCompletionPort(pDirInfo->m_hDir, m_hCompPort, reinterpret_cast<ULONG_PTR>(pDirInfo), 0);
					if (port == INVALID_HANDLE_VALUE)
					{
						CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": CreateIoCompletionPort failed. Can't watch directory %s\n", watchedPath.GetWinPath());

						// we must close the directory handle to allow ClearInfoMap()
						// to close the completion port properly
						pDirInfo->CloseDirectoryHandle();

						ClearInfoMap();
						CleanupWatchInfo();
						delete pDirInfo;
						pDirInfo = nullptr;

						watchedPaths.RemovePath(watchedPath);
						break;
					}
					m_hCompPort = std::move(port);

					if (!ReadDirectoryChangesW(pDirInfo->m_hDir,
												pDirInfo->m_Buffer,
												READ_DIR_CHANGE_BUFFER_SIZE,
												TRUE,
												FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
												&numBytes,// not used
												&pDirInfo->m_Overlapped,
												nullptr))  //no completion routine!
					{
						CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": ReadDirectoryChangesW failed. Can't watch directory %s\n", watchedPath.GetWinPath());

						// we must close the directory handle to allow ClearInfoMap()
						// to close the completion port properly
						pDirInfo->CloseDirectoryHandle();

						ClearInfoMap();
						CleanupWatchInfo();
						delete pDirInfo;
						pDirInfo = nullptr;
						watchedPaths.RemovePath(watchedPath);
						break;
					}

					CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": watching path %s\n", pDirInfo->m_DirName.GetWinPath());
					watchInfoMap[pDirInfo->m_hDir] = pDirInfo;
				}
			}
			else
			{
				if (!m_bRunning)
					return;
				if (watchInfoMap.empty())
					continue;

				// NOTE: the longer this code takes to execute until ReadDirectoryChangesW
				// is called again, the higher the chance that we miss some
				// changes in the file system!
				if (pdi)
				{
					BOOL bRet = false;
					std::list<CTGitPath> notifyPaths;
					{
						AutoLocker lock(m_critSec);
						// in case the CDirectoryWatcher objects have been cleaned,
						// the m_bCleaned variable will be set to true here. If the
						// objects haven't been cleared, we can access them here.
						if (InterlockedExchange(&m_bCleaned, FALSE))
							continue;
						if (   (!pdi->m_hDir) || watchInfoMap.empty()
							|| (watchInfoMap.find(pdi->m_hDir) == watchInfoMap.end()))
						{
							continue;
						}
						auto pnotify = reinterpret_cast<PFILE_NOTIFY_INFORMATION>(pdi->m_Buffer);
						DWORD nOffset = 0;

						do
						{
							pnotify = reinterpret_cast<PFILE_NOTIFY_INFORMATION>(reinterpret_cast<LPBYTE>(pnotify) + nOffset);

							if (reinterpret_cast<ULONG_PTR>(pnotify) - reinterpret_cast<ULONG_PTR>(pdi->m_Buffer) > READ_DIR_CHANGE_BUFFER_SIZE)
								break;

							nOffset = pnotify->NextEntryOffset;

							if (pnotify->FileNameLength >= (READ_DIR_CHANGE_BUFFER_SIZE*sizeof(TCHAR)))
								continue;

							SecureZeroMemory(buf, READ_DIR_CHANGE_BUFFER_SIZE*sizeof(TCHAR));
							wcsncpy_s(buf, pdi->m_DirPath, _countof(buf) - 1);
							errno_t err = wcsncat_s(buf + pdi->m_DirPath.GetLength(), READ_DIR_CHANGE_BUFFER_SIZE - pdi->m_DirPath.GetLength(), pnotify->FileName, min(READ_DIR_CHANGE_BUFFER_SIZE - pdi->m_DirPath.GetLength(), int(pnotify->FileNameLength / sizeof(TCHAR))));
							if (err == STRUNCATE)
								continue;
							buf[(pnotify->FileNameLength / sizeof(TCHAR)) + pdi->m_DirPath.GetLength()] = L'\0';

							if (m_FolderCrawler)
							{
								if ((pFound = StrStrI(buf, L"\\tmp")) != nullptr)
								{
									pFound += 4;
									if (((*pFound) == '\\') || ((*pFound) == '\0'))
										continue;
								}
								if ((pFound = StrStrI(buf, L":\\RECYCLER")) != nullptr)
								{
									if ((*(pFound + 10) == '\0') || (*(pFound + 10) == '\\'))
										continue;
								}
								if ((pFound = StrStrI(buf, L":\\$Recycle.Bin")) != nullptr)
								{
									if ((*(pFound + 14) == '\0') || (*(pFound + 14) == '\\'))
										continue;
								}

								if (StrStrI(buf, L".tmp"))
								{
									// assume files with a .tmp extension are not versioned and interesting,
									// so ignore them.
									continue;
								}

								CTGitPath path;
								bool isIndex = false;
								if ((pFound = wcsstr(buf, L".git")) != nullptr)
								{
									// omit repository data change except .git/index.lock- or .git/HEAD.lock-files
									if (reinterpret_cast<ULONG_PTR>(pnotify) - reinterpret_cast<ULONG_PTR>(pdi->m_Buffer) > READ_DIR_CHANGE_BUFFER_SIZE)
										break;

									path = g_AdminDirMap.GetWorkingCopy(CTGitPath(buf).GetContainingDirectory().GetWinPathString());

									if ((wcsstr(pFound, L"index.lock") || wcsstr(pFound, L"HEAD.lock")) && pnotify->Action == FILE_ACTION_ADDED)
									{
										CGitStatusCache::Instance().BlockPath(path);
										continue;
									}
									else if (((wcsstr(pFound, L"index.lock") || wcsstr(pFound, L"HEAD.lock")) && pnotify->Action == FILE_ACTION_REMOVED) || (((wcsstr(pFound, L"index") && !wcsstr(pFound, L"index.lock")) || (wcsstr(pFound, L"HEAD") && wcsstr(pFound, L"HEAD.lock"))) && pnotify->Action == FILE_ACTION_MODIFIED) || ((!wcsstr(pFound, L"index.lock") || wcsstr(pFound, L"HEAD.lock")) && pnotify->Action == FILE_ACTION_RENAMED_NEW_NAME))
									{
										isIndex = true;
										CGitStatusCache::Instance().BlockPath(path, 1);
									}
									else
										continue;
								}
								else
									path.SetFromUnknown(buf);

								if(!path.HasAdminDir() && !isIndex)
									continue;

								CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": change notification for %s\n", buf);
								notifyPaths.push_back(path);
							}
						} while ((nOffset > 0)&&(nOffset < READ_DIR_CHANGE_BUFFER_SIZE));

						// setup next notification cycle
						SecureZeroMemory (pdi->m_Buffer, sizeof(pdi->m_Buffer));
						SecureZeroMemory (&pdi->m_Overlapped, sizeof(OVERLAPPED));
						bRet = ReadDirectoryChangesW(pdi->m_hDir,
							pdi->m_Buffer,
							READ_DIR_CHANGE_BUFFER_SIZE,
							TRUE,
							FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
							&numBytes,// not used
							&pdi->m_Overlapped,
							nullptr); //no completion routine!
					}
					if (!notifyPaths.empty())
					{
						for (auto nit = notifyPaths.cbegin(); nit != notifyPaths.cend(); ++nit)
						{
							m_FolderCrawler->AddPathForUpdate(*nit);
						}
					}

					// any clean-up to do?

					CleanupWatchInfo();

					if (!bRet)
					{
						// Since the call to ReadDirectoryChangesW failed, just
						// wait a while. We don't want to have this thread
						// running using 100% CPU if something goes completely
						// wrong.
						Sleep(200);
					}
				}
			}
		}// if (!watchedPaths.IsEmpty())
		else
			Sleep(200);
	}// while (m_bRunning)
}

// call this before destroying async I/O structures:

void CDirectoryWatcher::CloseWatchHandles()
{
	AutoLocker lock(m_critSec);

	for (auto I = watchInfoMap.cbegin(); I != watchInfoMap.cend(); ++I)
		if (I->second)
			I->second->CloseDirectoryHandle();

	CloseCompletionPort();
}

void CDirectoryWatcher::ClearInfoMap()
{
	CloseWatchHandles();
	if (!watchInfoMap.empty())
	{
		AutoLocker lock(m_critSec);
		for (TInfoMap::iterator I = watchInfoMap.begin(); I != watchInfoMap.end(); ++I)
		{
			CDirectoryWatcher::CDirWatchInfo * info = I->second;
			I->second = nullptr;
			ScheduleForDeletion (info);
		}
		watchInfoMap.clear();
	}
}

CTGitPath CDirectoryWatcher::CloseInfoMap(HANDLE hDir)
{
	CTGitPath path;
	AutoLocker lock(m_critSec);
	TInfoMap::const_iterator d = watchInfoMap.find(hDir);
	if (d != watchInfoMap.end())
	{
		path = CTGitPath(CTGitPath(d->second->m_DirPath).GetRootPathString());
		RemovePathAndChildren(path);
		BlockPath(path);
	}
	CloseWatchHandles();

	if (watchInfoMap.empty())
		return path;

	for (auto I = watchInfoMap.cbegin(); I != watchInfoMap.cend(); ++I)
	{
		CDirectoryWatcher::CDirWatchInfo * info = I->second;

		ScheduleForDeletion (info);
	}
	watchInfoMap.clear();

	return path;
}

bool CDirectoryWatcher::CloseHandlesForPath(const CTGitPath& path)
{
	AutoLocker lock(m_critSec);
	CloseWatchHandles();

	if (watchInfoMap.empty())
		return false;

	for (TInfoMap::iterator I = watchInfoMap.begin(); I != watchInfoMap.end(); ++I)
	{
		CDirectoryWatcher::CDirWatchInfo * info = I->second;
		I->second = nullptr;
		CTGitPath p = CTGitPath(info->m_DirPath);
		if (path.IsAncestorOf(p))
		{
			RemovePathAndChildren(p);
			BlockPath(p);
		}
		ScheduleForDeletion(info);
	}
	watchInfoMap.clear();
	return true;
}

CDirectoryWatcher::CDirWatchInfo::CDirWatchInfo(HANDLE hDir, const CTGitPath& DirectoryName)
	: m_hDir(std::move(hDir))
	, m_DirName(DirectoryName)
{
	ATLASSERT(m_hDir && !DirectoryName.IsEmpty());
	m_Buffer[0] = '\0';
	SecureZeroMemory(&m_Overlapped, sizeof(m_Overlapped));
	m_DirPath = m_DirName.GetWinPathString();
	if (m_DirPath.GetAt(m_DirPath.GetLength() - 1) != L'\\')
		m_DirPath += L'\\';
	m_hDevNotify = nullptr;
}

CDirectoryWatcher::CDirWatchInfo::~CDirWatchInfo()
{
	CloseDirectoryHandle();
}

bool CDirectoryWatcher::CDirWatchInfo::CloseDirectoryHandle()
{
	bool b = m_hDir.CloseHandle();

	if (m_hDevNotify)
	{
		UnregisterDeviceNotification(m_hDevNotify);
		m_hDevNotify = nullptr;
	}
	return b;
}

