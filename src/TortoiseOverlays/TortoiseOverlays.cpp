// TortoiseOverlays - an overlay handler for Tortoise clients
// Copyright (C) 2007, 2010 - TortoiseSVN
#include "stdafx.h"
#include "ShellExt.h"
#include "Guids.h"
#include "ShellExtClassFactory.h"

volatile LONG		g_cRefThisDll = 0;				///< reference count of this DLL.
HINSTANCE			g_hmodThisDll = NULL;			///< handle to this DLL itself.


#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /* lpReserved */)
{
#ifdef _DEBUG
	// if no debugger is present, then don't load the dll.
	// this prevents other apps from loading the dll and locking
	// it.

	if (!::IsDebuggerPresent())
	{
		return FALSE;
	}
#endif

    if (dwReason == DLL_PROCESS_ATTACH)
    {
        // Extension DLL one-time initialization
        g_hmodThisDll = hInstance;
    }
    return 1;   // ok
}

STDAPI DllCanUnloadNow(void)
{
	return (g_cRefThisDll == 0 ? S_OK : S_FALSE);
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppvOut)
{
    if(ppvOut == 0)
		return E_POINTER;
	*ppvOut = NULL;
	
    FileState state = FileStateInvalid;
    if (IsEqualIID(rclsid, CLSID_Tortoise_NORMAL))
        state = FileStateNormal;
    else if (IsEqualIID(rclsid, CLSID_Tortoise_MODIFIED))
        state = FileStateModified;
    else if (IsEqualIID(rclsid, CLSID_Tortoise_CONFLICT))
        state = FileStateConflict;
	else if (IsEqualIID(rclsid, CLSID_Tortoise_DELETED))
		state = FileStateDeleted;
	else if (IsEqualIID(rclsid, CLSID_Tortoise_READONLY))
		state = FileStateReadOnly;
	else if (IsEqualIID(rclsid, CLSID_Tortoise_LOCKED))
		state = FileStateLocked;
	else if (IsEqualIID(rclsid, CLSID_Tortoise_ADDED))
		state = FileStateAdded;
	else if (IsEqualIID(rclsid, CLSID_Tortoise_IGNORED))
		state = FileStateIgnored;
	else if (IsEqualIID(rclsid, CLSID_Tortoise_UNVERSIONED))
		state = FileStateUnversioned;
	
    if (state != FileStateInvalid)
    {
		CShellExtClassFactory *pcf = new (std::nothrow) CShellExtClassFactory(state);
		if (pcf == NULL)
			return E_OUTOFMEMORY;

		return pcf->QueryInterface(riid, ppvOut);
    }
	
    return CLASS_E_CLASSNOTAVAILABLE;

}

