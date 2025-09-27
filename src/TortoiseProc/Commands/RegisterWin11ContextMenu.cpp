// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2023-2025 - TortoiseGit
// Copyright (C) 2025 - TortoiseSVN

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
#include "RegisterWin11ContextMenu.h"
#include "PathUtils.h"
#include "UnicodeUtils.h"
#include <winrt/Windows.Management.Deployment.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.ApplicationModel.h>
#include <future>

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Management::Deployment;

#pragma comment(lib, "windowsapp.lib")

std::wstring RegisterWin11ContextMenuCommand::ReRegisterPackage()
{
	auto future = std::async([]() -> std::wstring {
		// use a new thread to do the initialization
		try
		{
			CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
			SCOPE_EXIT { CoUninitialize(); };

			PackageManager manager;
			// first unregister if already registered
			Collections::IIterable<winrt::Windows::ApplicationModel::Package> packages;
			try
			{
				packages = manager.FindPackagesForUser(L"");
			}
			catch (winrt::hresult_error const& ex)
			{
				std::wstring error = L"FindPackagesForUser failed (Errorcode: ";
				error += std::to_wstring(ex.code().value);
				error += L"):\n";
				error += ex.message();
				return error;
			}

			for (const auto& package : packages)
			{
				if (package.Id().Name() != L"0BF99681-825C-4B2A-A14F-2AC01DB9B70E")
					continue;

				winrt::hstring fullName = package.Id().FullName();
				auto deploymentOperation = manager.RemovePackageAsync(fullName, RemovalOptions::None);
				auto deployResult = deploymentOperation.get();
				if (SUCCEEDED(deployResult.ExtendedErrorCode()))
					break;

				// Undeployment failed
				std::wstring error = L"RemovePackageAsync failed (Errorcode: ";
				error += std::to_wstring(deployResult.ExtendedErrorCode());
				error += L"):\n";
				error += deployResult.ErrorText();
				return error;
			}

			// now register the package
			auto appDir = CPathUtils::GetAppParentDirectory();
			Uri externalUri(static_cast<LPCWSTR>(appDir));
			auto packagePath = appDir + L"bin\\package.msix";
			Uri packageUri(static_cast<LPCWSTR>(packagePath));
			AddPackageOptions options;
			options.ExternalLocationUri(externalUri);
			auto deploymentOperation = manager.AddPackageByUriAsync(packageUri, options);
			if (auto deployResult = deploymentOperation.get(); !SUCCEEDED(deployResult.ExtendedErrorCode()))
			{
				std::wstring error = L"AddPackageByUriAsync failed (Errorcode: ";
				error += std::to_wstring(deployResult.ExtendedErrorCode());
				error += L"):\n";
				error += deployResult.ErrorText();
				return error;
			}
		}
		catch (const std::exception& ex)
		{
			return CUnicodeUtils::StdGetUnicode(ex.what());
		}
		return {};
	});
	return future.get();
}
