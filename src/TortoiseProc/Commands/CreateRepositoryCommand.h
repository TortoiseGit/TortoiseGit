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
#pragma once
#include "Command.h"

#include "MessageBox.h"
#include "CommonResource.h"
#include "git.h"

#include "CreateRepoDlg.h"
/**
 * \ingroup TortoiseProc
 * Creates a repository
 */
class CreateRepositoryCommand : public Command
{
public:
	/**
	 * Executes the command.
	 */
	virtual bool			Execute()
	{
		CCreateRepoDlg dlg;
		if(dlg.DoModal()==IDOK)
		{
			CGit git;
			git.m_CurrentDir=this->orgCmdLinePath.GetWinPath();
			CString output;
			int Ret;

			if (dlg.m_bBare) Ret = git.Run(_T("git.exe init-db --bare"),&output,CP_UTF8);
			else Ret = git.Run(_T("git.exe init-db"),&output,CP_UTF8);

			if (output.IsEmpty()) output = _T("git.Run() had no output");

			if (Ret)
			{
				CMessageBox::Show(hwndExplorer, output, _T("TortoiseGit"), MB_ICONERROR);
				return false;
			}
			else
			{
				CMessageBox::Show(hwndExplorer, output, _T("TortoiseGit"), MB_OK | MB_ICONINFORMATION);
			}
			return true;
		}
		return false;
	}
};


