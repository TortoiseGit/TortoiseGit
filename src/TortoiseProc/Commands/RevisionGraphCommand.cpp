// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2007, 2009-2011 - TortoiseSVN
// Copyright (C) 2012, 2018-2019 - TortoiseGit

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
#include "RevisionGraphCommand.h"
#include "MessageBox.h"
#include "RevisionGraph/RevisionGraphDlg.h"

bool RevisionGraphCommand::Execute()
{
	if (!GitAdminDir::IsWorkingTreeOrBareRepo(g_Git.m_CurrentDir))
	{
		CMessageBox::Show(GetExplorerHWND(), IDS_NOGITREPO, IDS_APPNAME, MB_ICONERROR);
		return false;
	}

	CRevisionGraphDlg dlg;
	theApp.m_pMainWnd = &dlg;
	dlg.SetPath(g_Git.m_CurrentDir);
	if (parser.HasVal(L"output"))
	{
		dlg.SetOutputFile(parser.GetVal(L"output"));
		dlg.StartHidden();
	}
	dlg.DoModal();

	return true;
}
