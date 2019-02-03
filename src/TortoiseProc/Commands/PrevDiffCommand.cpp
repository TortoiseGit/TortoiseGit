// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2011, 2016, 2018-2019 - TortoiseGit
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
#include "PrevDiffCommand.h"
#include "GitDiff.h"
#include "MessageBox.h"
#include "ChangedDlg.h"
#include "LogDlgHelper.h"
#include "FileDiffDlg.h"

bool PrevDiffCommand::Execute()
{
	if (!GitAdminDir::HasAdminDir(g_Git.m_CurrentDir))
	{
		CMessageBox::Show(GetExplorerHWND(), IDS_NOWORKINGCOPY, IDS_APPNAME, MB_ICONERROR);
		return false;
	}

	bool bAlternativeTool = !!parser.HasKey(L"alternative");
	bool bUnified = !!parser.HasKey(L"unified");
	if (this->orgCmdLinePath.IsDirectory())
	{
		CFileDiffDlg dlg;
		theApp.m_pMainWnd = &dlg;
		dlg.m_strRev1 = L"HEAD~1";
		dlg.m_strRev2 = GIT_REV_ZERO;
		dlg.m_sFilter = this->cmdLinePath.GetGitPathString();

		dlg.DoModal();
		return true;
	}

	CLogDataVector revs;
	CLogCache cache;
	revs.m_pLogCache = &cache;
	revs.ParserFromLog(&cmdLinePath, 2, CGit::LOG_INFO_ONLY_HASH);

	if (revs.size() != 2)
	{
		CMessageBox::Show(GetExplorerHWND(), IDS_ERR_NOPREVREVISION, IDS_APPNAME, MB_ICONERROR);
		return false;
	}

	return !!CGitDiff::Diff(GetExplorerHWND(), &cmdLinePath, &cmdLinePath, GIT_REV_ZERO, revs.GetGitRevAt(1).m_CommitHash.ToString(), false, bUnified, 0, bAlternativeTool);
}
