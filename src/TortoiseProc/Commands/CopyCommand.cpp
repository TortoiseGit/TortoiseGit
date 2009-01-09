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
#include "CopyCommand.h"

#include "CopyDlg.h"
#include "SVNProgressDlg.h"
#include "StringUtils.h"

bool CopyCommand::Execute()
{
	bool bRet = false;
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

	BOOL repeat = FALSE;
	CCopyDlg dlg;
	dlg.m_sLogMessage = msg;

	dlg.m_path = cmdLinePath;
	CString url = parser.GetVal(_T("url"));
	CString logmessage;
	SVNRev copyRev = SVNRev::REV_HEAD;
	BOOL doSwitch = FALSE;
	do 
	{
		repeat = FALSE;
		dlg.m_URL = url;
		dlg.m_sLogMessage = logmessage;
		dlg.m_CopyRev = copyRev;
		dlg.m_bDoSwitch = doSwitch;
		if (dlg.DoModal() == IDOK)
		{
			theApp.m_pMainWnd = NULL;
			TRACE(_T("copy %s to %s\n"), (LPCTSTR)cmdLinePath.GetWinPathString(), (LPCTSTR)dlg.m_URL);
			CSVNProgressDlg progDlg;
			progDlg.SetCommand(CSVNProgressDlg::SVNProgress_Copy);
			if (parser.HasVal(_T("closeonend")))
				progDlg.SetAutoClose(parser.GetLongVal(_T("closeonend")));
			progDlg.SetOptions(dlg.m_bDoSwitch ? ProgOptSwitchAfterCopy : ProgOptNone);
			progDlg.SetPathList(pathList);
			progDlg.SetUrl(dlg.m_URL);
			progDlg.SetCommitMessage(dlg.m_sLogMessage);
			progDlg.SetRevision(dlg.m_CopyRev);
			url = dlg.m_URL;
			logmessage = dlg.m_sLogMessage;
			copyRev = dlg.m_CopyRev;
			doSwitch = dlg.m_bDoSwitch;
			progDlg.DoModal();
			CRegDWORD err = CRegDWORD(_T("Software\\TortoiseGit\\ErrorOccurred"), FALSE);
			err = (DWORD)progDlg.DidErrorsOccur();
			bRet = !progDlg.DidErrorsOccur();
			repeat = progDlg.DidErrorsOccur();
			CRegDWORD bFailRepeat = CRegDWORD(_T("Software\\TortoiseGit\\CommitReopen"), FALSE);
			if (DWORD(bFailRepeat) == FALSE)
				repeat = false;		// do not repeat if the user chose not to in the settings.
		}
	} while(repeat);
	return bRet;
}
