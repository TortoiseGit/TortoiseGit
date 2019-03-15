// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2011-2016, 2018-2019 - TortoiseGit

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
#include "AppUtils.h"

bool AddProgressCommand::Run(CGitProgressList* list, CString& sWindowTitle, int& m_itemCountTotal, int& m_itemCount)
{
	ATLASSERT(!(m_bExecutable && m_bSymlink));
	list->SetWindowTitle(IDS_PROGRS_TITLE_ADD, g_Git.CombinePath(m_targetPathList.GetCommonRoot().GetUIPathString()), sWindowTitle);
	list->SetBackgroundImage(IDI_ADD_BKG);
	if (m_bExecutable)
		list->ReportCmd(CString(MAKEINTRESOURCE(IDS_STATUSLIST_CONTEXT_ADD_EXE)));
	else if (m_bSymlink)
		list->ReportCmd(CString(MAKEINTRESOURCE(IDS_STATUSLIST_CONTEXT_ADD_LINK)));
	else
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
			CStringA filePathA = CUnicodeUtils::GetMulti(m_targetPathList[m_itemCount].GetGitPathString(), CP_UTF8).TrimRight(L'/');
			if (git_index_add_bypath(index, filePathA))
			{
				list->ReportGitError();
				return false;
			}

			if (!m_targetPathList[m_itemCount].IsDirectory() && (m_bExecutable || m_bSymlink))
			{
				auto entry = const_cast<git_index_entry*>(git_index_get_bypath(index, filePathA, 0));
				if (m_bExecutable)
					entry->mode = GIT_FILEMODE_BLOB_EXECUTABLE;
				else if (m_bSymlink)
					entry->mode = GIT_FILEMODE_LINK;
				if (git_index_add(index, entry))
				{
					list->ReportGitError();
					return false;
				}
			}

			list->AddNotify(new CGitProgressList::WC_File_NotificationData(m_targetPathList[m_itemCount], CGitProgressList::WC_File_NotificationData::git_wc_notify_add));

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
		if (!mgt.ExecuteWithNotify(&m_targetPathList, list->m_bCancelled, CGitProgressList::WC_File_NotificationData::git_wc_notify_add, list))
			return false;
		if (m_bExecutable)
		{
			if (!SetFileMode(GIT_FILEMODE_BLOB_EXECUTABLE))
				return false;
		}
		else if (m_bSymlink)
		{
			if (!SetFileMode(GIT_FILEMODE_LINK))
				return false;
		}
	}

	CShellUpdater::Instance().AddPathsForUpdate(m_targetPathList);

	m_PostCmdCallback = [this](DWORD status, PostCmdList& postCmdList)
	{
		if (status)
			return;

		if (m_bShowCommitButtonAfterAdd)
			postCmdList.emplace_back(IDI_COMMIT, IDS_MENUCOMMIT, []
			{
				CString sCmd;
				sCmd.Format(L"/command:commit /path:\"%s\"", static_cast<LPCTSTR>(g_Git.m_CurrentDir));
				CAppUtils::RunTortoiseGitProc(sCmd);
			});
		if (!(m_bExecutable || m_bSymlink))
		{
			postCmdList.emplace_back(IDI_ADD, IDS_STATUSLIST_CONTEXT_ADD_EXE, [this] {
				SetFileMode(GIT_FILEMODE_BLOB_EXECUTABLE);
			});
			postCmdList.emplace_back(IDI_ADD, IDS_STATUSLIST_CONTEXT_ADD_LINK, [this] {
				SetFileMode(GIT_FILEMODE_LINK);
			});
		}
	};

	return true;
}

bool AddProgressCommand::SetFileMode(uint32_t mode)
{
	CAutoRepository repo(g_Git.GetGitRepository());
	if (!repo)
	{
		MessageBox(nullptr, g_Git.GetLibGit2LastErr(L"Could not open repository."), L"TortoiseGit", MB_ICONERROR);
		return false;
	}

	CAutoIndex index;
	if (git_repository_index(index.GetPointer(), repo))
	{
		MessageBox(nullptr, g_Git.GetLibGit2LastErr(L"Could not get index."), L"TortoiseGit", MB_ICONERROR);
		return false;
	}
	if (git_index_read(index, true))
	{
		MessageBox(nullptr, g_Git.GetLibGit2LastErr(L"Could not read index."), L"TortoiseGit", MB_ICONERROR);
		return false;
	}

	for (int i = 0; i < m_targetPathList.GetCount(); ++i)
	{
		if (m_targetPathList[i].IsDirectory())
			continue;
		CStringA filePathA = CUnicodeUtils::GetMulti(m_targetPathList[i].GetGitPathString(), CP_UTF8).TrimRight(L'/');
		auto entry = const_cast<git_index_entry*>(git_index_get_bypath(index, filePathA, 0));
		entry->mode = mode;
		if (git_index_add(index, entry))
		{
			MessageBox(nullptr, g_Git.GetLibGit2LastErr(L"Could not update index."), L"TortoiseGit", MB_ICONERROR);
			return false;
		}
	}

	if (git_index_write(index))
	{
		MessageBox(nullptr, g_Git.GetLibGit2LastErr(L"Could not write index."), L"TortoiseGit", MB_ICONERROR);
		return false;
	}

	return true;
}
