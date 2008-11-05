// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007 - TortoiseSVN

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
#define _WIN32_WINNT 0x0400
#include <Windows.h>
#include <tchar.h>
#include "Debug.h"


const HINSTANCE NIL = (HINSTANCE)((char*)(0)-1);

static HINSTANCE hInst = NULL;

static HINSTANCE hTortoiseSVN = NULL;
static LPFNGETCLASSOBJECT pDllGetClassObject = NULL;
static LPFNCANUNLOADNOW pDllCanUnloadNow = NULL;




/**
 * \ingroup TortoiseShell
 * Check whether to load the full TortoiseSVN.dll or not.
 */
static BOOL WantRealVersion(void)
{
	static const WCHAR TSVNRootKey[]=_T("Software\\TortoiseSVN");
	static const WCHAR ExplorerOnlyValue[]=_T("LoadDllOnlyInExplorer");

	static const WCHAR ExplorerEnvPath[]=_T("%SystemRoot%\\explorer.exe");


	DWORD bExplorerOnly = 0;
	WCHAR ModuleName[MAX_PATH] = {0};
	WCHAR ExplorerPath[MAX_PATH] = {0};

	HKEY hKey = HKEY_CURRENT_USER;
	LONG Result = ERROR;
	DWORD Type = REG_DWORD;
	DWORD Len = sizeof(DWORD);

	BOOL bWantReal = TRUE;


	TRACE(_T("WantRealVersion() - Enter\n"));

	Result = RegOpenKeyEx(HKEY_CURRENT_USER, TSVNRootKey, 0, KEY_READ, &hKey);
	if (Result == ERROR_SUCCESS)
	{
		Result = RegQueryValueEx(hKey, ExplorerOnlyValue, NULL, &Type, (BYTE *)&bExplorerOnly, &Len);
		if ((Result == ERROR_SUCCESS) && (Type == REG_DWORD) && (Len == sizeof(DWORD)) && bExplorerOnly)
		{
			TRACE(_T("WantRealVersion() - Explorer Only\n"));

			// check if the current process is in fact the explorer
			Len = GetModuleFileName(NULL, ModuleName, sizeof(ModuleName));
			if (Len)
			{
				TRACE(_T("Process is %s\n"), ModuleName);

				Len = ExpandEnvironmentStrings(ExplorerEnvPath, ExplorerPath, sizeof(ExplorerPath));
				if (Len && (Len <= sizeof(ExplorerPath)))
				{
					TRACE(_T("Explorer path is %s\n"), ExplorerPath);
					bWantReal = !lstrcmpi(ModuleName, ExplorerPath);
				}
			}
		}

		RegCloseKey(hKey);
	}

	TRACE(_T("WantRealVersion() - Exit\n"));
	return bWantReal;
}

static void LoadRealLibrary(void)
{
	static const char GetClassObject[] = "DllGetClassObject";
	static const char CanUnloadNow[] = "DllCanUnloadNow";

	WCHAR ModuleName[MAX_PATH] = {0};
	DWORD Len = 0;


	if (hTortoiseSVN)
		return;

	if (!WantRealVersion())
	{
		TRACE(_T("LoadRealLibrary() - Bypass\n"));
		hTortoiseSVN = NIL;
		return;
	}

	Len = GetModuleFileName(hInst, ModuleName, sizeof(ModuleName));
	if (!Len)
	{
		TRACE(_T("LoadRealLibrary() - Fail\n"));
		hTortoiseSVN = NIL;
		return;
	}

	// truncate the string at the last '\' char
	while(Len > 0)
	{
		--Len;
		if (ModuleName[Len] == '\\')
		{
			ModuleName[Len] = '\0';
			break;
		}
	}
	if (Len == 0)
	{
		TRACE(_T("LoadRealLibrary() - Fail\n"));
		hTortoiseSVN = NIL;
		return;
	}
	lstrcat(ModuleName, _T("\\TortoiseSVN.dll"));

	TRACE(_T("LoadRealLibrary() - Load %s\n"), ModuleName);

	hTortoiseSVN = LoadLibraryEx(ModuleName, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
	if (!hTortoiseSVN)
	{
		TRACE(_T("LoadRealLibrary() - Fail\n"));
		hTortoiseSVN = NIL;
		return;
	}

	TRACE(_T("LoadRealLibrary() - Success\n"));
	pDllGetClassObject = NULL;
	pDllCanUnloadNow = NULL;
	pDllGetClassObject = (LPFNGETCLASSOBJECT)GetProcAddress(hTortoiseSVN, GetClassObject);
	if (pDllGetClassObject == NULL)
	{
		TRACE(_T("LoadRealLibrary() - Fail\n"));
		FreeLibrary(hTortoiseSVN);
		hTortoiseSVN = NIL;
		return;
	}
	pDllCanUnloadNow = (LPFNCANUNLOADNOW)GetProcAddress(hTortoiseSVN, CanUnloadNow);
	if (pDllCanUnloadNow == NULL)
	{
		TRACE(_T("LoadRealLibrary() - Fail\n"));
		FreeLibrary(hTortoiseSVN);
		hTortoiseSVN = NIL;
		return;
	}
}

static void UnloadRealLibrary(void)
{
	if (!hTortoiseSVN)
		return;

	if (hTortoiseSVN != NIL)
		FreeLibrary(hTortoiseSVN);

	hTortoiseSVN = NULL;
	pDllGetClassObject = NULL;
	pDllCanUnloadNow = NULL;
}


BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD Reason, LPVOID Reserved)
{
#ifdef _DEBUG
	// if no debugger is present, then don't load the dll.
	// this prevents other apps from loading the dll and locking
	// it.

	BOOL bInShellTest = FALSE;
	TCHAR buf[_MAX_PATH + 1];		// MAX_PATH ok, the test really is for debugging anyway.
	DWORD pathLength = GetModuleFileName(NULL, buf, _MAX_PATH);

	UNREFERENCED_PARAMETER(Reserved);

	if (pathLength >= 14)
	{
		if ((lstrcmpi(&buf[pathLength-14], _T("\\ShellTest.exe"))) == 0)
		{
			bInShellTest = TRUE;
		}
	}

	if (!IsDebuggerPresent() && !bInShellTest)
	{
		TRACE(_T("In debug load preventer\n"));
		return FALSE;
	}
#else
	UNREFERENCED_PARAMETER(Reserved);
#endif


	switch(Reason)
	{
	case DLL_PROCESS_ATTACH:
		hInst = hInstance;
		break;

		//case DLL_THREAD_ATTACH:
		//  break;

		//case DLL_THREAD_DETACH:
		//  break;

		//case DLL_PROCESS_DETACH:
		//  break;
	}

	return TRUE;
}


STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
	TRACE(_T("DllGetClassObject() - Enter\n"));

	LoadRealLibrary();
	if (!pDllGetClassObject)
	{
		if (ppv)
			*ppv = NULL;
		else
			ppv = NULL;

		TRACE(_T("DllGetClassObject() - Bypass\n"));
		return CLASS_E_CLASSNOTAVAILABLE;
	}

	TRACE(_T("DllGetClassObject() - Forward\n"));
	return pDllGetClassObject(rclsid, riid, ppv);
}

STDAPI DllCanUnloadNow(void)
{
	HRESULT Result;

	TRACE(_T("DllCanUnloadNow() - Enter\n"));

	if (pDllCanUnloadNow)
	{
		TRACE(_T("DllCanUnloadNow() - Forward\n"));
		Result = pDllCanUnloadNow();
		if (Result != S_OK)
			return Result;
	}

	TRACE(_T("DllCanUnloadNow() - Unload\n"));
	UnloadRealLibrary();
	return S_OK;
}

