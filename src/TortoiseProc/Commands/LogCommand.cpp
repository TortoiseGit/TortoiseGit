// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2007-2008,2011 - TortoiseSVN
// Copyright (C) 2008-2013 - TortoiseGit

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
#include "stdafx.h"
#include "LogCommand.h"
#include "StringUtils.h"
#include "LogDlg.h"

bool LogCommand::Execute()
{
	//the log command line looks like this:
	//command:log path:<path_to_file_or_directory_to_show_the_log_messages> [startrev:<startrevision>] [endrev:<endrevision>]

	CString range;

	CString revstart = parser.GetVal(_T("startrev"));
	if (revstart.IsEmpty())
	{
		// support deprecated parameter prior 1.5.0
		revstart = parser.GetVal(_T("revstart"));
	}
	if (revstart == GIT_REV_ZERO)
		revstart.Empty();
	if (!revstart.IsEmpty())
		range.Format(_T("%s.."), g_Git.FixBranchName(revstart));

	CString revend = parser.GetVal(_T("endrev"));
	if (revend.IsEmpty())
	{
		// support deprecated parameter prior 1.5.0
		revend = parser.GetVal(_T("revend"));
	}
	if (revend == GIT_REV_ZERO)
		revend.Empty();
	if (!revend.IsEmpty())
		range += g_Git.FixBranchName(revend);

	if (parser.HasVal(_T("range")))
		range = parser.GetVal(_T("range"));

	CString val = parser.GetVal(_T("limit"));
	int limit = _tstoi(val);
	CString rev = parser.GetVal(_T("rev"));

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
	dlg.SetParams(orgCmdLinePath, cmdLinePath, rev, range, limit);
	dlg.SetFilter(findStr, findType, findRegex);
	dlg.DoModal();
	if (parser.HasVal(_T("outfile")))
	{
		CString sText = dlg.GetSelectedHash();
		CStringUtils::WriteStringToTextFile(parser.GetVal(L"outfile"), (LPCTSTR)sText, true);
	}
	return true;
}
