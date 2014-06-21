// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2014 - TortoiseGit

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
#include "ResolveProgressCommand.h"
#include "ShellUpdater.h"
#include "AppUtils.h"

bool ResolveProgressCommand::Run(CGitProgressList* list, CString& sWindowTitle, int& m_itemCountTotal, int& m_itemCount)
{
	ASSERT(m_targetPathList.GetCount() == 1);
	list->SetWindowTitle(IDS_PROGRS_TITLE_RESOLVE, g_Git.CombinePath(m_targetPathList.GetCommonRoot().GetUIPathString()), sWindowTitle);
	list->SetBackgroundImage(IDI_RESOLVE_BKG);

	m_itemCountTotal = m_targetPathList.GetCount();
	for (m_itemCount = 0; m_itemCount < m_itemCountTotal; ++m_itemCount)
	{
		CString cmd, out, tempmergefile;
		cmd.Format(_T("git.exe add -f -- \"%s\""), m_targetPathList[m_itemCount].GetGitPathString());
		if (g_Git.Run(cmd, &out, CP_UTF8))
		{
			list->ReportError(out);
			return false;
		}

		CAppUtils::RemoveTempMergeFile((CTGitPath &)m_targetPathList[m_itemCount]);

		list->Notify(m_targetPathList[m_itemCount], git_wc_notify_resolved);
	}

	CShellUpdater::Instance().AddPathsForUpdate(m_targetPathList);

	return true;
}
