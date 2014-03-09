// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013 - TortoiseGit

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

#define BLAME_DETECT_MOVED_OR_COPIED_LINES_DISABLED 0
#define BLAME_DETECT_MOVED_OR_COPIED_LINES_WITHIN_FILE 1
#define BLAME_DETECT_MOVED_OR_COPIED_LINES_FROM_MODIFIED_FILES 2
#define BLAME_DETECT_MOVED_OR_COPIED_LINES_FROM_EXISTING_FILES_AT_FILE_CREATION 3
#define BLAME_DETECT_MOVED_OR_COPIED_LINES_FROM_EXISTING_FILES 4

#define BLAME_DETECT_MOVED_OR_COPIED_LINES_NUM_CHARACTERS_WITHIN_FILE_DEFAULT 20
#define BLAME_DETECT_MOVED_OR_COPIED_LINES_NUM_CHARACTERS_FROM_FILES_DEFAULT 40

inline bool BlameIsLimitedToOneFilename(DWORD dwDetectMovedOrCopiedLines)
{
	return (dwDetectMovedOrCopiedLines == BLAME_DETECT_MOVED_OR_COPIED_LINES_DISABLED || dwDetectMovedOrCopiedLines == BLAME_DETECT_MOVED_OR_COPIED_LINES_WITHIN_FILE);
}
