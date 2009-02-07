// TortoiseOverlays - an overlay handler for Tortoise clients
// Copyright (C) 2007 - TortoiseSVN
#include "stdafx.h"
#include "ShellExt.h"
#include "ShellExtClassFactory.h"


CShellExtClassFactory::CShellExtClassFactory(FileState state)
{
    m_StateToMake = state;

    m_cRef = 0L;
	
    g_cRefThisDll++; 
}

CShellExtClassFactory::~CShellExtClassFactory()          
{
    g_cRefThisDll--;
}

STDMETHODIMP CShellExtClassFactory::QueryInterface(REFIID riid,
                                                   LPVOID FAR *ppv)
{
    *ppv = NULL;

    // Any interface on this object is the object pointer
	
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory))
    {
        *ppv = (LPCLASSFACTORY)this;
		
        AddRef();
		
        return NOERROR;
    }
	
    return E_NOINTERFACE;
}  

STDMETHODIMP_(ULONG) CShellExtClassFactory::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CShellExtClassFactory::Release()
{
    if (--m_cRef)
        return m_cRef;

    delete this;
	
    return 0L;
}

STDMETHODIMP CShellExtClassFactory::CreateInstance(LPUNKNOWN pUnkOuter,
												   REFIID riid,
												   LPVOID *ppvObj)
{
    *ppvObj = NULL;
	
    // Shell extensions typically don't support aggregation (inheritance)
	
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;
	
    // Create the main shell extension object.  The shell will then call
    // QueryInterface with IID_IShellExtInit--this is how shell extensions are
    // initialized.
	
    CShellExt* pShellExt = new CShellExt(m_StateToMake);  //Create the CShellExt object
		
    if (NULL == pShellExt)
        return E_OUTOFMEMORY;
	
    return pShellExt->QueryInterface(riid, ppvObj);
}


STDMETHODIMP CShellExtClassFactory::LockServer(BOOL /*fLock*/)
{
    return E_NOTIMPL;
}
