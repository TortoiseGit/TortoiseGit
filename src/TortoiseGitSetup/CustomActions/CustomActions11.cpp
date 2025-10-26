﻿// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2021, 2025 - TortoiseGit
// Copyright (C) 2021 - TortoiseSVN

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
#include <shlwapi.h>
#include <winrt/Windows.Management.Deployment.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.ApplicationModel.h>
#include <oobenotification.h>

#pragma comment(lib, "windowsapp.lib")

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Management::Deployment;

BOOL APIENTRY DllMain(HANDLE /*hModule*/, DWORD /*ul_reason_for_call*/, LPVOID /*lpReserved*/)
{
	return TRUE;
}

UINT __stdcall RegisterSparsePackage(MSIHANDLE hModule)
{
	if (BOOL isOOBEComplete = false, ooBECompleteOk = OOBEComplete(&isOOBEComplete); !ooBECompleteOk || !isOOBEComplete)
	{
		PMSIHANDLE hRecord = MsiCreateRecord(0);
		std::wstring error = L"Skipping RegisterSparsePackage (OOBEComplete: ";
		error += std::to_wstring(ooBECompleteOk);
		error += L", isOOBEComplete: ";
		error += std::to_wstring(isOOBEComplete);
		error += L"):\n";
		MsiRecordSetStringW(hRecord, 0, error.c_str());
		MsiProcessMessage(hModule, INSTALLMESSAGE_INFO, hRecord);
		MsiCloseHandle(hRecord);
		return ERROR_SUCCESS;
	}

	DWORD len = 0;
	wchar_t emptyBuf[1]{};
	MsiGetPropertyW(hModule, L"INSTALLDIR", emptyBuf, &len);
	auto sparseExtPath = std::make_unique<wchar_t[]>(len + 1LL);
	len += 1;
	MsiGetPropertyW(hModule, L"INSTALLDIR", sparseExtPath.get(), &len);

	len = 0;
	MsiGetPropertyW(hModule, L"SPARSEPACKAGEFILE", emptyBuf, &len);
	auto sparsePackageFile = std::make_unique<wchar_t[]>(len + 1LL);
	len += 1;
	MsiGetPropertyW(hModule, L"SPARSEPACKAGEFILE", sparsePackageFile.get(), &len);

	std::wstring sSparsePackagePath = sparseExtPath.get();
	sSparsePackagePath += L"\\bin\\";
	sSparsePackagePath += sparsePackageFile.get();

	PackageManager manager;
	AddPackageOptions options;
	Uri externalUri(sparseExtPath.get());
	Uri packageUri(sSparsePackagePath.c_str());
	options.ExternalLocationUri(externalUri);
	auto deploymentOperation = manager.AddPackageByUriAsync(packageUri, options);

	auto deployResult = deploymentOperation.get();

	if (!SUCCEEDED(deployResult.ExtendedErrorCode()))
	{
		// Deployment failed
		PMSIHANDLE hRecord = MsiCreateRecord(0);
		std::wstring error = L"AddPackageByUriAsync failed (Errorcode: ";
		error += std::to_wstring(deployResult.ExtendedErrorCode());
		error += L"):\n";
		error += deployResult.ErrorText();
		MsiRecordSetStringW(hRecord, 0, error.c_str());
		MsiProcessMessage(hModule, INSTALLMESSAGE_ERROR, hRecord);
		MsiCloseHandle(hRecord);
	}
	return ERROR_SUCCESS;
}

UINT __stdcall UnregisterSparsePackage(MSIHANDLE hModule)
{
	DWORD len = 0;
	wchar_t emptyBuf[1]{};
	MsiGetPropertyW(hModule, L"SPARSEPACKAGENAME", emptyBuf, &len);
	auto sparsePackageName = std::make_unique<wchar_t[]>(len + 1LL);
	len += 1;
	MsiGetPropertyW(hModule, L"SPARSEPACKAGENAME", sparsePackageName.get(), &len);

	PackageManager packageManager;
	winrt::Windows::Foundation::Collections::IIterable<winrt::Windows::ApplicationModel::Package> packages;
	try
	{
		packages = packageManager.FindPackagesForUser(L"");
	}
	catch (winrt::hresult_error const& ex)
	{
		PMSIHANDLE hRecord = MsiCreateRecord(0);
		std::wstring error = L"FindPackagesForUser failed (Errorcode: ";
		error += std::to_wstring(ex.code().value);
		error += L"):\n";
		error += ex.message();
		MsiRecordSetStringW(hRecord, 0, error.c_str());
		MsiProcessMessage(hModule, INSTALLMESSAGE_ERROR, hRecord);
		MsiCloseHandle(hRecord);

		return ERROR_SUCCESS;
	}

	for (const auto& package : packages)
	{
		if (package.Id().Name() != sparsePackageName.get())
			continue;

		winrt::hstring fullName = package.Id().FullName();
		auto deploymentOperation = packageManager.RemovePackageAsync(fullName, RemovalOptions::None);
		auto deployResult = deploymentOperation.get();
		if (SUCCEEDED(deployResult.ExtendedErrorCode()))
			break;

		// Undeployment failed
		PMSIHANDLE hRecord = MsiCreateRecord(0);
		std::wstring error = L"RemovePackageAsync failed (Errorcode: ";
		error += std::to_wstring(deployResult.ExtendedErrorCode());
		error += L"):\n";
		error += deployResult.ErrorText();
		MsiRecordSetStringW(hRecord, 0, error.c_str());
		MsiProcessMessage(hModule, INSTALLMESSAGE_ERROR, hRecord);
		MsiCloseHandle(hRecord);
	}

	return ERROR_SUCCESS;
}
