// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2011 - TortoiseGit

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
#include "GitStatus.h"
#include "AtlTime.h"
#include "GitHash.h"
#include "GitDll.h"
#include "git.h"

typedef std::vector<CGitHash> GIT_REV_LIST;

#define LOG_REV_AUTHOR_NAME		_T('0')
#define LOG_REV_AUTHOR_EMAIL	_T('1')
#define LOG_REV_AUTHOR_DATE		_T('2')
#define LOG_REV_COMMIT_NAME		_T('3')
#define LOG_REV_COMMIT_EMAIL	_T('4')
#define LOG_REV_COMMIT_DATE		_T('5')
#define LOG_REV_COMMIT_SUBJECT	_T('6')
#define LOG_REV_COMMIT_BODY		_T('7')
#define LOG_REV_COMMIT_HASH		_T('8')
#define LOG_REV_COMMIT_PARENT	_T('9')
#define LOG_REV_COMMIT_FILE		_T('A')
#define LOG_REV_ITEM_BEGIN		_T('B')
#define LOG_REV_ITEM_END		_T('C')

class CGit;
extern CGit g_Git;
class GitRev;
class CLogCache;

typedef int CALL_UPDATE_DIFF_ASYNC(GitRev *pRev, void *data);

class GitRev
{
public:
	friend class CLogCache;

protected:
	CString	m_AuthorName;
	CString	m_AuthorEmail;
	CTime	m_AuthorDate;
	CString	m_CommitterName;
	CString	m_CommitterEmail;
	CTime	m_CommitterDate;
	CString	m_Subject;
	CString	m_Body;

	CTGitPathList	m_Files;
	int		m_Action;

public:
	GitRev(void);

	CALL_UPDATE_DIFF_ASYNC *m_CallDiffAsync;
	int CheckAndDiff()
	{
		if(!m_IsDiffFiles && !m_CommitHash.IsEmpty())
		{
			SafeFetchFullInfo(&g_Git);
			InterlockedExchange(&m_IsDiffFiles, TRUE);
			if(m_IsDiffFiles && m_IsCommitParsed)
				InterlockedExchange(&m_IsFull, TRUE);
			return 0;
		}
		return 1;
	}

	int & GetAction(void * data)
	{
		CheckAndParser();
		if(!m_IsDiffFiles && m_CallDiffAsync)
			m_CallDiffAsync(this, data);
		else
			CheckAndDiff();
		return m_Action;
	}

	CTGitPathList & GetFiles(void * data)
	{
		CheckAndParser();
		if(data && !m_IsDiffFiles && m_CallDiffAsync)
			m_CallDiffAsync(this, data);
		else
			CheckAndDiff();
		return m_Files;
	}

//	GitRev(GitRev &rev);
//	GitRev &operator=(GitRev &rev);
	int CheckAndParser()
	{
		if(!m_IsCommitParsed && m_GitCommit.m_pGitCommit)
		{
			ParserFromCommit(&m_GitCommit);
			InterlockedExchange(&m_IsCommitParsed, TRUE);
			git_free_commit(&m_GitCommit);
			if(m_IsDiffFiles && m_IsCommitParsed)
				InterlockedExchange(&m_IsFull, TRUE);
			return 0;
		}
		return 1;
	}

	CString & GetAuthorName()
	{
		CheckAndParser();
		return m_AuthorName;
	}

	CString & GetAuthorEmail()
	{
		CheckAndParser();
		return m_AuthorEmail;
	}

	CTime & GetAuthorDate()
	{
		CheckAndParser();
		return m_AuthorDate;
	}

	CString & GetCommitterName()
	{
		CheckAndParser();
		return m_CommitterName;
	}

	CString &GetCommitterEmail()
	{
		CheckAndParser();
		return m_CommitterEmail;
	}

	CTime &GetCommitterDate()
	{
		CheckAndParser();
		return m_CommitterDate;
	}

	CString & GetSubject()
	{
		CheckAndParser();
		return m_Subject;
	}

	CString & GetBody()
	{
		CheckAndParser();
		return m_Body;
	}


	~GitRev(void);

	GIT_COMMIT m_GitCommit;

	enum
	{
		REV_HEAD = -1,			///< head revision
		REV_BASE = -2,			///< base revision
		REV_WC = -3,			///< revision of the working copy
		REV_UNSPECIFIED = -4,	///< unspecified revision
	};

	int CopyFrom(GitRev &rev,bool OmitParentAndMark=false);

	static CString GetHead(){return CString(_T("HEAD"));};
	static CString GetWorkingCopy(){return CString(GIT_REV_ZERO);};

	CString m_Notes;

	CGitHash m_CommitHash;
	GIT_REV_LIST m_ParentHash;


	TCHAR m_Mark;
	CString m_Ref;
	CString m_RefAction;

	BOOL IsBoundary(){return m_Mark == _T('-');}

	void Clear();
	//int ParserFromLog(BYTE_VECTOR &log,int start=0);
	CTime ConverFromString(CString input);
	inline int ParentsCount(){return m_ParentHash.size();}

	//Show version tree Graphic
	std::vector<int> m_Lanes;

	volatile LONG m_IsFull;
	volatile LONG m_IsUpdateing;
	volatile LONG m_IsCommitParsed;
	volatile LONG m_IsDiffFiles;

	int SafeFetchFullInfo(CGit *git);

	int ParserFromCommit(GIT_COMMIT *commit);
	int ParserParentFromCommit(GIT_COMMIT *commit);


	int GetParentFromHash(CGitHash &hash);
	int GetCommitFromHash(CGitHash &hash);
	int GetCommit(CString Rev);

	int SafeGetSimpleList(CGit *git);
	volatile LONG m_IsSimpleListReady;
	std::vector<CString> m_SimpleFileList;  /* use for find and filter*/
										/* no rename detect and line num stat infor*/

public:
	void DbgPrint();
	int	AddMergeFiles();
private:
	TIME_ZONE_INFORMATION m_TimeZone;
};
