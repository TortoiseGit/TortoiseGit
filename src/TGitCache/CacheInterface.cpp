// TortoiseGit - a Windows shell extension for easy version control

// External Cache Copyright (C) 2007,2009-2012 - TortoiseSVN
// Copyright (C) 2008-2013, 2016, 2019 - TortoiseGit

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
#include "CacheInterface.h"
#include "SmartHandle.h"
#include <memory>

CString GetCachePipeName()
{
	return TGIT_CACHE_PIPE_NAME + GetCacheID();
}

CString GetCacheCommandPipeName()
{
	return TGIT_CACHE_COMMANDPIPE_NAME + GetCacheID();
}

CString GetCacheMutexName()
{
	return TGIT_CACHE_MUTEX_NAME + GetCacheID();
}

CString GetCacheID()
{
	CString t;
	CAutoGeneralHandle token;
	BOOL result = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, token.GetPointer());
	if(result)
	{
		DWORD len = 0;
		GetTokenInformation(token, TokenStatistics, nullptr, 0, &len);
		if (len >= sizeof (TOKEN_STATISTICS))
		{
			auto data = std::make_unique<BYTE[]>(len);
			GetTokenInformation(token, TokenStatistics, data.get(), len, &len);
			LUID uid = reinterpret_cast<PTOKEN_STATISTICS>(data.get())->AuthenticationId;
			t.Format(L"-%08x%08x", uid.HighPart, uid.LowPart);
		}
	}
	return t;
}

bool SendCacheCommand(BYTE command, const WCHAR* path /* = nullptr */)
{
	CAutoFile hPipe;
	CString pipeName = GetCacheCommandPipeName();
	for (int retry = 0; retry < 2; ++retry)
	{
		if (retry > 0)
			WaitNamedPipe(pipeName, 50);

		hPipe = CreateFile(
			pipeName,						// pipe name
			GENERIC_READ |					// read and write access
			GENERIC_WRITE,
			0,								// no sharing
			nullptr,						// default security attributes
			OPEN_EXISTING,					// opens existing pipe
			FILE_FLAG_OVERLAPPED,			// default attributes
			nullptr);						// no template file

		if (!hPipe && GetLastError() == ERROR_PIPE_BUSY)
		{
			// TGitCache is running but is busy connecting a different client.
			// Do not give up immediately but wait for a few milliseconds until
			// the server has created the next pipe instance
			continue;
		}
		break;
	}

	if (!hPipe)
	{
		CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Could not connect to pipe\n");
		return false;
	}

	// The pipe connected; change to message-read mode.
	DWORD dwMode = PIPE_READMODE_MESSAGE;
	if (SetNamedPipeHandleState(
		hPipe,		// pipe handle
		&dwMode,	// new pipe mode
		nullptr,	// don't set maximum bytes
		nullptr))	// don't set maximum time
	{
		DWORD cbWritten;
		TGITCacheCommand cmd = { 0 };
		cmd.command = command;
		if (path)
			wcsncpy_s(cmd.path, path, _TRUNCATE);

		int retrycount = 2;
		BOOL fSuccess = FALSE;
		do
		{
			fSuccess = WriteFile(
				hPipe,			// handle to pipe
				&cmd,			// buffer to write from
				sizeof(cmd),	// number of bytes to write
				&cbWritten,		// number of bytes written
				nullptr);		// not overlapped I/O
			retrycount--;
			if (! fSuccess || sizeof(cmd) != cbWritten)
				Sleep(10);
		} while ((retrycount) && (! fSuccess || sizeof(cmd) != cbWritten));

		if (! fSuccess || sizeof(cmd) != cbWritten)
		{
			CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Could not write to pipe\n");
			DisconnectNamedPipe(hPipe);
			return false;
		}
		// now tell the cache we don't need it's command thread anymore
		SecureZeroMemory(&cmd, sizeof(TGITCacheCommand));
		cmd.command = TGITCACHECOMMAND_END;
		WriteFile(
			hPipe,			// handle to pipe
			&cmd,			// buffer to write from
			sizeof(cmd),	// number of bytes to write
			&cbWritten,		// number of bytes written
			nullptr);		// not overlapped I/O
		DisconnectNamedPipe(hPipe);
	}
	else
	{
		CTraceToOutputDebugString::Instance()(__FUNCTION__ ": SetNamedPipeHandleState failed");
		return false;
	}

	return true;
}

CBlockCacheForPath::CBlockCacheForPath(const WCHAR * aPath)
	: m_bBlocked(false)
{
	wcsncpy_s(path, aPath, _countof(path) - 1);

	if (!SendCacheCommand(TGITCACHECOMMAND_BLOCK, path))
		return;

	// Wait a short while to make sure the cache has
	// processed this command. Without this, we risk
	// executing the svn command before the cache has
	// blocked the path and already gets change notifications.
	Sleep(20);
	m_bBlocked = true;
}

CBlockCacheForPath::~CBlockCacheForPath()
{
	if (!m_bBlocked)
		return;

	for (int retry = 0; retry < 3; ++retry)
	{
		if (retry > 0)
			Sleep(10);

		if (SendCacheCommand(TGITCACHECOMMAND_UNBLOCK, path))
			break;
	}
}
