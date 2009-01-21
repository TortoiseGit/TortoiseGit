// TortoiseSVN - a Windows shell extension for easy version control

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
#include "StdAfx.h"
#include "MergeCommand.h"
#include "git.h"
#include "MergeDlg.h"
#include "MessageBox.h"

bool MergeCommand::Execute()
{
	CMergeDlg dlg;
	if(dlg.DoModal()==IDOK)
	{
		CString cmd;
		CString noff;
		CString squash;

		if(dlg.m_bNoFF)
			noff=_T("--no-ff");

		if(dlg.m_bSquash)
			squash=_T("--squash");

		cmd.Format(_T("git.exe merge %s %s %s"),
			noff,
			squash,
			dlg.m_VersionName);

		CString output;
		g_Git.Run(cmd,&output,CP_OEMCP);

		CMessageBox::Show(NULL,output,_T("TortoiseGit"),MB_OK);
	}
	return false;
}