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

MenuInfo menuSeperator = { MenuItem::Seperator, 0, 0, 0, [](CShellExt*) {}, [](CShellExt*) { return true; } };

static const IntegritySession& getIntegritySession() {
	return ICache::getInstance().getIntegritySession();
}

std::vector<MenuInfo> menuInfo =
{
	{ MenuItem::ViewSandbox, 0, IDS_VIEW_SANDBOX, IDS_VIEW_SANDBOX,
		[](CShellExt* shell) 
		{
			std::wstring path;
			if (shell->getSelectedItems().size() == 0) {
				path = shell->getCurrentFolder();
			} else {
				path = shell->getSelectedItems().front();
			}
			IntegrityActions::launchSandboxView(getIntegritySession(), path);
		},
		[](CShellExt* shell) { 
			return (shell->getSelectedItems().size() == 0 && shell->isCurrentFolderControlled())
											||
					  (shell->getSelectedItems().size() == 1 &&
								  hasFileStatus(shell->getSelectedItemsStatus(), FileStatus::Folder) &&
								  hasFileStatus(shell->getSelectedItemsStatus(), FileStatus::Member)); 
		}
	},
	{ MenuItem::CreateSandbox, 0, IDS_CREATE_SANDBOX, IDS_CREATE_SANDBOX,
	[](CShellExt* shell)
		{
			std::wstring path;
			if (shell->getSelectedItems().size() == 0) {
				path = shell->getCurrentFolder();
			} else {
				path = shell->getSelectedItems().front();
			}

			IntegrityActions::createSandbox(getIntegritySession(), path,
				[]{ ICache::getInstance().getRootFolderCache().forceRefresh(); });
			
		},
		[](CShellExt* shell) { 
			return (shell->getSelectedItems().size() == 0 && !shell->isCurrentFolderControlled())
							||
						(shell->getSelectedItems().size() == 1 &&
							hasFileStatus(shell->getSelectedItemsStatus(), FileStatus::Folder) &&
							!hasFileStatus(shell->getSelectedItemsStatus(), FileStatus::Member)); 
		}
	},
	menuSeperator,

};
