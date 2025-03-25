// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2020, 2024-2025 - TortoiseGit

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
#include "gittype.h"
#include "Git.h"
#include "GitRevRefBrowser.h"
#include "StringUtils.h"
#include "GitMailmap.h"

GitRevRefBrowser::GitRevRefBrowser() : GitRev()
{
}

void GitRevRefBrowser::Clear()
{
	GitRev::Clear();
	m_Description.Empty();
	m_UpstreamRef.Empty();
}

int GitRevRefBrowser::GetGitRevRefMap(MAP_REF_GITREVREFBROWSER& map, int mergefilter, CString& err, std::function<bool(const CString& refName)> filterCallback)
{
	err.Empty();
	MAP_STRING_STRING descriptions;
	g_Git.GetBranchDescriptions(descriptions);

	CGitMailmap mailmap;
	if (g_Git.UsingLibGit2(CGit::GIT_CMD_FOREACHREF) && mergefilter <= 0) // libgit2 implementation is significantly slower because of "git_graph_descendant_of", hence, only use it when merge status is not a filter
	{
		CAutoRepository repo = g_Git.GetGitRepository();
		if (!repo)
		{
			err = g_Git.GetLibGit2LastErr();
			return -1;
		}

		CAutoReference head;
		if (mergefilter > 0)
		{
			if (const auto ret = git_repository_head(head.GetPointer(), repo); ret == GIT_EUNBORNBRANCH)
				mergefilter = 0;
			else if (ret < 0) {
				err = g_Git.GetLibGit2LastErr();
				return -1;
			}
		}

		CAutoReferenceIterator iter;
		if (git_reference_iterator_new(iter.GetPointer(), repo) < 0)
		{
			err = g_Git.GetLibGit2LastErr();
			return -1;
		}

		CAutoReference ref;
		int error;
		while (!(error = git_reference_next(ref.GetPointer(), iter)))
		{
			CString refName = CUnicodeUtils::GetUnicode(git_reference_name(ref));
			if (filterCallback && !filterCallback(refName))
				continue;

			if (git_reference_type(ref) == GIT_REFERENCE_SYMBOLIC)
			{
				CAutoReference target;
				if (git_reference_resolve(target.GetPointer(), ref))
				{
					err = g_Git.GetLibGit2LastErr();
					return -1;
				}
				ref.Swap(target);
			}

			CAutoTag tag;
			if (git_reference_is_tag(ref))
				git_tag_lookup(tag.GetPointer(), repo, git_reference_target(ref));

			if (mergefilter > 0)
			{
				const git_oid* oid = git_reference_target(ref);
				if (tag)
					oid = git_tag_target_id(tag);

				const int ret = git_graph_descendant_of(repo, git_reference_target(head), oid);
				if (ret < 0)
				{
					err = g_Git.GetLibGit2LastErr();
					return -1;
				}
				if (mergefilter == 1 && ret == 0 && git_oid_cmp(oid, git_reference_target(head)) != 0)
					continue;
				if (mergefilter == 2 && (ret == 1 || git_oid_cmp(oid, git_reference_target(head)) == 0))
					continue;
			}

			GitRevRefBrowser entry;
			entry.m_CommitHash = git_reference_target(ref);
			if (git_reference_is_tag(ref) && tag)
			{
				const char* msg = git_tag_message(tag);
				if (const char* body = strchr(msg, '\n'); !body)
					entry.m_Subject = CUnicodeUtils::GetUnicode(msg);
				else
					entry.m_Subject = CUnicodeUtils::GetUnicodeLengthSizeT(msg, body - msg);
				auto tagger = git_tag_tagger(tag);
				entry.m_CommitterName = CUnicodeUtils::GetUnicode(tagger->name);
				entry.m_CommitterEmail = CUnicodeUtils::GetUnicode(tagger->email);
				entry.m_CommitterDate = tagger->when.time;
				if (mailmap)
					entry.m_CommitterEmail = mailmap.TranslateAuthor(entry.m_CommitterEmail, entry.m_AuthorEmail);
				entry.m_AuthorName = entry.m_CommitterName;
				entry.m_AuthorEmail = entry.m_CommitterEmail;
				entry.m_AuthorDate = entry.m_CommitterDate;

				map.emplace(refName, entry);

				continue;
			}

			CAutoCommit commit;
			if (git_commit_lookup(commit.GetPointer(), repo, git_reference_target(ref)) < 0)
			{
				err = g_Git.GetLibGit2LastErr();
				return -1;
			}

			auto author = git_commit_author(commit);
			auto committer = git_commit_committer(commit);
			entry.m_Subject = CUnicodeUtils::GetUnicode(git_commit_summary(commit));
			entry.m_AuthorName = CUnicodeUtils::GetUnicode(author->name);
			entry.m_AuthorEmail = CUnicodeUtils::GetUnicode(author->email);
			entry.m_AuthorDate = author->when.time;
			entry.m_CommitterName = CUnicodeUtils::GetUnicode(committer->name);
			entry.m_CommitterEmail = CUnicodeUtils::GetUnicode(committer->email);
			entry.m_CommitterDate = committer->when.time;
			if (mailmap)
			{
				entry.m_AuthorName = mailmap.TranslateAuthor(entry.m_AuthorName, entry.m_AuthorEmail);
				entry.m_CommitterName = mailmap.TranslateAuthor(entry.m_CommitterName, entry.m_CommitterEmail);
			}

			if (git_reference_is_branch(ref))
			{
				entry.m_Description = descriptions[refName.Mid(static_cast<int>(wcslen(L"refs/heads/")))];

				CAutoBuf buf;
				if (const auto ret = git_branch_upstream_name(buf, repo, git_reference_name(ref)); ret == 0)
					entry.m_UpstreamRef = CUnicodeUtils::GetUnicodeLengthSizeT(buf->ptr, buf->size);
				else if (ret != GIT_ENOTFOUND)
				{
					err = g_Git.GetLibGit2LastErr();
					return -1;
				}
			}

			map.emplace(refName, entry);
		}
		if (error != GIT_ITEROVER)
		{
			err = g_Git.GetLibGit2LastErr();
			return -1;
		}

		return 0;
	}

	CString args;
	switch (mergefilter)
	{
	case 1:
		args = L" --merged HEAD";
		break;
	case 2:
		args = L" --no-merged HEAD";
		break;
	}

	CString cmd;
	cmd.Format(L"git.exe for-each-ref%s --format=\"%%(refname)%%04 %%(objectname)%%04 %%(upstream)%%04 %%(subject)%%04 %%(authorname)%%04 %%(authoremail)%%04 %%(authordate:raw)%%04 %%(committername)%%04 %%(committeremail)%%04 %%(committerdate:raw)%%04%%(creator)%%04 %%(creatordate:raw)%%03\"", static_cast<LPCWSTR>(args));
	CString allRefs;
	if (g_Git.Run(cmd, &allRefs, &err, CP_UTF8))
		return -1;

	int linePos = 0;
	CString singleRef;
	while (!(singleRef = allRefs.Tokenize(L"\03", linePos)).IsEmpty())
	{
		singleRef.TrimLeft(L"\r\n");
		int valuePos = 0;
		CString refName = singleRef.Tokenize(L"\04", valuePos);

		if (refName.IsEmpty() || (filterCallback && !filterCallback(refName)))
			continue;

		GitRevRefBrowser ref;
		ref.m_CommitHash = CGitHash::FromHexStr(singleRef.Tokenize(L"\04", valuePos).Trim()); if (valuePos < 0) continue;
		ref.m_UpstreamRef = singleRef.Tokenize(L"\04", valuePos).Trim(); if (valuePos < 0) continue;
		ref.m_Subject = singleRef.Tokenize(L"\04", valuePos).Trim(); if (valuePos < 0) continue;
		ref.m_AuthorName = singleRef.Tokenize(L"\04", valuePos).Trim(); if (valuePos < 0) continue;
		CString email = singleRef.Tokenize(L"\04", valuePos).Trim().Trim(L"<>"); if (valuePos < 0) continue;
		CString date = singleRef.Tokenize(L"\04", valuePos).Trim();
		ref.m_AuthorDate = _wtoll(date);
		ref.m_CommitterName = singleRef.Tokenize(L"\04", valuePos).Trim(); if (valuePos < 0) continue;
		CString committerEmail = singleRef.Tokenize(L"\04", valuePos).Trim().Trim(L"<>"); if (valuePos < 0) continue;
		ref.m_CommitterDate = _wtoll(singleRef.Tokenize(L"\04", valuePos).Trim());
		if (ref.m_AuthorName.IsEmpty())
		{
			ref.m_AuthorName = singleRef.Tokenize(L"\04", valuePos).Trim(); if (valuePos < 0) continue;
			email = ref.m_AuthorName.Mid(ref.m_AuthorName.Find(L" <") + static_cast<int>(wcslen(L" <")));
			email.Truncate(max(0, email.Find(L'>')));
			ref.m_AuthorName.Truncate(max(0, ref.m_AuthorName.Find(L" <")));
			date = singleRef.Tokenize(L"\04", valuePos).Trim();
			ref.m_AuthorDate = _wtoll(date);
			if (ref.m_CommitterName.IsEmpty())
			{
				ref.m_CommitterName = ref.m_AuthorName;
				committerEmail = email;
				ref.m_CommitterDate = ref.m_AuthorDate;
			}
		}
		if (mailmap)
		{
			ref.m_AuthorName = mailmap.TranslateAuthor(ref.m_AuthorName, email);
			ref.m_CommitterName = mailmap.TranslateAuthor(ref.m_CommitterName, committerEmail);
		}

		if (CStringUtils::StartsWith(refName, L"refs/heads/"))
			ref.m_Description = descriptions[refName.Mid(static_cast<int>(wcslen(L"refs/heads/")))];

		map.emplace(refName, ref);
	}

	return 0;
}
