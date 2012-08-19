// TortoiseGit - a Windows shell extension for easy version control

// External Cache Copyright (C) 2005-2006,2008,2010 - TortoiseSVN
// Copyright (C) 2008-2012 - TortoiseGit

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
#include "wininet.h"

#include "GitStatus.h"
// The name of the named-pipe for the cache

#define TGIT_CACHE_PIPE_NAME _T("\\\\.\\pipe\\TGitCache")
#define TGIT_CACHE_COMMANDPIPE_NAME _T("\\\\.\\pipe\\TGitCacheCommand")
#define TGIT_CACHE_WINDOW_NAME _T("TGitCacheWindow")
#define TGIT_CACHE_MUTEX_NAME _T("TGitCacheMutex")


CString GetCachePipeName();
CString GetCacheCommandPipeName();
CString GetCacheMutexName();

CString GetCacheID();
bool	SendCacheCommand(BYTE command, const WCHAR * path = NULL);

typedef enum git_node_kind_t
{
	git_node_none,
	git_node_file,
	git_node_dir,
	git_node_unknown,

}git_node_kind;


/**
 * RAII class that temporarily disables updates for the given path
 * by sending TGITCACHECOMMAND_BLOCK and TGITCACHECOMMAND_UNBLOCK.
 */
class CBlockCacheForPath
{
private:
	WCHAR path[MAX_PATH+1];

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
	WCHAR path[MAX_PATH+1];
};

// CustomActions will use this header but does not need nor understand the SVN types ...

/**
 * \ingroup TGitCache
 * The structure returned as a response
 */
struct TGITCacheResponse
{
	git_wc_status2_t m_status;
	bool m_bAssumeValid;
	bool m_bSkipWorktree;
};

/**
 * \ingroup TGitCache
 * a cache command
 */
struct TGITCacheCommand
{
	BYTE command;				///< the command to execute
	WCHAR path[MAX_PATH+1];		///< path to do the command for
};

#define		TGITCACHECOMMAND_END		0		///< ends the thread handling the pipe communication
#define		TGITCACHECOMMAND_CRAWL		1		///< start crawling the specified path for changes
#define		TGITCACHECOMMAND_REFRESHALL	2		///< Refreshes the whole cache, usually necessary after the "treat unversioned files as modified" option changed.
#define		TGITCACHECOMMAND_RELEASE	3		///< Releases all open handles for the specified path and all paths below
#define		TGITCACHECOMMAND_BLOCK		4		///< Blocks a path from getting crawled for a specific amount of time or until the TSVNCACHECOMMAND_UNBLOCK command is sent for that path
#define		TGITCACHECOMMAND_UNBLOCK		5		///< Removes a path from the list of paths blocked from getting crawled


/// Set this flag if you already know whether or not the item is a folder
#define TGITCACHE_FLAGS_FOLDERISKNOWN		0x01
/// Set this flag if the item is a folder
#define TGITCACHE_FLAGS_ISFOLDER			0x02
/// Set this flag if you want recursive folder status (safely ignored for file paths)
#define TGITCACHE_FLAGS_RECUSIVE_STATUS		0x04
/// Set this flag if notifications to the shell are not allowed
#define TGITCACHE_FLAGS_NONOTIFICATIONS		0x08
