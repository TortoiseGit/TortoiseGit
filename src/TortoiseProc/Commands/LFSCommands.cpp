// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2019-2022, 2025 - TortoiseGit

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
#include "LFSCommands.h"
#include "GitProgressDlg.h"
#include "LFSLocksDlg.h"
#include "ProgressCommands/LFSSetLockedProgressCommand.h"
#include "MessageBox.h"

static bool setLockedState(bool isLocked, bool force, CTGitPathList& pathList)
{
	if (!GitAdminDir::HasAdminDir(g_Git.m_CurrentDir))
	{
		CMessageBox::Show(GetExplorerHWND(), IDS_NOWORKINGCOPY, IDS_APPNAME, MB_ICONERROR);
		return false;
	}

	CGitProgressDlg progDlg;
	theApp.m_pMainWnd = &progDlg;
	LFSSetLockedProgressCommand lfsCommand(isLocked, force);
	progDlg.SetCommand(&lfsCommand);
	lfsCommand.SetPathList(pathList);
	progDlg.SetItemCount(pathList.GetCount());
	progDlg.DoModal();
	return !progDlg.DidErrorsOccur();
}

bool LFSLockCommand::Execute()
{
	return setLockedState(true, false, pathList);
}

bool LFSUnlockCommand::Execute()
{
	return setLockedState(false, parser.HasKey(L"force"), pathList);
}

bool LFSLocksCommand::Execute()
{
	if (!GitAdminDir::HasAdminDir(g_Git.m_CurrentDir))
	{
		CMessageBox::Show(GetExplorerHWND(), IDS_NOWORKINGCOPY, IDS_APPNAME, MB_ICONERROR);
		return false;
	}

	CLFSLocksDlg dlg;
	dlg.m_pathList = pathList;
	return dlg.DoModal() == IDOK;
}
