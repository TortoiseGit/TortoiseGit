// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2014, 2016, 2019, 2022-2023 - TortoiseGit

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
#include "MassiveGitTask.h"

RevertProgressCommand::RevertProgressCommand(const CString& revertToRevision)
	: m_sRevertToRevision(revertToRevision)
{}

bool RevertProgressCommand::Run(CGitProgressList* list, CString& sWindowTitle, int& m_itemCountTotal, int& m_itemCount)
{
	list->SetWindowTitle(IDS_PROGRS_TITLE_REVERT, g_Git.CombinePath(m_targetPathList.GetCommonRoot().GetUIPathString()), sWindowTitle);
	list->SetBackgroundImage(IDI_REVERT_BKG);

	CBlockCacheForPath block(g_Git.m_CurrentDir);

	m_itemCountTotal = 2 * m_targetPathList.GetCount();
	CTGitPathList delList;
	bool recycleRenamedFiles = CRegDWORD(L"Software\\TortoiseGit\\RevertRenamedFilesWithRecycleBin", TRUE) != FALSE;
	for (m_itemCount = 0; m_itemCount < m_targetPathList.GetCount(); ++m_itemCount)
	{
		CTGitPath path;
		path.SetFromWin(g_Git.CombinePath(m_targetPathList[m_itemCount]));
		auto action = m_targetPathList[m_itemCount].m_Action;
		/* rename file can't delete because it needs original file*/
		if ((action & CTGitPath::LOGACTIONS_ADDED) || !recycleRenamedFiles && (action & CTGitPath::LOGACTIONS_REPLACED))
			continue;
		delList.AddPath(path);
	}
	if (DWORD(CRegDWORD(L"Software\\TortoiseGit\\RevertWithRecycleBin", TRUE)))
		delList.DeleteAllFiles(true);

	list->ReportCmd(CString(MAKEINTRESOURCE(IDS_PROGRS_CMD_REVERT)));

	CTGitPathList moveList;
	CMassiveGitTask unstageTask{ L"rm -f --cached" };
	CMassiveGitTask checkoutTask{ L"checkout " + m_sRevertToRevision + L" -f" };
	CMassiveGitTask addTask{ L"add -f" };
	CMassiveGitTask deleteTask{ L"rm --ignore-unmatch" };
	bool hasSubmodule = false;

	// prepare mass revert operation
	for (int i = 0; i < m_targetPathList.GetCount(); ++i)
	{
		auto& path = m_targetPathList[i];
		if (path.m_Action & CTGitPath::LOGACTIONS_REPLACED && !path.GetGitOldPathString().IsEmpty())
		{
			if (CTGitPath(path.GetGitOldPathString()).IsDirectory())
			{
				CString err;
				err.Format(L"Revert failed:\nCannot revert renaming of \"%s\". A directory with the old name \"%s\" exists.", static_cast<LPCWSTR>(path.GetGitPathString()), static_cast<LPCWSTR>(path.GetGitOldPathString()));
				list->ReportError(err);
				return false;
			}
			if (path.Exists())
			{
				if (path.IsDirectory())
					moveList.AddPath(path); // submodule renames need special handling
				else
				{
					deleteTask.AddFile(path);
					checkoutTask.AddFile(path.GetGitOldPathString());
				}
			}
			else
			{
				unstageTask.AddFile(path);
				checkoutTask.AddFile(path.GetGitPathString());
			}
		}
		else if (path.m_Action & CTGitPath::LOGACTIONS_ADDED)
			unstageTask.AddFile(path);
		else
			checkoutTask.AddFile(path);

		if (path.m_Action & CTGitPath::LOGACTIONS_DELETED)
			addTask.AddFile(path);
		if (path.IsDirectory() || ((path.m_Action & CTGitPath::LOGACTIONS_REPLACED) == 0 && !path.GetGitOldPathString().IsEmpty() && CTGitPath(path.GetGitOldPathString()).IsDirectory()))
			hasSubmodule = true;
	}

	unstageTask.SetProgressCallback([&m_itemCount, &list](const CTGitPath& path, int) {
		if ((path.m_Action & CTGitPath::LOGACTIONS_DELETED) != 0)
			return;
		list->AddNotify(new CGitProgressList::WC_File_NotificationData(path, CGitProgressList::WC_File_NotificationData::git_wc_notify_revert));
		++m_itemCount;
	});
	unstageTask.SetProgressList(list);
	if (!unstageTask.Execute(list->m_bCancelled))
		return false;

	deleteTask.SetProgressCallback([&m_itemCount, &list](const CTGitPath&, int) {});
	deleteTask.SetProgressList(list);
	if (!deleteTask.Execute(list->m_bCancelled))
		return false;

	checkoutTask.SetProgressCallback([&m_itemCount, &list](const CTGitPath& path, int) {
		if ((path.m_Action & CTGitPath::LOGACTIONS_DELETED) != 0)
			return;
		list->AddNotify(new CGitProgressList::WC_File_NotificationData(path, CGitProgressList::WC_File_NotificationData::git_wc_notify_revert));
		++m_itemCount;
	});
	checkoutTask.SetProgressList(list);
	if (!checkoutTask.Execute(list->m_bCancelled))
		return false;

	addTask.SetProgressCallback([&m_itemCount, &list](const CTGitPath& path, int) {
		list->AddNotify(new CGitProgressList::WC_File_NotificationData(path, CGitProgressList::WC_File_NotificationData::git_wc_notify_revert));
		++m_itemCount;
	});
	addTask.SetProgressList(list);
	if (!addTask.Execute(list->m_bCancelled))
		return false;

	// for handling submodule renames
	for (const auto& path : moveList)
	{
		CString force;
		// if the filenames only differ in case, we have to pass "-f"
		if (path.GetGitPathString().CompareNoCase(path.GetGitOldPathString()) == 0)
			force = L"-f ";
		CString cmd;
		cmd.Format(L"git.exe mv %s-- \"%s\" \"%s\"", static_cast<LPCWSTR>(force), static_cast<LPCWSTR>(path.GetGitPathString()), static_cast<LPCWSTR>(path.GetGitOldPathString()));
		if (CString err; g_Git.Run(cmd, &err, CP_UTF8))
		{
			list->ReportError(err);
			return false;
		}

		cmd.Format(L"git.exe checkout %s -f -- \"%s\"", static_cast<LPCWSTR>(m_sRevertToRevision), static_cast<LPCWSTR>(path.GetGitOldPathString()));
		if (CString err; g_Git.Run(cmd, &err, CP_UTF8))
		{
			list->ReportError(err);
			return false;
		}

		list->AddNotify(new CGitProgressList::WC_File_NotificationData(path, CGitProgressList::WC_File_NotificationData::git_wc_notify_revert));
		++m_itemCount;

		if (list->IsCancelled())
		{
			list->ReportUserCanceled();
			return false;
		}
	}

	if (hasSubmodule)
	{
		m_PostCmdCallback = [this](DWORD status, PostCmdList& postCmdList) {
			if (status)
				return;

			postCmdList.emplace_back(IDI_DIFF, IDS_HANDLESUBMODULES, [this] {
				for (const auto& path : m_targetPathList)
				{
					if (!path.IsDirectory() && !((path.m_Action & CTGitPath::LOGACTIONS_REPLACED) == 0 && !path.GetGitOldPathString().IsEmpty() && CTGitPath(path.GetGitOldPathString()).IsDirectory()))
						continue;
					CString pathString{ path.GetGitPathString() };
					if (path.m_Action & CTGitPath::LOGACTIONS_REPLACED)
						pathString = path.GetGitOldPathString();

					CString sCmd;
					sCmd.Format(L"/command:diff /submodule /startrev:%s /endrev:%s /path:\"%s\"", static_cast<LPCWSTR>(m_sRevertToRevision), GIT_REV_ZERO, static_cast<LPCWSTR>(pathString));
					CCommonAppUtils::RunTortoiseGitProc(sCmd);
				}
			});
		};
	}

	CShellUpdater::Instance().AddPathsForUpdate(m_targetPathList);

	return true;
}
