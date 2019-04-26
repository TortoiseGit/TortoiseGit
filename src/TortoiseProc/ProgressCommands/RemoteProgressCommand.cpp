// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013-2016, 2018-2019 - TortoiseGit

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
#include "RemoteProgressCommand.h"
#include "../TortoiseShell/resource.h"
#include "AppUtils.h"

int RemoteProgressCommand::RemoteProgressCallback(const char* str, int len, void* data)
{
	auto payload = static_cast<CGitProgressList::Payload*>(data);
	payload->list->SetProgressLabelText(CUnicodeUtils::GetUnicode(CStringA(str, len)));
	if (payload->list->m_bCancelled)
	{
		git_error_set_str(GIT_ERROR_NONE, "User cancelled.");
		return GIT_EUSER;
	}
	return 0;
}

int RemoteProgressCommand::RemoteCompletionCallback(git_remote_completion_t /*type*/, void* /*data*/)
{
	// this method is unused by libgit2 so far
	// TODO: "m_pAnimate->Stop();" and "m_pAnimate->ShowWindow(SW_HIDE);", cleanup possible in GitProgressList::Notify
	return 0;
}

int RemoteProgressCommand::RemoteUpdatetipsCallback(const char* refname, const git_oid* oldOid, const git_oid* newOid, void* data)
{
	auto ptr = static_cast<CGitProgressList::Payload*>(data);
	/*bool nonff = false;
	if (!git_oid_iszero(oldOid) && !git_oid_iszero(newOid))
	{
		git_oid baseOid = { 0 };
		if (!git_merge_base(&baseOid, ptr->repo, newOid, oldOid))
			if (!git_oid_equal(oldOid, &baseOid))
				nonff = true;
	}*/

	CString change;
	if (!git_oid_iszero(oldOid) && !git_oid_iszero(newOid))
	{
		if (git_oid_equal(oldOid, newOid))
			change.LoadString(IDS_SAME);
		else
		{
			size_t ahead = 0, behind = 0;
			if (!git_graph_ahead_behind(&ahead, &behind, ptr->repo, newOid, oldOid))
			{
				if (ahead > 0 && behind == 0)
					change.Format(IDS_FORWARDN, ahead);
				else if (ahead == 0 && behind > 0)
					change.Format(IDS_REWINDN, behind);
				else
				{
					git_commit* oldCommit, * newCommit;
					git_time_t oldTime = 0, newTime = 0;
					if (!git_commit_lookup(&oldCommit, ptr->repo, oldOid))
						oldTime = git_commit_committer(oldCommit)->when.time;
					if (!git_commit_lookup(&newCommit, ptr->repo, newOid))
						newTime = git_commit_committer(newCommit)->when.time;
					if (oldTime < newTime)
						change.LoadString(IDS_SUBMODULEDIFF_NEWERTIME);
					else if (oldTime > newTime)
						change.LoadString(IDS_SUBMODULEDIFF_OLDERTIME);
					else
						change.LoadString(IDS_SUBMODULEDIFF_SAMETIME);
				}
			}
		}
	}
	else if (!git_oid_iszero(oldOid))
		change.LoadString(IDS_DELETED);
	else if (!git_oid_iszero(newOid))
		change.LoadString(IDS_NEW);

	ptr->list->AddNotify(new RefUpdateNotificationData(refname, oldOid, newOid, change));
	return 0;
}

RemoteProgressCommand::RefUpdateNotificationData::RefUpdateNotificationData(const char* refname, const git_oid* oldOid, const git_oid* newOid, const CString& change)
	: NotificationData()
{
	CString str = CUnicodeUtils::GetUnicode(refname);
	m_NewHash = newOid;
	m_OldHash = oldOid;
	sActionColumnText.LoadString(IDS_GITACTION_UPDATE_REF);
	sPathColumnText.Format(L"%s\t %s -> %s (%s)", static_cast<LPCTSTR>(str), static_cast<LPCTSTR>(m_OldHash.ToString(g_Git.GetShortHASHLength())), static_cast<LPCTSTR>(m_NewHash.ToString(g_Git.GetShortHASHLength())), static_cast<LPCTSTR>(change));
}

void RemoteProgressCommand::RefUpdateNotificationData::GetContextMenu(CIconMenu& popup, CGitProgressList::ContextMenuActionList& actions)
{
	actions.push_back([&]()
	{
		CString cmd = L"/command:log";
		cmd += L" /path:\"" + g_Git.m_CurrentDir + L'"';
		if (!m_OldHash.IsEmpty())
			cmd += L" /startrev:" + m_OldHash.ToString();
		if (!m_NewHash.IsEmpty())
			cmd += L" /endrev:" + m_NewHash.ToString();
		CAppUtils::RunTortoiseGitProc(cmd);
	});
	popup.AppendMenuIcon(actions.size(), IDS_MENULOG, IDI_LOG);
}
