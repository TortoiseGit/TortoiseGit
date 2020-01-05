// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2020 - TortoiseGit

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
#include "TGitPath.h"
#include "gitdll.h"

class CGit;
extern CGit g_Git;
class GitRevLoglist;
class CLogCache;
class IAsyncDiffCB;

typedef void CALL_UPDATE_DIFF_ASYNC(GitRevLoglist* pRev, IAsyncDiffCB* data);

class GitRevLoglist : public GitRev
{
public:
	friend class CLogCache;

	GitRevLoglist(void);
	~GitRevLoglist(void);

	class GitRevLoglistSharedFiles
	{
	public:
		GitRevLoglistSharedFiles(PSRWLOCK lock, CTGitPathList& files)
			: m_lock(lock)
			, m_files(files)
		{
			AcquireSRWLockShared(m_lock);
		}

		~GitRevLoglistSharedFiles() { ReleaseSRWLockShared(m_lock); }

		inline int GetCount() const { return m_files.GetCount(); }
		inline const CTGitPath& operator[](INT_PTR index) const { return m_files[index]; }

		const CTGitPathList& m_files;

	private:
		PSRWLOCK m_lock;
	};

	class GitRevLoglistSharedFilesWriter
	{
	public:
		GitRevLoglistSharedFilesWriter(PSRWLOCK lock, CTGitPathList& files, CTGitPathList& unrevfiles)
			: m_lock(lock)
			, m_files(files)
			, m_UnRevFiles(unrevfiles)
		{
			AcquireSRWLockExclusive(m_lock);
		}

		~GitRevLoglistSharedFilesWriter() { ReleaseSRWLockExclusive(m_lock); }

		CTGitPathList& m_files;
		CTGitPathList& m_UnRevFiles;

	private:
		PSRWLOCK m_lock;
	};

protected:
	int				m_RebaseAction;
	unsigned int	m_Action;
	CTGitPathList	m_Files;
	CTGitPathList	m_UnRevFiles;

	SRWLOCK m_lock;

public:
	CString m_Notes;

	TCHAR m_Mark;
	CString m_Ref; // for Refloglist
	CString m_RefAction; // for Refloglist

	// Show version tree Graphic
	std::vector<int> m_Lanes;

	static std::shared_ptr<CGitMailmap> s_Mailmap;

	volatile LONG m_IsDiffFiles;

	CALL_UPDATE_DIFF_ASYNC *m_CallDiffAsync;

	int CheckAndDiff()
	{
		if (!m_IsDiffFiles && !m_CommitHash.IsEmpty())
		{
			int ret = 0;
			ret = SafeFetchFullInfo(&g_Git);
			InterlockedExchange(&m_IsDiffFiles, TRUE);
			return ret;
		}
		return 1;
	}

public:
	unsigned int& GetAction(IAsyncDiffCB* data)
	{
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

	GitRevLoglistSharedFiles GetFiles(IAsyncDiffCB* data)
	{
		// data might be nullptr when instant data is requested, cf. CGitLogListAction
		if (data && !m_IsDiffFiles && m_CallDiffAsync)
			m_CallDiffAsync(this, data);
		else
			CheckAndDiff();
		return GitRevLoglistSharedFiles(&m_lock, m_Files);
	}

	GitRevLoglistSharedFilesWriter GetFilesWriter()
	{
		return GitRevLoglistSharedFilesWriter(&m_lock, m_Files, m_UnRevFiles);
	}

	CTGitPathList& GetUnRevFiles()
	{
		return m_UnRevFiles;
	}

	void Parse(GIT_COMMIT* commit, const CGitMailmap* mailmap)
	{
		ParserParentFromCommit(commit);
		ParserFromCommit(commit);
		// no caching here, because mailmap might have changed
		if (mailmap)
			ApplyMailmap(*mailmap);
	}

public:
	CString& GetAuthorName()
	{
		return m_AuthorName;
	}

	CString& GetAuthorEmail()
	{
		return m_AuthorEmail;
	}

	CTime& GetAuthorDate()
	{
		return m_AuthorDate;
	}

	CString& GetCommitterName()
	{
		return m_CommitterName;
	}

	CString& GetCommitterEmail()
	{
		return m_CommitterEmail;
	}

	CTime& GetCommitterDate()
	{
		return m_CommitterDate;
	}

	CString& GetSubject()
	{
		return m_Subject;
	}

	CString& GetBody()
	{
		return m_Body;
	}

	CString GetSubjectBody(bool crlf = false)
	{
		CString ret(m_Subject);
		if (!crlf)
		{
			ret += L"\n";
			ret += m_Body;
		}
		else
		{
			ret.TrimRight();
			ret += L"\r\n";
			CString body(m_Body);
			body.Replace(L"\n", L"\r\n");
			ret += body.TrimRight();
		}
		return ret;
	}

	BOOL IsBoundary() { return m_Mark == L'-'; }

	virtual void Clear() override;

	int SafeFetchFullInfo(CGit* git);

	int SafeGetSimpleList(CGit* git);
	volatile LONG m_IsSimpleListReady;
	STRING_VECTOR m_SimpleFileList;  /* use for find and filter, no rename detection and line num stat info */

	static int GetRefLog(const CString& ref, std::vector<GitRevLoglist>& refloglist, CString& error);
};
