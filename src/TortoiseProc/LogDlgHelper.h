// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2007 - TortoiseSVN
// Copyright (C) 2008-2013 - TortoiseGit

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
#include <set>
#include "GitHash.h"
#include "GitLogCache.h"
class CLogDlg;

/**
 * \ingroup TortoiseProc
 * Instances of CStoreSelection save the selection of the CLogDlg. When the instance
 * is deleted the destructor restores the selection.
 */
typedef std::map<CGitHash, int> MAP_HASH_REV;

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
 * Helper class for the log dialog, handles all the log entries, including
 * sorting.
 */
class CLogDataVector : 	public std::vector<CGitHash>
{
public:
	CLogCache *m_pLogCache;
	/// De-allocates log items.
	CLogDataVector(CLogCache *pLogCache)
	{
		m_pLogCache=pLogCache;
		m_FirstFreeLane=0;
	}
	CLogDataVector()
	{
		m_pLogCache=NULL;
		m_FirstFreeLane=0;
	}
	void SetLogCache(CLogCache *pLogCache)
	{
		m_pLogCache = pLogCache;
	}
	GitRev & GetGitRevAt(size_t i)
	{
		ASSERT(i<size());
		return 	m_pLogCache->m_HashMap[(*this)[i]];
	}
	void ClearAll();
	int  ParserFromLog(CTGitPath *path =NULL, int count = -1, int infomask = CGit::LOG_INFO_STAT | CGit::LOG_INFO_FILESTATE | CGit::LOG_INFO_SHOW_MERGEDFILE, CString *range = NULL);
	int  Fill(std::set<CGitHash>& hashes);

	int  FetchShortLog(CTGitPath *path , CString &hash,int count=-1 ,int mask=CGit::LOG_INFO_ONLY_HASH, int showWC=0 );
	int  ParserShortLog(CTGitPath *path ,CString &hash,int count=-1 ,int mask=CGit::LOG_INFO_ONLY_HASH );

	int FetchFullInfo(int i);
//	void AddFullInfo(

	Lanes m_Lns;
	int	 m_FirstFreeLane;
	MAP_HASH_REV m_HashMap;
	void updateLanes(GitRev& c, Lanes& lns, CGitHash &sha) ;
	void setLane(CGitHash& sha) ;
	void append(CGitHash& sha, bool storeInVector);

	BYTE_VECTOR		 m_RawlogData;
	std::vector<int> m_RawLogStart;

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
