// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009, 2011, 2014, 2017, 2019 - TortoiseGit
// Copyright (C) 2003-2011, 2014, 2017 - TortoiseSVN

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
#include "SmartHandle.h"

struct TGITCacheResponse;
class CTGitPath;

#include "GitStatus.h"

/**
 * \ingroup TortoiseShell
 * This class provides the link to the cache process.
 */
class CRemoteCacheLink
{
public:
	CRemoteCacheLink(void);
	~CRemoteCacheLink(void);

public:
	bool GetStatusFromRemoteCache(const CTGitPath& Path, TGITCacheResponse* pReturnedStatus, bool bRecursive);
	bool ReleaseLockForPath(const CTGitPath& path);

private:
	bool InternalEnsurePipeOpen(CAutoFile& hPipe, const CString& pipeName, bool overlapped) const;

	bool EnsurePipeOpen();
	void ClosePipe();

	bool EnsureCommandPipeOpen();
	void CloseCommandPipe();

	DWORD GetProcessIntegrityLevel() const;
	bool RunTGitCacheProcess();
	CString GetTGitCachePath() const;

private:
	CAutoFile m_hPipe;
	OVERLAPPED m_Overlapped;
	CAutoGeneralHandle m_hEvent;

	CAutoFile m_hCommandPipe;


	CComAutoCriticalSection m_critSec;
	LONGLONG m_lastTimeout;
};
