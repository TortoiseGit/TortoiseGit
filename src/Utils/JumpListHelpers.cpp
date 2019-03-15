// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2013 - TortoiseSVN

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
#include "SmartHandle.h"
#include "JumpListHelpers.h"
#include "propvarutil.h"
#include "propsys.h"
#include <propkey.h>

HRESULT SetAppID(LPCTSTR appID)
{
	HRESULT hRes = S_FALSE;
	typedef HRESULT STDAPICALLTYPE SetCurrentProcessExplicitAppUserModelIDFN(PCWSTR AppID);
	CAutoLibrary hShell = AtlLoadSystemLibraryUsingFullPath(L"shell32.dll");
	if (hShell)
	{
		auto pfnSetCurrentProcessExplicitAppUserModelID = reinterpret_cast<SetCurrentProcessExplicitAppUserModelIDFN*>(GetProcAddress(hShell, "SetCurrentProcessExplicitAppUserModelID"));
		if (pfnSetCurrentProcessExplicitAppUserModelID)
			hRes = pfnSetCurrentProcessExplicitAppUserModelID(appID);
	}
	return hRes;
}

HRESULT CreateShellLink(PCWSTR pszArguments, PCWSTR pszTitle, int iconIndex, IShellLink **ppsl)
{
	ATL::CComPtr<IShellLink> psl;
	HRESULT hr = psl.CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER);
	if (FAILED(hr))
		return hr;

	WCHAR szAppPath[MAX_PATH] = {0};
	if (GetModuleFileName(nullptr, szAppPath, ARRAYSIZE(szAppPath)) == 0)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		return hr;
	}
	hr = psl->SetPath(szAppPath);
	if (FAILED(hr))
		return hr;

	hr = psl->SetArguments(pszArguments);
	if (FAILED(hr))
		return hr;

	hr = psl->SetIconLocation(szAppPath, iconIndex);
	if (FAILED(hr))
		return hr;

	ATL::CComPtr<IPropertyStore> pps;
	hr = psl.QueryInterface(&pps);
	if (FAILED(hr))
		return hr;

	PROPVARIANT propvar;
	hr = InitPropVariantFromString(pszTitle, &propvar);
	if (SUCCEEDED(hr))
	{
		hr = pps->SetValue(PKEY_Title, propvar);
		if (SUCCEEDED(hr)) {
			hr = pps->Commit();
			if (SUCCEEDED(hr)) {
				hr = psl.QueryInterface(ppsl);
			}
		}
		PropVariantClear(&propvar);
	}
	return hr;
}

HRESULT CreateSeparatorLink(IShellLink **ppsl)
{
	ATL::CComPtr<IPropertyStore> pps;
	HRESULT hr = pps.CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER);
	if (FAILED(hr))
		return hr;

	PROPVARIANT propvar;
	hr = InitPropVariantFromBoolean(TRUE, &propvar);
	if (FAILED(hr))
		return hr;

	hr = pps->SetValue(PKEY_AppUserModel_IsDestListSeparator, propvar);
	if (SUCCEEDED(hr)) {
		hr = pps->Commit();
		if (SUCCEEDED(hr))
		{
			hr = pps.QueryInterface(ppsl);
		}
	}
	PropVariantClear(&propvar);
	return hr;
}

bool IsItemInArray(IShellItem *psi, IObjectArray *poaRemoved)
{
	UINT cItems;
	if (FAILED(poaRemoved->GetCount(&cItems)))
		return false;

	bool fRet = false;
	for (UINT i = 0; !fRet && i < cItems; i++) {
		ATL::CComPtr<IShellItem> psiCompare;
		if (FAILED(poaRemoved->GetAt(i, IID_PPV_ARGS(&psiCompare))))
			continue;
		int iOrder;
		fRet = SUCCEEDED(psiCompare->Compare(psi, SICHINT_CANONICAL, &iOrder)) && (0 == iOrder);
	}
	return fRet;
}

void DeleteJumpList(LPCTSTR appID)
{
	ATL::CComPtr<ICustomDestinationList> pcdl;
	HRESULT hr = pcdl.CoCreateInstance(CLSID_DestinationList, nullptr, CLSCTX_INPROC_SERVER);
	if (SUCCEEDED(hr)) {
		pcdl->DeleteList(appID);
	}
}
