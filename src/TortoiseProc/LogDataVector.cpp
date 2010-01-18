// TortoiseSVN - a Windows shell extension for easy version control

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
#include "PropDlg.h"
#include "SVNProgressDlg.h"
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

int CLogDataVector::ParserShortLog(CTGitPath *path ,CString &hash,int count ,int mask )
{
	BYTE_VECTOR log;
	GitRev rev;

	if(g_Git.IsInitRepos())
		return 0;

	CString begin;
	begin.Format(_T("#<%c>"),LOG_REV_ITEM_BEGIN);

	//g_Git.GetShortLog(log,path,count);

	g_Git.GetLog(log,hash,path,count,mask);

	if(log.size()==0)
		return 0;
	
	int start=4;
	int length;
	int next =0;
	while( next>=0 && next<log.size())
	{
		next=rev.ParserFromLog(log,next);

		rev.m_Subject=_T("Load .................................");
		this->push_back(rev.m_CommitHash);

		if(this->m_pLogCache->m_HashMap.IsExist(rev.m_CommitHash))
		{
			if(!this->m_pLogCache->m_HashMap[rev.m_CommitHash].m_IsFull)
				this->m_pLogCache->m_HashMap[rev.m_CommitHash].CopyFrom(rev);
		}else
			this->m_pLogCache->m_HashMap[rev.m_CommitHash].CopyFrom(rev);

		m_HashMap[rev.m_CommitHash]=size()-1;

		//next=log.find(0,next);
	}

	return 0;

}
int CLogDataVector::FetchShortLog(CTGitPath *path ,CString &hash,int count ,int mask, int ShowWC )
{
	//BYTE_VECTOR log;
	m_RawlogData.clear();
	m_RawLogStart.clear();

	GitRev rev;
	
	if(g_Git.IsInitRepos())
		return 0;

	CString begin;
	begin.Format(_T("#<%c>"),LOG_REV_ITEM_BEGIN);

	//g_Git.GetShortLog(log,path,count);
	ULONGLONG  t1,t2;
	t1=GetTickCount();
	g_Git.GetLog(m_RawlogData, hash,path,count,mask);
	t2=GetTickCount();

	TRACE(_T("GetLog Time %ld\r\n"),t2-t1);

	if(m_RawlogData.size()==0)
		return 0;
	
	int start=4;
	int length;
	int next =0;
	t1=GetTickCount();
	int a1=0,b1=0;

	while( next>=0 && next<m_RawlogData.size())
	{
		static const BYTE dataToFind[]={0,0};
		m_RawLogStart.push_back(next);
		//this->at(i).m_Subject=_T("parser...");
		next=m_RawlogData.findData(dataToFind,2,next+1);
		//next=log.find(0,next);
	}

	resize(m_RawLogStart.size() + ShowWC);

	t2=GetTickCount();

	return 0;
}
int CLogDataVector::FetchFullInfo(int i)
{
	return GetGitRevAt(i).SafeFetchFullInfo(&g_Git);
}
//CLogDataVector Class
int CLogDataVector::ParserFromLog(CTGitPath *path ,int count ,int infomask,CString *from,CString *to)
{
	BYTE_VECTOR log;
	GitRev rev;
	CString emptyhash;
	this->m_pLogCache->ClearAllParent();

	g_Git.GetLog(log,emptyhash,path,count,infomask,from,to);

	CString begin;
	begin.Format(_T("#<%c>"),LOG_REV_ITEM_BEGIN);
	
	if(log.size()==0)
		return 0;
	
	int start=4;
	int length;
	int next =0;
	while( next>=0 )
	{
		next=rev.ParserFromLog(log,next);

		if(this->m_pLogCache->m_HashMap.IsExist(rev.m_CommitHash))
		{
			if(!this->m_pLogCache->m_HashMap[rev.m_CommitHash].m_IsFull)
			{
				this->m_pLogCache->m_HashMap[rev.m_CommitHash].CopyFrom(rev);
			}
		}else
			this->m_pLogCache->m_HashMap[rev.m_CommitHash].CopyFrom(rev);

		this->m_pLogCache->m_HashMap[rev.m_CommitHash].m_IsFull=true;

		this->push_back(rev.m_CommitHash);

		m_HashMap[rev.m_CommitHash]=size()-1;		
	}

	return 0;
}

int CLogDataVector::ParserFromRefLog(CString ref)
{
	CString cmd,out;
	GitRev rev;
	cmd.Format(_T("git.exe reflog show %s"),ref);
	if(g_Git.Run(cmd,&out,CP_UTF8))
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
				rev.m_Subject=one.Right(one.GetLength()-message-1);
			}
		}

		if(this->m_pLogCache->m_HashMap.IsExist(rev.m_CommitHash))
		{
			if(!this->m_pLogCache->m_HashMap[rev.m_CommitHash].m_IsFull)
				this->m_pLogCache->m_HashMap[rev.m_CommitHash].CopyFrom(rev);
		}else
			this->m_pLogCache->m_HashMap[rev.m_CommitHash].CopyFrom(rev);

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

	for (int cnt = size(); i < cnt; ++i) {

		GitRev* r = & this->GetGitRevAt(i); 
		CGitHash curSha=r->m_CommitHash;

		if (r->m_Lanes.size() == 0)
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