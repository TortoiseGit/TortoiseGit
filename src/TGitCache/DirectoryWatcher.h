// TortoiseGit - a Windows shell extension for easy version control

// External Cache Copyright (C) 2005-2008, 2012 - TortoiseSVN

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
#pragma once
#include "TGitPath.h"
#include "FolderCrawler.h"
#include "ShellCache.h"
#include "SmartHandle.h"

#define READ_DIR_CHANGE_BUFFER_SIZE 4096

/**
 * \ingroup TGitCache
 * Watches the file system for changes.
 * When changes are detected, those changes are reported back to the CFolderCrawler
 * which then can update the status cache.
 *
 * When a CDirectoryWatcher object is created, a new thread is started which
 * waits for file system change notifications.
 * To add folders to the list of watched folders, call \c AddPath().
 *
 * The folders are watched recursively. To prevent having too many folders watched,
 * children of already watched folders are automatically removed from watching.
 * This leads to having only the roots of file systems watched (e.g. C:\, D:\,...)
 * after a few paths have been added to the watched list (at least, when the
 * CGitStatusCache adds those paths).
 */
class CDirectoryWatcher
{
public:
	CDirectoryWatcher(void);
	~CDirectoryWatcher(void);

	/**
	 * Adds a new path to be watched. The path \b must point to a directory.
	 * If the path is already watched because a parent of that path is already
	 * watched recursively, then the new path is just ignored and the method
	 * returns false.
	 */
	bool AddPath(const CTGitPath& path, bool bCloseInfoMap = true);
	/**
	 * Removes a path and all its children from the watched list.
	 */
	bool RemovePathAndChildren(const CTGitPath& path);
	/**
	 * Checks if a path is watched
	 */
	bool IsPathWatched(const CTGitPath& path);

	/**
	 * Returns the number of recursively watched paths.
	 */
	int GetNumberOfWatchedPaths() {return watchedPaths.GetCount();}

	/**
	 * Sets the CFolderCrawler object which the change notifications are sent to.
	 */
	void SetFolderCrawler(CFolderCrawler * crawler);

	/**
	 * Stops the watching thread.
	 */
	void Stop();

	CTGitPath CloseInfoMap(HANDLE hDir);
	void ClearInfoMap();
	bool CloseHandlesForPath(const CTGitPath& path);

private:
	static unsigned int __stdcall ThreadEntry(void* pContext);
	void WorkerThread();

	void CloseWatchHandles();

	void BlockPath(const CTGitPath& path);

	// close handle (if open) and
	// release all async I/O objects

	void CloseCompletionPort();

	// enqueue the info object for deletion as soon as the
	// completion port is no longer used

	class CDirWatchInfo;
	void ScheduleForDeletion(CDirWatchInfo* info);
	void CleanupWatchInfo();

private:
	CComAutoCriticalSection m_critSec;
	CAutoGeneralHandle		m_hThread;
	CAutoGeneralHandle		m_hCompPort;
	volatile LONG			m_bRunning;
	volatile LONG			m_bCleaned;

	CFolderCrawler *		m_FolderCrawler;	///< where the change reports go to

	CTGitPathList			watchedPaths;	///< list of watched paths.

	CTGitPath				blockedPath;
	ULONGLONG				blockTickCount;

	/**
	 * \ingroup TGitCache
	 * Helper class: provides information about watched directories.
	 */
	class CDirWatchInfo
	{
	private:
		CDirWatchInfo() = delete;
		CDirWatchInfo & operator=(const CDirWatchInfo & rhs) = delete; //so that they're aren't accidentally used. -- you'll get a linker error
	public:
		CDirWatchInfo(HANDLE hDir, const CTGitPath& DirectoryName);
		~CDirWatchInfo();

	protected:
	public:
		bool	CloseDirectoryHandle();

		CAutoFile	m_hDir;			///< handle to the directory that we're watching
		CTGitPath	m_DirName;		///< the directory that we're watching
		CHAR		m_Buffer[READ_DIR_CHANGE_BUFFER_SIZE]; ///< buffer for ReadDirectoryChangesW
		OVERLAPPED	m_Overlapped;
		CString		m_DirPath;		///< the directory name we're watching with a backslash at the end
		HDEVNOTIFY	m_hDevNotify;	///< Notification handle
	};

	typedef std::map<HANDLE, CDirWatchInfo *> TInfoMap;
	TInfoMap watchInfoMap;

	HDEVNOTIFY		m_hdev;

	// scheduled for deletion upon the next CleanupWatchInfo()
	std::vector<CDirWatchInfo*> infoToDelete;
};
