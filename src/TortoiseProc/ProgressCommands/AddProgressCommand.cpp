// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2011-2014 - TortoiseGit

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
#include "AddProgressCommand.h"
#include "ShellUpdater.h"
#include "MassiveGitTask.h"

bool AddProgressCommand::Run(CGitProgressList* list, CString& sWindowTitle, int& m_itemCountTotal, int& m_itemCount)
{
	list->SetWindowTitle(IDS_PROGRS_TITLE_ADD, g_Git.CombinePath(m_targetPathList.GetCommonRoot().GetUIPathString()), sWindowTitle);
	list->SetBackgroundImage(IDI_ADD_BKG);
	list->ReportCmd(CString(MAKEINTRESOURCE(IDS_PROGRS_CMD_ADD)));

	m_itemCountTotal = m_targetPathList.GetCount();

	if (g_Git.UsingLibGit2(CGit::GIT_CMD_ADD))
	{
		CAutoRepository repo(g_Git.GetGitRepository());
		if (!repo)
		{
			list->ReportGitError();
			return false;
		}

		CAutoIndex index;
		if (git_repository_index(index.GetPointer(), repo))
		{
			list->ReportGitError();
			return false;
		}
		if (git_index_read(index, true))
		{
			list->ReportGitError();
			return false;
		}

		for (m_itemCount = 0; m_itemCount < m_itemCountTotal; ++m_itemCount)
		{
			if (git_index_add_bypath(index, CUnicodeUtils::GetUTF8(m_targetPathList[m_itemCount].GetGitPathString())))
			{
				list->ReportGitError();
				return false;
			}
			list->Notify(m_targetPathList[m_itemCount], git_wc_notify_add);

			if (list->IsCancelled() == TRUE)
			{
				list->ReportUserCanceled();
				return false;
			}
		}

		if (git_index_write(index))
		{
			list->ReportGitError();
			return false;
		}
	}
	else
	{
		CMassiveGitTask mgt(L"add -f");
		if (!mgt.ExecuteWithNotify(&m_targetPathList, list->m_bCancelled, git_wc_notify_add, list))
			return false;
	}

	CShellUpdater::Instance().AddPathsForUpdate(m_targetPathList);

	return true;
}
