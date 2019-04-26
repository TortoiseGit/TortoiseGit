// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2014, 2016-2018 - TortoiseGit

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
#include "GitRev.h"
#include "Git.h"

class CGitDiff
{
public:
	CGitDiff() = delete;

	// if you change something here, also update SubmoduleDiffDlg.cpp and SubmoduleResolveConflictDlg.cpp!
	enum ChangeType
	{
		Unknown,
		Identical,
		NewSubmodule,
		DeleteSubmodule,
		FastForward,
		Rewind,
		NewerTime,
		OlderTime,
		SameTime
	};
	static void GetSubmoduleChangeType(CGit& subgit, const CGitHash& oldhash, const CGitHash& newhash, bool& oldOK, bool& newOK, ChangeType& changeType, CString& oldsub, CString& newsub);

	// Use two path to handle rename cases
	static int Diff(HWND hWnd, const CTGitPath* pPath1, const CTGitPath* pPath2, const CString& rev1, const CString& rev2, bool blame = false, bool unified = false, int jumpToLine = 0, bool bAlternativeTool = false, bool mustExist = true);
	static int SubmoduleDiff(HWND hWnd, const CTGitPath* pPath1, const CTGitPath* pPath2, const CGitHash& rev1, const CGitHash& rev2, bool blame = false, bool unified = false);
	static int DiffNull(HWND hWnd, const CTGitPath* pPath, const CString& rev1, bool bIsAdd = true, int jumpToLine = 0, bool bAlternative = false);
	static int DiffCommit(HWND hWnd, const CTGitPath& path, const GitRev* r1, const GitRev* r2, bool bAlternative = false);
	static int DiffCommit(HWND hWnd, const CTGitPath& path1, const CTGitPath& path2, const GitRev* r1, const GitRev* r2, bool bAlternative = false);
	static int DiffCommit(HWND hWnd, const CTGitPath& path, const CString& r1, const CString& r2, bool bAlternative = false);
	static int DiffCommit(HWND hWnd, const CTGitPath& path1, const CTGitPath& path2, const CString& r1, const CString& r2, bool bAlternative = false);
	static int SubmoduleDiffNull(HWND hWnd, const CTGitPath* pPath1, const CGitHash& hash);
};
