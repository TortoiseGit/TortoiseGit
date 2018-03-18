// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2017-2018 - TortoiseGit
// Copyright (C) 2003-2015 - TortoiseSVN

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

/**
 * \ingroup GitWCRev
 * This structure is used as the status baton for WC crawling
 * and contains all the information we are collecting.
 */
struct GitWCRev_t
{
	GitWCRev_t()
		: HasMods(FALSE)
		, HasUnversioned(FALSE)
		, bIsGitItem(FALSE)
		, bNoSubmodules(FALSE)
		, bHasSubmodule(FALSE)
		, bHasSubmoduleNewCommits(FALSE)
		, bHasSubmoduleMods(FALSE)
		, bHasSubmoduleUnversioned(FALSE)
		, bIsTagged(FALSE)
		, bIsUnborn(FALSE)
		, HeadTime(0)
		, NumCommits(0)
	{
		HeadHash[0] = '\0';
		HeadHashReadable[0] = '\0';
	}

	char HeadHash[GIT_OID_RAWSZ];
	char HeadHashReadable[GIT_OID_HEXSZ + 1];
	std::string HeadAuthor;
	std::string HeadEmail;
	__time64_t HeadTime;
	BOOL HasMods;					// True if local modifications found
	BOOL HasUnversioned;			// True if unversioned items found
	BOOL bIsGitItem;				// True if the item is under Git version control
	BOOL bNoSubmodules;				// If TRUE if submodules should be omitted
	BOOL bHasSubmodule;				// True if working tree has submodules
	BOOL bHasSubmoduleNewCommits;	// True if HEAD of submodule does not match committed rev. in parent repo
	BOOL bHasSubmoduleMods;			// True if local modifications in an submodule found
	BOOL bHasSubmoduleUnversioned;	// True if unversioned items in submodule found
	BOOL bIsTagged;					// True if HEAD is tagged
	BOOL bIsUnborn;					// True if branch in unborn
	size_t NumCommits;				// Number of commits for the current branch
	std::string CurrentBranch;		// Name of the current branch, SHA-1 if detached head
	std::set<std::string> ignorepatterns; // a list of file patterns to ignore
};
