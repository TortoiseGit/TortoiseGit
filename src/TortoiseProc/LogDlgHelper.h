// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2007 - TortoiseGit

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
#include "Git.h"
#include "GitRev.h"
#include "GitStatus.h"
#include "ILogReceiver.h"
#include "lanes.h"

class CLogDlg;

/**
 * \ingroup TortoiseProc
 * Instances of CStoreSelection save the selection of the CLogDlg. When the instance
 * is deleted the destructor restores the selection.
 */
typedef std::map<CString, GitRev*> MAP_HASH_REV;

class CStoreSelection
{
public:
	CStoreSelection(CLogDlg* dlg);
	~CStoreSelection();
protected:
	CLogDlg* m_logdlg;
	std::set<LONG> m_SetSelectedRevisions;
};

/**
 * \ingroup TortoiseProc
 * Contains the data of one log entry, used in the log dialog
 */
#if 0
typedef struct LogEntryData
{   
	git_revnum_t Rev;
	__time64_t tmDate;
	CString sDate;
	CString sAuthor;
	CString sMessage;
	CString sShortMessage;
	CString sBugIDs;
	DWORD dwFileChanges;
	LogChangedPathArray* pArChangedPaths;
	BOOL bCopies;
	BOOL bCopiedSelf;
	DWORD actions;
	BOOL haschildren;
	DWORD childStackDepth;
} LOGENTRYDATA, *PLOGENTRYDATA;
#endif
/**
 * \ingroup TortoiseProc
 * Helper class for the log dialog, handles all the log entries, including
 * sorting.
 */
class CLogDataVector : 	public std::vector<GitRev>
{
public:
	/// De-allocates log items.
	CLogDataVector()
	{
		m_FirstFreeLane=0;
	}
	void ClearAll();
	int  ParserFromLog();
	Lanes m_Lanes;
	int	 m_FirstFreeLane;
	MAP_HASH_REV m_HashMap;
	void updateLanes(GitRev& c, Lanes& lns, CString &sha) ;
	void setLane(CString& sha) ;


#if 0
	/// Ascending date sorting.
	struct AscDateSort
	{
		bool operator()(GitRev& pStart, GitRev& pEnd)
		{
			return pStart->tmDate < pEnd->tmDate;
		}
	};
	/// Descending date sorting.
	struct DescDateSort
	{
		bool operator()(GitRev& pStart, GitRev& pEnd)
		{
			return pStart->tmDate > pEnd->tmDate;
		}
	};
	/// Ascending revision sorting.
	struct AscRevSort
	{
		bool operator()(GitRev& pStart, GitRev& pEnd)
		{
			return pStart->Rev < pEnd->Rev;
		}
	};
	/// Descending revision sorting.
	struct DescRevSort
	{
		bool operator()(GitRev& pStart, GitRev& pEnd)
		{
			return pStart->Rev > pEnd->Rev;
		}
	};
	/// Ascending author sorting.
	struct AscAuthorSort
	{
		bool operator()(GitRev& pStart, GitRev& pEnd)
		{
			int ret = pStart->sAuthor.CompareNoCase(pEnd->sAuthor);
			if (ret == 0)
				return pStart->Rev < pEnd->Rev;
			return ret<0;
		}
	};
	/// Descending author sorting.
	struct DescAuthorSort
	{
		bool operator()(GitRev& pStart, GitRev& pEnd)
		{
			int ret = pStart->sAuthor.CompareNoCase(pEnd->sAuthor);
			if (ret == 0)
				return pStart->Rev > pEnd->Rev;
			return ret>0;
		}
	};
	/// Ascending bugID sorting.
	struct AscBugIDSort
	{
		bool operator()(GitRev& pStart, GitRev& pEnd)
		{
			int ret = pStart->sBugIDs.CompareNoCase(pEnd->sBugIDs);
			if (ret == 0)
				return pStart->Rev < pEnd->Rev;
			return ret<0;
		}
	};
	/// Descending bugID sorting.
	struct DescBugIDSort
	{
		bool operator()(GitRev& pStart, GitRev& pEnd)
		{
			int ret = pStart->sBugIDs.CompareNoCase(pEnd->sBugIDs);
			if (ret == 0)
				return pStart->Rev > pEnd->Rev;
			return ret>0;
		}
	};
	/// Ascending message sorting.
	struct AscMessageSort
	{
		bool operator()(GitRev& pStart, GitRev& pEnd)
		{
			return pStart->sShortMessage.CompareNoCase(pEnd->sShortMessage)<0;
		}
	};
	/// Descending message sorting.
	struct DescMessageSort
	{
		bool operator()(GitRev& pStart, GitRev& pEnd)
		{
			return pStart->sShortMessage.CompareNoCase(pEnd->sShortMessage)>0;
		}
	};
	/// Ascending action sorting
	struct AscActionSort
	{
		bool operator() (GitRev& pStart, GitRev& pEnd)
		{
			if (pStart->actions == pEnd->actions)
				return pStart->Rev < pEnd->Rev;
			return pStart->actions < pEnd->actions;
		}
	};
	/// Descending action sorting
	struct DescActionSort
	{
		bool operator() (GitRev& pStart, GitRev& pEnd)
		{
			if (pStart->actions == pEnd->actions)
				return pStart->Rev > pEnd->Rev;
			return pStart->actions > pEnd->actions;
		}
	};
#endif
};
