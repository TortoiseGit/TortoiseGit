// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2008,2011, 2014 - TortoiseSVN
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

#include "stdafx.h"
#include "ShellCache.h"
#include "GitFolderStatus.h"
#include "UnicodeUtils.h"
#include "..\TGitCache\CacheInterface.h"
#include "Git.h"
#include "gitindex.h"

extern ShellCache g_ShellCache;

GitFolderStatus::GitFolderStatus(void)
{
	m_TimeStamp = 0;
	invalidstatus.askedcounter = -1;
	invalidstatus.status = git_wc_status_none;
	invalidstatus.assumeValid = FALSE;
	invalidstatus.skipWorktree = FALSE;
	dirstat.askedcounter = -1;
	dirstat.assumeValid = dirstat.skipWorktree = false;
	dirstat.status = git_wc_status_none;
	dirstatus = nullptr;
	m_mostRecentStatus = nullptr;
	sCacheKey.reserve(MAX_PATH);

	g_Git.SetCurrentDir(L"");
	m_hInvalidationEvent = CreateEvent(nullptr, FALSE, FALSE, L"TortoiseGitCacheInvalidationEvent"); // no need to explicitly close m_hInvalidationEvent in ~GitFolderStatus as it is CAutoGeneralHandle
}

GitFolderStatus::~GitFolderStatus(void)
{
}

const FileStatusCacheEntry * GitFolderStatus::BuildCache(const CTGitPath& filepath, const CString& /*sProjectRoot*/, BOOL bIsFolder, BOOL bDirectFolder)
{
	//dont' build the cache if an instance of TortoiseGitProc is running
	//since this could interfere with svn commands running (concurrent
	//access of the .git directory).
	if (g_ShellCache.BlockStatus())
	{
		CAutoGeneralHandle TGitMutex = ::CreateMutex(nullptr, FALSE, L"TortoiseGitProc.exe");
		if (TGitMutex != nullptr)
		{
			if (::GetLastError() == ERROR_ALREADY_EXISTS)
				return &invalidstatus;
		}
	}

	ClearCache();

	if (bIsFolder)
	{
		if (bDirectFolder)
		{
			// NOTE: see not in GetFullStatus about project inside another project, we should only get here when
			//       that occurs, and this is not correctly handled yet

			// initialize record members
			dirstat.status = git_wc_status_none;
			dirstat.askedcounter = GITFOLDERSTATUS_CACHETIMES;
			dirstat.assumeValid = FALSE;
			dirstat.skipWorktree = FALSE;

			dirstatus = nullptr;
//			rev.kind = git_opt_revision_unspecified;


			if (dirstatus)
				dirstat.status = dirstatus->status;
			m_cache[filepath.GetWinPath()] = dirstat;
			m_TimeStamp = GetTickCount64();
			return &dirstat;
		}
	} // if (bIsFolder)

	git_wc_status2_t status = { git_wc_status_none, false, false };
	int t1,t2;
	t2=t1=0;
	try
	{
		t1 = ::GetCurrentTime();
		if (m_GitStatus.GetAllStatus(filepath, g_ShellCache.GetCacheType() != ShellCache::dll, status))
			status = { git_wc_status_none, false, false };
		t2 = ::GetCurrentTime();
	}
	catch ( ... )
	{
		status = { git_wc_status_none, false, false };
	}

	CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": building cache for %s - time %d\n", filepath.GetWinPath(), t2 - t1);

	m_TimeStamp = GetTickCount64();
	FileStatusCacheEntry* ret = nullptr;

	if (wcslen(filepath.GetWinPath()) == 3)
		ret = &m_cache[static_cast<LPCTSTR>(filepath.GetWinPathString().Left(2))];
	else
		ret = &m_cache[filepath.GetWinPath()];

	if (ret)
	{
		ret->status = status.status;
		ret->assumeValid = status.assumeValid;
		ret->skipWorktree = status.skipWorktree;
	}

	m_mostRecentPath = filepath;
	m_mostRecentStatus = ret;

	if (ret)
		return ret;
	return &invalidstatus;
}

ULONGLONG GitFolderStatus::GetTimeoutValue()
{
	ULONGLONG timeout = GITFOLDERSTATUS_CACHETIMEOUT;
	ULONGLONG factor = static_cast<ULONGLONG>(m_cache.size()) / 200UL;
	if (factor==0)
		factor = 1;
	return factor*timeout;
}

const FileStatusCacheEntry * GitFolderStatus::GetFullStatus(const CTGitPath& filepath, BOOL bIsFolder)
{
	CString sProjectRoot;
	BOOL bHasAdminDir = g_ShellCache.HasGITAdminDir(filepath.GetWinPath(), bIsFolder, &sProjectRoot);

	//no overlay for unversioned folders
	if (!bHasAdminDir)
		return &invalidstatus;
	//for the SVNStatus column, we have to check the cache to see
	//if it's not just unversioned but ignored
	const FileStatusCacheEntry* ret = GetCachedItem(filepath);
	if ((ret)&&(ret->status == git_wc_status_unversioned)&&(bIsFolder)&&(bHasAdminDir))
	{
		// an 'unversioned' folder, but with an ADMIN dir --> nested layout!
		// NOTE: this could be a sub-project in git, or just some standalone project inside of another, either way a TODO
		ret = BuildCache(filepath, sProjectRoot, bIsFolder, TRUE);
		if (ret)
			return ret;
		else
			return &invalidstatus;
	}
	if (ret)
		return ret;

	//if it's not in the cache and has no admin dir, then we assume
	//it's not ignored too
	ret = BuildCache(filepath, sProjectRoot, bIsFolder);
	if (ret)
		return ret;
	else
		return &invalidstatus;
}

const FileStatusCacheEntry * GitFolderStatus::GetCachedItem(const CTGitPath& filepath)
{
	sCacheKey.assign(filepath.GetWinPath());
	FileStatusMap::const_iterator iter;
	const FileStatusCacheEntry* retVal = nullptr;

	if(m_mostRecentPath.IsEquivalentTo(CTGitPath(sCacheKey.c_str())))
	{
		// We've hit the same result as we were asked for last time
		CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": fast cache hit for %s\n", filepath.GetWinPath());
		retVal = m_mostRecentStatus;
	}
	else if ((iter = m_cache.find(sCacheKey)) != m_cache.end())
	{
		CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": cache found for %s\n", filepath.GetWinPath());
		retVal = &iter->second;
		m_mostRecentStatus = retVal;
		m_mostRecentPath = CTGitPath(sCacheKey.c_str());
	}

	if (!retVal)
		return nullptr;

	// We found something in a cache - check that the cache is not timed-out or force-invalidated
	ULONGLONG now = GetTickCount64();

	if ((now >= m_TimeStamp) && ((now - m_TimeStamp) > GetTimeoutValue()))
	{
		// Cache is timed-out
		CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Cache timed-out\n");
		ClearCache();
		return nullptr;
	}
	else if (WaitForSingleObject(m_hInvalidationEvent, 0) == WAIT_OBJECT_0)
	{
		// TortoiseGitProc has just done something which has invalidated the cache
		CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Cache invalidated\n");
		ClearCache();
		return nullptr;
	}
	return retVal;
}

void GitFolderStatus::ClearCache()
{
	m_cache.clear();
	m_mostRecentStatus = nullptr;
	m_mostRecentPath.Reset();
	// If we're about to rebuild the cache, there's no point hanging on to
	// an event which tells us that it's invalid
	ResetEvent(m_hInvalidationEvent);
}
