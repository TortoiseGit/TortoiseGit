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
#include "resource.h"
//#include "TortoiseProc.h"
//#include "PathUtils.h"
#include "TortoiseGitBlameAppUtils.h"
//#include "GitProperties.h"
//#include "StringUtils.h"
//#include "MessageBox.h"
#include "Registry.h"
//#include "TGitPath.h"
//include "Git.h"
//#include "RepositoryBrowser.h"
//#include "BrowseFolder.h"
//#include "UnicodeUtils.h"
//#include "ExportDlg.h"
//#include "ProgressDlg.h"
//#include "GitAdminDir.h"
//#include "ProgressDlg.h"
//#include "BrowseFolder.h"
//#include "DirFileEnum.h"
//#include "MessageBox.h"
//#include "GitStatus.h"
//#include "CreateBranchTagDlg.h"
//#include "GitSwitchDlg.h"
//#include "ResetDlg.h"
//#include "commctrl.h"

CAppUtils::CAppUtils(void)
{
}

CAppUtils::~CAppUtils(void)
{
}

/**
 * FUNCTION    :   FormatDateAndTime
 * DESCRIPTION :   Generates a displayable string from a CTime object in
 *                 system short or long format dependant on setting of option
 *				   as DATE_SHORTDATE or DATE_LONGDATE
 *				   If HKCU\Software\TortoiseGit\UseSystemLocaleForDates is 0 then use fixed format
 *				   rather than locale
 * RETURN      :   CString containing date/time
 */
CString CAppUtils::FormatDateAndTime( const CTime& cTime, DWORD option, bool bIncludeTime /*=true*/  )
{
	CString datetime;
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
	return datetime;
}
