// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2017, 2019 - TortoiseGit
// Copyright (C) 2003-2014, 2017 - TortoiseSVN

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
#include "RemoteCacheLink.h"
#include "ShellExt.h"
#include "../TGitCache/CacheInterface.h"
#include "TGitPath.h"
#include "PathUtils.h"
#include "CreateProcessHelper.h"

CRemoteCacheLink::CRemoteCacheLink(void)
{
	SecureZeroMemory(&m_Overlapped, sizeof(m_Overlapped));
	m_lastTimeout = 0;
}

CRemoteCacheLink::~CRemoteCacheLink(void)
{
	ClosePipe();
	CloseCommandPipe();
}

bool CRemoteCacheLink::InternalEnsurePipeOpen(CAutoFile& hPipe, const CString& pipeName, bool overlapped) const
{
	if (hPipe)
		return true;

	int tryleft = 2;

	while (!hPipe && tryleft--)
	{
		hPipe = CreateFile(
							pipeName,                       // pipe name
							GENERIC_READ |                  // read and write access
							GENERIC_WRITE,
							0,                              // no sharing
							nullptr,                        // default security attributes
							OPEN_EXISTING,                  // opens existing pipe
							overlapped ? FILE_FLAG_OVERLAPPED : 0, // default attributes
							nullptr);                       // no template file
		if ((!hPipe) && (GetLastError() == ERROR_PIPE_BUSY))
		{
			// TGitCache is running but is busy connecting a different client.
			// Do not give up immediately but wait for a few milliseconds until
			// the server has created the next pipe instance
			if (!WaitNamedPipe (pipeName, 50))
				continue;
		}
	}

	if (hPipe)
	{
		// The pipe connected; change to message-read mode.
		DWORD dwMode;

		dwMode = PIPE_READMODE_MESSAGE;
		if(!SetNamedPipeHandleState(
			hPipe,    // pipe handle
			&dwMode,  // new pipe mode
			nullptr,  // don't set maximum bytes
			nullptr)) // don't set maximum time
		{
			CTraceToOutputDebugString::Instance()(__FUNCTION__ ": SetNamedPipeHandleState failed");
			hPipe.CloseHandle();
		}
	}

	return hPipe;
}

bool CRemoteCacheLink::EnsurePipeOpen()
{
	AutoLocker lock(m_critSec);

	if (InternalEnsurePipeOpen(m_hPipe, GetCachePipeName(), true))
	{
		// create an unnamed (=local) manual reset event for use in the overlapped structure
		if (m_hEvent)
			return true;

		m_hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
		if (m_hEvent)
			return true;

		CTraceToOutputDebugString::Instance()(__FUNCTION__ ": CreateEvent failed");
		ClosePipe();
	}

	return false;
}

bool CRemoteCacheLink::EnsureCommandPipeOpen()
{
	return InternalEnsurePipeOpen(m_hCommandPipe, GetCacheCommandPipeName(), false);
}

void CRemoteCacheLink::ClosePipe()
{
	AutoLocker lock(m_critSec);

	m_hPipe.CloseHandle();
	m_hEvent.CloseHandle();
}

void CRemoteCacheLink::CloseCommandPipe()
{
	AutoLocker lock(m_critSec);

	if(m_hCommandPipe)
	{
		// now tell the cache we don't need it's command thread anymore
		DWORD cbWritten;
		TGITCacheCommand cmd = { 0 };
		cmd.command = TGITCACHECOMMAND_END;
		WriteFile(
			m_hCommandPipe,         // handle to pipe
			&cmd,           // buffer to write from
			sizeof(cmd),    // number of bytes to write
			&cbWritten,     // number of bytes written
			nullptr);       // not overlapped I/O
		DisconnectNamedPipe(m_hCommandPipe);
		m_hCommandPipe.CloseHandle();
	}
}

bool CRemoteCacheLink::GetStatusFromRemoteCache(const CTGitPath& Path, TGITCacheResponse* pReturnedStatus, bool bRecursive)
{
	if(!EnsurePipeOpen())
	{
		// We've failed to open the pipe - try and start the cache
		// but only if the last try to start the cache was a certain time
		// ago. If we just try over and over again without a small pause
		// in between, the explorer is rendered unusable!
		// Failing to start the cache can have different reasons: missing exe,
		// missing registry key, corrupt exe, ...
		if ((static_cast<LONGLONG>(GetTickCount64()) - m_lastTimeout) < 0)
			return false;
		// if we're in protected mode, don't try to start the cache: since we're
		// here, we know we can't access it anyway and starting a new process will
		// trigger a warning dialog in IE7+ on Vista - we don't want that.
		if (GetProcessIntegrityLevel() < SECURITY_MANDATORY_MEDIUM_RID)
			return false;

		if (!RunTGitCacheProcess())
			return false;

		// Wait for the cache to open
		LONGLONG endTime = static_cast<LONGLONG>(GetTickCount64()) + 1000;
		while(!EnsurePipeOpen())
		{
			if ((static_cast<LONGLONG>(GetTickCount64()) - endTime) > 0)
			{
				m_lastTimeout = static_cast<LONGLONG>(GetTickCount64()) + 10000;
				return false;
			}
		}
		m_lastTimeout = static_cast<LONGLONG>(GetTickCount64()) + 10000;
	}

	AutoLocker lock(m_critSec);

	DWORD nBytesRead;
	TGITCacheRequest request;
	request.flags = TGITCACHE_FLAGS_NONOTIFICATIONS;
	if(bRecursive)
		request.flags |= TGITCACHE_FLAGS_RECUSIVE_STATUS;
	wcsncpy_s(request.path, Path.GetWinPath(), _countof(request.path) - 1);
	SecureZeroMemory(&m_Overlapped, sizeof(OVERLAPPED));
	m_Overlapped.hEvent = m_hEvent;
	// Do the transaction in overlapped mode.
	// That way, if anything happens which might block this call
	// we still can get out of it. We NEVER MUST BLOCK THE SHELL!
	// A blocked shell is a very bad user impression, because users
	// who don't know why it's blocked might find the only solution
	// to such a problem is a reboot and therefore they might loose
	// valuable data.
	// One particular situation where the shell could hang is when
	// the cache crashes and our crash report dialog comes up.
	// Sure, it would be better to have no situations where the shell
	// even can get blocked, but the timeout of 10 seconds is long enough
	// so that users still recognize that something might be wrong and
	// report back to us so we can investigate further.

	BOOL fSuccess = TransactNamedPipe(m_hPipe,
		&request, sizeof(request),
		pReturnedStatus, sizeof(*pReturnedStatus),
		&nBytesRead, &m_Overlapped);

	if (!fSuccess)
	{
		if (GetLastError()!=ERROR_IO_PENDING)
		{
			//OutputDebugStringA("TortoiseShell: TransactNamedPipe failed\n");
			ClosePipe();
			return false;
		}

		// TransactNamedPipe is working in an overlapped operation.
		// Wait for it to finish
		DWORD dwWait = WaitForSingleObject(m_hEvent, 10000);
		if (dwWait == WAIT_OBJECT_0)
			fSuccess = GetOverlappedResult(m_hPipe, &m_Overlapped, &nBytesRead, FALSE);
		else
		{
			// the cache didn't respond!
			fSuccess = FALSE;
		}
	}

	if (fSuccess)
		return true;
	ClosePipe();
	return false;
}

bool CRemoteCacheLink::ReleaseLockForPath(const CTGitPath& path)
{
	AutoLocker lock(m_critSec);
	EnsureCommandPipeOpen();
	if (m_hCommandPipe)
	{
		DWORD cbWritten;
		TGITCacheCommand cmd = { 0 };
		cmd.command = TGITCACHECOMMAND_RELEASE;
		wcsncpy_s(cmd.path, path.GetDirectory().GetWinPath(), _countof(cmd.path) - 1);
		BOOL fSuccess = WriteFile(
			m_hCommandPipe, // handle to pipe
			&cmd,           // buffer to write from
			sizeof(cmd),    // number of bytes to write
			&cbWritten,     // number of bytes written
			nullptr);       // not overlapped I/O
		if (! fSuccess || sizeof(cmd) != cbWritten)
		{
			CloseCommandPipe();
			return false;
		}
		return true;
	}
	return false;
}

DWORD CRemoteCacheLink::GetProcessIntegrityLevel() const
{
	DWORD dwIntegrityLevel = SECURITY_MANDATORY_MEDIUM_RID;

	CAutoGeneralHandle hProcess = GetCurrentProcess();
	CAutoGeneralHandle hToken;
	if (OpenProcessToken(hProcess, TOKEN_QUERY |
		TOKEN_QUERY_SOURCE, hToken.GetPointer()))
	{
		// Get the Integrity level.
		DWORD dwLengthNeeded;
		if (!GetTokenInformation(hToken, TokenIntegrityLevel,
			nullptr, 0, &dwLengthNeeded))
		{
			DWORD dwError = GetLastError();
			if (dwError == ERROR_INSUFFICIENT_BUFFER)
			{
				auto pTIL = static_cast<PTOKEN_MANDATORY_LABEL>(LocalAlloc(0, dwLengthNeeded));
				if (pTIL)
				{
					if (GetTokenInformation(hToken, TokenIntegrityLevel,
						pTIL, dwLengthNeeded, &dwLengthNeeded))
					{
						dwIntegrityLevel = *GetSidSubAuthority(pTIL->Label.Sid,
							static_cast<DWORD>(*GetSidSubAuthorityCount(pTIL->Label.Sid) - 1));
					}
					LocalFree(pTIL);
				}
			}
		}
	}

	return dwIntegrityLevel;
}

bool CRemoteCacheLink::RunTGitCacheProcess()
{
	const CString sCachePath = GetTGitCachePath();
	if (!CCreateProcessHelper::CreateProcessDetached(sCachePath, static_cast<LPTSTR>(nullptr)))
	{
		// It's not appropriate to do a message box here, because there may be hundreds of calls
		CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Failed to start cache\n");
		return false;
	}
	return true;
}

CString CRemoteCacheLink::GetTGitCachePath() const
{
	CString sCachePath = CPathUtils::GetAppDirectory(g_hmodThisDll) + L"TGitCache.exe";
	return sCachePath;
}
