// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2006, 2015 - TortoiseSVN

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
#pragma once

#include "TGitPath.h"

/**
* \ingroup Utils
* This singleton class contains a list of items which require a shell-update notification
* This update is done lazily at the end of a run of Git operations
*/
class CShellUpdater
{
private:
	CShellUpdater(void);
	~CShellUpdater(void);
	// prevent cloning
	CShellUpdater(const CShellUpdater&) = delete;
	CShellUpdater& operator=(const CShellUpdater&) = delete;
public:
	static CShellUpdater& Instance();

public:
	/**
	 * Add a single path for updating.
	 * The update will happen at some suitable time in the future
	 */
	void AddPathForUpdate(const CTGitPath& path);
	/**
	 * Add a list of paths for updating.
	 * The update will happen at some suitable time in the future
	 */
	void AddPathsForUpdate(const CTGitPathList& pathList);
	/**
	 * Do the update, and clear the list of items waiting
	 */
	void Flush();

	static bool RebuildIcons();

private:
	void UpdateShell();

private:
	// The list of paths which will need updating
	CTGitPathList m_pathsForUpdating;
	// A handle to an event which, when set, tells the ShellExtension to purge its status cache
	HANDLE			m_hInvalidationEvent;
};
