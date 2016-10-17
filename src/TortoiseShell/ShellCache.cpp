// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2012-2016 - TortoiseGit
// Copyright (C) 2003-2008 - Stefan Kueng

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
#include "ShellCache.h"
#include "GitAdminDir.h"

ShellCache::ShellCache()
{
	cachetype = CRegStdDWORD(L"Software\\TortoiseGit\\CacheType", GetSystemMetrics(SM_REMOTESESSION) ? dll : exe, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
	showrecursive = CRegStdDWORD(L"Software\\TortoiseGit\\RecursiveOverlay", TRUE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
	folderoverlay = CRegStdDWORD(L"Software\\TortoiseGit\\FolderOverlay", TRUE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
	driveremote = CRegStdDWORD(L"Software\\TortoiseGit\\DriveMaskRemote", FALSE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
	drivefixed = CRegStdDWORD(L"Software\\TortoiseGit\\DriveMaskFixed", TRUE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
	drivecdrom = CRegStdDWORD(L"Software\\TortoiseGit\\DriveMaskCDROM", FALSE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
	driveremove = CRegStdDWORD(L"Software\\TortoiseGit\\DriveMaskRemovable", FALSE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
	drivefloppy = CRegStdDWORD(L"Software\\TortoiseGit\\DriveMaskFloppy", FALSE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
	driveram = CRegStdDWORD(L"Software\\TortoiseGit\\DriveMaskRAM", FALSE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
	driveunknown = CRegStdDWORD(L"Software\\TortoiseGit\\DriveMaskUnknown", FALSE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
	shellmenuaccelerators = CRegStdDWORD(L"Software\\TortoiseGit\\ShellMenuAccelerators", TRUE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
	excludelist = CRegStdString(L"Software\\TortoiseGit\\OverlayExcludeList", L"", false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
	includelist = CRegStdString(L"Software\\TortoiseGit\\OverlayIncludeList", L"", false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
	simplecontext = CRegStdDWORD(L"Software\\TortoiseGit\\SimpleContext", FALSE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
	unversionedasmodified = CRegStdDWORD(L"Software\\TortoiseGit\\UnversionedAsModified", FALSE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
	recursesubmodules = CRegStdDWORD(L"Software\\TortoiseGit\\TGitCacheRecurseSubmodules", FALSE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
	hidemenusforunversioneditems = CRegStdDWORD(L"Software\\TortoiseGit\\HideMenusForUnversionedItems", FALSE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
	showunversionedoverlay = CRegStdDWORD(L"Software\\TortoiseGit\\ShowUnversionedOverlay", TRUE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
	showignoredoverlay = CRegStdDWORD(L"Software\\TortoiseGit\\ShowIgnoredOverlay", TRUE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
	getlocktop = CRegStdDWORD(L"Software\\TortoiseGit\\GetLockTop", TRUE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
	excludedasnormal = CRegStdDWORD(L"Software\\TortoiseGit\\ShowExcludedAsNormal", TRUE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
	cachetypeticker = GetTickCount64();
	recursiveticker = cachetypeticker;
	folderoverlayticker = cachetypeticker;
	driveticker = cachetypeticker;
	drivetypeticker = cachetypeticker;
	langticker = cachetypeticker;
	excludelistticker = cachetypeticker;
	includelistticker = cachetypeticker;
	simplecontextticker = cachetypeticker;
	shellmenuacceleratorsticker = cachetypeticker;
	unversionedasmodifiedticker = cachetypeticker;
	recursesubmodulesticker = cachetypeticker;
	showunversionedoverlayticker = cachetypeticker;
	showignoredoverlayticker = cachetypeticker;
	getlocktopticker = cachetypeticker;
	excludedasnormalticker = cachetypeticker;
	hidemenusforunversioneditemsticker = cachetypeticker;
	excontextticker = cachetypeticker;

	unsigned __int64 entries = (DEFAULTMENUTOPENTRIES);
	menulayoutlow = CRegStdDWORD(L"Software\\TortoiseGit\\ContextMenuEntries", entries & 0xFFFFFFFF, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
	menulayouthigh = CRegStdDWORD(L"Software\\TortoiseGit\\ContextMenuEntrieshigh", entries >> 32, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
	layoutticker = cachetypeticker;

	unsigned __int64 ext = (DEFAULTMENUEXTENTRIES);
	menuextlow = CRegStdDWORD(L"Software\\TortoiseGit\\ContextMenuExtEntriesLow", ext & 0xFFFFFFFF, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
	menuexthigh = CRegStdDWORD(L"Software\\TortoiseGit\\ContextMenuExtEntriesHigh", ext >> 32, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
	exticker = cachetypeticker;

	menumasklow_lm = CRegStdDWORD(L"Software\\TortoiseGit\\ContextMenuEntriesMaskLow", 0, FALSE, HKEY_LOCAL_MACHINE, KEY_WOW64_64KEY);
	menumaskhigh_lm = CRegStdDWORD(L"Software\\TortoiseGit\\ContextMenuEntriesMaskHigh", 0, FALSE, HKEY_LOCAL_MACHINE, KEY_WOW64_64KEY);
	menumasklow_cu = CRegStdDWORD(L"Software\\TortoiseGit\\ContextMenuEntriesMaskLow", 0, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
	menumaskhigh_cu = CRegStdDWORD(L"Software\\TortoiseGit\\ContextMenuEntriesMaskHigh", 0, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
	menumaskticker = cachetypeticker;
	langid = CRegStdDWORD(L"Software\\TortoiseGit\\LanguageID", 1033, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
	blockstatus = CRegStdDWORD(L"Software\\TortoiseGit\\BlockStatus", 0, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
	blockstatusticker = cachetypeticker;
	std::fill_n(drivetypecache, 27, (UINT)-1);
	if (DWORD(drivefloppy) == 0)
	{
		// A: and B: are floppy disks
		drivetypecache[0] = DRIVE_REMOVABLE;
		drivetypecache[1] = DRIVE_REMOVABLE;
	}
	drivetypepathcache[0] = L'\0';
	nocontextpaths = CRegStdString(L"Software\\TortoiseGit\\NoContextPaths", L"", false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
	m_critSec.Init();
}

void ShellCache::ForceRefresh()
{
	cachetype.read();
	showrecursive.read();
	folderoverlay.read();
	driveremote.read();
	drivefixed.read();
	drivecdrom.read();
	driveremove.read();
	drivefloppy.read();
	driveram.read();
	driveunknown.read();
	excludelist.read();
	includelist.read();
	simplecontext.read();
	shellmenuaccelerators.read();
	unversionedasmodified.read();
	recursesubmodules.read();
	showunversionedoverlay.read();
	showignoredoverlay.read();
	excludedasnormal.read();
	hidemenusforunversioneditems.read();
	menulayoutlow.read();
	menulayouthigh.read();
	langid.read();
	blockstatus.read();
	getlocktop.read();
	menumasklow_lm.read();
	menumaskhigh_lm.read();
	menumasklow_cu.read();
	menumaskhigh_cu.read();
	nocontextpaths.read();
}

ShellCache::CacheType ShellCache::GetCacheType()
{
	if ((GetTickCount64() - cachetypeticker) > REGISTRYTIMEOUT)
	{
		cachetypeticker = GetTickCount64();
		cachetype.read();
	}
	return CacheType(DWORD((cachetype)));
}

DWORD ShellCache::BlockStatus()
{
	if ((GetTickCount64() - blockstatusticker) > REGISTRYTIMEOUT)
	{
		blockstatusticker = GetTickCount64();
		blockstatus.read();
	}
	return (blockstatus);
}

unsigned __int64 ShellCache::GetMenuLayout()
{
	if ((GetTickCount64() - layoutticker) > REGISTRYTIMEOUT)
	{
		layoutticker = GetTickCount64();
		menulayoutlow.read();
		menulayouthigh.read();
	}
	unsigned __int64 temp = unsigned __int64(DWORD(menulayouthigh)) << 32;
	temp |= unsigned __int64(DWORD(menulayoutlow));
	return temp;
}

unsigned __int64 ShellCache::GetMenuExt()
{
	if ((GetTickCount64() - exticker) > REGISTRYTIMEOUT)
	{
		exticker = GetTickCount64();
		menuextlow.read();
		menuexthigh.read();
	}
	unsigned __int64 temp = unsigned __int64(DWORD(menuexthigh)) << 32;
	temp |= unsigned __int64(DWORD(menuextlow));
	return temp;
}

unsigned __int64 ShellCache::GetMenuMask()
{
	if ((GetTickCount64() - menumaskticker) > REGISTRYTIMEOUT)
	{
		menumaskticker = GetTickCount64();
		menumasklow_lm.read();
		menumaskhigh_lm.read();
		menumasklow_cu.read();
		menumaskhigh_cu.read();
	}
	DWORD low = (DWORD)menumasklow_lm | (DWORD)menumasklow_cu;
	DWORD high = (DWORD)menumaskhigh_lm | (DWORD)menumaskhigh_cu;
	unsigned __int64 temp = unsigned __int64(high) << 32;
	temp |= unsigned __int64(low);
	return temp;
}

BOOL ShellCache::IsRecursive()
{
	if ((GetTickCount64() - recursiveticker) > REGISTRYTIMEOUT)
	{
		recursiveticker = GetTickCount64();
		showrecursive.read();
	}
	return (showrecursive);
}

BOOL ShellCache::IsFolderOverlay()
{
	if ((GetTickCount64() - folderoverlayticker) > REGISTRYTIMEOUT)
	{
		folderoverlayticker = GetTickCount64();
		folderoverlay.read();
	}
	return (folderoverlay);
}

BOOL ShellCache::IsSimpleContext()
{
	if ((GetTickCount64() - simplecontextticker) > REGISTRYTIMEOUT)
	{
		simplecontextticker = GetTickCount64();
		simplecontext.read();
	}
	return (simplecontext != 0);
}

BOOL ShellCache::HasShellMenuAccelerators()
{
	if ((GetTickCount64() - shellmenuacceleratorsticker) > REGISTRYTIMEOUT)
	{
		shellmenuacceleratorsticker = GetTickCount64();
		shellmenuaccelerators.read();
	}
	return (shellmenuaccelerators != 0);
}

BOOL ShellCache::IsUnversionedAsModified()
{
	if ((GetTickCount64() - unversionedasmodifiedticker) > REGISTRYTIMEOUT)
	{
		unversionedasmodifiedticker = GetTickCount64();
		unversionedasmodified.read();
	}
	return (unversionedasmodified);
}

BOOL ShellCache::IsRecurseSubmodules()
{
	if ((GetTickCount64() - recursesubmodulesticker) > REGISTRYTIMEOUT)
	{
		recursesubmodulesticker = GetTickCount64();
		recursesubmodules.read();
	}
	return (recursesubmodules);
}

BOOL ShellCache::ShowUnversionedOverlay()
{
	if ((GetTickCount64() - showunversionedoverlayticker) > REGISTRYTIMEOUT)
	{
		showunversionedoverlayticker = GetTickCount64();
		showunversionedoverlay.read();
	}
	return (showunversionedoverlay);
}

BOOL ShellCache::ShowIgnoredOverlay()
{
	if ((GetTickCount64() - showignoredoverlayticker) > REGISTRYTIMEOUT)
	{
		showignoredoverlayticker = GetTickCount64();
		showignoredoverlay.read();
	}
	return (showignoredoverlay);
}

BOOL ShellCache::IsGetLockTop()
{
	if ((GetTickCount64() - getlocktopticker) > REGISTRYTIMEOUT)
	{
		getlocktopticker = GetTickCount64();
		getlocktop.read();
	}
	return (getlocktop);
}

BOOL ShellCache::ShowExcludedAsNormal()
{
	if ((GetTickCount64() - excludedasnormalticker) > REGISTRYTIMEOUT)
	{
		excludedasnormalticker = GetTickCount64();
		excludedasnormal.read();
	}
	return (excludedasnormal);
}

BOOL ShellCache::HideMenusForUnversionedItems()
{
	if ((GetTickCount64() - hidemenusforunversioneditemsticker) > REGISTRYTIMEOUT)
	{
		hidemenusforunversioneditemsticker = GetTickCount64();
		hidemenusforunversioneditems.read();
	}
	return (hidemenusforunversioneditems);
}

BOOL ShellCache::IsRemote()
{
	DriveValid();
	return (driveremote);
}

BOOL ShellCache::IsFixed()
{
	DriveValid();
	return (drivefixed);
}

BOOL ShellCache::IsCDRom()
{
	DriveValid();
	return (drivecdrom);
}

BOOL ShellCache::IsRemovable()
{
	DriveValid();
	return (driveremove);
}

BOOL ShellCache::IsRAM()
{
	DriveValid();
	return (driveram);
}

BOOL ShellCache::IsUnknown()
{
	DriveValid();
	return (driveunknown);
}

BOOL ShellCache::IsContextPathAllowed(LPCTSTR path)
{
	Locker lock(m_critSec);
	ExcludeContextValid();
	for (const auto& exPath : excontextvector)
	{
		if (exPath.empty())
			continue;
		if (exPath.at(exPath.size() - 1) == '*')
		{
			tstring str = exPath.substr(0, exPath.size() - 1);
			if (_tcsnicmp(str.c_str(), path, str.size()) == 0)
				return FALSE;
		}
		else if (_tcsicmp(exPath.c_str(), path) == 0)
			return FALSE;
	}
	return TRUE;
}

BOOL ShellCache::IsPathAllowed(LPCTSTR path)
{
	Locker lock(m_critSec);
	IncludeListValid();
	for (const auto& pathAllowed : invector)
	{
		if (pathAllowed.empty())
			continue;
		if (pathAllowed.at(pathAllowed.size() - 1) == '*')
		{
			tstring str = pathAllowed.substr(0, pathAllowed.size() - 1);
			if (_tcsnicmp(str.c_str(), path, str.size()) == 0)
				return TRUE;
			if (!str.empty() && (str.at(str.size() - 1) == '\\') && (_tcsnicmp(str.c_str(), path, str.size() - 1) == 0))
				return TRUE;
		}
		else if (_tcsicmp(pathAllowed.c_str(), path) == 0)
			return TRUE;
		else if ((pathAllowed.at(pathAllowed.size() - 1) == '\\') &&
			((_tcsnicmp(pathAllowed.c_str(), path, pathAllowed.size()) == 0) || (_tcsicmp(pathAllowed.c_str(), path) == 0)))
			return TRUE;

	}
	UINT drivetype = 0;
	int drivenumber = PathGetDriveNumber(path);
	if ((drivenumber >= 0) && (drivenumber < 25))
	{
		drivetype = drivetypecache[drivenumber];
		if ((drivetype == -1) || ((GetTickCount64() - drivetypeticker) > DRIVETYPETIMEOUT))
		{
			if ((DWORD(drivefloppy) == 0) && ((drivenumber == 0) || (drivenumber == 1)))
				drivetypecache[drivenumber] = DRIVE_REMOVABLE;
			else
			{
				drivetypeticker = GetTickCount64();
				TCHAR pathbuf[MAX_PATH + 4] = { 0 };		// MAX_PATH ok here. PathStripToRoot works with partial paths too.
				_tcsncpy_s(pathbuf, MAX_PATH + 4, path, MAX_PATH + 3);
				PathStripToRoot(pathbuf);
				PathAddBackslash(pathbuf);
				CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": GetDriveType for %s, Drive %d\n", pathbuf, drivenumber);
				drivetype = GetDriveType(pathbuf);
				drivetypecache[drivenumber] = drivetype;
			}
		}
	}
	else
	{
		TCHAR pathbuf[MAX_PATH + 4] = { 0 };		// MAX_PATH ok here. PathIsUNCServer works with partial paths too.
		_tcsncpy_s(pathbuf, MAX_PATH + 4, path, MAX_PATH + 3);
		if (PathIsUNCServer(pathbuf))
			drivetype = DRIVE_REMOTE;
		else
		{
			PathStripToRoot(pathbuf);
			PathAddBackslash(pathbuf);
			if (_tcsncmp(pathbuf, drivetypepathcache, MAX_PATH - 1) == 0)		// MAX_PATH ok.
				drivetype = drivetypecache[26];
			else
			{
				CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L"GetDriveType for %s\n", pathbuf);
				drivetype = GetDriveType(pathbuf);
				drivetypecache[26] = drivetype;
				_tcsncpy_s(drivetypepathcache, MAX_PATH, pathbuf, MAX_PATH - 1);			// MAX_PATH ok.
			}
		}
	}
	if ((drivetype == DRIVE_REMOVABLE) && (!IsRemovable()))
		return FALSE;
	if ((drivetype == DRIVE_FIXED) && (!IsFixed()))
		return FALSE;
	if (((drivetype == DRIVE_REMOTE) || (drivetype == DRIVE_NO_ROOT_DIR)) && (!IsRemote()))
		return FALSE;
	if ((drivetype == DRIVE_CDROM) && (!IsCDRom()))
		return FALSE;
	if ((drivetype == DRIVE_RAMDISK) && (!IsRAM()))
		return FALSE;
	if ((drivetype == DRIVE_UNKNOWN) && (IsUnknown()))
		return FALSE;

	ExcludeListValid();
	for (const auto& exPath : exvector)
	{
		if (exPath.empty())
			continue;
		if (exPath.at(exPath.size() - 1) == '*')
		{
			tstring str = exPath.substr(0, exPath.size() - 1);
			if (_tcsnicmp(str.c_str(), path, str.size()) == 0)
				return FALSE;
		}
		else if (_tcsicmp(exPath.c_str(), path) == 0)
			return FALSE;
	}
	return TRUE;
}

DWORD ShellCache::GetLangID()
{
	if ((GetTickCount64() - langticker) > REGISTRYTIMEOUT)
	{
		langticker = GetTickCount64();
		langid.read();
	}
	return (langid);
}

BOOL ShellCache::HasGITAdminDir(LPCTSTR path, BOOL bIsDir, CString* ProjectTopDir /*= nullptr*/)
{
	tstring folder(path);
	if (!bIsDir)
	{
		size_t pos = folder.rfind(_T('\\'));
		if (pos != tstring::npos)
			folder.erase(pos);
	}
	std::map<tstring, AdminDir_s>::const_iterator iter;
	if ((iter = admindircache.find(folder)) != admindircache.cend())
	{
		Locker lock(m_critSec);
		if ((GetTickCount64() - iter->second.timeout) < ADMINDIRTIMEOUT)
		{
			if (ProjectTopDir && iter->second.bHasAdminDir)
				*ProjectTopDir = iter->second.sProjectRoot.c_str();
			return iter->second.bHasAdminDir;
		}
	}

	CString sProjectRoot;
	BOOL hasAdminDir = GitAdminDir::HasAdminDir(folder.c_str(), true, &sProjectRoot);

	Locker lock(m_critSec);
	AdminDir_s& ad = admindircache[folder];
	ad.bHasAdminDir = hasAdminDir;
	ad.timeout = GetTickCount64();
	if (hasAdminDir)
	{
		ad.sProjectRoot.assign(sProjectRoot);
		if (ProjectTopDir)
			*ProjectTopDir = sProjectRoot;
	}

	return hasAdminDir;
}

void ShellCache::DriveValid()
{
	if ((GetTickCount64() - driveticker) > REGISTRYTIMEOUT)
	{
		driveticker = GetTickCount64();
		driveremote.read();
		drivefixed.read();
		drivecdrom.read();
		driveremove.read();
		drivefloppy.read();
	}
}

void ShellCache::ExcludeContextValid()
{
	if ((GetTickCount64() - excontextticker) > EXCLUDELISTTIMEOUT)
	{
		Locker lock(m_critSec);
		excontextticker = GetTickCount64();
		nocontextpaths.read();
		if (excludecontextstr.compare((tstring)nocontextpaths) == 0)
			return;
		excludecontextstr = (tstring)nocontextpaths;
		excontextvector.clear();
		size_t pos = 0, pos_ant = 0;
		pos = excludecontextstr.find(L"\n", pos_ant);
		while (pos != tstring::npos)
		{
			tstring token = excludecontextstr.substr(pos_ant, pos - pos_ant);
			excontextvector.push_back(token);
			pos_ant = pos + 1;
			pos = excludecontextstr.find(L"\n", pos_ant);
		}
		if (!excludecontextstr.empty())
			excontextvector.push_back(excludecontextstr.substr(pos_ant, excludecontextstr.size() - 1));
		excludecontextstr = (tstring)nocontextpaths;
	}
}

void ShellCache::ExcludeListValid()
{
	if ((GetTickCount64() - excludelistticker) > EXCLUDELISTTIMEOUT)
	{
		Locker lock(m_critSec);
		excludelistticker = GetTickCount64();
		excludelist.read();
		if (excludeliststr.compare((tstring)excludelist) == 0)
			return;
		excludeliststr = (tstring)excludelist;
		exvector.clear();
		size_t pos = 0, pos_ant = 0;
		pos = excludeliststr.find(L"\n", pos_ant);
		while (pos != tstring::npos)
		{
			tstring token = excludeliststr.substr(pos_ant, pos - pos_ant);
			exvector.push_back(token);
			pos_ant = pos + 1;
			pos = excludeliststr.find(L"\n", pos_ant);
		}
		if (!excludeliststr.empty())
			exvector.push_back(excludeliststr.substr(pos_ant, excludeliststr.size() - 1));
		excludeliststr = (tstring)excludelist;
	}
}

void ShellCache::IncludeListValid()
{
	if ((GetTickCount64() - includelistticker) > EXCLUDELISTTIMEOUT)
	{
		Locker lock(m_critSec);
		includelistticker = GetTickCount64();
		includelist.read();
		if (includeliststr.compare((tstring)includelist) == 0)
			return;
		includeliststr = (tstring)includelist;
		invector.clear();
		size_t pos = 0, pos_ant = 0;
		pos = includeliststr.find(L"\n", pos_ant);
		while (pos != tstring::npos)
		{
			tstring token = includeliststr.substr(pos_ant, pos - pos_ant);
			invector.push_back(token);
			pos_ant = pos + 1;
			pos = includeliststr.find(L"\n", pos_ant);
		}
		if (!includeliststr.empty())
			invector.push_back(includeliststr.substr(pos_ant, includeliststr.size() - 1));
		includeliststr = (tstring)includelist;
	}
}
