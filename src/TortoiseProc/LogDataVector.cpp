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
#include "GitLogList.h"
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

int CLogDataVector::ParserShortLog(CTGitPath *path ,int count  )
{
	CString log;
	GitRev rev;

	if(g_Git.IsInitRepos())
		return 0;

	CString begin;
	begin.Format(_T("#<%c>"),LOG_REV_ITEM_BEGIN);

	g_Git.GetShortLog(log,path,count);
	if(log.GetLength()==0)
		return 0;
	
	int start=4;
	int length;
	int next =1;
	while( next>0 )
	{
		next=log.Find(begin,start);
		if(next >0 )
			length = next - start+4;
		else
			length = log.GetLength()-start+4;

		CString onelog =log;
		onelog=log.Mid(start -4,length);
		rev.ParserFromLog(onelog);
		rev.m_Subject=_T("Load .................................");
		this->push_back(rev);
		m_HashMap[rev.m_CommitHash]=size()-1;
		start = next +4;
	}

	return 0;
	return 0;

}
int CLogDataVector::FetchFullInfo(int i)
{
	return at(i).SafeFetchFullInfo(&g_Git);
}
//CLogDataVector Class
int CLogDataVector::ParserFromLog()
{
	CString log;
	GitRev rev;
	CString emptyhash;
	g_Git.GetLog(log,emptyhash);

	CString begin;
	begin.Format(_T("#<%c>"),LOG_REV_ITEM_BEGIN);
	
	if(log.GetLength()==0)
		return 0;
	
	int start=4;
	int length;
	int next =1;
	while( next>0 )
	{
		next=log.Find(begin,start);
		if(next >0 )
			length = next - start+4;
		else
			length = log.GetLength()-start+4;

		CString onelog =log;
		onelog=log.Mid(start -4,length);
		rev.ParserFromLog(onelog);
		this->push_back(rev);
		m_HashMap[rev.m_CommitHash]=size()-1;
		start = next +4;
	}

	return 0;
}

void CLogDataVector::setLane(CString& sha) 
{
	Lanes* l = &(this->m_Lanes);
	int i = m_FirstFreeLane;
	
//	QVector<QByteArray> ba;
//	const ShaString& ss = toPersistentSha(sha, ba);
//	const ShaVect& shaVec(fh->revOrder);

	for (int cnt = size(); i < cnt; ++i) {

		GitRev* r = &(*this)[i]; 
		CString &curSha=r->m_CommitHash;

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


void CLogDataVector::updateLanes(GitRev& c, Lanes& lns, CString &sha) 
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

	lns.setBoundary(0/*c.isBoundary()*/); // update must be here

	if (isFork)
		lns.setFork(sha);
	if (isMerge)
		lns.setMerge(c.m_ParentHash);
	//if (c.isApplied)
	//	lns.setApplied();
	if (isInitial)
		lns.setInitial();

	lns.getLanes(c.m_Lanes); // here lanes are snapshotted

	CString nextSha = (isInitial) ? CString(_T("")) : QString(c.m_ParentHash[0]);

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