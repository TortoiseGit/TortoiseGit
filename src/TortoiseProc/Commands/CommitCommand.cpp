// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2018 - TortoiseGit
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
#include "stdafx.h"
#include "CommitCommand.h"
#include "CommitDlg.h"
#include "StringUtils.h"
#include "MessageBox.h"
#include "AppUtils.h"

CString CommitCommand::LoadLogMessage()
{
	CString msg;
	if (parser.HasKey(L"logmsg"))
		msg = parser.GetVal(L"logmsg");
	if (parser.HasKey(L"logmsgfile"))
	{
		CString logmsgfile = parser.GetVal(L"logmsgfile");
		CStringUtils::ReadStringFromTextFile(logmsgfile, msg);
	}
	return msg;
}

bool CommitCommand::Execute()
{
	if (parser.HasKey(L"logmsg") && parser.HasKey(L"logmsgfile"))
	{
		CMessageBox::Show(GetExplorerHWND(), IDS_ERR_TWOLOGPARAMS, IDS_APPNAME, MB_ICONERROR);
		return false;
	}
	CString sLogMsg = LoadLogMessage();
	bool bSelectFilesForCommit = !!DWORD(CRegStdDWORD(L"Software\\TortoiseGit\\SelectFilesForCommit", TRUE));

	if (!GitAdminDir::HasAdminDir(g_Git.m_CurrentDir))
	{
		CMessageBox::Show(GetExplorerHWND(), IDS_NOWORKINGCOPY, IDS_APPNAME, MB_ICONERROR);
		return false;
	}

	return !!CAppUtils::Commit(GetExplorerHWND(),
								parser.GetVal(L"bugid"),
								parser.HasKey(L"wholeproject"),
								sLogMsg,
								pathList,
								bSelectFilesForCommit);
}
