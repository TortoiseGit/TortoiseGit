// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2022, 2026 - TortoiseGit

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
#include "WorktreeCommand.h"
#include "WorktreeListDlg.h"
#include "AppUtils.h"

bool WorktreeCreateCommand::Execute()
{
	return CAppUtils::CreateWorktree(GetExplorerHWND());
}

bool WorktreeListCommand::Execute()
{
	CWorktreeListDlg dlg;
	theApp.m_pMainWnd = &dlg;
	return dlg.DoModal() == IDOK;
}

bool DropWorktreeCreateCommand::Execute()
{
	CString target = parser.GetVal(L"droptarget");
	CPathUtils::EnsureTrailingPathDelimiter(target);

	CString name = CPathUtils::GetFileNameFromPath(g_Git.m_CurrentDir);
	if (CStringUtils::EndsWith(name, L".git"))
		name = name.Left(name.GetLength() - static_cast<int>(wcslen(L".git")));
	if (!CPathUtils::IsSamePath(target + name, g_Git.m_CurrentDir))
		target += name;

	return CAppUtils::CreateWorktree(GetExplorerHWND(), target);
}
