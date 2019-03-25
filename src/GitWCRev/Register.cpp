// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2017, 2019 - TortoiseGit
// Copyright (C) 2005-2008, 2010-2011, 2014 - TortoiseSVN

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

#include "stdafx.h"
#include <objbase.h> // ATL base
#include <atlbase.h>
#include <assert.h>

#include "Register.h"

//
// Internal helper functions prototypes
//

// Set the given key and its value.
BOOL setKeyAndValue(const TCHAR* pszPath, const TCHAR* szSubkey, const TCHAR* szValue);


BOOL setValue(const TCHAR* szKey, const TCHAR* szEntry, const TCHAR* szValue);


// Convert a CLSID into a char string.
void CLSIDtochar(const CLSID& clsid, TCHAR* szCLSID, int length);

// Delete szKeyChild and all of its descendants.
LONG recursiveDeleteKey(HKEY hKeyParent, const TCHAR* szKeyChild);

////////////////////////////////////////////////////////
//
// Constants
//

// Size of a CLSID as a string
const int CLSID_STRING_SIZE = 39;

/////////////////////////////////////////////////////////
//
// Public function implementation
//

//
// Register the component in the registry.
//
HRESULT RegisterServer(HMODULE hModule,            // DLL module handle
					   const CLSID& clsid,         // Class ID
					   const TCHAR* szFriendlyName, // Friendly Name
					   const TCHAR* szVerIndProgID, // Programmatic
					   const TCHAR* szProgID,
					   const CLSID& libid)       //   Type lib ID
{
	// Get server location.
	TCHAR szModule[1024] = { 0 };
	::GetModuleFileName(hModule, szModule, _countof(szModule) - 1);

	wcscat_s(szModule, L" /automation");
	// Convert the CLSID into a TCHAR.
	TCHAR szCLSID[CLSID_STRING_SIZE];
	CLSIDtochar(clsid, szCLSID, _countof(szCLSID));
	TCHAR szLIBID[CLSID_STRING_SIZE];
	CLSIDtochar(libid, szLIBID, _countof(szLIBID));

	// Build the key CLSID\\{...}
	TCHAR szKey[64];
	wcscpy_s(szKey, L"CLSID\\");
	wcscat_s(szKey, szCLSID);

	// Add the CLSID to the registry.
	setKeyAndValue(szKey, nullptr, szFriendlyName);

	// Add the server filename subkey under the CLSID key.
	setKeyAndValue(szKey, L"LocalServer32", szModule);

	// Add the ProgID subkey under the CLSID key.
	setKeyAndValue(szKey, L"ProgID", szProgID);

	// Add the version-independent ProgID subkey under CLSID key.
	setKeyAndValue(szKey, L"VersionIndependentProgID", szVerIndProgID);

	// Add the typelib
	setKeyAndValue(szKey, L"TypeLib", szLIBID);


	// Add the version-independent ProgID subkey under HKEY_CLASSES_ROOT.
	setKeyAndValue(szVerIndProgID, nullptr, szFriendlyName);
	setKeyAndValue(szVerIndProgID, L"CLSID", szCLSID);
	setKeyAndValue(szVerIndProgID, L"CurVer", szProgID);

	// Add the versioned ProgID subkey under HKEY_CLASSES_ROOT.
	setKeyAndValue(szProgID, nullptr, szFriendlyName);
	setKeyAndValue(szProgID, L"CLSID", szCLSID);

	// add TypeLib keys
	wcscpy_s(szKey, L"TypeLib\\");
	wcscat_s(szKey, szLIBID);

	// Add the CLSID to the registry.
	setKeyAndValue(szKey, nullptr, nullptr);
	wcscat_s(szKey, L"\\1.0");
	setKeyAndValue(szKey, nullptr, szFriendlyName);
	wcscat_s(szKey, L"\\0");
	setKeyAndValue(szKey, nullptr, nullptr);
	wcscat_s(szKey, L"\\win32");
	setKeyAndValue(szKey, nullptr, szModule);

	return S_OK;
}

void RegisterInterface(HMODULE hModule,            // DLL module handle
					   const CLSID& clsid,         // Class ID
					   const TCHAR* szFriendlyName, // Friendly Name
					   const CLSID& libid,
					   const IID& iid)
{
	// Get server location.
	TCHAR szModule[512] = { 0 };
	::GetModuleFileName(hModule, szModule, _countof(szModule) - 1);

	// Convert the CLSID into a TCHAR.
	TCHAR szCLSID[CLSID_STRING_SIZE];
	CLSIDtochar(clsid, szCLSID, _countof(szCLSID));
	TCHAR szLIBID[CLSID_STRING_SIZE];
	CLSIDtochar(libid, szLIBID, _countof(szCLSID));
	TCHAR szIID[CLSID_STRING_SIZE];
	CLSIDtochar(iid, szIID, _countof(szCLSID));

	// Build the key Interface\\{...}
	TCHAR szKey[64];
	wcscpy_s(szKey, L"Interface\\");
	wcscat_s(szKey, szIID);

	// Add the value to the registry.
	setKeyAndValue(szKey, nullptr, szFriendlyName);

	TCHAR szKey2[MAX_PATH] = { 0 };
	wcscpy_s(szKey2, szKey);
	wcscat_s(szKey2, L"\\ProxyStubClsID");
	// Add the server filename subkey under the IID key.
	setKeyAndValue(szKey2, nullptr, L"{00020424-0000-0000-C000-000000000046}"); //IUnknown

	wcscpy_s(szKey2, szKey);
	wcscat_s(szKey2, L"\\ProxyStubClsID32");
	// Add the server filename subkey under the IID key.
	setKeyAndValue(szKey2, nullptr, L"{00020424-0000-0000-C000-000000000046}"); //IUnknown

	wcscpy_s(szKey2, szKey);
	wcscat_s(szKey2, L"\\TypeLib");
	// Add the server filename subkey under the CLSID key.
	setKeyAndValue(szKey2, nullptr, szLIBID);

	setValue(szKey2, L"Version", L"1.0");
}

void UnregisterInterface(const IID& iid)
{
	TCHAR szIID[CLSID_STRING_SIZE];
	CLSIDtochar(iid, szIID, _countof(szIID));

	// Build the key Interface\\{...}
	TCHAR szKey[64];
	wcscpy_s(szKey, L"Interface\\");
	wcscat_s(szKey, szIID);

	recursiveDeleteKey(HKEY_CLASSES_ROOT, szKey);
}

//
// Remove the component from the registry.
//
LONG UnregisterServer(const CLSID& clsid,         // Class ID
					  const TCHAR* szVerIndProgID, // Programmatic
					  const TCHAR* szProgID,
					  const CLSID& libid)       //   Type lib ID
{
	// Convert the CLSID into a TCHAR.
	TCHAR szCLSID[CLSID_STRING_SIZE];
	CLSIDtochar(clsid, szCLSID, _countof(szCLSID));

	// Build the key CLSID\\{...}
	TCHAR szKey[64];
	wcscpy_s(szKey, L"CLSID\\");
	wcscat_s(szKey, szCLSID);

	// Delete the CLSID Key - CLSID\{...}
	LONG lResult = recursiveDeleteKey(HKEY_CLASSES_ROOT, szKey);
	assert((lResult == ERROR_SUCCESS) || (lResult == ERROR_FILE_NOT_FOUND)); // Subkey may not exist.

	// Delete the version-independent ProgID Key.
	lResult = recursiveDeleteKey(HKEY_CLASSES_ROOT, szVerIndProgID);
	assert((lResult == ERROR_SUCCESS) || (lResult == ERROR_FILE_NOT_FOUND)); // Subkey may not exist.

	// Delete the ProgID key.
	lResult = recursiveDeleteKey(HKEY_CLASSES_ROOT, szProgID);
	assert((lResult == ERROR_SUCCESS) || (lResult == ERROR_FILE_NOT_FOUND)); // Subkey may not exist.

	TCHAR szLIBID[CLSID_STRING_SIZE];
	CLSIDtochar(libid, szLIBID, _countof(szLIBID));

	wcscpy_s(szKey, L"TypeLib\\");
	wcscat_s(szKey, szLIBID);

	// Delete the TypeLib Key - LIBID\{...}
	lResult = recursiveDeleteKey(HKEY_CLASSES_ROOT, szKey);
	assert((lResult == ERROR_SUCCESS) || (lResult == ERROR_FILE_NOT_FOUND)); // Subkey may not exist.

	return S_OK;
}

///////////////////////////////////////////////////////////
//
// Internal helper functions
//

// Convert a CLSID to a TCHAR string.
void CLSIDtochar(const CLSID& clsid, TCHAR* szCLSID, int length)
{
	assert(length >= CLSID_STRING_SIZE);
	// Get CLSID
	CComHeapPtr<OLECHAR> wszCLSID;
	StringFromCLSID(clsid, &wszCLSID);

	// Covert from wide characters to non-wide.
	wcscpy_s(szCLSID, length, wszCLSID);
}

//
// Delete a key and all of its descendants.
//
LONG recursiveDeleteKey(HKEY hKeyParent,            // Parent of key to delete
						const TCHAR* lpszKeyChild)  // Key to delete
{
	// Open the child.
	HKEY hKeyChild;
	LONG lRes = RegOpenKeyEx(hKeyParent, lpszKeyChild, 0, KEY_ALL_ACCESS, &hKeyChild);
	if (lRes != ERROR_SUCCESS)
		return lRes;

	// Enumerate all of the descendants of this child.
	FILETIME time;
	TCHAR szBuffer[256];
	DWORD dwSize = _countof(szBuffer);
	while (RegEnumKeyEx(hKeyChild, 0, szBuffer, &dwSize, nullptr, nullptr, nullptr, &time) == S_OK)
	{
		// Delete the descendants of this child.
		lRes = recursiveDeleteKey(hKeyChild, szBuffer);
		if (lRes != ERROR_SUCCESS)
		{
			// Cleanup before exiting.
			RegCloseKey(hKeyChild);
			return lRes;
		}
		dwSize = _countof(szBuffer);
	}

	// Close the child.
	RegCloseKey(hKeyChild);

	// Delete this child.
	return RegDeleteKey(hKeyParent, lpszKeyChild);
}

//
// Create a key and set its value.
//   - This helper function was borrowed and modified from
//     Kraig Brockschmidt's book Inside OLE.
//
BOOL setKeyAndValue(const TCHAR* szKey, const TCHAR* szSubkey, const TCHAR* szValue)
{
	HKEY hKey;
	TCHAR szKeyBuf[1024];

	// Copy key name into buffer.
	wcscpy_s(szKeyBuf, szKey);

	// Add subkey name to buffer.
	if (szSubkey)
	{
		wcscat_s(szKeyBuf, L"\\");
		wcscat_s(szKeyBuf, szSubkey);
	}

	// Create and open key and subkey.
	long lResult = RegCreateKeyEx(HKEY_CLASSES_ROOT, szKeyBuf, 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, nullptr, &hKey, nullptr);
	if (lResult != ERROR_SUCCESS)
		return FALSE;

	// Set the Value.
	if (szValue)
		RegSetValueEx(hKey, nullptr, 0, REG_SZ, reinterpret_cast<BYTE*>(const_cast<TCHAR*>(szValue)), DWORD((1 + wcslen(szValue)) * sizeof(TCHAR)));

	RegCloseKey(hKey);
	return TRUE;
}

BOOL setValue(const TCHAR* szKey, const TCHAR* szEntry, const TCHAR* szValue)
{
	HKEY hKey;

	// Create and open key and subkey.
	long lResult = RegOpenKeyEx(HKEY_CLASSES_ROOT , szKey, 0, KEY_ALL_ACCESS, &hKey);
	if (lResult != ERROR_SUCCESS)
		return FALSE;

	// Set the Value.
	if (szValue)
		RegSetValueEx(hKey, szEntry, 0, REG_SZ, reinterpret_cast<BYTE*>(const_cast<TCHAR*>(szValue)), DWORD((1 + wcslen(szValue)) * sizeof(TCHAR)));

	RegCloseKey(hKey);

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// TypeLib registration

HRESULT LoadTypeLib(HINSTANCE hInstTypeLib, LPCOLESTR lpszIndex, BSTR* pbstrPath, ITypeLib** ppTypeLib)
{
	ATLASSERT(pbstrPath && ppTypeLib);
	if (!pbstrPath || !ppTypeLib)
		return E_POINTER;

	*pbstrPath = nullptr;
	*ppTypeLib = nullptr;

	USES_CONVERSION;
	ATLASSERT(hInstTypeLib);
	TCHAR szModule[MAX_PATH+10] = { 0 };

	DWORD dwFLen = ::GetModuleFileName(hInstTypeLib, szModule, MAX_PATH);
	if (dwFLen == 0)
		return HRESULT_FROM_WIN32(::GetLastError());
	else if (dwFLen == MAX_PATH)
		return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);

	// get the extension pointer in case of fail
	LPTSTR lpszExt = ::PathFindExtension(szModule);

	if (lpszIndex)
		lstrcat(szModule, OLE2CT(lpszIndex));
	LPOLESTR lpszModule = T2OLE(szModule);
	HRESULT hr = ::LoadTypeLib(lpszModule, ppTypeLib);
	if (FAILED(hr))
	{
		// typelib not in module, try <module>.tlb instead
		lstrcpy(lpszExt, L".tlb");
		lpszModule = T2OLE(szModule);
		hr = ::LoadTypeLib(lpszModule, ppTypeLib);
	}
	if (SUCCEEDED(hr))
	{
		*pbstrPath = ::SysAllocString(lpszModule);
		if (!*pbstrPath)
			hr = E_OUTOFMEMORY;
	}
	return hr;
}

static inline UINT WINAPI GetDirLen(LPCOLESTR lpszPathName) throw()
{
	ATLASSERT(lpszPathName);

	// always capture the complete file name including extension (if present)
	LPCOLESTR lpszTemp = lpszPathName;
	for (LPCOLESTR lpsz = lpszPathName; *lpsz;)
	{
		LPCOLESTR lp = CharNextW(lpsz);
		// remember last directory/drive separator
		if (*lpsz == OLESTR('\\') || *lpsz == OLESTR('/') || *lpsz == OLESTR(':'))
			lpszTemp = lp;
		lpsz = lp;
	}

	return UINT(lpszTemp-lpszPathName);
}

HRESULT RegisterTypeLib(HINSTANCE hInstTypeLib, LPCOLESTR lpszIndex)
{
	CComBSTR bstrPath;
	CComPtr<ITypeLib> pTypeLib;
	HRESULT hr = LoadTypeLib(hInstTypeLib, lpszIndex, &bstrPath, &pTypeLib);
	if (SUCCEEDED(hr))
	{
		OLECHAR szDir[MAX_PATH] = { 0 };
		ocscpy_s(szDir, _countof(szDir), bstrPath);
		// If index is specified remove it from the path
		if (lpszIndex)
		{
			size_t nLenPath = ocslen(szDir);
			size_t nLenIndex = ocslen(lpszIndex);
			if (memcmp(szDir + nLenPath - nLenIndex, lpszIndex, nLenIndex) == 0)
				szDir[nLenPath - nLenIndex] = 0;
		}
		szDir[GetDirLen(szDir)] = '\0';
		hr = ::RegisterTypeLib(pTypeLib, bstrPath, szDir);
	}
	return hr;
}

HRESULT UnRegisterTypeLib(HINSTANCE hInstTypeLib, LPCOLESTR lpszIndex)
{
	CComBSTR bstrPath;
	CComPtr<ITypeLib> pTypeLib;
	HRESULT hr = LoadTypeLib(hInstTypeLib, lpszIndex, &bstrPath, &pTypeLib);
	if (SUCCEEDED(hr))
	{
		TLIBATTR* ptla = 0;
		hr = pTypeLib->GetLibAttr(&ptla);
		if ((SUCCEEDED(hr)) && (ptla != 0))
		{
			hr = ::UnRegisterTypeLib(ptla->guid, ptla->wMajorVerNum, ptla->wMinorVerNum, ptla->lcid, ptla->syskind);
			pTypeLib->ReleaseTLibAttr(ptla);
		}
	}
	return hr;
}
