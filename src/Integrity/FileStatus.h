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
	None		= 0x00000000l,
	Member		= 0x00000100l,
	FormerMember= 0x00000200l,
	Incoming	= 0x00000400l, // (member)
	Locked		= 0x00000800l, // (member)

	// local status
	Add			= 0x00000001l,
	Drop		= 0x00000002l, 
	Modified	= 0x00000004l,  // (member)
	Moved		= 0x00000008l,  // (member)
	Renamed		= 0x00000010l,  // (member)
	MergeNeeded = 0x00000020l,  // (member)

	// errors - note: these are not returned by Integrtiy, but are used
	//	              by the local code to handle errors talking to Integrity
	//			      Used by the caching layer to avoid caching error results
	TimeoutError= 0x10000000l,
	GenericError= 0x10000000l,

	// local stuff (ie things we figure out in the extension)
	Folder		= 0x00100000l,
	File		= 0x00200000l,
};

typedef int FileStatusFlags;

inline FileStatus operator& (FileStatusFlags flags, FileStatus status) 
{
	return (FileStatus) (flags & (FileStatusFlags)status);
}

inline FileStatusFlags operator| (FileStatusFlags flags, FileStatus status)
{
	return flags | (FileStatusFlags)status;
}

inline FileStatusFlags operator| (FileStatus status1, FileStatus status2)
{
	return ((FileStatusFlags)status1) | status2;
}

inline bool hasFileStatus(FileStatusFlags statusFlags, FileStatus status) 
{ 
	return (statusFlags & status) == status; 
}
