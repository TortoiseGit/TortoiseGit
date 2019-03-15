// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2012, 2015-2019 - TortoiseGit
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
#include "RenameCommand.h"

#include "MessageBox.h"
#include "RenameDlg.h"
#include "Git.h"
#include "ShellUpdater.h"

bool RenameCommand::Execute()
{
	if (!GitAdminDir::HasAdminDir(g_Git.m_CurrentDir))
	{
		CMessageBox::Show(GetExplorerHWND(), IDS_NOWORKINGCOPY, IDS_APPNAME, MB_ICONERROR);
		return false;
	}

	bool bRet = true;
	CString filename = cmdLinePath.GetFileOrDirectoryName();
	CString basePath = cmdLinePath.GetContainingDirectory().GetGitPathString();

	// show the rename dialog until the user either cancels or enters a new
	// name (one that's different to the original name
	CString sNewName;
	CRenameDlg dlg;
	dlg.SetInputValidator([&](const int /*nID*/, const CString& input) -> CString
	{
		CString newName;
		if (!basePath.IsEmpty())
			newName = basePath + "/" + input;
		else
			newName = input;

		if (newName.CompareNoCase(cmdLinePath.GetGitPathString()) != 0 && PathFileExists(g_Git.CombinePath(newName)))
			return CString(CFormatMessageWrapper(ERROR_FILE_EXISTS));

		return{};
	});
	dlg.m_sBaseDir = g_Git.CombinePath(basePath);
	dlg.m_name = filename;
	if (dlg.DoModal() != IDOK)
		return FALSE;
	if (!basePath.IsEmpty())
		sNewName = basePath + "/" + dlg.m_name;
	else
		sNewName = dlg.m_name;

	CString force;
	// if the filenames only differ in case, we have to pass "-f"
	if (sNewName.CompareNoCase(cmdLinePath.GetGitPathString()) == 0)
		force = L"-f ";

	CString cmd;
	CString output;
	cmd.Format(L"git.exe mv %s-- \"%s\" \"%s\"",
					static_cast<LPCTSTR>(force),
					static_cast<LPCTSTR>(cmdLinePath.GetGitPathString()),
					static_cast<LPCTSTR>(sNewName));

	if (g_Git.Run(cmd, &output, CP_UTF8))
	{
		CMessageBox::Show(GetExplorerHWND(), output, L"TortoiseGit", MB_OK);
		bRet = false;
	}

	CTGitPath newpath;
	newpath.SetFromGit(sNewName);

	CShellUpdater::Instance().AddPathForUpdate(newpath);
	CShellUpdater::Instance().Flush();
	return bRet;
}
