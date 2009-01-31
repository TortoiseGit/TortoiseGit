// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseGit

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
#include "resource.h"
#include "TortoiseGitBlameAppUtils.h"
#include "Registry.h"

CAppUtils::CAppUtils(void)
{
}

CAppUtils::~CAppUtils(void)
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
CString CAppUtils::FormatDateAndTime( const CTime& cTime, DWORD option, bool bIncludeTime /*=true*/,
	bool bRelative /*=false*/)
{
	CString datetime;
	if ( bRelative )
	{
		datetime = ToRelativeTimeString( cTime );
	}
	else
	{
		// should we use the locale settings for formatting the date/time?
		if (CRegDWORD(_T("Software\\TortoiseGit\\UseSystemLocaleForDates"), TRUE))
		{
			// yes
			SYSTEMTIME sysTime;
			cTime.GetAsSystemTime( sysTime );
			
			TCHAR buf[100];
			
			GetDateFormat(LOCALE_USER_DEFAULT, option, &sysTime, NULL, buf, 
				sizeof(buf)/sizeof(TCHAR)-1);
			datetime = buf;
			if ( bIncludeTime )
			{
				datetime += _T(" ");
				GetTimeFormat(LOCALE_USER_DEFAULT, 0, &sysTime, NULL, buf, sizeof(buf)/sizeof(TCHAR)-1);
				datetime += buf;
			}
		}
		else
		{
			// no, so fixed format
			if ( bIncludeTime )
			{
				datetime = cTime.Format(_T("%Y-%m-%d %H:%M:%S"));
			}
			else
			{
				datetime = cTime.Format(_T("%Y-%m-%d"));
			}
		}
	}
	return datetime;
}

/**
 *	Converts a given time to a relative display string (relative to current time)
 *	Given time must be in local timezone
 */
CString CAppUtils::ToRelativeTimeString(CTime time)
{
    CString answer;
	// convert to COleDateTime
	SYSTEMTIME sysTime;
	time.GetAsSystemTime( sysTime );
	COleDateTime oleTime( sysTime );
	answer = ToRelativeTimeString(oleTime, COleDateTime::GetCurrentTime());
	return answer;
}

/**
 *	Generates a display string showing the relative time between the two given times as COleDateTimes
 */
CString CAppUtils::ToRelativeTimeString(COleDateTime time,COleDateTime RelativeTo)
{
    CString answer;
	COleDateTimeSpan ts = RelativeTo - time;
    //years
	if(fabs(ts.GetTotalDays()) >= 3*365)
    {
		answer = ExpandRelativeTime( (int)ts.GetTotalDays()/365, IDS_YEAR_AGO, IDS_YEARS_AGO );
	}
	//Months
	if(fabs(ts.GetTotalDays()) >= 60)
	{
		answer = ExpandRelativeTime( (int)ts.GetTotalDays()/30, IDS_MONTH_AGO, IDS_MONTHS_AGO );
		return answer;
	}
	//Weeks
	if(fabs(ts.GetTotalDays()) >= 14)
	{
		answer = ExpandRelativeTime( (int)ts.GetTotalDays()/7, IDS_WEEK_AGO, IDS_WEEKS_AGO );
		return answer;
	}
	//Days
	if(fabs(ts.GetTotalDays()) >= 2)
	{
		answer = ExpandRelativeTime( (int)ts.GetTotalDays(), IDS_DAY_AGO, IDS_DAYS_AGO );
		return answer;
	}
	//hours
	if(fabs(ts.GetTotalHours()) >= 2)
	{
		answer = ExpandRelativeTime( (int)ts.GetTotalHours(), IDS_HOUR_AGO, IDS_HOURS_AGO );
		return answer;
	}
	//minutes
	if(fabs(ts.GetTotalMinutes()) >= 2)
	{
		answer = ExpandRelativeTime( (int)ts.GetTotalMinutes(), IDS_MINUTE_AGO, IDS_MINUTES_AGO );
		return answer;
	}
	//seconds
		answer = ExpandRelativeTime( (int)ts.GetTotalSeconds(), IDS_SECOND_AGO, IDS_SECONDS_AGO );
    return answer;
}

/** 
 * Passed a value and two resource string ids
 * if count is 1 then FormatString is called with format_1 and the value
 * otherwise format_2 is used
 * the formatted string is returned
*/
CString CAppUtils::ExpandRelativeTime( int count, UINT format_1, UINT format_n )
{
	CString answer;
	answer.LoadString(9604);
	answer.LoadString(9605);
	if ( count == 1 )
	{
		answer.FormatMessage( format_1, count );
	}
	else
	{
		answer.FormatMessage( format_n, count );
	}
	return answer;
}

