// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2013, 2015-2019 - TortoiseGit
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
#include "ImportPatchCommand.h"
#include "ImportPatchDlg.h"
#include "TGitPath.h"
#include "Git.h"
#include "AppUtils.h"

bool ImportPatchCommand::Execute()
{
	CImportPatchDlg dlg;
	theApp.m_pMainWnd = &dlg;

	CString cmd;
	CString output;

	CString droppath = parser.GetVal(L"droptarget");
	if (!droppath.IsEmpty())
	{
		if (CTGitPath(droppath).IsAdminDir())
			return FALSE;

		if (!CTGitPath(droppath).HasAdminDir(&g_Git.m_CurrentDir))
		{
			CString err;
			err.Format(IDS_ERR_NOT_REPOSITORY, static_cast<LPCTSTR>(g_Git.m_CurrentDir));
			MessageBox(GetExplorerHWND(), err, L"TortoiseGit", MB_OK | MB_ICONERROR);
			return FALSE;
		}
	}
	else if (!orgPathList.IsEmpty() && !orgPathList[0].HasAdminDir())
	{
		CString str = CAppUtils::ChooseRepository(GetExplorerHWND(), nullptr);
		if(str.IsEmpty())
			return FALSE;

		CTGitPath path;
		path.SetFromWin(str);

		if(!path.HasAdminDir())
		{
			CString err;
			err.Format(IDS_ERR_NOT_REPOSITORY, static_cast<LPCTSTR>(str));
			MessageBox(GetExplorerHWND(), err, L"TortoiseGit", MB_OK | MB_ICONERROR);
			return FALSE;
		}
		g_Git.m_CurrentDir=str;
	}

	for(int i = 0 ; i < this->orgPathList.GetCount(); ++i)
	{
		if(!orgPathList[i].IsDirectory())
			dlg.m_PathList.AddPath(orgPathList[i]);
	}

	if(dlg.DoModal()==IDOK)
		return TRUE;

	return FALSE;
}
