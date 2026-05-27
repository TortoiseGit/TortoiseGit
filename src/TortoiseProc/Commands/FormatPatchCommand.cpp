// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009, 2015-2016, 2018-2019, 2023, 2026 - TortoiseGit
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
#include "FormatPatchDlg.h"
#include "Git.h"
#include "ShellUpdater.h"
#include "ProgressDlg.h"
#include "AppUtils.h"

bool FormatPatchCommand::Execute()
{
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

		try
		{
			switch (dlg.m_Radio)
			{
			case IDC_RADIO_SINCE:
				range = L"--end-of-options " + CGit::QuoteParameter(g_Git.FixBranchName(dlg.m_Since));
				break;
			case IDC_RADIO_NUM:
				range.Format(L"-%d", dlg.m_Num);
				break;
			case IDC_RADIO_RANGE:
				range.Format(L"--end-of-options %s", static_cast<LPCWSTR>(CGit::QuoteParameter(dlg.m_From + L".." + dlg.m_To)));
				break;
			}
			cmd.Format(L"git.exe format-patch%s -o %s %s --",
					   dlg.m_bNoPrefix ? L" --no-prefix" : L"",
					   static_cast<LPCWSTR>(CGit::QuoteParameter(dlg.m_Dir)),
					   static_cast<LPCWSTR>(range));
		}
		catch (illegal_git_parameter& e)
		{
			MessageBox(GetExplorerHWND(), e.cause(), L"TortoiseGit", MB_OK | MB_ICONERROR);
			return false;
		}

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
