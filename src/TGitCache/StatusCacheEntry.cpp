// TortoiseGit - a Windows shell extension for easy version control

// External Cache Copyright (C) 2005-2006,2008,2014 - TortoiseSVN
// Copyright (C) 2008-2014, 2016-2017, 2019 - TortoiseGit

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
#include "StatusCacheEntry.h"
#include "GitStatus.h"
#include "CacheInterface.h"
#include "registry.h"

#define CACHEVERION 8

ULONGLONG cachetimeout = static_cast<ULONGLONG>(CRegStdDWORD(L"Software\\TortoiseGit\\Cachetimeout", LONG_MAX));

CStatusCacheEntry::CStatusCacheEntry()
	: m_bSet(false)
	, m_highestPriorityLocalStatus(git_wc_status_none)
	, m_bAssumeValid(false)
	, m_bSkipWorktree(false)
{
	SetAsUnversioned();
}

CStatusCacheEntry::CStatusCacheEntry(const git_wc_status_kind status)
	: m_bSet(true)
	, m_highestPriorityLocalStatus(status)
	, m_bAssumeValid(false)
	, m_bSkipWorktree(false)
	, m_lastWriteTime(0)
{
	m_GitStatus.status = status;
	m_GitStatus.assumeValid = m_GitStatus.skipWorktree = false;
	m_discardAtTime = GetTickCount64() + cachetimeout;
}

CStatusCacheEntry::CStatusCacheEntry(const git_wc_status2_t* pGitStatus, __int64 lastWriteTime, LONGLONG validuntil /* = 0*/)
	: m_bSet(false)
	, m_highestPriorityLocalStatus(git_wc_status_none)
{
	SetStatus(pGitStatus);
	m_lastWriteTime = lastWriteTime;
	if (validuntil)
		m_discardAtTime = validuntil;
	else
		m_discardAtTime = GetTickCount64() + cachetimeout;
}

bool CStatusCacheEntry::SaveToDisk(FILE* pFile) const
{
#define WRITEVALUETOFILE(x) if (fwrite(&x, sizeof(x), 1, pFile)!=1) return false;
#define WRITESTRINGTOFILE(x) if (x.IsEmpty()) {value=0;WRITEVALUETOFILE(value);}else{value=x.GetLength();WRITEVALUETOFILE(value);if (fwrite(static_cast<LPCSTR>(x), sizeof(char), value, pFile)!=value) return false;}

	unsigned int value = CACHEVERION;
	WRITEVALUETOFILE(value); // 'version' of this save-format
	WRITEVALUETOFILE(m_highestPriorityLocalStatus);
	WRITEVALUETOFILE(m_lastWriteTime);
	WRITEVALUETOFILE(m_bSet);

	// now save the status struct (without the entry field, because we don't use that)
	WRITEVALUETOFILE(m_GitStatus.status);
	WRITEVALUETOFILE(m_GitStatus.assumeValid);
	WRITEVALUETOFILE(m_GitStatus.skipWorktree);
	return true;
}

bool CStatusCacheEntry::LoadFromDisk(FILE * pFile)
{
#define LOADVALUEFROMFILE(x) if (fread(&x, sizeof(x), 1, pFile)!=1) return false;
	try
	{
		unsigned int value = 0;
		LOADVALUEFROMFILE(value);
		if (value != CACHEVERION)
			return false;		// not the correct version
		LOADVALUEFROMFILE(m_highestPriorityLocalStatus);
		LOADVALUEFROMFILE(m_lastWriteTime);
		LOADVALUEFROMFILE(m_bSet);
		SecureZeroMemory(&m_GitStatus, sizeof(m_GitStatus));
		LOADVALUEFROMFILE(m_GitStatus.status);
		LOADVALUEFROMFILE(m_GitStatus.assumeValid);
		LOADVALUEFROMFILE(m_GitStatus.skipWorktree);
		m_discardAtTime = GetTickCount64() + cachetimeout;
	}
	catch ( CAtlException )
	{
		return false;
	}
	return true;
}

void CStatusCacheEntry::SetStatus(const git_wc_status2_t* pGitStatus)
{
	if (!pGitStatus)
	{
		SetAsUnversioned();
	}
	else
	{
		m_highestPriorityLocalStatus = pGitStatus->status;
		m_GitStatus = *pGitStatus;
		m_bAssumeValid = pGitStatus->assumeValid;
		m_bSkipWorktree = pGitStatus->skipWorktree;
	}
	m_discardAtTime = GetTickCount64() + cachetimeout;
	m_bSet = true;
}


void CStatusCacheEntry::SetAsUnversioned()
{
	SecureZeroMemory(&m_GitStatus, sizeof(m_GitStatus));
	m_discardAtTime = GetTickCount64() + cachetimeout;
	git_wc_status_kind status = git_wc_status_none;
	if (m_highestPriorityLocalStatus == git_wc_status_unversioned)
		status = git_wc_status_unversioned;
	m_highestPriorityLocalStatus = status;
	m_GitStatus.status = git_wc_status_none;
	m_lastWriteTime = 0;
	m_bAssumeValid = false;
	m_bSkipWorktree = false;
}

bool CStatusCacheEntry::HasExpired(LONGLONG now) const
{
	return m_discardAtTime != 0 && (now - m_discardAtTime) >= 0;
}

void CStatusCacheEntry::BuildCacheResponse(TGITCacheResponse& response, DWORD& responseLength) const
{
	SecureZeroMemory(&response, sizeof(response));
	response.m_status = static_cast<INT8>(m_GitStatus.status);
	response.m_bAssumeValid = m_bAssumeValid;
	response.m_bSkipWorktree = m_bSkipWorktree;
	responseLength = sizeof(response);
}

bool CStatusCacheEntry::IsVersioned() const
{
	return m_highestPriorityLocalStatus > git_wc_status_unversioned;
}

bool CStatusCacheEntry::DoesFileTimeMatch(__int64 testTime) const
{
	return m_lastWriteTime == testTime;
}


bool CStatusCacheEntry::ForceStatus(git_wc_status_kind forcedStatus)
{
	git_wc_status_kind newStatus = forcedStatus;

	if(newStatus != m_highestPriorityLocalStatus)
	{
		// We've had a status change
		m_highestPriorityLocalStatus = newStatus;
		m_GitStatus.status = newStatus;
		m_discardAtTime = GetTickCount64() + cachetimeout;
		return true;
	}
	return false;
}

bool
CStatusCacheEntry::HasBeenSet() const
{
	return m_bSet;
}

void CStatusCacheEntry::Invalidate()
{
	m_bSet = false;
}
