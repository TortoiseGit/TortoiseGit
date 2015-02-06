// TortoiseSI - a Windows shell extension for easy version control

// Copyright (C) 2015 - TortoiseSI
// Copyright (C) 2011-2014 - TortoiseGit
// Copyright (C) 2003-2013 - TortoiseSVN

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
#include "UnicodeUtils.h"
#include "ShellExt.h"
#include "ShellObjects.h"
#include "..\version.h"
#undef swprintf

extern ShellObjects g_shellObjects;

// *********************** CShellExt *************************
CShellExt::CShellExt(FileState state)
	: m_State(state)
	, selectedItemsStatus(0)
	, currentFolderIsControlled(false)
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
	if ((g_langid != g_ShellCache.GetLangID())&&((g_langTimeout == 0)||(g_langTimeout < GetTickCount())))
	{
		g_langid = g_ShellCache.GetLangID();
		DWORD langId = g_langid;
		TCHAR langDll[MAX_PATH*4] = {0};
		HINSTANCE hInst = NULL;
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

				if (pBuffer != (void*) NULL)
				{
					UINT        nInfoSize = 0;
					UINT        nFixedLength = 0;
					LPSTR       lpVersion = NULL;
					VOID*       lpFixedPointer;
					TRANSARRAY* lpTransArray;
					TCHAR       strLangProductVersion[MAX_PATH] = {0};

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
							lpTransArray = (TRANSARRAY*) lpFixedPointer;

							_stprintf_s(strLangProductVersion, _T("\\StringFileInfo\\%04x%04x\\ProductVersion"),
								lpTransArray[0].wLanguageID, lpTransArray[0].wCharacterSet);

							if (VerQueryValue(pBuffer,
								(LPTSTR)strLangProductVersion,
								(LPVOID *)&lpVersion,
								&nInfoSize))
							{
								versionmatch = (_tcscmp((LPCTSTR)lpVersion, _T(STRPRODUCTVER)) == 0);
							}

						}
					}
					free(pBuffer);
				} // if (pBuffer != (void*) NULL)
			} // if (dwBufferSize > 0)
			else
				versionmatch = FALSE;

			if (versionmatch)
				hInst = LoadLibrary(langDll);
			if (hInst != NULL)
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
				{
					langId = MAKELANGID(PRIMARYLANGID(langId), lid);
				}
				else
					langId = 0;
			}
		} while ((hInst == NULL) && (langId != 0));
		if (hInst == NULL)
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
				g_langTimeout = GetTickCount() + 10000;
		}
		else
			g_langTimeout = 0;
	} // if (g_langid != g_ShellCache.GetLangID())
}

STDMETHODIMP CShellExt::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
	if(ppv == 0)
		return E_POINTER;

	*ppv = NULL;

	if (IsEqualIID(riid, IID_IShellExtInit) || IsEqualIID(riid, IID_IUnknown))
	{
		*ppv = static_cast<LPSHELLEXTINIT>(this);
	}
	else if (IsEqualIID(riid, IID_IContextMenu))
	{
		*ppv = static_cast<LPCONTEXTMENU>(this);
	}
	else if (IsEqualIID(riid, IID_IShellIconOverlayIdentifier))
	{
		*ppv = static_cast<IShellIconOverlayIdentifier*>(this);
	}
	else if (IsEqualIID(riid, IID_IShellPropSheetExt))
	{
		*ppv = static_cast<LPSHELLPROPSHEETEXT>(this);
	}
	else
	{
		return E_NOINTERFACE;
	}

	AddRef();
	return S_OK;
}

STDMETHODIMP_(ULONG) CShellExt::AddRef()
{
	return ++m_cRef;
}

STDMETHODIMP_(ULONG) CShellExt::Release()
{
	if (--m_cRef)
		return m_cRef;

	delete this;

	return 0L;
}

namespace EventLog {
	void writeDebug(std::wstring info) {
		if (g_ShellCache.IsDebugLogging()) {
			EventLog::writeInformation(info);
		}
	}
}

std::wstring getTortoiseSIString(DWORD stringID)
{
	wchar_t buffer[1024];
	LoadStringEx(g_hResInst, stringID, buffer, _countof(buffer), (WORD)CRegStdDWORD(_T("Software\\TortoiseSI\\LanguageID"), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)));
	return buffer;
}