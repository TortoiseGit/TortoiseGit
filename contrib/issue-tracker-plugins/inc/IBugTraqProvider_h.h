

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 7.00.0500 */
/* at Sun Mar 08 16:07:08 2009
 */
/* Compiler settings for .\IBugTraqProvider.idl:
    Oicf, W4, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data
    VC __declspec() decoration level:
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __IBugTraqProvider_h_h__
#define __IBugTraqProvider_h_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */

#ifndef __IBugTraqProvider_FWD_DEFINED__
#define __IBugTraqProvider_FWD_DEFINED__
typedef interface IBugTraqProvider IBugTraqProvider;
#endif  /* __IBugTraqProvider_FWD_DEFINED__ */


#ifndef __IBugTraqProvider2_FWD_DEFINED__
#define __IBugTraqProvider2_FWD_DEFINED__
typedef interface IBugTraqProvider2 IBugTraqProvider2;
#endif  /* __IBugTraqProvider2_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif


/* interface __MIDL_itf_IBugTraqProvider_0000_0000 */
/* [local] */





extern RPC_IF_HANDLE __MIDL_itf_IBugTraqProvider_0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_IBugTraqProvider_0000_0000_v0_0_s_ifspec;

#ifndef __IBugTraqProvider_INTERFACE_DEFINED__
#define __IBugTraqProvider_INTERFACE_DEFINED__

/* interface IBugTraqProvider */
/* [unique][helpstring][uuid][object] */


EXTERN_C const IID IID_IBugTraqProvider;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("298B927C-7220-423C-B7B4-6E241F00CD93")
    IBugTraqProvider : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ValidateParameters(
            /* [in] */ HWND hParentWnd,
            /* [in] */ BSTR parameters,
            /* [retval][out] */ VARIANT_BOOL *valid) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetLinkText(
            /* [in] */ HWND hParentWnd,
            /* [in] */ BSTR parameters,
            /* [retval][out] */ BSTR *linkText) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetCommitMessage(
            /* [in] */ HWND hParentWnd,
            /* [in] */ BSTR parameters,
            /* [in] */ BSTR commonRoot,
            /* [in] */ SAFEARRAY * pathList,
            /* [in] */ BSTR originalMessage,
            /* [retval][out] */ BSTR *newMessage) = 0;

    };

#else   /* C style interface */

    typedef struct IBugTraqProviderVtbl
    {
        BEGIN_INTERFACE

        HRESULT ( STDMETHODCALLTYPE *QueryInterface )(
            IBugTraqProvider * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */
            __RPC__deref_out  void **ppvObject);

        ULONG ( STDMETHODCALLTYPE *AddRef )(
            IBugTraqProvider * This);

        ULONG ( STDMETHODCALLTYPE *Release )(
            IBugTraqProvider * This);

        HRESULT ( STDMETHODCALLTYPE *ValidateParameters )(
            IBugTraqProvider * This,
            /* [in] */ HWND hParentWnd,
            /* [in] */ BSTR parameters,
            /* [retval][out] */ VARIANT_BOOL *valid);

        HRESULT ( STDMETHODCALLTYPE *GetLinkText )(
            IBugTraqProvider * This,
            /* [in] */ HWND hParentWnd,
            /* [in] */ BSTR parameters,
            /* [retval][out] */ BSTR *linkText);

        HRESULT ( STDMETHODCALLTYPE *GetCommitMessage )(
            IBugTraqProvider * This,
            /* [in] */ HWND hParentWnd,
            /* [in] */ BSTR parameters,
            /* [in] */ BSTR commonRoot,
            /* [in] */ SAFEARRAY * pathList,
            /* [in] */ BSTR originalMessage,
            /* [retval][out] */ BSTR *newMessage);

        END_INTERFACE
    } IBugTraqProviderVtbl;

    interface IBugTraqProvider
    {
        CONST_VTBL struct IBugTraqProviderVtbl *lpVtbl;
    };



#ifdef COBJMACROS


#define IBugTraqProvider_QueryInterface(This,riid,ppvObject)    \
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) )

#define IBugTraqProvider_AddRef(This)   \
    ( (This)->lpVtbl -> AddRef(This) )

#define IBugTraqProvider_Release(This)  \
    ( (This)->lpVtbl -> Release(This) )


#define IBugTraqProvider_ValidateParameters(This,hParentWnd,parameters,valid)   \
    ( (This)->lpVtbl -> ValidateParameters(This,hParentWnd,parameters,valid) )

#define IBugTraqProvider_GetLinkText(This,hParentWnd,parameters,linkText)   \
    ( (This)->lpVtbl -> GetLinkText(This,hParentWnd,parameters,linkText) )

#define IBugTraqProvider_GetCommitMessage(This,hParentWnd,parameters,commonRoot,pathList,originalMessage,newMessage)    \
    ( (This)->lpVtbl -> GetCommitMessage(This,hParentWnd,parameters,commonRoot,pathList,originalMessage,newMessage) )

#endif /* COBJMACROS */


#endif  /* C style interface */




#endif  /* __IBugTraqProvider_INTERFACE_DEFINED__ */


#ifndef __IBugTraqProvider2_INTERFACE_DEFINED__
#define __IBugTraqProvider2_INTERFACE_DEFINED__

/* interface IBugTraqProvider2 */
/* [unique][helpstring][uuid][object] */


EXTERN_C const IID IID_IBugTraqProvider2;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("C5C85E31-2F9B-4916-A7BA-8E27D481EE83")
    IBugTraqProvider2 : public IBugTraqProvider
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetCommitMessage2(
            /* [in] */ HWND hParentWnd,
            /* [in] */ BSTR parameters,
            /* [in] */ BSTR commonURL,
            /* [in] */ BSTR commonRoot,
            /* [in] */ SAFEARRAY * pathList,
            /* [in] */ BSTR originalMessage,
            /* [in] */ BSTR bugID,
            /* [out] */ BSTR *bugIDOut,
            /* [out] */ SAFEARRAY * *revPropNames,
            /* [out] */ SAFEARRAY * *revPropValues,
            /* [retval][out] */ BSTR *newMessage) = 0;

        virtual HRESULT STDMETHODCALLTYPE CheckCommit(
            /* [in] */ HWND hParentWnd,
            /* [in] */ BSTR parameters,
            /* [in] */ BSTR commonURL,
            /* [in] */ BSTR commonRoot,
            /* [in] */ SAFEARRAY * pathList,
            /* [in] */ BSTR commitMessage,
            /* [retval][out] */ BSTR *errorMessage) = 0;

        virtual HRESULT STDMETHODCALLTYPE OnCommitFinished(
            /* [in] */ HWND hParentWnd,
            /* [in] */ BSTR commonRoot,
            /* [in] */ SAFEARRAY * pathList,
            /* [in] */ BSTR logMessage,
            /* [in] */ ULONG revision,
            /* [retval][out] */ BSTR *error) = 0;

        virtual HRESULT STDMETHODCALLTYPE HasOptions(
            /* [retval][out] */ VARIANT_BOOL *ret) = 0;

        virtual HRESULT STDMETHODCALLTYPE ShowOptionsDialog(
            /* [in] */ HWND hParentWnd,
            /* [in] */ BSTR parameters,
            /* [retval][out] */ BSTR *newparameters) = 0;

    };

#else   /* C style interface */

    typedef struct IBugTraqProvider2Vtbl
    {
        BEGIN_INTERFACE

        HRESULT ( STDMETHODCALLTYPE *QueryInterface )(
            IBugTraqProvider2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */
            __RPC__deref_out  void **ppvObject);

        ULONG ( STDMETHODCALLTYPE *AddRef )(
            IBugTraqProvider2 * This);

        ULONG ( STDMETHODCALLTYPE *Release )(
            IBugTraqProvider2 * This);

        HRESULT ( STDMETHODCALLTYPE *ValidateParameters )(
            IBugTraqProvider2 * This,
            /* [in] */ HWND hParentWnd,
            /* [in] */ BSTR parameters,
            /* [retval][out] */ VARIANT_BOOL *valid);

        HRESULT ( STDMETHODCALLTYPE *GetLinkText )(
            IBugTraqProvider2 * This,
            /* [in] */ HWND hParentWnd,
            /* [in] */ BSTR parameters,
            /* [retval][out] */ BSTR *linkText);

        HRESULT ( STDMETHODCALLTYPE *GetCommitMessage )(
            IBugTraqProvider2 * This,
            /* [in] */ HWND hParentWnd,
            /* [in] */ BSTR parameters,
            /* [in] */ BSTR commonRoot,
            /* [in] */ SAFEARRAY * pathList,
            /* [in] */ BSTR originalMessage,
            /* [retval][out] */ BSTR *newMessage);

        HRESULT ( STDMETHODCALLTYPE *GetCommitMessage2 )(
            IBugTraqProvider2 * This,
            /* [in] */ HWND hParentWnd,
            /* [in] */ BSTR parameters,
            /* [in] */ BSTR commonURL,
            /* [in] */ BSTR commonRoot,
            /* [in] */ SAFEARRAY * pathList,
            /* [in] */ BSTR originalMessage,
            /* [in] */ BSTR bugID,
            /* [out] */ BSTR *bugIDOut,
            /* [out] */ SAFEARRAY * *revPropNames,
            /* [out] */ SAFEARRAY * *revPropValues,
            /* [retval][out] */ BSTR *newMessage);

        HRESULT ( STDMETHODCALLTYPE *CheckCommit )(
            IBugTraqProvider2 * This,
            /* [in] */ HWND hParentWnd,
            /* [in] */ BSTR parameters,
            /* [in] */ BSTR commonURL,
            /* [in] */ BSTR commonRoot,
            /* [in] */ SAFEARRAY * pathList,
            /* [in] */ BSTR commitMessage,
            /* [retval][out] */ BSTR *errorMessage);

        HRESULT ( STDMETHODCALLTYPE *OnCommitFinished )(
            IBugTraqProvider2 * This,
            /* [in] */ HWND hParentWnd,
            /* [in] */ BSTR commonRoot,
            /* [in] */ SAFEARRAY * pathList,
            /* [in] */ BSTR logMessage,
            /* [in] */ ULONG revision,
            /* [retval][out] */ BSTR *error);

        HRESULT ( STDMETHODCALLTYPE *HasOptions )(
            IBugTraqProvider2 * This,
            /* [retval][out] */ VARIANT_BOOL *ret);

        HRESULT ( STDMETHODCALLTYPE *ShowOptionsDialog )(
            IBugTraqProvider2 * This,
            /* [in] */ HWND hParentWnd,
            /* [in] */ BSTR parameters,
            /* [retval][out] */ BSTR *newparameters);

        END_INTERFACE
    } IBugTraqProvider2Vtbl;

    interface IBugTraqProvider2
    {
        CONST_VTBL struct IBugTraqProvider2Vtbl *lpVtbl;
    };



#ifdef COBJMACROS


#define IBugTraqProvider2_QueryInterface(This,riid,ppvObject)   \
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) )

#define IBugTraqProvider2_AddRef(This)  \
    ( (This)->lpVtbl -> AddRef(This) )

#define IBugTraqProvider2_Release(This) \
    ( (This)->lpVtbl -> Release(This) )


#define IBugTraqProvider2_ValidateParameters(This,hParentWnd,parameters,valid)  \
    ( (This)->lpVtbl -> ValidateParameters(This,hParentWnd,parameters,valid) )

#define IBugTraqProvider2_GetLinkText(This,hParentWnd,parameters,linkText)  \
    ( (This)->lpVtbl -> GetLinkText(This,hParentWnd,parameters,linkText) )

#define IBugTraqProvider2_GetCommitMessage(This,hParentWnd,parameters,commonRoot,pathList,originalMessage,newMessage)   \
    ( (This)->lpVtbl -> GetCommitMessage(This,hParentWnd,parameters,commonRoot,pathList,originalMessage,newMessage) )


#define IBugTraqProvider2_GetCommitMessage2(This,hParentWnd,parameters,commonURL,commonRoot,pathList,originalMessage,bugID,bugIDOut,revPropNames,revPropValues,newMessage)  \
    ( (This)->lpVtbl -> GetCommitMessage2(This,hParentWnd,parameters,commonURL,commonRoot,pathList,originalMessage,bugID,bugIDOut,revPropNames,revPropValues,newMessage) )

#define IBugTraqProvider2_CheckCommit(This,hParentWnd,parameters,commonURL,commonRoot,pathList,commitMessage,errorMessage)  \
    ( (This)->lpVtbl -> CheckCommit(This,hParentWnd,parameters,commonURL,commonRoot,pathList,commitMessage,errorMessage) )

#define IBugTraqProvider2_OnCommitFinished(This,hParentWnd,commonRoot,pathList,logMessage,revision,error)   \
    ( (This)->lpVtbl -> OnCommitFinished(This,hParentWnd,commonRoot,pathList,logMessage,revision,error) )

#define IBugTraqProvider2_HasOptions(This,ret)  \
    ( (This)->lpVtbl -> HasOptions(This,ret) )

#define IBugTraqProvider2_ShowOptionsDialog(This,hParentWnd,parameters,newparameters)   \
    ( (This)->lpVtbl -> ShowOptionsDialog(This,hParentWnd,parameters,newparameters) )

#endif /* COBJMACROS */


#endif  /* C style interface */




#endif  /* __IBugTraqProvider2_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * );
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * );
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * );
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * );

unsigned long             __RPC_USER  HWND_UserSize(     unsigned long *, unsigned long            , HWND * );
unsigned char * __RPC_USER  HWND_UserMarshal(  unsigned long *, unsigned char *, HWND * );
unsigned char * __RPC_USER  HWND_UserUnmarshal(unsigned long *, unsigned char *, HWND * );
void                      __RPC_USER  HWND_UserFree(     unsigned long *, HWND * );

unsigned long             __RPC_USER  LPSAFEARRAY_UserSize(     unsigned long *, unsigned long            , LPSAFEARRAY * );
unsigned char * __RPC_USER  LPSAFEARRAY_UserMarshal(  unsigned long *, unsigned char *, LPSAFEARRAY * );
unsigned char * __RPC_USER  LPSAFEARRAY_UserUnmarshal(unsigned long *, unsigned char *, LPSAFEARRAY * );
void                      __RPC_USER  LPSAFEARRAY_UserFree(     unsigned long *, LPSAFEARRAY * );

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


