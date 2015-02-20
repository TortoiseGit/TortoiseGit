// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2015 - TortoiseGit

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

	CGitHash upstreamOldHash;
	if (g_Git.GetHash(upstreamOldHash, out))
	{
		MessageBox(hwndExplorer, g_Git.GetGitLastErr(_T("Could not get upstream hash.")), _T("TortoiseGit"), MB_ICONERROR);
		return false;
	}

	CProgressDlg progress;
	progress.m_GitCmd=_T("git.exe svn fetch");

	CGitHash upstreamNewHash; // declare outside lambda, because it is captured by reference
	progress.m_PostCmdCallback = [&](DWORD status, PostCmdList& postCmdList)
	{
		if (status)
			return;

		if (g_Git.GetHash(upstreamNewHash, out))
		{
			MessageBox(hwndExplorer, g_Git.GetGitLastErr(_T("Could not get upstream hash after fetching.")), _T("TortoiseGit"), MB_ICONERROR);
			return;
		}
		if (upstreamOldHash == upstreamNewHash)
			return;

		postCmdList.emplace_back(IDI_DIFF, _T("Fetched Diff"), [&]
		{
			CLogDlg dlg;
			dlg.SetParams(CTGitPath(_T("")), CTGitPath(_T("")), _T(""), upstreamOldHash.ToString() + _T("..") + upstreamNewHash.ToString(), 0);
			dlg.DoModal();
		});

		postCmdList.emplace_back(IDI_LOG, _T("Fetched Log"), [&]
		{
			CFileDiffDlg dlg;
			dlg.SetDiff(NULL, upstreamNewHash.ToString(), upstreamOldHash.ToString());
			dlg.DoModal();
		});
	};

	return progress.DoModal() == IDOK;
}
