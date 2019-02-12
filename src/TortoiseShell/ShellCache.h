// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2012-2019 - TortoiseGit
// Copyright (C) 2003-2011, 2017 - TortoiseSVN

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

#define ADMINDIRTIMEOUT 10000
#define DRIVETYPETIMEOUT 300000		// 5 min

#define DEFAULTMENUTOPENTRIES	MENUSYNC|MENUCREATEREPOS|MENUCLONE|MENUCOMMIT
#define DEFAULTMENUEXTENTRIES	MENUSVNIGNORE|MENUSTASHAPPLY|MENUSUBSYNC

typedef CComCritSecLock<CComCriticalSection> Locker;

typedef enum tristate_t
{
	/** state known to be false (the constant does not evaulate to false) */
	tristate_false = 2,
	/** state known to be true */
	tristate_true,
	/** state could be true or false */
	tristate_unknown
} tristate_t;

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
	~ShellCache();

	bool RefreshIfNeeded();

	CacheType GetCacheType();
	DWORD BlockStatus();
	unsigned __int64 GetMenuLayout();
	unsigned __int64 GetMenuExt();
	unsigned __int64 GetMenuMask();

	bool IsProcessElevated();
	BOOL IsOnlyNonElevated();

	BOOL IsRecursive();
	BOOL IsFolderOverlay();
	BOOL IsSimpleContext();
	BOOL HasShellMenuAccelerators();
	BOOL IsUnversionedAsModified();
	BOOL IsRecurseSubmodules();
	BOOL ShowUnversionedOverlay();
	BOOL ShowIgnoredOverlay();
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
	void ExcludeContextValid();

	class CPathFilter
	{
	public:
		/// node in the lookup tree
		struct SEntry
		{
			tstring path;

			/// default (path spec did not end a '?').
			/// if this is not set, the default for all
			/// sub-paths is !included.
			/// This is a temporary setting an be invalid
			/// after @ref PostProcessData
			bool recursive;

			/// this is an "include" specification
			tristate_t included;

			/// if @ref recursive is not set, this is
			/// the parent path status being passed down
			/// combined with the information of other
			/// entries for the same @ref path.
			tristate_t subPathIncluded;

			/// do entries for sub-paths exist?
			bool hasSubFolderEntries;

			/// STL support
			/// For efficient folding, it is imperative that
			/// "recursive" entries are first
			bool operator<(const SEntry& rhs) const
			{
				int diff = _wcsicmp(path.c_str(), rhs.path.c_str());
				return (diff < 0) || ((diff == 0) && recursive && !rhs.recursive);
			}

			friend bool operator<(const SEntry& rhs, const std::pair<LPCTSTR, size_t>& lhs);
			friend bool operator<(const std::pair<LPCTSTR, size_t>& lhs, const SEntry& rhs);
		};

	private:
		/// lookup by path (all entries sorted by path)
		typedef std::vector<SEntry> TData;
		TData data;

		/// registry keys plus cached last content
		CRegStdString excludelist;
		tstring excludeliststr;

		CRegStdString includelist;
		tstring includeliststr;

		/// construct \ref data content
		void AddEntry(const tstring& s, bool include);
		void AddEntries(const tstring& s, bool include);

		/// for all paths, have at least one entry in data
		void PostProcessData();

		/// lookup. default result is "unknown".
		/// We must look for *every* parent path because of situations like:
		/// excluded: C:, C:\some\deep\path
		/// include: C:\some
		/// lookup for C:\some\deeper\path
		tristate_t IsPathAllowed(LPCTSTR path, TData::const_iterator begin, TData::const_iterator end) const;

	public:
		/// construction
		CPathFilter();

		/// notify of (potential) registry settings
		void Refresh();

		/// data access
		tristate_t IsPathAllowed(LPCTSTR path) const;
	};

	friend bool operator< (const CPathFilter::SEntry& rhs, const std::pair<LPCTSTR, size_t>& lhs);
	friend bool operator< (const std::pair<LPCTSTR, size_t>& lhs, const CPathFilter::SEntry& rhs);

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
	CRegStdDWORD onlynonelevated;
	CRegStdDWORD showrecursive;
	CRegStdDWORD folderoverlay;
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
	CRegStdDWORD hidemenusforunversioneditems;

	CPathFilter pathFilter;

	ULONGLONG drivetypeticker;
	ULONGLONG menumaskticker;
	UINT  drivetypecache[27];
	TCHAR drivetypepathcache[MAX_PATH];		// MAX_PATH ok.
	TCHAR szDecSep[5];
	TCHAR szThousandsSep[5];
	std::map<tstring, AdminDir_s> admindircache;
	CRegStdString nocontextpaths;
	tstring excludecontextstr;
	std::vector<tstring> excontextvector;
	CComAutoCriticalSection m_critSec;
	HANDLE m_registryChangeEvent;
	HKEY m_hNotifyRegKey;
	bool isElevated;
};

inline bool operator<(const ShellCache::CPathFilter::SEntry& lhs, const std::pair<LPCTSTR, size_t>& rhs)
{
	return _wcsnicmp(lhs.path.c_str(), rhs.first, rhs.second) < 0;
}

inline bool operator<(const std::pair<LPCTSTR, size_t>& lhs, const ShellCache::CPathFilter::SEntry& rhs)
{
	return _wcsnicmp(lhs.first, rhs.path.c_str(), lhs.second) < 0;
}
