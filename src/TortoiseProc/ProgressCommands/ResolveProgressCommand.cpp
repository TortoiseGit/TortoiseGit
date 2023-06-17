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
#include "../TGitCache/CacheInterface.h"
#include "AppUtils.h"
#include "MassiveGitTask.h"
#include "MessageBox.h"

using Git_WC_Notify_Action = CGitProgressList::WC_File_NotificationData::Git_WC_Notify_Action;

bool ResolveProgressCommand::Run(CGitProgressList* list, CString& sWindowTitle, int& m_itemCountTotal, int& m_itemCount)
{
	list->SetWindowTitle(IDS_PROGRS_TITLE_RESOLVE, g_Git.CombinePath(m_targetPathList.GetCommonRoot().GetUIPathString()), sWindowTitle);
	list->SetBackgroundImage(IDI_RESOLVE_BKG);

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

	CBlockCacheForPath block(g_Git.m_CurrentDir);

	if (m_resolveWith == ResolveWith::Current)
	{
		CMassiveGitTask addTask{ L"add -f" };
		for (auto& path : m_targetPathList)
		{
			if ((path.m_Action & CTGitPath::LOGACTIONS_UNMERGED) == 0)
			{
				list->AddNotify(new CGitProgressList::WC_File_NotificationData(path, Git_WC_Notify_Action::Skip));
				++m_itemCount;
				continue;
			}

			addTask.AddFile(path);
			CAppUtils::RemoveTempMergeFile(path);
		}

		m_itemCountTotal = addTask.GetListCount();

		addTask.SetProgressList(list);
		addTask.SetProgressCallback([&m_itemCount, &list](const CTGitPath& path, int) {
			list->AddNotify(new CGitProgressList::WC_File_NotificationData(path, Git_WC_Notify_Action::Resolved));
			++m_itemCount;
		});
		if (!addTask.Execute(list->m_bCancelled))
			return false;

		CShellUpdater::Instance().AddPathsForUpdate(m_targetPathList);
		return true;
	}

	CAutoIndex gitIndex;
	if (g_Git.UsingLibGit2(CGit::GIT_CMD_GETCONFLICTINFO))
	{
		CAutoRepository repo = g_Git.GetGitRepository();
		if (!repo || git_repository_index(gitIndex.GetPointer(), repo) < 0)
		{
			list->ReportGitError();
			return false;
		}
	}

	int destinationStage = (m_resolveWith == ResolveWith::Theirs ? 3 : 2);
	CMassiveGitTask addTask{ L"add -f" };
	CMassiveGitTask rmTask{ L"rm -f" };
	CString checkoutFromIndexTaskCmd;
	checkoutFromIndexTaskCmd.Format(L"checkout-index -f --stage=%d", destinationStage);
	CMassiveGitTask checkoutFromIndexTask{ checkoutFromIndexTaskCmd };
	for (auto& path : m_targetPathList)
	{
		bool b_local = false, b_remote = false;
		bool baseIsFile = true, localIsFile = true, remoteIsFile = true;
		CGitHash baseHash, localHash, remoteHash;
		if (!gitIndex)
		{
			CString cmd;
			cmd.Format(L"git.exe ls-files -u -t -z -- \"%s\"", static_cast<LPCWSTR>(path.GetGitPathString()));
			BYTE_VECTOR vector;
			if (g_Git.Run(cmd, &vector))
			{
				list->ReportError(L"git ls-files failed!");
				return false;
			}

			CTGitPathList pathList;
			if (pathList.ParserFromLsFile(vector))
			{
				list->ReportError(L"Parsing git ls-files failed!");
				return false;
			}

			if (pathList.IsEmpty())
			{
				list->AddNotify(new CGitProgressList::WC_File_NotificationData(path, Git_WC_Notify_Action::Skip));
				++m_itemCount;
				continue;
			}
			for (int i = 0; i < pathList.GetCount(); ++i)
			{
				if (pathList[i].m_Stage == 2)
					b_local = true;
				if (pathList[i].m_Stage == 3)
					b_remote = true;
			}

			if (CGit::ParseConflictHashesFromLsFile(vector, baseHash, baseIsFile, localHash, localIsFile, remoteHash, remoteIsFile))
			{
				list->ReportError(L"Parsing git ls-files for conflict info failed!");
				return false;
			}
		}
		else
		{
			const git_index_entry* ancestor = nullptr;
			const git_index_entry* our = nullptr;
			const git_index_entry* their = nullptr;
			if (int err = git_index_conflict_get(&ancestor, &our, &their, gitIndex, CUnicodeUtils::GetUTF8(path.GetGitPathString())); err == GIT_ENOTFOUND)
			{
				list->AddNotify(new CGitProgressList::WC_File_NotificationData(path, Git_WC_Notify_Action::Skip));
				++m_itemCount;
				continue;
			}
			else if (err < 0)
			{
				list->ReportGitError();
				return false;
			}

			if (ancestor) // stage == 1
			{
				baseHash = ancestor->id;
				baseIsFile = (ancestor->mode & S_IFDIR) != S_IFDIR;
			}
			if (our) // stage == 2
			{
				b_local = true;
				localHash = our->id;
				localIsFile = (our->mode & S_IFDIR) != S_IFDIR;
			}
			if (their) // stage == 3
			{
				b_remote = true;
				remoteHash = their->id;
				remoteIsFile = (their->mode & S_IFDIR) != S_IFDIR;
			}
		}

		if ((m_resolveWith == ResolveWith::Theirs && !b_remote) || (m_resolveWith == ResolveWith::Mine && !b_local))
		{
			if (!PathIsDirectory(path.GetGitPathString()))
			{
				rmTask.AddFile(path);
				CAppUtils::RemoveTempMergeFile(path);
				continue;
			}

			CString gitcmd, output; // retest with registered submodule!
			gitcmd.Format(L"git.exe rm -f -- \"%s\"", static_cast<LPCWSTR>(path.GetGitPathString()));
			if (g_Git.Run(gitcmd, &output, CP_UTF8))
			{
				// a .git folder in a submodule which is not in .gitmodules cannot be deleted using "git rm"
				if (!PathIsDirectoryEmpty(path.GetGitPathString()))
				{
					CString message(output);
					output += L"\n\n";
					output.AppendFormat(IDS_PROC_DELETEBRANCHTAG, path.GetWinPath());
					CString deleteButton;
					deleteButton.LoadString(IDS_DELETEBUTTON);
					CString abortButton;
					abortButton.LoadString(IDS_ABORTBUTTON);
					if (CMessageBox::Show(list->GetSafeHwnd(), output, L"TortoiseGit", 2, IDI_QUESTION, deleteButton, abortButton) == 2)
					{
						list->ReportUserCanceled();
						return false;
					}
					path.Delete(true, true);
					output.Empty();
					if (!g_Git.Run(gitcmd, &output, CP_UTF8))
					{
						list->AddNotify(new CGitProgressList::WC_File_NotificationData(path, Git_WC_Notify_Action::Resolved));
						++m_itemCount;
						CAppUtils::RemoveTempMergeFile(path);
						continue;
					}
				}
				list->ReportError(output);
				return false;
			}
			continue;
		}

		CGitHash destinationHash = (m_resolveWith == ResolveWith::Theirs ? remoteHash : localHash);
		if (bool targetWillBeAFile = (m_resolveWith == ResolveWith::Theirs ? remoteIsFile : localIsFile); !targetWillBeAFile)
		{
			CTGitPath fullPath;
			fullPath.SetFromWin(g_Git.CombinePath(path));
			if (!fullPath.IsWCRoot()) // check if submodule is initialized
			{
				CString gitcmd, output;
				if (!fullPath.IsDirectory())
				{
					gitcmd.Format(L"git.exe checkout-index -f --stage=%d -- \"%s\"", destinationStage, static_cast<LPCWSTR>(path.GetGitPathString()));
					if (g_Git.Run(gitcmd, &output, CP_UTF8))
					{
						list->ReportError(output);
						return false;
					}
				}
				gitcmd.Format(L"git.exe update-index --replace --cacheinfo 0160000,%s,\"%s\"", static_cast<LPCWSTR>(destinationHash.ToString()), static_cast<LPCWSTR>(path.GetGitPathString()));
				if (g_Git.Run(gitcmd, &output, CP_UTF8))
				{
					list->ReportError(output);
					return false;
				}
				CAppUtils::RemoveTempMergeFile(path);
				list->AddNotify(new CGitProgressList::WC_File_NotificationData(path, Git_WC_Notify_Action::Resolved));
				++m_itemCount;
				continue;
			}

			CGit subgit;
			subgit.m_IsUseGitDLL = false;
			subgit.m_CurrentDir = fullPath.GetWinPath();
			CGitHash submoduleHead;
			if (subgit.GetHash(submoduleHead, L"HEAD"))
			{
				list->ReportError(subgit.GetGitLastErr(L"Could not get HEAD hash of submodule, this should not happen!"));
				return false;
			}
			if (submoduleHead != destinationHash)
			{
				CString origPath = g_Git.m_CurrentDir;
				g_Git.m_CurrentDir = fullPath.GetWinPath();
				SetCurrentDirectory(g_Git.m_CurrentDir);
				if (!CAppUtils::GitReset(list->GetSafeHwnd(), destinationHash.ToString()))
				{
					g_Git.m_CurrentDir = origPath;
					SetCurrentDirectory(g_Git.m_CurrentDir);
					list->ReportUserCanceled();
					return false;
				}
				g_Git.m_CurrentDir = origPath;
				SetCurrentDirectory(g_Git.m_CurrentDir);
			}
			addTask.AddFile(path);
			CAppUtils::RemoveTempMergeFile(path);
			continue;
		}

		if (b_local && b_remote)
			checkoutFromIndexTask.AddFile(path);
		addTask.AddFile(path);
		CAppUtils::RemoveTempMergeFile(path);
	}

	rmTask.SetProgressList(list);
	rmTask.SetProgressCallback([&m_itemCount, &list](const CTGitPath& path, int) {
		list->AddNotify(new CGitProgressList::WC_File_NotificationData(path, Git_WC_Notify_Action::Resolved));
		++m_itemCount;
	});
	checkoutFromIndexTask.SetProgressList(list);
	checkoutFromIndexTask.SetProgressCallback([](const CTGitPath&, int) {});
	addTask.SetProgressList(list);
	addTask.SetProgressCallback([&m_itemCount, &list](const CTGitPath& path, int) {
		list->AddNotify(new CGitProgressList::WC_File_NotificationData(path, Git_WC_Notify_Action::Resolved));
		++m_itemCount;
	});

	BOOL cancel = FALSE;
	if (!rmTask.Execute(cancel) || !checkoutFromIndexTask.Execute(cancel) || !addTask.Execute(cancel))
		return false;

	CShellUpdater::Instance().AddPathsForUpdate(m_targetPathList);

	return true;
}

bool ResolveProgressCommand::ShowInfo(CString& info)
{
	info = L"You need commit your change after resolve conflict";
	return true;
}
