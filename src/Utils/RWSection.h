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
#pragma once

/**
 * \ingroup Utils
 * Implements a reader-writer lock.
 */
class CRWSection
{
public:
	CRWSection();
	~CRWSection();

	bool WaitToRead(DWORD waitTime = INFINITE);
	bool WaitToWrite(DWORD waitTime = INFINITE);
	void Done();
	bool IsWriter() {return ((m_nWaitingWriters > 0) || (m_nActive < 0));}
#if defined (DEBUG) || defined (_DEBUG)
	void AssertLock() {ATLASSERT(m_nActive);}
	void AssertWriting() {ATLASSERT((m_nWaitingWriters || (m_nActive < 0)));}
#else
	void AssertLock() {;}
	void AssertWriting() {;}
#endif
private:
	int					m_nWaitingReaders;	// Number of readers waiting for access
	int					m_nWaitingWriters;	// Number of writers waiting for access
	int					m_nActive;			// Number of threads accessing the section
	CRITICAL_SECTION	m_cs;
	HANDLE				m_hReaders;
	HANDLE				m_hWriters;
};
