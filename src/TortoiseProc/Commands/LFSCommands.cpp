// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2019-2020 - TortoiseGit

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
#include "ProgressDlg.h"
#include "LFSLocksDlg.h"
#include "ProgressCommands/LFSSetLockedProgressCommand.h"

static bool setLockedState(bool isLocked, bool force, CTGitPathList& pathList)
{
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
	return setLockedState(false, parser.HasVal(L"force"), pathList);
}

bool LFSLocksCommand::Execute()
{
	CLFSLocksDlg dlg;
	dlg.m_pathList = pathList;
	return dlg.DoModal() == IDOK;
}
