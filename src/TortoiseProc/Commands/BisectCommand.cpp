// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2013, 2015-2016 - TortoiseGit

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
#include "AppUtils.h"
#include "ProgressDlg.h"
#include "MessageBox.h"
#include "resource.h"

bool BisectCommand::Execute()
{
	CTGitPath path = g_Git.m_CurrentDir;

	if (this->parser.HasKey(_T("start")) && !path.IsBisectActive())
	{
		CString lastGood, firstBad;
		if (parser.HasKey(_T("good")))
			lastGood = parser.GetVal(_T("good"));
		if (parser.HasKey(_T("bad")))
			firstBad = parser.GetVal(_T("bad"));

		return CAppUtils::BisectStart(lastGood, firstBad, true);
	}
	else if ((this->parser.HasKey(_T("good")) || this->parser.HasKey(_T("bad")) || this->parser.HasKey(_T("reset"))) && path.IsBisectActive())
	{
		CString op;
		CString ref;

		if (this->parser.HasKey(_T("good")))
			g_Git.GetBisectTerms(&op, nullptr);
		else if (this->parser.HasKey(_T("bad")))
			g_Git.GetBisectTerms(nullptr, &op);
		else if (this->parser.HasKey(_T("reset")))
			op = _T("reset");

		if (this->parser.HasKey(_T("ref")) &&! this->parser.HasKey(_T("reset")))
			ref = this->parser.GetVal(_T("ref"));

		return CAppUtils::BisectOperation(op, ref);
	}
	else
	{
		CMessageBox::Show(NULL,_T("Operation unknown or not allowed."), _T("TortoiseGit"), MB_OK|MB_ICONINFORMATION);
	}
	return false;
}
