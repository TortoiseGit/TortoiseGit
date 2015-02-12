// TortoiseSI- a Windows shell extension for easy version control

// Copyright (C) 2015 - TortoiseSI
// Copyright (C) 2008-2014 - TortoiseGit
// Copyright (C) 2003-2008 - TortoiseSVN

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
#include "menuinfo.h"
#include "resource.h"
#include "Globals.h"
#include "ShellExt.h"
#include "IntegrityActions.h"
#include "StatusCache.h"

// defaults are specified in ShellCache.h

MenuInfo menuSeperator = { MenuItem::Seperator, 0, 0, 0, [](const std::vector<std::wstring>&, HWND) {}, [](const std::vector<std::wstring>&, FileStatusFlags) { return true; } };

static const IntegritySession& getIntegritySession() {
	return IStatusCache::getInstance().getIntegritySession();
}

/**
 *  return true if there was a decentant path that was controlled
 */
bool warnIfPathHasControlledDecendantFolders(std::wstring path, HWND parentWindow) {
	std::transform(path.begin(), path.end(), path.begin(), ::tolower);

	// show the user where the sandboxes when they are decandants of the current folder since 
	// it may not be obvious to the user why they can't create a sandbox here
	std::wstring message;
	for (std::wstring rootFolder : IStatusCache::getInstance().getRootFolderCache().getRootFolders()) {
		if (startsWith(rootFolder, path)) {
			if (message.empty()) {
				message = getTortoiseSIString(IDS_SANDBOX_NOTALLOWED1);
			}
			message += L"\n\t '" + rootFolder + L"'";
		}
	}

	if (!message.empty()) {
		MessageBoxW(parentWindow, message.c_str(), NULL, MB_ICONERROR);
		return true;
	} else {
		return false;
	}
}

std::vector<MenuInfo> menuInfo =
{
	{ MenuItem::ViewSandbox, 0, IDS_VIEW_SANDBOX, IDS_VIEW_SANDBOX,
		[](const std::vector<std::wstring>& selectedItems, HWND)
		{
			IntegrityActions::launchSandboxView(getIntegritySession(), selectedItems.front());
		},
		[](const std::vector<std::wstring>& selectedItems, FileStatusFlags selectedItemsStatus)
		{
			return selectedItems.size() == 1 &&
				hasFileStatus(selectedItemsStatus, FileStatus::Folder) &&
				hasFileStatus(selectedItemsStatus, FileStatus::Member);
		}
	},
	{ MenuItem::CreateSandbox, 0, IDS_CREATE_SANDBOX, IDS_CREATE_SANDBOX,
		[](const std::vector<std::wstring>& selectedItems, HWND parentWindow)
		{
			if (warnIfPathHasControlledDecendantFolders(selectedItems.front(), parentWindow)) {
				return;
			}

			IntegrityActions::createSandbox(getIntegritySession(), selectedItems.front(),
				[]{ IStatusCache::getInstance().getRootFolderCache().forceRefresh(); });
			
		},
		[](const std::vector<std::wstring>& selectedItems, FileStatusFlags selectedItemsStatus) 
		{
			return selectedItems.size() == 1 &&
				hasFileStatus(selectedItemsStatus, FileStatus::Folder) &&
				!hasFileStatus(selectedItemsStatus, FileStatus::Member);
		}
	},
	menuSeperator,

};
