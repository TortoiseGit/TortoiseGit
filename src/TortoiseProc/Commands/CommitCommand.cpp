// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2011 - TortoiseGit
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
#include "CommitCommand.h"
#include "CommitDlg.h"
//#include "SVNProgressDlg.h"
#include "StringUtils.h"
#include "Hooks.h"
#include "MessageBox.h"
#include "AppUtils.h"

CString CommitCommand::LoadLogMessage()
{
	CString msg;
	if (parser.HasKey(_T("logmsg")))
	{
		msg = parser.GetVal(_T("logmsg"));
	}
	if (parser.HasKey(_T("logmsgfile")))
	{
		CString logmsgfile = parser.GetVal(_T("logmsgfile"));
		CStringUtils::ReadStringFromTextFile(logmsgfile, msg);
	}
	return msg;
}

bool CommitCommand::Execute()
{
	CTGitPathList selectedList;
	if (parser.HasKey(_T("logmsg")) && (parser.HasKey(_T("logmsgfile"))))
	{
		CMessageBox::Show(hwndExplorer, IDS_ERR_TWOLOGPARAMS, IDS_APPNAME, MB_ICONERROR);
		return false;
	}
	CString sLogMsg = LoadLogMessage();
	bool bSelectFilesForCommit = !!DWORD(CRegStdDWORD(_T("Software\\TortoiseGit\\SelectFilesForCommit"), TRUE));
#if 0
	DWORD exitcode = 0;
	CString error;
	if (CHooks::Instance().StartCommit(pathList, sLogMsg, exitcode, error))
	{
		if (exitcode)
		{
			CString temp;
			temp.Format(IDS_ERR_HOOKFAILED, (LPCTSTR)error);
			CMessageBox::Show(hwndExplorer, temp, _T("TortoiseGit"), MB_ICONERROR);
			return false;
		}
	}
#endif

	if (!GitAdminDir().HasAdminDir(g_Git.m_CurrentDir)) {
		CMessageBox::Show(hwndExplorer, _T("No working directory found."), _T("TortoiseGit"), MB_ICONERROR);
		return false;
	}

	bool autoClose = false;
	if (parser.HasVal(_T("closeonend")))
		autoClose = !!parser.GetLongVal(_T("closeonend"));

	return !!CAppUtils::Commit(	parser.GetVal(_T("bugid")),
								parser.HasKey(_T("wholeproject")),
								sLogMsg,
								pathList,
								selectedList,
								bSelectFilesForCommit,
								autoClose);
}
