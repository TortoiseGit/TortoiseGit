// TortoiseGit - a Windows shell extension for easy version control

// External Cache Copyright (C) 2005 - 2006 - Will Dean, Stefan Kueng
// Copyright (C) 2008-2012, 2017 - TortoiseGit

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

struct TGITCacheResponse;
extern ULONGLONG cachetimeout;

#include "CacheInterface.h"

/**
 * \ingroup TGitCache
 * Holds all the status data of one file or folder.
 */
class CStatusCacheEntry
{
public:
	CStatusCacheEntry();
	CStatusCacheEntry(const git_wc_status_kind status);
	CStatusCacheEntry(const git_wc_status2_t* pGitStatus, __int64 lastWriteTime, LONGLONG validuntil = 0);
	bool HasExpired(LONGLONG now) const;
	void BuildCacheResponse(TGITCacheResponse& response, DWORD& responseLength) const;
	bool IsVersioned() const;
	bool DoesFileTimeMatch(__int64 testTime) const;
	bool ForceStatus(git_wc_status_kind forcedStatus);
	git_wc_status_kind GetEffectiveStatus() const { return m_highestPriorityLocalStatus; }
	void SetStatus(const git_wc_status2_t* pGitStatus);
	bool HasBeenSet() const;
	void Invalidate();
	bool SaveToDisk(FILE* pFile) const;
	bool LoadFromDisk(FILE * pFile);
private:
	void SetAsUnversioned();

private:
	LONGLONG			m_discardAtTime;
	git_wc_status_kind	m_highestPriorityLocalStatus;
	git_wc_status2_t	m_GitStatus;
	__int64				m_lastWriteTime;
	bool				m_bSet;
	bool				m_bAssumeValid;
	bool				m_bSkipWorktree;

	friend class CGitStatusCache;
};
