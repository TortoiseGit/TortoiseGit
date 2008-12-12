// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008 - TortoiseSVN

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

#include "LogDlg.h"

bool LogCommand::Execute()
{
	//the log command line looks like this:
	//command:log path:<path_to_file_or_directory_to_show_the_log_messages> [startrev:<startrevision>] [endrev:<endrevision>]
#if 0 
	CString val = parser.GetVal(_T("startrev"));
	if ( val.IsEmpty() )
	{
		// support deprecated parameter prior 1.5.0
		val = parser.GetVal(_T("revstart"));
	}
	SVNRev revstart = val.IsEmpty() ? SVNRev() : SVNRev(val);
	val = parser.GetVal(_T("endrev"));
	if ( val.IsEmpty() )
	{
		// support deprecated parameter prior 1.5.0
		val = parser.GetVal(_T("revend"));
	}
	SVNRev revend = val.IsEmpty() ? SVNRev() : SVNRev(val);
	val = parser.GetVal(_T("limit"));
	int limit = _tstoi(val);
	val = parser.GetVal(_T("pegrev"));
	if ( val.IsEmpty() )
	{
		// support deprecated parameter prior 1.5.0
		val = parser.GetVal(_T("revpeg"));
	}
	SVNRev pegrev = val.IsEmpty() ? SVNRev() : SVNRev(val);
	if (!revstart.IsValid())
		revstart = SVNRev::REV_HEAD;
	if (!revend.IsValid())
		revend = 0;

	if (limit == 0)
	{
		CRegDWORD reg = CRegDWORD(_T("Software\\TortoiseSVN\\NumberOfLogs"), 100);
		limit = (int)(LONG)reg;
	}
	BOOL bStrict = (DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\LastLogStrict"), FALSE);
	if (parser.HasKey(_T("strict")))
	{
		bStrict = TRUE;
	}
#endif	
	
	if(pathList.GetCount()>0)
		g_Git.m_CurrentDir=pathList[0].GetWinPathString();
	
	CLogDlg dlg;
	theApp.m_pMainWnd = &dlg;
//	dlg.SetParams(cmdLinePath, pegrev, revstart, revend, limit, bStrict);
//	dlg.SetIncludeMerge(!!parser.HasKey(_T("merge")));
//	val = parser.GetVal(_T("propspath"));
//	if (!val.IsEmpty())
//		dlg.SetProjectPropertiesPath(CTSVNPath(val));
	dlg.DoModal();			
	return true;
}
