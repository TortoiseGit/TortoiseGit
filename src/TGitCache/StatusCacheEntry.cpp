// TortoiseSVN - a Windows shell extension for easy version control

// External Cache Copyright (C) 2005-2006,2008 - TortoiseSVN

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
#include "SVNStatus.h"
#include "CacheInterface.h"
#include "registry.h"

DWORD cachetimeout = (DWORD)CRegStdWORD(_T("Software\\TortoiseSVN\\Cachetimeout"), CACHETIMEOUT);

CStatusCacheEntry::CStatusCacheEntry()
	: m_bSet(false)
	, m_bSVNEntryFieldSet(false)
	, m_kind(svn_node_unknown)
	, m_bReadOnly(false)
	, m_highestPriorityLocalStatus(svn_wc_status_none)
{
	SetAsUnversioned();
}

CStatusCacheEntry::CStatusCacheEntry(const svn_wc_status2_t* pSVNStatus, __int64 lastWriteTime, bool bReadOnly, DWORD validuntil /* = 0*/)
	: m_bSet(false)
	, m_bSVNEntryFieldSet(false)
	, m_kind(svn_node_unknown)
	, m_bReadOnly(false)
	, m_highestPriorityLocalStatus(svn_wc_status_none)
{
	SetStatus(pSVNStatus);
	m_lastWriteTime = lastWriteTime;
	if (validuntil)
		m_discardAtTime = validuntil;
	else
		m_discardAtTime = GetTickCount()+cachetimeout;
	m_bReadOnly = bReadOnly;
}

bool CStatusCacheEntry::SaveToDisk(FILE * pFile)
{
#define WRITEVALUETOFILE(x) if (fwrite(&x, sizeof(x), 1, pFile)!=1) return false;
#define WRITESTRINGTOFILE(x) if (x.IsEmpty()) {value=0;WRITEVALUETOFILE(value);}else{value=x.GetLength();WRITEVALUETOFILE(value);if (fwrite((LPCSTR)x, sizeof(char), value, pFile)!=value) return false;}

	unsigned int value = 4;
	WRITEVALUETOFILE(value); // 'version' of this save-format
	WRITEVALUETOFILE(m_highestPriorityLocalStatus);
	WRITEVALUETOFILE(m_lastWriteTime);
	WRITEVALUETOFILE(m_bSet);
	WRITEVALUETOFILE(m_bSVNEntryFieldSet);
	WRITEVALUETOFILE(m_commitRevision);
	WRITESTRINGTOFILE(m_sUrl);
	WRITESTRINGTOFILE(m_sOwner);
	WRITESTRINGTOFILE(m_sAuthor);
	WRITEVALUETOFILE(m_kind);
	WRITEVALUETOFILE(m_bReadOnly);
	WRITESTRINGTOFILE(m_sPresentProps);

	// now save the status struct (without the entry field, because we don't use that)
	WRITEVALUETOFILE(m_svnStatus.copied);
	WRITEVALUETOFILE(m_svnStatus.locked);
	WRITEVALUETOFILE(m_svnStatus.prop_status);
	WRITEVALUETOFILE(m_svnStatus.repos_prop_status);
	WRITEVALUETOFILE(m_svnStatus.repos_text_status);
	WRITEVALUETOFILE(m_svnStatus.switched);
	WRITEVALUETOFILE(m_svnStatus.text_status);
	return true;
}

bool CStatusCacheEntry::LoadFromDisk(FILE * pFile)
{
#define LOADVALUEFROMFILE(x) if (fread(&x, sizeof(x), 1, pFile)!=1) return false;
	try
	{
		unsigned int value = 0;
		LOADVALUEFROMFILE(value);
		if (value != 4)
			return false;		// not the correct version
		LOADVALUEFROMFILE(m_highestPriorityLocalStatus);
		LOADVALUEFROMFILE(m_lastWriteTime);
		LOADVALUEFROMFILE(m_bSet);
		LOADVALUEFROMFILE(m_bSVNEntryFieldSet);
		LOADVALUEFROMFILE(m_commitRevision);
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
			if (fread(m_sOwner.GetBuffer(value+1), sizeof(char), value, pFile)!=value)
			{
				m_sOwner.ReleaseBuffer(0);
				return false;
			}
			m_sOwner.ReleaseBuffer(value);
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
		LOADVALUEFROMFILE(m_bReadOnly);
		LOADVALUEFROMFILE(value);
		if (value != 0)
		{
			if (fread(m_sPresentProps.GetBuffer(value+1), sizeof(char), value, pFile)!=value)
			{
				m_sPresentProps.ReleaseBuffer(0);
				return false;
			}
			m_sPresentProps.ReleaseBuffer(value);
		}
		SecureZeroMemory(&m_svnStatus, sizeof(m_svnStatus));
		LOADVALUEFROMFILE(m_svnStatus.copied);
		LOADVALUEFROMFILE(m_svnStatus.locked);
		LOADVALUEFROMFILE(m_svnStatus.prop_status);
		LOADVALUEFROMFILE(m_svnStatus.repos_prop_status);
		LOADVALUEFROMFILE(m_svnStatus.repos_text_status);
		LOADVALUEFROMFILE(m_svnStatus.switched);
		LOADVALUEFROMFILE(m_svnStatus.text_status);
		m_svnStatus.entry = NULL;
		m_discardAtTime = GetTickCount()+cachetimeout;
	}
	catch ( CAtlException )
	{
		return false;
	}
	return true;
}

void CStatusCacheEntry::SetStatus(const svn_wc_status2_t* pSVNStatus)
{
	if(pSVNStatus == NULL)
	{
		SetAsUnversioned();
	}
	else
	{
		m_highestPriorityLocalStatus = SVNStatus::GetMoreImportant(pSVNStatus->prop_status, pSVNStatus->text_status);
		m_svnStatus = *pSVNStatus;

		// Currently we don't deep-copy the whole entry value, but we do take a few members
        if(pSVNStatus->entry != NULL)
		{
			m_sUrl = pSVNStatus->entry->url;
			m_commitRevision = pSVNStatus->entry->cmt_rev;
			m_bSVNEntryFieldSet = true;
			m_sOwner = pSVNStatus->entry->lock_owner;
			m_kind = pSVNStatus->entry->kind;
			m_sAuthor = pSVNStatus->entry->cmt_author;
			if (pSVNStatus->entry->present_props)
				m_sPresentProps = pSVNStatus->entry->present_props;
		}
		else
		{
			m_sUrl.Empty();
			m_commitRevision = 0;
			m_bSVNEntryFieldSet = false;
		}
		m_svnStatus.entry = NULL;
	}
	m_discardAtTime = GetTickCount()+cachetimeout;
	m_bSet = true;
}


void CStatusCacheEntry::SetAsUnversioned()
{
	SecureZeroMemory(&m_svnStatus, sizeof(m_svnStatus));
	m_discardAtTime = GetTickCount()+cachetimeout;
	svn_wc_status_kind status = svn_wc_status_none;
	if (m_highestPriorityLocalStatus == svn_wc_status_ignored)
		status = svn_wc_status_ignored;
	if (m_highestPriorityLocalStatus == svn_wc_status_unversioned)
		status = svn_wc_status_unversioned;
	m_highestPriorityLocalStatus = status;
	m_svnStatus.prop_status = svn_wc_status_none;
	m_svnStatus.text_status = status;
	m_lastWriteTime = 0;
}

bool CStatusCacheEntry::HasExpired(long now) const
{
	return m_discardAtTime != 0 && (now - m_discardAtTime) >= 0;
}

void CStatusCacheEntry::BuildCacheResponse(TSVNCacheResponse& response, DWORD& responseLength) const
{
	SecureZeroMemory(&response, sizeof(response));
	if(m_bSVNEntryFieldSet)
	{
		response.m_status = m_svnStatus;
		response.m_entry.cmt_rev = m_commitRevision;

		// There is no point trying to set these pointers here, because this is not 
		// the process which will be using the data.
		// The process which receives this response (generally the TSVN Shell Extension)
		// must fix-up these pointers when it gets them
		response.m_status.entry = NULL;
		response.m_entry.url = NULL;

		response.m_kind = m_kind;
		response.m_readonly = m_bReadOnly;

		if (m_sPresentProps.Find("svn:needs-lock")>=0)
		{
			response.m_needslock = true;
		}
		else
			response.m_needslock = false;
		// The whole of response has been zeroed, so this will copy safely 
		strncat_s(response.m_url, INTERNET_MAX_URL_LENGTH, m_sUrl, _TRUNCATE);
		strncat_s(response.m_owner, 255, m_sOwner, _TRUNCATE);
		strncat_s(response.m_author, 255, m_sAuthor, _TRUNCATE);
		responseLength = sizeof(response);
	}
	else
	{
		response.m_status = m_svnStatus;
		responseLength = sizeof(response.m_status);
	}
}

bool CStatusCacheEntry::IsVersioned() const
{
	return m_highestPriorityLocalStatus > svn_wc_status_unversioned;
}

bool CStatusCacheEntry::DoesFileTimeMatch(__int64 testTime) const
{
	return m_lastWriteTime == testTime;
}


bool CStatusCacheEntry::ForceStatus(svn_wc_status_kind forcedStatus)
{
	svn_wc_status_kind newStatus = forcedStatus; 

	if(newStatus != m_highestPriorityLocalStatus)
	{
		// We've had a status change
		m_highestPriorityLocalStatus = newStatus;
		m_svnStatus.text_status = newStatus;
		m_svnStatus.prop_status = newStatus;
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
