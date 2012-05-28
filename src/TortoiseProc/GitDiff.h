// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2012 - TortoiseGit

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
	CGitDiff(void);
	~CGitDiff(void);
	static int Parser(git_revnum_t &rev);

	// Use two path to handle rename cases
	static int Diff(CTGitPath * pPath1, CTGitPath *pPath2 ,git_revnum_t rev1, git_revnum_t rev2, bool blame=false, bool unified=false);
	static int SubmoduleDiff(CTGitPath * pPath1, CTGitPath *pPath2 ,git_revnum_t  rev1, git_revnum_t rev2, bool blame=false, bool unified=false);
	static int DiffNull(CTGitPath *pPath, git_revnum_t rev1,bool bIsAdd=true);
	static int DiffCommit(CTGitPath &path, GitRev *r1, GitRev *r2);
	static int DiffCommit(CTGitPath &path1, CTGitPath &path2, GitRev *r1, GitRev *r2);
	static int DiffCommit(CTGitPath &path, CString r1, CString r2);
	static int DiffCommit(CTGitPath &path1, CTGitPath &path2, CString r1, CString r2);
	static int SubmoduleDiffNull(CTGitPath *pPath1,git_revnum_t &rev1);
};
