// TortoiseGit - a Windows shell extension for easy version control

// External Cache Copyright (C) 2005 - 2006, 2008, 2014 - TortoiseSVN
// Copyright (C) 2008-2012, 2014, 2016-2017 - TortoiseGit

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
#include "TGitPath.h"

/**
 * \ingroup TGitCache
 * Holds the status for a folder and all files and folders directly inside
 * that folder.
 */
#define GIT_CACHE_VERSION 2

class CCachedDirectory
{
public:
	typedef std::map<CTGitPath, CCachedDirectory *> CachedDirMap;
	typedef CachedDirMap::iterator ItDir;

public:

	CCachedDirectory();
	CCachedDirectory(const CTGitPath& directoryPath);
	~CCachedDirectory(void);
	CStatusCacheEntry GetStatusForMember(const CTGitPath& path, bool bRecursive, bool bFetch = true);

private:
	CStatusCacheEntry GetCacheStatusForMember(const CTGitPath& path);

	// If path is not emtpy, means fetch special file status.
	int EnumFiles(const CTGitPath& path, CString sProjectRoot, const CString& sSubPath, bool isSelf);
	CStatusCacheEntry GetOwnStatus(bool bRecursive);
	bool IsOwnStatusValid() const;
public:
	void Invalidate();
	void RefreshStatus(bool bRecursive);
private:
	void RefreshMostImportant(bool bUpdateShell = true);
	BOOL SaveToDisk(FILE * pFile);
	BOOL LoadFromDisk(FILE * pFile);
public:
	/// Get the current full status of this folder
	git_wc_status_kind GetCurrentFullStatus() const {return m_currentFullStatus;}
private:

	CStatusCacheEntry GetStatusFromCache(const CTGitPath &path, bool bRecursive);
	CStatusCacheEntry GetStatusFromGit(const CTGitPath &path, const CString& sProjectRoot, bool isSelf);

	static BOOL GetStatusCallback(const CString& path, const git_wc_status2_t* status, bool isDir, __int64 lastwritetime, void* baton);
	void AddEntry(const CTGitPath& path, const git_wc_status2_t* pGitStatus, __int64 lastwritetime);
	CString GetCacheKey(const CTGitPath& path);
	CString GetFullPathString(const CString& cacheKey);
	void UpdateChildDirectoryStatus(const CTGitPath& childDir, git_wc_status_kind childStatus);

	// Calculate the complete, composite status from ourselves, our files, and our descendants
	git_wc_status_kind CalculateRecursiveStatus();

	// Update our composite status and deal with things if it's changed
	void UpdateCurrentStatus();
	void SetChildStatus(const CString& childDir, git_wc_status_kind childStatus);
	void KeepChildStatus(const CString& childDir);

private:
	CComAutoCriticalSection m_critSec;

	// The cache of files and directories within this directory
	typedef std::map<CString, CStatusCacheEntry> CacheEntryMap;
	CacheEntryMap m_entryCache;
	CacheEntryMap m_entryCache_tmp; // used for updating m_entryCache and removing "removed" entries

	/// A vector if iterators to child directories - used to put-together recursive status
	typedef std::map<CString, git_wc_status_kind>  ChildDirStatus;
	ChildDirStatus m_childDirectories;
	ChildDirStatus m_childDirectories_tmp; // used for updating m_childDirectories and removing "removed" entries

	// The path of the directory with this object looks after
	CTGitPath	m_directoryPath;

	// The status of THIS directory (not a composite of children or members)
	CStatusCacheEntry m_ownStatus;

	// Our current fully recursive status
	git_wc_status_kind  m_currentFullStatus;

	// The most important status from all our file entries
	git_wc_status_kind m_mostImportantFileStatus;

	bool m_bRecursive;		// used in the status callback
	friend class CGitStatusCache;
};

