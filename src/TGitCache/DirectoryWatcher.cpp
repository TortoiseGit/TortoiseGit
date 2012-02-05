// TortoiseGit - a Windows shell extension for easy version control

// External Cache Copyright (C) 2005 - 2007 - TortoiseSVN
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
//
#include "StdAfx.h"
#include "Dbt.h"
#include "GitStatusCache.h"
#include "directorywatcher.h"

extern HWND hWnd;

CDirectoryWatcher::CDirectoryWatcher(void) : m_hCompPort(NULL)
	, m_bRunning(TRUE)
	, m_FolderCrawler(NULL)
	, blockTickCount(0)
{
	// enable the required privileges for this process

	LPCTSTR arPrivelegeNames[] = {	SE_BACKUP_NAME,
									SE_RESTORE_NAME,
									SE_CHANGE_NOTIFY_NAME
								 };

	for (int i=0; i<(sizeof(arPrivelegeNames)/sizeof(LPCTSTR)); ++i)
	{
		HANDLE hToken;
		if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
		{
			TOKEN_PRIVILEGES tp = { 1 };

			if (LookupPrivilegeValue(NULL, arPrivelegeNames[i],  &tp.Privileges[0].Luid))
			{
				tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

				AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
			}
			CloseHandle(hToken);
		}
	}

	unsigned int threadId;
	m_hThread = (HANDLE)_beginthreadex(NULL,0,ThreadEntry,this,0,&threadId);
}

CDirectoryWatcher::~CDirectoryWatcher(void)
{
	InterlockedExchange(&m_bRunning, FALSE);
	if (m_hThread != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hThread);
		m_hThread = INVALID_HANDLE_VALUE;
	}
	AutoLocker lock(m_critSec);
	ClearInfoMap();
}

void CDirectoryWatcher::Stop()
{
	InterlockedExchange(&m_bRunning, FALSE);
	if (m_hThread != INVALID_HANDLE_VALUE)
		CloseHandle(m_hThread);
	m_hThread = INVALID_HANDLE_VALUE;
	if (m_hCompPort != INVALID_HANDLE_VALUE)
		CloseHandle(m_hCompPort);
	m_hCompPort = INVALID_HANDLE_VALUE;
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
	blockTickCount = GetTickCount()+4000;
	ATLTRACE(_T("Blocking path: %s\n"), path.GetWinPath());
}

bool CDirectoryWatcher::AddPath(const CTGitPath& path)
{
	if (!CGitStatusCache::Instance().IsPathAllowed(path))
		return false;
	if ((!blockedPath.IsEmpty())&&(blockedPath.IsAncestorOf(path)))
	{
		if (GetTickCount() < blockTickCount)
		{
			ATLTRACE(_T("Path %s prevented from being watched\n"), path.GetWinPath());
			return false;
		}
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
					{
						newroot = CTGitPath(sPath.Left(len));
					}
					else if (watched.GetAt(len)=='\\')
					{
						newroot = CTGitPath(watched.Left(len));
					}
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
					{
						newroot = path;
					}
					else if (sPath.GetLength() == 3 && sPath[1] == ':')
					{
						newroot = path;
					}
				}
			}
			else
			{
				if (sPath.GetLength() > minlen)
				{
					if (sPath.GetAt(len)=='\\')
					{
						newroot = CTGitPath(watched);
					}
					else if (watched.GetLength() == 3 && watched[1] == ':')
					{
						newroot = CTGitPath(watched);
					}
				}
			}
		}
	}
	if (!newroot.IsEmpty())
	{
		ATLTRACE(_T("add path to watch %s\n"), newroot.GetWinPath());
		watchedPaths.AddPath(newroot);
		watchedPaths.RemoveChildren();
		CloseInfoMap();
		m_hCompPort = INVALID_HANDLE_VALUE;
		return true;
	}
	ATLTRACE(_T("add path to watch %s\n"), path.GetWinPath());
	watchedPaths.AddPath(path);
	CloseInfoMap();
	m_hCompPort = INVALID_HANDLE_VALUE;
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
	((CDirectoryWatcher*)pContext)->WorkerThread();
	return 0;
}

void CDirectoryWatcher::WorkerThread()
{
	DWORD numBytes;
	CDirWatchInfo * pdi = NULL;
	LPOVERLAPPED lpOverlapped;
	WCHAR buf[READ_DIR_CHANGE_BUFFER_SIZE] = {0};
	WCHAR * pFound = NULL;
	CTGitPath path;

	while (m_bRunning)
	{
		if (watchedPaths.GetCount())
		{
			if (!GetQueuedCompletionStatus(m_hCompPort,
											&numBytes,
											(PULONG_PTR) &pdi,
											&lpOverlapped,
											INFINITE))
			{
				// Error retrieving changes
				// Clear the list of watched objects and recreate that list
				if (!m_bRunning)
					return;
				{
					AutoLocker lock(m_critSec);
					ClearInfoMap();
				}
				DWORD lasterr = GetLastError();
				if ((m_hCompPort != INVALID_HANDLE_VALUE)&&(lasterr!=ERROR_SUCCESS)&&(lasterr!=ERROR_INVALID_HANDLE))
				{
					CloseHandle(m_hCompPort);
					m_hCompPort = INVALID_HANDLE_VALUE;
				}
				// Since we pass m_hCompPort to CreateIoCompletionPort, we
				// have to set this to NULL to have that API create a new
				// handle.
				m_hCompPort = NULL;
				for (int i=0; i<watchedPaths.GetCount(); ++i)
				{
					CTGitPath watchedPath = watchedPaths[i];

					HANDLE hDir = CreateFile(watchedPath.GetWinPath(),
											FILE_LIST_DIRECTORY,
											FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
											NULL, //security attributes
											OPEN_EXISTING,
											FILE_FLAG_BACKUP_SEMANTICS | //required privileges: SE_BACKUP_NAME and SE_RESTORE_NAME.
											FILE_FLAG_OVERLAPPED,
											NULL);
					if (hDir == INVALID_HANDLE_VALUE)
					{
						// this could happen if a watched folder has been removed/renamed
						ATLTRACE(_T("CDirectoryWatcher: CreateFile failed. Can't watch directory %s\n"), watchedPaths[i].GetWinPath());
						CloseHandle(m_hCompPort);
						m_hCompPort = INVALID_HANDLE_VALUE;
						AutoLocker lock(m_critSec);
						watchedPaths.RemovePath(watchedPath);
						i--; if (i<0) i=0;
						break;
					}

					DEV_BROADCAST_HANDLE NotificationFilter;
					SecureZeroMemory(&NotificationFilter, sizeof(NotificationFilter));
					NotificationFilter.dbch_size = sizeof(DEV_BROADCAST_HANDLE);
					NotificationFilter.dbch_devicetype = DBT_DEVTYP_HANDLE;
					NotificationFilter.dbch_handle = hDir;
					NotificationFilter.dbch_hdevnotify = RegisterDeviceNotification(hWnd, &NotificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE);

					CDirWatchInfo * pDirInfo = new CDirWatchInfo(hDir, watchedPath);
					pDirInfo->m_hDevNotify = NotificationFilter.dbch_hdevnotify;
					m_hCompPort = CreateIoCompletionPort(hDir, m_hCompPort, (ULONG_PTR)pDirInfo, 0);
					if (m_hCompPort == NULL)
					{
						ATLTRACE(_T("CDirectoryWatcher: CreateIoCompletionPort failed. Can't watch directory %s\n"), watchedPath.GetWinPath());
						AutoLocker lock(m_critSec);
						ClearInfoMap();
						delete pDirInfo;
						pDirInfo = NULL;
						watchedPaths.RemovePath(watchedPath);
						i--; if (i<0) i=0;
						break;
					}
					if (!ReadDirectoryChangesW(pDirInfo->m_hDir,
												pDirInfo->m_Buffer,
												READ_DIR_CHANGE_BUFFER_SIZE,
												TRUE,
												FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
												&numBytes,// not used
												&pDirInfo->m_Overlapped,
												NULL))	//no completion routine!
					{
						ATLTRACE(_T("CDirectoryWatcher: ReadDirectoryChangesW failed. Can't watch directory %s\n"), watchedPath.GetWinPath());
						AutoLocker lock(m_critSec);
						ClearInfoMap();
						delete pDirInfo;
						pDirInfo = NULL;
						watchedPaths.RemovePath(watchedPath);
						i--; if (i<0) i=0;
						break;
					}
					AutoLocker lock(m_critSec);
					watchInfoMap[pDirInfo->m_hDir] = pDirInfo;
					ATLTRACE(_T("watching path %s\n"), pDirInfo->m_DirName.GetWinPath());
				}
			}
			else
			{
				if (!m_bRunning)
					return;
				// NOTE: the longer this code takes to execute until ReadDirectoryChangesW
				// is called again, the higher the chance that we miss some
				// changes in the file system!
				if (pdi)
				{
					if (numBytes == 0)
					{
						goto continuewatching;
					}
					PFILE_NOTIFY_INFORMATION pnotify = (PFILE_NOTIFY_INFORMATION)pdi->m_Buffer;
					if ((ULONG_PTR)pnotify - (ULONG_PTR)pdi->m_Buffer > READ_DIR_CHANGE_BUFFER_SIZE)
						goto continuewatching;
					DWORD nOffset = pnotify->NextEntryOffset;

					do
					{
						nOffset = pnotify->NextEntryOffset;
						if (pnotify->FileNameLength >= (READ_DIR_CHANGE_BUFFER_SIZE*sizeof(TCHAR)))
							continue;
						SecureZeroMemory(buf, READ_DIR_CHANGE_BUFFER_SIZE*sizeof(TCHAR));
						_tcsncpy_s(buf, READ_DIR_CHANGE_BUFFER_SIZE, pdi->m_DirPath, READ_DIR_CHANGE_BUFFER_SIZE);
						errno_t err = _tcsncat_s(buf+pdi->m_DirPath.GetLength(), READ_DIR_CHANGE_BUFFER_SIZE-pdi->m_DirPath.GetLength(), pnotify->FileName, _TRUNCATE);
						if (err == STRUNCATE)
						{
							pnotify = (PFILE_NOTIFY_INFORMATION)((LPBYTE)pnotify + nOffset);
							continue;
						}
						buf[(pnotify->FileNameLength/sizeof(TCHAR))+pdi->m_DirPath.GetLength()] = 0;
						pnotify = (PFILE_NOTIFY_INFORMATION)((LPBYTE)pnotify + nOffset);
						if (m_FolderCrawler)
						{
							if ((pFound = wcsstr(buf, L"\\tmp"))!=NULL)
							{
								pFound += 4;
								if (((*pFound)=='\\')||((*pFound)=='\0'))
								{
									if ((ULONG_PTR)pnotify - (ULONG_PTR)pdi->m_Buffer > READ_DIR_CHANGE_BUFFER_SIZE)
										break;
									continue;
								}
							}
							if ((pFound = wcsstr(buf, L":\\RECYCLER\\"))!=NULL)
							{
								if ((pFound-buf) < 5)
								{
									// a notification for the recycle bin - ignore it
									if ((ULONG_PTR)pnotify - (ULONG_PTR)pdi->m_Buffer > READ_DIR_CHANGE_BUFFER_SIZE)
										break;
									continue;
								}
							}
							if ((pFound = wcsstr(buf, L":\\$Recycle.Bin\\"))!=NULL)
							{
								if ((pFound-buf) < 5)
								{
									// a notification for the recycle bin - ignore it
									if ((ULONG_PTR)pnotify - (ULONG_PTR)pdi->m_Buffer > READ_DIR_CHANGE_BUFFER_SIZE)
										break;
									continue;
								}
							}
							if ((pFound = wcsstr(buf, L".tmp"))!=NULL)
							{
								// assume files with a .tmp extension are not versioned and interesting,
								// so ignore them.
								if ((ULONG_PTR)pnotify - (ULONG_PTR)pdi->m_Buffer > READ_DIR_CHANGE_BUFFER_SIZE)
									break;
								continue;
							}
							bool isIndex = false;
							if ((pFound = wcsstr(buf, L".git"))!=NULL)
							{
								// omit repository data change except .git/index.lock- or .git/HEAD.lock-files
								if ((ULONG_PTR)pnotify - (ULONG_PTR)pdi->m_Buffer > READ_DIR_CHANGE_BUFFER_SIZE)
									break;

								if ((wcsstr(pFound, L"index.lock") != NULL || wcsstr(pFound, L"HEAD.lock") != NULL) && pnotify->Action == FILE_ACTION_ADDED)
								{
									CGitStatusCache::Instance().BlockPath(CTGitPath(buf).GetContainingDirectory().GetContainingDirectory());
									continue;
								}
								else if (((wcsstr(pFound, L"index.lock") != NULL || wcsstr(pFound, L"HEAD.lock") != NULL) && pnotify->Action == FILE_ACTION_REMOVED) || ((wcsstr(pFound, L"index") != NULL || wcsstr(pFound, L"HEAD") != NULL) && pnotify->Action == FILE_ACTION_MODIFIED))
								{
									isIndex = true;
									CGitStatusCache::Instance().BlockPath(CTGitPath(buf).GetContainingDirectory().GetContainingDirectory(), 1);
								}
								else
								{
									continue;
								}
							}

							path.SetFromWin(buf);
							if(!path.HasAdminDir() && !isIndex)
								continue;

							ATLTRACE(_T("change notification: %s\n"), buf);
							m_FolderCrawler->AddPathForUpdate(CTGitPath(buf));
						}
						if ((ULONG_PTR)pnotify - (ULONG_PTR)pdi->m_Buffer > READ_DIR_CHANGE_BUFFER_SIZE)
							break;
					} while (nOffset);
continuewatching:
					SecureZeroMemory(pdi->m_Buffer, sizeof(pdi->m_Buffer));
					SecureZeroMemory(&pdi->m_Overlapped, sizeof(OVERLAPPED));
					if (!ReadDirectoryChangesW(pdi->m_hDir,
												pdi->m_Buffer,
												READ_DIR_CHANGE_BUFFER_SIZE,
												TRUE,
												FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
												&numBytes,// not used
												&pdi->m_Overlapped,
												NULL))	//no completion routine!
					{
						// Since the call to ReadDirectoryChangesW failed, just
						// wait a while. We don't want to have this thread
						// running using 100% CPU if something goes completely
						// wrong.
						Sleep(200);
					}
				}
			}
		} // if (watchedPaths.GetCount())
		else
			Sleep(200);
	} // while (m_bRunning)
}

void CDirectoryWatcher::ClearInfoMap()
{
	if (watchInfoMap.size()!=0)
	{
		AutoLocker lock(m_critSec);
		for (std::map<HANDLE, CDirWatchInfo *>::iterator I = watchInfoMap.begin(); I != watchInfoMap.end(); ++I)
		{
			CDirectoryWatcher::CDirWatchInfo * info = I->second;
			delete info;
			info = NULL;
		}
	}
	watchInfoMap.clear();
	if (m_hCompPort != INVALID_HANDLE_VALUE)
		CloseHandle(m_hCompPort);
	m_hCompPort = INVALID_HANDLE_VALUE;
}

CTGitPath CDirectoryWatcher::CloseInfoMap(HDEVNOTIFY hdev)
{
	CTGitPath path;
	if (watchInfoMap.size() == 0)
		return path;
	AutoLocker lock(m_critSec);
	for (std::map<HANDLE, CDirWatchInfo *>::iterator I = watchInfoMap.begin(); I != watchInfoMap.end(); ++I)
	{
		CDirectoryWatcher::CDirWatchInfo * info = I->second;
		if (info->m_hDevNotify == hdev)
		{
			path = info->m_DirName;
			RemovePathAndChildren(path);
			BlockPath(path);
		}
		info->CloseDirectoryHandle();
	}
	watchInfoMap.clear();
	if (m_hCompPort != INVALID_HANDLE_VALUE)
		CloseHandle(m_hCompPort);
	m_hCompPort = INVALID_HANDLE_VALUE;
	return path;
}

bool CDirectoryWatcher::CloseHandlesForPath(const CTGitPath& path)
{
	if (watchInfoMap.size() == 0)
		return false;
	AutoLocker lock(m_critSec);
	for (std::map<HANDLE, CDirWatchInfo *>::iterator I = watchInfoMap.begin(); I != watchInfoMap.end(); ++I)
	{
		CDirectoryWatcher::CDirWatchInfo * info = I->second;
		CTGitPath p = CTGitPath(info->m_DirPath);
		if (path.IsAncestorOf(p))
		{
			RemovePathAndChildren(p);
			BlockPath(p);
		}
		info->CloseDirectoryHandle();
	}
	watchInfoMap.clear();
	if (m_hCompPort != INVALID_HANDLE_VALUE)
		CloseHandle(m_hCompPort);
	m_hCompPort = INVALID_HANDLE_VALUE;
	return true;
}

CDirectoryWatcher::CDirWatchInfo::CDirWatchInfo(HANDLE hDir, const CTGitPath& DirectoryName) :
	m_hDir(hDir),
	m_DirName(DirectoryName)
{
	ATLASSERT( hDir != INVALID_HANDLE_VALUE
		&& !DirectoryName.IsEmpty());
	memset(&m_Overlapped, 0, sizeof(m_Overlapped));
	m_DirPath = m_DirName.GetWinPathString();
	if (m_DirPath.GetAt(m_DirPath.GetLength()-1) != '\\')
		m_DirPath += _T("\\");
	m_hDevNotify = INVALID_HANDLE_VALUE;
}

CDirectoryWatcher::CDirWatchInfo::~CDirWatchInfo()
{
	CloseDirectoryHandle();
}

bool CDirectoryWatcher::CDirWatchInfo::CloseDirectoryHandle()
{
	bool b = TRUE;
	if( m_hDir != INVALID_HANDLE_VALUE )
	{
		b = !!CloseHandle(m_hDir);
		m_hDir = INVALID_HANDLE_VALUE;
	}
	if (m_hDevNotify != INVALID_HANDLE_VALUE)
	{
		UnregisterDeviceNotification(m_hDevNotify);
	}
	return b;
}

