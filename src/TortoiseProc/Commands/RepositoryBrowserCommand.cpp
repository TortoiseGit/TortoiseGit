// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2012 - TortoiseGit

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
#include "StdAfx.h"
#include "RepositoryBrowserCommand.h"
#include "MessageBox.h"
#include "RepositoryBrowser.h"

bool RepositoryBrowserCommand::Execute()
{
	if (!GitAdminDir().HasAdminDir(g_Git.m_CurrentDir) && !GitAdminDir().IsBareRepo(g_Git.m_CurrentDir)) {
		CMessageBox::Show(hwndExplorer, IDS_NOWORKINGCOPY, IDS_APPNAME, MB_ICONERROR);
		return false;
	}

	CString rev = _T("HEAD");
	CString val = parser.GetVal(_T("rev"));
	if (!val.IsEmpty())
		rev = val;
	CRepositoryBrowser(rev).DoModal();
	return true;
}
