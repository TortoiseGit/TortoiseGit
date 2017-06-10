// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2012, 2014, 2016-2017 - TortoiseGit
// Copyright (C) 2003-2006,2008,2011 - TortoiseSVN

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

#pragma once

#include "GitStatus.h"
#include "TGitPath.h"
#include "SmartHandle.h"

typedef struct FileStatusCacheEntry
{
	git_wc_status_kind		status;
	int						askedcounter;
	bool					assumeValid;
	bool					skipWorktree;
} FileStatusCacheEntry;

#define GITFOLDERSTATUS_CACHETIMES				10
#define GITFOLDERSTATUS_CACHETIMEOUT			2000
#define GITFOLDERSTATUS_RECURSIVECACHETIMEOUT	4000
#define GITFOLDERSTATUS_FOLDER					500
/**
 * \ingroup TortoiseShell
 * This class represents a caching mechanism for the
 * git statuses. Once a status for a versioned
 * file is requested (GetFileStatus()) first its checked
 * if that status is already in the cache. If it is not
 * then the git statuses for ALL files in the same
 * directory is fetched and cached. This is because git
 * needs almost the same time to get one or all status (in
 * the same directory).
 * To prevent a cache flush for the explorer folder view
 * the cache is only fetched for versioned files and
 * not for folders.
 */
class GitFolderStatus
{
public:
	GitFolderStatus(void);
	~GitFolderStatus(void);
	const FileStatusCacheEntry *	GetFullStatus(const CTGitPath& filepath, BOOL bIsFolder);
	const FileStatusCacheEntry *	GetCachedItem(const CTGitPath& filepath);

	FileStatusCacheEntry		invalidstatus;

	GitStatus	m_GitStatus;

private:
	const FileStatusCacheEntry * BuildCache(const CTGitPath& filepath, const CString& sProjectRoot, BOOL bIsFolder, BOOL bDirectFolder = FALSE);
	ULONGLONG			GetTimeoutValue();

	void				ClearCache();

	typedef std::map<std::wstring, FileStatusCacheEntry> FileStatusMap;
	FileStatusMap			m_cache;
	ULONGLONG				m_TimeStamp;
	FileStatusCacheEntry	dirstat;
	git_wc_status2_t *		dirstatus;

	std::wstring			sCacheKey;

	CAutoGeneralHandle	m_hInvalidationEvent;

	// The item we most recently supplied status for
	CTGitPath		m_mostRecentPath;
	const FileStatusCacheEntry* m_mostRecentStatus;
};

