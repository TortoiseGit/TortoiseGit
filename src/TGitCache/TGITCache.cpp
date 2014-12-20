// TortoiseGit - a Windows shell extension for easy version control

// External Cache Copyright (C) 2005 - 2006,2010 - Will Dean, Stefan Kueng
// Copyright (C) 2008-2014 - TortoiseGit

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
#include <ShellAPI.h>
#include "TGITCache.h"
#include "GitStatusCache.h"
#include "CacheInterface.h"
#include "Resource.h"
#include "registry.h"
#include "CrashReport.h"
#include "GitAdminDir.h"
#include <Dbt.h>
#include <InitGuid.h>
#include <Ioevent.h>
#include "..\version.h"
#include "SmartHandle.h"
#include "DllVersion.h"
#include "CreateProcessHelper.h"
#include "gitindex.h"

#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp)                        ((int)(short)LOWORD(lp))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lp)                        ((int)(short)HIWORD(lp))
#endif


#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#if ENABLE_CRASHHANLDER
CCrashReportTGit crasher(L"TGitCache " _T(APP_X64_STRING), TGIT_VERMAJOR, TGIT_VERMINOR, TGIT_VERMICRO, TGIT_VERBUILD, TGIT_VERDATE);
#endif

DWORD WINAPI 		InstanceThread(LPVOID);
DWORD WINAPI		PipeThread(LPVOID);
DWORD WINAPI		CommandWaitThread(LPVOID);
DWORD WINAPI		CommandThread(LPVOID);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
bool				bRun = true;
bool				bRestart = false;
NOTIFYICONDATA		niData;
HWND				hWnd;
HWND				hTrayWnd;
TCHAR				szCurrentCrawledPath[MAX_CRAWLEDPATHS][MAX_CRAWLEDPATHSLEN];
int					nCurrentCrawledpathIndex = 0;
CComAutoCriticalSection critSec;

// must put this before any global variables that auto free git objects,
// so this destructor is called after freeing git objects
class CGit2InitClass
{
public:
	~CGit2InitClass()
	{
		git_libgit2_shutdown();
	}
} git2init;

CGitIndexFileMap g_IndexFileMap;

volatile LONG		nThreadCount = 0;

#define PACKVERSION(major,minor) MAKELONG(minor,major)

void DebugOutputLastError()
{
	LPVOID lpMsgBuf;
	if (!FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL ))
	{
		return;
	}

	// Display the string.
	OutputDebugStringA("TGitCache GetLastError(): ");
	OutputDebugString((LPCTSTR)lpMsgBuf);
	OutputDebugStringA("\n");

	// Free the buffer.
	LocalFree( lpMsgBuf );
}

void HandleCommandLine(LPSTR lpCmdLine)
{
	char *ptr = strstr(lpCmdLine, "/kill:");
	if (ptr)
	{
		DWORD pid = (DWORD)atoi(ptr + strlen("/kill:"));
		HANDLE hProcess = ::OpenProcess(PROCESS_TERMINATE, FALSE, pid);
		if (hProcess)
		{
			if (::WaitForSingleObject(hProcess, 5000) != WAIT_OBJECT_0)
			{
				CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Killing previous TGitCache PID %d\n", pid);
				if (!::TerminateProcess(hProcess, (UINT)-1))
					CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Kill previous TGitCache PID %d failed\n", pid);
				::WaitForSingleObject(hProcess, 5000);
			}
			::CloseHandle(hProcess);
			for (int i = 0; i < 5; i++)
			{
				HANDLE hMutex = ::OpenMutex(MUTEX_ALL_ACCESS, FALSE, GetCacheMutexName());
				if (!hMutex)
					break;
				::CloseHandle(hMutex);
				::Sleep(1000);
			}
		}
	}
}

void HandleRestart()
{
	if (bRestart)
	{
		TCHAR exeName[MAX_PATH] = { 0 };
		::GetModuleFileName(nullptr, exeName, _countof(exeName));
		TCHAR cmdLine[20] = { 0 };
		_stprintf_s(cmdLine, _T(" /kill:%d"), GetCurrentProcessId());
		if (!CCreateProcessHelper::CreateProcessDetached(exeName, cmdLine))
			CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Failed to start cache\n");
	}
}

int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPSTR lpCmdLine, int /*cmdShow*/)
{
	SetDllDirectory(L"");
	git_libgit2_init();
	HandleCommandLine(lpCmdLine);
	CAutoGeneralHandle hReloadProtection = ::CreateMutex(NULL, FALSE, GetCacheMutexName());

	if ((!hReloadProtection) || (GetLastError() == ERROR_ALREADY_EXISTS))
	{
		// An instance of TGitCache is already running
		CTraceToOutputDebugString::Instance()(__FUNCTION__ ": TGitCache ignoring restart\n");
		return 0;
	}

	CGitStatusCache::Create();
	CGitStatusCache::Instance().Init();

	SecureZeroMemory(szCurrentCrawledPath, sizeof(szCurrentCrawledPath));

	DWORD dwThreadId;
	MSG msg;
	TCHAR szWindowClass[] = {TGIT_CACHE_WINDOW_NAME};

	// create a hidden window to receive window messages.
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= 0;
	wcex.hCursor		= 0;
	wcex.hbrBackground	= 0;
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= 0;
	RegisterClassEx(&wcex);
	hWnd = CreateWindow(TGIT_CACHE_WINDOW_NAME, TGIT_CACHE_WINDOW_NAME, WS_CAPTION, 0, 0, 800, 300, NULL, 0, hInstance, 0);
	hTrayWnd = hWnd;
	if (hWnd == NULL)
	{
		return 0;
	}
	if (CRegStdDWORD(_T("Software\\TortoiseGit\\CacheTrayIcon"), FALSE)==TRUE)
	{
		SecureZeroMemory(&niData,sizeof(NOTIFYICONDATA));

		DWORD dwMajor = 0;
		DWORD dwMinor = 0;
		GetShellVersion(&dwMajor, &dwMinor);
		DWORD dwVersion = PACKVERSION(dwMajor, dwMinor);
		if (dwVersion >= PACKVERSION(6,0))
			niData.cbSize = sizeof(NOTIFYICONDATA);
		else if (dwVersion >= PACKVERSION(5,0))
			niData.cbSize = NOTIFYICONDATA_V2_SIZE;
		else
			niData.cbSize = NOTIFYICONDATA_V1_SIZE;

		niData.uID = TRAY_ID;		// own tray icon ID
		niData.hWnd	 = hWnd;
		niData.uFlags = NIF_ICON|NIF_MESSAGE;

		// load the icon
		niData.hIcon =
			(HICON)LoadImage(hInstance,
			MAKEINTRESOURCE(IDI_TGITCACHE),
			IMAGE_ICON,
			GetSystemMetrics(SM_CXSMICON),
			GetSystemMetrics(SM_CYSMICON),
			LR_DEFAULTCOLOR);

		// set the message to send
		// note: the message value should be in the
		// range of WM_APP through 0xBFFF
		niData.uCallbackMessage = TRAY_CALLBACK;
		Shell_NotifyIcon(NIM_ADD,&niData);
		// free icon handle
		if(niData.hIcon && DestroyIcon(niData.hIcon))
			niData.hIcon = NULL;
	}

	// Create a thread which waits for incoming pipe connections
	CAutoGeneralHandle hPipeThread = CreateThread(
		NULL,              // no security attribute
		0,                 // default stack size
		PipeThread,
		(LPVOID) &bRun,    // thread parameter
		0,                 // not suspended
		&dwThreadId);      // returns thread ID

	if (!hPipeThread)
	{
		return 0;
	}
	else hPipeThread.CloseHandle();

	// Create a thread which waits for incoming pipe connections
	CAutoGeneralHandle hCommandWaitThread = CreateThread(
		NULL,              // no security attribute
		0,                 // default stack size
		CommandWaitThread,
		(LPVOID) &bRun,    // thread parameter
		0,                 // not suspended
		&dwThreadId);      // returns thread ID

	if (!hCommandWaitThread)
	{
		return 0;
	}


	// loop to handle window messages.
	while (bRun)
	{
		BOOL bLoopRet = GetMessage(&msg, NULL, 0, 0);
		if ((bLoopRet != -1)&&(bLoopRet != 0))
		{
			DispatchMessage(&msg);
		}
	}

	bRun = false;

	Shell_NotifyIcon(NIM_DELETE,&niData);
	CGitStatusCache::Destroy();
	HandleRestart();
	return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case TRAY_CALLBACK:
	{
		switch(lParam)
		{
		case WM_LBUTTONDBLCLK:
			if (IsWindowVisible(hWnd))
				ShowWindow(hWnd, SW_HIDE);
			else
				ShowWindow(hWnd, SW_RESTORE);
			break;
		case WM_MOUSEMOVE:
			{
				CString sInfoTip;
				NOTIFYICONDATA SystemTray;
				sInfoTip.Format(_T("TortoiseGit Overlay Icon Server\nCached Directories: %Id\nWatched paths: %d"),
					CGitStatusCache::Instance().GetCacheSize(),
					CGitStatusCache::Instance().GetNumberOfWatchedPaths());

				SystemTray.cbSize = sizeof(NOTIFYICONDATA);
				SystemTray.hWnd   = hTrayWnd;
				SystemTray.uID    = TRAY_ID;
				SystemTray.uFlags = NIF_TIP;
				_tcscpy_s(SystemTray.szTip, sInfoTip);
				Shell_NotifyIcon(NIM_MODIFY, &SystemTray);
			}
			break;
		case WM_RBUTTONUP:
		case WM_CONTEXTMENU:
			{
				POINT pt;
				GetCursorPos(&pt);
				HMENU hMenu = CreatePopupMenu();
				if(hMenu)
				{
					bool enabled = (DWORD)CRegStdDWORD(_T("Software\\TortoiseGit\\CacheType"), GetSystemMetrics(SM_REMOTESESSION) ? ShellCache::dll : ShellCache::exe) != ShellCache::none;
					InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, TRAYPOP_ENABLE, enabled ? _T("Disable Status Cache") : _T("Enable Status Cache"));
					InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, TRAYPOP_EXIT, _T("Exit"));
					SetForegroundWindow(hWnd);
					TrackPopupMenu(hMenu, TPM_BOTTOMALIGN, pt.x, pt.y, 0, hWnd, NULL);
					DestroyMenu(hMenu);
				}
			}
			break;
		}
	}
	break;
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			RECT rect;
			GetClientRect(hWnd, &rect);
			// clear the background
			HBRUSH background = CreateSolidBrush(::GetSysColor(COLOR_WINDOW));
			HGDIOBJ oldbrush = SelectObject(hdc, background);
			FillRect(hdc, &rect, background);

			int line = 0;
			SIZE fontsize = {0};
			AutoLocker print(critSec);
			GetTextExtentPoint32( hdc, szCurrentCrawledPath[0], (int)_tcslen(szCurrentCrawledPath[0]), &fontsize );
			for (int i=nCurrentCrawledpathIndex; i<MAX_CRAWLEDPATHS; ++i)
			{
				TextOut(hdc, 0, line*fontsize.cy, szCurrentCrawledPath[i], (int)_tcslen(szCurrentCrawledPath[i]));
				++line;
			}
			for (int i=0; i<nCurrentCrawledpathIndex; ++i)
			{
				TextOut(hdc, 0, line*fontsize.cy, szCurrentCrawledPath[i], (int)_tcslen(szCurrentCrawledPath[i]));
				++line;
			}

			SelectObject(hdc,oldbrush);
			EndPaint(hWnd, &ps);
			DeleteObject(background);
			return 0L;
		}
		break;
	case WM_COMMAND:
		{
			WORD wmId    = LOWORD(wParam);

			switch (wmId)
			{
			case TRAYPOP_ENABLE:
				{
					CRegStdDWORD reg = CRegStdDWORD(_T("Software\\TortoiseGit\\CacheType"), GetSystemMetrics(SM_REMOTESESSION) ? ShellCache::dll : ShellCache::exe);
					bool enabled = (DWORD)reg != ShellCache::none;
					reg = enabled ? ShellCache::none : ShellCache::exe;
					if (enabled)
					{
						bRestart = true;
						DestroyWindow(hWnd);
					}
					break;
				}
			case TRAYPOP_EXIT:
				DestroyWindow(hWnd);
				break;
			}
			return 1;
		}
	case WM_QUERYENDSESSION:
		{
			CTraceToOutputDebugString::Instance()(__FUNCTION__ ": WM_QUERYENDSESSION\n");
			CAutoWriteWeakLock writeLock(CGitStatusCache::Instance().GetGuard(), 200);
			CGitStatusCache::Instance().Stop();
			return TRUE;
		}
		break;
	case WM_CLOSE:
	case WM_ENDSESSION:
	case WM_DESTROY:
	case WM_QUIT:
		{
			CTraceToOutputDebugString::Instance()(__FUNCTION__ ": WM_CLOSE/DESTROY/ENDSESSION/QUIT\n");
			CAutoWriteLock writeLock(CGitStatusCache::Instance().GetGuard());
			CGitStatusCache::Instance().Stop();
			CGitStatusCache::Instance().SaveCache();
			if (message != WM_QUIT)
				PostQuitMessage(0);
			bRun = false;
			return 1;
		}
		break;
	case WM_DEVICECHANGE:
		{
			DEV_BROADCAST_HDR * phdr = (DEV_BROADCAST_HDR*)lParam;
			switch (wParam)
			{
			case DBT_CUSTOMEVENT:
				{
					CTraceToOutputDebugString::Instance()(__FUNCTION__ ": WM_DEVICECHANGE with DBT_CUSTOMEVENT\n");
					if (phdr->dbch_devicetype == DBT_DEVTYP_HANDLE)
					{
						DEV_BROADCAST_HANDLE * phandle = (DEV_BROADCAST_HANDLE*)lParam;
						if (IsEqualGUID(phandle->dbch_eventguid, GUID_IO_VOLUME_DISMOUNT))
						{
							CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Device to be dismounted\n");
							CAutoWriteLock writeLock(CGitStatusCache::Instance().GetGuard());
							CGitStatusCache::Instance().CloseWatcherHandles(phandle->dbch_handle);
						}
						if (IsEqualGUID(phandle->dbch_eventguid, GUID_IO_VOLUME_LOCK))
						{
							CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Device lock event\n");
							CAutoWriteLock writeLock(CGitStatusCache::Instance().GetGuard());
							CGitStatusCache::Instance().CloseWatcherHandles(phandle->dbch_handle);
						}
					}
				}
				break;
			case DBT_DEVICEREMOVEPENDING:
			case DBT_DEVICEQUERYREMOVE:
			case DBT_DEVICEREMOVECOMPLETE:
				CTraceToOutputDebugString::Instance()(__FUNCTION__ ": WM_DEVICECHANGE with DBT_DEVICEREMOVEPENDING/DBT_DEVICEQUERYREMOVE/DBT_DEVICEREMOVECOMPLETE\n");
				if (phdr->dbch_devicetype == DBT_DEVTYP_HANDLE)
				{
					DEV_BROADCAST_HANDLE * phandle = (DEV_BROADCAST_HANDLE*)lParam;
					CAutoWriteLock writeLock(CGitStatusCache::Instance().GetGuard());
					CGitStatusCache::Instance().CloseWatcherHandles(phandle->dbch_handle);
				}
				else if (phdr->dbch_devicetype == DBT_DEVTYP_VOLUME)
				{
					DEV_BROADCAST_VOLUME * pVolume = (DEV_BROADCAST_VOLUME*)lParam;
					CAutoWriteLock writeLock(CGitStatusCache::Instance().GetGuard());
					for (BYTE i = 0; i < 26; ++i)
					{
						if (pVolume->dbcv_unitmask & (1 << i))
						{
							TCHAR driveletter = 'A' + i;
							CString drive = CString(driveletter);
							drive += L":\\";
							CGitStatusCache::Instance().CloseWatcherHandles(CTGitPath(drive));
						}
					}
				}
				else
				{
					CAutoWriteLock writeLock(CGitStatusCache::Instance().GetGuard());
					CGitStatusCache::Instance().CloseWatcherHandles(INVALID_HANDLE_VALUE);
				}
				break;
			}
		}
		break;
	default:
		break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

//////////////////////////////////////////////////////////////////////////

VOID GetAnswerToRequest(const TGITCacheRequest* pRequest, TGITCacheResponse* pReply, DWORD* pResponseLength)
{
	CTGitPath path;
	*pResponseLength = 0;
	if(pRequest->flags & TGITCACHE_FLAGS_FOLDERISKNOWN)
	{
		path.SetFromWin(pRequest->path, !!(pRequest->flags & TGITCACHE_FLAGS_ISFOLDER));
	}
	else
	{
		path.SetFromWin(pRequest->path);
	}

	CAutoReadWeakLock readLock(CGitStatusCache::Instance().GetGuard(), 2000);
	if (readLock.IsAcquired())
	{
		CGitStatusCache::Instance().GetStatusForPath(path, pRequest->flags, false).BuildCacheResponse(*pReply, *pResponseLength);
	}
	else
	{
		CStatusCacheEntry entry;
		entry.BuildCacheResponse(*pReply, *pResponseLength);
	}
}

DWORD WINAPI PipeThread(LPVOID lpvParam)
{
	CTraceToOutputDebugString::Instance()(__FUNCTION__ ": PipeThread started\n");
	bool * bRun = (bool *)lpvParam;
	// The main loop creates an instance of the named pipe and
	// then waits for a client to connect to it. When the client
	// connects, a thread is created to handle communications
	// with that client, and the loop is repeated.
	DWORD dwThreadId;
	BOOL fConnected;
	CAutoFile hPipe;

	while (*bRun)
	{
		hPipe = CreateNamedPipe(
			GetCachePipeName(),
			PIPE_ACCESS_DUPLEX,       // read/write access
			PIPE_TYPE_MESSAGE |       // message type pipe
			PIPE_READMODE_MESSAGE |   // message-read mode
			PIPE_WAIT,                // blocking mode
			PIPE_UNLIMITED_INSTANCES, // max. instances
			BUFSIZE,                  // output buffer size
			BUFSIZE,                  // input buffer size
			NMPWAIT_USE_DEFAULT_WAIT, // client time-out
			NULL);					  // NULL DACL

		if (!hPipe)
		{
			if (*bRun)
				Sleep(200);
			continue; // never leave the thread!
		}

		// Wait for the client to connect; if it succeeds,
		// the function returns a nonzero value. If the function returns
		// zero, GetLastError returns ERROR_PIPE_CONNECTED.
		fConnected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
		if (fConnected)
		{
			// Create a thread for this client.
			CAutoGeneralHandle hInstanceThread = CreateThread(
				NULL,              // no security attribute
				0,                 // default stack size
				InstanceThread,
				(HANDLE) hPipe,    // thread parameter
				0,                 // not suspended
				&dwThreadId);      // returns thread ID

			if (!hInstanceThread)
			{
				DisconnectNamedPipe(hPipe);
				// since we're now closing this thread, we also have to close the whole application!
				// otherwise the thread is dead, but the app is still running, refusing new instances
				// but no pipe will be available anymore.
				PostMessage(hWnd, WM_CLOSE, 0, 0);
				return 1;
			}
			// detach the handle, since we passed it to the thread
			hPipe.Detach();
		}
		else
		{
			// The client could not connect, so close the pipe.
			hPipe.CloseHandle();
			if (*bRun)
				Sleep(200);
			continue;	// don't end the thread!
		}
	}
	CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Pipe thread exited\n");
	return 0;
}

DWORD WINAPI CommandWaitThread(LPVOID lpvParam)
{
	CTraceToOutputDebugString::Instance()(__FUNCTION__ ": CommandWaitThread started\n");
	bool * bRun = (bool *)lpvParam;
	// The main loop creates an instance of the named pipe and
	// then waits for a client to connect to it. When the client
	// connects, a thread is created to handle communications
	// with that client, and the loop is repeated.
	DWORD dwThreadId;
	BOOL fConnected;
	CAutoFile hPipe;

	while (*bRun)
	{
		hPipe = CreateNamedPipe(
			GetCacheCommandPipeName(),
			PIPE_ACCESS_DUPLEX,       // read/write access
			PIPE_TYPE_MESSAGE |       // message type pipe
			PIPE_READMODE_MESSAGE |   // message-read mode
			PIPE_WAIT,                // blocking mode
			PIPE_UNLIMITED_INSTANCES, // max. instances
			BUFSIZE,                  // output buffer size
			BUFSIZE,                  // input buffer size
			NMPWAIT_USE_DEFAULT_WAIT, // client time-out
			NULL);                // NULL DACL

		if (!hPipe)
		{
			if (*bRun)
				Sleep(200);
			continue; // never leave the thread!
		}

		// Wait for the client to connect; if it succeeds,
		// the function returns a nonzero value. If the function returns
		// zero, GetLastError returns ERROR_PIPE_CONNECTED.
		fConnected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
		if (fConnected)
		{
			// Create a thread for this client.
			CAutoGeneralHandle hCommandThread = CreateThread(
				NULL,              // no security attribute
				0,                 // default stack size
				CommandThread,
				(HANDLE) hPipe,    // thread parameter
				0,                 // not suspended
				&dwThreadId);      // returns thread ID

			if (!hCommandThread)
			{
				DisconnectNamedPipe(hPipe);
				hPipe.CloseHandle();
				// since we're now closing this thread, we also have to close the whole application!
				// otherwise the thread is dead, but the app is still running, refusing new instances
				// but no pipe will be available anymore.
				PostMessage(hWnd, WM_CLOSE, 0, 0);
				return 1;
			}
			// detach the handle, since we passed it to the thread
			hPipe.Detach();
		}
		else
		{
			// The client could not connect, so close the pipe.
			hPipe.CloseHandle();
			if (*bRun)
				Sleep(200);
			continue;	// don't end the thread!
		}
	}
	CTraceToOutputDebugString::Instance()(__FUNCTION__ ": CommandWait thread exited\n");
	return 0;
}

DWORD WINAPI InstanceThread(LPVOID lpvParam)
{
	CTraceToOutputDebugString::Instance()(__FUNCTION__ ": InstanceThread started\n");
	TGITCacheResponse response;
	DWORD cbBytesRead, cbWritten;
	CAutoFile hPipe;

	// The thread's parameter is a handle to a pipe instance.

	hPipe = lpvParam;
	InterlockedIncrement(&nThreadCount);
	while (bRun)
	{
		// Read client requests from the pipe.
		TGITCacheRequest request;
		BOOL fSuccess = ReadFile(
			hPipe,        // handle to pipe
			&request,    // buffer to receive data
			sizeof(request), // size of buffer
			&cbBytesRead, // number of bytes read
			NULL);        // not overlapped I/O

		if (! fSuccess || cbBytesRead == 0)
		{
			DisconnectNamedPipe(hPipe);
			CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Instance thread exited\n");
			InterlockedDecrement(&nThreadCount);
			if (nThreadCount == 0)
				PostMessage(hWnd, WM_CLOSE, 0, 0);
			return 1;
		}

		DWORD responseLength;
		GetAnswerToRequest(&request, &response, &responseLength);

		// Write the reply to the pipe.
		fSuccess = WriteFile(
			hPipe,        // handle to pipe
			&response,      // buffer to write from
			responseLength, // number of bytes to write
			&cbWritten,   // number of bytes written
			NULL);        // not overlapped I/O

		if (! fSuccess || responseLength != cbWritten)
		{
			DisconnectNamedPipe(hPipe);
			CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Instance thread exited\n");
			InterlockedDecrement(&nThreadCount);
			if (nThreadCount == 0)
				PostMessage(hWnd, WM_CLOSE, 0, 0);
			return 1;
		}
	}

	// Flush the pipe to allow the client to read the pipe's contents
	// before disconnecting. Then disconnect the pipe, and close the
	// handle to this pipe instance.

	FlushFileBuffers(hPipe);
	DisconnectNamedPipe(hPipe);
	CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Instance thread exited\n");
	InterlockedDecrement(&nThreadCount);
	if (nThreadCount == 0)
		PostMessage(hWnd, WM_CLOSE, 0, 0);
	return 0;
}

DWORD WINAPI CommandThread(LPVOID lpvParam)
{
	CTraceToOutputDebugString::Instance()(__FUNCTION__ ": CommandThread started\n");
	DWORD cbBytesRead;
	CAutoFile hPipe;

	// The thread's parameter is a handle to a pipe instance.

	hPipe = lpvParam;

	while (bRun)
	{
		// Read client requests from the pipe.
		TGITCacheCommand command;
		BOOL fSuccess = ReadFile(
			hPipe,				// handle to pipe
			&command,			// buffer to receive data
			sizeof(command),	// size of buffer
			&cbBytesRead,		// number of bytes read
			NULL);				// not overlapped I/O

		if (! fSuccess || cbBytesRead == 0)
		{
			DisconnectNamedPipe(hPipe);
			CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Command thread exited\n");
			return 1;
		}

		switch (command.command)
		{
			case TGITCACHECOMMAND_END:
				FlushFileBuffers(hPipe);
				DisconnectNamedPipe(hPipe);
				CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Command thread exited\n");
				return 0;
			case TGITCACHECOMMAND_CRAWL:
				{
					CTGitPath changedpath;
					changedpath.SetFromWin(command.path, true);
					// remove the path from our cache - that will 'invalidate' it.
					{
						CAutoWriteLock writeLock(CGitStatusCache::Instance().GetGuard());
						CGitStatusCache::Instance().RemoveCacheForPath(changedpath);
					}
					CGitStatusCache::Instance().AddFolderForCrawling(changedpath.GetDirectory());
				}
				break;
			case TGITCACHECOMMAND_REFRESHALL:
				{
					CAutoWriteLock writeLock(CGitStatusCache::Instance().GetGuard());
					CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) _T(": refresh all\n"));
					CGitStatusCache::Instance().Refresh();
				}
				break;
			case TGITCACHECOMMAND_RELEASE:
				{
					CTGitPath changedpath;
					changedpath.SetFromWin(command.path, true);
					CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) _T(": release handle for path %s\n"), changedpath.GetWinPath());
					CAutoWriteLock writeLock(CGitStatusCache::Instance().GetGuard());
					CGitStatusCache::Instance().CloseWatcherHandles(changedpath);
					CGitStatusCache::Instance().RemoveCacheForPath(changedpath);
				}
				break;
			case TGITCACHECOMMAND_BLOCK:
				{
					CTGitPath changedpath;
					changedpath.SetFromWin(command.path);
					CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) _T(": block path %s\n"), changedpath.GetWinPath());
					CGitStatusCache::Instance().BlockPath(changedpath);
				}
				break;
			case TGITCACHECOMMAND_UNBLOCK:
				{
					CTGitPath changedpath;
					changedpath.SetFromWin(command.path);
					CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) _T(": unblock path %s\n"), changedpath.GetWinPath());
					CGitStatusCache::Instance().UnBlockPath(changedpath);
				}
				break;
		}
	}

	// Flush the pipe to allow the client to read the pipe's contents
	// before disconnecting. Then disconnect the pipe, and close the
	// handle to this pipe instance.

	FlushFileBuffers(hPipe);
	DisconnectNamedPipe(hPipe);
	CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Command thread exited\n");
	return 0;
}
