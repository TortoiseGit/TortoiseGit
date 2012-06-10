// TortoiseGit - a Windows shell extension for easy version control

// External Cache Copyright (C) 2005-2006,2008 - TortoiseSVN
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
#include "StdAfx.h"
#include ".\statuscacheentry.h"
#include "GitStatus.h"
#include "CacheInterface.h"
#include "registry.h"

#define CACHEVERION 5

DWORD cachetimeout = (DWORD)CRegStdDWORD(_T("Software\\TortoiseGit\\Cachetimeout"), CACHETIMEOUT);

CStatusCacheEntry::CStatusCacheEntry()
	: m_bSet(false)
	, m_bSVNEntryFieldSet(false)
	, m_kind(git_node_unknown)
	, m_highestPriorityLocalStatus(git_wc_status_none)
{
	SetAsUnversioned();
}

CStatusCacheEntry::CStatusCacheEntry(const git_wc_status_kind status)
	: m_bSet(true)
	, m_bSVNEntryFieldSet(false)
	, m_kind(git_node_unknown)
	, m_highestPriorityLocalStatus(status)
{
	m_GitStatus.prop_status=m_GitStatus.text_status = status;
	m_discardAtTime = GetTickCount()+cachetimeout;
}

CStatusCacheEntry::CStatusCacheEntry(const git_wc_status2_t* pGitStatus, __int64 lastWriteTime, bool bReadOnly, DWORD validuntil /* = 0*/)
	: m_bSet(false)
	, m_bSVNEntryFieldSet(false)
	, m_kind(git_node_unknown)
	, m_highestPriorityLocalStatus(git_wc_status_none)
{
	SetStatus(pGitStatus);
	m_lastWriteTime = lastWriteTime;
	if (validuntil)
		m_discardAtTime = validuntil;
	else
		m_discardAtTime = GetTickCount()+cachetimeout;
}

bool CStatusCacheEntry::SaveToDisk(FILE * pFile)
{
#define WRITEVALUETOFILE(x) if (fwrite(&x, sizeof(x), 1, pFile)!=1) return false;
#define WRITESTRINGTOFILE(x) if (x.IsEmpty()) {value=0;WRITEVALUETOFILE(value);}else{value=x.GetLength();WRITEVALUETOFILE(value);if (fwrite((LPCSTR)x, sizeof(char), value, pFile)!=value) return false;}

	unsigned int value = CACHEVERION;
	WRITEVALUETOFILE(value); // 'version' of this save-format
	WRITEVALUETOFILE(m_highestPriorityLocalStatus);
	WRITEVALUETOFILE(m_lastWriteTime);
	WRITEVALUETOFILE(m_bSet);
	WRITEVALUETOFILE(m_bSVNEntryFieldSet);
	WRITESTRINGTOFILE(m_sUrl);
	WRITESTRINGTOFILE(m_sAuthor);
	WRITEVALUETOFILE(m_kind);

	// now save the status struct (without the entry field, because we don't use that)	WRITEVALUETOFILE(m_GitStatus.copied);
	WRITEVALUETOFILE(m_GitStatus.prop_status);
//	WRITEVALUETOFILE(m_GitStatus.repos_prop_status);
//	WRITEVALUETOFILE(m_GitStatus.repos_text_status);
	WRITEVALUETOFILE(m_GitStatus.text_status);
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
		LOADVALUEFROMFILE(m_bSVNEntryFieldSet);
		LOADVALUEFROMFILE(value);
		if (value != 0)
		{
			if (value > INTERNET_MAX_URL_LENGTH)
				return false;		// invalid length for an url
			if (fread(m_sUrl.GetBuffer(value+1), sizeof(char), value, pFile)!=value)
			{
				m_sUrl.ReleaseBuffer(0);
				return false;
			}
			m_sUrl.ReleaseBuffer(value);
		}
		LOADVALUEFROMFILE(value);
		if (value != 0)
		{
			if (fread(m_sAuthor.GetBuffer(value+1), sizeof(char), value, pFile)!=value)
			{
				m_sAuthor.ReleaseBuffer(0);
				return false;
			}
			m_sAuthor.ReleaseBuffer(value);
		}
		LOADVALUEFROMFILE(m_kind);
		SecureZeroMemory(&m_GitStatus, sizeof(m_GitStatus));
		LOADVALUEFROMFILE(m_GitStatus.prop_status);
//		LOADVALUEFROMFILE(m_GitStatus.repos_prop_status);
//		LOADVALUEFROMFILE(m_GitStatus.repos_text_status);
		LOADVALUEFROMFILE(m_GitStatus.text_status);
//		m_GitStatus.entry = NULL;
		m_discardAtTime = GetTickCount()+cachetimeout;
	}
	catch ( CAtlException )
	{
		return false;
	}
	return true;
}

void CStatusCacheEntry::SetStatus(const git_wc_status2_t* pGitStatus)
{
	if(pGitStatus == NULL)
	{
		SetAsUnversioned();
	}
	else
	{
		m_highestPriorityLocalStatus = GitStatus::GetMoreImportant(pGitStatus->prop_status, pGitStatus->text_status);
		m_GitStatus = *pGitStatus;

		// Currently we don't deep-copy the whole entry value, but we do take a few members
/*        if(pGitStatus->entry != NULL)
		{
			m_sUrl = pGitStatus->entry->url;
			m_bSVNEntryFieldSet = true;
			m_kind = pGitStatus->entry->kind;
			m_sAuthor = pGitStatus->entry->cmt_author;
		}
		else*/
		{
			m_sUrl.Empty();
			m_bSVNEntryFieldSet = false;
		}
//		m_GitStatus.entry = NULL;
	}
	m_discardAtTime = GetTickCount()+cachetimeout;
	m_bSet = true;
}


void CStatusCacheEntry::SetAsUnversioned()
{
	SecureZeroMemory(&m_GitStatus, sizeof(m_GitStatus));
	m_discardAtTime = GetTickCount()+cachetimeout;
	git_wc_status_kind status = git_wc_status_none;
	if (m_highestPriorityLocalStatus == git_wc_status_missing)
		status = git_wc_status_missing;
	if (m_highestPriorityLocalStatus == git_wc_status_unversioned)
		status = git_wc_status_unversioned;
	m_highestPriorityLocalStatus = status;
	m_GitStatus.prop_status = git_wc_status_none;
	m_GitStatus.text_status = status;
	m_lastWriteTime = 0;
}

bool CStatusCacheEntry::HasExpired(long now) const
{
	return m_discardAtTime != 0 && (now - m_discardAtTime) >= 0;
}

void CStatusCacheEntry::BuildCacheResponse(TGITCacheResponse& response, DWORD& responseLength) const
{
	SecureZeroMemory(&response, sizeof(response));
	if(m_bSVNEntryFieldSet)
	{
		response.m_status = m_GitStatus;
		// There is no point trying to set these pointers here, because this is not 
		// the process which will be using the data.
		// The process which receives this response (generally the TSVN Shell Extension)
		// must fix-up these pointers when it gets them
		// The whole of response has been zeroed, so this will copy safely 
		responseLength = sizeof(response);
	}
	else
	{
		response.m_status = m_GitStatus;
		responseLength = sizeof(response.m_status);
	}

	// directories that are empty or only contain unversioned files will be git_wc_status_incomplete, report as unversioned
	if (response.m_status.text_status == git_wc_status_incomplete)
	{
		response.m_status.text_status = response.m_status.prop_status = git_wc_status_unversioned;
	}
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
		m_GitStatus.text_status = newStatus;
		m_GitStatus.prop_status = newStatus;
		m_discardAtTime = GetTickCount()+cachetimeout;
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
