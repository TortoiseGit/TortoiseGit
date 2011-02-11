// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2011 - Sven Strickroth <email@cs-ware.de>

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
#include "RequestPullCommand.h"
#include "AppUtils.h"
#include "RequestPullDlg.h"
#include "ProgressDlg.h"

bool RequestPullCommand::Execute()
{
	CRequestPullDlg dlg;
	if (dlg.DoModal()==IDOK)
	{
		CString cmd;
		cmd.Format(_T("git.exe request-pull %s \"%s\" %s"), dlg.m_StartRevision, dlg.m_RepositoryURL, dlg.m_EndRevision);

		CProgressDlg progress;
		progress.m_GitCmd=cmd;
		progress.DoModal();
	}
	return true;
}
