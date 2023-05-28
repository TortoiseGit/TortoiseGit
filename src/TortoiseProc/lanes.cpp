/*
	Description: history graph computation

	Author: Marco Costalba (C) 2005-2007
	Copyright (C) 2008-2015-2017, 2022 - TortoiseGit

	Copyright: See COPYING file that comes with this distribution

*/
#include "stdafx.h"
#include "lanes.h"

void Lanes::init(const CGitHash& expectedSha) {
	clear();
	activeLane = 0;
	setBoundary(false, false);
	bool wasEmptyCross = false;
	add(LaneType::BRANCH, expectedSha, activeLane, wasEmptyCross);
}

void Lanes::clear() {
	typeVec.clear();
	nextShaVec.clear();
}

void Lanes::setBoundary(bool b, bool initial) {
// changes the state so must be called as first one

	NODE = b ? LaneType::BOUNDARY_C : LaneType::MERGE_FORK;
	NODE_R = b ? LaneType::BOUNDARY_R : LaneType::MERGE_FORK_R;
	NODE_L = b ? LaneType::BOUNDARY_L : (initial ? LaneType::MERGE_FORK_L_INITIAL : LaneType::MERGE_FORK_L);
	boundary = b;

	if (boundary)
		typeVec[activeLane] = LaneType::BOUNDARY;
}

bool Lanes::isFork(const CGitHash& sha, bool& isDiscontinuity) {
	int pos = findNextSha(sha, 0);
	isDiscontinuity = (activeLane != pos);
	if (pos == -1) // new branch case
		return false;

	return (findNextSha(sha, pos + 1) != -1);
/*
	int cnt = 0;
	while (pos != -1) {
		cnt++;
		pos = findNextSha(sha, pos + 1);
//		if (isDiscontinuity)
//			isDiscontinuity = (activeLane != pos);
	}
	return (cnt > 1);
*/
}

void Lanes::setFork(const CGitHash& sha) {
	int rangeStart, rangeEnd, idx;
	rangeStart = rangeEnd = idx = findNextSha(sha, 0);

	while (idx != -1) {
		rangeEnd = idx;
		typeVec[idx] = LaneType::TAIL;
		idx = findNextSha(sha, idx + 1);
	}
	typeVec[activeLane] = NODE;

	LaneType& startT = typeVec[rangeStart];
	LaneType& endT = typeVec[rangeEnd];

	if (startT == NODE)
		startT = NODE_L;

	if (endT == NODE)
		endT = NODE_R;

	if (startT == LaneType::TAIL)
		startT = LaneType::TAIL_L;

	if (endT == LaneType::TAIL)
		endT = LaneType::TAIL_R;

	for (int i = rangeStart + 1; i < rangeEnd; ++i) {
		LaneType& t = typeVec[i];

		if (t == LaneType::NOT_ACTIVE)
			t = LaneType::CROSS;

		else if (t == LaneType::EMPTY)
			t = LaneType::CROSS_EMPTY;
	}
}

void Lanes::setMerge(const CGitHashList& parents, bool onlyFirstParent) {
// setFork() must be called before setMerge()

	if (boundary)
		return; // handle as a simple active line

	LaneType& t = typeVec[activeLane];
	bool wasFork   = (t == NODE);
	bool wasFork_L = (t == NODE_L);
	bool wasFork_R = (t == NODE_R);
	bool startJoinWasACross = false, endJoinWasACross = false;
	bool endWasEmptyCross = false;

	t = NODE;

	int rangeStart = activeLane, rangeEnd = activeLane;
	if (!onlyFirstParent) {
		CGitHashList::const_iterator it(parents.cbegin());
		for (++it; it != parents.cend(); ++it) { // skip first parent

			int idx = findNextSha(*it, 0);
			if (idx != -1) {
				if (idx > rangeEnd) {
					rangeEnd = idx;
					endJoinWasACross = typeVec[idx] == LaneType::CROSS;
				}

				if (idx < rangeStart) {
					rangeStart = idx;
					startJoinWasACross = typeVec[idx] == LaneType::CROSS;
				}

				typeVec[idx] = LaneType::JOIN;
			}
			else
				rangeEnd = add(LaneType::HEAD, *it, rangeEnd + 1, endWasEmptyCross);
		}
	}
	LaneType& startT = typeVec[rangeStart];
	LaneType& endT = typeVec[rangeEnd];

	if (startT == NODE && !wasFork && !wasFork_R)
		startT = NODE_L;

	if (endT == NODE && !wasFork && !wasFork_L)
		endT = NODE_R;

	if (startT == LaneType::JOIN && !startJoinWasACross)
		startT = LaneType::JOIN_L;

	if (endT == LaneType::JOIN && !endJoinWasACross)
		endT = LaneType::JOIN_R;

	if (startT == LaneType::HEAD)
		startT = LaneType::HEAD_L;

	if (endT == LaneType::HEAD && !endWasEmptyCross)
		endT = LaneType::HEAD_R;

	for (int i = rangeStart + 1; i < rangeEnd; ++i) {
		LaneType& t2 = typeVec[i];

		if (t2 == LaneType::NOT_ACTIVE)
			t2 = LaneType::CROSS;

		else if (t2 == LaneType::EMPTY)
			t2 = LaneType::CROSS_EMPTY;

		else if (t2 == LaneType::TAIL_R || t2 == LaneType::TAIL_L)
			t2 = LaneType::TAIL;
	}
}

void Lanes::setInitial() {
	LaneType& t = typeVec[activeLane];
	if (!IS_NODE(t) && t != LaneType::APPLIED)
		t = (boundary ? LaneType::BOUNDARY : LaneType::INITIAL);
}

void Lanes::setApplied() {
	// applied patches are not merges, nor forks
	typeVec[activeLane] = LaneType::APPLIED; // TODO test with boundaries
}

void Lanes::changeActiveLane(const CGitHash& sha) {
	LaneType& t = typeVec[activeLane];
	if (t == LaneType::INITIAL || isBoundary(t))
		t = LaneType::EMPTY;
	else
		t = LaneType::NOT_ACTIVE;

	int idx = findNextSha(sha, 0); // find first sha
	if (idx != -1)
		typeVec[idx] = LaneType::ACTIVE; // called before setBoundary()
	else {
		bool wasEmptyCross = false;
		idx = add(LaneType::BRANCH, sha, activeLane, wasEmptyCross); // new branch
	}

	activeLane = idx;
}

void Lanes::afterMerge() {
	if (boundary)
		return; // will be reset by changeActiveLane()

	for (unsigned int i = 0; i < typeVec.size(); ++i) {
		LaneType& t = typeVec[i];

		if (isHead(t) || isJoin(t) || t == LaneType::CROSS)
			t = LaneType::NOT_ACTIVE;

		else if (t == LaneType::CROSS_EMPTY)
			t = LaneType::EMPTY;

		else if (IS_NODE(t))
			t = LaneType::ACTIVE;
	}
}

void Lanes::afterFork() {
	for (unsigned int i = 0; i < typeVec.size(); ++i) {
		LaneType& t = typeVec[i];

		if (t == LaneType::CROSS)
			t = LaneType::NOT_ACTIVE;

		else if (isTail(t) || t == LaneType::CROSS_EMPTY)
			t = LaneType::EMPTY;

		if (!boundary && IS_NODE(t))
			t = LaneType::ACTIVE; // boundary will be reset by changeActiveLane()
	}
	while (typeVec.back() == LaneType::EMPTY)
	{
		typeVec.pop_back();
		nextShaVec.pop_back();
	}
}

bool Lanes::isBranch() {
	return (typeVec[activeLane] == LaneType::BRANCH);
}

void Lanes::afterBranch() {
	typeVec[activeLane] = LaneType::ACTIVE; // TODO test with boundaries
}

void Lanes::afterApplied() {
	typeVec[activeLane] = LaneType::ACTIVE; // TODO test with boundaries
}

void Lanes::nextParent(const CGitHash& sha) {
	if(boundary)
		nextShaVec[activeLane].Empty();
	else
		nextShaVec[activeLane] = sha;
}

int Lanes::findNextSha(const CGitHash& next, int pos) {
	for (unsigned int i = pos; i < nextShaVec.size(); ++i)
		if (nextShaVec[i] == next)
			return i;
	return -1;
}

int Lanes::findType(LaneType type, int pos)
{
	for (unsigned int i = pos; i < typeVec.size(); ++i)
		if (typeVec[i] == type)
			return i;
	return -1;
}

int Lanes::add(LaneType type, const CGitHash& next, int pos, bool& wasEmptyCross)
{
	wasEmptyCross = false;
	// first check empty lanes starting from pos
	if (pos < static_cast<int>(typeVec.size()))
	{
		int posEmpty = findType(LaneType::EMPTY, pos);
		int posCrossEmpty = findType(LaneType::CROSS_EMPTY, pos);
		// Use first "empty" position.
		if (posEmpty != -1 && posCrossEmpty != -1)
			pos = min(posEmpty, posCrossEmpty);
		else if (posEmpty != -1)
			pos = posEmpty;
		else if (posCrossEmpty != -1)
			pos = posCrossEmpty;
		else
			pos = -1;

		if (pos != -1) {
			wasEmptyCross = (pos == posCrossEmpty);

			typeVec[pos] = type;
			nextShaVec[pos] = next;
			return pos;
		}
	}
	// if all lanes are occupied add a new lane
	typeVec.push_back(type);
	nextShaVec.push_back(next);
	return static_cast<int>(typeVec.size()) - 1;
}
