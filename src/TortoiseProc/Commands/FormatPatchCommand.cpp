// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009, 2015-2016, 2018-2019 - TortoiseGit
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
#include "FormatPatchCommand.h"
#include "MessageBox.h"
#include "FormatPatchDlg.h"
#include "Git.h"
#include "ShellUpdater.h"
#include "ProgressDlg.h"
#include "AppUtils.h"

bool FormatPatchCommand::Execute()
{
	if (!GitAdminDir::IsWorkingTreeOrBareRepo(g_Git.m_CurrentDir))
	{
		CMessageBox::Show(GetExplorerHWND(), IDS_NOGITREPO, IDS_APPNAME, MB_ICONERROR);
		return false;
	}

	CFormatPatchDlg dlg;
	CString startval = parser.GetVal(L"startrev");
	CString endval = parser.GetVal(L"endrev");

	if( endval.IsEmpty() && (!startval.IsEmpty()))
	{
		dlg.m_Since=startval;
		dlg.m_From = startval + L"~1";
		dlg.m_To = startval;
		dlg.m_Radio = IDC_RADIO_SINCE;
	}
	else if( (!endval.IsEmpty()) && (!startval.IsEmpty()))
	{
		dlg.m_From=startval;
		dlg.m_To=endval;
		dlg.m_Radio = IDC_RADIO_RANGE;
	}

	if(dlg.DoModal()==IDOK)
	{
		CString cmd;
		CString range;

		switch(dlg.m_Radio)
		{
		case IDC_RADIO_SINCE:
			range=g_Git.FixBranchName(dlg.m_Since);
			break;
		case IDC_RADIO_NUM:
			range.Format(L"-%d", dlg.m_Num);
			break;
		case IDC_RADIO_RANGE:
			range.Format(L"%s..%s", static_cast<LPCTSTR>(dlg.m_From), static_cast<LPCTSTR>(dlg.m_To));
			break;
		}
		dlg.m_Dir.Replace(L'\\', L'/');
		cmd.Format(L"git.exe format-patch%s -o \"%s\" %s",
			dlg.m_bNoPrefix ? L" --no-prefix" : L"",
			static_cast<LPCTSTR>(dlg.m_Dir),
			static_cast<LPCTSTR>(range)
			);

		CProgressDlg progress;
		progress.m_GitCmd=cmd;
		progress.DoModal();

		CShellUpdater::Instance().AddPathForUpdate(CTGitPath(dlg.m_Dir));
		CShellUpdater::Instance().Flush();

		if(!progress.m_GitStatus)
		{
			if(dlg.m_bSendMail)
				CAppUtils::SendPatchMail(GetExplorerHWND(), cmd, progress.m_LogText);
		}
		return !progress.m_GitStatus;
	}
	return FALSE;
}
