// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2019 - TortoiseGit

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
#include "GitRevLoglist.h"
#include "Git.h"
#include "gitdll.h"
#include "UnicodeUtils.h"
#include <sys/stat.h>

typedef CComCritSecLock<CComCriticalSection> CAutoLocker;

GitRevLoglist::GitRevLoglist(void) : GitRev()
, m_Action(0)
, m_RebaseAction(0)
, m_IsFull(FALSE)
, m_IsUpdateing(FALSE)
, m_IsCommitParsed(FALSE)
, m_IsDiffFiles(FALSE)
, m_CallDiffAsync(nullptr)
, m_IsSimpleListReady(FALSE)
, m_Mark(0)
{
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
		if (git_commit_lookup(commit.GetPointer(), repo, m_CommitHash) < 0)
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

		std::sort(m_SimpleFileList.begin(), m_SimpleFileList.end());
		m_SimpleFileList.erase(std::unique(m_SimpleFileList.begin(), m_SimpleFileList.end()), m_SimpleFileList.end());

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
		if (git_get_commit_from_hash(&commit, m_CommitHash.ToRaw()))
			return -1;
	}
	catch (char *)
	{
		return -1;
	}

	git_get_commit_first_parent(&commit, &list);
	bool isRoot = git_commit_is_root(&commit) == 0;
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
			int status, isBin, inc, dec, isDir;

			try
			{
				git_get_diff_file(git->GetGitSimpleListDiff(), file, j, &newname, &oldname, &isDir, &status, &isBin, &inc, &dec);
			}
			catch (char *)
			{
				return -1;
			}

			m_SimpleFileList.push_back(CUnicodeUtils::GetUnicode(newname));
		}

		git_diff_flush(git->GetGitSimpleListDiff());
	}

	std::sort(m_SimpleFileList.begin(), m_SimpleFileList.end());
	m_SimpleFileList.erase(std::unique(m_SimpleFileList.begin(), m_SimpleFileList.end()), m_SimpleFileList.end());

	InterlockedExchange(&m_IsUpdateing, FALSE);
	InterlockedExchange(&m_IsSimpleListReady, TRUE);
	if (m_GitCommit.m_pGitCommit != commit.m_pGitCommit)
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
		{
			m_sErr = CGit::GetLibGit2LastErr();
			return -1;
		}
		CAutoCommit commit;
		if (git_commit_lookup(commit.GetPointer(), repo, m_CommitHash) < 0)
		{
			m_sErr = CGit::GetLibGit2LastErr();
			return -1;
		}

		CAutoTree commitTree;
		if (git_commit_tree(commitTree.GetPointer(), commit) < 0)
		{
			m_sErr = CGit::GetLibGit2LastErr();
			return -1;
		}

		bool isRoot = git_commit_parentcount(commit) == 0;
		for (unsigned int parentId = 0; isRoot || parentId < git_commit_parentcount(commit); ++parentId)
		{
			CAutoTree parentTree;
			if (!isRoot)
			{
				CAutoCommit parentCommit;
				if (git_commit_parent(parentCommit.GetPointer(), commit, parentId) < 0)
				{
					m_sErr = CGit::GetLibGit2LastErr();
					return -1;
				}

				if (git_commit_tree(parentTree.GetPointer(), parentCommit) < 0)
				{
					m_sErr = CGit::GetLibGit2LastErr();
					return -1;
				}
			}
			isRoot = false;

			CAutoDiff diff;
			if (git_diff_tree_to_tree(diff.GetPointer(), repo, parentTree, commitTree, nullptr) < 0)
			{
				m_sErr = CGit::GetLibGit2LastErr();
				return -1;
			}

			// cf. CGit::GetGitDiff()
			if (CGit::ms_iSimilarityIndexThreshold > 0)
			{
				git_diff_find_options diffopts = GIT_DIFF_FIND_OPTIONS_INIT;
				diffopts.flags = GIT_DIFF_FIND_COPIES | GIT_DIFF_FIND_RENAMES;
				diffopts.rename_threshold = diffopts.copy_threshold = static_cast<uint16_t>(CGit::ms_iSimilarityIndexThreshold);
				if (git_diff_find_similar(diff, &diffopts) < 0)
				{
					m_sErr = CGit::GetLibGit2LastErr();
					return -1;
				}
			}

			const git_diff_delta* lastDelta = nullptr;
			int oldAction = 0;
			size_t deltas = git_diff_num_deltas(diff);
			for (size_t i = 0; i < deltas; ++i)
			{
				CAutoPatch patch;
				if (git_patch_from_diff(patch.GetPointer(), diff, i) < 0)
				{
					m_sErr = CGit::GetLibGit2LastErr();
					return -1;
				}

				const git_diff_delta* delta = git_patch_get_delta(patch);

				int isDir = (delta->new_file.mode & S_IFDIR) == S_IFDIR;
				if (delta->status == GIT_DELTA_DELETED)
					isDir = (delta->old_file.mode & S_IFDIR) == S_IFDIR;

				if (lastDelta && strcmp(lastDelta->new_file.path, delta->new_file.path) == 0 && (lastDelta->status == GIT_DELTA_DELETED && delta->status == GIT_DELTA_ADDED || delta->status == GIT_DELTA_DELETED && lastDelta->status == GIT_DELTA_ADDED))
				{
					// rename handling
					CTGitPath path = m_Files[m_Files.GetCount() - 1];
					m_Files.RemoveItem(path);
					if (path.IsDirectory() && !isDir)
						path.UnsetDirectoryStatus();
					path.m_StatAdd = L"-";
					path.m_StatDel = L"-";
					path.m_Action = CTGitPath::LOGACTIONS_MODIFIED;
					m_Action = oldAction | CTGitPath::LOGACTIONS_MODIFIED;
					m_Files.AddPath(path);
					lastDelta = nullptr;
					continue;
				}
				lastDelta = delta;

				CString newname = CUnicodeUtils::GetUnicode(delta->new_file.path);
				CTGitPath path;
				if (delta->new_file.path == delta->old_file.path)
					path.SetFromGit(newname, isDir != FALSE);
				else
				{
					CString oldname = CUnicodeUtils::GetUnicode(delta->old_file.path);
					path.SetFromGit(newname, &oldname, &isDir);
				}
				oldAction = m_Action;
				m_Action |= path.ParserAction(delta->status);
				path.m_ParentNo = parentId;

				if (delta->flags & GIT_DIFF_FLAG_BINARY)
				{
					path.m_StatAdd = L"-";
					path.m_StatDel = L"-";
				}
				else
				{
					size_t adds, dels;
					if (git_patch_line_stats(nullptr, &adds, &dels, patch) < 0)
					{
						m_sErr = CGit::GetLibGit2LastErr();
						return -1;
					}
					path.m_StatAdd.Format(L"%zu", adds);
					path.m_StatDel.Format(L"%zu", dels);
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
		if (git_get_commit_from_hash(&commit, m_CommitHash.ToRaw()))
		{
			m_sErr = L"git_get_commit_from_hash failed for " + m_CommitHash.ToString();
			return -1;
		}
	}
	catch (const char* msg)
	{
		m_sErr = L"Could not get commit \"" + m_CommitHash.ToString() + L"\".\nlibgit reports:\n" + CString(msg);
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
				git_root_diff(git->GetGitDiff(), m_CommitHash.ToRaw(), &file, &count, 1);
			else
				git_do_diff(git->GetGitDiff(), parent, commit.m_hash, &file, &count, 1);
		}
		catch (const char* msg)
		{
			m_sErr = L"Could do diff for \"" + m_CommitHash.ToString() + L"\".\nlibgit reports:\n" + CString(msg);
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

			int status, isBin, inc, dec, isDir;
			git_get_diff_file(git->GetGitDiff(), file, j, &newname, &oldname, &isDir, &status, &isBin, &inc, &dec);

			CGit::StringAppend(&strnewname, newname, CP_UTF8);
			if (strcmp(newname, oldname) == 0)
				path.SetFromGit(strnewname, isDir != FALSE);
			else
			{
				CGit::StringAppend(&stroldname, oldname, CP_UTF8);
				path.SetFromGit(strnewname, &stroldname, &isDir);
			}
			path.ParserAction(static_cast<BYTE>(status));
			path.m_ParentNo = i;

			m_Action |= path.m_Action;

			if (isBin)
			{
				path.m_StatAdd = L"-";
				path.m_StatDel = L"-";
			}
			else
			{
				path.m_StatAdd.Format(L"%d", inc);
				path.m_StatDel.Format(L"%d", dec);
			}
			m_Files.AddPath(path);
		}
		git_diff_flush(git->GetGitDiff());
		++i;
	}

	InterlockedExchange(&m_IsUpdateing, FALSE);
	InterlockedExchange(&m_IsFull, TRUE);
	if (m_GitCommit.m_pGitCommit != commit.m_pGitCommit)
		git_free_commit(&commit);

	return 0;
}

int GitRevLoglist::GetRefLog(const CString& ref, std::vector<GitRevLoglist>& refloglist, CString& error)
{
	refloglist.clear();
	if (g_Git.m_IsUseLibGit2)
	{
		CAutoRepository repo(g_Git.GetGitRepository());
		if (!repo)
		{
			error = g_Git.GetLibGit2LastErr();
			return -1;
		}

		CAutoReflog reflog;
		if (git_reflog_read(reflog.GetPointer(), repo, CUnicodeUtils::GetUTF8(ref)) < 0)
		{
			error = g_Git.GetLibGit2LastErr();
			return -1;
		}

		for (size_t i = 0; i < git_reflog_entrycount(reflog); ++i)
		{
			const git_reflog_entry *entry = git_reflog_entry_byindex(reflog, i);
			if (!entry)
				continue;

			GitRevLoglist rev;
			rev.m_CommitHash = git_reflog_entry_id_new(entry);
			rev.m_Ref.Format(L"%s@{%zu}", static_cast<LPCTSTR>(ref), i);
			rev.GetCommitterDate() = CTime(git_reflog_entry_committer(entry)->when.time);
			if (git_reflog_entry_message(entry) != nullptr)
			{
				CString one = CUnicodeUtils::GetUnicode(git_reflog_entry_message(entry));
				int message = one.Find(L": ");
				if (message > 0)
				{
					rev.m_RefAction = one.Left(message);
					rev.GetSubject() = one.Mid(message + 2);
				}
				else
					rev.m_RefAction = one;
			}
			refloglist.push_back(rev);
		}
		return 0;
	}
	else if (g_Git.m_IsUseGitDLL)
	{
		g_Git.CheckAndInitDll();
		std::vector<GitRevLoglist> tmp;
		// no error checking, because the only error which could occur is file not found
		git_for_each_reflog_ent(CUnicodeUtils::GetUTF8(ref), [](struct GIT_OBJECT_OID* /*old_oid*/, struct GIT_OBJECT_OID* new_oid, const char* /*committer*/, unsigned long long time, int /*sz*/, const char* msg, void* data)
		{
			auto vector = static_cast<std::vector<GitRevLoglist>*>(data);
			GitRevLoglist rev;
			rev.m_CommitHash = CGitHash::FromRaw(new_oid->hash);
			rev.GetCommitterDate() = CTime(time);

			CString one = CUnicodeUtils::GetUnicode(msg);
			int message = one.Find(L": ");
			if (message > 0)
			{
				rev.m_RefAction = one.Left(message);
				rev.GetSubject() = one.Mid(message + 2).TrimRight(L'\n');
			}
			else
				rev.m_RefAction = one.TrimRight(L'\n');
			vector->push_back(rev);

			return 0;
		}, &tmp);

		for (size_t i = tmp.size(), id = 0; i > 0; --i, ++id)
		{
			GitRevLoglist rev = tmp[i - 1];
			rev.m_Ref.Format(L"%s@{%zu}", static_cast<LPCTSTR>(ref), id);
			refloglist.push_back(rev);
		}
		return 0;
	}

	CString dotGitDir;
	if (!GitAdminDir::GetAdminDirPath(g_Git.m_CurrentDir, dotGitDir))
	{
		error = L".git directory not found";
		return -1;
	}

	error.Empty();
	// git.exe would fail with "branch not known"
	if (!PathFileExists(dotGitDir + ref))
		return 0;

	CString cmd, out;
	cmd.Format(L"git.exe reflog show --pretty=\"%%H %%gD: %%gs\" --date=raw %s", static_cast<LPCTSTR>(ref));
	if (g_Git.Run(cmd, &out, &error, CP_UTF8))
		return -1;

	int i = 0;
	CString prefix = ref + L"@{";
	int pos = 0;
	while (pos >= 0)
	{
		CString one = out.Tokenize(L"\n", pos);
		int refPos = one.Find(L' ');
		if (refPos < 0)
			continue;

		GitRevLoglist rev;
		rev.m_CommitHash = CGitHash::FromHexStrTry(one.Left(refPos));
		rev.m_Ref.Format(L"%s@{%d}", static_cast<LPCTSTR>(ref), i++);
		int prefixPos = one.Find(prefix, refPos + 1);
		if (prefixPos != refPos + 1)
			continue;

		int spacePos = one.Find(L' ', prefixPos + prefix.GetLength() + 1);
		if (spacePos < 0)
			continue;

		CString timeStr = one.Mid(prefixPos + prefix.GetLength(), spacePos - prefixPos - prefix.GetLength());
		rev.GetCommitterDate() = CTime(_wtoi(timeStr));
		int action = one.Find(L"}: ", spacePos + 1);
		if (action > 0)
		{
			action += 2;
			int message = one.Find(L": ", action);
			if (message > 0)
			{
				rev.m_RefAction = one.Mid(action + 1, message - action - 1);
				rev.GetSubject() = one.Right(one.GetLength() - message - 2);
			}
			else
				rev.m_RefAction = one.Mid(action + 1);
		}

		refloglist.push_back(rev);
	}
	return 0;
}
