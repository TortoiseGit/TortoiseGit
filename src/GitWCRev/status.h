// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2017-2018 - TortoiseGit

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

#pragma once
#include "GitWCRev.h"

// Internal error codes
// Note: these error codes are documented in /doc/source/en/TortoiseGit/tgit_GitWCRev.xml
#define ERR_SYNTAX		1	// Syntax error
#define ERR_FNF			2	// File/folder not found
#define ERR_OPEN		3	// File open error
#define ERR_ALLOC		4	// Memory allocation error
#define ERR_READ		5	// File read/write/size error
#define ERR_GIT_ERR		6	// Git error
// Documented error codes
#define ERR_GIT_MODS	7	// Local mods found (-mM)
#define ERR_OUT_EXISTS	9	// Output file already exists (-d)
#define ERR_NOWC		10	// the path is not a working copy or part of one
#define ERR_GIT_UNVER	11	// Unversioned items found (-uU)

int GetStatus(const TCHAR* wc, GitWCRev_t& GitStat);
int GetStatusUnCleanPath(const TCHAR* wc, GitWCRev_t& GitStat);
