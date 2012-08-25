// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2012 - TortoiseGit
// Copyright (C) 2007-2008 - TortoiseSVN

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
/*
	Description: start-up repository opening and reading

	Author: Marco Costalba (C) 2005-2007

	Copyright: See COPYING file that comes with this distribution

*/

#include "stdafx.h"
#include "TortoiseProc.h"
#include "GitLogListBase.h"
#include "GitRev.h"
//#include "VssStyle.h"
#include "IconMenu.h"
// CGitLogList
#include "cursor.h"
#include "InputDlg.h"
#include "GITProgressDlg.h"
#include "ProgressDlg.h"
//#include "RepositoryBrowser.h"
//#include "CopyDlg.h"
//#include "StatGraphDlg.h"
#include "Logdlg.h"
#include "MessageBox.h"
#include "Registry.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "StringUtils.h"
#include "UnicodeUtils.h"
#include "TempFile.h"
//#include "GitInfo.h"
//#include "GitDiff.h"
#include "IconMenu.h"
//#include "RevisionRangeDlg.h"
//#include "BrowseFolder.h"
//#include "BlameDlg.h"
//#include "Blame.h"
//#include "GitHelpers.h"
#include "GitStatus.h"
//#include "LogDlgHelper.h"
//#include "CachedLogInfo.h"
//#include "RepositoryInfo.h"
//#include "EditPropertiesDlg.h"
#include "FileDiffDlg.h"
#include "GitHash.h"
CGitHashMap a;

void CLogDataVector::ClearAll()
{

	clear();
	m_HashMap.clear();
	m_Lns.clear();

	m_FirstFreeLane=0;
	m_Lns.clear();

	m_RawlogData.clear();
	m_RawLogStart.clear();
}

//CLogDataVector Class
int CLogDataVector::ParserFromLog(CTGitPath *path ,int count ,int infomask,CString *from,CString *to)
{
	// only enable --follow on files
	if ((path == NULL || path->IsDirectory()) && (infomask & CGit::LOG_INFO_FOLLOW))
		infomask = infomask ^ CGit::LOG_INFO_FOLLOW;

	CString hash;
	CString cmd=g_Git.GetLogCmd(hash,path,count,infomask,from,to,true);

	if(g_Git.IsInitRepos())
		return 0;

	git_init();

	GIT_LOG handle;
	try
	{
		if (git_open_log(&handle,CUnicodeUtils::GetMulti(cmd, CP_UTF8).GetBuffer()))
		{
			return -1;
		}
	}
	catch (char * g_last_error)
	{
		MessageBox(NULL, CString(g_last_error), _T("TortoiseGit"), MB_ICONERROR);
		return -1;
	}

	git_get_log_firstcommit(handle);

	GIT_COMMIT commit;

	GitRev rev;

	while (git_get_log_nextcommit(handle, &commit, infomask & CGit::LOG_INFO_FOLLOW) == 0)
	{
		if (commit.m_ignore == 1)
		{
			git_free_commit(&commit);
			continue;
		}

		CGitHash hash = (char*)commit.m_hash ;
		rev.Clear();

		GitRev *pRev = this->m_pLogCache->GetCacheData(hash);

		char *note=NULL;
		git_get_notes(commit.m_hash,&note);
		if(note)
		{
			pRev->m_Notes.Empty();
			g_Git.StringAppend(&pRev->m_Notes,(BYTE*)note);
		}

		if(pRev == NULL || !pRev->m_IsFull)
		{
			pRev->ParserFromCommit(&commit);
			pRev->ParserParentFromCommit(&commit);
			git_free_commit(&commit);
			//Must call free commit before SafeFetchFullInfo, commit parent is rewrite by log.
			//file list will wrong if parent rewrite.
			try
			{
				pRev->SafeFetchFullInfo(&g_Git);
			}
			catch (char * g_last_error)
			{
				MessageBox(NULL, _T("Could not fetch full info of a commit.\nlibgit reports:\n") + CString(g_last_error), _T("TortoiseGit"), MB_ICONERROR);
				return -1;
			}
		}
		else
		{
			ASSERT(pRev->m_CommitHash == hash);
			pRev->ParserParentFromCommit(&commit);
			git_free_commit(&commit);
		}

		this->push_back(pRev->m_CommitHash);

		m_HashMap[rev.m_CommitHash] = (int)size() - 1;

	}

	git_close_log(handle);

	return 0;
}

int AddTolist(unsigned char * /*osha1*/, unsigned char *nsha1, const char * /*name*/, unsigned long /*time*/, int /*sz*/, const char *msg, void *data)
{
	CLogDataVector *vector = (CLogDataVector*)data;
	GitRev rev;
	rev.m_CommitHash=(char*)nsha1;

	CString one;
	g_Git.StringAppend(&one, (BYTE *)msg);

	int message=one.Find(_T(":"),0);
	if(message>0)
	{
		rev.m_RefAction=one.Left(message);
		rev.GetSubject()=one.Mid(message+1);
	}

	vector->m_pLogCache->m_HashMap[rev.m_CommitHash]=rev;
	vector->insert(vector->begin(),rev.m_CommitHash);

	return 0;
}

int CLogDataVector::ParserFromRefLog(CString ref)
{
	if(g_Git.m_IsUseGitDLL)
	{
		git_for_each_reflog_ent(CUnicodeUtils::GetUTF8(ref),AddTolist,this);
		for(int i=0;i<size();i++)
		{
			m_pLogCache->m_HashMap[at(i)].m_Ref.Format(_T("%s{%d}"), ref,i);
		}

	}
	else
	{

		CString cmd, out;
		GitRev rev;
		cmd.Format(_T("git.exe reflog show %s"),ref);
		if (g_Git.Run(cmd, &out, NULL, CP_UTF8))
			return -1;

		int pos=0;
		while(pos>=0)
		{
			CString one=out.Tokenize(_T("\n"),pos);
			int ref=one.Find(_T(' '),0);
			if(ref<0)
				continue;

			rev.Clear();

			rev.m_CommitHash=g_Git.GetHash(one.Left(ref));
			int action=one.Find(_T(' '),ref+1);
			int message;
			if(action>0)
			{
				rev.m_Ref=one.Mid(ref+1,action-ref-2);
				message=one.Find(_T(":"),action);
				if(message>0)
				{
					rev.m_RefAction=one.Mid(action+1,message-action-1);
					rev.GetSubject()=one.Right(one.GetLength()-message-1);
				}
			}

			this->m_pLogCache->m_HashMap[rev.m_CommitHash]=rev;

			this->push_back(rev.m_CommitHash);

		}
	}
	return 0;
}

void CLogDataVector::setLane(CGitHash& sha)
{
	Lanes* l = &(this->m_Lns);
	int i = m_FirstFreeLane;

//	QVector<QByteArray> ba;
//	const ShaString& ss = toPersistentSha(sha, ba);
//	const ShaVect& shaVec(fh->revOrder);

	for (int cnt = (int)size(); i < cnt; ++i) {

		GitRev* r = & this->GetGitRevAt(i);
		CGitHash curSha=r->m_CommitHash;

		if (r->m_Lanes.empty())
			updateLanes(*r, *l, curSha);

		if (curSha == sha)
			break;
	}
	m_FirstFreeLane = ++i;

#if 0
	Lanes* l = &(this->m_Lanes);
	int i = m_FirstFreeLane;

	QVector<QByteArray> ba;
	const ShaString& ss = toPersistentSha(sha, ba);
	const ShaVect& shaVec(fh->revOrder);

	for (uint cnt = shaVec.count(); i < cnt; ++i) {

		const ShaString& curSha = shaVec[i];
		Rev* r = m_HashMap[curSha]const_cast<Rev*>(revLookup(curSha, fh));
		if (r->lanes.count() == 0)
			updateLanes(*r, *l, curSha);

		if (curSha == ss)
			break;
	}
	fh->firstFreeLane = ++i;
#endif
}


void CLogDataVector::updateLanes(GitRev& c, Lanes& lns, CGitHash &sha)
{
// we could get third argument from c.sha(), but we are in fast path here
// and c.sha() involves a deep copy, so we accept a little redundancy

	if (lns.isEmpty())
		lns.init(sha);

	bool isDiscontinuity;
	bool isFork = lns.isFork(sha, isDiscontinuity);
	bool isMerge = (c.ParentsCount() > 1);
	bool isInitial = (c.ParentsCount() == 0);

	if (isDiscontinuity)
		lns.changeActiveLane(sha); // uses previous isBoundary state

	lns.setBoundary(c.IsBoundary() == TRUE); // update must be here
	TRACE(_T("%s %d"),c.m_CommitHash.ToString(),c.IsBoundary());

	if (isFork)
		lns.setFork(sha);
	if (isMerge)
		lns.setMerge(c.m_ParentHash);
	//if (c.isApplied)
	//	lns.setApplied();
	if (isInitial)
		lns.setInitial();

	lns.getLanes(c.m_Lanes); // here lanes are snapshotted

	CGitHash nextSha;
	if( !isInitial)
		nextSha = c.m_ParentHash[0];

	lns.nextParent(nextSha);

	//if (c.isApplied)
	//	lns.afterApplied();
	if (isMerge)
		lns.afterMerge();
	if (isFork)
		lns.afterFork();
	if (lns.isBranch())
		lns.afterBranch();

//	QString tmp = "", tmp2;
//	for (uint i = 0; i < c.lanes.count(); i++) {
//		tmp2.setNum(c.lanes[i]);
//		tmp.append(tmp2 + "-");
//	}
//	qDebug("%s %s",tmp.latin1(), c.sha.latin1());
}