// TortoiseSI - a Windows shell extension for easy version control

// Copyright (C) 2015 TortoiseSI
// Copyright (C) 2009-2013 - TortoiseGit
// Copyright (C) 2003-2008 - TortoiseSVN

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
#include "StatusCache.h"
#include "EventLog.h"

// "The Shell calls IShellIconOverlayIdentifier::GetOverlayInfo to request the
//  location of the handler's icon overlay. The icon overlay handler returns
//  the name of the file containing the overlay image, and its index within
//  that file. The Shell then adds the icon overlay to the system image list."

STDMETHODIMP CShellExt::GetOverlayInfo(LPWSTR pwszIconFile, int cchMax, int* pIndex, DWORD* pdwFlags)
{
	__try
	{
		return GetOverlayInfo_Wrap(pwszIconFile, cchMax, pIndex, pdwFlags);
	}
	__except (HandleException(GetExceptionInformation()))
	{
	}
	return E_FAIL;
}

STDMETHODIMP CShellExt::GetOverlayInfo_Wrap(LPWSTR pwszIconFile, int cchMax, int* pIndex, DWORD* pdwFlags)
{
	if(pwszIconFile == 0)
		return E_POINTER;
	if(pIndex == 0)
		return E_POINTER;
	if(pdwFlags == 0)
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
	__try
	{
		return GetPriority_Wrap(pPriority);
	}
	__except (HandleException(GetExceptionInformation()))
	{
	}
	return E_FAIL;
}

STDMETHODIMP CShellExt::GetPriority_Wrap(int *pPriority)
{
	if (pPriority == 0)
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

STDMETHODIMP CShellExt::IsMemberOf(LPCWSTR pwszPath, DWORD dwAttrib)
{
	__try
	{
		return IsMemberOf_Wrap(pwszPath, dwAttrib);
	}
	__except (HandleException(GetExceptionInformation()))
	{
	}
	return E_FAIL;
}

STDMETHODIMP CShellExt::IsMemberOf_Wrap(LPCWSTR pwszPath, DWORD /*dwAttrib*/)
{
	if (pwszPath == NULL)
		return E_INVALIDARG;
     std::wstring path = pwszPath;
	// the shell sometimes asks overlays for invalid paths, e.g. for network
	// printers (in that case the path is "0", at least for me here).
	if (path.length() < 2) {
		return S_FALSE;
	}

	FileStatusFlags fileStatus = getPathStatus(path);

	HRESULT result = doesStatusMatch(fileStatus);
	if (result == S_OK) {
		IStatusCache::getInstance().clear(path);
		EventLog::writeDebug(std::wstring(L"IShellIconOverlayIdentifier::IsMemberOf ") + path + L" has icon " +
			to_wstring(m_State));
	}
	return result;
}

FileStatusFlags	CShellExt::getPathStatus(std::wstring path)
{
	if (IsPathAllowed(path) ){
		if (IStatusCache::getInstance().getRootFolderCache().isPathControlled(path)) {
			if (PathIsDirectoryW(path.c_str())) {
				return FileStatus::Folder | FileStatus::Member;
			} else {
				return IStatusCache::getInstance().getFileStatus(path) | FileStatus::File;
			}
		} else {
			if (PathIsDirectoryW(path.c_str())) {
				return (FileStatusFlags)FileStatus::Folder;
			} else {
				return (FileStatusFlags)FileStatus::File;
			}
		}
	}
	return (FileStatusFlags)FileStatus::None;
}

//the priority system of the shell doesn't seem to work as expected (or as I expected):
//as it seems that if one handler returns S_OK then that handler is used, no matter
//if other handlers would return S_OK too (they're never called on my machine!)
//So we return S_OK for ONLY ONE handler!
 HRESULT CShellExt::doesStatusMatch(FileStatusFlags fileStatusFlags) 
 {
		// note: we can show other overlays if due to lack of enough free overlay
		// slots some of our overlays aren't loaded. But we assume that
		// at least the 'normal' and 'modified' overlay are available.

	if (fileStatusFlags == 0) {
		 return S_FALSE;	
	} else if (hasFileStatus(fileStatusFlags, FileStatus::FormerMember)) {
		 if (g_deletedovlloaded && m_State == FileStateDeleted) {
			 return S_OK;
		 } else if (m_State == FileStateModified) {
			 // the 'deleted' overlay isn't available (due to lack of enough
			 // overlay slots). So just show the 'modified' overlay instead.
			 return S_OK;
		 } else {
			return S_FALSE;
		 }
	} else if (hasFileStatus(fileStatusFlags, FileStatus::Incoming)) {
		 if (g_conflictedovlloaded && m_State == FileStateConflict) {
			 return S_OK;
		 } else if (m_State == FileStateModified) {
			 // the 'conflicted' overlay isn't available (due to lack of enough
			 // overlay slots). So just show the 'modified' overlay instead.
			 return S_OK;
		 } else {
			return S_FALSE;
		 }
	} else if (hasFileStatus(fileStatusFlags, FileStatus::Modified) 
		|| hasFileStatus(fileStatusFlags, FileStatus::Moved)
		|| hasFileStatus(fileStatusFlags, FileStatus::Renamed)) {

		if (m_State == FileStateModified) {
			return S_OK;
		} else {
			return S_FALSE;
		}
	 } else if (hasFileStatus(fileStatusFlags, FileStatus::Add)) {
		 if (g_addedovlloaded && m_State == FileStateAddedOverlay){
			 return S_OK;
		 } else	if (m_State == FileStateModified) {
			// the 'added' overlay isn't available (due to lack of enough
			// overlay slots). So just show the 'modified' overlay instead.
			return S_OK;
		 } else {
			return S_FALSE;
		}
	} else if (hasFileStatus(fileStatusFlags, FileStatus::Locked)) {
		 if (g_lockedovlloaded && m_State == FileStateLockedOverlay) {
			 return S_OK;
		 } else if (m_State == FileStateVersioned) {
			 return S_OK;
		 } else {
			 return S_FALSE;
		 }
	 } else if (hasFileStatus(fileStatusFlags, FileStatus::Member)) {
		if (m_State == FileStateVersioned) {
			 return S_OK;
		 } else {
			 return S_FALSE;
		 }
	 }
	 return S_FALSE;
 }
