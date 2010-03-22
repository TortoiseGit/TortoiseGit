// TortoiseOverlays - an overlay handler for Tortoise clients
// Copyright (C) 2007 - TortoiseSVN
#include "stdafx.h"

#pragma warning (disable : 4786)

// Initialize GUIDs (should be done only and at-least once per DLL/EXE)
#include <initguid.h>
#include "Guids.h"

#include "ShellExt.h"


// *********************** CShellExt *************************
CShellExt::CShellExt(FileState state)
{
    m_State = state;

    m_cRef = 0L;
	InterlockedIncrement(g_cRefThisDll);	
}

CShellExt::~CShellExt()
{
	InterlockedDecrement(g_cRefThisDll);
}


STDMETHODIMP CShellExt::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
    if(ppv == 0)
		return E_POINTER;
	*ppv = NULL; 

    if (IsEqualIID(riid, IID_IShellExtInit) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (LPSHELLEXTINIT)this;
    }
    else if (IsEqualIID(riid, IID_IContextMenu))
    {
        *ppv = (LPCONTEXTMENU)this;
    }
    else if (IsEqualIID(riid, IID_IContextMenu2))
    {
        *ppv = (LPCONTEXTMENU2)this;
    }
    else if (IsEqualIID(riid, IID_IContextMenu3))
    {
        *ppv = (LPCONTEXTMENU3)this;
    }
    else if (IsEqualIID(riid, IID_IShellIconOverlayIdentifier))
    {
        *ppv = (IShellIconOverlayIdentifier*)this;
    }
    else if (IsEqualIID(riid, IID_IShellPropSheetExt))
    {
        *ppv = (LPSHELLPROPSHEETEXT)this;
    }
    if (*ppv)
    {
        AddRef();
		
        return NOERROR;
    }
	
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CShellExt::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CShellExt::Release()
{
    if (--m_cRef)
        return m_cRef;

	for (vector<DLLPointers>::iterator it = m_dllpointers.begin(); it != m_dllpointers.end(); ++it)
	{
		if (it->pShellIconOverlayIdentifier != NULL)
			it->pShellIconOverlayIdentifier->Release();
		if (it->hDll != NULL)
			FreeLibrary(it->hDll);

		it->hDll = NULL;
		it->pDllGetClassObject = NULL;
		it->pDllCanUnloadNow = NULL;
		it->pShellIconOverlayIdentifier = NULL;
	}

	m_dllpointers.clear();

    delete this;
	
    return 0L;
}


