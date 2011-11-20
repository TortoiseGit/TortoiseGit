// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN

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
#include "RWSection.h"

CRWSection::CRWSection()
{
	m_nWaitingReaders = m_nWaitingWriters = m_nActive = 0;
	m_hReaders = CreateSemaphore(NULL, 0, MAXLONG, NULL);
	m_hWriters = CreateSemaphore(NULL, 0, MAXLONG, NULL);
	InitializeCriticalSection(&m_cs);
}

CRWSection::~CRWSection()
{
#ifdef _DEBUG
	if (m_nActive > 0)
		DebugBreak();
#endif

	m_nWaitingReaders = m_nWaitingWriters = m_nActive = 0;
	DeleteCriticalSection(&m_cs);
	CloseHandle(m_hWriters);
	CloseHandle(m_hReaders);
}

bool CRWSection::WaitToRead(DWORD waitTime)
{
	EnterCriticalSection(&m_cs);

	// Writers have priority
	BOOL fResourceWritePending = (m_nWaitingWriters || (m_nActive < 0));

	if (fResourceWritePending)
	{
		m_nWaitingReaders++;
	} 
	else 
	{
		m_nActive++;
	}

	LeaveCriticalSection(&m_cs);

	if (fResourceWritePending)
	{
		// wait until writer is finished
		if (WaitForSingleObject(m_hReaders, waitTime) != WAIT_OBJECT_0)
		{
			EnterCriticalSection(&m_cs);
			m_nWaitingReaders--;
			LeaveCriticalSection(&m_cs);
			return false;
		}
	}
	return true;
}

bool CRWSection::WaitToWrite(DWORD waitTime)
{
	EnterCriticalSection(&m_cs);

	BOOL fResourceOwned = (m_nActive != 0);

	if (fResourceOwned)
	{
		m_nWaitingWriters++;
	} 
	else 
	{
		m_nActive = -1;
	}

	LeaveCriticalSection(&m_cs);

	if (fResourceOwned)
	{
		if (WaitForSingleObject(m_hWriters, waitTime) != WAIT_OBJECT_0)
		{
			EnterCriticalSection(&m_cs);
			m_nWaitingWriters--;
			LeaveCriticalSection(&m_cs);
			return false;
		}
	}
	return true;
}

void CRWSection::Done()
{
	EnterCriticalSection(&m_cs);

	if (m_nActive > 0)
	{
		m_nActive--;
	}
	else
	{
		m_nActive++;
	}

	HANDLE hsem = NULL;
	LONG lCount = 1;

	if (m_nActive == 0)
	{
		// Note: If there are always writers waiting, then
		// it's possible that a reader never gets access
		// (reader starvation)
		if (m_nWaitingWriters > 0)
		{
			m_nActive = -1;
			m_nWaitingWriters--;
			hsem = m_hWriters;
		}
		else if (m_nWaitingReaders > 0)
		{
			m_nActive = m_nWaitingReaders;
			m_nWaitingReaders = 0;
			hsem = m_hReaders;
			lCount = m_nActive;
		}
		else
		{
			// no threads waiting, nothing to do
		}
	}

	LeaveCriticalSection(&m_cs);

	if (hsem != NULL)
	{
		ReleaseSemaphore(hsem, lCount, NULL);
	}
}

