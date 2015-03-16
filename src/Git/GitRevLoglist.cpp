// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2015 - TortoiseGit

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
#include <ATLComTime.h>
#include "GitRevLoglist.h"
#include "Git.h"
#include "gitdll.h"
#include "UnicodeUtils.h"

typedef CComCritSecLock<CComCriticalSection> CAutoLocker;

GitRevLoglist::GitRevLoglist(void)
{
	GitRev();
	m_Action = 0;
	m_RebaseAction = 0;
	m_IsFull = FALSE;
	m_IsUpdateing = FALSE;
	m_IsCommitParsed = FALSE;
	m_IsDiffFiles = FALSE;
	m_CallDiffAsync = nullptr;
	m_IsSimpleListReady = FALSE;
	m_Mark = 0;

	SecureZeroMemory(&m_GitCommit, sizeof(GIT_COMMIT));
}

GitRevLoglist::~GitRevLoglist(void)
{
	if (!m_IsCommitParsed && m_GitCommit.m_pGitCommit)
		git_free_commit(&m_GitCommit);
}

void GitRevLoglist::Clear()
{
	GitRev::Clear();
	m_Action = 0;
	m_Files.Clear();
	m_UnRevFiles.Clear();
	m_Ref.Empty();
	m_RefAction.Empty();
	m_Mark = 0;
	m_IsFull = FALSE;
	m_IsUpdateing = FALSE;
	m_IsCommitParsed = FALSE;
	m_IsDiffFiles = FALSE;
	m_CallDiffAsync = nullptr;
	m_IsSimpleListReady = FALSE;
}


int GitRevLoglist::SafeGetSimpleList(CGit* git)
{
	if (InterlockedExchange(&m_IsUpdateing, TRUE) == TRUE)
		return 0;

	m_SimpleFileList.clear();
	if (git->UsingLibGit2(CGit::GIT_CMD_LOGLISTDIFF))
	{
		CAutoRepository repo(git->GetGitRepository());
		if (!repo)
			return -1;
		CAutoCommit commit;
		if (git_commit_lookup(commit.GetPointer(), repo, (const git_oid*)m_CommitHash.m_hash) < 0)
			return -1;

		CAutoTree commitTree;
		if (git_commit_tree(commitTree.GetPointer(), commit) < 0)
			return -1;

		bool isRoot = git_commit_parentcount(commit) == 0;
		for (unsigned int parentId = 0; isRoot || parentId < git_commit_parentcount(commit); ++parentId)
		{
			CAutoTree parentTree;
			if (!isRoot)
			{
				CAutoCommit parentCommit;
				if (git_commit_parent(parentCommit.GetPointer(), commit, parentId) < 0)
					return -1;

				if (git_commit_tree(parentTree.GetPointer(), parentCommit) < 0)
					return -1;
			}
			isRoot = false;

			CAutoDiff diff;
			if (git_diff_tree_to_tree(diff.GetPointer(), repo, parentTree, commitTree, nullptr) < 0)
				return -1;

			size_t deltas = git_diff_num_deltas(diff);
			for (size_t i = 0; i < deltas; ++i)
			{
				const git_diff_delta* delta = git_diff_get_delta(diff, i);
				m_SimpleFileList.push_back(CUnicodeUtils::GetUnicode(delta->new_file.path));
			}
		}

		InterlockedExchange(&m_IsUpdateing, FALSE);
		InterlockedExchange(&m_IsSimpleListReady, TRUE);
		return 0;
	}
	git->CheckAndInitDll();
	GIT_COMMIT commit = { 0 };
	GIT_COMMIT_LIST list;
	GIT_HASH parent;

	CAutoLocker lock(g_Git.m_critGitDllSec);

	try
	{
		if (git_get_commit_from_hash(&commit, m_CommitHash.m_hash))
			return -1;
	}
	catch (char *)
	{
		return -1;
	}

	int i = 0;
	bool isRoot = m_ParentHash.empty();
	git_get_commit_first_parent(&commit, &list);
	while (git_get_commit_next_parent(&list, parent) == 0 || isRoot)
	{
		GIT_FILE file = 0;
		int count = 0;
		try
		{
			if (isRoot)
				git_root_diff(git->GetGitSimpleListDiff(), commit.m_hash, &file, &count, 0);
			else
				git_do_diff(git->GetGitSimpleListDiff(), parent, commit.m_hash, &file, &count, 0);
		}
		catch (char *)
		{
			return -1;
		}

		isRoot = false;

		for (int j = 0; j < count; ++j)
		{
			char* newname;
			char* oldname;
			int mode, IsBin, inc, dec;

			try
			{
				git_get_diff_file(git->GetGitSimpleListDiff(), file, j, &newname, &oldname, &mode, &IsBin, &inc, &dec);
			}
			catch (char *)
			{
				return -1;
			}

			m_SimpleFileList.push_back(CUnicodeUtils::GetUnicode(newname));

		}
		git_diff_flush(git->GetGitSimpleListDiff());
		++i;
	}

	InterlockedExchange(&m_IsUpdateing, FALSE);
	InterlockedExchange(&m_IsSimpleListReady, TRUE);
	git_free_commit(&commit);

	return 0;
}

int GitRevLoglist::SafeFetchFullInfo(CGit* git)
{
	if (InterlockedExchange(&m_IsUpdateing, TRUE) == TRUE)
		return 0;


	m_Files.Clear();
	if (git->UsingLibGit2(CGit::GIT_CMD_LOGLISTDIFF))
	{
		CAutoRepository repo(git->GetGitRepository());
		if (!repo)
			return -1;
		CAutoCommit commit;
		if (git_commit_lookup(commit.GetPointer(), repo, (const git_oid*)m_CommitHash.m_hash) < 0)
			return -1;

		CAutoTree commitTree;
		if (git_commit_tree(commitTree.GetPointer(), commit) < 0)
			return -1;

		bool isRoot = git_commit_parentcount(commit) == 0;
		for (unsigned int parentId = 0; isRoot || parentId < git_commit_parentcount(commit); ++parentId)
		{
			CAutoTree parentTree;
			if (!isRoot)
			{
				CAutoCommit parentCommit;
				if (git_commit_parent(parentCommit.GetPointer(), commit, parentId) < 0)
					return -1;

				if (git_commit_tree(parentTree.GetPointer(), parentCommit) < 0)
					return -1;
			}
			isRoot = false;

			CAutoDiff diff;
			if (git_diff_tree_to_tree(diff.GetPointer(), repo, parentTree, commitTree, nullptr) < 0)
				return -1;

			if (git_diff_find_similar(diff, nullptr) < 0)
				return-1;

			const git_diff_delta* lastDelta = nullptr;
			int oldAction = 0;
			size_t deltas = git_diff_num_deltas(diff);
			for (size_t i = 0; i < deltas; ++i)
			{
				CAutoPatch patch;
				if (git_patch_from_diff(patch.GetPointer(), diff, i) < 0)
					return -1;

				const git_diff_delta* delta = git_patch_get_delta(patch);

				if (lastDelta && strcmp(lastDelta->new_file.path, delta->new_file.path) == 0 && (lastDelta->status == GIT_DELTA_DELETED && delta->status == GIT_DELTA_ADDED || delta->status == GIT_DELTA_DELETED && lastDelta->status == GIT_DELTA_ADDED))
				{
					CTGitPath path = m_Files[m_Files.GetCount() - 1];
					m_Files.RemoveItem(path);
					path.m_StatAdd = _T("-");
					path.m_StatDel = _T("-");
					path.m_Action = CTGitPath::LOGACTIONS_MODIFIED;
					m_Action = oldAction | CTGitPath::LOGACTIONS_MODIFIED;
					m_Files.AddPath(path);
					lastDelta = nullptr;
					continue;
				}
				lastDelta = delta;

				CString newname = CUnicodeUtils::GetUnicode(delta->new_file.path);
				CString oldname = CUnicodeUtils::GetUnicode(delta->old_file.path);

				CTGitPath path;
				path.SetFromGit(newname, &oldname);
				oldAction = m_Action;
				m_Action |= path.ParserAction(delta->status);
				path.m_ParentNo = parentId;

				if (delta->flags & GIT_DIFF_FLAG_BINARY)
				{
					path.m_StatAdd = _T("-");
					path.m_StatDel = _T("-");
				}
				else
				{
					size_t adds, dels;
					if (git_patch_line_stats(nullptr, &adds, &dels, patch) < 0)
						return -1;
					path.m_StatAdd.Format(_T("%d"), adds);
					path.m_StatDel.Format(_T("%d"), dels);
				}
				m_Files.AddPath(path);
			}
		}

		InterlockedExchange(&m_IsUpdateing, FALSE);
		InterlockedExchange(&m_IsFull, TRUE);
		return 0;
	}

	git->CheckAndInitDll();
	GIT_COMMIT commit = { 0 };
	GIT_COMMIT_LIST list;
	GIT_HASH parent;

	CAutoLocker lock(g_Git.m_critGitDllSec);

	try
	{
		if (git_get_commit_from_hash(&commit, m_CommitHash.m_hash))
			return -1;
	}
	catch (char *)
	{
		return -1;
	}

	int i = 0;
	git_get_commit_first_parent(&commit, &list);
	bool isRoot = (list == nullptr);

	while (git_get_commit_next_parent(&list, parent) == 0 || isRoot)
	{
		GIT_FILE file = 0;
		int count = 0;

		try
		{
			if (isRoot)
				git_root_diff(git->GetGitDiff(), m_CommitHash.m_hash, &file, &count, 1);
			else
				git_do_diff(git->GetGitDiff(), parent, commit.m_hash, &file, &count, 1);
		}
		catch (char*)
		{
			git_free_commit(&commit);
			return -1;
		}
		isRoot = false;

		CTGitPath path;
		CString strnewname;
		CString stroldname;

		for (int j = 0; j < count; ++j)
		{
			path.Reset();
			char* newname;
			char* oldname;

			strnewname.Empty();
			stroldname.Empty();

			int mode, IsBin, inc, dec;
			git_get_diff_file(git->GetGitDiff(), file, j, &newname, &oldname, &mode, &IsBin, &inc, &dec);

			git->StringAppend(&strnewname, (BYTE*)newname, CP_UTF8);
			git->StringAppend(&stroldname, (BYTE*)oldname, CP_UTF8);

			path.SetFromGit(strnewname, &stroldname);
			path.ParserAction((BYTE)mode);
			path.m_ParentNo = i;

			m_Action |= path.m_Action;

			if (IsBin)
			{
				path.m_StatAdd = _T("-");
				path.m_StatDel = _T("-");
			}
			else
			{
				path.m_StatAdd.Format(_T("%d"), inc);
				path.m_StatDel.Format(_T("%d"), dec);
			}
			m_Files.AddPath(path);
		}
		git_diff_flush(git->GetGitDiff());
		++i;
	}

	InterlockedExchange(&m_IsUpdateing, FALSE);
	InterlockedExchange(&m_IsFull, TRUE);
	git_free_commit(&commit);

	return 0;
}
