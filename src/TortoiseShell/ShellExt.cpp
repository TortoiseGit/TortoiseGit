// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2011-2016 - TortoiseGit
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
#include "..\version.h"
#undef swprintf

extern ShellObjects g_shellObjects;

// *********************** CShellExt *************************
CShellExt::CShellExt(FileState state)
	: m_State(state)
	, itemStates(0)
	, itemStatesFolder(0)
	, space(0)
#if ENABLE_CRASHHANLDER
	, m_crasher(L"TortoiseGit", TGIT_VERMAJOR, TGIT_VERMINOR, TGIT_VERMICRO, TGIT_VERBUILD, TGIT_VERDATE, false)
#endif
	,regDiffLater(L"Software\\TortoiseGit\\DiffLater", L"")
{
	m_cRef = 0L;
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
		TCHAR langDll[MAX_PATH*4] = {0};
		HINSTANCE hInst = nullptr;
		TCHAR langdir[MAX_PATH] = {0};
		char langdirA[MAX_PATH] = {0};
		if (GetModuleFileName(g_hmodThisDll, langdir, _countof(langdir))==0)
			return;
		if (GetModuleFileNameA(g_hmodThisDll, langdirA, _countof(langdirA))==0)
			return;
		TCHAR * dirpoint = _tcsrchr(langdir, '\\');
		char * dirpointA = strrchr(langdirA, '\\');
		if (dirpoint)
			*dirpoint = 0;
		if (dirpointA)
			*dirpointA = 0;
		dirpoint = _tcsrchr(langdir, '\\');
		dirpointA = strrchr(langdirA, '\\');
		if (dirpoint)
			*dirpoint = 0;
		if (dirpointA)
			*dirpointA = 0;
		strcat_s(langdirA, "\\Languages");

		BOOL bIsWow = FALSE;
		IsWow64Process(GetCurrentProcess(), &bIsWow);

		do
		{
			if (bIsWow)
				_stprintf_s(langDll, _T("%s\\Languages\\TortoiseProc32%lu.dll"), langdir, langId);
			else
				_stprintf_s(langDll, _T("%s\\Languages\\TortoiseProc%lu.dll"), langdir, langId);
			BOOL versionmatch = TRUE;

			struct TRANSARRAY
			{
				WORD wLanguageID;
				WORD wCharacterSet;
			};

			DWORD dwReserved,dwBufferSize;
			dwBufferSize = GetFileVersionInfoSize((LPTSTR)langDll,&dwReserved);

			if (dwBufferSize > 0)
			{
				LPVOID pBuffer = (void*) malloc(dwBufferSize);

				if (pBuffer)
				{
					UINT        nInfoSize = 0;
					UINT        nFixedLength = 0;
					LPSTR       lpVersion = nullptr;
					VOID*       lpFixedPointer;

					if (GetFileVersionInfo((LPTSTR)langDll,
						dwReserved,
						dwBufferSize,
						pBuffer))
					{
						// Query the current language
						if (VerQueryValue(	pBuffer,
							_T("\\VarFileInfo\\Translation"),
							&lpFixedPointer,
							&nFixedLength))
						{
							auto lpTransArray = reinterpret_cast<TRANSARRAY*>(lpFixedPointer);
							TCHAR strLangProductVersion[MAX_PATH] = { 0 };

							_stprintf_s(strLangProductVersion, _T("\\StringFileInfo\\%04x%04x\\ProductVersion"),
								lpTransArray[0].wLanguageID, lpTransArray[0].wCharacterSet);

							if (VerQueryValue(pBuffer, (LPTSTR)strLangProductVersion, (LPVOID*)&lpVersion, &nInfoSize))
								versionmatch = (_tcscmp((LPCTSTR)lpVersion, _T(STRPRODUCTVER)) == 0);

						}
					}
					free(pBuffer);
				} // if (pBuffer)
			} // if (dwBufferSize > 0)
			else
				versionmatch = FALSE;

			if (versionmatch)
				hInst = LoadLibrary(langDll);
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
UINT __stdcall CShellExt::CopyCallback(HWND hWnd, UINT wFunc, UINT wFlags, LPCTSTR pszSrcFile, DWORD dwSrcAttribs, LPCTSTR pszDestFile, DWORD dwDestAttribs)
{
	__try
	{
		return CopyCallback_Wrap(hWnd, wFunc, wFlags, pszSrcFile, dwSrcAttribs, pszDestFile, dwDestAttribs);
	}
	__except(CCrashReport::Instance().SendReport(GetExceptionInformation()))
	{
	}
	return IDYES;
}

UINT __stdcall CShellExt::CopyCallback_Wrap(HWND /*hWnd*/, UINT wFunc, UINT /*wFlags*/, LPCTSTR pszSrcFile, DWORD /*dwSrcAttribs*/, LPCTSTR /*pszDestFile*/, DWORD /*dwDestAttribs*/)
{
	switch (wFunc)
	{
	case FO_MOVE:
	case FO_DELETE:
	case FO_RENAME:
		if (pszSrcFile && pszSrcFile[0])
		{
			CString topDir;
			if (GitAdminDir::HasAdminDir(pszSrcFile, &topDir))
				m_CachedStatus.m_GitStatus.ReleasePath(topDir);
			m_remoteCacheLink.ReleaseLockForPath(CTGitPath(pszSrcFile));
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
