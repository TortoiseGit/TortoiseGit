// TortoiseGit - a Windows shell extension for easy version control

// External Cache Copyright (C) 2005 - 2006 - Will Dean, Stefan Kueng
// Copyright (C) 2008-2012 - TortoiseGit

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
#define CACHETIMEOUT	0x7FFFFFFF
extern DWORD cachetimeout;

#include "CacheInterface.h"

/**
 * \ingroup TSVNCache
 * Holds all the status data of one file or folder.
 */
class CStatusCacheEntry
{
public:
	CStatusCacheEntry();
	CStatusCacheEntry(const git_wc_status_kind status);
	CStatusCacheEntry(const git_wc_status2_t* pGitStatus, __int64 lastWriteTime, bool bReadOnly, DWORD validuntil = 0);
	bool HasExpired(long now) const;
	void BuildCacheResponse(TGITCacheResponse& response, DWORD& responseLength) const;
	bool IsVersioned() const;
	bool DoesFileTimeMatch(__int64 testTime) const;
	bool ForceStatus(git_wc_status_kind forcedStatus);
	git_wc_status_kind GetEffectiveStatus() const { return m_highestPriorityLocalStatus; }
	bool IsKindKnown() const { return ((m_kind != git_node_none)&&(m_kind != git_node_unknown)); }
	void SetStatus(const git_wc_status2_t* pGitStatus);
	bool HasBeenSet() const;
	void Invalidate();
	bool IsDirectory() const { return ((m_kind == git_node_dir)&&(m_highestPriorityLocalStatus != git_wc_status_ignored)); }
	bool SaveToDisk(FILE * pFile);
	bool LoadFromDisk(FILE * pFile);
	void SetKind(git_node_kind_t kind) { m_kind = kind; if (kind == git_node_dir) { m_bAssumeValid = false; m_bSkipWorktree = false; } }
private:
	void SetAsUnversioned();

private:
	long				m_discardAtTime;
	git_wc_status_kind	m_highestPriorityLocalStatus;
	git_wc_status2_t	m_GitStatus;
	__int64				m_lastWriteTime;
	bool				m_bSet;
	git_node_kind_t		m_kind;
	bool				m_bAssumeValid;
	bool				m_bSkipWorktree;

	friend class CGitStatusCache;
};
