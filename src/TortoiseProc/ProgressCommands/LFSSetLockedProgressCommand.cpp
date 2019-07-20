// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2019-2020 - TortoiseGit

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
#include "LFSSetLockedProgressCommand.h"
#include "MassiveGitTask.h"
#include "AppUtils.h"
#include "TempFile.h"

bool LFSSetLockedProgressCommand::Run(CGitProgressList* list, CString& sWindowTitle, int& m_itemCountTotal, int& m_itemCount)
{
	m_itemCountTotal = m_targetPathList.GetCount();
	m_itemCount = 0;

	list->SetWindowTitle(m_bIsLock ? IDS_PROGRS_TITLE_LFS_LOCK : IDS_PROGRS_TITLE_LFS_UNLOCK, g_Git.CombinePath(m_targetPathList.GetCommonRoot().GetUIPathString()), sWindowTitle);
	list->SetBackgroundImage(m_bIsLock ? IDI_LOCK_BKG : IDI_UNLOCK_BKG);
	list->ReportCmd(CString(MAKEINTRESOURCE(m_bIsLock ? IDS_PROGRS_CMD_LFS_LOCK : IDS_PROGRS_CMD_LFS_UNLOCK)));

	CGitProgressList::WC_File_NotificationData::git_wc_notify_action_t notifyAction =
		m_bIsLock ?	CGitProgressList::WC_File_NotificationData::git_wc_notify_lfs_lock :
					CGitProgressList::WC_File_NotificationData::git_wc_notify_lfs_unlock;

	CString cmdBase = L"git.exe lfs ";
	cmdBase += m_bIsLock ? L"lock " : L"unlock ";
	cmdBase += m_bIsForce ? L"--force " : L"";
	cmdBase += L'"';

	bool hasError = false;

	m_PostCmdCallback = [this](DWORD status, PostCmdList& postCmdList)
	{
		if (status && !m_bIsLock)
		{
			postCmdList.emplace_back(IDI_LFSUNLOCK, IDS_PROGS_LFS_FORCEUNLOCK, [this]()
			{
				CString tempfilename = CTempFiles::Instance().GetTempFilePath(false).GetWinPathString();
				VERIFY(m_targetPathList.WriteToFile(tempfilename));
				CString sCmd;
				sCmd.Format(L"/command:lfsunlock /force /pathfile:\"%s\" /deletepathfile", static_cast<LPCTSTR>(tempfilename));
				CAppUtils::RunTortoiseGitProc(sCmd);
			});
		}
	};

	for (int i = 0; i < m_targetPathList.GetCount(); ++i)
	{
		CString out;
		CString cmd = cmdBase + m_targetPathList[i].GetGitPathString() + "\"";

		list->AddNotify(new CGitProgressList::WC_File_NotificationData(m_targetPathList[i], notifyAction));

		if (g_Git.Run(cmd, &out, CP_UTF8))
		{
			hasError = true;
			list->ReportError(out);
		}

		++m_itemCount;

		if (list->IsCancelled() == TRUE)
		{
			list->ReportUserCanceled();
			return true;
		}
	}

	return true;
}
