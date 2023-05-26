/*
	Author: Marco Costalba (C) 2005-2007
	Author: TortoiseGit (C) 2008-2013, 2017, 2021, 2023

	Copyright: See COPYING file that comes with this distribution

*/
#ifndef LANES_H
#define LANES_H

#include "githash.h"

using CGitHashList = std::vector<CGitHash>;

class Lanes {
public:
	// graph elements
	enum class LaneType {
		EMPTY,
		ACTIVE,
		NOT_ACTIVE,
		MERGE_FORK,
		MERGE_FORK_R,
		MERGE_FORK_L,
		MERGE_FORK_L_INITIAL,
		JOIN,
		JOIN_R,
		JOIN_L,
		HEAD,
		HEAD_R,
		HEAD_L,
		TAIL,
		TAIL_R,
		TAIL_L,
		CROSS,
		CROSS_EMPTY,
		INITIAL,
		BRANCH,
		UNAPPLIED,
		APPLIED,
		BOUNDARY,
		BOUNDARY_C, // corresponds to MERGE_FORK
		BOUNDARY_R, // corresponds to MERGE_FORK_R
		BOUNDARY_L, // corresponds to MERGE_FORK_L
	};

	static constexpr int COLORS_NUM = 8;

	// graph helpers
	static inline bool isHead(LaneType x) { return (x == LaneType::HEAD || x == LaneType::HEAD_R || x == LaneType::HEAD_L); }
	static inline bool isTail(LaneType x) { return (x == LaneType::TAIL || x == LaneType::TAIL_R || x == LaneType::TAIL_L); }
	static inline bool isJoin(LaneType x) { return (x == LaneType::JOIN || x == LaneType::JOIN_R || x == LaneType::JOIN_L); }
	static inline bool isFreeLane(LaneType x) { return (x == LaneType::NOT_ACTIVE || x == LaneType::CROSS || isJoin(x)); }
	static inline bool isBoundary(LaneType x) { return (x == LaneType::BOUNDARY || x == LaneType::BOUNDARY_C || x == LaneType::BOUNDARY_R || x == LaneType::BOUNDARY_L); }
	static inline bool isMerge(LaneType x) { return (x == LaneType::MERGE_FORK || x == LaneType::MERGE_FORK_R || x == LaneType::MERGE_FORK_L || isBoundary(x)); }
	static inline bool isActive(LaneType x) { return (x == LaneType::ACTIVE || x == LaneType::INITIAL || x == LaneType::BRANCH || isMerge(x)); }

	Lanes() = default;
	bool isEmpty() const { return typeVec.empty(); }
	void init(const CGitHash& expectedSha);
	void clear();
	bool isFork(const CGitHash& sha, bool& isDiscontinuity);
	void setBoundary(bool isBoundary, bool isInitial);
	void setFork(const CGitHash& sha);
	void setMerge(const CGitHashList& parents, bool onlyFirstParent);
	void setInitial();
	void setApplied();
	void changeActiveLane(const CGitHash& sha);
	void afterMerge();
	void afterFork();
	bool isBranch();
	void afterBranch();
	void afterApplied();
	void nextParent(const CGitHash& sha);
	void getLanes(std::vector<LaneType>& ln) const { ln = typeVec; } // O(1) vector is implicitly shared

private:
	int findNextSha(const CGitHash& next, int pos);
	int findType(LaneType type, int pos);
	int add(LaneType type, const CGitHash& next, int pos, bool& wasEmptyCross);
	inline bool IS_NODE(LaneType x) { return x == NODE || x == NODE_R || x == NODE_L; }

	int activeLane = 0;
	std::vector<LaneType> typeVec;
	std::vector<CGitHash> nextShaVec;
	bool boundary = false;
	LaneType NODE = LaneType::EMPTY;
	LaneType NODE_L = LaneType::EMPTY;
	LaneType NODE_R = LaneType::EMPTY;
};

#endif
