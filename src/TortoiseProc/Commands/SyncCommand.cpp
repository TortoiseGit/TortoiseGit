// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2009 - TortoiseSVN
// Copyright (C) 2008-2012, 2015 - TortoiseGit

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
#include "TortoiseProc.h"
#include "messagebox.h"
#include "DirFileEnum.h"
#include "SyncDlg.h"
//#include "SVNConfig.h"
#include "registry.h"
#include "SyncCommand.h"

bool SyncCommand::Execute()
{
	if (!GitAdminDir::HasAdminDir(g_Git.m_CurrentDir) && !GitAdminDir::IsBareRepo(g_Git.m_CurrentDir)) {
		CMessageBox::Show(hwndExplorer, IDS_NOWORKINGCOPY, IDS_APPNAME, MB_ICONERROR);
		return false;
	}

	bool bRet = false;
	CSyncDlg dlg;
	dlg.m_seq = parser.GetLongVal(_T("seq"));
	if(dlg.DoModal() == IDOK)
		bRet=true;

	return bRet;
}

