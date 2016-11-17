#pragma once
#include "resource.h"       // main symbols
#include "ExampleAtlPlugin_i.h"
#include "..\inc\IBugTraqProvider_h.h"

class ATL_NO_VTABLE CProvider :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CProvider, &CLSID_Provider>,
    public IBugTraqProvider2
{
public:
    CProvider();

DECLARE_REGISTRY_RESOURCEID(IDR_PROVIDER)

DECLARE_NOT_AGGREGATABLE(CProvider)

BEGIN_COM_MAP(CProvider)
    COM_INTERFACE_ENTRY(IBugTraqProvider)
    COM_INTERFACE_ENTRY(IBugTraqProvider2)
END_COM_MAP()

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    HRESULT FinalConstruct();
    void FinalRelease();

// IBugTraqProvider
public:
    virtual HRESULT STDMETHODCALLTYPE ValidateParameters(
        /* [in] */ HWND hParentWnd,
        /* [in] */ BSTR parameters,
        /* [retval][out] */ VARIANT_BOOL *valid);

    virtual HRESULT STDMETHODCALLTYPE GetLinkText(
        /* [in] */ HWND hParentWnd,
        /* [in] */ BSTR parameters,
        /* [retval][out] */ BSTR *linkText);

    virtual HRESULT STDMETHODCALLTYPE GetCommitMessage(
        /* [in] */ HWND hParentWnd,
        /* [in] */ BSTR parameters,
        /* [in] */ BSTR commonRoot,
        /* [in] */ SAFEARRAY * pathList,
        /* [in] */ BSTR originalMessage,
        /* [retval][out] */ BSTR *newMessage);

    virtual HRESULT STDMETHODCALLTYPE GetCommitMessage2(
        /* [in] */ HWND hParentWnd,
        /* [in] */ BSTR parameters,
        /* [in] */ BSTR commonURL,
        /* [in] */ BSTR commonRoot,
        /* [in] */ SAFEARRAY * pathList,
        /* [in] */ BSTR originalMessage,
        /* [in] */ BSTR bugID,
        /* [out]*/ BSTR * bugIDOut,
        /* [out]*/ SAFEARRAY ** revPropNames,
        /* [out]*/ SAFEARRAY ** revPropValues,
        /* [retval][out] */ BSTR *newMessage);

    virtual HRESULT STDMETHODCALLTYPE CheckCommit (
        /* [in] */ HWND hParentWnd,
        /* [in] */ BSTR parameters,
        /* [in] */ BSTR commonURL,
        /* [in] */ BSTR commonRoot,
        /* [in] */ SAFEARRAY * pathList,
        /* [in] */ BSTR commitMessage,
        /* [out, retval] */ BSTR * errorMessage);

    virtual HRESULT STDMETHODCALLTYPE OnCommitFinished (
        /* [in] */ HWND hParentWnd,
        /* [in] */ BSTR commonRoot,
        /* [in] */ SAFEARRAY * pathList,
        /* [in] */ BSTR logMessage,
        /* [in] */ ULONG revision,
        /* [out, retval] */ BSTR * error);

    virtual HRESULT STDMETHODCALLTYPE HasOptions(
        /* [out, retval] */ VARIANT_BOOL *ret           // Whether the provider provides options
        );

    // this method is called if HasOptions() returned true before.
    // Use this to show a custom dialog so the user doesn't have to
    // create the parameters string manually
    virtual HRESULT STDMETHODCALLTYPE ShowOptionsDialog(
        /* [in] */ HWND hParentWnd,                 // Parent window for the options dialog
        /* [in] */ BSTR parameters,
        /* [out, retval] */ BSTR * newparameters    // the parameters string
        );

private:
    typedef std::map< CString, CString > parameters_t;
    parameters_t ParseParameters(BSTR parameters) const;
};

OBJECT_ENTRY_AUTO(__uuidof(Provider), CProvider)
