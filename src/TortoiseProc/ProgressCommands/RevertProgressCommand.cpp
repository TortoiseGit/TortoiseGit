// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2014, 2016, 2019 - TortoiseGit

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
#include "RevertProgressCommand.h"
#include "ShellUpdater.h"
#include "AppUtils.h"
#include "../TGitCache/CacheInterface.h"

bool RevertProgressCommand::Run(CGitProgressList* list, CString& sWindowTitle, int& m_itemCountTotal, int& m_itemCount)
{
	list->SetWindowTitle(IDS_PROGRS_TITLE_REVERT, g_Git.CombinePath(m_targetPathList.GetCommonRoot().GetUIPathString()), sWindowTitle);
	list->SetBackgroundImage(IDI_REVERT_BKG);

	CBlockCacheForPath block(g_Git.m_CurrentDir);

	m_itemCountTotal = 2 * m_targetPathList.GetCount();
	CTGitPathList delList;
	for (m_itemCount = 0; m_itemCount < m_targetPathList.GetCount(); ++m_itemCount)
	{
		CTGitPath path;
		path.SetFromWin(g_Git.CombinePath(m_targetPathList[m_itemCount]));
		auto action = m_targetPathList[m_itemCount].m_Action;
		/* rename file can't delete because it needs original file*/
		if ((!(action & CTGitPath::LOGACTIONS_ADDED)) &&
			(!(action & CTGitPath::LOGACTIONS_REPLACED)))
			delList.AddPath(path);
	}
	if (DWORD(CRegDWORD(L"Software\\TortoiseGit\\RevertWithRecycleBin", TRUE)))
		delList.DeleteAllFiles(true);

	list->ReportCmd(CString(MAKEINTRESOURCE(IDS_PROGRS_CMD_REVERT)));
	for (int i = 0; i < m_targetPathList.GetCount(); ++i)
	{
		if (CString err; g_Git.Revert(L"HEAD", m_targetPathList[i], err))
		{
			list->ReportError(L"Revert failed:\n" + err);
			return false;
		}
		list->AddNotify(new CGitProgressList::WC_File_NotificationData(m_targetPathList[i], CGitProgressList::WC_File_NotificationData::git_wc_notify_revert));
		++m_itemCount;

		if (list->IsCancelled() == TRUE)
		{
			list->ReportUserCanceled();
			return false;
		}
	}

	CShellUpdater::Instance().AddPathsForUpdate(m_targetPathList);

	return true;
}
