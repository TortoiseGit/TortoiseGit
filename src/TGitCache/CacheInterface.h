// TortoiseGit - a Windows shell extension for easy version control

// External Cache Copyright (C) 2005-2006,2008-2010 - TortoiseSVN
// Copyright (C) 2008-2013, 2016-2017, 2019 - TortoiseGit

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
#include <wininet.h>
#include "GitStatus.h"

// The name of the named-pipe for the cache

#define TGIT_CACHE_PIPE_NAME L"\\\\.\\pipe\\TGitCache"
#define TGIT_CACHE_COMMANDPIPE_NAME L"\\\\.\\pipe\\TGitCacheCommand"
#define TGIT_CACHE_WINDOW_NAME L"TGitCacheWindow"
#define TGIT_CACHE_MUTEX_NAME L"TGitCacheMutex"


CString GetCachePipeName();
CString GetCacheCommandPipeName();
CString GetCacheMutexName();

CString GetCacheID();
bool	SendCacheCommand(BYTE command, const WCHAR* path = nullptr);

/**
 * \ingroup TGitCache
 * RAII class that temporarily disables updates for the given path
 * by sending TGITCACHECOMMAND_BLOCK and TGITCACHECOMMAND_UNBLOCK.
 */
class CBlockCacheForPath
{
private:
	WCHAR path[MAX_PATH];
	bool m_bBlocked;

public:
	CBlockCacheForPath(const WCHAR * aPath);
	~CBlockCacheForPath();
};

/**
 * \ingroup TGitCache
 * A structure passed as a request from the shell (or other client) to the external cache
 */
struct TGITCacheRequest
{
	DWORD flags;
	WCHAR path[MAX_PATH];
};

// CustomActions will use this header but does not need nor understand the SVN types ...

/**
 * \ingroup TGitCache
 * The structure returned as a response
 */
struct TGITCacheResponse
{
	UINT8 m_status;
	bool m_bAssumeValid;
	bool m_bSkipWorktree;
};
static_assert(static_cast<UINT8>(git_wc_status_unknown) == git_wc_status_unknown, "type git_wc_status_kind fits into UINT8");
static_assert(sizeof(TGITCacheResponse) == 3 && offsetof(TGITCacheResponse, m_status) == 0 && offsetof(TGITCacheResponse, m_bAssumeValid) == 1 && offsetof(TGITCacheResponse, m_bSkipWorktree) == 2, "Cross platform compatibility");

/**
 * \ingroup TGitCache
 * a cache command
 */
struct TGITCacheCommand
{
	BYTE command;				///< the command to execute
	WCHAR path[MAX_PATH];		///< path to do the command for
};

#define		TGITCACHECOMMAND_END		0		///< ends the thread handling the pipe communication
#define		TGITCACHECOMMAND_CRAWL		1		///< start crawling the specified path for changes
#define		TGITCACHECOMMAND_REFRESHALL	2		///< Refreshes the whole cache, usually necessary after the "treat unversioned files as modified" option changed.
#define		TGITCACHECOMMAND_RELEASE	3		///< Releases all open handles for the specified path and all paths below
#define		TGITCACHECOMMAND_BLOCK		4		///< Blocks a path from getting crawled for a specific amount of time or until the TGITCACHECOMMAND_UNBLOCK command is sent for that path
#define		TGITCACHECOMMAND_UNBLOCK		5		///< Removes a path from the list of paths blocked from getting crawled

/// Set this flag if you already know whether or not the item is a folder
#define TGITCACHE_FLAGS_FOLDERISKNOWN		0x01
/// Set this flag if the item is a folder
#define TGITCACHE_FLAGS_ISFOLDER			0x02
/// Set this flag if you want recursive folder status (safely ignored for file paths)
#define TGITCACHE_FLAGS_RECUSIVE_STATUS		0x04
/// Set this flag if notifications to the shell are not allowed
#define TGITCACHE_FLAGS_NONOTIFICATIONS		0x08
