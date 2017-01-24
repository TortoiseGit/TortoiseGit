// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2014, 2016-2017 - TortoiseGit

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
#include "GitStatus.h"
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
	static void GetSubmoduleChangeType(CGit& subgit, const CString& oldhash, const CString& newhash, bool& oldOK, bool& newOK, ChangeType& changeType, CString& oldsub, CString& newsub);

	// Use two path to handle rename cases
	static int Diff(const CTGitPath* pPath1, const CTGitPath* pPath2, CString rev1, CString rev2, bool blame = false, bool unified = false, int jumpToLine = 0, bool bAlternativeTool = false);
	static int SubmoduleDiff(const CTGitPath* pPath1, const CTGitPath* pPath2, const CString& rev1, const CString& rev2, bool blame = false, bool unified = false);
	static int DiffNull(const CTGitPath* pPath, CString rev1, bool bIsAdd = true, int jumpToLine = 0, bool bAlternative = false);
	static int DiffCommit(const CTGitPath& path, const GitRev* r1, const GitRev* r2, bool bAlternative = false);
	static int DiffCommit(const CTGitPath& path1, const CTGitPath& path2, const GitRev* r1, const GitRev* r2, bool bAlternative = false);
	static int DiffCommit(const CTGitPath& path, const CString& r1, const CString& r2, bool bAlternative = false);
	static int DiffCommit(const CTGitPath& path1, const CTGitPath& path2, const CString& r1, const CString& r2, bool bAlternative = false);
	static int SubmoduleDiffNull(const CTGitPath* pPath1, const CString& rev1);
};
