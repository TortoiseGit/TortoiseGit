// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2010-2011 - TortoiseSVN

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
#include "stdafx.h"
#include "Libraries.h"
#include "PathUtils.h"
#include "resource.h"
#include <initguid.h>
#include <propkeydef.h>
#if (NTDDI_VERSION < 0x06010000)

#define INITGUID

#ifdef _WIN64
// {DC9E616B-7611-461c-9D37-730FDD4CE278}
DEFINE_GUID(FOLDERTYPEID_GITWC,       0xdc9e616b, 0x7611, 0x461c, 0x9d, 0x37, 0x73, 0xf, 0xdd, 0x4c, 0xe2, 0x78);
// {B118C031-A977-4a67-9344-47F057388105}
DEFINE_GUID(FOLDERTYPEID_GITWC32,     0xb118c031, 0xa977, 0x4a67, 0x93, 0x44, 0x47, 0xf0, 0x57, 0x38, 0x81, 0x5);
#else
DEFINE_GUID(FOLDERTYPEID_GITWC,       0xb118c031, 0xa977, 0x4a67, 0x93, 0x44, 0x47, 0xf0, 0x57, 0x38, 0x81, 0x5);
#endif

#endif /* (NTDDI_VERSION < NTDDI_WIN7) */

#include "StdAfx.h"
#include "Libraries.h"
#include "win7.h"
#include "SmartHandle.h"

/**
 * Makes sure a library named "Subversion" exists and has our template
 * set to it.
 * If the library already exists, the template is set.
 * If the library doesn't exist, it is created.
 */
void EnsureGitLibrary(bool bCreate /* = true*/)
{
    // when running the 32-bit version of TortoiseProc on x64 OS,
    // we must not create the library! This would break
    // the library in the x64 explorer.
    BOOL bIsWow64 = FALSE;
    IsWow64Process(GetCurrentProcess(), &bIsWow64);
    if (bIsWow64)
        return;

    CComPtr<IShellLibrary> pLibrary = NULL;
    if (FAILED(OpenShellLibrary(L"Git", &pLibrary)))
    {
        if (!bCreate)
            return;
        if (FAILED(SHCreateLibrary(IID_PPV_ARGS(&pLibrary))))
            return;

        // Save the new library under the user's Libraries folder.
        CComPtr<IShellItem> pSavedTo = NULL;
        if (FAILED(pLibrary->SaveInKnownFolder(FOLDERID_UsersLibraries, L"Git", LSF_OVERRIDEEXISTING, &pSavedTo)))
            return;
    }

    if (SUCCEEDED(pLibrary->SetFolderType(FOLDERTYPEID_GITWC)))
    {
        // create the path for the icon
        CString path;
        CString appDir = CPathUtils::GetAppDirectory();
        if (appDir.GetLength() < MAX_PATH)
        {
            TCHAR buf[MAX_PATH] = {0};
            PathCanonicalize(buf, (LPCTSTR)appDir);
            appDir = buf;
        }
        path.Format(_T("%s%s,-%d"), (LPCTSTR)appDir, _T("TortoiseProc.exe"), IDI_LIBRARY);
        pLibrary->SetIcon((LPCTSTR)path);
        pLibrary->Commit();
    }
}

/**
 * Open the shell library under the user's Libraries folder according to the
 * specified library name with both read and write permissions.
 *
 * \param pwszLibraryName
 * The name of the shell library to be opened.
 *
 * \param ppShellLib
 * If the open operation succeeds, ppShellLib outputs the IShellLibrary
 * interface of the shell library object. The caller is responsible for calling
 * Release on the shell library. If the function fails, NULL is returned from
 * *ppShellLib.
 */
HRESULT OpenShellLibrary(LPWSTR pwszLibraryName, IShellLibrary** ppShellLib)
{
    HRESULT hr;
    *ppShellLib = NULL;

    IShellItem2* pShellItem = NULL;
    hr = GetShellLibraryItem(pwszLibraryName, &pShellItem);
    if (FAILED(hr))
        return hr;

    // Get the shell library object from the shell item with a read and write permissions
    hr = SHLoadLibraryFromItem(pShellItem, STGM_READWRITE, IID_PPV_ARGS(ppShellLib));

    pShellItem->Release();

    return hr;
}

/**
 * Get the shell item that represents the library.
 *
 * \param pwszLibraryName
 * The name of the shell library
 *
 * \param ppShellItem
 * If the operation succeeds, ppShellItem outputs the IShellItem2 interface
 * that represents the library. The caller is responsible for calling
 * Release on the shell item. If the function fails, NULL is returned from
 * *ppShellItem.
 */
HRESULT GetShellLibraryItem(LPWSTR pwszLibraryName, IShellItem2** ppShellItem)
{
    HRESULT hr = E_NOINTERFACE;
    *ppShellItem = NULL;

    // Create the real library file name
    WCHAR wszRealLibraryName[MAX_PATH];
    swprintf_s(wszRealLibraryName, L"%s%s", pwszLibraryName, L".library-ms");

    typedef HRESULT STDAPICALLTYPE SHCreateItemInKnownFolderFN(REFKNOWNFOLDERID kfid, DWORD dwKFFlags, __in_opt PCWSTR pszItem, REFIID riid, __deref_out void **ppv);
    CAutoLibrary hShell = ::LoadLibrary(_T("shell32.dll"));
    if (hShell)
    {
        SHCreateItemInKnownFolderFN *pfnSHCreateItemInKnownFolder = (SHCreateItemInKnownFolderFN*)GetProcAddress(hShell, "SHCreateItemInKnownFolder");
        if (pfnSHCreateItemInKnownFolder)
        {
            hr = pfnSHCreateItemInKnownFolder(FOLDERID_UsersLibraries, KF_FLAG_DEFAULT_PATH | KF_FLAG_NO_ALIAS, wszRealLibraryName, IID_PPV_ARGS(ppShellItem));
        }
    }

    return hr;
}


