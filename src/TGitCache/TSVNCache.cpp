// TortoiseSVN - a Windows shell extension for easy version control

// External Cache Copyright (C) 2005 - 2006 - Will Dean, Stefan Kueng

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
#include "shellapi.h"
#include "TSVNCache.h"
#include "GitStatusCache.h"
#include "CacheInterface.h"
#include "Resource.h"
#include "registry.h"
#include "..\crashrpt\CrashReport.h"
#include "GitAdminDir.h"
#include "Dbt.h"
#include <initguid.h>
#include "ioevent.h"
#include "..\version.h"
//#include "svn_dso.h"

#include <ShellAPI.h>

#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp)                        ((int)(short)LOWORD(lp))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lp)                        ((int)(short)HIWORD(lp))
#endif


#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

CCrashReport crasher("tortoisegit-bug@googlegroups.com", "Crash Report for TSVNCache " APP_X64_STRING " : " STRPRODUCTVER, TRUE);// crash

DWORD WINAPI 		InstanceThread(LPVOID); 
DWORD WINAPI		PipeThread(LPVOID);
DWORD WINAPI		CommandWaitThread(LPVOID);
DWORD WINAPI		CommandThread(LPVOID);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
bool				bRun = true;
NOTIFYICONDATA		niData; 
HWND				hWnd;
HWND				hTrayWnd;
TCHAR				szCurrentCrawledPath[MAX_CRAWLEDPATHS][MAX_CRAWLEDPATHSLEN];
int					nCurrentCrawledpathIndex = 0;
CComAutoCriticalSection critSec;

volatile LONG		nThreadCount = 0;

#define PACKVERSION(major,minor) MAKELONG(minor,major)
DWORD GetDllVersion(LPCTSTR lpszDllName)
{
	HINSTANCE hinstDll;
	DWORD dwVersion = 0;

	/* For security purposes, LoadLibrary should be provided with a 
	fully-qualified path to the DLL. The lpszDllName variable should be
	tested to ensure that it is a fully qualified path before it is used. */
	hinstDll = LoadLibrary(lpszDllName);

	if(hinstDll)
	{
		DLLGETVERSIONPROC pDllGetVersion;
		pDllGetVersion = (DLLGETVERSIONPROC)GetProcAddress(hinstDll, 
			"DllGetVersion");

		/* Because some DLLs might not implement this function, you
		must test for it explicitly. Depending on the particular 
		DLL, the lack of a DllGetVersion function can be a useful
		indicator of the version. */

		if(pDllGetVersion)
		{
			DLLVERSIONINFO dvi;
			HRESULT hr;

			SecureZeroMemory(&dvi, sizeof(dvi));
			dvi.cbSize = sizeof(dvi);

			hr = (*pDllGetVersion)(&dvi);

			if(SUCCEEDED(hr))
			{
				dwVersion = PACKVERSION(dvi.dwMajorVersion, dvi.dwMinorVersion);
			}
		}

		FreeLibrary(hinstDll);
	}
	return dwVersion;
}

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
	OutputDebugStringA("TSVNCache GetLastError(): ");
	OutputDebugString((LPCTSTR)lpMsgBuf);
	OutputDebugStringA("\n");

	// Free the buffer.
	LocalFree( lpMsgBuf );
}

int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int /*cmdShow*/)
{
	HANDLE hReloadProtection = ::CreateMutex(NULL, FALSE, GetCacheMutexName());

	if (hReloadProtection == 0 || GetLastError() == ERROR_ALREADY_EXISTS)
	{
		// An instance of TSVNCache is already running
		ATLTRACE("TSVNCache ignoring restart\n");
		return 0;
	}

//	apr_initialize();
//	svn_dso_initialize2();
	g_GitAdminDir.Init();
	CGitStatusCache::Create();
	CGitStatusCache::Instance().Init();

	SecureZeroMemory(szCurrentCrawledPath, sizeof(szCurrentCrawledPath));
	
	DWORD dwThreadId; 
	HANDLE hPipeThread; 
	HANDLE hCommandWaitThread;
	MSG msg;
	TCHAR szWindowClass[] = {TSVN_CACHE_WINDOW_NAME};

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
	hWnd = CreateWindow(TSVN_CACHE_WINDOW_NAME, TSVN_CACHE_WINDOW_NAME, WS_CAPTION, 0, 0, 800, 300, NULL, 0, hInstance, 0);
	hTrayWnd = hWnd;
	if (hWnd == NULL)
	{
		return 0;
	}
	if (CRegStdWORD(_T("Software\\TortoiseGit\\CacheTrayIcon"), FALSE)==TRUE)
	{
		SecureZeroMemory(&niData,sizeof(NOTIFYICONDATA));

		DWORD dwVersion = GetDllVersion(_T("Shell32.dll"));

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
			MAKEINTRESOURCE(IDI_TSVNCACHE),
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
	hPipeThread = CreateThread( 
		NULL,              // no security attribute 
		0,                 // default stack size 
		PipeThread, 
		(LPVOID) &bRun,    // thread parameter 
		0,                 // not suspended 
		&dwThreadId);      // returns thread ID 

	if (hPipeThread == NULL) 
	{
		//OutputDebugStringA("TSVNCache: Could not create pipe thread\n");
		//DebugOutputLastError();
		return 0;
	}
	else CloseHandle(hPipeThread); 

	// Create a thread which waits for incoming pipe connections 
	hCommandWaitThread = CreateThread( 
		NULL,              // no security attribute 
		0,                 // default stack size 
		CommandWaitThread, 
		(LPVOID) &bRun,    // thread parameter 
		0,                 // not suspended 
		&dwThreadId);      // returns thread ID 

	if (hCommandWaitThread == NULL) 
	{
		//OutputDebugStringA("TSVNCache: Could not create command wait thread\n");
		//DebugOutputLastError();
		return 0;
	}
	else CloseHandle(hCommandWaitThread); 


	// loop to handle window messages.
	BOOL bLoopRet;
	while (bRun)
	{
		bLoopRet = GetMessage(&msg, NULL, 0, 0);
		if ((bLoopRet != -1)&&(bLoopRet != 0))
		{
			DispatchMessage(&msg);
		}
	}

	bRun = false;

	Shell_NotifyIcon(NIM_DELETE,&niData);
	CGitStatusCache::Destroy();
	g_GitAdminDir.Close();
//	apr_terminate();

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
				sInfoTip.Format(_T("Cached Directories : %ld\nWatched paths : %ld"), 
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
				DWORD ptW = GetMessagePos();
				pt.x = GET_X_LPARAM(ptW);
				pt.y = GET_Y_LPARAM(ptW);
				HMENU hMenu = CreatePopupMenu();
				if(hMenu)
				{
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
				line++;
			}
			for (int i=0; i<nCurrentCrawledpathIndex; ++i)
			{
				TextOut(hdc, 0, line*fontsize.cy, szCurrentCrawledPath[i], (int)_tcslen(szCurrentCrawledPath[i]));
				line++;
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
			case TRAYPOP_EXIT:
				DestroyWindow(hWnd);
				break;
			}
			return 1;
		}
	case WM_QUERYENDSESSION:
		{
			ATLTRACE("WM_QUERYENDSESSION\n");
			if (CGitStatusCache::Instance().WaitToWrite(200))
			{
				CGitStatusCache::Instance().Stop();
				CGitStatusCache::Instance().Done();
			}
			return TRUE;
		}
		break;
	case WM_CLOSE:
	case WM_ENDSESSION:
	case WM_DESTROY:
	case WM_QUIT:
		{
			ATLTRACE("WM_CLOSE/DESTROY/ENDSESSION/QUIT\n");
			CGitStatusCache::Instance().WaitToWrite();
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
					ATLTRACE("WM_DEVICECHANGE with DBT_CUSTOMEVENT\n");
					if (phdr->dbch_devicetype == DBT_DEVTYP_HANDLE)
					{
						DEV_BROADCAST_HANDLE * phandle = (DEV_BROADCAST_HANDLE*)lParam;
						if (IsEqualGUID(phandle->dbch_eventguid, GUID_IO_VOLUME_DISMOUNT))
						{
							ATLTRACE("Device to be dismounted\n");
							CGitStatusCache::Instance().WaitToWrite();
							CGitStatusCache::Instance().CloseWatcherHandles(phandle->dbch_hdevnotify);
							CGitStatusCache::Instance().Done();
						}
						if (IsEqualGUID(phandle->dbch_eventguid, GUID_IO_VOLUME_LOCK))
						{
							ATLTRACE("Device lock event\n");
							CGitStatusCache::Instance().WaitToWrite();
							CGitStatusCache::Instance().CloseWatcherHandles(phandle->dbch_hdevnotify);
							CGitStatusCache::Instance().Done();
						}
					}
				}
				break;
			case DBT_DEVICEREMOVEPENDING:
				ATLTRACE("WM_DEVICECHANGE with DBT_DEVICEREMOVEPENDING\n");
				if (phdr->dbch_devicetype == DBT_DEVTYP_HANDLE)
				{
					DEV_BROADCAST_HANDLE * phandle = (DEV_BROADCAST_HANDLE*)lParam;
					CGitStatusCache::Instance().WaitToWrite();
					CGitStatusCache::Instance().CloseWatcherHandles(phandle->dbch_hdevnotify);
					CGitStatusCache::Instance().Done();
				}
				else
				{
					CGitStatusCache::Instance().WaitToWrite();
					CGitStatusCache::Instance().CloseWatcherHandles(INVALID_HANDLE_VALUE);
					CGitStatusCache::Instance().Done();
				}
				break;
			case DBT_DEVICEQUERYREMOVE:
				ATLTRACE("WM_DEVICECHANGE with DBT_DEVICEQUERYREMOVE\n");
				if (phdr->dbch_devicetype == DBT_DEVTYP_HANDLE)
				{
					DEV_BROADCAST_HANDLE * phandle = (DEV_BROADCAST_HANDLE*)lParam;
					CGitStatusCache::Instance().WaitToWrite();
					CGitStatusCache::Instance().CloseWatcherHandles(phandle->dbch_hdevnotify);
					CGitStatusCache::Instance().Done();
				}
				else
				{
					CGitStatusCache::Instance().WaitToWrite();
					CGitStatusCache::Instance().CloseWatcherHandles(INVALID_HANDLE_VALUE);
					CGitStatusCache::Instance().Done();
				}
				break;
			case DBT_DEVICEREMOVECOMPLETE:
				ATLTRACE("WM_DEVICECHANGE with DBT_DEVICEREMOVECOMPLETE\n");
				if (phdr->dbch_devicetype == DBT_DEVTYP_HANDLE)
				{
					DEV_BROADCAST_HANDLE * phandle = (DEV_BROADCAST_HANDLE*)lParam;
					CGitStatusCache::Instance().WaitToWrite();
					CGitStatusCache::Instance().CloseWatcherHandles(phandle->dbch_hdevnotify);
					CGitStatusCache::Instance().Done();
				}
				else
				{
					CGitStatusCache::Instance().WaitToWrite();
					CGitStatusCache::Instance().CloseWatcherHandles(INVALID_HANDLE_VALUE);
					CGitStatusCache::Instance().Done();
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

VOID GetAnswerToRequest(const TSVNCacheRequest* pRequest, TSVNCacheResponse* pReply, DWORD* pResponseLength)
{
	CTGitPath path;
	*pResponseLength = 0;
	if(pRequest->flags & TSVNCACHE_FLAGS_FOLDERISKNOWN)
	{
		path.SetFromWin(pRequest->path, !!(pRequest->flags & TSVNCACHE_FLAGS_ISFOLDER));
	}
	else
	{
		path.SetFromWin(pRequest->path);
	}

	if (CGitStatusCache::Instance().WaitToRead(2000))
	{
		CGitStatusCache::Instance().GetStatusForPath(path, pRequest->flags, false).BuildCacheResponse(*pReply, *pResponseLength);
		CGitStatusCache::Instance().Done();
	}
	else
	{
		CStatusCacheEntry entry;
		entry.BuildCacheResponse(*pReply, *pResponseLength);
	}
}

DWORD WINAPI PipeThread(LPVOID lpvParam)
{
	ATLTRACE("PipeThread started\n");
	bool * bRun = (bool *)lpvParam;
	// The main loop creates an instance of the named pipe and 
	// then waits for a client to connect to it. When the client 
	// connects, a thread is created to handle communications 
	// with that client, and the loop is repeated. 
	DWORD dwThreadId; 
	BOOL fConnected;
	HANDLE hPipe = INVALID_HANDLE_VALUE;
	HANDLE hInstanceThread = INVALID_HANDLE_VALUE;

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

		if (hPipe == INVALID_HANDLE_VALUE) 
		{
			//OutputDebugStringA("TSVNCache: CreatePipe failed\n");
			//DebugOutputLastError();
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
			hInstanceThread = CreateThread( 
				NULL,              // no security attribute 
				0,                 // default stack size 
				InstanceThread, 
				(LPVOID) hPipe,    // thread parameter 
				0,                 // not suspended 
				&dwThreadId);      // returns thread ID 

			if (hInstanceThread == NULL) 
			{
				//OutputDebugStringA("TSVNCache: Could not create Instance thread\n");
				//DebugOutputLastError();
				DisconnectNamedPipe(hPipe);
				CloseHandle(hPipe);
				// since we're now closing this thread, we also have to close the whole application!
				// otherwise the thread is dead, but the app is still running, refusing new instances
				// but no pipe will be available anymore.
				PostMessage(hWnd, WM_CLOSE, 0, 0);
				return 1;
			}
			else CloseHandle(hInstanceThread); 
		} 
		else
		{
			// The client could not connect, so close the pipe. 
			//OutputDebugStringA("TSVNCache: ConnectNamedPipe failed\n");
			//DebugOutputLastError();
			CloseHandle(hPipe); 
			if (*bRun)
				Sleep(200);
			continue;	// don't end the thread!
		}
	}
	ATLTRACE("Pipe thread exited\n");
	return 0;
}

DWORD WINAPI CommandWaitThread(LPVOID lpvParam)
{
	ATLTRACE("CommandWaitThread started\n");
	bool * bRun = (bool *)lpvParam;
	// The main loop creates an instance of the named pipe and 
	// then waits for a client to connect to it. When the client 
	// connects, a thread is created to handle communications 
	// with that client, and the loop is repeated. 
	DWORD dwThreadId; 
	BOOL fConnected;
	HANDLE hPipe = INVALID_HANDLE_VALUE;
	HANDLE hCommandThread = INVALID_HANDLE_VALUE;

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

		if (hPipe == INVALID_HANDLE_VALUE) 
		{
			//OutputDebugStringA("TSVNCache: CreatePipe failed\n");
			//DebugOutputLastError();
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
			hCommandThread = CreateThread( 
				NULL,              // no security attribute 
				0,                 // default stack size 
				CommandThread, 
				(LPVOID) hPipe,    // thread parameter 
				0,                 // not suspended 
				&dwThreadId);      // returns thread ID 

			if (hCommandThread == NULL) 
			{
				//OutputDebugStringA("TSVNCache: Could not create Command thread\n");
				//DebugOutputLastError();
				DisconnectNamedPipe(hPipe);
				CloseHandle(hPipe);
				// since we're now closing this thread, we also have to close the whole application!
				// otherwise the thread is dead, but the app is still running, refusing new instances
				// but no pipe will be available anymore.
				PostMessage(hWnd, WM_CLOSE, 0, 0);
				return 1;
			}
			else CloseHandle(hCommandThread); 
		} 
		else
		{
			// The client could not connect, so close the pipe. 
			//OutputDebugStringA("TSVNCache: ConnectNamedPipe failed\n");
			//DebugOutputLastError();
			CloseHandle(hPipe); 
			if (*bRun)
				Sleep(200);
			continue;	// don't end the thread!
		}
	}
	ATLTRACE("CommandWait thread exited\n");
	return 0;
}

DWORD WINAPI InstanceThread(LPVOID lpvParam) 
{ 
	ATLTRACE("InstanceThread started\n");
	TSVNCacheResponse response; 
	DWORD cbBytesRead, cbWritten; 
	BOOL fSuccess; 
	HANDLE hPipe; 

	// The thread's parameter is a handle to a pipe instance. 

	hPipe = (HANDLE) lpvParam; 
	InterlockedIncrement(&nThreadCount);
	while (bRun) 
	{ 
		// Read client requests from the pipe. 
		TSVNCacheRequest request;
		fSuccess = ReadFile( 
			hPipe,        // handle to pipe 
			&request,    // buffer to receive data 
			sizeof(request), // size of buffer 
			&cbBytesRead, // number of bytes read 
			NULL);        // not overlapped I/O 

		if (! fSuccess || cbBytesRead == 0)
		{
			DisconnectNamedPipe(hPipe); 
			CloseHandle(hPipe); 
			ATLTRACE("Instance thread exited\n");
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
			CloseHandle(hPipe); 
			ATLTRACE("Instance thread exited\n");
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
	CloseHandle(hPipe); 
	ATLTRACE("Instance thread exited\n");
	InterlockedDecrement(&nThreadCount);
	if (nThreadCount == 0)
		PostMessage(hWnd, WM_CLOSE, 0, 0);
	return 0;
}

DWORD WINAPI CommandThread(LPVOID lpvParam) 
{ 
	ATLTRACE("CommandThread started\n");
	DWORD cbBytesRead; 
	BOOL fSuccess; 
	HANDLE hPipe; 

	// The thread's parameter is a handle to a pipe instance. 

	hPipe = (HANDLE) lpvParam; 

	while (bRun) 
	{ 
		// Read client requests from the pipe. 
		TSVNCacheCommand command;
		fSuccess = ReadFile( 
			hPipe,				// handle to pipe 
			&command,			// buffer to receive data 
			sizeof(command),	// size of buffer 
			&cbBytesRead,		// number of bytes read 
			NULL);				// not overlapped I/O 

		if (! fSuccess || cbBytesRead == 0)
		{
			DisconnectNamedPipe(hPipe); 
			CloseHandle(hPipe); 
			ATLTRACE("Command thread exited\n");
			return 1;
		}
		
		switch (command.command)
		{
			case TSVNCACHECOMMAND_END:
				FlushFileBuffers(hPipe); 
				DisconnectNamedPipe(hPipe); 
				CloseHandle(hPipe); 
				ATLTRACE("Command thread exited\n");
				return 0;
			case TSVNCACHECOMMAND_CRAWL:
				{
					CTGitPath changedpath;
					changedpath.SetFromWin(CString(command.path), true);
					// remove the path from our cache - that will 'invalidate' it.
					CGitStatusCache::Instance().WaitToWrite();
					CGitStatusCache::Instance().RemoveCacheForPath(changedpath);
					CGitStatusCache::Instance().Done();
					CGitStatusCache::Instance().AddFolderForCrawling(changedpath.GetDirectory());
				}
				break;
			case TSVNCACHECOMMAND_REFRESHALL:
				CGitStatusCache::Instance().WaitToWrite();
				CGitStatusCache::Instance().Refresh();
				CGitStatusCache::Instance().Done();
				break;
			case TSVNCACHECOMMAND_RELEASE:
				{
					CTGitPath changedpath;
					changedpath.SetFromWin(CString(command.path), true);
					ATLTRACE(_T("release handle for path %s\n"), changedpath.GetWinPath());
					CGitStatusCache::Instance().WaitToWrite();
					CGitStatusCache::Instance().CloseWatcherHandles(changedpath);
					CGitStatusCache::Instance().RemoveCacheForPath(changedpath);
					CGitStatusCache::Instance().Done();
				}
				break;

		}
	} 

	// Flush the pipe to allow the client to read the pipe's contents 
	// before disconnecting. Then disconnect the pipe, and close the 
	// handle to this pipe instance. 

	FlushFileBuffers(hPipe); 
	DisconnectNamedPipe(hPipe); 
	CloseHandle(hPipe); 
	ATLTRACE("Command thread exited\n");
	return 0;
}
