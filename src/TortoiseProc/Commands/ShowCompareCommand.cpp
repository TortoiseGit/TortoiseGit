// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2012-2013, 2016, 2018-2019 - TortoiseGit

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
#include "ShowCompareCommand.h"
#include "MessageBox.h"
#include "AppUtils.h"
#include "GitDiff.h"

bool ShowCompareCommand::Execute()
{
	if (!GitAdminDir::IsWorkingTreeOrBareRepo(g_Git.m_CurrentDir))
	{
		CMessageBox::Show(GetExplorerHWND(), IDS_NOGITREPO, IDS_APPNAME, MB_ICONERROR);
		return false;
	}

	CString		rev1;
	CString		rev2;

	bool		unified = !!parser.HasKey(L"unified");

	if (parser.HasVal(L"revision1"))
		rev1 = parser.GetVal(L"revision1");
	if (parser.HasVal(L"revision2"))
		rev2 = parser.GetVal(L"revision2");

	bool bAlternativeTool = !!parser.HasKey(L"alternative");

	if (unified)
		return !!CAppUtils::StartShowUnifiedDiff(nullptr, cmdLinePath, rev1, CTGitPath(), rev2, bAlternativeTool);
	else
		return !!CGitDiff::DiffCommit(GetExplorerHWND(), cmdLinePath, rev2, rev1, bAlternativeTool);
}
