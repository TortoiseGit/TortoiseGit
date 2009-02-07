// TortoiseOverlays - an overlay handler for Tortoise clients
// Copyright (C) 2007 - TortoiseSVN
#include "stdafx.h"
#include "ShellExt.h"
#include "Guids.h"

// "The Shell calls IShellIconOverlayIdentifier::GetOverlayInfo to request the
//  location of the handler's icon overlay. The icon overlay handler returns
//  the name of the file containing the overlay image, and its index within
//  that file. The Shell then adds the icon overlay to the system image list."
STDMETHODIMP CShellExt::GetOverlayInfo(LPWSTR pwszIconFile, int cchMax, int *pIndex, DWORD *pdwFlags)
{
	int nInstalledOverlays = GetInstalledOverlays();
	
	if ((m_State == FileStateAdded)&&(nInstalledOverlays > 12))
		return S_FALSE;		// don't use the 'added' overlay
	if ((m_State == FileStateLocked)&&(nInstalledOverlays > 13))
		return S_FALSE;		// don't show the 'locked' overlay
	if ((m_State == FileStateIgnored)&&(nInstalledOverlays > 12))
		return S_FALSE;		// don't use the 'ignored' overlay
	if ((m_State == FileStateUnversioned)&&(nInstalledOverlays > 13))
		return S_FALSE;		// don't show the 'unversioned' overlay

    // Get folder icons from registry
	// Default icons are stored in LOCAL MACHINE
	// User selected icons are stored in CURRENT USER
	TCHAR regVal[1024];
	DWORD len = 1024;

	wstring icon;
	wstring iconName;

	HKEY hkeys [] = { HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE };
	switch (m_State)
	{
		case FileStateNormal		: iconName = _T("NormalIcon"); break;
		case FileStateModified		: iconName = _T("ModifiedIcon"); break;
		case FileStateConflict		: iconName = _T("ConflictIcon"); break;
		case FileStateDeleted		: iconName = _T("DeletedIcon"); break;
		case FileStateReadOnly		: iconName = _T("ReadOnlyIcon"); break;
		case FileStateLocked		: iconName = _T("LockedIcon"); break;
		case FileStateAdded			: iconName = _T("AddedIcon"); break;
		case FileStateIgnored		: iconName = _T("IgnoredIcon"); break;
		case FileStateUnversioned	: iconName = _T("UnversionedIcon"); break;
	}

	for (int i = 0; i < 2; ++i)
	{
		HKEY hkey = 0;

		if (::RegOpenKeyEx (hkeys[i],
			_T("Software\\TortoiseOverlays"),
                    0,
                    KEY_QUERY_VALUE,
                    &hkey) != ERROR_SUCCESS)
			continue;

		if (icon.empty() == true
			&& (::RegQueryValueEx (hkey,
							 iconName.c_str(),
							 NULL,
							 NULL,
							 (LPBYTE) regVal,
							 &len)) == ERROR_SUCCESS)
			icon.assign (regVal, len);

		::RegCloseKey(hkey);

	}

	// now load the Tortoise handlers and call their GetOverlayInfo method
	// note: we overwrite any changes a Tortoise handler will do to the
	// params and overwrite them later.
	LoadHandlers(pwszIconFile, cchMax, pIndex, pdwFlags);

    // Add name of appropriate icon
    if (icon.empty() == false)
        wcsncpy_s (pwszIconFile, cchMax, icon.c_str(), cchMax);
    else
        return S_FALSE;


    *pIndex = 0;
    *pdwFlags = ISIOI_ICONFILE;
    return S_OK;
};

STDMETHODIMP CShellExt::GetPriority(int *pPriority)
{
	switch (m_State)
	{
		case FileStateConflict:
			*pPriority = 0;
			break;
		case FileStateModified:
			*pPriority = 1;
			break;
		case FileStateDeleted:
			*pPriority = 2;
			break;
		case FileStateReadOnly:
			*pPriority = 3;
			break;
		case FileStateLocked:
			*pPriority = 4;
			break;
		case FileStateAdded:
			*pPriority = 5;
			break;
		case FileStateNormal:
			*pPriority = 6;
			break;
		case FileStateUnversioned:
			*pPriority = 8;
			break;
		case FileStateIgnored:
			*pPriority = 7;
			break;
		default:
			*pPriority = 100;
			return S_FALSE;
	}
    return S_OK;
}

STDMETHODIMP CShellExt::IsMemberOf(LPCWSTR pwszPath, DWORD dwAttrib)
{
	for (vector<DLLPointers>::iterator it = m_dllpointers.begin(); it != m_dllpointers.end(); ++it)
	{
		if (it->pShellIconOverlayIdentifier)
		{
			if (it->pShellIconOverlayIdentifier->IsMemberOf(pwszPath, dwAttrib) == S_OK)
				return S_OK;
		}
	}
	return S_FALSE;
}

int CShellExt::GetInstalledOverlays()
{
	// if there are more than 12 overlay handlers installed, then that means not all
	// of the overlay handlers can be shown. Windows chooses the ones first
	// returned by RegEnumKeyEx() and just drops the ones that come last in
	// that enumeration.
	int nInstalledOverlayhandlers = 0;
	// scan the registry for installed overlay handlers
	HKEY hKey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
		_T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellIconOverlayIdentifiers"),
		0, KEY_ENUMERATE_SUB_KEYS, &hKey)==ERROR_SUCCESS)
	{
		for (int i = 0, rc = ERROR_SUCCESS; rc == ERROR_SUCCESS; i++)
		{ 
			TCHAR value[1024];
			DWORD size = sizeof value / sizeof TCHAR;
			FILETIME last_write_time;
			rc = RegEnumKeyEx(hKey, i, value, &size, NULL, NULL, NULL, &last_write_time);
			if (rc == ERROR_SUCCESS) 
			{
				nInstalledOverlayhandlers++;
			}
		}
	}
	RegCloseKey(hKey);
	return nInstalledOverlayhandlers;
}

void CShellExt::LoadHandlers(LPWSTR pwszIconFile, int cchMax, int *pIndex, DWORD *pdwFlags)
{
	HKEY hKey;
	wstring name;
	switch (m_State)
	{
	case FileStateNormal		: name = _T("Software\\TortoiseOverlays\\Normal"); break;
	case FileStateModified		: name = _T("Software\\TortoiseOverlays\\Modified"); break;
	case FileStateConflict		: name = _T("Software\\TortoiseOverlays\\Conflict"); break;
	case FileStateDeleted		: name = _T("Software\\TortoiseOverlays\\Deleted"); break;
	case FileStateReadOnly		: name = _T("Software\\TortoiseOverlays\\ReadOnly"); break;
	case FileStateLocked		: name = _T("Software\\TortoiseOverlays\\Locked"); break;
	case FileStateAdded			: name = _T("Software\\TortoiseOverlays\\Added"); break;
	case FileStateIgnored		: name = _T("Software\\TortoiseOverlays\\Ignored"); break;
	case FileStateUnversioned	: name = _T("Software\\TortoiseOverlays\\Unversioned"); break;
	}
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
		name.c_str(),
		0, KEY_READ, &hKey)==ERROR_SUCCESS)
	{
		for (int i = 0, rc = ERROR_SUCCESS; rc == ERROR_SUCCESS; i++)
		{ 
			TCHAR k[MAX_PATH];
			TCHAR value[MAX_PATH];
			DWORD sizek = sizeof k / sizeof TCHAR;
			DWORD sizev = sizeof value / sizeof TCHAR;
			rc = RegEnumValue(hKey, i, k, &sizek, NULL, NULL, (LPBYTE)value, &sizev);
			if (rc == ERROR_SUCCESS) 
			{
				TCHAR comobj[MAX_PATH];
				TCHAR modpath[MAX_PATH];
				_tcscpy_s(comobj, MAX_PATH, _T("CLSID\\"));
				_tcscat_s(comobj, MAX_PATH, value);
				_tcscat_s(comobj, MAX_PATH, _T("\\InprocServer32"));
				if (SHRegGetPath(HKEY_CLASSES_ROOT, comobj, _T(""), modpath, 0) == ERROR_SUCCESS)
				{
					LoadRealLibrary(modpath, value, pwszIconFile, cchMax, pIndex, pdwFlags);
				}
			}
		}
	}
	RegCloseKey(hKey);
}

void CShellExt::LoadRealLibrary(LPCTSTR ModuleName, LPCTSTR clsid, LPWSTR pwszIconFile, int cchMax, int *pIndex, DWORD *pdwFlags)
{
	static const char GetClassObject[] = "DllGetClassObject";
	static const char CanUnloadNow[] = "DllCanUnloadNow";

	DLLPointers pointers;

	pointers.hDll = LoadLibraryEx(ModuleName, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
	if (!pointers.hDll)
	{
		pointers.hDll = NULL;
		return;
	}

	pointers.pDllGetClassObject = NULL;
	pointers.pDllCanUnloadNow = NULL;
	pointers.pDllGetClassObject = (LPFNGETCLASSOBJECT)GetProcAddress(pointers.hDll, GetClassObject);
	if (pointers.pDllGetClassObject == NULL)
	{
		FreeLibrary(pointers.hDll);
		pointers.hDll = NULL;
		return;
	}
	pointers.pDllCanUnloadNow = (LPFNCANUNLOADNOW)GetProcAddress(pointers.hDll, CanUnloadNow);
	if (pointers.pDllCanUnloadNow == NULL)
	{
		FreeLibrary(pointers.hDll);
		pointers.hDll = NULL;
		return;
	}

	IID c;
	if (IIDFromString((LPOLESTR)clsid, &c) == S_OK)
	{
		IClassFactory * pClassFactory = NULL;
		if (SUCCEEDED(pointers.pDllGetClassObject(c, IID_IClassFactory, (LPVOID*)&pClassFactory)))
		{
			IShellIconOverlayIdentifier * pShellIconOverlayIdentifier = NULL;
			if (SUCCEEDED(pClassFactory->CreateInstance(NULL, IID_IShellIconOverlayIdentifier, (LPVOID*)&pShellIconOverlayIdentifier)))
			{
				pointers.pShellIconOverlayIdentifier = pShellIconOverlayIdentifier;
				if (pointers.pShellIconOverlayIdentifier->GetOverlayInfo(pwszIconFile, cchMax, pIndex, pdwFlags) != S_OK)
				{
					// the overlay handler refused to be properly initialized
					FreeLibrary(pointers.hDll);
					pointers.hDll = NULL;
					pClassFactory->Release();
					pointers.pShellIconOverlayIdentifier->Release();
					return;
				}
			}
			pClassFactory->Release();
		}
	}

	m_dllpointers.push_back(pointers);
}



