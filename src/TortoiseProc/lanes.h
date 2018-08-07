/*
	Author: Marco Costalba (C) 2005-2007
	Author: TortoiseGit (C) 2008-2013, 2017

	Copyright: See COPYING file that comes with this distribution

*/
#ifndef LANES_H
#define LANES_H

#include "githash.h"

#define QVector std::vector

typedef std::vector<CGitHash> CGitHashList ;

class Lanes {
public:
	// graph elements
	enum LaneType {
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

		LANE_TYPES_NUM,
		COLORS_NUM=8
	};

	// graph helpers
	static inline bool isHead(int x) { return (x == HEAD || x == HEAD_R || x == HEAD_L); }
	static inline bool isTail(int x) { return (x == TAIL || x == TAIL_R || x == TAIL_L); }
	static inline bool isJoin(int x) { return (x == JOIN || x == JOIN_R || x == JOIN_L); }
	static inline bool isFreeLane(int x) { return (x == NOT_ACTIVE || x == CROSS || isJoin(x)); }
	static inline bool isBoundary(int x) { return (x == BOUNDARY || x == BOUNDARY_C || x == BOUNDARY_R || x == BOUNDARY_L); }
	static inline bool isMerge(int x) { return (x == MERGE_FORK || x == MERGE_FORK_R || x == MERGE_FORK_L || isBoundary(x)); }
	static inline bool isActive(int x) { return (x == ACTIVE || x == INITIAL || x == BRANCH || isMerge(x)); }

	Lanes() // init() will setup us later, when data is available
		: activeLane(0)
		, boundary(false)
		, NODE(0)
		, NODE_L(0)
		, NODE_R(0)
	{}
	bool isEmpty() { return typeVec.empty(); }
	void init(const CGitHash& expectedSha);
	void clear();
	bool isFork(const CGitHash& sha, bool& isDiscontinuity);
	void setBoundary(bool isBoundary, bool isInitial);
	void setFork(const CGitHash& sha);
	void setMerge(const CGitHashList& parents);
	void setInitial();
	void setApplied();
	void changeActiveLane(const CGitHash& sha);
	void afterMerge();
	void afterFork();
	bool isBranch();
	void afterBranch();
	void afterApplied();
	void nextParent(const CGitHash& sha);
	void getLanes(QVector<int> &ln) { ln = typeVec; } // O(1) vector is implicitly shared

private:
	int findNextSha(const CGitHash& next, int pos);
	int findType(int type, int pos);
	int add(int type, const CGitHash& next, int pos, bool& wasEmptyCross);

	int activeLane;
	QVector<int> typeVec;
	QVector<CGitHash> nextShaVec;
	bool boundary;
	int NODE, NODE_L, NODE_R;
};

#endif
