// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2012 - TortoiseGit
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
public:
	GitAdminDir(void);
	~GitAdminDir(void);

	/// Returns true if the path points to or below an admin directory
	bool IsAdminDirPath(const CString& path) const;

	/// Returns true if the path points is a bare repository
	bool IsBareRepo(const CString& path) const;

	/// Returns true if the path (file or folder) has an admin directory
	/// associated, i.e. if the path is in a working copy.
	bool HasAdminDir(const CString& path) const;
	bool HasAdminDir(const CString& path,CString * ProjectTopDir) const;
	bool HasAdminDir(const CString& path, bool bDir,CString * ProjectTopDir=NULL) const;
	CString GetSuperProjectRoot(const CString& path);

	bool GetAdminDirPath(const CString &projectTopDir, CString &adminDir) const;

	CString GetGitTopDir(const CString& path);

	CString GetAdminDirName() const {return _T(".git");}
};

extern GitAdminDir g_GitAdminDir;
