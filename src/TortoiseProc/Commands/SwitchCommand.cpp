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
#include "SwitchCommand.h"

#include "GitSwitchDlg.h"
#include "ProgressDlg.h"
#include "MessageBox.h"

bool SwitchCommand::Execute()
{
	CGitSwitchDlg dlg;
	
	if (dlg.DoModal() == IDOK)
	{
		CString cmd;
		CString track;
		CString base;
		CString force;
		CString branch;

		if(dlg.m_bBranch)
			branch.Format(_T("-b %s"),dlg.m_NewBranch);
		if(dlg.m_bForce)
			force=_T("-f");
		if(dlg.m_bTrack)
			track=_T("--track");

		cmd.Format(_T("git.exe checkout %s %s %s %s"),
			 force,
			 track,
			 branch,
			 dlg.m_Base);

		CProgressDlg progress;
		progress.m_GitCmd=cmd;
		if(progress.DoModal()==IDOK)
			return TRUE;

	}
	return false;
}
