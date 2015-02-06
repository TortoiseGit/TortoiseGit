// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2015 - TortoiseSI

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
#include "RootFolderCache.h"
#include "IntegrityActions.h"
#include "ShellExt.h"

std::vector<std::wstring> RootFolderCache::getRootFolders()
{
	std::lock_guard<std::mutex> lock(lockObject);

	return rootFolders;
};

bool RootFolderCache::refreshIfStale() 
{
	std::lock_guard<std::mutex> lock( lockObject );

	auto now = std::chrono::system_clock::now();

	if (now - lastRefresh > std::chrono::seconds(60) && !refreshInProgress) {
		refreshInProgress = true;
		std::async(std::launch::async, [&]{ this->updateFoldersList(); });
		return true;
	} else {
		return false;
	}
}

void RootFolderCache::forceRefresh() 
{
	{
		std::lock_guard<std::mutex> lock( lockObject );

		if (refreshInProgress) {
			return;
		}
		refreshInProgress = true;
	}

	std::async(std::launch::async, [&]{ this->updateFoldersList(); });
}

bool startsWith(std::wstring text, std::wstring prefix) 
{
	return text.length() >= prefix.length()
		&&
		text.substr(0, prefix.length()) == prefix;
}

bool RootFolderCache::isPathControlled(std::wstring path)
{
	refreshIfStale();

	std::transform(path.begin(), path.end(), path.begin(), ::tolower);

	{
		std::lock_guard<std::mutex> lock( lockObject );

		// TODO binary search...?
		for (std::wstring rootPath : rootFolders) {
			if (startsWith(path, rootPath)) {
				return true;
			}
		}
		return false;
	}
}

void RootFolderCache::updateFoldersList()
{
	std::vector<std::wstring> rootFolders = IntegrityActions::getControlledPaths(integritySession);

	// to lower case everything
	for (std::wstring& rootPath : rootFolders) {
		std::transform(rootPath.begin(), rootPath.end(), rootPath.begin(), ::tolower);
	}

	// lock cach and copy back
	std::vector<std::wstring> oldRootFolders;

	{
		std::lock_guard<std::mutex> lock( lockObject );

		oldRootFolders = this->rootFolders;
		this->rootFolders = rootFolders;
		this->lastRefresh = std::chrono::system_clock::now();
		this->refreshInProgress = false;
	}

	std::vector<std::wstring> foldersAddedOrRemoved;

	std::set_symmetric_difference(oldRootFolders.begin(), oldRootFolders.end(),
			rootFolders.begin(), rootFolders.end(), std::back_inserter(foldersAddedOrRemoved));

	// update shell with root folders that were added or removed
	for (std::wstring rootFolder : foldersAddedOrRemoved) {
		EventLog::writeDebug(L"sending update notification for " + rootFolder);

		SHChangeNotify(SHCNE_ATTRIBUTES, SHCNF_PATH| SHCNF_FLUSH, (LPCVOID) rootFolder.c_str(), NULL );
	}
}