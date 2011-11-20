// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2010 - TortoiseSVN

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

// helper:
// declares and defines stuff which is not available in the Vista SDK or
// which isn't available in the Win7 SDK but not unless NTDDI_VERSION is
// set to NTDDI_WIN7

#pragma once

static UINT	WM_TASKBARBTNCREATED = RegisterWindowMessage(_T("TaskbarButtonCreated"));

#if (NTDDI_VERSION < 0x06010000)

/*
* Message filter info values (CHANGEFILTERSTRUCT.ExtStatus)
*/
#define MSGFLTINFO_NONE                         (0)
#define MSGFLTINFO_ALREADYALLOWED_FORWND        (1)
#define MSGFLTINFO_ALREADYDISALLOWED_FORWND     (2)
#define MSGFLTINFO_ALLOWED_HIGHER               (3)

typedef struct tagCHANGEFILTERSTRUCT {
    DWORD cbSize;
    DWORD ExtStatus;
} CHANGEFILTERSTRUCT, *PCHANGEFILTERSTRUCT;

/*
* Message filter action values (action parameter to ChangeWindowMessageFilterEx)
*/
#define MSGFLT_RESET                            (0)
#define MSGFLT_ALLOW                            (1)
#define MSGFLT_DISALLOW                         (2)

#ifdef __cplusplus

// Define operator overloads to enable bit operations on enum values that are
// used to define flags. Use DEFINE_ENUM_FLAG_OPERATORS(YOUR_TYPE) to enable these
// operators on YOUR_TYPE.

// Moved here from objbase.w.

#define DEFINE_ENUM_FLAG_OPERATORS(ENUMTYPE) \
    extern "C++" { \
    inline ENUMTYPE operator | (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((int)a) | ((int)b)); } \
    inline ENUMTYPE &operator |= (ENUMTYPE &a, ENUMTYPE b) { return (ENUMTYPE &)(((int &)a) |= ((int)b)); } \
    inline ENUMTYPE operator & (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((int)a) & ((int)b)); } \
    inline ENUMTYPE &operator &= (ENUMTYPE &a, ENUMTYPE b) { return (ENUMTYPE &)(((int &)a) &= ((int)b)); } \
    inline ENUMTYPE operator ~ (ENUMTYPE a) { return ENUMTYPE(~((int)a)); } \
    inline ENUMTYPE operator ^ (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((int)a) ^ ((int)b)); } \
    inline ENUMTYPE &operator ^= (ENUMTYPE &a, ENUMTYPE b) { return (ENUMTYPE &)(((int &)a) ^= ((int)b)); } \
}
#else
#define DEFINE_ENUM_FLAG_OPERATORS(ENUMTYPE) // NOP, C allows these operators.
#endif

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 7.00.0555 */
/* Compiler settings for objectarray.idl:
    Oicf, W1, Zp8, env=Win32 (32b run), target_arch=X86 7.00.0555
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data
    VC __declspec() decoration level:
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 500
#endif

/* verify that the <rpcsal.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCSAL_H_VERSION__
#define __REQUIRED_RPCSAL_H_VERSION__ 100
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

#ifndef __objectarray_h__
#define __objectarray_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */

#ifndef __IObjectArray_FWD_DEFINED__
#define __IObjectArray_FWD_DEFINED__
typedef interface IObjectArray IObjectArray;
#endif  /* __IObjectArray_FWD_DEFINED__ */


#ifndef __IObjectCollection_FWD_DEFINED__
#define __IObjectCollection_FWD_DEFINED__
typedef interface IObjectCollection IObjectCollection;
#endif  /* __IObjectCollection_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif


    /* interface __MIDL_itf_shobjidl_0000_0093 */
    /* [local] */

#ifdef MIDL_PASS
    typedef IUnknown *HIMAGELIST;

#endif
    typedef /* [v1_enum] */
        enum THUMBBUTTONFLAGS
    {   THBF_ENABLED    = 0,
    THBF_DISABLED   = 0x1,
    THBF_DISMISSONCLICK = 0x2,
    THBF_NOBACKGROUND   = 0x4,
    THBF_HIDDEN = 0x8,
    THBF_NONINTERACTIVE = 0x10
    }   THUMBBUTTONFLAGS;

    DEFINE_ENUM_FLAG_OPERATORS(THUMBBUTTONFLAGS)
        typedef /* [v1_enum] */
        enum THUMBBUTTONMASK
    {   THB_BITMAP  = 0x1,
    THB_ICON    = 0x2,
    THB_TOOLTIP = 0x4,
    THB_FLAGS   = 0x8
    }   THUMBBUTTONMASK;

    DEFINE_ENUM_FLAG_OPERATORS(THUMBBUTTONMASK)
#include <pshpack8.h>
        typedef struct THUMBBUTTON
    {
        THUMBBUTTONMASK dwMask;
        UINT iId;
        UINT iBitmap;
        HICON hIcon;
        WCHAR szTip[ 260 ];
        THUMBBUTTONFLAGS dwFlags;
    }   THUMBBUTTON;

    typedef struct THUMBBUTTON *LPTHUMBBUTTON;

#include <poppack.h>
#define THBN_CLICKED        0x1800


    extern RPC_IF_HANDLE __MIDL_itf_shobjidl_0000_0093_v0_0_c_ifspec;
    extern RPC_IF_HANDLE __MIDL_itf_shobjidl_0000_0093_v0_0_s_ifspec;

#ifndef __ITaskbarList3_INTERFACE_DEFINED__
#define __ITaskbarList3_INTERFACE_DEFINED__

    /* interface ITaskbarList3 */
    /* [object][uuid] */

    typedef /* [v1_enum] */
        enum TBPFLAG
    {   TBPF_NOPROGRESS = 0,
    TBPF_INDETERMINATE  = 0x1,
    TBPF_NORMAL = 0x2,
    TBPF_ERROR  = 0x4,
    TBPF_PAUSED = 0x8
    }   TBPFLAG;

    DEFINE_ENUM_FLAG_OPERATORS(TBPFLAG)

        EXTERN_C const IID IID_ITaskbarList3;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("ea1afb91-9e28-4b86-90e9-9e9f8a5eefaf")
ITaskbarList3 : public ITaskbarList2
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetProgressValue(
            /* [in] */ __RPC__in HWND hwnd,
            /* [in] */ ULONGLONG ullCompleted,
            /* [in] */ ULONGLONG ullTotal) = 0;

        virtual HRESULT STDMETHODCALLTYPE SetProgressState(
            /* [in] */ __RPC__in HWND hwnd,
            /* [in] */ TBPFLAG tbpFlags) = 0;

        virtual HRESULT STDMETHODCALLTYPE RegisterTab(
            /* [in] */ __RPC__in HWND hwndTab,
            /* [in] */ __RPC__in HWND hwndMDI) = 0;

        virtual HRESULT STDMETHODCALLTYPE UnregisterTab(
            /* [in] */ __RPC__in HWND hwndTab) = 0;

        virtual HRESULT STDMETHODCALLTYPE SetTabOrder(
            /* [in] */ __RPC__in HWND hwndTab,
            /* [in] */ __RPC__in HWND hwndInsertBefore) = 0;

        virtual HRESULT STDMETHODCALLTYPE SetTabActive(
            /* [in] */ __RPC__in HWND hwndTab,
            /* [in] */ __RPC__in HWND hwndMDI,
            /* [in] */ DWORD dwReserved) = 0;

        virtual HRESULT STDMETHODCALLTYPE ThumbBarAddButtons(
            /* [in] */ __RPC__in HWND hwnd,
            /* [in] */ UINT cButtons,
            /* [size_is][in] */ __RPC__in_ecount_full(cButtons) LPTHUMBBUTTON pButton) = 0;

        virtual HRESULT STDMETHODCALLTYPE ThumbBarUpdateButtons(
            /* [in] */ __RPC__in HWND hwnd,
            /* [in] */ UINT cButtons,
            /* [size_is][in] */ __RPC__in_ecount_full(cButtons) LPTHUMBBUTTON pButton) = 0;

        virtual HRESULT STDMETHODCALLTYPE ThumbBarSetImageList(
            /* [in] */ __RPC__in HWND hwnd,
            /* [in] */ __RPC__in_opt HIMAGELIST himl) = 0;

        virtual HRESULT STDMETHODCALLTYPE SetOverlayIcon(
            /* [in] */ __RPC__in HWND hwnd,
            /* [in] */ __RPC__in HICON hIcon,
            /* [string][unique][in] */ __RPC__in_opt_string LPCWSTR pszDescription) = 0;

        virtual HRESULT STDMETHODCALLTYPE SetThumbnailTooltip(
            /* [in] */ __RPC__in HWND hwnd,
            /* [string][unique][in] */ __RPC__in_opt_string LPCWSTR pszTip) = 0;

        virtual HRESULT STDMETHODCALLTYPE SetThumbnailClip(
            /* [in] */ __RPC__in HWND hwnd,
            /* [in] */ __RPC__in RECT *prcClip) = 0;

    };

#else   /* C style interface */

    typedef struct ITaskbarList3Vtbl
    {
        BEGIN_INTERFACE

            HRESULT ( STDMETHODCALLTYPE *QueryInterface )(
            __RPC__in ITaskbarList3 * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [annotation][iid_is][out] */
            __RPC__deref_out  void **ppvObject);

            ULONG ( STDMETHODCALLTYPE *AddRef )(
                __RPC__in ITaskbarList3 * This);

            ULONG ( STDMETHODCALLTYPE *Release )(
                __RPC__in ITaskbarList3 * This);

            HRESULT ( STDMETHODCALLTYPE *HrInit )(
                __RPC__in ITaskbarList3 * This);

            HRESULT ( STDMETHODCALLTYPE *AddTab )(
                __RPC__in ITaskbarList3 * This,
                /* [in] */ __RPC__in HWND hwnd);

            HRESULT ( STDMETHODCALLTYPE *DeleteTab )(
                __RPC__in ITaskbarList3 * This,
                /* [in] */ __RPC__in HWND hwnd);

            HRESULT ( STDMETHODCALLTYPE *ActivateTab )(
                __RPC__in ITaskbarList3 * This,
                /* [in] */ __RPC__in HWND hwnd);

            HRESULT ( STDMETHODCALLTYPE *SetActiveAlt )(
                __RPC__in ITaskbarList3 * This,
                /* [in] */ __RPC__in HWND hwnd);

            HRESULT ( STDMETHODCALLTYPE *MarkFullscreenWindow )(
                __RPC__in ITaskbarList3 * This,
                /* [in] */ __RPC__in HWND hwnd,
                /* [in] */ BOOL fFullscreen);

            HRESULT ( STDMETHODCALLTYPE *SetProgressValue )(
                __RPC__in ITaskbarList3 * This,
                /* [in] */ __RPC__in HWND hwnd,
                /* [in] */ ULONGLONG ullCompleted,
                /* [in] */ ULONGLONG ullTotal);

            HRESULT ( STDMETHODCALLTYPE *SetProgressState )(
                __RPC__in ITaskbarList3 * This,
                /* [in] */ __RPC__in HWND hwnd,
                /* [in] */ TBPFLAG tbpFlags);

            HRESULT ( STDMETHODCALLTYPE *RegisterTab )(
                __RPC__in ITaskbarList3 * This,
                /* [in] */ __RPC__in HWND hwndTab,
                /* [in] */ __RPC__in HWND hwndMDI);

            HRESULT ( STDMETHODCALLTYPE *UnregisterTab )(
                __RPC__in ITaskbarList3 * This,
                /* [in] */ __RPC__in HWND hwndTab);

            HRESULT ( STDMETHODCALLTYPE *SetTabOrder )(
                __RPC__in ITaskbarList3 * This,
                /* [in] */ __RPC__in HWND hwndTab,
                /* [in] */ __RPC__in HWND hwndInsertBefore);

            HRESULT ( STDMETHODCALLTYPE *SetTabActive )(
                __RPC__in ITaskbarList3 * This,
                /* [in] */ __RPC__in HWND hwndTab,
                /* [in] */ __RPC__in HWND hwndMDI,
                /* [in] */ DWORD dwReserved);

            HRESULT ( STDMETHODCALLTYPE *ThumbBarAddButtons )(
                __RPC__in ITaskbarList3 * This,
                /* [in] */ __RPC__in HWND hwnd,
                /* [in] */ UINT cButtons,
                /* [size_is][in] */ __RPC__in_ecount_full(cButtons) LPTHUMBBUTTON pButton);

            HRESULT ( STDMETHODCALLTYPE *ThumbBarUpdateButtons )(
                __RPC__in ITaskbarList3 * This,
                /* [in] */ __RPC__in HWND hwnd,
                /* [in] */ UINT cButtons,
                /* [size_is][in] */ __RPC__in_ecount_full(cButtons) LPTHUMBBUTTON pButton);

            HRESULT ( STDMETHODCALLTYPE *ThumbBarSetImageList )(
                __RPC__in ITaskbarList3 * This,
                /* [in] */ __RPC__in HWND hwnd,
                /* [in] */ __RPC__in_opt HIMAGELIST himl);

            HRESULT ( STDMETHODCALLTYPE *SetOverlayIcon )(
                __RPC__in ITaskbarList3 * This,
                /* [in] */ __RPC__in HWND hwnd,
                /* [in] */ __RPC__in HICON hIcon,
                /* [string][unique][in] */ __RPC__in_opt_string LPCWSTR pszDescription);

            HRESULT ( STDMETHODCALLTYPE *SetThumbnailTooltip )(
                __RPC__in ITaskbarList3 * This,
                /* [in] */ __RPC__in HWND hwnd,
                /* [string][unique][in] */ __RPC__in_opt_string LPCWSTR pszTip);

            HRESULT ( STDMETHODCALLTYPE *SetThumbnailClip )(
                __RPC__in ITaskbarList3 * This,
                /* [in] */ __RPC__in HWND hwnd,
                /* [in] */ __RPC__in RECT *prcClip);

        END_INTERFACE
    } ITaskbarList3Vtbl;

    interface ITaskbarList3
    {
        CONST_VTBL struct ITaskbarList3Vtbl *lpVtbl;
    };



#ifdef COBJMACROS


#define ITaskbarList3_QueryInterface(This,riid,ppvObject)   \
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) )

#define ITaskbarList3_AddRef(This)  \
    ( (This)->lpVtbl -> AddRef(This) )

#define ITaskbarList3_Release(This) \
    ( (This)->lpVtbl -> Release(This) )


#define ITaskbarList3_HrInit(This)  \
    ( (This)->lpVtbl -> HrInit(This) )

#define ITaskbarList3_AddTab(This,hwnd) \
    ( (This)->lpVtbl -> AddTab(This,hwnd) )

#define ITaskbarList3_DeleteTab(This,hwnd)  \
    ( (This)->lpVtbl -> DeleteTab(This,hwnd) )

#define ITaskbarList3_ActivateTab(This,hwnd)    \
    ( (This)->lpVtbl -> ActivateTab(This,hwnd) )

#define ITaskbarList3_SetActiveAlt(This,hwnd)   \
    ( (This)->lpVtbl -> SetActiveAlt(This,hwnd) )


#define ITaskbarList3_MarkFullscreenWindow(This,hwnd,fFullscreen)   \
    ( (This)->lpVtbl -> MarkFullscreenWindow(This,hwnd,fFullscreen) )


#define ITaskbarList3_SetProgressValue(This,hwnd,ullCompleted,ullTotal) \
    ( (This)->lpVtbl -> SetProgressValue(This,hwnd,ullCompleted,ullTotal) )

#define ITaskbarList3_SetProgressState(This,hwnd,tbpFlags)  \
    ( (This)->lpVtbl -> SetProgressState(This,hwnd,tbpFlags) )

#define ITaskbarList3_RegisterTab(This,hwndTab,hwndMDI) \
    ( (This)->lpVtbl -> RegisterTab(This,hwndTab,hwndMDI) )

#define ITaskbarList3_UnregisterTab(This,hwndTab)   \
    ( (This)->lpVtbl -> UnregisterTab(This,hwndTab) )

#define ITaskbarList3_SetTabOrder(This,hwndTab,hwndInsertBefore)    \
    ( (This)->lpVtbl -> SetTabOrder(This,hwndTab,hwndInsertBefore) )

#define ITaskbarList3_SetTabActive(This,hwndTab,hwndMDI,dwReserved) \
    ( (This)->lpVtbl -> SetTabActive(This,hwndTab,hwndMDI,dwReserved) )

#define ITaskbarList3_ThumbBarAddButtons(This,hwnd,cButtons,pButton)    \
    ( (This)->lpVtbl -> ThumbBarAddButtons(This,hwnd,cButtons,pButton) )

#define ITaskbarList3_ThumbBarUpdateButtons(This,hwnd,cButtons,pButton) \
    ( (This)->lpVtbl -> ThumbBarUpdateButtons(This,hwnd,cButtons,pButton) )

#define ITaskbarList3_ThumbBarSetImageList(This,hwnd,himl)  \
    ( (This)->lpVtbl -> ThumbBarSetImageList(This,hwnd,himl) )

#define ITaskbarList3_SetOverlayIcon(This,hwnd,hIcon,pszDescription)    \
    ( (This)->lpVtbl -> SetOverlayIcon(This,hwnd,hIcon,pszDescription) )

#define ITaskbarList3_SetThumbnailTooltip(This,hwnd,pszTip) \
    ( (This)->lpVtbl -> SetThumbnailTooltip(This,hwnd,pszTip) )

#define ITaskbarList3_SetThumbnailClip(This,hwnd,prcClip)   \
    ( (This)->lpVtbl -> SetThumbnailClip(This,hwnd,prcClip) )

#endif /* COBJMACROS */


#endif  /* C style interface */




#endif  /* __ITaskbarList3_INTERFACE_DEFINED__ */


#ifndef __ITaskbarList4_INTERFACE_DEFINED__
#define __ITaskbarList4_INTERFACE_DEFINED__

    /* interface ITaskbarList4 */
    /* [object][uuid] */

    typedef /* [v1_enum] */
        enum STPFLAG
    {   STPF_NONE   = 0,
    STPF_USEAPPTHUMBNAILALWAYS  = 0x1,
    STPF_USEAPPTHUMBNAILWHENACTIVE  = 0x2,
    STPF_USEAPPPEEKALWAYS   = 0x4,
    STPF_USEAPPPEEKWHENACTIVE   = 0x8
    }   STPFLAG;

    DEFINE_ENUM_FLAG_OPERATORS(STPFLAG)

        EXTERN_C const IID IID_ITaskbarList4;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("c43dc798-95d1-4bea-9030-bb99e2983a1a")
ITaskbarList4 : public ITaskbarList3
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetTabProperties(
            /* [in] */ __RPC__in HWND hwndTab,
            /* [in] */ STPFLAG stpFlags) = 0;

    };

#else   /* C style interface */

    typedef struct ITaskbarList4Vtbl
    {
        BEGIN_INTERFACE

            HRESULT ( STDMETHODCALLTYPE *QueryInterface )(
            __RPC__in ITaskbarList4 * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [annotation][iid_is][out] */
            __RPC__deref_out  void **ppvObject);

            ULONG ( STDMETHODCALLTYPE *AddRef )(
                __RPC__in ITaskbarList4 * This);

            ULONG ( STDMETHODCALLTYPE *Release )(
                __RPC__in ITaskbarList4 * This);

            HRESULT ( STDMETHODCALLTYPE *HrInit )(
                __RPC__in ITaskbarList4 * This);

            HRESULT ( STDMETHODCALLTYPE *AddTab )(
                __RPC__in ITaskbarList4 * This,
                /* [in] */ __RPC__in HWND hwnd);

            HRESULT ( STDMETHODCALLTYPE *DeleteTab )(
                __RPC__in ITaskbarList4 * This,
                /* [in] */ __RPC__in HWND hwnd);

            HRESULT ( STDMETHODCALLTYPE *ActivateTab )(
                __RPC__in ITaskbarList4 * This,
                /* [in] */ __RPC__in HWND hwnd);

            HRESULT ( STDMETHODCALLTYPE *SetActiveAlt )(
                __RPC__in ITaskbarList4 * This,
                /* [in] */ __RPC__in HWND hwnd);

            HRESULT ( STDMETHODCALLTYPE *MarkFullscreenWindow )(
                __RPC__in ITaskbarList4 * This,
                /* [in] */ __RPC__in HWND hwnd,
                /* [in] */ BOOL fFullscreen);

            HRESULT ( STDMETHODCALLTYPE *SetProgressValue )(
                __RPC__in ITaskbarList4 * This,
                /* [in] */ __RPC__in HWND hwnd,
                /* [in] */ ULONGLONG ullCompleted,
                /* [in] */ ULONGLONG ullTotal);

            HRESULT ( STDMETHODCALLTYPE *SetProgressState )(
                __RPC__in ITaskbarList4 * This,
                /* [in] */ __RPC__in HWND hwnd,
                /* [in] */ TBPFLAG tbpFlags);

            HRESULT ( STDMETHODCALLTYPE *RegisterTab )(
                __RPC__in ITaskbarList4 * This,
                /* [in] */ __RPC__in HWND hwndTab,
                /* [in] */ __RPC__in HWND hwndMDI);

            HRESULT ( STDMETHODCALLTYPE *UnregisterTab )(
                __RPC__in ITaskbarList4 * This,
                /* [in] */ __RPC__in HWND hwndTab);

            HRESULT ( STDMETHODCALLTYPE *SetTabOrder )(
                __RPC__in ITaskbarList4 * This,
                /* [in] */ __RPC__in HWND hwndTab,
                /* [in] */ __RPC__in HWND hwndInsertBefore);

            HRESULT ( STDMETHODCALLTYPE *SetTabActive )(
                __RPC__in ITaskbarList4 * This,
                /* [in] */ __RPC__in HWND hwndTab,
                /* [in] */ __RPC__in HWND hwndMDI,
                /* [in] */ DWORD dwReserved);

            HRESULT ( STDMETHODCALLTYPE *ThumbBarAddButtons )(
                __RPC__in ITaskbarList4 * This,
                /* [in] */ __RPC__in HWND hwnd,
                /* [in] */ UINT cButtons,
                /* [size_is][in] */ __RPC__in_ecount_full(cButtons) LPTHUMBBUTTON pButton);

            HRESULT ( STDMETHODCALLTYPE *ThumbBarUpdateButtons )(
                __RPC__in ITaskbarList4 * This,
                /* [in] */ __RPC__in HWND hwnd,
                /* [in] */ UINT cButtons,
                /* [size_is][in] */ __RPC__in_ecount_full(cButtons) LPTHUMBBUTTON pButton);

            HRESULT ( STDMETHODCALLTYPE *ThumbBarSetImageList )(
                __RPC__in ITaskbarList4 * This,
                /* [in] */ __RPC__in HWND hwnd,
                /* [in] */ __RPC__in_opt HIMAGELIST himl);

            HRESULT ( STDMETHODCALLTYPE *SetOverlayIcon )(
                __RPC__in ITaskbarList4 * This,
                /* [in] */ __RPC__in HWND hwnd,
                /* [in] */ __RPC__in HICON hIcon,
                /* [string][unique][in] */ __RPC__in_opt_string LPCWSTR pszDescription);

            HRESULT ( STDMETHODCALLTYPE *SetThumbnailTooltip )(
                __RPC__in ITaskbarList4 * This,
                /* [in] */ __RPC__in HWND hwnd,
                /* [string][unique][in] */ __RPC__in_opt_string LPCWSTR pszTip);

            HRESULT ( STDMETHODCALLTYPE *SetThumbnailClip )(
                __RPC__in ITaskbarList4 * This,
                /* [in] */ __RPC__in HWND hwnd,
                /* [in] */ __RPC__in RECT *prcClip);

            HRESULT ( STDMETHODCALLTYPE *SetTabProperties )(
                __RPC__in ITaskbarList4 * This,
                /* [in] */ __RPC__in HWND hwndTab,
                /* [in] */ STPFLAG stpFlags);

        END_INTERFACE
    } ITaskbarList4Vtbl;

    interface ITaskbarList4
    {
        CONST_VTBL struct ITaskbarList4Vtbl *lpVtbl;
    };



#ifdef COBJMACROS


#define ITaskbarList4_QueryInterface(This,riid,ppvObject)   \
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) )

#define ITaskbarList4_AddRef(This)  \
    ( (This)->lpVtbl -> AddRef(This) )

#define ITaskbarList4_Release(This) \
    ( (This)->lpVtbl -> Release(This) )


#define ITaskbarList4_HrInit(This)  \
    ( (This)->lpVtbl -> HrInit(This) )

#define ITaskbarList4_AddTab(This,hwnd) \
    ( (This)->lpVtbl -> AddTab(This,hwnd) )

#define ITaskbarList4_DeleteTab(This,hwnd)  \
    ( (This)->lpVtbl -> DeleteTab(This,hwnd) )

#define ITaskbarList4_ActivateTab(This,hwnd)    \
    ( (This)->lpVtbl -> ActivateTab(This,hwnd) )

#define ITaskbarList4_SetActiveAlt(This,hwnd)   \
    ( (This)->lpVtbl -> SetActiveAlt(This,hwnd) )


#define ITaskbarList4_MarkFullscreenWindow(This,hwnd,fFullscreen)   \
    ( (This)->lpVtbl -> MarkFullscreenWindow(This,hwnd,fFullscreen) )


#define ITaskbarList4_SetProgressValue(This,hwnd,ullCompleted,ullTotal) \
    ( (This)->lpVtbl -> SetProgressValue(This,hwnd,ullCompleted,ullTotal) )

#define ITaskbarList4_SetProgressState(This,hwnd,tbpFlags)  \
    ( (This)->lpVtbl -> SetProgressState(This,hwnd,tbpFlags) )

#define ITaskbarList4_RegisterTab(This,hwndTab,hwndMDI) \
    ( (This)->lpVtbl -> RegisterTab(This,hwndTab,hwndMDI) )

#define ITaskbarList4_UnregisterTab(This,hwndTab)   \
    ( (This)->lpVtbl -> UnregisterTab(This,hwndTab) )

#define ITaskbarList4_SetTabOrder(This,hwndTab,hwndInsertBefore)    \
    ( (This)->lpVtbl -> SetTabOrder(This,hwndTab,hwndInsertBefore) )

#define ITaskbarList4_SetTabActive(This,hwndTab,hwndMDI,dwReserved) \
    ( (This)->lpVtbl -> SetTabActive(This,hwndTab,hwndMDI,dwReserved) )

#define ITaskbarList4_ThumbBarAddButtons(This,hwnd,cButtons,pButton)    \
    ( (This)->lpVtbl -> ThumbBarAddButtons(This,hwnd,cButtons,pButton) )

#define ITaskbarList4_ThumbBarUpdateButtons(This,hwnd,cButtons,pButton) \
    ( (This)->lpVtbl -> ThumbBarUpdateButtons(This,hwnd,cButtons,pButton) )

#define ITaskbarList4_ThumbBarSetImageList(This,hwnd,himl)  \
    ( (This)->lpVtbl -> ThumbBarSetImageList(This,hwnd,himl) )

#define ITaskbarList4_SetOverlayIcon(This,hwnd,hIcon,pszDescription)    \
    ( (This)->lpVtbl -> SetOverlayIcon(This,hwnd,hIcon,pszDescription) )

#define ITaskbarList4_SetThumbnailTooltip(This,hwnd,pszTip) \
    ( (This)->lpVtbl -> SetThumbnailTooltip(This,hwnd,pszTip) )

#define ITaskbarList4_SetThumbnailClip(This,hwnd,prcClip)   \
    ( (This)->lpVtbl -> SetThumbnailClip(This,hwnd,prcClip) )


#define ITaskbarList4_SetTabProperties(This,hwndTab,stpFlags)   \
    ( (This)->lpVtbl -> SetTabProperties(This,hwndTab,stpFlags) )

#endif /* COBJMACROS */


#endif  /* C style interface */




#endif  /* __ITaskbarList4_INTERFACE_DEFINED__ */


#ifndef __IObjectArray_INTERFACE_DEFINED__
#define __IObjectArray_INTERFACE_DEFINED__

/* interface IObjectArray */
/* [unique][object][uuid][helpstring] */


EXTERN_C const IID IID_IObjectArray;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("92CA9DCD-5622-4bba-A805-5E9F541BD8C9")
    IObjectArray : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetCount(
            /* [out] */ __RPC__out UINT *pcObjects) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetAt(
            /* [in] */ UINT uiIndex,
            /* [in] */ __RPC__in REFIID riid,
            /* [iid_is][out] */ __RPC__deref_out_opt void **ppv) = 0;

    };

#else   /* C style interface */

    typedef struct IObjectArrayVtbl
    {
        BEGIN_INTERFACE

        HRESULT ( STDMETHODCALLTYPE *QueryInterface )(
            __RPC__in IObjectArray * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [annotation][iid_is][out] */
            __RPC__deref_out  void **ppvObject);

        ULONG ( STDMETHODCALLTYPE *AddRef )(
            __RPC__in IObjectArray * This);

        ULONG ( STDMETHODCALLTYPE *Release )(
            __RPC__in IObjectArray * This);

        HRESULT ( STDMETHODCALLTYPE *GetCount )(
            __RPC__in IObjectArray * This,
            /* [out] */ __RPC__out UINT *pcObjects);

        HRESULT ( STDMETHODCALLTYPE *GetAt )(
            __RPC__in IObjectArray * This,
            /* [in] */ UINT uiIndex,
            /* [in] */ __RPC__in REFIID riid,
            /* [iid_is][out] */ __RPC__deref_out_opt void **ppv);

        END_INTERFACE
    } IObjectArrayVtbl;

    interface IObjectArray
    {
        CONST_VTBL struct IObjectArrayVtbl *lpVtbl;
    };



#ifdef COBJMACROS


#define IObjectArray_QueryInterface(This,riid,ppvObject)    \
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) )

#define IObjectArray_AddRef(This)   \
    ( (This)->lpVtbl -> AddRef(This) )

#define IObjectArray_Release(This)  \
    ( (This)->lpVtbl -> Release(This) )


#define IObjectArray_GetCount(This,pcObjects)   \
    ( (This)->lpVtbl -> GetCount(This,pcObjects) )

#define IObjectArray_GetAt(This,uiIndex,riid,ppv)   \
    ( (This)->lpVtbl -> GetAt(This,uiIndex,riid,ppv) )

#endif /* COBJMACROS */


#endif  /* C style interface */




#endif  /* __IObjectArray_INTERFACE_DEFINED__ */


#ifndef __IObjectCollection_INTERFACE_DEFINED__
#define __IObjectCollection_INTERFACE_DEFINED__

/* interface IObjectCollection */
/* [unique][object][uuid] */


EXTERN_C const IID IID_IObjectCollection;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("5632b1a4-e38a-400a-928a-d4cd63230295")
    IObjectCollection : public IObjectArray
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddObject(
            /* [in] */ __RPC__in_opt IUnknown *punk) = 0;

        virtual HRESULT STDMETHODCALLTYPE AddFromArray(
            /* [in] */ __RPC__in_opt IObjectArray *poaSource) = 0;

        virtual HRESULT STDMETHODCALLTYPE RemoveObjectAt(
            /* [in] */ UINT uiIndex) = 0;

        virtual HRESULT STDMETHODCALLTYPE Clear( void) = 0;

    };

#else   /* C style interface */

    typedef struct IObjectCollectionVtbl
    {
        BEGIN_INTERFACE

        HRESULT ( STDMETHODCALLTYPE *QueryInterface )(
            __RPC__in IObjectCollection * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [annotation][iid_is][out] */
            __RPC__deref_out  void **ppvObject);

        ULONG ( STDMETHODCALLTYPE *AddRef )(
            __RPC__in IObjectCollection * This);

        ULONG ( STDMETHODCALLTYPE *Release )(
            __RPC__in IObjectCollection * This);

        HRESULT ( STDMETHODCALLTYPE *GetCount )(
            __RPC__in IObjectCollection * This,
            /* [out] */ __RPC__out UINT *pcObjects);

        HRESULT ( STDMETHODCALLTYPE *GetAt )(
            __RPC__in IObjectCollection * This,
            /* [in] */ UINT uiIndex,
            /* [in] */ __RPC__in REFIID riid,
            /* [iid_is][out] */ __RPC__deref_out_opt void **ppv);

        HRESULT ( STDMETHODCALLTYPE *AddObject )(
            __RPC__in IObjectCollection * This,
            /* [in] */ __RPC__in_opt IUnknown *punk);

        HRESULT ( STDMETHODCALLTYPE *AddFromArray )(
            __RPC__in IObjectCollection * This,
            /* [in] */ __RPC__in_opt IObjectArray *poaSource);

        HRESULT ( STDMETHODCALLTYPE *RemoveObjectAt )(
            __RPC__in IObjectCollection * This,
            /* [in] */ UINT uiIndex);

        HRESULT ( STDMETHODCALLTYPE *Clear )(
            __RPC__in IObjectCollection * This);

        END_INTERFACE
    } IObjectCollectionVtbl;

    interface IObjectCollection
    {
        CONST_VTBL struct IObjectCollectionVtbl *lpVtbl;
    };



#ifdef COBJMACROS


#define IObjectCollection_QueryInterface(This,riid,ppvObject)   \
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) )

#define IObjectCollection_AddRef(This)  \
    ( (This)->lpVtbl -> AddRef(This) )

#define IObjectCollection_Release(This) \
    ( (This)->lpVtbl -> Release(This) )


#define IObjectCollection_GetCount(This,pcObjects)  \
    ( (This)->lpVtbl -> GetCount(This,pcObjects) )

#define IObjectCollection_GetAt(This,uiIndex,riid,ppv)  \
    ( (This)->lpVtbl -> GetAt(This,uiIndex,riid,ppv) )


#define IObjectCollection_AddObject(This,punk)  \
    ( (This)->lpVtbl -> AddObject(This,punk) )

#define IObjectCollection_AddFromArray(This,poaSource)  \
    ( (This)->lpVtbl -> AddFromArray(This,poaSource) )

#define IObjectCollection_RemoveObjectAt(This,uiIndex)  \
    ( (This)->lpVtbl -> RemoveObjectAt(This,uiIndex) )

#define IObjectCollection_Clear(This)   \
    ( (This)->lpVtbl -> Clear(This) )

#endif /* COBJMACROS */


#endif  /* C style interface */




#endif  /* __IObjectCollection_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif






extern RPC_IF_HANDLE __MIDL_itf_shobjidl_0000_0188_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_shobjidl_0000_0188_v0_0_s_ifspec;

#ifndef __ICustomDestinationList_INTERFACE_DEFINED__
#define __ICustomDestinationList_INTERFACE_DEFINED__

/* interface ICustomDestinationList */
/* [unique][object][uuid] */

typedef /* [v1_enum] */
enum KNOWNDESTCATEGORY
    {   KDC_FREQUENT    = 1,
    KDC_RECENT  = ( KDC_FREQUENT + 1 )
    }   KNOWNDESTCATEGORY;


EXTERN_C const IID IID_ICustomDestinationList;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("6332debf-87b5-4670-90c0-5e57b408a49e")
    ICustomDestinationList : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetAppID(
            /* [string][in] */ __RPC__in_string LPCWSTR pszAppID) = 0;

        virtual HRESULT STDMETHODCALLTYPE BeginList(
            /* [out] */ __RPC__out UINT *pcMinSlots,
            /* [in] */ __RPC__in REFIID riid,
            /* [iid_is][out] */ __RPC__deref_out_opt void **ppv) = 0;

        virtual HRESULT STDMETHODCALLTYPE AppendCategory(
            /* [string][in] */ __RPC__in_string LPCWSTR pszCategory,
            /* [in] */ __RPC__in_opt IObjectArray *poa) = 0;

        virtual HRESULT STDMETHODCALLTYPE AppendKnownCategory(
            /* [in] */ KNOWNDESTCATEGORY category) = 0;

        virtual HRESULT STDMETHODCALLTYPE AddUserTasks(
            /* [in] */ __RPC__in_opt IObjectArray *poa) = 0;

        virtual HRESULT STDMETHODCALLTYPE CommitList( void) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetRemovedDestinations(
            /* [in] */ __RPC__in REFIID riid,
            /* [iid_is][out] */ __RPC__deref_out_opt void **ppv) = 0;

        virtual HRESULT STDMETHODCALLTYPE DeleteList(
            /* [string][unique][in] */ __RPC__in_opt_string LPCWSTR pszAppID) = 0;

        virtual HRESULT STDMETHODCALLTYPE AbortList( void) = 0;

    };

#else   /* C style interface */

    typedef struct ICustomDestinationListVtbl
    {
        BEGIN_INTERFACE

        HRESULT ( STDMETHODCALLTYPE *QueryInterface )(
            __RPC__in ICustomDestinationList * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [annotation][iid_is][out] */
            __RPC__deref_out  void **ppvObject);

        ULONG ( STDMETHODCALLTYPE *AddRef )(
            __RPC__in ICustomDestinationList * This);

        ULONG ( STDMETHODCALLTYPE *Release )(
            __RPC__in ICustomDestinationList * This);

        HRESULT ( STDMETHODCALLTYPE *SetAppID )(
            __RPC__in ICustomDestinationList * This,
            /* [string][in] */ __RPC__in_string LPCWSTR pszAppID);

        HRESULT ( STDMETHODCALLTYPE *BeginList )(
            __RPC__in ICustomDestinationList * This,
            /* [out] */ __RPC__out UINT *pcMinSlots,
            /* [in] */ __RPC__in REFIID riid,
            /* [iid_is][out] */ __RPC__deref_out_opt void **ppv);

        HRESULT ( STDMETHODCALLTYPE *AppendCategory )(
            __RPC__in ICustomDestinationList * This,
            /* [string][in] */ __RPC__in_string LPCWSTR pszCategory,
            /* [in] */ __RPC__in_opt IObjectArray *poa);

        HRESULT ( STDMETHODCALLTYPE *AppendKnownCategory )(
            __RPC__in ICustomDestinationList * This,
            /* [in] */ KNOWNDESTCATEGORY category);

        HRESULT ( STDMETHODCALLTYPE *AddUserTasks )(
            __RPC__in ICustomDestinationList * This,
            /* [in] */ __RPC__in_opt IObjectArray *poa);

        HRESULT ( STDMETHODCALLTYPE *CommitList )(
            __RPC__in ICustomDestinationList * This);

        HRESULT ( STDMETHODCALLTYPE *GetRemovedDestinations )(
            __RPC__in ICustomDestinationList * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [iid_is][out] */ __RPC__deref_out_opt void **ppv);

        HRESULT ( STDMETHODCALLTYPE *DeleteList )(
            __RPC__in ICustomDestinationList * This,
            /* [string][unique][in] */ __RPC__in_opt_string LPCWSTR pszAppID);

        HRESULT ( STDMETHODCALLTYPE *AbortList )(
            __RPC__in ICustomDestinationList * This);

        END_INTERFACE
    } ICustomDestinationListVtbl;

    interface ICustomDestinationList
    {
        CONST_VTBL struct ICustomDestinationListVtbl *lpVtbl;
    };



#ifdef COBJMACROS


#define ICustomDestinationList_QueryInterface(This,riid,ppvObject)  \
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) )

#define ICustomDestinationList_AddRef(This) \
    ( (This)->lpVtbl -> AddRef(This) )

#define ICustomDestinationList_Release(This)    \
    ( (This)->lpVtbl -> Release(This) )


#define ICustomDestinationList_SetAppID(This,pszAppID)  \
    ( (This)->lpVtbl -> SetAppID(This,pszAppID) )

#define ICustomDestinationList_BeginList(This,pcMinSlots,riid,ppv)  \
    ( (This)->lpVtbl -> BeginList(This,pcMinSlots,riid,ppv) )

#define ICustomDestinationList_AppendCategory(This,pszCategory,poa) \
    ( (This)->lpVtbl -> AppendCategory(This,pszCategory,poa) )

#define ICustomDestinationList_AppendKnownCategory(This,category)   \
    ( (This)->lpVtbl -> AppendKnownCategory(This,category) )

#define ICustomDestinationList_AddUserTasks(This,poa)   \
    ( (This)->lpVtbl -> AddUserTasks(This,poa) )

#define ICustomDestinationList_CommitList(This) \
    ( (This)->lpVtbl -> CommitList(This) )

#define ICustomDestinationList_GetRemovedDestinations(This,riid,ppv)    \
    ( (This)->lpVtbl -> GetRemovedDestinations(This,riid,ppv) )

#define ICustomDestinationList_DeleteList(This,pszAppID)    \
    ( (This)->lpVtbl -> DeleteList(This,pszAppID) )

#define ICustomDestinationList_AbortList(This)  \
    ( (This)->lpVtbl -> AbortList(This) )

#endif /* COBJMACROS */


#endif  /* C style interface */




#endif  /* __ICustomDestinationList_INTERFACE_DEFINED__ */


#ifndef __IApplicationDestinations_INTERFACE_DEFINED__
#define __IApplicationDestinations_INTERFACE_DEFINED__

/* interface IApplicationDestinations */
/* [unique][object][uuid] */


EXTERN_C const IID IID_IApplicationDestinations;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("12337d35-94c6-48a0-bce7-6a9c69d4d600")
    IApplicationDestinations : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetAppID(
            /* [in] */ __RPC__in LPCWSTR pszAppID) = 0;

        virtual HRESULT STDMETHODCALLTYPE RemoveDestination(
            /* [in] */ __RPC__in_opt IUnknown *punk) = 0;

        virtual HRESULT STDMETHODCALLTYPE RemoveAllDestinations( void) = 0;

    };

#else   /* C style interface */

    typedef struct IApplicationDestinationsVtbl
    {
        BEGIN_INTERFACE

        HRESULT ( STDMETHODCALLTYPE *QueryInterface )(
            __RPC__in IApplicationDestinations * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [annotation][iid_is][out] */
            __RPC__deref_out  void **ppvObject);

        ULONG ( STDMETHODCALLTYPE *AddRef )(
            __RPC__in IApplicationDestinations * This);

        ULONG ( STDMETHODCALLTYPE *Release )(
            __RPC__in IApplicationDestinations * This);

        HRESULT ( STDMETHODCALLTYPE *SetAppID )(
            __RPC__in IApplicationDestinations * This,
            /* [in] */ __RPC__in LPCWSTR pszAppID);

        HRESULT ( STDMETHODCALLTYPE *RemoveDestination )(
            __RPC__in IApplicationDestinations * This,
            /* [in] */ __RPC__in_opt IUnknown *punk);

        HRESULT ( STDMETHODCALLTYPE *RemoveAllDestinations )(
            __RPC__in IApplicationDestinations * This);

        END_INTERFACE
    } IApplicationDestinationsVtbl;

    interface IApplicationDestinations
    {
        CONST_VTBL struct IApplicationDestinationsVtbl *lpVtbl;
    };



#ifdef COBJMACROS


#define IApplicationDestinations_QueryInterface(This,riid,ppvObject)    \
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) )

#define IApplicationDestinations_AddRef(This)   \
    ( (This)->lpVtbl -> AddRef(This) )

#define IApplicationDestinations_Release(This)  \
    ( (This)->lpVtbl -> Release(This) )


#define IApplicationDestinations_SetAppID(This,pszAppID)    \
    ( (This)->lpVtbl -> SetAppID(This,pszAppID) )

#define IApplicationDestinations_RemoveDestination(This,punk)   \
    ( (This)->lpVtbl -> RemoveDestination(This,punk) )

#define IApplicationDestinations_RemoveAllDestinations(This)    \
    ( (This)->lpVtbl -> RemoveAllDestinations(This) )

#endif /* COBJMACROS */


#endif  /* C style interface */

#endif  /* __IApplicationDestinations_INTERFACE_DEFINED__ */

EXTERN_C const CLSID CLSID_DestinationList;

#ifdef __cplusplus

class DECLSPEC_UUID("77f10cf0-3db5-4966-b520-b7c54fd35ed6")
DestinationList;
#endif

EXTERN_C const CLSID CLSID_ApplicationDestinations;

#ifdef __cplusplus

class DECLSPEC_UUID("86c14003-4d6b-4ef3-a7b4-0506663b2e68")
ApplicationDestinations;
#endif

EXTERN_C const CLSID CLSID_EnumerableObjectCollection;

EXTERN_C const CLSID CLSID_ShellLibrary;

#ifdef __cplusplus

class DECLSPEC_UUID("2d3468c1-36a7-43b6-ac24-d3f02fd9607a")
EnumerableObjectCollection;
#endif

DEFINE_PROPERTYKEY(PKEY_Title, 0xF29F85E0, 0x4FF9, 0x1068, 0xAB, 0x91, 0x08, 0x00, 0x2B, 0x27, 0xB3, 0xD9, 2);
DEFINE_PROPERTYKEY(PKEY_AppUserModel_IsDestListSeparator, 0x9F4C2855, 0x9F79, 0x4B39, 0xA8, 0xD0, 0xE1, 0xD4, 0x2D, 0xE1, 0xD5, 0xF3, 6);


typedef /* [v1_enum] */ 
    enum LIBRARYMANAGEDIALOGOPTIONS
{	LMD_DEFAULT	= 0,
LMD_ALLOWUNINDEXABLENETWORKLOCATIONS	= 0x1
} 	LIBRARYMANAGEDIALOGOPTIONS;

DEFINE_ENUM_FLAG_OPERATORS(LIBRARYMANAGEDIALOGOPTIONS)
    SHSTDAPI SHShowManageLibraryUI(__in IShellItem *psiLibrary, __in HWND hwndOwner, __in_opt LPCWSTR pszTitle, __in_opt LPCWSTR pszInstruction, __in LIBRARYMANAGEDIALOGOPTIONS lmdOptions);
SHSTDAPI SHResolveLibrary(__in IShellItem *psiLibrary);

#ifndef __IShellLibrary_INTERFACE_DEFINED__
#define __IShellLibrary_INTERFACE_DEFINED__

/* interface IShellLibrary */
/* [unique][object][uuid][helpstring] */ 

typedef /* [v1_enum] */ 
enum LIBRARYFOLDERFILTER
    {	LFF_FORCEFILESYSTEM	= 1,
	LFF_STORAGEITEMS	= 2,
	LFF_ALLITEMS	= 3
    } 	LIBRARYFOLDERFILTER;

typedef /* [v1_enum] */ 
enum LIBRARYOPTIONFLAGS
    {	LOF_DEFAULT	= 0,
	LOF_PINNEDTONAVPANE	= 0x1,
	LOF_MASK_ALL	= 0x1
    } 	LIBRARYOPTIONFLAGS;

DEFINE_ENUM_FLAG_OPERATORS(LIBRARYOPTIONFLAGS)
typedef /* [v1_enum] */ 
enum DEFAULTSAVEFOLDERTYPE
    {	DSFT_DETECT	= 1,
	DSFT_PRIVATE	= ( DSFT_DETECT + 1 ) ,
	DSFT_PUBLIC	= ( DSFT_PRIVATE + 1 ) 
    } 	DEFAULTSAVEFOLDERTYPE;

typedef /* [v1_enum] */ 
enum LIBRARYSAVEFLAGS
    {	LSF_FAILIFTHERE	= 0,
	LSF_OVERRIDEEXISTING	= 0x1,
	LSF_MAKEUNIQUENAME	= 0x2
    } 	LIBRARYSAVEFLAGS;

DEFINE_ENUM_FLAG_OPERATORS(LIBRARYSAVEFLAGS)

EXTERN_C const IID IID_IShellLibrary;


#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("11a66efa-382e-451a-9234-1e0e12ef3085")
    IShellLibrary : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE LoadLibraryFromItem( 
            /* [in] */ __RPC__in_opt IShellItem *psiLibrary,
            /* [in] */ DWORD grfMode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LoadLibraryFromKnownFolder( 
            /* [in] */ __RPC__in REFKNOWNFOLDERID kfidLibrary,
            /* [in] */ DWORD grfMode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddFolder( 
            /* [in] */ __RPC__in_opt IShellItem *psiLocation) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveFolder( 
            /* [in] */ __RPC__in_opt IShellItem *psiLocation) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFolders( 
            /* [in] */ LIBRARYFOLDERFILTER lff,
            /* [in] */ __RPC__in REFIID riid,
            /* [iid_is][out] */ __RPC__deref_out_opt void **ppv) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ResolveFolder( 
            /* [in] */ __RPC__in_opt IShellItem *psiFolderToResolve,
            /* [in] */ DWORD dwTimeout,
            /* [in] */ __RPC__in REFIID riid,
            /* [iid_is][out] */ __RPC__deref_out_opt void **ppv) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDefaultSaveFolder( 
            /* [in] */ DEFAULTSAVEFOLDERTYPE dsft,
            /* [in] */ __RPC__in REFIID riid,
            /* [iid_is][out] */ __RPC__deref_out_opt void **ppv) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetDefaultSaveFolder( 
            /* [in] */ DEFAULTSAVEFOLDERTYPE dsft,
            /* [in] */ __RPC__in_opt IShellItem *psi) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOptions( 
            /* [out] */ __RPC__out LIBRARYOPTIONFLAGS *plofOptions) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetOptions( 
            /* [in] */ LIBRARYOPTIONFLAGS lofMask,
            /* [in] */ LIBRARYOPTIONFLAGS lofOptions) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFolderType( 
            /* [out] */ __RPC__out FOLDERTYPEID *pftid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFolderType( 
            /* [in] */ __RPC__in REFFOLDERTYPEID ftid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetIcon( 
            /* [string][out] */ __RPC__deref_out_opt_string LPWSTR *ppszIcon) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetIcon( 
            /* [string][in] */ __RPC__in_string LPCWSTR pszIcon) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Commit( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Save( 
            /* [in] */ __RPC__in_opt IShellItem *psiFolderToSaveIn,
            /* [string][in] */ __RPC__in_string LPCWSTR pszLibraryName,
            /* [in] */ LIBRARYSAVEFLAGS lsf,
            /* [out] */ __RPC__deref_out_opt IShellItem **ppsiSavedTo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SaveInKnownFolder( 
            /* [in] */ __RPC__in REFKNOWNFOLDERID kfidToSaveIn,
            /* [string][in] */ __RPC__in_string LPCWSTR pszLibraryName,
            /* [in] */ LIBRARYSAVEFLAGS lsf,
            /* [out] */ __RPC__deref_out_opt IShellItem **ppsiSavedTo) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IShellLibraryVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            __RPC__in IShellLibrary * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            __RPC__in IShellLibrary * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            __RPC__in IShellLibrary * This);
        
        HRESULT ( STDMETHODCALLTYPE *LoadLibraryFromItem )( 
            __RPC__in IShellLibrary * This,
            /* [in] */ __RPC__in_opt IShellItem *psiLibrary,
            /* [in] */ DWORD grfMode);
        
        HRESULT ( STDMETHODCALLTYPE *LoadLibraryFromKnownFolder )( 
            __RPC__in IShellLibrary * This,
            /* [in] */ __RPC__in REFKNOWNFOLDERID kfidLibrary,
            /* [in] */ DWORD grfMode);
        
        HRESULT ( STDMETHODCALLTYPE *AddFolder )( 
            __RPC__in IShellLibrary * This,
            /* [in] */ __RPC__in_opt IShellItem *psiLocation);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveFolder )( 
            __RPC__in IShellLibrary * This,
            /* [in] */ __RPC__in_opt IShellItem *psiLocation);
        
        HRESULT ( STDMETHODCALLTYPE *GetFolders )( 
            __RPC__in IShellLibrary * This,
            /* [in] */ LIBRARYFOLDERFILTER lff,
            /* [in] */ __RPC__in REFIID riid,
            /* [iid_is][out] */ __RPC__deref_out_opt void **ppv);
        
        HRESULT ( STDMETHODCALLTYPE *ResolveFolder )( 
            __RPC__in IShellLibrary * This,
            /* [in] */ __RPC__in_opt IShellItem *psiFolderToResolve,
            /* [in] */ DWORD dwTimeout,
            /* [in] */ __RPC__in REFIID riid,
            /* [iid_is][out] */ __RPC__deref_out_opt void **ppv);
        
        HRESULT ( STDMETHODCALLTYPE *GetDefaultSaveFolder )( 
            __RPC__in IShellLibrary * This,
            /* [in] */ DEFAULTSAVEFOLDERTYPE dsft,
            /* [in] */ __RPC__in REFIID riid,
            /* [iid_is][out] */ __RPC__deref_out_opt void **ppv);
        
        HRESULT ( STDMETHODCALLTYPE *SetDefaultSaveFolder )( 
            __RPC__in IShellLibrary * This,
            /* [in] */ DEFAULTSAVEFOLDERTYPE dsft,
            /* [in] */ __RPC__in_opt IShellItem *psi);
        
        HRESULT ( STDMETHODCALLTYPE *GetOptions )( 
            __RPC__in IShellLibrary * This,
            /* [out] */ __RPC__out LIBRARYOPTIONFLAGS *plofOptions);
        
        HRESULT ( STDMETHODCALLTYPE *SetOptions )( 
            __RPC__in IShellLibrary * This,
            /* [in] */ LIBRARYOPTIONFLAGS lofMask,
            /* [in] */ LIBRARYOPTIONFLAGS lofOptions);
        
        HRESULT ( STDMETHODCALLTYPE *GetFolderType )( 
            __RPC__in IShellLibrary * This,
            /* [out] */ __RPC__out FOLDERTYPEID *pftid);
        
        HRESULT ( STDMETHODCALLTYPE *SetFolderType )( 
            __RPC__in IShellLibrary * This,
            /* [in] */ __RPC__in REFFOLDERTYPEID ftid);
        
        HRESULT ( STDMETHODCALLTYPE *GetIcon )( 
            __RPC__in IShellLibrary * This,
            /* [string][out] */ __RPC__deref_out_opt_string LPWSTR *ppszIcon);
        
        HRESULT ( STDMETHODCALLTYPE *SetIcon )( 
            __RPC__in IShellLibrary * This,
            /* [string][in] */ __RPC__in_string LPCWSTR pszIcon);
        
        HRESULT ( STDMETHODCALLTYPE *Commit )( 
            __RPC__in IShellLibrary * This);
        
        HRESULT ( STDMETHODCALLTYPE *Save )( 
            __RPC__in IShellLibrary * This,
            /* [in] */ __RPC__in_opt IShellItem *psiFolderToSaveIn,
            /* [string][in] */ __RPC__in_string LPCWSTR pszLibraryName,
            /* [in] */ LIBRARYSAVEFLAGS lsf,
            /* [out] */ __RPC__deref_out_opt IShellItem **ppsiSavedTo);
        
        HRESULT ( STDMETHODCALLTYPE *SaveInKnownFolder )( 
            __RPC__in IShellLibrary * This,
            /* [in] */ __RPC__in REFKNOWNFOLDERID kfidToSaveIn,
            /* [string][in] */ __RPC__in_string LPCWSTR pszLibraryName,
            /* [in] */ LIBRARYSAVEFLAGS lsf,
            /* [out] */ __RPC__deref_out_opt IShellItem **ppsiSavedTo);
        
        END_INTERFACE
    } IShellLibraryVtbl;

    interface IShellLibrary
    {
        CONST_VTBL struct IShellLibraryVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IShellLibrary_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IShellLibrary_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IShellLibrary_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IShellLibrary_LoadLibraryFromItem(This,psiLibrary,grfMode)	\
    ( (This)->lpVtbl -> LoadLibraryFromItem(This,psiLibrary,grfMode) ) 

#define IShellLibrary_LoadLibraryFromKnownFolder(This,kfidLibrary,grfMode)	\
    ( (This)->lpVtbl -> LoadLibraryFromKnownFolder(This,kfidLibrary,grfMode) ) 

#define IShellLibrary_AddFolder(This,psiLocation)	\
    ( (This)->lpVtbl -> AddFolder(This,psiLocation) ) 

#define IShellLibrary_RemoveFolder(This,psiLocation)	\
    ( (This)->lpVtbl -> RemoveFolder(This,psiLocation) ) 

#define IShellLibrary_GetFolders(This,lff,riid,ppv)	\
    ( (This)->lpVtbl -> GetFolders(This,lff,riid,ppv) ) 

#define IShellLibrary_ResolveFolder(This,psiFolderToResolve,dwTimeout,riid,ppv)	\
    ( (This)->lpVtbl -> ResolveFolder(This,psiFolderToResolve,dwTimeout,riid,ppv) ) 

#define IShellLibrary_GetDefaultSaveFolder(This,dsft,riid,ppv)	\
    ( (This)->lpVtbl -> GetDefaultSaveFolder(This,dsft,riid,ppv) ) 

#define IShellLibrary_SetDefaultSaveFolder(This,dsft,psi)	\
    ( (This)->lpVtbl -> SetDefaultSaveFolder(This,dsft,psi) ) 

#define IShellLibrary_GetOptions(This,plofOptions)	\
    ( (This)->lpVtbl -> GetOptions(This,plofOptions) ) 

#define IShellLibrary_SetOptions(This,lofMask,lofOptions)	\
    ( (This)->lpVtbl -> SetOptions(This,lofMask,lofOptions) ) 

#define IShellLibrary_GetFolderType(This,pftid)	\
    ( (This)->lpVtbl -> GetFolderType(This,pftid) ) 

#define IShellLibrary_SetFolderType(This,ftid)	\
    ( (This)->lpVtbl -> SetFolderType(This,ftid) ) 

#define IShellLibrary_GetIcon(This,ppszIcon)	\
    ( (This)->lpVtbl -> GetIcon(This,ppszIcon) ) 

#define IShellLibrary_SetIcon(This,pszIcon)	\
    ( (This)->lpVtbl -> SetIcon(This,pszIcon) ) 

#define IShellLibrary_Commit(This)	\
    ( (This)->lpVtbl -> Commit(This) ) 

#define IShellLibrary_Save(This,psiFolderToSaveIn,pszLibraryName,lsf,ppsiSavedTo)	\
    ( (This)->lpVtbl -> Save(This,psiFolderToSaveIn,pszLibraryName,lsf,ppsiSavedTo) ) 

#define IShellLibrary_SaveInKnownFolder(This,kfidToSaveIn,pszLibraryName,lsf,ppsiSavedTo)	\
    ( (This)->lpVtbl -> SaveInKnownFolder(This,kfidToSaveIn,pszLibraryName,lsf,ppsiSavedTo) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */

#endif /*__IShellLibrary_INTERFACE_DEFINED__*/

__inline HRESULT SHCreateLibrary(__in REFIID riid, __deref_out void **ppv)
{
    return CoCreateInstance(CLSID_ShellLibrary, NULL, CLSCTX_INPROC_SERVER, riid, ppv);
}

__inline HRESULT SHLoadLibraryFromItem(__in IShellItem *psiLibrary, __in DWORD grfMode, __in REFIID riid, __deref_out void **ppv)
{
    *ppv = NULL;
    IShellLibrary *plib;
    HRESULT hr = CoCreateInstance(CLSID_ShellLibrary, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&plib));
    if (SUCCEEDED(hr))
    {
        hr = plib->LoadLibraryFromItem(psiLibrary, grfMode);
        if (SUCCEEDED(hr))
        {
            hr = plib->QueryInterface(riid, ppv);
        }
        plib->Release();
    }
    return hr;
}

__inline HRESULT SHLoadLibraryFromKnownFolder(__in REFKNOWNFOLDERID kfidLibrary, __in DWORD grfMode, __in REFIID riid, __deref_out void **ppv)
{
    *ppv = NULL;
    IShellLibrary *plib;
    HRESULT hr = CoCreateInstance(CLSID_ShellLibrary, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&plib));
    if (SUCCEEDED(hr))
    {
        hr = plib->LoadLibraryFromKnownFolder(kfidLibrary, grfMode);
        if (SUCCEEDED(hr))
        {
            hr = plib->QueryInterface(riid, ppv);
        }
        plib->Release();
    }
    return hr;
}

__inline HRESULT SHLoadLibraryFromParsingName(__in PCWSTR pszParsingName, __in DWORD grfMode, __in REFIID riid, __deref_out void **ppv)
{
    *ppv = NULL;
    IShellItem *psiLibrary;
    HRESULT hr = SHCreateItemFromParsingName(pszParsingName, NULL, IID_PPV_ARGS(&psiLibrary));
    if (SUCCEEDED(hr))
    {
        hr = SHLoadLibraryFromItem(psiLibrary, grfMode, riid, ppv);
        psiLibrary->Release();
    }
    return hr;
}

__inline HRESULT SHAddFolderPathToLibrary(__in IShellLibrary *plib, __in PCWSTR pszFolderPath)
{
    IShellItem *psiFolder;
    HRESULT hr = SHCreateItemFromParsingName(pszFolderPath, NULL, IID_PPV_ARGS(&psiFolder));
    if (SUCCEEDED(hr))
    {
        hr = plib->AddFolder(psiFolder);
        psiFolder->Release();
    }
    return hr;
}

__inline HRESULT SHRemoveFolderPathFromLibrary(__in IShellLibrary *plib, __in PCWSTR pszFolderPath)
{
    PIDLIST_ABSOLUTE pidlFolder = SHSimpleIDListFromPath(pszFolderPath);
    HRESULT hr = pidlFolder ? S_OK : E_INVALIDARG;
    if (SUCCEEDED(hr))
    {
        IShellItem *psiFolder;
        hr = SHCreateItemFromIDList(pidlFolder, IID_PPV_ARGS(&psiFolder));
        if (SUCCEEDED(hr))
        {
            hr = plib->RemoveFolder(psiFolder);
            psiFolder->Release();
        }
        CoTaskMemFree(pidlFolder);
    }
    return hr;
}

__inline HRESULT SHResolveFolderPathInLibrary(__in IShellLibrary *plib, __in PCWSTR pszFolderPath, __in DWORD dwTimeout, __deref_out PWSTR *ppszResolvedPath)
{
    *ppszResolvedPath = NULL;
    PIDLIST_ABSOLUTE pidlFolder = SHSimpleIDListFromPath(pszFolderPath);
    HRESULT hr = pidlFolder ? S_OK : E_INVALIDARG;
    if (SUCCEEDED(hr))
    {
        IShellItem *psiFolder;
        hr = SHCreateItemFromIDList(pidlFolder, IID_PPV_ARGS(&psiFolder));
        if (SUCCEEDED(hr))
        {
            IShellItem *psiResolved;
            hr = plib->ResolveFolder(psiFolder, dwTimeout, IID_PPV_ARGS(&psiResolved));
            if (SUCCEEDED(hr))
            {
                hr = psiResolved->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, ppszResolvedPath);
                psiResolved->Release();
            }
            psiFolder->Release();
        }
        CoTaskMemFree(pidlFolder);
    }
    return hr;
}

__inline HRESULT SHSaveLibraryInFolderPath(__in IShellLibrary *plib, __in PCWSTR pszFolderPath, __in PCWSTR pszLibraryName, __in LIBRARYSAVEFLAGS lsf, __deref_opt_out PWSTR *ppszSavedToPath)
{
    if (ppszSavedToPath)
    {
        *ppszSavedToPath = NULL;
    }

    IShellItem *psiFolder;
    HRESULT hr = SHCreateItemFromParsingName(pszFolderPath, NULL, IID_PPV_ARGS(&psiFolder));
    if (SUCCEEDED(hr))
    {
        IShellItem *psiSavedTo;
        hr = plib->Save(psiFolder, pszLibraryName, lsf, &psiSavedTo);
        if (SUCCEEDED(hr))
        {
            if (ppszSavedToPath)
            {
                hr = psiSavedTo->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, ppszSavedToPath);
            }
            psiSavedTo->Release();
        }
        psiFolder->Release();
    }
    return hr;
}
#endif /* (NTDDI_VERSION < NTDDI_WIN7) */
