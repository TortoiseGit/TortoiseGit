// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2012 - TortoiseGit
// Copyright (C) 2003-2008,2011 - TortoiseSVN

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
#include "Shellupdater.h"
#include "../TGitCache/CacheInterface.h"
#include "Registry.h"
#include "git.h"
#include "SmartHandle.h"

CShellUpdater::CShellUpdater(void)
{
	m_hInvalidationEvent = CreateEvent(NULL, FALSE, FALSE, _T("TortoiseGitCacheInvalidationEvent"));
}

CShellUpdater::~CShellUpdater(void)
{
	Flush();

	CloseHandle(m_hInvalidationEvent);
}

CShellUpdater& CShellUpdater::Instance()
{
	static CShellUpdater instance;
	return instance;
}

/** 
* Add a single path for updating.
* The update will happen at some suitable time in the future
*/
void CShellUpdater::AddPathForUpdate(const CTGitPath& path)
{
	// Tell the shell extension to purge its cache - we'll redo this when 
	// we actually do the shell-updates, but sometimes there's an earlier update, which
	// might benefit from cache invalidation
	SetEvent(m_hInvalidationEvent);

	m_pathsForUpdating.AddPath(path);
}
/** 
* Add a list of paths for updating.
* The update will happen when the list is destroyed, at the end of execution
*/
void CShellUpdater::AddPathsForUpdate(const CTGitPathList& pathList)
{
	for(int nPath=0; nPath < pathList.GetCount(); nPath++)
	{
		AddPathForUpdate(pathList[nPath]);
	}
}

void CShellUpdater::Flush()
{
	if(m_pathsForUpdating.GetCount() > 0)
	{
		ATLTRACE("Flushing shell update list\n");

		UpdateShell();
		m_pathsForUpdating.Clear();
	}
}

void CShellUpdater::UpdateShell()
{
	// Tell the shell extension to purge its cache
	ATLTRACE("Setting cache invalidation event %d\n", GetTickCount());
	SetEvent(m_hInvalidationEvent);

	// We use the SVN 'notify' call-back to add items to the list
	// Because this might call-back more than once per file (for example, when committing)
	// it's possible that there may be duplicates in the list.
	// There's no point asking the shell to do more than it has to, so we remove the duplicates before
	// passing the list on
	m_pathsForUpdating.RemoveDuplicates();

	// if we use the external cache, we tell the cache directly that something
	// has changed, without the detour via the shell.
	CAutoFile hPipe = CreateFile( 
		GetCacheCommandPipeName(),		// pipe name 
		GENERIC_READ |					// read and write access 
		GENERIC_WRITE, 
		0,								// no sharing 
		NULL,							// default security attributes
		OPEN_EXISTING,					// opens existing pipe 
		FILE_FLAG_OVERLAPPED,			// default attributes 
		NULL);							// no template file 


	if (!hPipe) 
	{
		// The pipe connected; change to message-read mode. 
		DWORD dwMode; 

		dwMode = PIPE_READMODE_MESSAGE; 
		if(SetNamedPipeHandleState( 
			hPipe,    // pipe handle 
			&dwMode,  // new pipe mode 
			NULL,     // don't set maximum bytes 
			NULL))    // don't set maximum time 
		{
			CTGitPath path;
			for(int nPath = 0; nPath < m_pathsForUpdating.GetCount(); nPath++)
			{
				path.SetFromWin(g_Git.m_CurrentDir+_T("\\")+m_pathsForUpdating[nPath].GetWinPathString());
				ATLTRACE(_T("Cache Item Update for %s (%d)\n"), path.GetWinPathString(), GetTickCount());
				if (!path.IsDirectory())
				{
					// send notifications to the shell for changed files - folders are updated by the cache itself.
					SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH | SHCNF_FLUSHNOWAIT, path.GetWinPath(), NULL);
				}
				DWORD cbWritten; 
				TGITCacheCommand cmd;
				cmd.command = TGITCACHECOMMAND_CRAWL;
				wcsncpy_s(cmd.path, MAX_PATH+1, path.GetDirectory().GetWinPath(), MAX_PATH);
				BOOL fSuccess = WriteFile( 
					hPipe,			// handle to pipe 
					&cmd,			// buffer to write from 
					sizeof(cmd),	// number of bytes to write 
					&cbWritten,		// number of bytes written 
					NULL);			// not overlapped I/O 

				if (! fSuccess || sizeof(cmd) != cbWritten)
				{
					DisconnectNamedPipe(hPipe); 
					return;
				}
			}
			if (!hPipe)
			{
				// now tell the cache we don't need it's command thread anymore
				DWORD cbWritten; 
				TGITCacheCommand cmd;
				cmd.command = TGITCACHECOMMAND_END;
				WriteFile( 
					hPipe,			// handle to pipe 
					&cmd,			// buffer to write from 
					sizeof(cmd),	// number of bytes to write 
					&cbWritten,		// number of bytes written 
					NULL);			// not overlapped I/O 
				DisconnectNamedPipe(hPipe); 
			}
		}
		else
		{
			ATLTRACE("SetNamedPipeHandleState failed"); 
		}
	}
}

bool CShellUpdater::RebuildIcons()
{
	const int BUFFER_SIZE = 1024;
	TCHAR *buf = NULL;
	HKEY hRegKey = 0;
	DWORD dwRegValue;
	DWORD dwRegValueTemp;
	DWORD dwSize;
	DWORD_PTR dwResult;
	LONG lRegResult;
	std::wstring sRegValueName;
	std::wstring sDefaultIconSize;
	int iDefaultIconSize;
	bool bResult = false;

	lRegResult = RegOpenKeyEx(HKEY_CURRENT_USER, _T("Control Panel\\Desktop\\WindowMetrics"),
		0, KEY_READ | KEY_WRITE, &hRegKey);
	if (lRegResult != ERROR_SUCCESS)
		goto Cleanup;

	buf = new TCHAR[BUFFER_SIZE];
	if(buf == NULL)
		goto Cleanup;

	// we're going to change the Shell Icon Size value
	sRegValueName = _T("Shell Icon Size");

	// Read registry value
	dwSize = BUFFER_SIZE;
	lRegResult = RegQueryValueEx(hRegKey, sRegValueName.c_str(), NULL, NULL, 
		(LPBYTE) buf, &dwSize);
	if (lRegResult != ERROR_FILE_NOT_FOUND)
	{
		// If registry key doesn't exist create it using system current setting
		iDefaultIconSize = ::GetSystemMetrics(SM_CXICON);
		if (0 == iDefaultIconSize)
			iDefaultIconSize = 32;
		_sntprintf_s(buf, BUFFER_SIZE, BUFFER_SIZE, _T("%d"), iDefaultIconSize); 
	}
	else if (lRegResult != ERROR_SUCCESS)
		goto Cleanup;

	// Change registry value
	dwRegValue = _ttoi(buf);
	dwRegValueTemp = dwRegValue-1;

	dwSize = _sntprintf_s(buf, BUFFER_SIZE, BUFFER_SIZE, _T("%d"), dwRegValueTemp) + sizeof(TCHAR); 
	lRegResult = RegSetValueEx(hRegKey, sRegValueName.c_str(), 0, REG_SZ, 
		(LPBYTE) buf, dwSize); 
	if (lRegResult != ERROR_SUCCESS)
		goto Cleanup;


	// Update all windows
	SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, SPI_SETNONCLIENTMETRICS, 
		0, SMTO_ABORTIFHUNG, 5000, &dwResult);

	// Reset registry value
	dwSize = _sntprintf_s(buf, BUFFER_SIZE, BUFFER_SIZE, _T("%d"), dwRegValue) + sizeof(TCHAR); 
	lRegResult = RegSetValueEx(hRegKey, sRegValueName.c_str(), 0, REG_SZ, 
		(LPBYTE) buf, dwSize); 
	if(lRegResult != ERROR_SUCCESS)
		goto Cleanup;

	// Update all windows
	SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, SPI_SETNONCLIENTMETRICS, 
		0, SMTO_ABORTIFHUNG, 5000, &dwResult);

	bResult = true;

Cleanup:
	if (hRegKey != 0)
	{
		RegCloseKey(hRegKey);
	}
	if (buf != NULL)
	{
		delete [] buf;
	}

	return bResult;

}
