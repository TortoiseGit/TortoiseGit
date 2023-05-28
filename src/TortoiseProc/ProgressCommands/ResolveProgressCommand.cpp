// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2016, 2019, 2023 - TortoiseGit

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
#include "MassiveGitTask.h"

using Git_WC_Notify_Action = CGitProgressList::WC_File_NotificationData::Git_WC_Notify_Action;

bool ResolveProgressCommand::Run(CGitProgressList* list, CString& sWindowTitle, int& m_itemCountTotal, int& m_itemCount)
{
	list->SetWindowTitle(IDS_PROGRS_TITLE_RESOLVE, g_Git.CombinePath(m_targetPathList.GetCommonRoot().GetUIPathString()), sWindowTitle);
	list->SetBackgroundImage(IDI_RESOLVE_BKG);

	m_itemCountTotal = m_targetPathList.GetCount();

	CMassiveGitTask mgt(L"add -f");
	mgt.SetProgressCallback([&m_itemCount, &list](const CTGitPath& path, int) {
		CAppUtils::RemoveTempMergeFile(path);
		list->AddNotify(new CGitProgressList::WC_File_NotificationData(path, Git_WC_Notify_Action::Resolved));
		++m_itemCount;
	});
	if (!mgt.ExecuteWithNotify(&m_targetPathList, list->m_bCancelled, Git_WC_Notify_Action::Resolved, list))
		return false;

	CShellUpdater::Instance().AddPathsForUpdate(m_targetPathList);

	m_PostCmdCallback = [](DWORD status, PostCmdList& postCmdList)
	{
		if (status)
			return;

		postCmdList.emplace_back(IDI_COMMIT, IDS_MENUCOMMIT, []
		{
			CString sCmd;
			sCmd.Format(L"/command:commit /path:\"%s\"", static_cast<LPCWSTR>(g_Git.m_CurrentDir));
			CAppUtils::RunTortoiseGitProc(sCmd);
		});
	};

	return true;
}

bool ResolveProgressCommand::ShowInfo(CString& info)
{
	info = L"You need commit your change after resolve conflict";
	return true;
}
