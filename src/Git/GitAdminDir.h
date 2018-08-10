// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2012, 2015-2017 - TortoiseGit
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

#pragma once

class GitAdminDir
{
private:
	GitAdminDir() = delete;

public:
	/// Returns true if the path points to or below an admin directory
	static bool IsAdminDirPath(const CString& path);

	static bool IsWorkingTreeOrBareRepo(const CString& path);

	/// Returns true if the path points is a bare repository
	static bool IsBareRepo(const CString& path);

	/// Returns true if the path (file or folder) has an admin directory
	/// associated, i.e. if the path is in a working copy.
	static bool HasAdminDir(const CString& path);
	static bool HasAdminDir(const CString& path, CString* ProjectTopDir);
	// IsAdminDirPath is only touched/set to true if and only if we the path is an AdminDirPath
	static bool HasAdminDir(const CString& path, bool bDir, CString* ProjectTopDir = nullptr, bool* IsAdminDirPath = nullptr);
	static CString GetSuperProjectRoot(const CString& path);

	static bool GetAdminDirPath(const CString& projectTopDir, CString& adminDir, bool* isWorktree = nullptr);
	static bool GetWorktreeAdminDirPath(const CString& projectTopDir, CString& adminDir);
	static CString ReadGitLink(const CString& topDir, const CString& dotGitPath);

	static CString GetAdminDirName() { return L".git"; }
};
