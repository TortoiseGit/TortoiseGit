// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2019 - TortoiseGit
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
#include "Git.h"
#include "GitHash.h"
#include "TGitPath.h"
#include "GitLogListBase.h"
#include "UnicodeUtils.h"

typedef CComCritSecLock<CComCriticalSection> CAutoLocker;

void CLogDataVector::ClearAll()
{
	clear();
	m_HashMap.clear();
	m_Lns.clear();

	m_FirstFreeLane=0;
	m_Lns.clear();
	if (m_pLogCache)
		m_pLogCache->ClearAllLanes();
}

//CLogDataVector Class
int CLogDataVector::ParserFromLog(CTGitPath* path, DWORD count, DWORD infomask, CString* range)
{
	ATLASSERT(m_pLogCache);
	// only enable --follow on files
	if ((!path || path->IsDirectory()) && (infomask & CGit::LOG_INFO_FOLLOW))
		infomask = infomask ^ CGit::LOG_INFO_FOLLOW;

	CString gitrange = L"HEAD";
	if (range != nullptr)
		gitrange = *range;
	CFilterData filter;
	if (count == 0)
		filter.m_NumberOfLogsScale = CFilterData::SHOW_NO_LIMIT;
	else
	{
		filter.m_NumberOfLogs = count;
		filter.m_NumberOfLogsScale = CFilterData::SHOW_LAST_N_COMMITS;
	}
	CString cmd = g_Git.GetLogCmd(gitrange, path, infomask, &filter, m_logOrderBy);

	if (!g_Git.CanParseRev(gitrange))
		return -1;

	try
	{
		[] { git_init(); } ();
	}
	catch (const char* msg)
	{
		MessageBox(nullptr, L"Could not initialize libgit.\nlibgit reports:\n" + CString(msg), L"TortoiseGit", MB_ICONERROR);
		return -1;
	}

	GIT_LOG handle;
	try
	{
		CAutoLocker lock(g_Git.m_critGitDllSec);
		if (git_open_log(&handle, CUnicodeUtils::GetMulti(cmd, CP_UTF8)))
			return -1;
	}
	catch (char* msg)
	{
		MessageBox(nullptr, L"Could not open log.\nlibgit reports:\n" + CString(msg), L"TortoiseGit", MB_ICONERROR);
		return -1;
	}

	try
	{
		CAutoLocker lock(g_Git.m_critGitDllSec);
		[&]{ git_get_log_firstcommit(handle); }();
	}
	catch (char* msg)
	{
		MessageBox(nullptr, L"Could not get first commit.\nlibgit reports:\n" + CString(msg), L"TortoiseGit", MB_ICONERROR);
		return -1;
	}

	int ret = 0;
	while (ret == 0)
	{
		GIT_COMMIT commit;

		try
		{
			CAutoLocker lock(g_Git.m_critGitDllSec);
			[&]{ ret = git_get_log_nextcommit(handle, &commit, infomask & CGit::LOG_INFO_FOLLOW); }();
		}
		catch (char* msg)
		{
			MessageBox(nullptr, L"Could not get next commit.\nlibgit reports:\n" + CString(msg), L"TortoiseGit", MB_ICONERROR);
			break;
		}

		if (ret)
		{
			if (ret != -2) // other than end of revision walking
				MessageBox(nullptr, (L"Could not get next commit.\nlibgit returns:" + std::to_wstring(ret)).c_str(), L"TortoiseGit", MB_ICONERROR);
			break;
		}

		if (commit.m_ignore == 1)
		{
			git_free_commit(&commit);
			continue;
		}

		CGitHash hash = CGitHash::FromRaw(commit.m_hash);

		GitRevLoglist* pRev = this->m_pLogCache->GetCacheData(hash);

		char *pNote = nullptr;
		{
			CAutoLocker lock(g_Git.m_critGitDllSec);
			git_get_notes(commit.m_hash, &pNote);
		}
		if (pNote)
		{
			pRev->m_Notes = CUnicodeUtils::GetUnicode(pNote);
			free(pNote);
			pNote = nullptr;
		}

		pRev->ParserFromCommit(&commit);
		pRev->ParserParentFromCommit(&commit);
		git_free_commit(&commit);
		// Must call free commit before SafeFetchFullInfo, commit parent is rewrite by log.
		// file list will wrong if parent rewrite.

		if (!pRev->m_IsFull && (infomask & CGit::LOG_INFO_FULL_DIFF))
		{
			if (pRev->SafeFetchFullInfo(&g_Git))
			{
				MessageBox(nullptr, pRev->GetLastErr(), L"TortoiseGit", MB_ICONERROR);
				return -1;
			}
		}

		this->push_back(pRev->m_CommitHash);

		m_HashMap[pRev->m_CommitHash] = static_cast<int>(size()) - 1;
	}

	{
		CAutoLocker lock(g_Git.m_critGitDllSec);
		git_close_log(handle);
	}

	return 0;
}

struct SortByParentDate
{
	bool operator()(GitRevLoglist* pLhs, GitRevLoglist* pRhs)
	{
		if (pLhs->m_CommitHash == pRhs->m_CommitHash)
			return false;
		if (std::any_of(pLhs->m_ParentHash.cbegin(), pLhs->m_ParentHash.cend(), [&pRhs](auto& hash) { return hash == pRhs->m_CommitHash; }))
			return true;
		if (std::any_of(pRhs->m_ParentHash.cbegin(), pRhs->m_ParentHash.cend(), [&pLhs](auto& hash) { return hash == pLhs->m_CommitHash; }))
			return false;
		return pLhs->GetCommitterDate()>pRhs->GetCommitterDate();
	}
};

int CLogDataVector::Fill(std::unordered_set<CGitHash>& hashes)
{
	ATLASSERT(m_pLogCache);
	try
	{
		[] { git_init(); } ();
	}
	catch (const char* msg)
	{
		MessageBox(nullptr, L"Could not initialize libgit.\nlibgit reports:\n" + CString(msg), L"TortoiseGit", MB_ICONERROR);
		return -1;
	}

	std::set<GitRevLoglist*, SortByParentDate> revs;

	for (const auto& hash : hashes)
	{
		GIT_COMMIT commit;
		try
		{
			CAutoLocker lock(g_Git.m_critGitDllSec);
			if (git_get_commit_from_hash(&commit, hash.ToRaw()))
				return -1;
		}
		catch (char * msg)
		{
			MessageBox(nullptr, L"Could not get commit \"" + hash.ToString() + L"\".\nlibgit reports:\n" + CString(msg), L"TortoiseGit", MB_ICONERROR);
			return -1;
		}

		GitRevLoglist* pRev = this->m_pLogCache->GetCacheData(hash);

		// right now this code is only used by TortoiseGitBlame,
		// as such git notes are not needed to be loaded

		pRev->ParserFromCommit(&commit);
		pRev->ParserParentFromCommit(&commit);
		git_free_commit(&commit);

		revs.insert(pRev);
	}

	for (const auto& pRev : revs)
	{
		this->push_back(pRev->m_CommitHash);
		m_HashMap[pRev->m_CommitHash] = static_cast<int>(size()) - 1;
	}

	return 0;
}

void CLogDataVector::append(CGitHash& sha, bool storeInVector)
{
	if (storeInVector)
		this->push_back(sha);

	GitRevLoglist* r = &m_pLogCache->m_HashMap[sha];
	updateLanes(*r, this->m_Lns, sha);
}

void CLogDataVector::setLane(CGitHash& sha)
{
	Lanes* l = &(this->m_Lns);
	int i = m_FirstFreeLane;

//	QVector<QByteArray> ba;
//	const ShaString& ss = toPersistentSha(sha, ba);
//	const ShaVect& shaVec(fh->revOrder);

	for (int cnt = static_cast<int>(size()); i < cnt; ++i) {
		GitRevLoglist* r = &this->GetGitRevAt(i);
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

void CLogDataVector::updateLanes(GitRevLoglist& c, Lanes& lns, CGitHash& sha)
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

	lns.setBoundary(c.IsBoundary() == TRUE, isInitial); // update must be here
	TRACE(L"%s %d", static_cast<LPCTSTR>(c.m_CommitHash.ToString()), c.IsBoundary());

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
	{
		lns.afterFork();
		if (isInitial)
			lns.setInitial();
	}
	if (lns.isBranch())
		lns.afterBranch();
}
