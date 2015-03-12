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
