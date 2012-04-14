// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2007-2008,2011 - TortoiseSVN
// Copyright (C) 2008-2012 - TortoiseGit

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
#include "LogCommand.h"
#include "StringUtils.h"
#include "LogDlg.h"

bool LogCommand::Execute()
{
	//the log command line looks like this:
	//command:log path:<path_to_file_or_directory_to_show_the_log_messages> [startrev:<startrevision>] [endrev:<endrevision>]

	CString val = parser.GetVal(_T("startrev"));
	if ( val.IsEmpty() )
	{
		// support deprecated parameter prior 1.5.0
		val = parser.GetVal(_T("revstart"));
	}
	CString revstart =val;
	val = parser.GetVal(_T("endrev"));
	if ( val.IsEmpty() )
	{
		// support deprecated parameter prior 1.5.0
		val = parser.GetVal(_T("revend"));
	}
	CString revend =val;
	val = parser.GetVal(_T("limit"));
	int limit = _tstoi(val);
	val = parser.GetVal(_T("rev"));
	if ( val.IsEmpty() )
	{
		// support deprecated parameter prior 1.5.0
		val = parser.GetVal(_T("rev"));
	}

	CString rev = val;

#if 0
	SVNRev pegrev = val.IsEmpty() ? SVNRev() : SVNRev(val);
	if (!revstart.IsValid())
		revstart = SVNRev::REV_HEAD;
	if (!revend.IsValid())
		revend = 0;
#endif

	if (limit == 0)
	{
		CRegDWORD reg = CRegDWORD(_T("Software\\TortoiseGit\\NumberOfLogs"), 100);
		limit = (int)(LONG)reg;
	}

	CString findStr = parser.GetVal(_T("findstring"));
	LONG findType = parser.GetLongVal(_T("findtype"));
	bool findRegex = !!CRegDWORD(_T("Software\\TortoiseGit\\UseRegexFilter"), FALSE);
	if (parser.HasKey(_T("findtext")))
		findRegex = false;
	if (parser.HasKey(_T("findregex")))
		findRegex = true;

	CLogDlg dlg;
	theApp.m_pMainWnd = &dlg;
	//dlg.SetParams(cmdLinePath);
	dlg.SetParams(orgCmdLinePath, cmdLinePath, rev, revstart, revend, limit);
	dlg.SetFilter(findStr, findType, findRegex);
//	dlg.SetIncludeMerge(!!parser.HasKey(_T("merge")));
//	val = parser.GetVal(_T("propspath"));
//	if (!val.IsEmpty())
//		dlg.SetProjectPropertiesPath(CTSVNPath(val));
	dlg.DoModal();
	if (parser.HasVal(_T("outfile")))
	{
		CString sText = dlg.GetSelectedHash();
		CStringUtils::WriteStringToTextFile(parser.GetVal(L"outfile"), (LPCTSTR)sText, true);
	}
	return true;
}
