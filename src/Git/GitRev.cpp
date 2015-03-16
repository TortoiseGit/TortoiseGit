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
#include "GitRev.h"
#include "Git.h"
#include "gitdll.h"
#include "UnicodeUtils.h"

typedef CComCritSecLock<CComCriticalSection> CAutoLocker;

GitRev::GitRev(void)
{
}

GitRev::~GitRev(void)
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

int GitRev::ParserParentFromCommit(GIT_COMMIT *commit)
{
	this->m_ParentHash.clear();
	GIT_COMMIT_LIST list;
	GIT_HASH   parent;

	git_get_commit_first_parent(commit,&list);
	while(git_get_commit_next_parent(&list,parent)==0)
	{
		m_ParentHash.push_back(CGitHash((char *)parent));
	}
	return 0;
}

int GitRev::ParserFromCommit(GIT_COMMIT *commit)
{
	int encode =CP_UTF8;

	if(commit->m_Encode != 0 && commit->m_EncodeSize != 0)
	{
		CString str;
		CGit::StringAppend(&str, (BYTE*)commit->m_Encode, CP_UTF8, commit->m_EncodeSize);
		encode = CUnicodeUtils::GetCPCode(str);
	}

	this->m_CommitHash = commit->m_hash;

	this->m_AuthorDate = commit->m_Author.Date;

	this->m_AuthorEmail.Empty();
	CGit::StringAppend(&m_AuthorEmail, (BYTE*)commit->m_Author.Email, encode, commit->m_Author.EmailSize);

	this->m_AuthorName.Empty();
	CGit::StringAppend(&m_AuthorName, (BYTE*)commit->m_Author.Name, encode, commit->m_Author.NameSize);

	this->m_Body.Empty();
	CGit::StringAppend(&m_Body, (BYTE*)commit->m_Body, encode, commit->m_BodySize);

	this->m_CommitterDate = commit->m_Committer.Date;

	this->m_CommitterEmail.Empty();
	CGit::StringAppend(&m_CommitterEmail, (BYTE*)commit->m_Committer.Email, encode, commit->m_Committer.EmailSize);

	this->m_CommitterName.Empty();
	CGit::StringAppend(&m_CommitterName, (BYTE*)commit->m_Committer.Name, encode, commit->m_Committer.NameSize);

	this->m_Subject.Empty();
	CGit::StringAppend(&m_Subject, (BYTE*)commit->m_Subject,encode,commit->m_SubjectSize);

	return 0;
}

int GitRev::ParserParentFromCommit(const git_commit* commit)
{
	m_ParentHash.clear();
	unsigned int parentCount = git_commit_parentcount(commit);
	for (unsigned int i = 0; i < parentCount; ++i)
		m_ParentHash.push_back(CGitHash((char*)git_commit_parent_id(commit, i)->id));

	return 0;
}

int GitRev::ParserFromCommit(const git_commit* commit)
{
	Clear();

	int encode = CP_UTF8;

	const char* encodingstr = git_commit_message_encoding(commit);
	if (encodingstr)
		encode = CUnicodeUtils::GetCPCode(CUnicodeUtils::GetUnicode(encodingstr));

	m_CommitHash = git_commit_id(commit)->id;

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
		m_Subject = CUnicodeUtils::GetUnicode(CStringA(msg, (int)(body - msg)), encode);
		m_Body = CUnicodeUtils::GetUnicode(body + 1, encode);
	}

	return 0;
}

int GitRev::GetCommitFromHash(git_repository* repo, const CGitHash& hash)
{
	CAutoCommit commit;
	if (git_commit_lookup(commit.GetPointer(), repo, (const git_oid*)hash.m_hash) < 0)
	{
		m_sErr = CGit::GetLibGit2LastErr();
		return -1;
	}

	return ParserFromCommit(commit);
}

int GitRev::GetCommit(git_repository* repo, const CString& refname)
{
	if (refname.GetLength() >= 8 && refname.Find(_T("00000000")) == 0)
	{
		Clear();
		m_Subject = _T("Working Copy");
		return 0;
	}

	CGitHash hash;
	if (CGit::GetHash(repo, hash, refname))
	{
		m_sErr = CGit::GetLibGit2LastErr();
		return -1;
	}

	return GetCommitFromHash(repo, hash);
}

void GitRev::DbgPrint()
{
	ATLTRACE(_T("Commit %s\r\n"), this->m_CommitHash.ToString());
	for (unsigned int i = 0; i < this->m_ParentHash.size(); ++i)
	{
		ATLTRACE(_T("Parent %i %s"), i, m_ParentHash[i].ToString());
	}
	ATLTRACE(_T("\n"));
}

int GitRev::GetParentFromHash(CGitHash &hash)
{
	CAutoLocker lock(g_Git.m_critGitDllSec);

	g_Git.CheckAndInitDll();

	GIT_COMMIT commit;
	try
	{
		if (git_get_commit_from_hash(&commit, hash.m_hash))
		{
			m_sErr = _T("git_get_commit_from_hash failed for ") + hash.ToString();
			return -1;
		}
	}
	catch (char* msg)
	{
		m_sErr = _T("Could not get parents of commit \"") + hash.ToString() + _T("\".\nlibgit reports:\n") + CString(msg);
		return -1;
	}

	this->ParserParentFromCommit(&commit);
	git_free_commit(&commit);

	this->m_CommitHash=hash;

	return 0;
}

int GitRev::GetCommitFromHash(CGitHash &hash)
{
	CAutoLocker lock(g_Git.m_critGitDllSec);

	g_Git.CheckAndInitDll();

	return GetCommitFromHash_withoutLock(hash);
}

int GitRev::GetCommitFromHash_withoutLock(CGitHash &hash)
{
	GIT_COMMIT commit;
	try
	{
		if (git_get_commit_from_hash(&commit, hash.m_hash))
		{
			m_sErr = _T("git_get_commit_from_hash failed for ") + hash.ToString();
			return -1;
		}
	}
	catch (char * msg)
	{
		m_sErr = _T("Could not get commit \"") + hash.ToString() + _T("\".\nlibgit reports:\n") + CString(msg);
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

	g_Git.CheckAndInitDll();

	if(refname.GetLength() >= 8)
		if(refname.Find(_T("00000000")) == 0)
		{
			this->m_CommitHash.Empty();
			this->m_Subject=_T("Working Copy");
			m_sErr.Empty();
			return 0;
		}
	CStringA rev;
	rev= CUnicodeUtils::GetUTF8(g_Git.FixBranchName(refname));
	GIT_HASH sha;

	try
	{
		if (git_get_sha1(rev.GetBuffer(), sha))
		{
			m_sErr = _T("Could not get SHA-1 of ref \"") + g_Git.FixBranchName(refname);
			return -1;
		}
	}
	catch (char * msg)
	{
		m_sErr = _T("Could not get SHA-1 of ref \"") + g_Git.FixBranchName(refname) + _T("\".\nlibgit reports:\n") + CString(msg);
		return -1;
	}

	CGitHash hash((char*)sha);
	return GetCommitFromHash_withoutLock(hash);
}
