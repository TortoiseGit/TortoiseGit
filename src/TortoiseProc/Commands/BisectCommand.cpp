// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2013, 2015-2016, 2018-2019 - TortoiseGit

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
#include "BisectCommand.h"
#include "MessageBox.h"
#include "AppUtils.h"
#include "ProgressDlg.h"
#include "resource.h"

bool BisectCommand::Execute()
{
	if (!GitAdminDir::HasAdminDir(g_Git.m_CurrentDir))
	{
		CMessageBox::Show(GetExplorerHWND(), IDS_NOWORKINGCOPY, IDS_APPNAME, MB_ICONERROR);
		return false;
	}

	CTGitPath path = g_Git.m_CurrentDir;

	if (parser.HasKey(L"start") && !path.IsBisectActive())
	{
		CString lastGood, firstBad;
		if (parser.HasKey(L"good"))
			lastGood = parser.GetVal(L"good");
		if (parser.HasKey(L"bad"))
			firstBad = parser.GetVal(L"bad");

		return CAppUtils::BisectStart(GetExplorerHWND(), lastGood, firstBad);
	}
	else if ((this->parser.HasKey(L"good") || this->parser.HasKey(L"bad") || this->parser.HasKey(L"skip") || this->parser.HasKey(L"reset")) && path.IsBisectActive())
	{
		CString op;
		CString ref;

		if (parser.HasKey(L"good"))
			g_Git.GetBisectTerms(&op, nullptr);
		else if (parser.HasKey(L"bad"))
			g_Git.GetBisectTerms(nullptr, &op);
		else if (this->parser.HasKey(L"skip"))
			op = L"skip";
		else if (parser.HasKey(L"reset"))
			op = L"reset";

		if (parser.HasKey(L"ref") && !parser.HasKey(L"reset"))
			ref = parser.GetVal(L"ref");

		return CAppUtils::BisectOperation(GetExplorerHWND(), op, ref);
	}
	else
		MessageBox(GetExplorerHWND(), L"Operation unknown or not allowed.", L"TortoiseGit", MB_OK | MB_ICONINFORMATION);
	return false;
}
