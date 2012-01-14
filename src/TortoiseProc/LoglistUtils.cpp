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
#include "StdAfx.h"
#include "math.h"
#include "..\Resources\LoglistCommonResource.h"
#include "LoglistUtils.h"
#include "Registry.h"

CLoglistUtils::CLoglistUtils(void)
{
}

CLoglistUtils::~CLoglistUtils(void)
{
}

/**
 * FUNCTION    :   FormatDateAndTime
 * DESCRIPTION :   Generates a displayable string from a CTime object in
 *                 system short or long format  or as a relative value
 *				   cTime - the time
 *				   option - DATE_SHORTDATE or DATE_LONGDATE
 *				   bIncluedeTime - whether to show time as well as date
 *				   bRelative - if true then relative time is shown if reasonable
 *				   If HKCU\Software\TortoiseGit\UseSystemLocaleForDates is 0 then use fixed format
 *				   rather than locale
 * RETURN      :   CString containing date/time
 */
CString CLoglistUtils::FormatDateAndTime(const CTime& cTime, DWORD option, bool bIncludeTime /*=true*/, bool bRelative /*=false*/)
{
	if (bRelative)
	{
		return ToRelativeTimeString(cTime);
	}
	else
	{
		// should we use the locale settings for formatting the date/time?
		if (CRegDWORD(_T("Software\\TortoiseGit\\UseSystemLocaleForDates"), TRUE))
		{
			// yes
			SYSTEMTIME sysTime;
			cTime.GetAsSystemTime(sysTime);

			TCHAR buf[100];

			GetDateFormat(LOCALE_USER_DEFAULT, option, &sysTime, NULL, buf, _countof(buf) - 1);
			CString datetime = buf;
			if (bIncludeTime)
			{
				datetime += _T(" ");
				GetTimeFormat(LOCALE_USER_DEFAULT, 0, &sysTime, NULL, buf, _countof(buf) - 1);
				datetime += buf;
			}
			return datetime;
		}
		else
		{
			// no, so fixed format
			if (bIncludeTime)
			{
				return cTime.Format(_T("%Y-%m-%d %H:%M:%S"));
			}
			else
			{
				return cTime.Format(_T("%Y-%m-%d"));
			}
		}
	}
}

/**
 *	Converts a given time to a relative display string (relative to current time)
 *	Given time must be in local timezone
 */
CString CLoglistUtils::ToRelativeTimeString(CTime time)
{
	// convert to COleDateTime
	SYSTEMTIME sysTime;
	time.GetAsSystemTime(sysTime);
	COleDateTime oleTime(sysTime);
	return ToRelativeTimeString(oleTime, COleDateTime::GetCurrentTime());
}

/**
 *	Generates a display string showing the relative time between the two given times as COleDateTimes
 */
CString CLoglistUtils::ToRelativeTimeString(COleDateTime time,COleDateTime RelativeTo)
{
	COleDateTimeSpan ts = RelativeTo - time;

	//years
	if(fabs(ts.GetTotalDays()) >= 3 * 365)
		return ExpandRelativeTime((int)ts.GetTotalDays()/365, IDS_YEAR_AGO, IDS_YEARS_AGO);

	//Months
	if(fabs(ts.GetTotalDays()) >= 60)
		return ExpandRelativeTime((int)ts.GetTotalDays()/30, IDS_MONTH_AGO, IDS_MONTHS_AGO);

	//Weeks
	if(fabs(ts.GetTotalDays()) >= 14)
		return ExpandRelativeTime((int)ts.GetTotalDays()/7, IDS_WEEK_AGO, IDS_WEEKS_AGO);

	//Days
	if(fabs(ts.GetTotalDays()) >= 2)
		return ExpandRelativeTime((int)ts.GetTotalDays(), IDS_DAY_AGO, IDS_DAYS_AGO);

	//hours
	if(fabs(ts.GetTotalHours()) >= 2)
		return ExpandRelativeTime((int)ts.GetTotalHours(), IDS_HOUR_AGO, IDS_HOURS_AGO);

	//minutes
	if(fabs(ts.GetTotalMinutes()) >= 2)
		return ExpandRelativeTime((int)ts.GetTotalMinutes(), IDS_MINUTE_AGO, IDS_MINUTES_AGO);

	//seconds
	return ExpandRelativeTime((int)ts.GetTotalSeconds(), IDS_SECOND_AGO, IDS_SECONDS_AGO);
}

/**
 * Passed a value and two resource string ids
 * if count is 1 then FormatString is called with format_1 and the value
 * otherwise format_2 is used
 * the formatted string is returned
*/
CString CLoglistUtils::ExpandRelativeTime(int count, UINT format_1, UINT format_n)
{
	CString answer;
	if (count == 1)
		answer.FormatMessage(format_1, count);
	else
		answer.FormatMessage(format_n, count);

	return answer;
}
