// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2013, 2015-2019 - TortoiseGit
// Copyright (C) 2003-2008, 2017 - TortoiseSVN

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
#include "ShellExt.h"
#include "Guids.h"
#include "PreserveChdir.h"
#include "UnicodeUtils.h"
#include "GitStatus.h"
#include "../TGitCache/CacheInterface.h"
#include "GitAdminDir.h"
#include "StringUtils.h"

// "The Shell calls IShellIconOverlayIdentifier::GetOverlayInfo to request the
//  location of the handler's icon overlay. The icon overlay handler returns
//  the name of the file containing the overlay image, and its index within
//  that file. The Shell then adds the icon overlay to the system image list."

STDMETHODIMP CShellExt::GetOverlayInfo(LPWSTR pwszIconFile, int cchMax, int* pIndex, DWORD* pdwFlags)
{
	if (!pwszIconFile)
		return E_POINTER;
	if (!pIndex)
		return E_POINTER;
	if (!pdwFlags)
		return E_POINTER;
	if(cchMax < 1)
		return E_INVALIDARG;

	// Set "out parameters" since we return S_OK later.
	*pwszIconFile = 0;
	*pIndex = 0;
	*pdwFlags = 0;

	// Now here's where we can find out if due to lack of enough overlay
	// slots some of our overlays won't be shown.
	// To do that we have to mark every overlay handler that's successfully
	// loaded, so we can later check if some are missing
	switch (m_State)
	{
		case FileStateVersioned				: g_normalovlloaded = true; break;
		case FileStateModified				: g_modifiedovlloaded = true; break;
		case FileStateConflict				: g_conflictedovlloaded = true; break;
		case FileStateDeleted				: g_deletedovlloaded = true; break;
		case FileStateReadOnly				: g_readonlyovlloaded = true; break;
		case FileStateLockedOverlay			: g_lockedovlloaded = true; break;
		case FileStateAddedOverlay			: g_addedovlloaded = true; break;
		case FileStateIgnoredOverlay		: g_ignoredovlloaded = true; break;
		case FileStateUnversionedOverlay	: g_unversionedovlloaded = true; break;
	}

	// we don't have to set the icon file and/or the index here:
	// the icons are handled by the TortoiseOverlays dll.
	return S_OK;
};

STDMETHODIMP CShellExt::GetPriority(int *pPriority)
{
	if (!pPriority)
		return E_POINTER;

	switch (m_State)
	{
		case FileStateConflict:
			*pPriority = 0;
			break;
		case FileStateModified:
			*pPriority = 1;
			break;
		case FileStateDeleted:
			*pPriority = 2;
			break;
		case FileStateReadOnly:
			*pPriority = 3;
			break;
		case FileStateLockedOverlay:
			*pPriority = 4;
			break;
		case FileStateAddedOverlay:
			*pPriority = 5;
			break;
		case FileStateVersioned:
			*pPriority = 6;
			break;
		default:
			*pPriority = 100;
			return S_FALSE;
	}
	return S_OK;
}

// "Before painting an object's icon, the Shell passes the object's name to
//  each icon overlay handler's IShellIconOverlayIdentifier::IsMemberOf
//  method. If a handler wants to have its icon overlay displayed,
//  it returns S_OK. The Shell then calls the handler's
//  IShellIconOverlayIdentifier::GetOverlayInfo method to determine which icon
//  to display."

STDMETHODIMP CShellExt::IsMemberOf(LPCWSTR pwszPath, DWORD /*dwAttrib*/)
{
	if (!pwszPath)
		return E_INVALIDARG;
	const TCHAR* pPath = pwszPath;
	// the shell sometimes asks overlays for invalid paths, e.g. for network
	// printers (in that case the path is "0", at least for me here).
	if (wcslen(pPath) < 2)
		return S_FALSE;
	PreserveChdir preserveChdir;
	git_wc_status_kind status = git_wc_status_none;
	bool readonlyoverlay = false;
	bool lockedoverlay = false;

	// since the shell calls each and every overlay handler with the same filepath
	// we use a small 'fast' cache of just one path here.
	// To make sure that cache expires, clear it as soon as one handler is used.

	AutoLocker lock(g_csGlobalCOMGuard);
	if (wcscmp(pPath, g_filepath.c_str()) == 0)
	{
		status = g_filestatus;
		readonlyoverlay = g_readonlyoverlay;
		lockedoverlay = g_lockedoverlay;
	}
	else
	{
		if (!g_ShellCache.IsPathAllowed(pPath))
		{
			if ((m_State == FileStateVersioned) && g_ShellCache.ShowExcludedAsNormal() &&
				(PathGetDriveNumber(pPath)>1) &&
				PathIsDirectory(pPath) && g_ShellCache.HasGITAdminDir(pPath, true))
			{
				return S_OK;
			}
			return S_FALSE;
		}

		auto cacheType = g_ShellCache.GetCacheType();
		if (g_ShellCache.IsOnlyNonElevated() && g_ShellCache.IsProcessElevated())
		{
			cacheType = ShellCache::none;
			CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": elevated overlays blocked\n");
		}
		switch (cacheType)
		{
		case ShellCache::exe:
			{
				CTGitPath tpath(pPath);
				if(!tpath.HasAdminDir())
				{
					status = git_wc_status_none;
					break;
				}
				if(tpath.IsAdminDir())
				{
					status = git_wc_status_none;
					break;
				}
				TGITCacheResponse itemStatus;
				SecureZeroMemory(&itemStatus, sizeof(itemStatus));
				if (m_remoteCacheLink.GetStatusFromRemoteCache(tpath, &itemStatus, true))
				{
					if (itemStatus.m_bAssumeValid)
						readonlyoverlay = true;
					if (itemStatus.m_bSkipWorktree)
						lockedoverlay = true;
					status = static_cast<git_wc_status_kind>(itemStatus.m_status);
				}
			}
			break;
		case ShellCache::dll:
		case ShellCache::dllFull:
			{
				// Look in our caches for this item
				const FileStatusCacheEntry * s = m_CachedStatus.GetCachedItem(CTGitPath(pPath));
				if (s)
				{
					status = s->status;
					if (s->assumeValid)
						readonlyoverlay = true;
					if (s->skipWorktree)
						lockedoverlay = true;
				}
				else
				{
					// No cached status available

					// since the dwAttrib param of the IsMemberOf() function does not
					// have the SFGAO_FOLDER flag set at all (it's 0 for files and folders!)
					// we have to check if the path is a folder ourselves :(
					if (PathIsDirectory(pPath))
					{
						if (g_ShellCache.HasGITAdminDir(pPath, TRUE))
						{
							if ((!g_ShellCache.IsRecursive()) && (!g_ShellCache.IsFolderOverlay()))
								status = git_wc_status_normal;
							else
							{
								s = m_CachedStatus.GetFullStatus(CTGitPath(pPath), TRUE);
								status = s->status;
							}
						}
						else
							status = git_wc_status_none;
					}
					else if (CStringUtils::EndsWith(pPath, GitAdminDir::GetAdminDirName()))
						status = git_wc_status_none;
					else
					{
						s = m_CachedStatus.GetFullStatus(CTGitPath(pPath), FALSE);
						status = s->status;
						if (s->assumeValid)
							readonlyoverlay = true;
						if (s->skipWorktree)
							lockedoverlay = true;
					}
				}
			}

			break;
		default:
		case ShellCache::none:
			{
				// no cache means we only show a 'versioned' overlay on folders
				// with an admin directory
				if (PathIsDirectory(pPath))
				{
					if (g_ShellCache.HasGITAdminDir(pPath, TRUE))
						status = git_wc_status_normal;
					else
						status = git_wc_status_none;
				}
				else
					status = git_wc_status_none;
			}
			break;
		}
		CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Status %d for file %s\n", status, pwszPath);
	}
	g_filepath.clear();
	g_filepath = pPath;
	g_filestatus = status;
	g_readonlyoverlay = readonlyoverlay;
	g_lockedoverlay = lockedoverlay;

	//the priority system of the shell doesn't seem to work as expected (or as I expected):
	//as it seems that if one handler returns S_OK then that handler is used, no matter
	//if other handlers would return S_OK too (they're never called on my machine!)
	//So we return S_OK for ONLY ONE handler!

	switch (status)
	{
		// note: we can show other overlays if due to lack of enough free overlay
		// slots some of our overlays aren't loaded. But we assume that
		// at least the 'normal' overlay is available.
		// if the 'modified' overlay isn't available, we show the 'normal' overlay,
		// but in this case the overlays don't really provide anything useful anymore.
		case git_wc_status_none:
			return S_FALSE;
		case git_wc_status_unversioned:
			if (g_ShellCache.ShowUnversionedOverlay() && g_unversionedovlloaded && (m_State == FileStateUnversionedOverlay))
			{
				g_filepath.clear();
				return S_OK;
			}
			return S_FALSE;
		case git_wc_status_ignored:
			if (g_ShellCache.ShowIgnoredOverlay() && g_ignoredovlloaded && (m_State == FileStateIgnoredOverlay))
			{
				g_filepath.clear();
				return S_OK;
			}
			return S_FALSE;
		case git_wc_status_normal:
			// skip-worktree aka locked has higher priority than assume-valid
			if ((lockedoverlay)&&(g_lockedovlloaded))
			{
				if (m_State == FileStateLockedOverlay)
				{
					g_filepath.clear();
					return S_OK;
				}
				else
					return S_FALSE;
			}
			else if ((readonlyoverlay)&&(g_readonlyovlloaded))
			{
				if (m_State == FileStateReadOnly)
				{
					g_filepath.clear();
					return S_OK;
				}
				else
					return S_FALSE;
			}
			else if (m_State == FileStateVersioned)
			{
				g_filepath.clear();
				return S_OK;
			}
			else
				return S_FALSE;
		case git_wc_status_deleted:
			if (g_deletedovlloaded)
			{
				if (m_State == FileStateDeleted)
				{
					g_filepath.clear();
					return S_OK;
				}
				else
					return S_FALSE;
			}
			else
			{
				// the 'deleted' overlay isn't available (due to lack of enough
				// overlay slots). So just show the 'modified' overlay instead.
				if (m_State == FileStateModified)
				{
					g_filepath.clear();
					return S_OK;
				}
				else
					return S_FALSE;
			}
		case git_wc_status_modified:
			if (g_modifiedovlloaded)
			{
				if (m_State == FileStateModified)
				{
					g_filepath.clear();
					return S_OK;
				}
				else
					return S_FALSE;
			}
			else
			{
				if (m_State == FileStateVersioned)
				{
					g_filepath.clear();
					return S_OK;
				}
				else
					return S_FALSE;
			}
		case git_wc_status_added:
			if (g_addedovlloaded)
			{
				if (m_State== FileStateAddedOverlay)
				{
					g_filepath.clear();
					return S_OK;
				}
				else
					return S_FALSE;
			}
			else
			{
				// the 'added' overlay isn't available (due to lack of enough
				// overlay slots). So just show the 'modified' overlay instead.
				if (m_State == FileStateModified)
				{
					g_filepath.clear();
					return S_OK;
				}
				else
					return S_FALSE;
			}
		case git_wc_status_conflicted:
			if (g_conflictedovlloaded)
			{
				if (m_State == FileStateConflict)
				{
					g_filepath.clear();
					return S_OK;
				}
				else
					return S_FALSE;
			}
			else
			{
				// the 'conflicted' overlay isn't available (due to lack of enough
				// overlay slots). So just show the 'modified' overlay instead.
				if (m_State == FileStateModified)
				{
					g_filepath.clear();
					return S_OK;
				}
				else
					return S_FALSE;
			}
		default:
			return S_FALSE;
	} // switch (status)
	//return S_FALSE;
}

