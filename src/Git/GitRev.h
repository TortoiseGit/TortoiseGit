// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2017, 2019 - TortoiseGit

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

#pragma once
#include "gittype.h"
#include "AtlTime.h"
#include "GitHash.h"
#include "GitDll.h"

typedef std::vector<CGitHash> GIT_REV_LIST;

#define LOG_REV_AUTHOR_NAME		L'0'
#define LOG_REV_AUTHOR_EMAIL	L'1'
#define LOG_REV_AUTHOR_DATE		L'2'
#define LOG_REV_COMMIT_NAME		L'3'
#define LOG_REV_COMMIT_EMAIL	L'4'
#define LOG_REV_COMMIT_DATE		L'5'
#define LOG_REV_COMMIT_SUBJECT	L'6'
#define LOG_REV_COMMIT_BODY		L'7'
#define LOG_REV_COMMIT_HASH		L'8'
#define LOG_REV_COMMIT_PARENT	L'9'
#define LOG_REV_COMMIT_FILE		L'A'
#define LOG_REV_ITEM_BEGIN		L'B'
#define LOG_REV_ITEM_END		L'C'

class GitRev
{
protected:
	CString	m_AuthorName;
	CString	m_AuthorEmail;
	CTime	m_AuthorDate;
	CString	m_CommitterName;
	CString	m_CommitterEmail;
	CTime	m_CommitterDate;
	CString	m_Subject;
	CString	m_Body;

	CString m_sErr;

public:
	GitRev(void);
	CString GetAuthorName() const
	{
		return m_AuthorName;
	}

	CString GetAuthorEmail() const
	{
		return m_AuthorEmail;
	}

	CTime GetAuthorDate() const
	{
		return m_AuthorDate;
	}

	CString GetCommitterName() const
	{
		return m_CommitterName;
	}

	CString GetCommitterEmail() const
	{
		return m_CommitterEmail;
	}

	CTime GetCommitterDate() const
	{
		return m_CommitterDate;
	}

	CString GetSubject() const
	{
		return m_Subject;
	}

	CString GetBody() const
	{
		return m_Body;
	}

	virtual ~GitRev(void);

	enum
	{
		REV_HEAD = -1,			///< head revision
		REV_BASE = -2,			///< base revision
		REV_WC = -3,			///< revision of the working copy
		REV_UNSPECIFIED = -4,	///< unspecified revision
	};

	static CString GetHead() { return L"HEAD"; };
	static CString GetWorkingCopy() { return GIT_REV_ZERO; };

	CGitHash m_CommitHash;
	GIT_REV_LIST m_ParentHash;

	virtual void Clear();
	inline int ParentsCount() {return static_cast<int>(m_ParentHash.size()); }

	int ParserFromCommit(GIT_COMMIT *commit);
	int ParserParentFromCommit(GIT_COMMIT *commit);

	int ParserFromCommit(const git_commit* commit);
	int ParserParentFromCommit(const git_commit* commit);
	int GetCommitFromHash(git_repository* repo, const CGitHash& hash);
	int GetCommit(git_repository* repo, const CString& Rev);

	int GetParentFromHash(const CGitHash& hash);
	int GetCommitFromHash(const CGitHash& hash);
	int GetCommit(const CString& rev);

	CString GetLastErr() const { return m_sErr; }

	void DbgPrint();

private:
	int GetCommitFromHash_withoutLock(const CGitHash& hash);
};
