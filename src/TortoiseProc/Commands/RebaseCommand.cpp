// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2012, 2015-2016, 2018-2019 - TortoiseGit
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
#include "RebaseCommand.h"
#include "MessageBox.h"
#include "RebaseDlg.h"
#include "Git.h"
#include "AppUtils.h"

bool RebaseCommand::Execute()
{
	if (!GitAdminDir::HasAdminDir(g_Git.m_CurrentDir))
	{
		CMessageBox::Show(GetExplorerHWND(), IDS_NOWORKINGCOPY, IDS_APPNAME, MB_ICONERROR);
		return false;
	}
	while(1)
	{
		CRebaseDlg dlg;
		dlg.m_PostButtonTexts.Add(CString(MAKEINTRESOURCE(IDS_MENULOG)));
		dlg.m_PostButtonTexts.Add(CString(MAKEINTRESOURCE(IDS_PROC_RESTARTREBASE)));
		INT_PTR ret = dlg.DoModal();
		if (ret == IDOK)
			return true;
		if (ret == IDCANCEL)
			return false;
		if (ret == IDC_REBASE_POST_BUTTON)
		{
			CString cmd = L"/command:log";
			cmd += L" /path:\"" + g_Git.m_CurrentDir + L'"';
			CAppUtils::RunTortoiseGitProc(cmd);
			return true;
		}
	}
}
