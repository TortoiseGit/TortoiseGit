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

#pragma once
#include "GitRev.h"

class CGit;
extern CGit g_Git;
class GitRevLoglist;
class CLogCache;

typedef int CALL_UPDATE_DIFF_ASYNC(GitRevLoglist* pRev, void* data);

class GitRevLoglist : public GitRev
{
public:
	friend class CLogCache;

	GitRevLoglist(void);
	~GitRevLoglist(void);

protected:
	int				m_RebaseAction;
	int				m_Action;
	CTGitPathList	m_Files;
	CTGitPathList	m_UnRevFiles;

public:
	GIT_COMMIT m_GitCommit;

	CString m_Notes;

	TCHAR m_Mark;
	CString m_Ref; // for Refloglist
	CString m_RefAction; // for Refloglist

	// Show version tree Graphic
	std::vector<int> m_Lanes;

	volatile LONG m_IsFull;
	volatile LONG m_IsUpdateing;
	volatile LONG m_IsCommitParsed;
	volatile LONG m_IsDiffFiles;

	CALL_UPDATE_DIFF_ASYNC *m_CallDiffAsync;

	int CheckAndDiff()
	{
		if (!m_IsDiffFiles && !m_CommitHash.IsEmpty())
		{
			int ret = 0;
			ret = SafeFetchFullInfo(&g_Git);
			InterlockedExchange(&m_IsDiffFiles, TRUE);
			if (m_IsDiffFiles && m_IsCommitParsed)
				InterlockedExchange(&m_IsFull, TRUE);
			return ret;
		}
		return 1;
	}

public:
	int& GetAction(void* data)
	{
		CheckAndParser();
		if (!m_IsDiffFiles && m_CallDiffAsync)
			m_CallDiffAsync(this, data);
		else
			CheckAndDiff();
		return m_Action;
	}

	int& GetRebaseAction()
	{
		return m_RebaseAction;
	}

	CTGitPathList& GetFiles(void* data)
	{
		CheckAndParser();
		if (data && !m_IsDiffFiles && m_CallDiffAsync)
			m_CallDiffAsync(this, data);
		else
			CheckAndDiff();
		return m_Files;
	}

	CTGitPathList& GetUnRevFiles()
	{
		return m_UnRevFiles;
	}

protected:
	void CheckAndParser()
	{
		if (!m_IsCommitParsed && m_GitCommit.m_pGitCommit)
		{
			ParserFromCommit(&m_GitCommit);
			InterlockedExchange(&m_IsCommitParsed, TRUE);
			git_free_commit(&m_GitCommit);
			if (m_IsDiffFiles && m_IsCommitParsed)
				InterlockedExchange(&m_IsFull, TRUE);
		}
	}

public:
	CString& GetAuthorName()
	{
		CheckAndParser();
		return m_AuthorName;
	}

	CString& GetAuthorEmail()
	{
		CheckAndParser();
		return m_AuthorEmail;
	}

	CTime& GetAuthorDate()
	{
		CheckAndParser();
		return m_AuthorDate;
	}

	CString& GetCommitterName()
	{
		CheckAndParser();
		return m_CommitterName;
	}

	CString& GetCommitterEmail()
	{
		CheckAndParser();
		return m_CommitterEmail;
	}

	CTime& GetCommitterDate()
	{
		CheckAndParser();
		return m_CommitterDate;
	}

	CString& GetSubject()
	{
		CheckAndParser();
		return m_Subject;
	}

	CString& GetBody()
	{
		CheckAndParser();
		return m_Body;
	}

	CString GetSubjectBody()
	{
		CheckAndParser();
		CString ret(m_Subject);
		ret += _T("\r\n\r\n");
		ret += m_Body;
		return ret;
	}

	BOOL IsBoundary() { return m_Mark == _T('-'); }

	virtual void Clear();

	int SafeFetchFullInfo(CGit* git);

	int SafeGetSimpleList(CGit* git);
	volatile LONG m_IsSimpleListReady;
	STRING_VECTOR m_SimpleFileList;  /* use for find and filter, no rename detection and line num stat info */

	static int GetRefLog(const CString& ref, std::vector<GitRevLoglist>& refloglist, CString& error);
};
