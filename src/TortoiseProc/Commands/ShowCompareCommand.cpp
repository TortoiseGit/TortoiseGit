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
#include "stdafx.h"
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
		return !!CAppUtils::StartShowUnifiedDiff(NULL, CTGitPath(), rev1, CTGitPath(), rev2);
	}
	else
		return !!CGitDiff::DiffCommit(cmdLinePath, rev2, rev1);
}
