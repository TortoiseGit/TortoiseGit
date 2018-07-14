// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2012, 2015, 2018 - TortoiseGit
// Copyright (C) 2007 - TortoiseSVN

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
#include "RepoStatusCommand.h"
#include "MessageBox.h"
#include "ChangedDlg.h"
#include "GitAdminDir.h"

bool RepoStatusCommand::Execute()
{
	if (!GitAdminDir::HasAdminDir(g_Git.m_CurrentDir))
	{
		CMessageBox::Show(GetExplorerHWND(), IDS_NOWORKINGCOPY, IDS_APPNAME, MB_ICONERROR);
		return false;
	}

	CChangedDlg dlg;
	theApp.m_pMainWnd = &dlg;
	dlg.m_pathList = pathList;
	dlg.DoModal();
	return true;
}