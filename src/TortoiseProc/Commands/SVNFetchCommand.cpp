// TortoiseGit - a Windows shell extension for easy version control

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
#include "SVNFetchCommand.h"

#include "SysProgressDlg.h"
#include "ProgressDlg.h"
#include "MessageBox.h"
#include "Git.h"
#include "LogDlg.h"
#include "FileDiffDlg.h"

bool SVNFetchCommand::Execute()
{
	bool autoClose = false;
	if (parser.HasVal(_T("closeonend")))
		autoClose = !!parser.GetLongVal(_T("closeonend"));

	CString cmd, out, err;
	cmd = _T("git.exe config svn-remote.svn.fetch");

	if (!g_Git.Run(cmd, &out, &err, CP_UTF8))
	{
		int start = out.Find(_T(':'));
		if( start >=0 )
			out=out.Mid(start);

		if(out.Left(5) == _T(":refs"))
			out=out.Mid(6);

		start = 0;
		out=out.Tokenize(_T("\n"),start);
	}
	else
	{
		CMessageBox::Show(NULL, _T("Found no SVN remote."), _T("TortoiseGit"), MB_OK|MB_ICONERROR);
		return false;
	}

	CGitHash upstreamOldHash, upstreamNewHash;
	if (g_Git.GetHash(upstreamOldHash, out))
	{
		MessageBox(hwndExplorer, g_Git.GetGitLastErr(_T("Could not get upstream hash.")), _T("TortoiseGit"), MB_ICONERROR);
		return false;
	}

	CProgressDlg progress;
	progress.m_GitCmd=_T("git.exe svn fetch");
	progress.m_PostCmdList.Add(_T("Fetched Diff"));
	progress.m_PostCmdList.Add(_T("Fetched Log"));
	progress.m_bAutoCloseOnSuccess = autoClose;

	INT_PTR userResponse = progress.DoModal();
	if (g_Git.GetHash(upstreamNewHash, out))
	{
		MessageBox(hwndExplorer, g_Git.GetGitLastErr(_T("Could not get upstream hash after fetching.")), _T("TortoiseGit"), MB_ICONERROR);
		return false;
	}
	if (userResponse == IDC_PROGRESS_BUTTON1)
	{
		if (upstreamOldHash == upstreamNewHash)
		{
			if (progress.m_GitStatus == 0)
				CMessageBox::Show(NULL, L"No new revisions fetched.", L"TortoiseGit Fetch", MB_OK | MB_ICONINFORMATION);
			return true;
		}

		CLogDlg dlg;
		dlg.SetParams(CTGitPath(_T("")), CTGitPath(_T("")), _T(""), upstreamOldHash.ToString() + _T("..") + upstreamNewHash.ToString(), 0);
		dlg.DoModal();
		return true;
	}
	else if (userResponse == IDC_PROGRESS_BUTTON1 + 1)
	{
		if (upstreamOldHash == upstreamNewHash)
		{
			if (progress.m_GitStatus == 0)
				CMessageBox::Show(NULL, L"No new revisions fetched.", L"TortoiseGit Fetch", MB_OK | MB_ICONINFORMATION);
			return true;
		}

		CFileDiffDlg dlg;
		dlg.SetDiff(NULL, upstreamNewHash.ToString(), upstreamOldHash.ToString());
		dlg.DoModal();
		return true;
	}
	else
		return false;
}
