// TortoiseSI - a Windows shell extension for easy version control

// Copyright (C) 2012-2014 - TortoiseGit
// Copyright (C) 2007, 2009, 2013-2014 - TortoiseSVN
// Copyright (C) 2015 - TortoiseSI 

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
#include <string>

const HINSTANCE NIL = (HINSTANCE)((char*)(0) - 1);

static HINSTANCE hInst = NULL;

static HINSTANCE hTortoiseSI = NULL;
static LPFNGETCLASSOBJECT pDllGetClassObject = NULL;
static LPFNCANUNLOADNOW pDllCanUnloadNow = NULL;

static const WCHAR TSIRootKey[] = L"Software\\TortoiseSI";
static const WCHAR ExplorerEnvPath[] = L"%SystemRoot%\\explorer.exe";

void writeEvent(std::wstring message, WORD wType) {
	HANDLE hEventSource = NULL;
	LPCWSTR lpszStrings[2] = { NULL, NULL };
	const wchar_t *name = L"TortoiseSI";

	hEventSource = RegisterEventSource(NULL, name);
	if (hEventSource) {
		lpszStrings[0] = name;
		lpszStrings[1] = message.c_str();

		ReportEvent(hEventSource,		// Event log handle
			wType,						// Event type
			0,							// Event category
			0,							// Event identifier
			NULL,						// No security identifier
			ARRAYSIZE(lpszStrings),		// Size of lpszStrings array
			0,							// No binary data
			lpszStrings,				// Array of strings
			NULL						// No binary data
			);

		DeregisterEventSource(hEventSource);
	}
}

//
//   FUNCTION: EventLog::writeErrorLogEntry(wstring)
//
//   PURPOSE: Log an ERROR type message to the Application event log.
//
//   PARAMETERS:
//   * error - the error message
//
void writeError(std::wstring error) {
	writeEvent(error, EVENTLOG_ERROR_TYPE);
}
//
//   FUNCTION: EventLog::writeInformationLogEntry(wstring)
//
//   PURPOSE: Log an INFORMATION type message to the Application event log.
//
//   PARAMETERS:
//   * info - the information message
//
void writeInformation(std::wstring info) {
	writeEvent(info, EVENTLOG_INFORMATION_TYPE);
}

static BOOL IsExplorer()
{
	WCHAR ModuleName[MAX_PATH] = { 0 };
	BOOL isExplorer = FALSE;

	// check if the current process is in fact the explorer
	DWORD Len = GetModuleFileName(NULL, ModuleName, _countof(ModuleName));
	if (Len)
	{
		WCHAR ExplorerPath[MAX_PATH] = { 0 };
		Len = ExpandEnvironmentStrings(ExplorerEnvPath, ExplorerPath, _countof(ExplorerPath));
		if (Len && (Len <= _countof(ExplorerPath)))
		{
			isExplorer = !lstrcmpi(ModuleName, ExplorerPath);
		}

		// we also have to allow the verclsid.exe process - that process determines
		// first whether the shell is allowed to even use an extension.
		Len = lstrlen(ModuleName);
		if ((Len > 13) && (lstrcmpi(&ModuleName[Len - 13], L"\\verclsid.exe")) == 0) {
			isExplorer = TRUE;
		}
	}
	return isExplorer;
}

/**
* \ingroup TortoiseShell
* Check whether to load the full TortoiseSI.dll or not.
*/
static BOOL WantRealVersion()
{
	static const WCHAR ExplorerOnlyValue[] = L"LoadDllOnlyInExplorer";

	DWORD bExplorerOnly = 0;

	HKEY hKey = HKEY_CURRENT_USER;
	LONG Result = ERROR;
	DWORD Type = REG_DWORD;
	DWORD Len = sizeof(DWORD);

	BOOL bWantReal = TRUE;

	Result = RegOpenKeyEx(HKEY_CURRENT_USER, TSIRootKey, 0, KEY_READ, &hKey);
	if (Result == ERROR_SUCCESS)
	{
		Result = RegQueryValueEx(hKey, ExplorerOnlyValue, NULL, &Type, (BYTE *)&bExplorerOnly, &Len);
		if ((Result == ERROR_SUCCESS) && (Type == REG_DWORD) && (Len == sizeof(DWORD)) && bExplorerOnly)
		{
			// check if the current process is in fact the explorer
			bWantReal = IsExplorer();
		}

		RegCloseKey(hKey);
	}
	return bWantReal;
}

static std::wstring PathToDll()
{
	WCHAR dllPath[MAX_PATH] = { 0 };
#ifdef _WIN64
	WCHAR dubugDllPathKey[] = L"DebugDllLocation";
#else
	WCHAR dubugDllPathKey[] = L"DebugDll32Location";
#endif

	HKEY hKey = HKEY_CURRENT_USER;
	LONG Result = ERROR;
	DWORD Type = REG_SZ;
	DWORD length = sizeof(dllPath);
	std::wstring path;

	Result = RegOpenKeyEx(HKEY_CURRENT_USER, TSIRootKey, 0, KEY_READ, &hKey);
	if (Result == ERROR_SUCCESS)
	{
		Result = RegQueryValueEx(hKey, dubugDllPathKey, NULL, &Type, (BYTE *)&dllPath, &length);
		length = length / 2; // 2 bytes per character

		if ((Result == ERROR_SUCCESS) && (Type == REG_SZ) && (length > 0) && dllPath)
		{
			// don't include the null terminating character, wstring adds one for us
			if (dllPath[length - 1] == '\0') {
				length--;
			}

			RegCloseKey(hKey);
			std::wstring path = std::wstring(dllPath, length);

			if (path[path.length() - 1] != '\\' || path[path.length() - 1] != '/') {
				path += L"\\";
			}
			return path;
		}

		RegCloseKey(hKey);
	}

	// use same folder location as this dll
	length = GetModuleFileName(hInst, dllPath, _countof(dllPath));
	if (!length)
	{
		writeError(L"PathToDll - failed to get location of TortoiseStub");
		return L"";
	}

	// truncate the string at the last '\' char
	while (length > 0)
	{
		--length;
		if (dllPath[length] == '\\')
		{
			// don't set the null terminating character, wstring does it for us
			break;
		}
	}

	if (length == 0)
	{
		writeError(L"PathToDll - failed to get location of " + std::wstring(dllPath));
		return L"";
	}

	return std::wstring(dllPath, length+1);
}

std::wstring getProcessFilesName()
{
	WCHAR moduleName[MAX_PATH] = { 0 };
	GetModuleFileName(NULL, moduleName, _countof(moduleName));
	return std::wstring(moduleName);
}


static void LoadRealLibrary(void)
{
	static const char GetClassObject[] = "DllGetClassObject";
	static const char CanUnloadNow[] = "DllCanUnloadNow";

	if (hTortoiseSI)
		return;

	if (!WantRealVersion())
	{
		writeInformation(L"not loading TortoiseSI, loading only enabled for explorer.exe, current module = " + getProcessFilesName());
		hTortoiseSI = NIL;
		return;
	}
	// if HKCU\Software\TortoiseSI\DebugShell is set, load the dlls from the location of the current process
	// which is for our debug purposes an instance of usually TortoiseProc. That way we can force the load
	// of the debug dlls.
	//	if (DebugActive()) {
	//	hUseInst = NULL;
	//}

	std::wstring path = PathToDll();

	if (path.size() == 0) {
		hTortoiseSI = NIL;
		return;
	}

#ifdef _WIN64
	path += L"TortoiseSI.dll";
#else
	path += L"TortoiseSI32.dll";
#endif
	writeInformation(L"attempting to load dll = " + path);

	hTortoiseSI = LoadLibraryEx(path.c_str(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
	if (!hTortoiseSI)
	{
		writeError(L"attempting to load dll = " + path + L" fail!");
		hTortoiseSI = NIL;
		return;
	}

	pDllGetClassObject = NULL;
	pDllCanUnloadNow = NULL;
	pDllGetClassObject = (LPFNGETCLASSOBJECT)GetProcAddress(hTortoiseSI, GetClassObject);
	if (pDllGetClassObject == NULL)
	{
		writeError(L"failed to find DllGetClassObject function");
		FreeLibrary(hTortoiseSI);
		hTortoiseSI = NIL;
		return;
	}
	pDllCanUnloadNow = (LPFNCANUNLOADNOW)GetProcAddress(hTortoiseSI, CanUnloadNow);
	if (pDllCanUnloadNow == NULL)
	{
		writeError(L"failed to find DllCanUnloadNow function");
		FreeLibrary(hTortoiseSI);
		hTortoiseSI = NIL;
		return;
	}
}

static void UnloadRealLibrary(void)
{
	if (!hTortoiseSI)
		return;

	if (hTortoiseSI != NIL)
		FreeLibrary(hTortoiseSI);

	hTortoiseSI = NULL;
	pDllGetClassObject = NULL;
	pDllCanUnloadNow = NULL;
}

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD Reason, LPVOID /*Reserved*/)
{
#ifdef _DEBUG
	// if no debugger is present, then don't load the dll.
	// this prevents other apps from loading the dll and locking
	// it.

	if (!IsDebuggerPresent() && !IsExplorer())
	{
		writeInformation(L"debugger not present, not loading dll");
		return FALSE;
	}
#endif

	switch (Reason)
	{
	case DLL_PROCESS_ATTACH:
		hInst = hInstance;
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
	LoadRealLibrary();
	if (!pDllGetClassObject)
	{
		if (ppv)
			*ppv = NULL;

		return CLASS_E_CLASSNOTAVAILABLE;
	}

	return pDllGetClassObject(rclsid, riid, ppv);
}

STDAPI DllCanUnloadNow(void)
{
	if (pDllCanUnloadNow)
	{
		HRESULT Result = pDllCanUnloadNow();
		if (Result != S_OK)
			return Result;
	}

	UnloadRealLibrary();
	return S_OK;
}

