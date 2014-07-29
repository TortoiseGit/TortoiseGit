// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013-2014 - TortoiseGit

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

#include "stdafx.h"
#include "ResetProgressCommand.h"
#include "ShellUpdater.h"
#include "AppUtils.h"

bool ResetProgressCommand::Run(CGitProgressList* list, CString& sWindowTitle, int& /*m_itemCountTotal*/, int& /*m_itemCount*/)
{
	if (!g_Git.UsingLibGit2(CGit::GIT_CMD_RESET))
	{
		// should never run to here
		ASSERT(0);
		return false;
	}

	list->SetWindowTitle(IDS_PROGRS_TITLE_RESET, g_Git.m_CurrentDir, sWindowTitle);
	list->SetBackgroundImage(IDI_UPDATE_BKG);
	int resetTypesResource[] = { IDS_RESET_SOFT, IDS_RESET_MIXED, IDS_RESET_HARD };
	list->ReportCmd(CString(MAKEINTRESOURCE(IDS_PROGRS_TITLE_RESET)) + _T(" ") + CString(MAKEINTRESOURCE(resetTypesResource[m_resetType])) + _T(" ") + m_revision);

	list->ShowProgressBar();
	CAutoRepository repo(g_Git.GetGitRepository());
	if (!repo)
	{
		list->ReportGitError();
		return false;
	}

	CAutoObject target;
	if (git_revparse_single(target.GetPointer(), repo, CUnicodeUtils::GetUTF8(m_revision)))
		goto error;
	if (git_reset(repo, target, (git_reset_t)(m_resetType + 1), nullptr, nullptr))
		goto error;

	return true;

error:
	list->ReportGitError();
	return false;
}
