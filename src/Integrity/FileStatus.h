// TortoiseSI - a Windows shell extension for easy version control

// Copyright (C) 2015 - TortoiseSI

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

// flags representating the status of a file in PTC Integrity
enum class FileStatus
{
	// repo status
	None		= 0x00000000,
	Member		= 0x00000100,
	FormerMember= 0x00000200,
	Incoming	= 0x00000400, // (member)
	Locked		= 0x00000800, // (member)

	// local status
	Add			= 0x00000001,
	Drop		= 0x00000002, 
	Modified	= 0x00000004,  // (member)
	Moved		= 0x00000008,  // (member)
	Renamed		= 0x00000010,  // (member)
	MergeNeeded = 0x00000020,  // (member)

	// errors - note: these are not returned by Integrtiy, but are used
	//	              by the local code to handle errors talking to Integrity
	//			      Used by the caching layer to avoid caching error results
	TimeoutError= 0x10000000,
	GenericError= 0x10000000,
};

typedef int FileStatusFlags;

inline FileStatus operator& (FileStatusFlags flags, FileStatus status) {
	return (FileStatus) (flags & (FileStatusFlags)status);
}

inline bool hasFileStatus(FileStatusFlags statusFlags, FileStatus status) 
{ 
	return (statusFlags & status) == status; 
}
