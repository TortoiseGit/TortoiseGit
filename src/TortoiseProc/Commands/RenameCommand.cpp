// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2012 - TortoiseGit
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
#include "RenameCommand.h"

#include "MessageBox.h"
#include "RenameDlg.h"
#include "Git.h"
#include "ShellUpdater.h"

bool RenameCommand::Execute()
{
	bool bRet = true;
	CString filename = cmdLinePath.GetFileOrDirectoryName();
	CString basePath = cmdLinePath.GetContainingDirectory().GetGitPathString();

	// show the rename dialog until the user either cancels or enters a new
	// name (one that's different to the original name
	CString sNewName;
	do
	{
		CRenameDlg dlg;
		dlg.m_name = filename;
		if (dlg.DoModal() != IDOK)
			return FALSE;
		sNewName = dlg.m_name;
	} while(PathIsRelative(sNewName) && !PathIsURL(sNewName) && (sNewName.IsEmpty() || (sNewName.Compare(filename)==0)));

	if(!basePath.IsEmpty())
		sNewName=basePath+"/"+sNewName;

	CString force;
	// if the filenames only differ in case, we have to pass "-f"
	if (sNewName.CompareNoCase(cmdLinePath.GetGitPathString()) == 0)
		force = _T("-f ");

	CString cmd;
	CString output;
	cmd.Format(_T("git.exe mv %s-- \"%s\" \"%s\""),
					force,
					cmdLinePath.GetGitPathString(),
					sNewName);

	if (g_Git.Run(cmd, &output, CP_UTF8))
	{
		CMessageBox::Show(hwndExplorer, output, _T("TortoiseGit"), MB_OK);
		bRet = false;
	}

	CTGitPath newpath;
	newpath.SetFromGit(sNewName);

	CShellUpdater::Instance().AddPathForUpdate(newpath);
	CShellUpdater::Instance().Flush();
	return bRet;
}
