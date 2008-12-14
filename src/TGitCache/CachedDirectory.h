// TortoiseSVN - a Windows shell extension for easy version control

// External Cache Copyright (C) 2005 - 2006, 2008 - TortoiseSVN

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

#include "StatusCacheEntry.h"
#include "TSVNPath.h"

/**
 * \ingroup TSVNCache
 * Holds the status for a folder and all files and folders directly inside
 * that folder.
 */
class CCachedDirectory
{
public:
	typedef std::map<CTSVNPath, CCachedDirectory *> CachedDirMap; 
	typedef CachedDirMap::iterator ItDir;

public:

	CCachedDirectory();
	CCachedDirectory(const CTSVNPath& directoryPath);
	~CCachedDirectory(void);
	CStatusCacheEntry GetStatusForMember(const CTSVNPath& path, bool bRecursive, bool bFetch = true);
	CStatusCacheEntry GetOwnStatus(bool bRecursive);
	bool IsOwnStatusValid() const;
	void Invalidate();
	void RefreshStatus(bool bRecursive);
	void RefreshMostImportant();
	BOOL SaveToDisk(FILE * pFile);
	BOOL LoadFromDisk(FILE * pFile);
	/// Get the current full status of this folder
	svn_wc_status_kind GetCurrentFullStatus() {return m_currentFullStatus;}
private:
	static svn_error_t* GetStatusCallback(void *baton, const char *path, svn_wc_status2_t *status, apr_pool_t *pool);
	void AddEntry(const CTSVNPath& path, const svn_wc_status2_t* pSVNStatus, DWORD validuntil = 0);
	CString GetCacheKey(const CTSVNPath& path);
	CString GetFullPathString(const CString& cacheKey);
	CStatusCacheEntry LookForItemInCache(const CTSVNPath& path, bool &bFound);
	void UpdateChildDirectoryStatus(const CTSVNPath& childDir, svn_wc_status_kind childStatus);

	// Calculate the complete, composite status from ourselves, our files, and our descendants
	svn_wc_status_kind CalculateRecursiveStatus();

	// Update our composite status and deal with things if it's changed
	void UpdateCurrentStatus();


private:
	CComAutoCriticalSection m_critSec;
	CComAutoCriticalSection m_critSecPath;

	CTSVNPath	m_currentStatusFetchingPath;
	DWORD		m_currentStatusFetchingPathTicks;
	// The cache of files and directories within this directory
	typedef std::map<CString, CStatusCacheEntry> CacheEntryMap; 
	CacheEntryMap m_entryCache; 

	/// A vector if iterators to child directories - used to put-together recursive status
	typedef std::map<CTSVNPath, svn_wc_status_kind>  ChildDirStatus;
	ChildDirStatus m_childDirectories;

	// The timestamp of the .SVN\entries file.  For an unversioned directory, this will be zero
	__int64 m_entriesFileTime;
	// The timestamp of the .SVN\props dir.  For an unversioned directory, this will be zero
	__int64 m_propsFileTime;
	
	// The path of the directory with this object looks after
	CTSVNPath	m_directoryPath;

	// The status of THIS directory (not a composite of children or members)
	CStatusCacheEntry m_ownStatus;

	// Our current fully recursive status
	svn_wc_status_kind  m_currentFullStatus;
	bool m_bCurrentFullStatusValid;

	// The most important status from all our file entries
	svn_wc_status_kind m_mostImportantFileStatus;

	bool m_bRecursive;		// used in the status callback
	friend class CSVNStatusCache;		
};

