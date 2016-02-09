﻿// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2015 - TortoiseGit

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

#include "stdafx.h"
#include "RepositoryFixtures.h"
#include "LogDlgHelper.h"

class CLogDataVectorCBasicGitWithTestRepoFixture : public CBasicGitWithTestRepoFixture
{
};

class CLogDataVectorCBasicGitWithEmptyRepositoryFixturee : public CBasicGitWithEmptyRepositoryFixture
{
};

class CLogDataVectorCBasicGitWithTestRepoBareFixture : public CBasicGitWithTestRepoBareFixture
{
};

class CLogDataVectorCBasicGitWithEmptyBareRepositoryFixturee : public CBasicGitWithEmptyBareRepositoryFixture
{
};

INSTANTIATE_TEST_CASE_P(CLogDataVector, CLogDataVectorCBasicGitWithTestRepoFixture, testing::Values(LIBGIT));
INSTANTIATE_TEST_CASE_P(CLogDataVector, CLogDataVectorCBasicGitWithTestRepoBareFixture, testing::Values(LIBGIT));
INSTANTIATE_TEST_CASE_P(CLogDataVector, CLogDataVectorCBasicGitWithEmptyRepositoryFixturee, testing::Values(LIBGIT));
INSTANTIATE_TEST_CASE_P(CLogDataVector, CLogDataVectorCBasicGitWithEmptyBareRepositoryFixturee, testing::Values(LIBGIT));

TEST(CLogDataVector, Empty)
{
	CLogDataVector logDataVector;

	EXPECT_EQ(0, logDataVector.size());
	EXPECT_EQ(0, logDataVector.m_HashMap.size());
}

TEST(CLogCache, GetCacheData)
{
	CLogCache logCache;
	EXPECT_EQ(0, logCache.m_HashMap.size());
	CGitHash hash1(L"4c5c93d2a0b368bc4570d5ec02ab03b9c4334d44");
	GitRevLoglist* pRev = logCache.GetCacheData(hash1);
	ASSERT_TRUE(pRev);
	EXPECT_EQ(hash1, pRev->m_CommitHash);
	EXPECT_EQ(1, logCache.m_HashMap.size());

	CGitHash hash2(L"dead91b4aedeaddeaddead2a56d3c473c705dead");
	pRev = logCache.GetCacheData(hash2);
	ASSERT_TRUE(pRev);
	EXPECT_EQ(hash2, pRev->m_CommitHash);
	EXPECT_EQ(2, logCache.m_HashMap.size());
}

TEST(CLogCache, ClearAllParent)
{
	CLogCache logCache;
	CGitHash hash1(L"4c5c93d2a0b368bc4570d5ec02ab03b9c4334d44");
	GitRevLoglist* pRev = logCache.GetCacheData(hash1);
	ASSERT_TRUE(pRev);
	pRev->m_ParentHash.push_back(CGitHash(L"0000000000000000000000000000000000000000"));
	EXPECT_EQ(1, pRev->m_ParentHash.size());
	pRev->m_Lanes.push_back(1);
	pRev->m_Lanes.push_back(2);
	pRev->m_Lanes.push_back(3);
	EXPECT_EQ(3, pRev->m_Lanes.size());
	CGitHash hash2(L"dead91b4aedeaddeaddead2a56d3c473c705dead");
	pRev = logCache.GetCacheData(hash2);
	ASSERT_TRUE(pRev);
	pRev->m_ParentHash.push_back(CGitHash(L"1111111111111111111111111111111111111111"));
	pRev->m_ParentHash.push_back(CGitHash(L"1111111111111111111111111111111111111112"));
	EXPECT_EQ(2, pRev->m_ParentHash.size());
	pRev->m_Lanes.push_back(4);
	EXPECT_EQ(1, pRev->m_Lanes.size());

	logCache.ClearAllParent();
	pRev = logCache.GetCacheData(hash1);
	ASSERT_TRUE(pRev);
	EXPECT_EQ(0, pRev->m_ParentHash.size());
	EXPECT_EQ(0, pRev->m_Lanes.size());

	pRev = logCache.GetCacheData(hash2);
	ASSERT_TRUE(pRev);
	EXPECT_EQ(0, pRev->m_ParentHash.size());
	EXPECT_EQ(0, pRev->m_Lanes.size());
}

TEST(CLogCache, ClearAllLanes)
{
	CLogCache logCache;
	CGitHash hash1(L"4c5c93d2a0b368bc4570d5ec02ab03b9c4334d44");
	GitRevLoglist* pRev = logCache.GetCacheData(hash1);
	ASSERT_TRUE(pRev);
	pRev->m_ParentHash.push_back(CGitHash(L"0000000000000000000000000000000000000000"));
	EXPECT_EQ(1, pRev->m_ParentHash.size());
	pRev->m_Lanes.push_back(1);
	pRev->m_Lanes.push_back(2);
	pRev->m_Lanes.push_back(3);
	EXPECT_EQ(3, pRev->m_Lanes.size());
	CGitHash hash2(L"dead91b4aedeaddeaddead2a56d3c473c705dead");
	pRev = logCache.GetCacheData(hash2);
	ASSERT_TRUE(pRev);
	pRev->m_ParentHash.push_back(CGitHash(L"1111111111111111111111111111111111111111"));
	pRev->m_ParentHash.push_back(CGitHash(L"1111111111111111111111111111111111111112"));
	EXPECT_EQ(2, pRev->m_ParentHash.size());
	pRev->m_Lanes.push_back(4);
	EXPECT_EQ(1, pRev->m_Lanes.size());

	logCache.ClearAllLanes();
	pRev = logCache.GetCacheData(hash1);
	ASSERT_TRUE(pRev);
	EXPECT_EQ(1, pRev->m_ParentHash.size());
	EXPECT_EQ(0, pRev->m_Lanes.size());

	pRev = logCache.GetCacheData(hash2);
	ASSERT_TRUE(pRev);
	EXPECT_EQ(2, pRev->m_ParentHash.size());
	EXPECT_EQ(0, pRev->m_Lanes.size());
}


static void ParserFromLogTests()
{
	CLogCache logCache;
	CLogDataVector logDataVector;
	logDataVector.SetLogCache(&logCache);

	EXPECT_EQ(0, logDataVector.ParserFromLog());
	EXPECT_EQ(12, logDataVector.size());
	EXPECT_EQ(12, logDataVector.m_HashMap.size());
	EXPECT_EQ(12, logCache.m_HashMap.size());

	logCache.m_HashMap.clear();
	logDataVector.ClearAll();
	EXPECT_EQ(0, logDataVector.size());
	EXPECT_EQ(0, logDataVector.m_HashMap.size());
	EXPECT_EQ(0, logCache.m_HashMap.size());

	EXPECT_EQ(0, logDataVector.ParserFromLog(nullptr, 5));
	ASSERT_EQ(5, logDataVector.size());
	EXPECT_EQ(5, logDataVector.m_HashMap.size());
	EXPECT_EQ(5, logCache.m_HashMap.size());
	EXPECT_STREQ(L"Changed ASCII file", logDataVector.GetGitRevAt(0).GetSubject());
	EXPECT_STREQ(L"7c3cbfe13a929d2291a574dca45e4fd2d2ac1aa6", logDataVector.GetGitRevAt(0).m_CommitHash.ToString());
	EXPECT_STREQ(L"49ecdfff36bfe2b9b499b33e5034f427e2fa54dd", logDataVector.GetGitRevAt(2).m_CommitHash.ToString());
	EXPECT_STREQ(L"560deea87853158b22d0c0fd73f60a458d47838a", logDataVector.GetGitRevAt(4).m_CommitHash.ToString());

	logCache.m_HashMap.clear();
	logDataVector.ClearAll();
	EXPECT_EQ(0, logDataVector.ParserFromLog(nullptr, 0, CGit::LOG_INFO_ALL_BRANCH));
	ASSERT_EQ(23, logDataVector.size());
	EXPECT_EQ(23, logDataVector.m_HashMap.size());
	EXPECT_EQ(23, logCache.m_HashMap.size());
	EXPECT_STREQ(L"60c1373baa174634824a80f7b74428a60e525b43", logDataVector.GetGitRevAt(0).m_CommitHash.ToString());
	EXPECT_STREQ(L"8eabf9a475b4a15c0f4d2169e5947534dff38037", logDataVector.GetGitRevAt(11).m_CommitHash.ToString());
	EXPECT_STREQ(L"844309789a13614b52d5e7cbfe6350dd73d1dc72", logDataVector.GetGitRevAt(22).m_CommitHash.ToString());

	logCache.m_HashMap.clear();
	logDataVector.ClearAll();
	CString range(L"does-not-exist");
	EXPECT_EQ(-1, logDataVector.ParserFromLog(nullptr, 0, CGit::LOG_INFO_ALL_BRANCH, &range));
	EXPECT_EQ(0, logDataVector.size());
	EXPECT_EQ(0, logDataVector.m_HashMap.size());
	EXPECT_EQ(0, logCache.m_HashMap.size());

	EXPECT_EQ(-1, logDataVector.ParserFromLog(nullptr, 0, 0, &range));
	EXPECT_EQ(0, logDataVector.size());
	EXPECT_EQ(0, logDataVector.m_HashMap.size());
	EXPECT_EQ(0, logCache.m_HashMap.size());

	logCache.m_HashMap.clear();
	logDataVector.ClearAll();
	range = L"master2..master";
	EXPECT_EQ(0, logDataVector.ParserFromLog(nullptr, 0, 0, &range));
	ASSERT_EQ(2, logDataVector.size());
	EXPECT_EQ(2, logDataVector.m_HashMap.size());
	EXPECT_EQ(2, logCache.m_HashMap.size());
	EXPECT_STREQ(L"7c3cbfe13a929d2291a574dca45e4fd2d2ac1aa6", logDataVector.GetGitRevAt(0).m_CommitHash.ToString());
	EXPECT_STREQ(L"1fc3c9688e27596d8717b54f2939dc951568f6cb", logDataVector.GetGitRevAt(1).m_CommitHash.ToString());

	logCache.m_HashMap.clear();
	logDataVector.ClearAll();
	EXPECT_EQ(0, logDataVector.ParserFromLog(nullptr, 0, CGit::LOG_INFO_ALL_BRANCH, &range));
	ASSERT_EQ(13, logDataVector.size());
	EXPECT_EQ(13, logDataVector.m_HashMap.size());
	EXPECT_EQ(13, logCache.m_HashMap.size());

	logCache.m_HashMap.clear();
	logDataVector.ClearAll();
	CTGitPath path(L"does-not-exist.txt");
	EXPECT_EQ(0, logDataVector.ParserFromLog(&path));
	EXPECT_EQ(0, logDataVector.size());
	EXPECT_EQ(0, logDataVector.m_HashMap.size());
	EXPECT_EQ(0, logCache.m_HashMap.size());

	logCache.m_HashMap.clear();
	logDataVector.ClearAll();
	path = CTGitPath(L"copy/utf16-be-bom.txt");
	EXPECT_EQ(0, logDataVector.ParserFromLog(&path));
	ASSERT_EQ(2, logDataVector.size());
	EXPECT_EQ(2, logDataVector.m_HashMap.size());
	EXPECT_EQ(2, logCache.m_HashMap.size());
	EXPECT_STREQ(L"49ecdfff36bfe2b9b499b33e5034f427e2fa54dd", logDataVector.GetGitRevAt(0).m_CommitHash.ToString());
	EXPECT_STREQ(L"560deea87853158b22d0c0fd73f60a458d47838a", logDataVector.GetGitRevAt(1).m_CommitHash.ToString());

	logCache.m_HashMap.clear();
	logDataVector.ClearAll();
	EXPECT_EQ(0, logDataVector.ParserFromLog(&path, 0, CGit::LOG_INFO_FOLLOW));
	ASSERT_EQ(4, logDataVector.size());
	EXPECT_EQ(4, logDataVector.m_HashMap.size());
	EXPECT_EQ(4, logCache.m_HashMap.size());
	EXPECT_STREQ(L"49ecdfff36bfe2b9b499b33e5034f427e2fa54dd", logDataVector.GetGitRevAt(0).m_CommitHash.ToString());
	EXPECT_STREQ(L"560deea87853158b22d0c0fd73f60a458d47838a", logDataVector.GetGitRevAt(1).m_CommitHash.ToString());
	EXPECT_STREQ(L"ff1fbef1a54a9849afd4a5e94d2ca4d80d5b96c2", logDataVector.GetGitRevAt(3).m_CommitHash.ToString());

	logCache.m_HashMap.clear();
	logDataVector.ClearAll();
	range = L"4c5c93d2a0b368bc4570d5ec02ab03b9c4334d44";
	EXPECT_EQ(0, logDataVector.ParserFromLog(&path, 0, CGit::LOG_INFO_FOLLOW, &range));
	ASSERT_EQ(3, logDataVector.size());
	EXPECT_EQ(3, logDataVector.m_HashMap.size());
	EXPECT_EQ(3, logCache.m_HashMap.size());
	EXPECT_STREQ(L"560deea87853158b22d0c0fd73f60a458d47838a", logDataVector.GetGitRevAt(0).m_CommitHash.ToString());
	EXPECT_STREQ(L"a9d53b535cb49640a6099860ac4999f5a0857b91", logDataVector.GetGitRevAt(1).m_CommitHash.ToString());
	EXPECT_STREQ(L"ff1fbef1a54a9849afd4a5e94d2ca4d80d5b96c2", logDataVector.GetGitRevAt(2).m_CommitHash.ToString());

	logCache.m_HashMap.clear();
	logDataVector.ClearAll();
	EXPECT_EQ(0, logDataVector.ParserFromLog(&path, 0, CGit::LOG_INFO_FULL_DIFF, &range));
	ASSERT_EQ(1, logDataVector.size());
	EXPECT_EQ(1, logDataVector.m_HashMap.size());
	EXPECT_EQ(1, logCache.m_HashMap.size());
	EXPECT_STREQ(L"560deea87853158b22d0c0fd73f60a458d47838a", logDataVector.GetGitRevAt(0).m_CommitHash.ToString());
}

TEST_P(CLogDataVectorCBasicGitWithTestRepoFixture, ParserFromLog)
{
	ParserFromLogTests();
}

TEST_P(CLogDataVectorCBasicGitWithTestRepoBareFixture, ParserFromLog)
{
	ParserFromLogTests();
}

static void ParserFromLogTests_EmptyRepo()
{
	CLogCache logCache;
	CLogDataVector logDataVector;
	logDataVector.SetLogCache(&logCache);
	EXPECT_EQ(-1, logDataVector.ParserFromLog());
	EXPECT_EQ(0, logDataVector.size());
	EXPECT_EQ(0, logDataVector.m_HashMap.size());
	EXPECT_EQ(0, logCache.m_HashMap.size());
}

TEST_P(CLogDataVectorCBasicGitWithEmptyRepositoryFixturee, ParserFromLog)
{
	ParserFromLogTests_EmptyRepo();
}

TEST_P(CLogDataVectorCBasicGitWithEmptyBareRepositoryFixturee, ParserFromLog)
{
	ParserFromLogTests_EmptyRepo();
}

static void FillTests()
{
	CLogCache logCache;
	CLogDataVector logDataVector;
	logDataVector.SetLogCache(&logCache);

	std::set<CGitHash> hashes;
	EXPECT_EQ(0, logDataVector.Fill(hashes));
	EXPECT_EQ(0, logDataVector.size());
	EXPECT_EQ(0, logCache.m_HashMap.size());

	hashes.insert(CGitHash(L"35c91b4ae2f77f4f21a7aba56d3c473c705d89e6"));
	logCache.m_HashMap.clear();
	logDataVector.ClearAll();
	EXPECT_EQ(0, logDataVector.Fill(hashes));
	ASSERT_EQ(1, logDataVector.size());
	EXPECT_TRUE(logDataVector.GetGitRevAt(0).m_CommitHash.ToString() == L"35c91b4ae2f77f4f21a7aba56d3c473c705d89e6");
	EXPECT_EQ(1, logCache.m_HashMap.size());

	hashes.insert(CGitHash(L"7c3cbfe13a929d2291a574dca45e4fd2d2ac1aa6"));
	logCache.m_HashMap.clear();
	logDataVector.ClearAll();
	EXPECT_EQ(0, logDataVector.Fill(hashes));
	EXPECT_EQ(2, logDataVector.size());
	EXPECT_EQ(2, logCache.m_HashMap.size());

	hashes.insert(CGitHash(L"dead91b4aedeaddeaddead2a56d3c473c705dead")); // non-existent commit
	logCache.m_HashMap.clear();
	logDataVector.ClearAll();
	EXPECT_EQ(-1, logDataVector.Fill(hashes));
	EXPECT_EQ(0, logDataVector.size());
	// size of logCache.m_HashMap.size() depends on the order of the set
}


TEST_P(CLogDataVectorCBasicGitWithTestRepoFixture, Fill)
{
	FillTests();
}

TEST_P(CLogDataVectorCBasicGitWithTestRepoBareFixture, Fill)
{
	FillTests();
}
