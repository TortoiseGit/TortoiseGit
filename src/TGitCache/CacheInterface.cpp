// TortoiseGit - a Windows shell extension for easy version control

// External Cache Copyright (C) 2007,2010-2011 - TortoiseSVN
// Copyright (C) 2008-2011 - TortoiseGit

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
	CAutoGeneralHandle token;
	DWORD len;
	BOOL result = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, token.GetPointer());
	if(result)
	{
		GetTokenInformation(token, TokenStatistics, NULL, 0, &len);
		LPBYTE data = new BYTE[len];
		GetTokenInformation(token, TokenStatistics, data, len, &len);
		LUID uid = ((PTOKEN_STATISTICS)data)->AuthenticationId;
		delete [ ] data;
		CString t;
		t.Format(_T("-%08x%08x"), uid.HighPart, uid.LowPart);
		return t;
	}
	return _T("");
}

bool SendCacheCommand(BYTE command, const WCHAR * path /* = NULL */)
{
	int retrycount = 2;
	CAutoFile hPipe;
	do
	{
		hPipe = CreateFile(
			GetCacheCommandPipeName(),		// pipe name
			GENERIC_READ |					// read and write access
			GENERIC_WRITE,
			0,								// no sharing
			NULL,							// default security attributes
			OPEN_EXISTING,					// opens existing pipe
			FILE_FLAG_OVERLAPPED,			// default attributes
			NULL);							// no template file
		retrycount--;
		if (!hPipe)
			Sleep(10);
	} while ((!hPipe) && (retrycount));

	if (!hPipe)
	{
		//CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Could not connect to pipe\n");
		return false;
	}

	// The pipe connected; change to message-read mode.
	DWORD dwMode = PIPE_READMODE_MESSAGE;
	if (SetNamedPipeHandleState(
		hPipe,		// pipe handle
		&dwMode,	// new pipe mode
		NULL,		// don't set maximum bytes
		NULL))		// don't set maximum time
	{
		DWORD cbWritten;
		TGITCacheCommand cmd;
		SecureZeroMemory(&cmd, sizeof(TGITCacheCommand));
		cmd.command = command;
		if (path)
			_tcsncpy_s(cmd.path, path, _TRUNCATE);

		retrycount = 2;
		BOOL fSuccess = FALSE;
		do
		{
			fSuccess = WriteFile(
				hPipe,			// handle to pipe
				&cmd,			// buffer to write from
				sizeof(cmd),	// number of bytes to write
				&cbWritten,		// number of bytes written
				NULL);			// not overlapped I/O
			retrycount--;
			if (! fSuccess || sizeof(cmd) != cbWritten)
				Sleep(10);
		} while ((retrycount) && (! fSuccess || sizeof(cmd) != cbWritten));

		if (! fSuccess || sizeof(cmd) != cbWritten)
		{
			//CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Could not write to pipe\n");
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
			NULL);			// not overlapped I/O
		DisconnectNamedPipe(hPipe);
	}
	else
	{
		//CTraceToOutputDebugString::Instance()(__FUNCTION__ ": SetNamedPipeHandleState failed");
		return false;
	}

	return true;
}

CBlockCacheForPath::CBlockCacheForPath(const WCHAR * aPath)
{
	wcsncpy_s(path, aPath, MAX_PATH);
	path[MAX_PATH] = 0;

	SendCacheCommand (TGITCACHECOMMAND_BLOCK, path);
}

CBlockCacheForPath::~CBlockCacheForPath()
{
	SendCacheCommand (TGITCACHECOMMAND_UNBLOCK, path);
}
