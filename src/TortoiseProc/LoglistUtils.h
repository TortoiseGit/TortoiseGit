// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2012 - TortoiseGit
// Copyright (C) 2003-2008 - TortoiseSVN

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
 * \ingroup TortoiseProc
 * An utility class with static functions for GitLoglist.
 */
class CLoglistUtils
{
public:
	CLoglistUtils(void);
	~CLoglistUtils(void);

	/**
	 * FUNCTION    :   FormatDateAndTime
	 * DESCRIPTION :   Generates a displayable string from a CTime object in
	 *                 system short or long format  or as a relative value
	 *                 cTime - the time
	 *                 option - DATE_SHORTDATE or DATE_LONGDATE
	 *                 bIncluedeTime - whether to show time as well as date
	 *                 bRelative - if true then relative time is shown if reasonable
	 *                 If HKCU\Software\TortoiseGit\UseSystemLocaleForDates is 0 then use fixed format
	 *                 rather than locale
	 * RETURN      :   CString containing date/time
	 */
	static CString FormatDateAndTime(const CTime& cTime, DWORD option, bool bIncludeTime=true, bool bRelative=false);
	/**
	 *	Converts a given time to a relative display string (relative to current time)
	 *	Given time must be in local timezone
	 */
	static CString ToRelativeTimeString(CTime time);

private:
	/**
	 *	Generates a display string showing the relative time between the two given times as COleDateTimes
	 */
	static CString ToRelativeTimeString(COleDateTime time, COleDateTime RelativeTo);
	static CString ExpandRelativeTime(int count, UINT format_1, UINT format_n);
};
