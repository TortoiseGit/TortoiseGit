// TortoiseMerge - a Diff/Patch program

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
enum DiffStates
{
	DIFFSTATE_UNKNOWN,					///< e.g. an empty file
	DIFFSTATE_NORMAL,					///< no diffs found
	DIFFSTATE_REMOVED,					///< line was removed
	DIFFSTATE_REMOVEDWHITESPACE,		///< line was removed (whitespace diff)
	DIFFSTATE_ADDED,					///< line was added
	DIFFSTATE_ADDEDWHITESPACE,			///< line was added (whitespace diff)
	DIFFSTATE_WHITESPACE,				///< line differs in whitespaces only
	DIFFSTATE_WHITESPACE_DIFF,			///< the in-line diffs of whitespaces
	DIFFSTATE_EMPTY,					///< empty line
	DIFFSTATE_CONFLICTED,				///< conflicted line
	DIFFSTATE_CONFLICTED_IGNORED,		///< a conflict which isn't conflicted due to ignore settings
	DIFFSTATE_CONFLICTADDED,			///< added line results in conflict
	DIFFSTATE_CONFLICTEMPTY,			///< removed line results in conflict
	DIFFSTATE_CONFLICTRESOLVED,			///< previously conflicted line, now resolved
	DIFFSTATE_CONFLICTRESOLVEDEMPTY,	///< previously conflicted line, now resolved but empty line
	DIFFSTATE_IDENTICALREMOVED,			///< identical removed lines in theirs and yours
	DIFFSTATE_IDENTICALADDED,			///< identical added lines in theirs and yours
	DIFFSTATE_THEIRSREMOVED,			///< removed line in theirs
	DIFFSTATE_THEIRSADDED,				///< added line in theirs
	DIFFSTATE_YOURSREMOVED,				///< removed line in yours
	DIFFSTATE_YOURSADDED,				///< added line in yours
	DIFFSTATE_EDITED,					///< manually edited line
	DIFFSTATE_FILTEREDDIFF,				///< filtered-out diffs (e.g., ignored comments, regex filters, ...)
	DIFFSTATE_END						///< end marker for enum
};
