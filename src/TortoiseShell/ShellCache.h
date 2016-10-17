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
#pragma once
#include "registry.h"
#include "Globals.h"

#define REGISTRYTIMEOUT 2000
#define EXCLUDELISTTIMEOUT 5000
#define ADMINDIRTIMEOUT 10000
#define DRIVETYPETIMEOUT 300000		// 5 min
#define MENUTIMEOUT 100

#define DEFAULTMENUTOPENTRIES	MENUSYNC|MENUCREATEREPOS|MENUCLONE|MENUCOMMIT
#define DEFAULTMENUEXTENTRIES	MENUSVNIGNORE|MENUSTASHAPPLY|MENUSUBSYNC

typedef CComCritSecLock<CComCriticalSection> Locker;

/**
 * \ingroup TortoiseShell
 * Helper class which caches access to the registry. Also provides helper methods
 * for checks against the settings stored in the registry.
 */
class ShellCache
{
public:
	enum CacheType
	{
		none,
		exe,
		dll,
		dllFull,// same as dll except it uses commandline git tool with all status modes supported
	};

	ShellCache();
	~ShellCache() {}

	void ForceRefresh();

	CacheType GetCacheType();
	DWORD BlockStatus();
	unsigned __int64 GetMenuLayout();
	unsigned __int64 GetMenuExt();
	unsigned __int64 GetMenuMask();

	BOOL IsRecursive();
	BOOL IsFolderOverlay();
	BOOL IsSimpleContext();
	BOOL HasShellMenuAccelerators();
	BOOL IsUnversionedAsModified();
	BOOL IsRecurseSubmodules();
	BOOL ShowUnversionedOverlay();
	BOOL ShowIgnoredOverlay();
	BOOL IsGetLockTop();
	BOOL ShowExcludedAsNormal();
	BOOL HideMenusForUnversionedItems();

	BOOL IsRemote();
	BOOL IsFixed();
	BOOL IsCDRom();
	BOOL IsRemovable();
	BOOL IsRAM();
	BOOL IsUnknown();

	BOOL IsContextPathAllowed(LPCTSTR path);
	BOOL IsPathAllowed(LPCTSTR path);
	DWORD GetLangID();
	BOOL HasGITAdminDir(LPCTSTR path, BOOL bIsDir, CString* ProjectTopDir = nullptr);

private:
	void DriveValid();
	void ExcludeContextValid();
	void ExcludeListValid();
	void IncludeListValid();

	struct AdminDir_s
	{
		BOOL bHasAdminDir;
		tstring sProjectRoot;
		ULONGLONG timeout;
	};

public:
	CRegStdDWORD cachetype;
	CRegStdDWORD blockstatus;
	CRegStdDWORD langid;
	CRegStdDWORD showrecursive;
	CRegStdDWORD folderoverlay;
	CRegStdDWORD getlocktop;
	CRegStdDWORD driveremote;
	CRegStdDWORD drivefixed;
	CRegStdDWORD drivecdrom;
	CRegStdDWORD driveremove;
	CRegStdDWORD drivefloppy;
	CRegStdDWORD driveram;
	CRegStdDWORD driveunknown;
	CRegStdDWORD menulayoutlow; /* Fist level mask */
	CRegStdDWORD menulayouthigh;
	CRegStdDWORD shellmenuaccelerators;
	CRegStdDWORD menuextlow;	   /* ext menu mask */
	CRegStdDWORD menuexthigh;
	CRegStdDWORD simplecontext;
	CRegStdDWORD menumasklow_lm;
	CRegStdDWORD menumaskhigh_lm;
	CRegStdDWORD menumasklow_cu;
	CRegStdDWORD menumaskhigh_cu;
	CRegStdDWORD unversionedasmodified;
	CRegStdDWORD recursesubmodules;
	CRegStdDWORD showunversionedoverlay;
	CRegStdDWORD showignoredoverlay;
	CRegStdDWORD excludedasnormal;
	CRegStdString excludelist;
	CRegStdDWORD hidemenusforunversioneditems;
	tstring excludeliststr;
	std::vector<tstring> exvector;
	CRegStdString includelist;
	tstring includeliststr;
	std::vector<tstring> invector;
	ULONGLONG cachetypeticker;
	ULONGLONG recursiveticker;
	ULONGLONG folderoverlayticker;
	ULONGLONG getlocktopticker;
	ULONGLONG driveticker;
	ULONGLONG drivetypeticker;
	ULONGLONG layoutticker;
	ULONGLONG exticker;
	ULONGLONG menumaskticker;
	ULONGLONG langticker;
	ULONGLONG blockstatusticker;
	ULONGLONG excludelistticker;
	ULONGLONG includelistticker;
	ULONGLONG simplecontextticker;
	ULONGLONG shellmenuacceleratorsticker;
	ULONGLONG unversionedasmodifiedticker;
	ULONGLONG recursesubmodulesticker;
	ULONGLONG showunversionedoverlayticker;
	ULONGLONG showignoredoverlayticker;
	ULONGLONG excludedasnormalticker;
	ULONGLONG hidemenusforunversioneditemsticker;
	UINT  drivetypecache[27];
	TCHAR drivetypepathcache[MAX_PATH];		// MAX_PATH ok.
	TCHAR szDecSep[5];
	TCHAR szThousandsSep[5];
	std::map<tstring, AdminDir_s> admindircache;
	CRegStdString nocontextpaths;
	tstring excludecontextstr;
	std::vector<tstring> excontextvector;
	ULONGLONG excontextticker;
	CComCriticalSection m_critSec;
};
