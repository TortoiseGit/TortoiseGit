// TortoiseGit - a Windows shell extension for easy version control

// External Cache Copyright (C) 2005 - 2006,2010 - Will Dean, Stefan Kueng
// Copyright (C) 2008-2014, 2016-2019 - TortoiseGit

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
#include "../version.h"
#include "SmartHandle.h"
#include "CreateProcessHelper.h"
#include "gitindex.h"
#include "LoadIconEx.h"

#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp)                        static_cast<int>(static_cast<short>(LOWORD(lp)))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lp)                        static_cast<int>(static_cast<short>(HIWORD(lp)))
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
HWND				hWndHidden;
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
		if (niData.hWnd)
			Shell_NotifyIcon(NIM_DELETE, &niData);
	}
} git2init;

volatile LONG		nThreadCount = 0;

#define PACKVERSION(major,minor) MAKELONG(minor,major)

void HandleCommandLine(LPSTR lpCmdLine)
{
	char *ptr = strstr(lpCmdLine, "/kill:");
	if (ptr)
	{
		DWORD pid = static_cast<DWORD>(atoi(ptr + strlen("/kill:")));
		HANDLE hProcess = ::OpenProcess(PROCESS_TERMINATE, FALSE, pid);
		if (hProcess)
		{
			if (::WaitForSingleObject(hProcess, 5000) != WAIT_OBJECT_0)
			{
				CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Killing previous TGitCache PID %d\n", pid);
				if (!::TerminateProcess(hProcess, static_cast<UINT>(-1)))
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
		swprintf_s(cmdLine, L" /kill:%d", GetCurrentProcessId());
		if (!CCreateProcessHelper::CreateProcessDetached(exeName, cmdLine))
			CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Failed to start cache\n");
	}
}

static void AddSystrayIcon()
{
	if (CRegStdDWORD(L"Software\\TortoiseGit\\CacheTrayIcon", FALSE) != TRUE)
		return;

	SecureZeroMemory(&niData, sizeof(NOTIFYICONDATA));
	niData.cbSize = sizeof(NOTIFYICONDATA);
	niData.uID = TRAY_ID;		// own tray icon ID
	niData.hWnd = hWndHidden;
	niData.uFlags = NIF_ICON | NIF_MESSAGE;

	// load the icon
	niData.hIcon = LoadIconEx(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_TGITCACHE));

	// set the message to send
	// note: the message value should be in the
	// range of WM_APP through 0xBFFF
	niData.uCallbackMessage = TRAY_CALLBACK;
	Shell_NotifyIcon(NIM_ADD, &niData);
	// free icon handle
	if (niData.hIcon && DestroyIcon(niData.hIcon))
		niData.hIcon = nullptr;
}

int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPSTR lpCmdLine, int /*cmdShow*/)
{
	SetDllDirectory(L"");
	git_libgit2_init();
	git_libgit2_opts(GIT_OPT_SET_WINDOWS_SHAREMODE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE);
	HandleCommandLine(lpCmdLine);
	CAutoGeneralHandle hReloadProtection = ::CreateMutex(nullptr, FALSE, GetCacheMutexName());

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
	WNDCLASSEX wcex = { 0 };
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= reinterpret_cast<WNDPROC>(WndProc);
	wcex.hInstance		= hInstance;
	wcex.lpszClassName	= szWindowClass;
	RegisterClassEx(&wcex);
	hWndHidden = CreateWindow(TGIT_CACHE_WINDOW_NAME, TGIT_CACHE_WINDOW_NAME, WS_CAPTION, 0, 0, 800, 300, nullptr, 0, hInstance, 0);
	hTrayWnd = hWndHidden;
	if (!hWndHidden)
		return 0;

	// Create a thread which waits for incoming pipe connections
	CAutoGeneralHandle hPipeThread = CreateThread(
		nullptr,           // no security attribute
		0,                 // default stack size
		PipeThread,
		&bRun,             // thread parameter
		0,                 // not suspended
		&dwThreadId);      // returns thread ID

	if (!hPipeThread)
		return 0;
	else hPipeThread.CloseHandle();

	// Create a thread which waits for incoming pipe connections
	CAutoGeneralHandle hCommandWaitThread = CreateThread(
		nullptr,           // no security attribute
		0,                 // default stack size
		CommandWaitThread,
		&bRun,             // thread parameter
		0,                 // not suspended
		&dwThreadId);      // returns thread ID

	if (!hCommandWaitThread)
		return 0;

	AddSystrayIcon();

	// loop to handle window messages.
	while (bRun)
	{
		BOOL bLoopRet = GetMessage(&msg, nullptr, 0, 0);
		if ((bLoopRet != -1)&&(bLoopRet != 0))
			DispatchMessage(&msg);
	}

	bRun = false;

	CGitStatusCache::Destroy();
	HandleRestart();
	return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static UINT s_uTaskbarRestart = RegisterWindowMessage(L"TaskbarCreated");
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
				sInfoTip.Format(L"TortoiseGit Overlay Icon Server\nCached Directories: %Id\nWatched paths: %d",
					CGitStatusCache::Instance().GetCacheSize(),
					CGitStatusCache::Instance().GetNumberOfWatchedPaths());

				SystemTray.cbSize = sizeof(NOTIFYICONDATA);
				SystemTray.hWnd   = hTrayWnd;
				SystemTray.uID    = TRAY_ID;
				SystemTray.uFlags = NIF_TIP;
				wcscpy_s(SystemTray.szTip, sInfoTip);
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
					bool enabled = static_cast<DWORD>(CRegStdDWORD(L"Software\\TortoiseGit\\CacheType", GetSystemMetrics(SM_REMOTESESSION)) ? ShellCache::dll : ShellCache::exe) != ShellCache::none;
					InsertMenu(hMenu, static_cast<UINT>(-1), MF_BYPOSITION, TRAYPOP_ENABLE, enabled ? L"Disable Status Cache" : L"Enable Status Cache");
					InsertMenu(hMenu, static_cast<UINT>(-1), MF_BYPOSITION, TRAYPOP_EXIT, L"Exit");
					SetForegroundWindow(hWnd);
					TrackPopupMenu(hMenu, TPM_BOTTOMALIGN, pt.x, pt.y, 0, hWnd, nullptr);
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
			GetTextExtentPoint32(hdc, szCurrentCrawledPath[0], static_cast<int>(wcslen(szCurrentCrawledPath[0])), &fontsize);
			for (int i=nCurrentCrawledpathIndex; i<MAX_CRAWLEDPATHS; ++i)
			{
				TextOut(hdc, 0, line*fontsize.cy, szCurrentCrawledPath[i], static_cast<int>(wcslen(szCurrentCrawledPath[i])));
				++line;
			}
			for (int i=0; i<nCurrentCrawledpathIndex; ++i)
			{
				TextOut(hdc, 0, line*fontsize.cy, szCurrentCrawledPath[i], static_cast<int>(wcslen(szCurrentCrawledPath[i])));
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
					CRegStdDWORD reg = CRegStdDWORD(L"Software\\TortoiseGit\\CacheType", GetSystemMetrics(SM_REMOTESESSION) ? ShellCache::dll : ShellCache::exe);
					bool enabled = static_cast<DWORD>(reg) != ShellCache::none;
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
			if (niData.hWnd)
			{
				niData.hIcon = LoadIconEx(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_TGITCACHE_STOPPING));
				Shell_NotifyIcon(NIM_MODIFY, &niData);
			}
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
			auto phdr = reinterpret_cast<DEV_BROADCAST_HDR*>(lParam);
			switch (wParam)
			{
			case DBT_CUSTOMEVENT:
				{
					CTraceToOutputDebugString::Instance()(__FUNCTION__ ": WM_DEVICECHANGE with DBT_CUSTOMEVENT\n");
					if (phdr->dbch_devicetype == DBT_DEVTYP_HANDLE)
					{
						auto phandle = reinterpret_cast<DEV_BROADCAST_HANDLE*>(lParam);
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
					auto phandle = reinterpret_cast<DEV_BROADCAST_HANDLE*>(lParam);
					CAutoWriteLock writeLock(CGitStatusCache::Instance().GetGuard());
					CGitStatusCache::Instance().CloseWatcherHandles(phandle->dbch_handle);
				}
				else if (phdr->dbch_devicetype == DBT_DEVTYP_VOLUME)
				{
					auto pVolume = reinterpret_cast<DEV_BROADCAST_VOLUME*>(lParam);
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
		if (message == s_uTaskbarRestart)
			AddSystrayIcon();
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
		path.SetFromWin(pRequest->path, !!(pRequest->flags & TGITCACHE_FLAGS_ISFOLDER));
	else
		path.SetFromWin(pRequest->path);

	if (!bRun)
	{
		CStatusCacheEntry entry;
		entry.BuildCacheResponse(*pReply, *pResponseLength);
		return;
	}

	CAutoReadWeakLock readLock(CGitStatusCache::Instance().GetGuard(), 2000);
	if (readLock.IsAcquired())
	{
		CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": app asked for status of %s\n", pRequest->path);
		CGitStatusCache::Instance().GetStatusForPath(path, pRequest->flags).BuildCacheResponse(*pReply, *pResponseLength);
	}
	else
	{
		CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": timeout for asked status of %s\n", pRequest->path);
		CStatusCacheEntry entry;
		entry.BuildCacheResponse(*pReply, *pResponseLength);
	}
}

DWORD WINAPI PipeThread(LPVOID lpvParam)
{
	CTraceToOutputDebugString::Instance()(__FUNCTION__ ": PipeThread started\n");
	auto bThreadRun = reinterpret_cast<bool*>(lpvParam);
	// The main loop creates an instance of the named pipe and
	// then waits for a client to connect to it. When the client
	// connects, a thread is created to handle communications
	// with that client, and the loop is repeated.
	DWORD dwThreadId;
	BOOL fConnected;
	CAutoFile hPipe;

	while (*bThreadRun)
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
			nullptr);				  // nullptr DACL

		if (!hPipe)
		{
			if (*bThreadRun)
				Sleep(200);
			continue; // never leave the thread!
		}

		// Wait for the client to connect; if it succeeds,
		// the function returns a nonzero value. If the function returns
		// zero, GetLastError returns ERROR_PIPE_CONNECTED.
		fConnected = ConnectNamedPipe(hPipe, nullptr) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
		if (fConnected)
		{
			// Create a thread for this client.
			CAutoGeneralHandle hInstanceThread = CreateThread(
				nullptr,           // no security attribute
				0,                 // default stack size
				InstanceThread,
				hPipe,             // thread parameter
				0,                 // not suspended
				&dwThreadId);      // returns thread ID

			if (!hInstanceThread)
			{
				DisconnectNamedPipe(hPipe);
				// since we're now closing this thread, we also have to close the whole application!
				// otherwise the thread is dead, but the app is still running, refusing new instances
				// but no pipe will be available anymore.
				PostMessage(hWndHidden, WM_CLOSE, 0, 0);
				return 1;
			}
			// detach the handle, since we passed it to the thread
			hPipe.Detach();
		}
		else
		{
			// The client could not connect, so close the pipe.
			hPipe.CloseHandle();
			if (*bThreadRun)
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
	auto bThreadRun = reinterpret_cast<bool*>(lpvParam);
	// The main loop creates an instance of the named pipe and
	// then waits for a client to connect to it. When the client
	// connects, a thread is created to handle communications
	// with that client, and the loop is repeated.
	DWORD dwThreadId;
	BOOL fConnected;
	CAutoFile hPipe;

	while (*bThreadRun)
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
			nullptr);                 // nullptr DACL

		if (!hPipe)
		{
			if (*bThreadRun)
				Sleep(200);
			continue; // never leave the thread!
		}

		// Wait for the client to connect; if it succeeds,
		// the function returns a nonzero value. If the function returns
		// zero, GetLastError returns ERROR_PIPE_CONNECTED.
		fConnected = ConnectNamedPipe(hPipe, nullptr) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
		if (fConnected)
		{
			// Create a thread for this client.
			CAutoGeneralHandle hCommandThread = CreateThread(
				nullptr,           // no security attribute
				0,                 // default stack size
				CommandThread,
				hPipe,             // thread parameter
				0,                 // not suspended
				&dwThreadId);      // returns thread ID

			if (!hCommandThread)
			{
				DisconnectNamedPipe(hPipe);
				hPipe.CloseHandle();
				// since we're now closing this thread, we also have to close the whole application!
				// otherwise the thread is dead, but the app is still running, refusing new instances
				// but no pipe will be available anymore.
				PostMessage(hWndHidden, WM_CLOSE, 0, 0);
				return 1;
			}
			// detach the handle, since we passed it to the thread
			hPipe.Detach();
		}
		else
		{
			// The client could not connect, so close the pipe.
			hPipe.CloseHandle();
			if (*bThreadRun)
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

	// The thread's parameter is a handle to a pipe instance.
	CAutoFile hPipe(std::move(lpvParam));

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
			nullptr);        // not overlapped I/O

		if (! fSuccess || cbBytesRead == 0)
		{
			DisconnectNamedPipe(hPipe);
			CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Instance thread exited\n");
			if (InterlockedDecrement(&nThreadCount) == 0)
				PostMessage(hWndHidden, WM_CLOSE, 0, 0);
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
			nullptr);        // not overlapped I/O

		if (! fSuccess || responseLength != cbWritten)
		{
			DisconnectNamedPipe(hPipe);
			CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Instance thread exited\n");
			if (InterlockedDecrement(&nThreadCount) == 0)
				PostMessage(hWndHidden, WM_CLOSE, 0, 0);
			return 1;
		}
	}

	// Flush the pipe to allow the client to read the pipe's contents
	// before disconnecting. Then disconnect the pipe, and close the
	// handle to this pipe instance.

	FlushFileBuffers(hPipe);
	DisconnectNamedPipe(hPipe);
	CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Instance thread exited\n");
	if (InterlockedDecrement(&nThreadCount) == 0)
		PostMessage(hWndHidden, WM_CLOSE, 0, 0);
	return 0;
}

DWORD WINAPI CommandThread(LPVOID lpvParam)
{
	CTraceToOutputDebugString::Instance()(__FUNCTION__ ": CommandThread started\n");
	DWORD cbBytesRead;

	// The thread's parameter is a handle to a pipe instance.
	CAutoFile hPipe(std::move(lpvParam));

	while (bRun)
	{
		// Read client requests from the pipe.
		TGITCacheCommand command;
		BOOL fSuccess = ReadFile(
			hPipe,				// handle to pipe
			&command,			// buffer to receive data
			sizeof(command),	// size of buffer
			&cbBytesRead,		// number of bytes read
			nullptr);			// not overlapped I/O

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
					CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": refresh all\n");
					CGitStatusCache::Instance().Refresh();
				}
				break;
			case TGITCACHECOMMAND_RELEASE:
				{
					CTGitPath changedpath;
					changedpath.SetFromWin(command.path, true);
					CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": release handle for path %s\n", changedpath.GetWinPath());
					CAutoWriteLock writeLock(CGitStatusCache::Instance().GetGuard());
					CGitStatusCache::Instance().CloseWatcherHandles(changedpath);
					CGitStatusCache::Instance().RemoveCacheForPath(changedpath);
					auto cachedDir = CGitStatusCache::Instance().GetDirectoryCacheEntryNoCreate(changedpath.GetContainingDirectory());
					if (cachedDir)
						cachedDir->Invalidate();
				}
				break;
			case TGITCACHECOMMAND_BLOCK:
				{
					CTGitPath changedpath;
					changedpath.SetFromWin(command.path);
					CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": block path %s\n", changedpath.GetWinPath());
					CGitStatusCache::Instance().BlockPath(changedpath);
				}
				break;
			case TGITCACHECOMMAND_UNBLOCK:
				{
					CTGitPath changedpath;
					changedpath.SetFromWin(command.path);
					CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": unblock path %s\n", changedpath.GetWinPath());
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
