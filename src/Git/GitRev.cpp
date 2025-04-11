﻿// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2016, 2018-2021, 2023, 2025 - TortoiseGit

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
#include "GitRev.h"
#include "Git.h"
#include "gitdll.h"
#include "UnicodeUtils.h"

GitRev::GitRev()
{
}

GitRev::~GitRev()
{
}

void GitRev::Clear()
{
	this->m_ParentHash.clear();
	m_AuthorName.Empty();
	m_AuthorEmail.Empty();
	m_CommitterName.Empty();
	m_CommitterEmail.Empty();
	m_Body.Empty();
	m_Subject.Empty();
	m_CommitHash.Empty();
	m_sErr.Empty();
}

int GitRev::ParserParentFromCommit(const GIT_COMMIT* commit)
{
	ATLASSERT(commit);
	this->m_ParentHash.clear();
	GIT_COMMIT_LIST list;
	GIT_OBJECT_OID parent;

	git_get_commit_first_parent(commit,&list);
	 while (git_get_commit_next_parent(&list, &parent) == 0)
		m_ParentHash.emplace_back(CGitHash::FromRaw(parent.hash, parent.algo));
	return 0;
}

int GitRev::ParserFromCommit(const GIT_COMMIT *commit)
{
	ATLASSERT(commit);
	int encode =CP_UTF8;
	if(commit->m_Encode != 0 && commit->m_EncodeSize != 0)
		encode = CUnicodeUtils::GetCPCode(CUnicodeUtils::GetUnicodeLength(commit->m_Encode, commit->m_EncodeSize));

	this->m_CommitHash = CGitHash::FromRaw(commit->m_oid.hash, commit->m_oid.algo);

	this->m_AuthorDate = commit->m_Author.Date;
	this->m_AuthorEmail = CUnicodeUtils::GetUnicodeLength(commit->m_Author.Email, commit->m_Author.EmailSize, encode);
	this->m_AuthorName = CUnicodeUtils::GetUnicodeLength(commit->m_Author.Name, commit->m_Author.NameSize, encode);

	this->m_Body = CUnicodeUtils::GetUnicodeLength(commit->m_Body, commit->m_BodySize, encode);

	this->m_CommitterDate = commit->m_Committer.Date;
	this->m_CommitterEmail = CUnicodeUtils::GetUnicodeLength(commit->m_Committer.Email, commit->m_Committer.EmailSize, encode);
	this->m_CommitterName = CUnicodeUtils::GetUnicodeLength(commit->m_Committer.Name, commit->m_Committer.NameSize, encode);

	this->m_Subject = CUnicodeUtils::GetUnicodeLength(commit->m_Subject, commit->m_SubjectSize, encode);

	return 0;
}

int GitRev::ParserParentFromCommit(const git_commit* commit)
{
	ATLASSERT(commit);
	m_ParentHash.clear();
	const unsigned int parentCount = git_commit_parentcount(commit);
	for (unsigned int i = 0; i < parentCount; ++i)
		m_ParentHash.emplace_back(git_commit_parent_id(commit, i));

	return 0;
}

int GitRev::ParserFromCommit(const git_commit* commit)
{
	ATLASSERT(commit);
	Clear();

	int encode = CP_UTF8;

	const char* encodingstr = git_commit_message_encoding(commit);
	if (encodingstr)
		encode = CUnicodeUtils::GetCPCode(CUnicodeUtils::GetUnicode(encodingstr));

	m_CommitHash = git_commit_id(commit);

	const git_signature* author = git_commit_author(commit);
	m_AuthorDate = author->when.time;
	m_AuthorEmail = CUnicodeUtils::GetUnicode(author->email, encode);
	m_AuthorName = CUnicodeUtils::GetUnicode(author->name, encode);

	const git_signature* committer = git_commit_committer(commit);
	m_CommitterDate = committer->when.time;
	m_CommitterEmail = CUnicodeUtils::GetUnicode(committer->email, encode);
	m_CommitterName = CUnicodeUtils::GetUnicode(committer->name, encode);

	const char* msg = git_commit_message_raw(commit);
	const char* body = strchr(msg, '\n');
	if (!body)
		m_Subject = CUnicodeUtils::GetUnicode(msg, encode);
	else
	{
		m_Subject = CUnicodeUtils::GetUnicodeLengthSizeT(msg, body - msg, encode);
		m_Body = CUnicodeUtils::GetUnicode(body + 1, encode);
	}

	return 0;
}

int GitRev::GetCommitFromHash(git_repository* repo, const CGitHash& hash)
{
	ATLASSERT(repo);
	CAutoCommit commit;
	if (git_commit_lookup(commit.GetPointer(), repo, hash) < 0)
	{
		m_sErr = CGit::GetLibGit2LastErr();
		return -1;
	}

	return ParserFromCommit(commit);
}

int GitRev::GetCommit(git_repository* repo, const CString& refname)
{
	ATLASSERT(repo);
	if (refname.GetLength() >= 8 && wcsncmp(refname, GitRev::GetWorkingCopyRef(static_cast<GIT_HASH_TYPE>(git_repository_oid_type(repo))), refname.GetLength()) == 0)
	{
		Clear();
		m_Subject = L"Working Tree";
		return 0;
	}

	CGitHash hash;
	if (CGit::GetHash(repo, hash, refname + L"^{}")) // add ^{} in order to dereference signed tags
	{
		m_sErr = CGit::GetLibGit2LastErr();
		return -1;
	}

	return GetCommitFromHash(repo, hash);
}

#if DEBUG
void GitRev::DbgPrint()
{
	ATLTRACE(L"Commit %s\r\n", static_cast<LPCWSTR>(this->m_CommitHash.ToString()));
	for (unsigned int i = 0; i < this->m_ParentHash.size(); ++i)
	{
		ATLTRACE(L"Parent %i %s\r\n", i, static_cast<LPCWSTR>(m_ParentHash[i].ToString()));
	}
	ATLTRACE(L"\r\n\r\n");
}
#endif

int GitRev::GetParentFromHash(const CGitHash& hash)
{
	CAutoLocker lock(g_Git.m_critGitDllSec);

	GIT_COMMIT commit;
	try
	{
		g_Git.CheckAndInitDll();

		if (git_get_commit_from_hash(&commit, hash.ToRaw(), hash.HashType()))
		{
			m_sErr = L"git_get_commit_from_hash failed for " + hash.ToString();
			return -1;
		}
	}
	catch (const char* msg)
	{
		m_sErr = L"Could not get parents of commit \"" + hash.ToString() + L"\".\nlibgit reports:\n" + CUnicodeUtils::GetUnicode(msg);
		return -1;
	}

	this->ParserParentFromCommit(&commit);
	git_free_commit(&commit);

	this->m_CommitHash=hash;

	return 0;
}

int GitRev::GetCommitFromHash(const CGitHash& hash)
{
	CAutoLocker lock(g_Git.m_critGitDllSec);

	g_Git.CheckAndInitDll();

	return GetCommitFromHash_withoutLock(hash);
}

int GitRev::GetCommitFromHash_withoutLock(const CGitHash& hash)
{
	GIT_COMMIT commit;
	try
	{
		if (git_get_commit_from_hash(&commit, hash.ToRaw(), hash.HashType()))
		{
			m_sErr = L"git_get_commit_from_hash failed for " + hash.ToString();
			return -1;
		}
	}
	catch (const char * msg)
	{
		m_sErr = L"Could not get commit \"" + hash.ToString() + L"\".\nlibgit reports:\n" + CUnicodeUtils::GetUnicode(msg);
		return -1;
	}

	this->ParserFromCommit(&commit);
	git_free_commit(&commit);

	m_sErr.Empty();

	return 0;
}

int GitRev::GetCommit(const CString& refname)
{
	if (g_Git.UsingLibGit2(CGit::GIT_CMD_GET_COMMIT))
	{
		CAutoRepository repo(g_Git.GetGitRepository());
		if (!repo)
		{
			m_sErr = g_Git.GetLibGit2LastErr();
			return -1;
		}
		return GetCommit(repo, refname);
	}

	CAutoLocker lock(g_Git.m_critGitDllSec);

	try
	{
		g_Git.CheckAndInitDll();
	}
	catch (const char* msg)
	{
		m_sErr = L"Could not initiate libgit.\nlibgit reports:\n" + CUnicodeUtils::GetUnicode(msg);
		return -1;
	}

	if(refname.GetLength() >= 8)
		if (refname.GetLength() >= 8 && wcsncmp(refname, GitRev::GetWorkingCopyRef(g_Git.GetCurrentRepoHashType()), refname.GetLength()) == 0)
		{
			this->m_CommitHash.Empty();
			this->m_Subject = L"Working Tree";
			m_sErr.Empty();
			return 0;
		}
	CStringA rev = CUnicodeUtils::GetUTF8(g_Git.FixBranchName(refname + L"^{}")); // add ^{} in order to dereference signed tags
	GIT_OBJECT_OID oid;

	try
	{
		if (git_get_oid(rev.GetBuffer(), &oid))
		{
			m_sErr = L"Could not get OID of ref \"" + g_Git.FixBranchName(refname);
			return -1;
		}
	}
	catch (const char* msg)
	{
		m_sErr = L"Could not get OID of ref \"" + g_Git.FixBranchName(refname) + L"\".\nlibgit reports:\n" + CUnicodeUtils::GetUnicode(msg);
		return -1;
	}

	CGitHash hash = CGitHash::FromRaw(oid.hash, oid.algo);
	return GetCommitFromHash_withoutLock(hash);
}

void GitRev::ApplyMailmap(const CGitMailmap& mailmap)
{
	if (!mailmap)
		return;
	mailmap.Translate(m_AuthorName, m_AuthorEmail);
	mailmap.Translate(m_CommitterName, m_CommitterEmail);
}
