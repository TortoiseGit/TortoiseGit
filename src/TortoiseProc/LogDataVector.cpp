// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2013, 2015-2016 - TortoiseGit
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

	if (infomask & CGit::LOG_INFO_FULL_DIFF)
		return -1;

	CAutoRepository repo(g_Git.GetGitRepository());
	if (!repo)
		return -1;
	CAutoRevwalk revwalk;
	if (git_revwalk_new(revwalk.GetPointer(), repo) || !revwalk)
		return -1;

	git_revwalk_sorting(revwalk, GIT_SORT_TOPOLOGICAL);

	/*if (!m_ToRev.IsEmpty() && !m_FromRev.IsEmpty())
	{
		CString range;
		range.Format(L"%s..%s", (LPCTSTR)g_Git.FixBranchName(m_FromRev), (LPCTSTR)g_Git.FixBranchName(m_ToRev));
		git_revwalk_push_range(revwalk, CUnicodeUtils::GetUTF8(range));
	}
	else if (!m_ToRev.IsEmpty())
	{
		CGitHash hash;
		g_Git.GetHash(hash, m_ToRev);
		git_revwalk_push(revwalk, (const git_oid*)hash.m_hash);
	}
	else if (!m_FromRev.IsEmpty())
	{
		CGitHash hash;
		g_Git.GetHash(hash, m_FromRev);
		git_revwalk_push(revwalk, (const git_oid*)hash.m_hash);
	}
	else*/
	if (git_revwalk_push_head(revwalk))
		return -1;

	MAP_HASH_NAME map;
	if (CGit::GetMapHashToFriendName(repo, map))
		return -1;
	
	//git_revwalk_simplify_first_parent(revwalk);

	git_oid oid;
	while ((git_revwalk_next(&oid, revwalk)) == 0)
	{
		CGitHash hash(oid.id);

		/*if (map.find(hash) == map.cend())
			continue;*/

		/*
		
		if (!strcmp(arg, "--simplify-by-decoration")) {
			revs->simplify_merges = 1;
			revs->topo_order = 1;
			revs->rewrite_parents = 1;
			revs->simplify_history = 0;
			revs->simplify_by_decoration = 1;
			revs->limited = 1;
			revs->prune = 1;
			load_ref_decorations(DECORATE_SHORT_REFS);
		}
		*/

		GitRevLoglist* pRev = this->m_pLogCache->GetCacheData(hash);
		CAutoCommit commit;
		if (git_commit_lookup(commit.GetPointer(), repo, &oid))
			return -1;

		pRev->ParserFromCommit(commit);
		pRev->ParserParentFromCommit(commit);

		//if (git_commit_parentcount(commit) == 0)
		{
			this->push_back(pRev->m_CommitHash);
			m_HashMap[pRev->m_CommitHash] = (int)size() - 1;
		}

		if (--count == 0)
			break;
	}

	return 0;
}

struct SortByParentDate
{
	bool operator()(GitRevLoglist* pLhs, GitRevLoglist* pRhs)
	{
		if (pLhs->m_CommitHash == pRhs->m_CommitHash)
			return false;
		for (const auto& hash : pLhs->m_ParentHash)
		{
			if (hash == pRhs->m_CommitHash)
				return true;
		}
		for (const auto& hash : pRhs->m_ParentHash)
		{
			if (hash == pLhs->m_CommitHash)
				return false;
		}
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
			if (git_get_commit_from_hash(&commit, hash.m_hash))
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
		m_HashMap[pRev->m_CommitHash] = (int)size() - 1;
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

	for (int cnt = (int)size(); i < cnt; ++i) {
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

	lns.setBoundary(c.IsBoundary() == TRUE); // update must be here
	TRACE(L"%s %d", (LPCTSTR)c.m_CommitHash.ToString(), c.IsBoundary());

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
}