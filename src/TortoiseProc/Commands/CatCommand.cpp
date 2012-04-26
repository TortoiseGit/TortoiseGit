// TortoiseGit - a Windows shell extension for easy version control

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
#include "CatCommand.h"

#include "PathUtils.h"
#include "Git.h"
#include "MessageBox.h"

bool CatCommand::Execute()
{

	CString savepath = CPathUtils::GetLongPathname(parser.GetVal(_T("savepath")));
	CString revision = parser.GetVal(_T("revision"));
	CString pegrevision = parser.GetVal(_T("pegrevision"));

	CString cmd, output, err;
	cmd.Format(_T("git.exe cat-file -t %s"),revision);

	if (g_Git.Run(cmd, &output, &err, CP_UTF8))
	{
		CMessageBox::Show(NULL, output + L"\n" + err, _T("TortoiseGit"), MB_ICONERROR);
		return false;
	}

	if(output.Find(_T("blob")) == 0)
	{
		cmd.Format(_T("git.exe cat-file -p %s"),revision);
	}
	else
	{
		cmd.Format(_T("git.exe show %s -- \"%s\""),revision,this->cmdLinePath);
	}

	if(g_Git.RunLogFile(cmd,savepath))
	{
		CMessageBox::Show(NULL,_T("Cat file fail"),_T("TortoiseGit"), MB_ICONERROR);
		return false;
	}
	return true;
}
