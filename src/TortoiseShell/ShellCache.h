// TortoiseGit - a Windows shell extension for easy version control

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
#include "GitAdminDir.h"
#include "Git.h"

#define REGISTRYTIMEOUT 2000
#define EXCLUDELISTTIMEOUT 5000
#define ADMINDIRTIMEOUT 10000
#define DRIVETYPETIMEOUT 300000		// 5 min
#define NUMBERFMTTIMEOUT 300000
#define MENUTIMEOUT 100

#define DEFAULTMENUTOPENTRIES	MENUSYNC|MENUCREATEREPOS|MENUCLONE|MENUCOMMIT
#define DEFAULTMENUEXTENTRIES	MENUSVNIGNORE|MENUREFLOG|MENUREFBROWSE|MENUSTASHAPPLY|MENUSUBSYNC

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
	ShellCache()
	{
		cachetype = CRegStdDWORD(_T("Software\\TortoiseGit\\CacheType"), GetSystemMetrics(SM_REMOTESESSION) ? dll : exe);
		showrecursive = CRegStdDWORD(_T("Software\\TortoiseGit\\RecursiveOverlay"), TRUE);
		folderoverlay = CRegStdDWORD(_T("Software\\TortoiseGit\\FolderOverlay"), TRUE);
		driveremote = CRegStdDWORD(_T("Software\\TortoiseGit\\DriveMaskRemote"));
		drivefixed = CRegStdDWORD(_T("Software\\TortoiseGit\\DriveMaskFixed"), TRUE);
		drivecdrom = CRegStdDWORD(_T("Software\\TortoiseGit\\DriveMaskCDROM"));
		driveremove = CRegStdDWORD(_T("Software\\TortoiseGit\\DriveMaskRemovable"));
		drivefloppy = CRegStdDWORD(_T("Software\\TortoiseGit\\DriveMaskFloppy"));
		driveram = CRegStdDWORD(_T("Software\\TortoiseGit\\DriveMaskRAM"));
		driveunknown = CRegStdDWORD(_T("Software\\TortoiseGit\\DriveMaskUnknown"));
		excludelist = CRegStdString(_T("Software\\TortoiseGit\\OverlayExcludeList"));
		includelist = CRegStdString(_T("Software\\TortoiseGit\\OverlayIncludeList"));
		simplecontext = CRegStdDWORD(_T("Software\\TortoiseGit\\SimpleContext"), FALSE);
		unversionedasmodified = CRegStdDWORD(_T("Software\\TortoiseGit\\UnversionedAsModified"), FALSE);
		hidemenusforunversioneditems = CRegStdDWORD(_T("Software\\TortoiseGit\\HideMenusForUnversionedItems"), FALSE);
		showunversionedoverlay = CRegStdDWORD(_T("Software\\TortoiseGit\\ShowUnversionedOverlay"), TRUE);
		showignoredoverlay = CRegStdDWORD(_T("Software\\TortoiseGit\\ShowIgnoredOverlay"), TRUE);
		getlocktop = CRegStdDWORD(_T("Software\\TortoiseGit\\GetLockTop"), TRUE);
		excludedasnormal = CRegStdDWORD(_T("Software\\TortoiseGit\\ShowExcludedAsNormal"), TRUE);
		cachetypeticker = GetTickCount();
		recursiveticker = cachetypeticker;
		folderoverlayticker = cachetypeticker;
		driveticker = cachetypeticker;
		drivetypeticker = cachetypeticker;
		langticker = cachetypeticker;
		columnrevformatticker = cachetypeticker;
		excludelistticker = cachetypeticker;
		includelistticker = cachetypeticker;
		simplecontextticker = cachetypeticker;
		unversionedasmodifiedticker = cachetypeticker;
		showunversionedoverlayticker = cachetypeticker;
		showignoredoverlayticker = cachetypeticker;
		admindirticker = cachetypeticker;
		columnseverywhereticker = cachetypeticker;
		getlocktopticker = cachetypeticker;
		excludedasnormalticker = cachetypeticker;
		hidemenusforunversioneditemsticker = cachetypeticker;
		excontextticker = cachetypeticker;

		unsigned __int64 entries = (DEFAULTMENUTOPENTRIES);
		menulayoutlow = CRegStdDWORD(_T("Software\\TortoiseGit\\ContextMenuEntries"),	  entries&0xFFFFFFFF);
		menulayouthigh = CRegStdDWORD(_T("Software\\TortoiseGit\\ContextMenuEntrieshigh"), entries>>32);

		unsigned __int64 ext = (DEFAULTMENUEXTENTRIES);
		menuextlow	= CRegStdDWORD(_T("Software\\TortoiseGit\\ContextMenuExtEntriesLow"), ext&0xFFFFFFFF  );
		menuexthigh = CRegStdDWORD(_T("Software\\TortoiseGit\\ContextMenuExtEntriesHigh"),	ext>>32	  );

		menumasklow_lm = CRegStdDWORD(_T("Software\\TortoiseGit\\ContextMenuEntriesMaskLow"), 0, FALSE, HKEY_LOCAL_MACHINE);
		menumaskhigh_lm = CRegStdDWORD(_T("Software\\TortoiseGit\\ContextMenuEntriesMaskHigh"), 0, FALSE, HKEY_LOCAL_MACHINE);
		menumasklow_cu = CRegStdDWORD(_T("Software\\TortoiseGit\\ContextMenuEntriesMaskLow"), 0);
		menumaskhigh_cu = CRegStdDWORD(_T("Software\\TortoiseGit\\ContextMenuEntriesMaskHigh"), 0);
		langid = CRegStdDWORD(_T("Software\\TortoiseGit\\LanguageID"), 1033);
		blockstatus = CRegStdDWORD(_T("Software\\TortoiseGit\\BlockStatus"), 0);
		columnseverywhere = CRegStdDWORD(_T("Software\\TortoiseGit\\ColumnsEveryWhere"), FALSE);
		for (int i=0; i<27; i++)
		{
			drivetypecache[i] = (UINT)-1;
		}
		// A: and B: are floppy disks
		drivetypecache[0] = DRIVE_REMOVABLE;
		drivetypecache[1] = DRIVE_REMOVABLE;
		TCHAR szBuffer[5];
		columnrevformatticker = GetTickCount();
		SecureZeroMemory(&columnrevformat, sizeof(NUMBERFMT));
		GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, &szDecSep[0], _countof(szDecSep));
		GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, &szThousandsSep[0], _countof(szThousandsSep));
		columnrevformat.lpDecimalSep = szDecSep;
		columnrevformat.lpThousandSep = szThousandsSep;
		GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, &szBuffer[0], _countof(szBuffer));
		columnrevformat.Grouping = _ttoi(szBuffer);
		GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_INEGNUMBER, &szBuffer[0], _countof(szBuffer));
		columnrevformat.NegativeOrder = _ttoi(szBuffer);
		sAdminDirCacheKey.reserve(MAX_PATH);		// MAX_PATH as buffer reservation ok.
		nocontextpaths = CRegStdString(_T("Software\\TortoiseGit\\NoContextPaths"), _T(""));
		m_critSec.Init();
	}
	void ForceRefresh()
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
		unversionedasmodified.read();
		showunversionedoverlay.read();
		showignoredoverlay.read();
		excludedasnormal.read();
		hidemenusforunversioneditems.read();
		menulayoutlow.read();
		menulayouthigh.read();
		langid.read();
		blockstatus.read();
		columnseverywhere.read();
		getlocktop.read();
		menumasklow_lm.read();
		menumaskhigh_lm.read();
		menumasklow_cu.read();
		menumaskhigh_cu.read();
		nocontextpaths.read();
	}
	CacheType GetCacheType()
	{
		if ((GetTickCount() - REGISTRYTIMEOUT) > cachetypeticker)
		{
			cachetypeticker = GetTickCount();
			cachetype.read();
		}
		return CacheType(DWORD((cachetype)));
	}
	DWORD BlockStatus()
	{
		if ((GetTickCount() - REGISTRYTIMEOUT) > blockstatusticker)
		{
			blockstatusticker = GetTickCount();
			blockstatus.read();
		}
		return (blockstatus);
	}
	unsigned __int64 GetMenuLayout()
	{
		if ((GetTickCount() - REGISTRYTIMEOUT) > layoutticker)
		{
			layoutticker = GetTickCount();
			menulayoutlow.read();
			menulayouthigh.read();
		}
		unsigned __int64 temp = unsigned __int64(DWORD(menulayouthigh))<<32;
		temp |= unsigned __int64(DWORD(menulayoutlow));
		return temp;
	}

	unsigned __int64 GetMenuExt()
	{
		if ((GetTickCount() - REGISTRYTIMEOUT) > exticker)
		{
			exticker = GetTickCount();
			menuextlow.read();
			menuexthigh.read();
		}
		unsigned __int64 temp = unsigned __int64(DWORD(menuexthigh))<<32;
		temp |= unsigned __int64(DWORD(menuextlow));
		return temp;
	}

	unsigned __int64 GetMenuMask()
	{
		if ((GetTickCount() - REGISTRYTIMEOUT) > menumaskticker)
		{
			menumaskticker = GetTickCount();
			menumasklow_lm.read();
			menumaskhigh_lm.read();
			menumasklow_cu.read();
			menumaskhigh_cu.read();
		}
		DWORD low = (DWORD)menumasklow_lm | (DWORD)menumasklow_cu;
		DWORD high = (DWORD)menumaskhigh_lm | (DWORD)menumaskhigh_cu;
		unsigned __int64 temp = unsigned __int64(high)<<32;
		temp |= unsigned __int64(low);
		return temp;
	}
	BOOL IsRecursive()
	{
		if ((GetTickCount() - REGISTRYTIMEOUT)>recursiveticker)
		{
			recursiveticker = GetTickCount();
			showrecursive.read();
		}
		return (showrecursive);
	}
	BOOL IsFolderOverlay()
	{
		if ((GetTickCount() - REGISTRYTIMEOUT)>folderoverlayticker)
		{
			folderoverlayticker = GetTickCount();
			folderoverlay.read();
		}
		return (folderoverlay);
	}
	BOOL IsSimpleContext()
	{
		if ((GetTickCount() - REGISTRYTIMEOUT)>simplecontextticker)
		{
			simplecontextticker = GetTickCount();
			simplecontext.read();
		}
		return (simplecontext!=0);
	}
	BOOL IsUnversionedAsModified()
	{
		if ((GetTickCount() - REGISTRYTIMEOUT)>unversionedasmodifiedticker)
		{
			unversionedasmodifiedticker = GetTickCount();
			unversionedasmodified.read();
		}
		return (unversionedasmodified);
	}
	BOOL ShowUnversionedOverlay()
	{
		if ((GetTickCount() - REGISTRYTIMEOUT)>showunversionedoverlayticker)
		{
			showunversionedoverlayticker = GetTickCount();
			showunversionedoverlay.read();
		}
		return (showunversionedoverlay);
	}
	BOOL ShowIgnoredOverlay()
	{
		if ((GetTickCount() - REGISTRYTIMEOUT)>showignoredoverlayticker)
		{
			showignoredoverlayticker = GetTickCount();
			showignoredoverlay.read();
		}
		return (showignoredoverlay);
	}
	BOOL IsGetLockTop()
	{
		if ((GetTickCount() - REGISTRYTIMEOUT)>getlocktopticker)
		{
			getlocktopticker = GetTickCount();
			getlocktop.read();
		}
		return (getlocktop);
	}
	BOOL ShowExcludedAsNormal()
	{
		if ((GetTickCount() - REGISTRYTIMEOUT)>excludedasnormalticker)
		{
			excludedasnormalticker = GetTickCount();
			excludedasnormal.read();
		}
		return (excludedasnormal);
	}
	BOOL ShellCache::HideMenusForUnversionedItems()
	{
	if ((GetTickCount() - hidemenusforunversioneditemsticker)>REGISTRYTIMEOUT)
		{
			hidemenusforunversioneditemsticker = GetTickCount();
			hidemenusforunversioneditems.read();
		}
		return (hidemenusforunversioneditems);
	}
	BOOL IsRemote()
	{
		DriveValid();
		return (driveremote);
	}
	BOOL IsFixed()
	{
		DriveValid();
		return (drivefixed);
	}
	BOOL IsCDRom()
	{
		DriveValid();
		return (drivecdrom);
	}
	BOOL IsRemovable()
	{
		DriveValid();
		return (driveremove);
	}
	BOOL IsRAM()
	{
		DriveValid();
		return (driveram);
	}
	BOOL IsUnknown()
	{
		DriveValid();
		return (driveunknown);
	}
	BOOL IsContextPathAllowed(LPCTSTR path)
	{
		Locker lock(m_critSec);
		ExcludeContextValid();
		for (std::vector<stdstring>::iterator I = excontextvector.begin(); I != excontextvector.end(); ++I)
		{
			if (I->empty())
				continue;
			if (I->size() && I->at(I->size()-1)=='*')
			{
				stdstring str = I->substr(0, I->size()-1);
				if (_tcsnicmp(str.c_str(), path, str.size())==0)
					return FALSE;
			}
			else if (_tcsicmp(I->c_str(), path)==0)
				return FALSE;
		}
		return TRUE;
	}
	BOOL IsPathAllowed(LPCTSTR path)
	{
		Locker lock(m_critSec);
		IncludeListValid();
		for (std::vector<stdstring>::iterator I = invector.begin(); I != invector.end(); ++I)
		{
			if (I->empty())
				continue;
			if (I->at(I->size()-1)=='*')
			{
				stdstring str = I->substr(0, I->size()-1);
				if (_tcsnicmp(str.c_str(), path, str.size())==0)
					return TRUE;
				if (!str.empty() && (str.at(str.size()-1) == '\\') && (_tcsnicmp(str.c_str(), path, str.size()-1)==0))
					return TRUE;
			}
			else if (_tcsicmp(I->c_str(), path)==0)
				return TRUE;
			else if ((I->at(I->size()-1) == '\\') &&
				((_tcsnicmp(I->c_str(), path, I->size())==0) || (_tcsicmp(I->c_str(), path)==0)) )
				return TRUE;

		}
		UINT drivetype = 0;
		int drivenumber = PathGetDriveNumber(path);
		if ((drivenumber >=0)&&(drivenumber < 25))
		{
			drivetype = drivetypecache[drivenumber];
			if ((drivetype == -1)||((GetTickCount() - DRIVETYPETIMEOUT)>drivetypeticker))
			{
				if ((drivenumber == 0)||(drivenumber == 1))
					drivetypecache[drivenumber] = DRIVE_REMOVABLE;
				else
				{
					drivetypeticker = GetTickCount();
					TCHAR pathbuf[MAX_PATH+4];		// MAX_PATH ok here. PathStripToRoot works with partial paths too.
					_tcsncpy_s(pathbuf, MAX_PATH+4, path, MAX_PATH+3);
					PathStripToRoot(pathbuf);
					PathAddBackslash(pathbuf);
					ATLTRACE2(_T("GetDriveType for %s, Drive %d\n"), pathbuf, drivenumber);
					drivetype = GetDriveType(pathbuf);
					drivetypecache[drivenumber] = drivetype;
				}
			}
		}
		else
		{
			TCHAR pathbuf[MAX_PATH+4];		// MAX_PATH ok here. PathIsUNCServer works with partial paths too.
			_tcsncpy_s(pathbuf, MAX_PATH+4, path, MAX_PATH+3);
			if (PathIsUNCServer(pathbuf))
				drivetype = DRIVE_REMOTE;
			else
			{
				PathStripToRoot(pathbuf);
				PathAddBackslash(pathbuf);
				if (_tcsncmp(pathbuf, drivetypepathcache, MAX_PATH-1)==0)		// MAX_PATH ok.
					drivetype = drivetypecache[26];
				else
				{
					ATLTRACE2(_T("GetDriveType for %s\n"), pathbuf);
					drivetype = GetDriveType(pathbuf);
					drivetypecache[26] = drivetype;
					_tcsncpy_s(drivetypepathcache, MAX_PATH, pathbuf, MAX_PATH);			// MAX_PATH ok.
				}
			}
		}
		if ((drivetype == DRIVE_REMOVABLE)&&(!IsRemovable()))
			return FALSE;
		if ((drivetype == DRIVE_REMOVABLE)&&(drivefloppy == 0)&&((drivenumber==0)||(drivenumber==1)))
			return FALSE;
		if ((drivetype == DRIVE_FIXED)&&(!IsFixed()))
			return FALSE;
		if (((drivetype == DRIVE_REMOTE)||(drivetype == DRIVE_NO_ROOT_DIR))&&(!IsRemote()))
			return FALSE;
		if ((drivetype == DRIVE_CDROM)&&(!IsCDRom()))
			return FALSE;
		if ((drivetype == DRIVE_RAMDISK)&&(!IsRAM()))
			return FALSE;
		if ((drivetype == DRIVE_UNKNOWN)&&(IsUnknown()))
			return FALSE;

		ExcludeListValid();
		for (std::vector<stdstring>::iterator I = exvector.begin(); I != exvector.end(); ++I)
		{
			if (I->empty())
				continue;
			if (I->size() && I->at(I->size()-1)=='*')
			{
				stdstring str = I->substr(0, I->size()-1);
				if (_tcsnicmp(str.c_str(), path, str.size())==0)
					return FALSE;
			}
			else if (_tcsicmp(I->c_str(), path)==0)
				return FALSE;
		}
		return TRUE;
	}
	DWORD GetLangID()
	{
		if ((GetTickCount() - REGISTRYTIMEOUT) > langticker)
		{
			langticker = GetTickCount();
			langid.read();
		}
		return (langid);
	}
	NUMBERFMT * GetNumberFmt()
	{
		if ((GetTickCount() - NUMBERFMTTIMEOUT) > columnrevformatticker)
		{
			TCHAR szBuffer[5];
			columnrevformatticker = GetTickCount();
			SecureZeroMemory(&columnrevformat, sizeof(NUMBERFMT));
			GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, &szDecSep[0], _countof(szDecSep));
			GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, &szThousandsSep[0], _countof(szThousandsSep));
			columnrevformat.lpDecimalSep = szDecSep;
			columnrevformat.lpThousandSep = szThousandsSep;
			GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, &szBuffer[0], _countof(szBuffer));
			columnrevformat.Grouping = _ttoi(szBuffer);
			GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_INEGNUMBER, &szBuffer[0], _countof(szBuffer));
			columnrevformat.NegativeOrder = _ttoi(szBuffer);
		}
		return &columnrevformat;
	}
	BOOL HasGITAdminDir(LPCTSTR path, BOOL bIsDir, CString *ProjectTopDir = NULL)
	{
		size_t len = _tcslen(path);
		TCHAR * buf = new TCHAR[len+1];
		_tcscpy_s(buf, len+1, path);
		if (! bIsDir)
		{
			TCHAR * ptr = _tcsrchr(buf, '\\');
			if (ptr != 0)
			{
				*ptr = 0;
			}
		}
		if ((GetTickCount() - ADMINDIRTIMEOUT) < admindirticker)
		{
			std::map<stdstring, AdminDir_s>::iterator iter;
			sAdminDirCacheKey.assign(buf);
			if ((iter = admindircache.find(sAdminDirCacheKey)) != admindircache.end())
			{
				delete [] buf;
				if (ProjectTopDir && iter->second.bHasAdminDir)
					*ProjectTopDir = iter->second.sProjectRoot.c_str();
				return iter->second.bHasAdminDir;
			}
		}
		CString sProjectRoot;
		BOOL hasAdminDir = g_GitAdminDir.HasAdminDir(buf, true, &sProjectRoot);
		admindirticker = GetTickCount();
		Locker lock(m_critSec);

		AdminDir_s &ad = admindircache[buf];
		ad.bHasAdminDir = hasAdminDir;
		if (hasAdminDir)
		{
			ad.sProjectRoot.assign(sProjectRoot);

			if (ProjectTopDir)
				*ProjectTopDir = sProjectRoot;
		}

		delete [] buf;
		return hasAdminDir;
	}
	bool IsColumnsEveryWhere()
	{
		if ((GetTickCount() - REGISTRYTIMEOUT) > columnseverywhereticker)
		{
			columnseverywhereticker = GetTickCount();
			columnseverywhere.read();
		}
		return !!(DWORD)columnseverywhere;
	}
private:
	void DriveValid()
	{
		if ((GetTickCount() - REGISTRYTIMEOUT)>driveticker)
		{
			driveticker = GetTickCount();
			driveremote.read();
			drivefixed.read();
			drivecdrom.read();
			driveremove.read();
			drivefloppy.read();
		}
	}
	void ExcludeContextValid()
	{
		if ((GetTickCount() - EXCLUDELISTTIMEOUT)>excontextticker)
		{
			Locker lock(m_critSec);
			excontextticker = GetTickCount();
			nocontextpaths.read();
			if (excludecontextstr.compare((stdstring)nocontextpaths)==0)
				return;
			excludecontextstr = (stdstring)nocontextpaths;
			excontextvector.clear();
			size_t pos = 0, pos_ant = 0;
			pos = excludecontextstr.find(_T("\n"), pos_ant);
			while (pos != stdstring::npos)
			{
				stdstring token = excludecontextstr.substr(pos_ant, pos-pos_ant);
				excontextvector.push_back(token);
				pos_ant = pos+1;
				pos = excludecontextstr.find(_T("\n"), pos_ant);
			}
			if (!excludecontextstr.empty())
			{
				excontextvector.push_back(excludecontextstr.substr(pos_ant, excludecontextstr.size()-1));
			}
			excludecontextstr = (stdstring)nocontextpaths;
		}
	}
	void ExcludeListValid()
	{
		if ((GetTickCount() - EXCLUDELISTTIMEOUT)>excludelistticker)
		{
			Locker lock(m_critSec);
			excludelistticker = GetTickCount();
			excludelist.read();
			if (excludeliststr.compare((stdstring)excludelist)==0)
				return;
			excludeliststr = (stdstring)excludelist;
			exvector.clear();
			size_t pos = 0, pos_ant = 0;
			pos = excludeliststr.find(_T("\n"), pos_ant);
			while (pos != stdstring::npos)
			{
				stdstring token = excludeliststr.substr(pos_ant, pos-pos_ant);
				exvector.push_back(token);
				pos_ant = pos+1;
				pos = excludeliststr.find(_T("\n"), pos_ant);
			}
			if (!excludeliststr.empty())
			{
				exvector.push_back(excludeliststr.substr(pos_ant, excludeliststr.size()-1));
			}
			excludeliststr = (stdstring)excludelist;
		}
	}
	void IncludeListValid()
	{
		if ((GetTickCount() - EXCLUDELISTTIMEOUT)>includelistticker)
		{
			Locker lock(m_critSec);
			includelistticker = GetTickCount();
			includelist.read();
			if (includeliststr.compare((stdstring)includelist)==0)
				return;
			includeliststr = (stdstring)includelist;
			invector.clear();
			size_t pos = 0, pos_ant = 0;
			pos = includeliststr.find(_T("\n"), pos_ant);
			while (pos != stdstring::npos)
			{
				stdstring token = includeliststr.substr(pos_ant, pos-pos_ant);
				invector.push_back(token);
				pos_ant = pos+1;
				pos = includeliststr.find(_T("\n"), pos_ant);
			}
			if (!includeliststr.empty())
			{
				invector.push_back(includeliststr.substr(pos_ant, includeliststr.size()-1));
			}
			includeliststr = (stdstring)includelist;
		}
	}

	struct AdminDir_s
	{
		BOOL bHasAdminDir;
		stdstring sProjectRoot;
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
	CRegStdDWORD menuextlow;	   /* ext menu mask */
	CRegStdDWORD menuexthigh;
	CRegStdDWORD simplecontext;
	CRegStdDWORD menumasklow_lm;
	CRegStdDWORD menumaskhigh_lm;
	CRegStdDWORD menumasklow_cu;
	CRegStdDWORD menumaskhigh_cu;
	CRegStdDWORD unversionedasmodified;
	CRegStdDWORD showunversionedoverlay;
	CRegStdDWORD showignoredoverlay;
	CRegStdDWORD excludedasnormal;
	CRegStdString excludelist;
	CRegStdDWORD hidemenusforunversioneditems;
	CRegStdDWORD columnseverywhere;
	stdstring excludeliststr;
	std::vector<stdstring> exvector;
	CRegStdString includelist;
	stdstring includeliststr;
	std::vector<stdstring> invector;
	DWORD cachetypeticker;
	DWORD recursiveticker;
	DWORD folderoverlayticker;
	DWORD getlocktopticker;
	DWORD driveticker;
	DWORD drivetypeticker;
	DWORD layoutticker;
	DWORD exticker;
	DWORD menumaskticker;
	DWORD langticker;
	DWORD blockstatusticker;
	DWORD columnrevformatticker;
	DWORD excludelistticker;
	DWORD includelistticker;
	DWORD simplecontextticker;
	DWORD unversionedasmodifiedticker;
	DWORD showunversionedoverlayticker;
	DWORD showignoredoverlayticker;
	DWORD excludedasnormalticker;
	DWORD hidemenusforunversioneditemsticker;
	DWORD columnseverywhereticker;
	UINT  drivetypecache[27];
	TCHAR drivetypepathcache[MAX_PATH];		// MAX_PATH ok.
	NUMBERFMT columnrevformat;
	TCHAR szDecSep[5];
	TCHAR szThousandsSep[5];
	std::map<stdstring, AdminDir_s> admindircache;
	stdstring sAdminDirCacheKey;
	CRegStdString nocontextpaths;
	stdstring excludecontextstr;
	std::vector<stdstring> excontextvector;
	DWORD excontextticker;
	DWORD admindirticker;
	CComCriticalSection m_critSec;
};
