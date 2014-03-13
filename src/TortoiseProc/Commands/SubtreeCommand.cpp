// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2009,2012-2013 - TortoiseGit

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
#include "SubtreeCommand.h"
#include "SubtreeCmdDlg.h"
#include "AppUtils.h"
#include "MessageBox.h"
#include "ProgressDlg.h"



bool SubtreeCommand::ExecuteSubtree( CSubtreeCmdDlg &dlg )
{
	CString subCommand;
	switch(dlg.m_eSubCommand)
	{
	case CSubtreeCmdDlg::SubCommand_Add:
		subCommand = _T("add");
		break;
	case CSubtreeCmdDlg::SubCommand_Push:
		subCommand = _T("push");
		break;
	case CSubtreeCmdDlg::SubCommand_Pull:
		subCommand = _T("pull");
		break;
	}

	if( dlg.DoModal() == IDOK )
	{
		if (dlg.m_bAutoloadPuttyKeyFile)
			CAppUtils::LaunchPAgent(&dlg.m_strPuttyKeyFile);

		CString args;
		dlg.m_strPath.Replace(_T('\\'),_T('/'));

		// remove the target if it's empty so we can re-create it.
		CTGitPath path = g_Git.m_CurrentDir + _T("\\") + dlg.m_strPath;
		if (path.Exists() && path.IsDirectory() && PathIsDirectoryEmpty(path.GetWinPath()))
			path.Delete(false);

		if (dlg.m_bSquash)
			args += _T(" --squash");

		CString commitMessage;
		if (!dlg.m_strLogMesage.IsEmpty())
		{
			commitMessage = dlg.m_strLogMesage;
			commitMessage.Replace(_T("\""), _T("\\\""));
			commitMessage.Format(_T("-m \"%s\""), commitMessage);
		}

		// TODO: This should alternatively support a commit id
		CString cmd;
		cmd.Format(_T("git.exe subtree %s %s --prefix=\"%s\" \"%s\" %s %s"),
			subCommand, args, dlg.m_strPath, dlg.m_URL, dlg.m_BranchName, commitMessage);

		CProgressDlg progress;
		progress.m_GitCmd=cmd;
		progress.DoModal();

		// TODO: Save puttykeyfile provided to the config

		return true;
	}
	return false;
}

bool SubtreeAddCommand::Execute()
{
	CSubtreeCmdDlg dlg(cmdLinePath.GetDirectory().GetWinPathString(), CSubtreeCmdDlg::SubCommand_Add);
	return ExecuteSubtree(dlg);
}

bool SubtreePushCommand::Execute()
{
	CSubtreeCmdDlg dlg(cmdLinePath.GetDirectory().GetWinPathString(), CSubtreeCmdDlg::SubCommand_Push);
	return ExecuteSubtree(dlg);
}

bool SubtreePullCommand::Execute()
{
	CSubtreeCmdDlg dlg(cmdLinePath.GetDirectory().GetWinPathString(), CSubtreeCmdDlg::SubCommand_Pull);
	return ExecuteSubtree(dlg);
}
