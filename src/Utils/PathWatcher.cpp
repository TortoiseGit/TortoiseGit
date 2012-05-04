// TortoiseGit - a Windows shell extension for easy version control

// External Cache Copyright (C) 2007 - 2007 - Stefan Kueng

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
#include "PathWatcher.h"

CPathWatcher::CPathWatcher(void) : m_hCompPort(NULL)
	, m_bRunning(TRUE)
{
	// enable the required privileges for this process
	LPCTSTR arPrivelegeNames[] = {	SE_BACKUP_NAME,
									SE_RESTORE_NAME,
									SE_CHANGE_NOTIFY_NAME
								 };

	for (int i=0; i<(sizeof(arPrivelegeNames)/sizeof(LPCTSTR)); ++i)
	{
		CAutoGeneralHandle hToken;
		if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, hToken.GetPointer()))
		{
			TOKEN_PRIVILEGES tp = { 1 };

			if (LookupPrivilegeValue(NULL, arPrivelegeNames[i],  &tp.Privileges[0].Luid))
			{
				tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

				AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
			}
		}
	}

	unsigned int threadId;
	m_hThread = (HANDLE)_beginthreadex(NULL,0,ThreadEntry,this,0,&threadId);
}

CPathWatcher::~CPathWatcher(void)
{
	InterlockedExchange(&m_bRunning, FALSE);
	m_hThread.CloseHandle();
	AutoLocker lock(m_critSec);
	ClearInfoMap();
}

void CPathWatcher::Stop()
{
	InterlockedExchange(&m_bRunning, FALSE);
	if (m_hCompPort)
	{
		PostQueuedCompletionStatus(m_hCompPort, 0, NULL, NULL);
		m_hCompPort.CloseHandle();
	}

	if (m_hThread)
	{
		if( WaitForSingleObject(m_hThread, 1000) != WAIT_OBJECT_0 )
		{
			TerminateThread(m_hThread, (DWORD)-1);
		}
		m_hThread.CloseHandle();
	}
}

bool CPathWatcher::RemovePathAndChildren(const CTGitPath& path)
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

bool CPathWatcher::AddPath(const CTGitPath& path)
{
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
		m_hCompPort.CloseHandle();
		return true;
	}
	ATLTRACE(_T("add path to watch %s\n"), path.GetWinPath());
	watchedPaths.AddPath(path);
	m_hCompPort.CloseHandle();
	return true;
}


unsigned int CPathWatcher::ThreadEntry(void* pContext)
{
	((CPathWatcher*)pContext)->WorkerThread();
	return 0;
}

void CPathWatcher::WorkerThread()
{
	DWORD numBytes;
	CDirWatchInfo * pdi = NULL;
	LPOVERLAPPED lpOverlapped;
	WCHAR buf[MAX_PATH*4] = {0};
	while (m_bRunning)
	{
		if (watchedPaths.GetCount())
		{
			if (!m_hCompPort || !GetQueuedCompletionStatus(m_hCompPort,
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
				if ((m_hCompPort)&&(lasterr!=ERROR_SUCCESS)&&(lasterr!=ERROR_INVALID_HANDLE))
				{
					m_hCompPort.CloseHandle();
				}
				// Since we pass m_hCompPort to CreateIoCompletionPort, we
				// have to set this to NULL to have that API create a new
				// handle.
				m_hCompPort = NULL;
				for (int i=0; i<watchedPaths.GetCount(); ++i)
				{
					CAutoFile hDir = CreateFile(watchedPaths[i].GetWinPath(),
											FILE_LIST_DIRECTORY,
											FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
											NULL, //security attributes
											OPEN_EXISTING,
											FILE_FLAG_BACKUP_SEMANTICS | //required privileges: SE_BACKUP_NAME and SE_RESTORE_NAME.
											FILE_FLAG_OVERLAPPED,
											NULL);
					if (!hDir)
					{
						// this could happen if a watched folder has been removed/renamed
						m_hCompPort.CloseHandle();
						AutoLocker lock(m_critSec);
						watchedPaths.RemovePath(watchedPaths[i]);
						i--; if (i<0) i=0;
						break;
					}

					CDirWatchInfo * pDirInfo = new CDirWatchInfo(hDir, watchedPaths[i]);
					hDir.Detach();  // the new CDirWatchInfo object owns the handle now
					m_hCompPort = CreateIoCompletionPort(pDirInfo->m_hDir, m_hCompPort, (ULONG_PTR)pDirInfo, 0);
					if (m_hCompPort == NULL)
					{
						AutoLocker lock(m_critSec);
						ClearInfoMap();
						delete pDirInfo;
						pDirInfo = NULL;
						watchedPaths.RemovePath(watchedPaths[i]);
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
						AutoLocker lock(m_critSec);
						ClearInfoMap();
						delete pDirInfo;
						pDirInfo = NULL;
						watchedPaths.RemovePath(watchedPaths[i]);
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
						SecureZeroMemory(buf, MAX_PATH*4*sizeof(TCHAR));
						_tcsncpy_s(buf, MAX_PATH*4, pdi->m_DirPath, MAX_PATH*4);
						errno_t err = _tcsncat_s(buf+pdi->m_DirPath.GetLength(), (MAX_PATH*4)-pdi->m_DirPath.GetLength(), pnotify->FileName, _TRUNCATE);
						if (err == STRUNCATE)
						{
							pnotify = (PFILE_NOTIFY_INFORMATION)((LPBYTE)pnotify + nOffset);
							continue;
						}
						buf[min(MAX_PATH*4-1, pdi->m_DirPath.GetLength()+(pnotify->FileNameLength/sizeof(WCHAR)))] = 0;
						pnotify = (PFILE_NOTIFY_INFORMATION)((LPBYTE)pnotify + nOffset);
						ATLTRACE(_T("change notification: %s\n"), buf);
						m_changedPaths.AddPath(CTGitPath(buf));
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
		}// if (watchedPaths.GetCount())
		else
			Sleep(200);
	}// while (m_bRunning)
}

void CPathWatcher::ClearInfoMap()
{
	if (watchInfoMap.size()!=0)
	{
		AutoLocker lock(m_critSec);
		for (std::map<HANDLE, CDirWatchInfo *>::iterator I = watchInfoMap.begin(); I != watchInfoMap.end(); ++I)
		{
			CPathWatcher::CDirWatchInfo * info = I->second;
			delete info;
			info = NULL;
		}
	}
	watchInfoMap.clear();
	m_hCompPort.CloseHandle();
}

CPathWatcher::CDirWatchInfo::CDirWatchInfo(HANDLE hDir, const CTGitPath& DirectoryName) :
	m_hDir(hDir),
	m_DirName(DirectoryName)
{
	ATLASSERT( hDir && !DirectoryName.IsEmpty());
	m_Buffer[0] = 0;
	memset(&m_Overlapped, 0, sizeof(m_Overlapped));
	m_DirPath = m_DirName.GetWinPathString();
	if (m_DirPath.GetAt(m_DirPath.GetLength()-1) != '\\')
		m_DirPath += _T("\\");
}

CPathWatcher::CDirWatchInfo::~CDirWatchInfo()
{
	CloseDirectoryHandle();
}

bool CPathWatcher::CDirWatchInfo::CloseDirectoryHandle()
{
	return m_hDir.CloseHandle();
}

