// TortoiseSI - a Windows shell extension for easy version control

// Copyright (C) 2015 - TortoiseSI
// Copyright (C) 2012-2014 - TortoiseGit
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

typedef std::wstring stdstring;

#define REGISTRYTIMEOUT 2000
#define EXCLUDELISTTIMEOUT 5000
#define ADMINDIRTIMEOUT 10000
#define DRIVETYPETIMEOUT 300000		// 5 min
#define MENUTIMEOUT 100

typedef CComCritSecLock<CComCriticalSection> Locker;

/**
 * \ingroup TortoiseShell
 * Helper class which caches access to the registry. Also provides helper methods
 * for checks against the settings stored in the registry.
 */
class ShellCache
{
public:
	
	ShellCache()
	{
		debugLogging = CRegStdDWORD(REGISTRYTIMEOUT, _T("Software\\TortoiseSI\\DebugLogging"), FALSE);
		drivetypeticker = GetTickCount();
		langticker = drivetypeticker;

		langid = CRegStdDWORD(_T("Software\\TortoiseSI\\LanguageID"), 1033);
		for (int i = 0; i < 27; ++i)
		{
			drivetypecache[i] = (UINT)-1;
		}
		// A: and B: are floppy disks
		drivetypecache[0] = DRIVE_REMOVABLE;
		drivetypecache[1] = DRIVE_REMOVABLE;
		drivetypepathcache[0] = 0;
		m_critSec.Init();
	}
	void ForceRefresh()
	{
		langid.read();
		debugLogging.read();
	}

	bool IsPathAllowed(std::wstring path)
	{
		Locker lock(m_critSec);
		UINT drivetype = 0;
		int drivenumber = PathGetDriveNumber(path.c_str());
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

					drivetype = GetDriveType(getPathRoot(path).c_str());
					drivetypecache[drivenumber] = drivetype;
				}
			}
		}
		else
		{
			if (PathIsUNCServer(path.c_str()))
				drivetype = DRIVE_REMOTE;
			else
			{
				std::wstring pathRoot = getPathRoot(path);
				if (pathRoot == drivetypepathcache)		// MAX_PATH ok.
					drivetype = drivetypecache[26];
				else
				{
					drivetype = GetDriveType(pathRoot.c_str());
					drivetypecache[26] = drivetype;
					drivetypepathcache = pathRoot;
				}
			}
		}
		
		return drivetype == DRIVE_FIXED;
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

	BOOL IsDebugLogging()
	{
		Locker lock(m_critSec);
		return (debugLogging);
	}

private:
	std::wstring getPathRoot(std::wstring path) {
		TCHAR pathbuf[MAX_PATH + 4] = { 0 };		// MAX_PATH ok here. PathIsUNCServer works with partial paths too.
		_tcsncpy_s(pathbuf, MAX_PATH + 4, path.c_str(), MAX_PATH + 3);

		PathStripToRoot(pathbuf);
		PathAddBackslash(pathbuf);
		return pathbuf;
	};

public:
	CRegStdDWORD debugLogging;
	CRegStdDWORD langid;
	DWORD drivetypeticker;
	DWORD langticker;
	UINT  drivetypecache[27];
	std::wstring drivetypepathcache;
	CComCriticalSection m_critSec;
};
