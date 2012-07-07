// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2012 - TortoiseGit

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

#include "StdAfx.h"
#include "ATLComTime.h"
#include "GitRev.h"
#include "Git.h"
#include "GitDLL.h"
#include "UnicodeUtils.h"

class CException; //Just in case afx.h is not included (cannot be included in every project which uses this file)

// provide an ASSERT macro for when compiled without MFC
#if !defined ASSERT
	// Don't use _asm here, it isn't supported by x64 version of compiler. In fact, MFC's ASSERT() is the same with _ASSERTE().
	#define ASSERT(x) _ASSERTE(x)
#endif

typedef CComCritSecLock<CComCriticalSection> CAutoLocker;

GitRev::GitRev(void)
{
	m_Action=0;
	m_IsFull = 0;
	m_IsUpdateing = 0;
	m_IsCommitParsed = 0;
	m_IsDiffFiles = 0;
	m_CallDiffAsync = NULL;
	m_IsSimpleListReady =0;

	memset(&this->m_GitCommit,0,sizeof(GIT_COMMIT));

	// fetch local machine timezone info
	if ( GetTimeZoneInformation( &m_TimeZone ) == TIME_ZONE_ID_INVALID )
	{
		ASSERT(false);
	}
}

GitRev::~GitRev(void)
{
}

#if 0
GitRev::GitRev(GitRev & rev)
{
}
GitRev& GitRev::operator=(GitRev &rev)
{
	return *this;
}
#endif
void GitRev::Clear()
{
	this->m_Action=0;
	this->m_Files.Clear();
	this->m_Action=0;
	this->m_ParentHash.clear();
	m_CommitterName.Empty();
	m_CommitterEmail.Empty();
	m_Body.Empty();
	m_Subject.Empty();
	m_CommitHash.Empty();
	m_Ref.Empty();
	m_RefAction.Empty();
	m_Mark=0;

}
int GitRev::CopyFrom(GitRev &rev,bool OmitParentAndMark)
{
	m_AuthorName	=rev.m_AuthorName;
	m_AuthorEmail	=rev.m_AuthorEmail;
	m_AuthorDate	=rev.m_AuthorDate;
	m_CommitterName	=rev.m_CommitterName;
	m_CommitterEmail=rev.m_CommitterEmail;
	m_CommitterDate	=rev.m_CommitterDate;
	m_Subject		=rev.m_Subject;
	m_Body			=rev.m_Body;
	m_CommitHash	=rev.m_CommitHash;
	m_Files			=rev.m_Files;
	m_Action		=rev.m_Action;
	m_IsFull		=rev.m_IsFull;

	if(!OmitParentAndMark)
	{
		m_ParentHash	=rev.m_ParentHash;
		m_Mark			=rev.m_Mark;
	}
	return 0;
}

CTime GitRev::ConverFromString(CString input)
{
	// pick up date from string
	try
	{
		COleDateTime tm(_wtoi(input.Mid(0,4)),
				 _wtoi(input.Mid(5,2)),
				 _wtoi(input.Mid(8,2)),
				 _wtoi(input.Mid(11,2)),
				 _wtoi(input.Mid(14,2)),
				 _wtoi(input.Mid(17,2)));
		if( tm.GetStatus() != COleDateTime::valid )
			return CTime();//Error parsing time-string

		// pick up utc offset
		CString sign = input.Mid(20,1);		// + or -
		int hoursOffset =  _wtoi(input.Mid(21,2));
		int minsOffset = _wtoi(input.Mid(23,2));
		// convert to a fraction of a day
		double offset = (hoursOffset*60 + minsOffset) / 1440.0; // 1440 mins = 1 day
		if ( sign == "-" )
		{
			offset = -offset;
		}
		// we have to subtract this from the time given to get UTC
		tm -= offset;
		// get utc time as a SYSTEMTIME
		SYSTEMTIME sysTime;
		tm.GetAsSystemTime( sysTime );
		// and convert to users local time
		SYSTEMTIME local;
		if ( SystemTimeToTzSpecificLocalTime( &m_TimeZone, &sysTime, &local ) )
		{
			sysTime = local;
		}
		else
		{
			ASSERT(false);	// this should not happen but leave time in utc if it does
		}
		// convert to CTime and return
		return CTime( sysTime, -1 );
	}
	catch(CException* e)
	{
		//Probably the date was something like 1970-01-01 00:00:00. _mktime64() doesnt like this.
		//Dont let the application crash on this exception

#ifdef _AFX //CException classes are only defined when afx.h is included.
			//When afx.h is not included, the exception is leaked.
			//This will probably never happen because when CException is not defined, it cannot be thrown.
		e->Delete();
#endif //ifdef _AFX
	}
	return CTime(); //Return an invalid time
}

int GitRev::SafeGetSimpleList(CGit *git)
{
	if(InterlockedExchange(&m_IsUpdateing,TRUE) == FALSE)
	{
		m_SimpleFileList.clear();
		git->CheckAndInitDll();
		GIT_COMMIT commit;
		GIT_COMMIT_LIST list;
		GIT_HASH   parent;
		memset(&commit,0,sizeof(GIT_COMMIT));

		CAutoLocker lock(g_Git.m_critGitDllSec);

		try
		{
			if(git_get_commit_from_hash(&commit, this->m_CommitHash.m_hash))
				return -1;
		}
		catch (char * msg)
		{
			return -1;
		}

		int i=0;
		bool isRoot = this->m_ParentHash.size()==0;
		git_get_commit_first_parent(&commit,&list);
		while(git_get_commit_next_parent(&list,parent) == 0 || isRoot)
		{
			GIT_FILE file=0;
			int count=0;
			try
			{
				if(isRoot)
					git_root_diff(git->GetGitSimpleListDiff(), commit.m_hash, &file, &count, 0);
				else
					git_diff(git->GetGitSimpleListDiff(), parent, commit.m_hash, &file, &count, 0);
			}
			catch (char * msg)
			{
				return -1;
			}

			isRoot = false;

			CTGitPath path;
			CString strnewname;
			CString stroldname;

			for(int j=0;j<count;j++)
			{
				path.Reset();
				char *newname;
				char *oldname;

				strnewname.Empty();
				stroldname.Empty();

				int mode,IsBin,inc,dec;
				try
				{
					git_get_diff_file(git->GetGitSimpleListDiff(), file, j, &newname, &oldname, &mode, &IsBin, &inc, &dec);
				}
				catch (char * msg)
				{
					return -1;
				}

				git->StringAppend(&strnewname, (BYTE*)newname, CP_UTF8);

				m_SimpleFileList.push_back(strnewname);

			}
			git_diff_flush(git->GetGitSimpleListDiff());
			i++;
		}

		InterlockedExchange(&m_IsUpdateing,FALSE);
		InterlockedExchange(&m_IsSimpleListReady, TRUE);
		git_free_commit(&commit);
	}

	return 0;
}
int GitRev::SafeFetchFullInfo(CGit *git)
{
	if(InterlockedExchange(&m_IsUpdateing,TRUE) == FALSE)
	{
		this->m_Files.Clear();
		git->CheckAndInitDll();
		GIT_COMMIT commit;
		GIT_COMMIT_LIST list;
		GIT_HASH   parent;
		memset(&commit,0,sizeof(GIT_COMMIT));

		CAutoLocker lock(g_Git.m_critGitDllSec);

		try
		{
			if (git_get_commit_from_hash(&commit, this->m_CommitHash.m_hash))
				return -1;
		}
		catch (char * msg)
		{
			return -1;
		}

		int i=0;

		git_get_commit_first_parent(&commit,&list);
		bool isRoot = (list==NULL);

		while(git_get_commit_next_parent(&list,parent) == 0 || isRoot)
		{
			GIT_FILE file=0;
			int count=0;

			try
			{
				if (isRoot)
					git_root_diff(git->GetGitDiff(), this->m_CommitHash.m_hash, &file, &count, 1);
				else
					git_diff(git->GetGitDiff(), parent, commit.m_hash, &file, &count, 1);
			}
			catch (char * msg)
			{
				git_free_commit(&commit);
				return -1;
			}
			isRoot = false;

			CTGitPath path;
			CString strnewname;
			CString stroldname;

			for(int j=0;j<count;j++)
			{
				path.Reset();
				char *newname;
				char *oldname;

				strnewname.Empty();
				stroldname.Empty();

				int mode,IsBin,inc,dec;
				git_get_diff_file(git->GetGitDiff(),file,j,&newname,&oldname,
						&mode,&IsBin,&inc,&dec);

				git->StringAppend(&strnewname, (BYTE*)newname, CP_UTF8);
				git->StringAppend(&stroldname, (BYTE*)oldname, CP_UTF8);

				path.SetFromGit(strnewname,&stroldname);
				path.ParserAction((BYTE)mode);
				path.m_ParentNo = i;

				this->m_Action|=path.m_Action;

				if(IsBin)
				{
					path.m_StatAdd=_T("-");
					path.m_StatDel=_T("-");
				}
				else
				{
					path.m_StatAdd.Format(_T("%d"),inc);
					path.m_StatDel.Format(_T("%d"),dec);
				}
				m_Files.AddPath(path);
			}
			git_diff_flush(git->GetGitDiff());
			i++;
		}


		InterlockedExchange(&m_IsUpdateing,FALSE);
		InterlockedExchange(&m_IsFull,TRUE);
		git_free_commit(&commit);
	}

	return 0;
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
		g_Git.StringAppend(&str, (BYTE*)commit->m_Encode, CP_UTF8, commit->m_EncodeSize);
		encode = CUnicodeUtils::GetCPCode(str);
	}

	this->m_AuthorDate = commit->m_Author.Date;

	this->m_AuthorEmail.Empty();
	g_Git.StringAppend(&m_AuthorEmail,(BYTE*)commit->m_Author.Email,encode,commit->m_Author.EmailSize);

	this->m_AuthorName.Empty();
	g_Git.StringAppend(&m_AuthorName,(BYTE*)commit->m_Author.Name,encode,commit->m_Author.NameSize);

	this->m_Body.Empty();
	g_Git.StringAppend(&m_Body,(BYTE*)commit->m_Body,encode,commit->m_BodySize);

	this->m_CommitterDate = commit->m_Committer.Date;

	this->m_CommitterEmail.Empty();
	g_Git.StringAppend(&m_CommitterEmail, (BYTE*)commit->m_Committer.Email,encode, commit->m_Committer.EmailSize);

	this->m_CommitterName.Empty();
	g_Git.StringAppend(&m_CommitterName, (BYTE*)commit->m_Committer.Name,encode, commit->m_Committer.NameSize);

	this->m_Subject.Empty();
	g_Git.StringAppend(&m_Subject, (BYTE*)commit->m_Subject,encode,commit->m_SubjectSize);

	return 0;
}
#ifndef TRACE
#define TRACE(x) 1?0:(x)
#endif
void GitRev::DbgPrint()
{
	TRACE(_T("Commit %s\r\n"), this->m_CommitHash.ToString());
	for(unsigned int i=0;i<this->m_ParentHash.size();i++)
	{
		TRACE(_T("Parent %i %s"),i, m_ParentHash[i].ToString());
	}
	TRACE(_T("\n"));
}

int GitRev::GetParentFromHash(CGitHash &hash)
{
	CAutoLocker lock(g_Git.m_critGitDllSec);

	g_Git.CheckAndInitDll();

	GIT_COMMIT commit;
	if(git_get_commit_from_hash( &commit, hash.m_hash))
		return -1;

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
	if(git_get_commit_from_hash( &commit, hash.m_hash))
		return -1;

	this->ParserFromCommit(&commit);
	git_free_commit(&commit);

	this->m_CommitHash=hash;

	return 0;
}


int GitRev::GetCommit(CString refname)
{
	CAutoLocker lock(g_Git.m_critGitDllSec);

	g_Git.CheckAndInitDll();

	if(refname.GetLength() >= 8)
		if(refname.Find(_T("00000000")) == 0)
		{
			this->m_CommitHash.Empty();
			this->m_Subject=_T("Working Copy");
			return 0;
		}
	CStringA rev;
	rev= CUnicodeUtils::GetUTF8(g_Git.FixBranchName(refname));
	GIT_HASH sha;

	if(git_get_sha1(rev.GetBuffer(),sha))
		return -1;

	CGitHash hash((char*)sha);
	GetCommitFromHash_withoutLock(hash);
	return 0;
}

int GitRev::AddMergeFiles()
{
	std::map<CString, int> map;
	std::map<CString, int>::iterator it;

	for(int i=0;i<m_Files.GetCount();i++)
	{
		if(map.find(m_Files[i].GetGitPathString()) == map.end())
		{
			map[m_Files[i].GetGitPathString()]=0;
		}
		else
		{
			map[m_Files[i].GetGitPathString()]++;
		}
	}

	for(it=map.begin();it!=map.end();it++)
	{
		if(it->second)
		{
			CTGitPath path;
			path.SetFromGit(it->first);
			path.m_ParentNo = MERGE_MASK;
			path.m_StatAdd=_T("-");
			path.m_StatDel=_T("-");
			path.m_Action = CTGitPath::LOGACTIONS_MERGED;
			m_Files.AddPath(path);
		}
	}
	return 0;
}
