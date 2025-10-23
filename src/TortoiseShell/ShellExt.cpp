﻿// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2011-2023, 2025 - TortoiseGit
// Copyright (C) 2003-2014 - TortoiseSVN

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

// Initialize GUIDs (should be done only and at-least once per DLL/EXE)
#include <initguid.h>
#include "Guids.h"

#include "ShellExt.h"
#include "ShellObjects.h"
#include "../version.h"
#include "I18NHelper.h"
#include "GitAdminDir.h"
#undef swprintf

extern ShellObjects g_shellObjects;

// *********************** CShellExt *************************
CShellExt::CShellExt(FileState state)
	: m_State(state)
	,regDiffLater(L"Software\\TortoiseGit\\DiffLater", L"")
{
	InterlockedIncrement(&g_cRefThisDll);

	{
		AutoLocker lock(g_csGlobalCOMGuard);
		g_shellObjects.Insert(this);
	}

	INITCOMMONCONTROLSEX used = {
		sizeof(INITCOMMONCONTROLSEX),
			ICC_LISTVIEW_CLASSES | ICC_WIN95_CLASSES | ICC_BAR_CLASSES | ICC_USEREX_CLASSES
	};
	InitCommonControlsEx(&used);
	LoadLangDll();
}

CShellExt::~CShellExt()
{
	AutoLocker lock(g_csGlobalCOMGuard);
	InterlockedDecrement(&g_cRefThisDll);
	g_shellObjects.Erase(this);
}

void LoadLangDll()
{
	if (g_langid != g_ShellCache.GetLangID() && (g_langTimeout == 0 || g_langTimeout < GetTickCount64()))
	{
		g_langid = g_ShellCache.GetLangID();
		DWORD langId = g_langid;
		wchar_t langDll[MAX_PATH * 4] = { 0 };
		HINSTANCE hInst = nullptr;
		wchar_t langdir[MAX_PATH] = { 0 };
		if (GetModuleFileName(g_hmodThisDll, langdir, _countof(langdir))==0)
			return;
		wchar_t* dirpoint = wcsrchr(langdir, L'\\');
		if (dirpoint)
			*dirpoint = L'\0';
		dirpoint = wcsrchr(langdir, L'\\');
		if (dirpoint)
			*dirpoint = L'\0';

		do
		{
			swprintf_s(langDll, L"%s\\Languages\\TortoiseProc%lu.dll", langdir, langId);
			if (CI18NHelper::DoVersionStringsMatch(CPathUtils::GetVersionFromFile(langDll), _T(STRPRODUCTVER)))
				hInst = ::LoadLibraryEx(langDll, nullptr, LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE);
			if (hInst)
			{
				if (g_hResInst != g_hmodThisDll)
					FreeLibrary(g_hResInst);
				g_hResInst = hInst;
			}
			else
			{
				DWORD lid = SUBLANGID(langId);
				lid--;
				if (lid > 0)
					langId = MAKELANGID(PRIMARYLANGID(langId), lid);
				else
					langId = 0;
			}
		} while (!hInst && langId != 0);
		if (!hInst)
		{
			// either the dll for the selected language is not present, or
			// it is the wrong version.
			// fall back to English and set a timeout so we don't retry
			// to load the language dll too often
			if (g_hResInst != g_hmodThisDll)
				FreeLibrary(g_hResInst);
			g_hResInst = g_hmodThisDll;
			g_langid = 1033;
			// set a timeout of 10 seconds
			if (g_ShellCache.GetLangID() != 1033)
				g_langTimeout = GetTickCount64() + 10000;
		}
		else
			g_langTimeout = 0;
	} // if (g_langid != g_ShellCache.GetLangID())
}

STDMETHODIMP CShellExt::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
	if (!ppv)
		return E_POINTER;

	*ppv = nullptr;

	if (IsEqualIID(riid, IID_IShellExtInit) || IsEqualIID(riid, IID_IUnknown))
		*ppv = static_cast<LPSHELLEXTINIT>(this);
	else if (IsEqualIID(riid, IID_IContextMenu))
		*ppv = static_cast<LPCONTEXTMENU>(this);
	else if (IsEqualIID(riid, IID_IContextMenu2))
		*ppv = static_cast<LPCONTEXTMENU2>(this);
	else if (IsEqualIID(riid, IID_IContextMenu3))
		*ppv = static_cast<LPCONTEXTMENU3>(this);
	else if (IsEqualIID(riid, IID_IShellIconOverlayIdentifier))
		*ppv = static_cast<IShellIconOverlayIdentifier*>(this);
	else if (IsEqualIID(riid, IID_IShellPropSheetExt))
		*ppv = static_cast<LPSHELLPROPSHEETEXT>(this);
	else if (IsEqualIID(riid, IID_IShellCopyHook))
		*ppv = static_cast<ICopyHook*>(this);
	else if (IsEqualIID(riid, IID_IExplorerCommand))
		*ppv = static_cast<IExplorerCommand*>(this);
	else if (IsEqualIID(riid, IID_IObjectWithSite))
		*ppv = static_cast<IObjectWithSite*>(this);
	else
		return E_NOINTERFACE;

	AddRef();
	return S_OK;
}

STDMETHODIMP_(ULONG) CShellExt::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CShellExt::Release()
{
	if (InterlockedDecrement(&m_cRef))
		return m_cRef;

	delete this;

	return 0L;
}

// IPersistFile members
STDMETHODIMP CShellExt::GetClassID(CLSID *pclsid)
{
	if (!pclsid)
		return E_POINTER;
	*pclsid = CLSID_Tortoisegit_UNCONTROLLED;
	return S_OK;
}

STDMETHODIMP CShellExt::Load(LPCOLESTR /*pszFileName*/, DWORD /*dwMode*/)
{
	return S_OK;
}

// ICopyHook member
UINT __stdcall CShellExt::CopyCallback(HWND /*hWnd*/, UINT wFunc, UINT /*wFlags*/, LPCWSTR pszSrcFile, DWORD /*dwSrcAttribs*/, LPCWSTR /*pszDestFile*/, DWORD /*dwDestAttribs*/)
{
	switch (wFunc)
	{
	case FO_MOVE:
	case FO_DELETE:
	case FO_RENAME:
		if (pszSrcFile && pszSrcFile[0] && g_ShellCache.IsPathAllowed(pszSrcFile))
		{
			const auto cacheType = g_ShellCache.GetCacheType();
			switch (cacheType)
			{
			case ShellCache::exe:
				m_remoteCacheLink.ReleaseLockForPath(CTGitPath(pszSrcFile));
				break;
			case ShellCache::dll:
			case ShellCache::dllFull:
			{
				CString topDir;
				if (GitAdminDir::HasAdminDir(pszSrcFile, &topDir))
					m_CachedStatus.m_GitStatus.ReleasePath(topDir);
				break;
			}
			default:
				break;
			}
		}
		break;
	default:
		break;
	}

	// we could now wait a little bit to give the cache time to release the handles.
	// but the explorer/shell already retries any action for about two seconds
	// if it first fails. So if the cache hasn't released the handle yet, the explorer
	// will retry anyway, so we just leave here immediately.
	return IDYES;
}

// IObjectWithSite
HRESULT __stdcall CShellExt::SetSite(IUnknown* pUnkSite)
{
	m_site = pUnkSite;
	return S_OK;
}

HRESULT __stdcall CShellExt::GetSite(REFIID riid, void** ppvSite)
{
	return m_site.CopyTo(riid, ppvSite);
}
