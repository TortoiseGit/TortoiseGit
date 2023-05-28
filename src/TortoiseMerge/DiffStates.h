// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2023 - TortoiseGit
// Copyright (C) 2007-2008, 2010, 2013, 2017 - TortoiseSVN

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

/**
 * \ingroup TortoiseMerge
 * the different diff states a line can have.
 */
enum class DiffState
{
	Unknown,				///< e.g. an empty file
	Normal,					///< no diffs found
	Removed,				///< line was removed
	RemovedWhitespace,		///< line was removed (whitespace diff)
	Added,					///< line was added
	AddedWhitespace,		///< line was added (whitespace diff)
	Whitespace,				///< line differs in whitespaces only
	WhitespaceDiff,			///< the in-line diffs of whitespaces
	Empty,					///< empty line
	Conflicted,				///< conflicted line
	Conflicted_Ignored,		///< a conflict which isn't conflicted due to ignore settings
	ConflictAdded,			///< added line results in conflict
	ConflictEmpty,			///< removed line results in conflict
	ConflictResolved,		///< previously conflicted line, now resolved
	ConflictResolvedEmpty,	///< previously conflicted line, now resolved but empty line
	IdenticalRemoved,		///< identical removed lines in theirs and yours
	IdenticalAdded,			///< identical added lines in theirs and yours
	TheirsRemoved,			///< removed line in theirs
	TheirsAdded,			///< added line in theirs
	YoursRemoved,			///< removed line in yours
	YoursAdded,				///< added line in yours
	Edited,					///< manually edited line
	FilteredDiff,			///< filtered-out diffs (e.g., ignored comments, regex filters, ...)
	End						///< end marker for enum
};
