// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2012-2014, 2016-2019 - TortoiseGit
// Copyright (C) 2007, 2009, 2013-2014, 2018 - TortoiseSVN

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
#include "../targetver.h"
#include <Windows.h>
#include <tchar.h>
#include "Debug.h"

const HINSTANCE NIL = reinterpret_cast<HINSTANCE>((static_cast<char*>(0)-1));

static HINSTANCE hInst = nullptr;

static HINSTANCE hTortoiseGit = nullptr;
static LPFNGETCLASSOBJECT pDllGetClassObject = nullptr;
static LPFNCANUNLOADNOW pDllCanUnloadNow = nullptr;

static wchar_t DebugDllPath[MAX_PATH] = { 0 };

static BOOL DebugActive(void)
{
	static const WCHAR TGitRootKey[] = L"Software\\TortoiseGit";
	static const WCHAR DebugShellValue[] = L"DebugShell";
	static const WCHAR DebugShellPathValue[] = L"DebugShellPath";

	DWORD bDebug = 0;

	HKEY hKey = HKEY_CURRENT_USER;
	LONG Result = ERROR;
	DWORD Type = REG_DWORD;
	DWORD Len = sizeof(DWORD);

	BOOL bDebugActive = FALSE;


	TRACE(L"DebugActive() - Enter\n");

	if (IsDebuggerPresent())
	{
		Result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TGitRootKey, 0, KEY_READ, &hKey);
		if (Result == ERROR_SUCCESS)
		{
			Result = RegQueryValueEx(hKey, DebugShellValue, nullptr, &Type, reinterpret_cast<BYTE*>(&bDebug), &Len);
			if ((Result == ERROR_SUCCESS) && (Type == REG_DWORD) && (Len == sizeof(DWORD)) && bDebug)
			{
				TRACE(L"DebugActive() - debug active\n");
				bDebugActive = TRUE;
				Len = sizeof(wchar_t)*MAX_PATH;
				Type = REG_SZ;
				Result = RegQueryValueEx(hKey, DebugShellPathValue, nullptr, &Type, reinterpret_cast<BYTE*>(DebugDllPath), &Len);
				if ((Result == ERROR_SUCCESS) && (Type == REG_SZ) && bDebug)
					TRACE(L"DebugActive() - debug path set\n");
			}

			RegCloseKey(hKey);
		}
	}
	else
	{
		Result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TGitRootKey, 0, KEY_READ, &hKey);
		if (Result == ERROR_SUCCESS)
		{
			Len = sizeof(wchar_t)*MAX_PATH;
			Type = REG_SZ;
			Result = RegQueryValueEx(hKey, DebugShellPathValue, nullptr, &Type, reinterpret_cast<BYTE*>(DebugDllPath), &Len);
			if ((Result == ERROR_SUCCESS) && (Type == REG_SZ) && bDebug)
				TRACE(L"DebugActive() - debug path set\n");
			RegCloseKey(hKey);
		}
	}

	TRACE(L"WantRealVersion() - Exit\n");
	return bDebugActive;
}

/**
 * \ingroup TortoiseShell
 * Check whether to load the full TortoiseGit.dll or not.
 */
static BOOL WantRealVersion(void)
{
	static const WCHAR TGitRootKey[] = L"Software\\TortoiseGit";
	static const WCHAR ExplorerOnlyValue[] = L"LoadDllOnlyInExplorer";

	static const WCHAR ExplorerEnvPath[] = L"%SystemRoot%\\explorer.exe";


	DWORD bExplorerOnly = 0;
	WCHAR ModuleName[MAX_PATH] = {0};

	HKEY hKey = HKEY_CURRENT_USER;
	DWORD Type = REG_DWORD;
	DWORD Len = sizeof(DWORD);

	BOOL bWantReal = TRUE;

	TRACE(L"WantRealVersion() - Enter\n");

	LONG Result = RegOpenKeyEx(HKEY_CURRENT_USER, TGitRootKey, 0, KEY_READ | KEY_WOW64_64KEY, &hKey);
	if (Result == ERROR_SUCCESS)
	{
		Result = RegQueryValueEx(hKey, ExplorerOnlyValue, nullptr, &Type, reinterpret_cast<BYTE*>(&bExplorerOnly), &Len);
		if ((Result == ERROR_SUCCESS) && (Type == REG_DWORD) && (Len == sizeof(DWORD)) && bExplorerOnly)
		{
			TRACE(L"WantRealVersion() - Explorer Only\n");

			// check if the current process is in fact the explorer
			Len = GetModuleFileName(nullptr, ModuleName, _countof(ModuleName));
			if (Len)
			{
				TRACE(L"Process is %s\n", ModuleName);

				WCHAR ExplorerPath[MAX_PATH] = {0};
				Len = ExpandEnvironmentStrings(ExplorerEnvPath, ExplorerPath, _countof(ExplorerPath));
				if (Len && (Len <= _countof(ExplorerPath)))
				{
					TRACE(L"Explorer path is %s\n", ExplorerPath);
					bWantReal = !lstrcmpi(ModuleName, ExplorerPath);
				}

				// we also have to allow the verclsid.exe process - that process determines
				// first whether the shell is allowed to even use an extension.
				Len = lstrlen(ModuleName);
				if ((Len > wcslen(L"\\verclsid.exe")) && (lstrcmpi(&ModuleName[Len - wcslen(L"\\verclsid.exe")], L"\\verclsid.exe") == 0))
					bWantReal = TRUE;
			}
		}

		RegCloseKey(hKey);
	}

	TRACE(L"WantRealVersion() - Exit\n");
	return bWantReal;
}

static void LoadRealLibrary(void)
{
	static const char GetClassObject[] = "DllGetClassObject";
	static const char CanUnloadNow[] = "DllCanUnloadNow";

	WCHAR ModuleName[MAX_PATH] = {0};
	DWORD Len = 0;
	HINSTANCE hUseInst = hInst;
	DebugDllPath[0] = 0;

	if (hTortoiseGit)
		return;

	if (!WantRealVersion())
	{
		TRACE(L"LoadRealLibrary() - Bypass\n");
		hTortoiseGit = NIL;
		return;
	}
	// if HKCU\Software\TortoiseGit\DebugShell is set, load the dlls from the location of the current process
	// which is for our debug purposes an instance of usually TortoiseProc. That way we can force the load
	// of the debug dlls.
	if (DebugActive())
		hUseInst = nullptr;
	Len = GetModuleFileName(hUseInst, ModuleName, _countof(ModuleName));
	if (!Len)
	{
		TRACE(L"LoadRealLibrary() - Fail\n");
		hTortoiseGit = NIL;
		return;
	}

	// truncate the string at the last '\' char
	while(Len > 0)
	{
		--Len;
		if (ModuleName[Len] == '\\')
		{
			ModuleName[Len] = L'\0';
			break;
		}
	}
	if (Len == 0)
	{
		TRACE(L"LoadRealLibrary() - Fail\n");
		hTortoiseGit = NIL;
		return;
	}
#ifdef _WIN64
	lstrcat(ModuleName, L"\\TortoiseGit.dll");
#else
	lstrcat(ModuleName, L"\\TortoiseGit32.dll");
#endif
	if (DebugDllPath[0])
		lstrcpy(ModuleName, DebugDllPath);
	TRACE(L"LoadRealLibrary() - Load %s\n", ModuleName);

	hTortoiseGit = LoadLibraryEx(ModuleName, nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
	if (!hTortoiseGit)
	{
		TRACE(L"LoadRealLibrary() - Fail\n");
		hTortoiseGit = NIL;
		return;
	}

	TRACE(L"LoadRealLibrary() - Success\n");
	pDllGetClassObject = nullptr;
	pDllCanUnloadNow = nullptr;
	pDllGetClassObject = reinterpret_cast<LPFNGETCLASSOBJECT>(GetProcAddress(hTortoiseGit, GetClassObject));
	if (!pDllGetClassObject)
	{
		TRACE(L"LoadRealLibrary() - Fail\n");
		FreeLibrary(hTortoiseGit);
		hTortoiseGit = NIL;
		return;
	}
	pDllCanUnloadNow = reinterpret_cast<LPFNCANUNLOADNOW>(GetProcAddress(hTortoiseGit, CanUnloadNow));
	if (!pDllCanUnloadNow)
	{
		TRACE(L"LoadRealLibrary() - Fail\n");
		FreeLibrary(hTortoiseGit);
		hTortoiseGit = NIL;
		return;
	}
}

static void UnloadRealLibrary(void)
{
	if (!hTortoiseGit)
		return;

	if (hTortoiseGit != NIL)
		FreeLibrary(hTortoiseGit);

	hTortoiseGit = nullptr;
	pDllGetClassObject = nullptr;
	pDllCanUnloadNow = nullptr;
}

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD Reason, LPVOID /*Reserved*/)
{
#ifdef _DEBUG
	// if no debugger is present, then don't load the dll.
	// this prevents other apps from loading the dll and locking
	// it.

	BOOL bInShellTest = FALSE;
	TCHAR buf[MAX_PATH + 1] = {0};       // MAX_PATH ok, the test really is for debugging anyway.
	DWORD pathLength = GetModuleFileName(nullptr, buf, _countof(buf) - 1);

	if (pathLength >= 14)
	{
		if ((lstrcmpi(&buf[pathLength-14], L"\\ShellTest.exe")) == 0)
		{
			bInShellTest = TRUE;
		}
		if ((_wcsicmp(&buf[pathLength-13], L"\\verclsid.exe")) == 0)
		{
			bInShellTest = TRUE;
		}
	}

	if (!IsDebuggerPresent() && !bInShellTest)
	{
		TRACE(L"In debug load preventer\n");
		return FALSE;
	}
#endif

	switch(Reason)
	{
	case DLL_PROCESS_ATTACH:
		hInst = hInstance;
		DebugDllPath[0] = 0;
		break;

	/*case DLL_THREAD_ATTACH:
		break;

	case DLL_THREAD_DETACH:
		break;

	case DLL_PROCESS_DETACH:
		break;*/
	}

	return TRUE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
	TRACE(L"DllGetClassObject() - Enter\n");

	LoadRealLibrary();
	if (!pDllGetClassObject)
	{
		if (ppv)
			*ppv = nullptr;

		TRACE(L"DllGetClassObject() - Bypass\n");
		return CLASS_E_CLASSNOTAVAILABLE;
	}

	TRACE(L"DllGetClassObject() - Forward\n");
	return pDllGetClassObject(rclsid, riid, ppv);
}

STDAPI DllCanUnloadNow(void)
{
	TRACE(L"DllCanUnloadNow() - Enter\n");

	if (pDllCanUnloadNow)
	{
		TRACE(L"DllCanUnloadNow() - Forward\n");
		HRESULT Result = pDllCanUnloadNow();
		if (Result != S_OK)
			return Result;
	}

	TRACE(L"DllCanUnloadNow() - Unload\n");
	UnloadRealLibrary();
	return S_OK;
}

