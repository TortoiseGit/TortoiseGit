// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2012-2023 - TortoiseGit
// Copyright (C) 2003-2017 - TortoiseSVN

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
	onlynonelevated = CRegStdDWORD(L"Software\\TortoiseGit\\ShowOverlaysOnlyNonElevated", FALSE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
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
	simplecontext = CRegStdDWORD(L"Software\\TortoiseGit\\SimpleContext", FALSE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
	unversionedasmodified = CRegStdDWORD(L"Software\\TortoiseGit\\UnversionedAsModified", FALSE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
	recursesubmodules = CRegStdDWORD(L"Software\\TortoiseGit\\TGitCacheRecurseSubmodules", FALSE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
	hidemenusforunversioneditems = CRegStdDWORD(L"Software\\TortoiseGit\\HideMenusForUnversionedItems", FALSE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
	showunversionedoverlay = CRegStdDWORD(L"Software\\TortoiseGit\\ShowUnversionedOverlay", TRUE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
	showignoredoverlay = CRegStdDWORD(L"Software\\TortoiseGit\\ShowIgnoredOverlay", TRUE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
	excludedasnormal = CRegStdDWORD(L"Software\\TortoiseGit\\ShowExcludedAsNormal", TRUE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
	drivetypeticker = 0;

	menuLayout11 = CRegStdQWORD(L"Software\\TortoiseGit\\ContextMenu11Entries", DEFAULTWIN11MENUTOPENTRIES, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);

	unsigned __int64 entries = (DEFAULTMENUTOPENTRIES);
	menulayoutlow = CRegStdDWORD(L"Software\\TortoiseGit\\ContextMenuEntries", entries & 0xFFFFFFFF, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
	menulayouthigh = CRegStdDWORD(L"Software\\TortoiseGit\\ContextMenuEntrieshigh", entries >> 32, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);

	unsigned __int64 ext = (DEFAULTMENUEXTENTRIES);
	menuextlow = CRegStdDWORD(L"Software\\TortoiseGit\\ContextMenuExtEntriesLow", ext & 0xFFFFFFFF, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
	menuexthigh = CRegStdDWORD(L"Software\\TortoiseGit\\ContextMenuExtEntriesHigh", ext >> 32, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);

	menumasklow_lm = CRegStdDWORD(L"Software\\TortoiseGit\\ContextMenuEntriesMaskLow", 0, FALSE, HKEY_LOCAL_MACHINE, KEY_WOW64_64KEY);
	menumaskhigh_lm = CRegStdDWORD(L"Software\\TortoiseGit\\ContextMenuEntriesMaskHigh", 0, FALSE, HKEY_LOCAL_MACHINE, KEY_WOW64_64KEY);
	menumasklow_cu = CRegStdDWORD(L"Software\\TortoiseGit\\ContextMenuEntriesMaskLow", 0, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
	menumaskhigh_cu = CRegStdDWORD(L"Software\\TortoiseGit\\ContextMenuEntriesMaskHigh", 0, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
	menumaskticker = 0;
	langid = CRegStdDWORD(L"Software\\TortoiseGit\\LanguageID", 1033, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
	blockstatus = CRegStdDWORD(L"Software\\TortoiseGit\\BlockStatus", 0, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
	std::fill_n(drivetypecache, 27, UINT(-1));
	if (DWORD(drivefloppy) == 0)
	{
		// A: and B: are floppy disks
		drivetypecache[0] = DRIVE_REMOVABLE;
		drivetypecache[1] = DRIVE_REMOVABLE;
	}
	drivetypepathcache[0] = L'\0';
	nocontextpaths = CRegStdString(L"Software\\TortoiseGit\\NoContextPaths", L"", false, HKEY_CURRENT_USER, KEY_WOW64_64KEY);
	// Use RegNotifyChangeKeyValue() to get a notification event whenever a registry value
	// below HKCU\Software\TortoiseGit is changed. If a value has changed, re-read all
	// the registry variables to ensure we use the latest ones
	RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\TortoiseGit", 0, KEY_NOTIFY | KEY_WOW64_64KEY, &m_hNotifyRegKey);
	m_registryChangeEvent = CreateEvent(nullptr, true, false, nullptr);
	if (RegNotifyChangeKeyValue(m_hNotifyRegKey, false, REG_NOTIFY_CHANGE_LAST_SET, m_registryChangeEvent, TRUE) != ERROR_SUCCESS)
	{
		if (m_registryChangeEvent)
			CloseHandle(m_registryChangeEvent);
		m_registryChangeEvent = nullptr;
		RegCloseKey(m_hNotifyRegKey);
		m_hNotifyRegKey = nullptr;
	}

	// find out if we're elevated
	isElevated = false;
	HANDLE hToken = nullptr;
	if (::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &hToken))
	{
		TOKEN_ELEVATION te = { 0 };
		DWORD dwReturnLength = 0;
		if (::GetTokenInformation(hToken, TokenElevation, &te, sizeof(te), &dwReturnLength))
			isElevated = (te.TokenIsElevated != 0);
		::CloseHandle(hToken);
	}
}

ShellCache::~ShellCache()
{
	if (m_registryChangeEvent)
		CloseHandle(m_registryChangeEvent);
	m_registryChangeEvent = nullptr;
	if (m_hNotifyRegKey)
		RegCloseKey(m_hNotifyRegKey);
	m_hNotifyRegKey = nullptr;
}

bool ShellCache::RefreshIfNeeded()
{
	// don't wait for the registry change event but only test if such an event
	// has occurred since the last time we got here.
	// if the event has occurred, re-read all registry variables and of course
	// re-set the notification event to get further notifications of registry changes.
	bool signalled = WaitForSingleObjectEx(m_registryChangeEvent, 0, true) != WAIT_TIMEOUT;
	if (!signalled)
		return signalled;

	if (RegNotifyChangeKeyValue(m_hNotifyRegKey, false, REG_NOTIFY_CHANGE_LAST_SET, m_registryChangeEvent, TRUE) != ERROR_SUCCESS)
	{
		CloseHandle(m_registryChangeEvent);
		m_registryChangeEvent = nullptr;
		RegCloseKey(m_hNotifyRegKey);
		m_hNotifyRegKey = nullptr;
	}

	cachetype.read();
	onlynonelevated.read();
	showrecursive.read();
	folderoverlay.read();
	driveremote.read();
	drivefixed.read();
	drivecdrom.read();
	driveremove.read();
	drivefloppy.read();
	driveram.read();
	driveunknown.read();
	simplecontext.read();
	shellmenuaccelerators.read();
	unversionedasmodified.read();
	recursesubmodules.read();
	showunversionedoverlay.read();
	showignoredoverlay.read();
	excludedasnormal.read();
	hidemenusforunversioneditems.read();
	menuLayout11.read();
	menulayoutlow.read();
	menulayouthigh.read();
	langid.read();
	blockstatus.read();
	menumasklow_lm.read();
	menumaskhigh_lm.read();
	menumasklow_cu.read();
	menumaskhigh_cu.read();
	menuextlow.read();
	menuexthigh.read();
	nocontextpaths.read();

	if (DWORD(drivefloppy) == 0)
	{
		// A: and B: are floppy disks
		drivetypecache[0] = DRIVE_REMOVABLE;
		drivetypecache[1] = DRIVE_REMOVABLE;
	}
	else
	{
		// reset floppy drive cache
		drivetypecache[0] = UINT(-1);
		drivetypecache[1] = UINT(-1);
	}

	Locker lock(m_critSec);
	pathFilter.Refresh();

	return signalled;
}

ShellCache::CacheType ShellCache::GetCacheType()
{
	RefreshIfNeeded();
	return CacheType(static_cast<DWORD>(cachetype));
}

DWORD ShellCache::BlockStatus()
{
	RefreshIfNeeded();
	return (blockstatus);
}

unsigned __int64 ShellCache::GetMenuLayout11()
{
	RefreshIfNeeded();
	return menuLayout11;
}

unsigned __int64 ShellCache::GetMenuLayout()
{
	RefreshIfNeeded();
	ULARGE_INTEGER temp;
	temp.HighPart = menulayouthigh;
	temp.LowPart = menulayoutlow;
	return temp.QuadPart;
}

unsigned __int64 ShellCache::GetMenuExt()
{
	RefreshIfNeeded();
	ULARGE_INTEGER temp;
	temp.HighPart = menuexthigh;
	temp.LowPart = menuextlow;
	return temp.QuadPart;
}

unsigned __int64 ShellCache::GetMenuMask()
{
	auto ticks = GetTickCount64();
	if ((ticks - menumaskticker) > ADMINDIRTIMEOUT)
	{
		menumaskticker = ticks;
		menumasklow_lm.read();
		menumaskhigh_lm.read();
	}

	ULARGE_INTEGER temp;
	temp.LowPart = menumasklow_lm | menumasklow_cu;
	temp.HighPart = menumaskhigh_lm | menumaskhigh_cu;
	return temp.QuadPart;
}

bool ShellCache::IsProcessElevated()
{
	return isElevated;
}

BOOL ShellCache::IsOnlyNonElevated()
{
	RefreshIfNeeded();
	return (onlynonelevated);
}

BOOL ShellCache::IsRecursive()
{
	RefreshIfNeeded();
	return (showrecursive);
}

BOOL ShellCache::IsFolderOverlay()
{
	RefreshIfNeeded();
	return (folderoverlay);
}

BOOL ShellCache::IsSimpleContext()
{
	RefreshIfNeeded();
	return (simplecontext != 0);
}

BOOL ShellCache::HasShellMenuAccelerators()
{
	RefreshIfNeeded();
	return (shellmenuaccelerators != 0);
}

BOOL ShellCache::IsUnversionedAsModified()
{
	RefreshIfNeeded();
	return (unversionedasmodified);
}

BOOL ShellCache::IsRecurseSubmodules()
{
	RefreshIfNeeded();
	return (recursesubmodules);
}

BOOL ShellCache::ShowUnversionedOverlay()
{
	RefreshIfNeeded();
	return (showunversionedoverlay);
}

BOOL ShellCache::ShowIgnoredOverlay()
{
	RefreshIfNeeded();
	return (showignoredoverlay);
}

BOOL ShellCache::ShowExcludedAsNormal()
{
	RefreshIfNeeded();
	return (excludedasnormal);
}

BOOL ShellCache::HideMenusForUnversionedItems()
{
	RefreshIfNeeded();
	return (hidemenusforunversioneditems);
}

BOOL ShellCache::IsRemote()
{
	RefreshIfNeeded();
	return (driveremote);
}

BOOL ShellCache::IsFixed()
{
	RefreshIfNeeded();
	return (drivefixed);
}

BOOL ShellCache::IsCDRom()
{
	RefreshIfNeeded();
	return (drivecdrom);
}

BOOL ShellCache::IsRemovable()
{
	RefreshIfNeeded();
	return (driveremove);
}

BOOL ShellCache::IsRAM()
{
	RefreshIfNeeded();
	return (driveram);
}

BOOL ShellCache::IsUnknown()
{
	RefreshIfNeeded();
	return (driveunknown);
}

BOOL ShellCache::IsContextPathAllowed(LPCWSTR path)
{
	Locker lock(m_critSec);
	ExcludeContextValid();
	for (const auto& exPath : excontextvector)
	{
		if (exPath.empty())
			continue;
		if (exPath[exPath.size() - 1] == '*')
		{
			std::wstring str = exPath.substr(0, exPath.size() - 1);
			if (_wcsnicmp(str.c_str(), path, str.size()) == 0)
				return FALSE;
		}
		else if (_wcsicmp(exPath.c_str(), path) == 0)
			return FALSE;
	}
	return TRUE;
}

BOOL ShellCache::IsPathAllowed(LPCWSTR path)
{
	RefreshIfNeeded();
	Locker lock(m_critSec);
	Tristate allowed = pathFilter.IsPathAllowed(path);
	if (allowed != Tristate::Unknown)
		return allowed == Tristate::True ? TRUE : FALSE;

	UINT drivetype = 0;
	int drivenumber = PathGetDriveNumber(path);
	if ((drivenumber >= 0) && (drivenumber <= 25))
	{
		drivetype = drivetypecache[drivenumber];
		if ((drivetype == -1) || ((GetTickCount64() - drivetypeticker) > DRIVETYPETIMEOUT))
		{
			if ((DWORD(drivefloppy) == 0) && ((drivenumber == 0) || (drivenumber == 1)))
				drivetype = DRIVE_REMOVABLE;
			else if (PathIsNetworkPath(path))
				drivetype = DRIVE_REMOTE;
			else
			{
				wchar_t pathbuf[MAX_PATH + 4] = { 0 };		// MAX_PATH ok here. PathStripToRoot works with partial paths too.
				wcsncpy_s(pathbuf, path, _countof(pathbuf) - 1);
				PathStripToRoot(pathbuf);
				PathAddBackslash(pathbuf);
				CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": GetDriveType for %s, Drive %d\n", pathbuf, drivenumber);
				drivetype = GetDriveType(pathbuf);
			}
			drivetypecache[drivenumber] = drivetype;
			drivetypeticker = GetTickCount64();
		}
	}
	else
	{
		if (PathIsNetworkPath(path))
			drivetype = DRIVE_REMOTE;
		else
		{
			wchar_t pathbuf[MAX_PATH + 4] = { 0 }; // MAX_PATH ok here. PathStripToRoot works with partial paths too.
			wcsncpy_s(pathbuf, path, _countof(pathbuf) - 1);
			PathStripToRoot(pathbuf);
			PathAddBackslash(pathbuf);
			if (wcsncmp(pathbuf, drivetypepathcache, MAX_PATH - 1) == 0) // MAX_PATH ok.
				drivetype = drivetypecache[26];
			else
			{
				CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L"GetDriveType for %s\n", pathbuf);
				drivetype = GetDriveType(pathbuf);
				drivetypecache[26] = drivetype;
				wcsncpy_s(drivetypepathcache, pathbuf, MAX_PATH - 1); // MAX_PATH ok.
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

	return TRUE;
}

DWORD ShellCache::GetLangID()
{
	RefreshIfNeeded();
	return (langid);
}

BOOL ShellCache::HasGITAdminDir(LPCWSTR path, BOOL bIsDir, CString* ProjectTopDir /*= nullptr*/)
{
	std::wstring folder(path);
	if (!bIsDir)
	{
		size_t pos = folder.rfind(L'\\');
		if (pos != std::wstring::npos)
			folder.erase(pos);
	}
	std::map<std::wstring, AdminDir_s>::const_iterator iter;
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

void ShellCache::ExcludeContextValid()
{
	// Lock must be taken by caller, which is done by IsContextPathAllowed()
	RefreshIfNeeded();
	if (excludecontextstr.compare(nocontextpaths) == 0)
		return;

	excludecontextstr = nocontextpaths;
	excontextvector.clear();
	size_t pos = 0, pos_ant = 0;
	pos = excludecontextstr.find(L'\n', pos_ant);
	while (pos != std::wstring::npos)
	{
		std::wstring token = excludecontextstr.substr(pos_ant, pos - pos_ant);
		if (!token.empty())
			excontextvector.push_back(token);
		pos_ant = pos + 1;
		pos = excludecontextstr.find(L'\n', pos_ant);
	}
	if (!excludecontextstr.empty())
	{
		std::wstring token = excludecontextstr.substr(pos_ant, excludecontextstr.size() - 1);
		if (!token.empty())
			excontextvector.push_back(token);
	}
}

// construct \ref data content
void ShellCache::CPathFilter::AddEntry(const std::wstring& s, bool include)
{
	static wchar_t pathbuf[MAX_PATH * 4] = { 0 };
	if (s.empty())
		return;

	wchar_t lastChar = *s.rbegin();

	SEntry entry;
	entry.hasSubFolderEntries = false;
	entry.recursive = lastChar != L'?';
	entry.included = include ? Tristate::True : Tristate::False;
	entry.subPathIncluded = include == entry.recursive ? Tristate::True : Tristate::False;

	entry.path = s;
	if ((lastChar == L'?') || (lastChar == L'*'))
		entry.path.erase(s.length() - 1);
	if (!entry.path.empty() && (*entry.path.rbegin() == L'\\'))
		entry.path.erase(entry.path.length() - 1);

	if (auto ret = ExpandEnvironmentStrings(entry.path.c_str(), pathbuf, _countof(pathbuf)); ret > 0 && ret < _countof(pathbuf))
		entry.path = pathbuf;

	data.push_back(entry);
}

void ShellCache::CPathFilter::AddEntries(const std::wstring& s, bool include)
{
	size_t pos = 0, pos_ant = 0;
	pos = s.find(L'\n', pos_ant);
	while (pos != std::wstring::npos)
	{
		AddEntry(s.substr(pos_ant, pos - pos_ant), include);
		pos_ant = pos + 1;
		pos = s.find(L'\n', pos_ant);
	}

	if (!s.empty())
		AddEntry(s.substr(pos_ant, s.size() - 1), include);
}

// for all paths, have at least one entry in data
void ShellCache::CPathFilter::PostProcessData()
{
	if (data.empty())
		return;

	std::sort(data.begin(), data.end());

	// update subPathIncluded props and remove duplicate entries
	auto begin = data.begin();
	auto end = data.end();
	auto dest = begin;
	for (auto source = begin; source != end; ++source)
	{
		if (_wcsicmp(source->path.c_str(), dest->path.c_str()) == 0)
		{
			// multiple entries for the same path -> merge them

			// update subPathIncluded
			// (all relevant parent info has already been normalized)
			if (!source->recursive)
				source->subPathIncluded = IsPathAllowed(source->path.c_str(), begin, dest);

			// multiple specs for the same path
			// -> merge them into the existing entry @ dest
			if (!source->recursive && dest->recursive)
			{
				// reset the marker for the this case
				dest->recursive = false;
				dest->included = source->included;
			}
			else
			{
				// include beats exclude
				if (source->included == Tristate::True)
					dest->included = Tristate::True;
				if (source->recursive && source->subPathIncluded == Tristate::True)
					dest->subPathIncluded = Tristate::True;
			}
		}
		else
		{
			// new path -> don't merge this entry
			size_t destSize = dest->path.size();
			dest->hasSubFolderEntries = (source->path.size() > destSize) && (source->path[destSize] == L'\\') && (_wcsnicmp(source->path.substr(0, destSize).c_str(), dest->path.c_str(), destSize) == 0);

			*++dest = *source;

			// update subPathIncluded
			// (all relevant parent info has already been normalized)
			if (!dest->recursive)
				dest->subPathIncluded = IsPathAllowed(source->path.c_str(), begin, dest);
		}
	}

	// remove duplicate info
	if (begin != end)
		data.erase(++dest, end);
}

// lookup. default result is "unknown".
// We must look for *every* parent path because of situations like:
// excluded: C:, C:\some\deep\path
// include: C:\some
// lookup for C:\some\deeper\path
Tristate ShellCache::CPathFilter::IsPathAllowed(LPCWSTR path, TData::const_iterator begin, TData::const_iterator end) const
{
	Tristate result = Tristate::Unknown;

	// handle special cases
	if (begin == end)
		return result;

	size_t maxLength = wcslen(path);
	if (maxLength == 0)
		return result;

	// look for the most specific entry, start at the root
	size_t pos = 0;
	do
	{
		LPCWSTR backslash = wcschr(path + pos + 1,L'\\');
		pos = backslash == nullptr ? maxLength : backslash - path;

		std::pair<LPCWSTR, size_t> toFind(path, pos);
		TData::const_iterator iter = std::lower_bound(begin, end, toFind);

		// found a relevant entry?
		if ((iter != end) && (iter->path.length() == pos) && (_wcsnicmp(iter->path.c_str(), path, pos) == 0))
		{
			// exact match?
			if (pos == maxLength)
				return iter->included;

			// parent match
			result = iter->subPathIncluded;

			// done?
			if (iter->hasSubFolderEntries)
				begin = iter;
			else
				return result;
		}
		else
		{
			// set a (potentially) closer lower limit
			if (iter != begin)
				begin = --iter;
		}

		// set a (potentially) closer upper limit
		end = std::upper_bound(begin, end, toFind);
	} while ((pos < maxLength) && (begin != end));

	// nothing more specific found
	return result;
}

// construction

ShellCache::CPathFilter::CPathFilter()
	: excludelist(L"Software\\TortoiseGit\\OverlayExcludeList", L"", false, HKEY_CURRENT_USER, KEY_WOW64_64KEY)
	, includelist(L"Software\\TortoiseGit\\OverlayIncludeList", L"", false, HKEY_CURRENT_USER, KEY_WOW64_64KEY)
{
	Refresh();
}

// notify of (potential) registry settings

void ShellCache::CPathFilter::Refresh()
{
	excludelist.read();
	includelist.read();

	if (excludeliststr.compare(excludelist) == 0 && includeliststr.compare(includelist) == 0)
		return;

	excludeliststr = excludelist;
	includeliststr = includelist;
	data.clear();
	AddEntries(excludeliststr, false);
	AddEntries(includeliststr, true);

	PostProcessData();
}

// data access
Tristate ShellCache::CPathFilter::IsPathAllowed(LPCWSTR path) const
{
	if (!path)
		return Tristate::Unknown;
	// always ignore the recycle bin
	PTSTR pFound = StrStrI(path, L":\\RECYCLER");
	if (pFound)
	{
		if ((*(pFound + 10) == L'\0') || (*(pFound + 10) == L'\\'))
			return Tristate::False;
	}
	pFound = StrStrI(path, L":\\$Recycle.Bin");
	if (pFound)
	{
		if ((*(pFound + 14) == '\0') || (*(pFound + 14) == L'\\'))
			return Tristate::False;
	}
	return IsPathAllowed(path, data.begin(), data.end());
}
