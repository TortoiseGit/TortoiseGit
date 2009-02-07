// TortoiseOverlays - an overlay handler for Tortoise clients
// Copyright (C) 2007 - TortoiseSVN
#pragma once


/**
 * This class factory object creates the main handlers -
 * its constructor says which OLE class it has to make.
 */
class CShellExtClassFactory : public IClassFactory
{
protected:
    ULONG m_cRef;
    /// variable to contain class of object (i.e. not under source control, up to date)
    FileState		m_StateToMake;

	
public:
    CShellExtClassFactory(FileState state);
    virtual ~CShellExtClassFactory();
	
	//@{
    /// IUnknown members
    STDMETHODIMP         QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
	//@}
    
	//@{
    /// IClassFactory members
    STDMETHODIMP      CreateInstance(LPUNKNOWN, REFIID, LPVOID FAR *);
    STDMETHODIMP      LockServer(BOOL);
	//@}
};
