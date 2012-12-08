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
#include "AppUtils.h"
#include "ShowCompareCommand.h"
#include "GitDiff.h"

bool ShowCompareCommand::Execute()
{
	CString		rev1;
	CString		rev2;

	bool		unified = !!parser.HasKey(_T("unified"));

	if (parser.HasVal(_T("revision1")))
		rev1 = parser.GetVal(_T("revision1"));
	if (parser.HasVal(_T("revision2")))
		rev2 = parser.GetVal(_T("revision2"));

	if (unified)
	{
		CString tempfile = GetTempFile();
		CString cmd;

		if (rev1.IsEmpty())
			cmd.Format(_T("git.exe diff -r -p --stat %s"), rev2);
		else if (rev2.IsEmpty())
			cmd.Format(_T("git.exe diff -r -p --stat %s"), rev1);
		else
			cmd.Format(_T("git.exe diff-tree -r -p --stat %s %s"), rev1, rev2);

		g_Git.RunLogFile(cmd, tempfile);
		return !!CAppUtils::StartUnifiedDiffViewer(tempfile, rev1 + _T(":") + rev2);
	}
	else
		return !!CGitDiff::DiffCommit(cmdLinePath, rev2, rev1);
}
