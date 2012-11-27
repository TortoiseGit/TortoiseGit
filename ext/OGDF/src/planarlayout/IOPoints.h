/*
 * $Revision: 2571 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-10 17:25:20 +0200 (Di, 10. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of classes InOutPoint and IOPoints which
 * implement the management of in-/out-points
 *
 * \author Carsten Gutwenger
 *
 * \par License:
 * This file is part of the Open Graph Drawing Framework (OGDF).
 *
 * \par
 * Copyright (C)<br>
 * See README.txt in the root directory of the OGDF installation for details.
 *
 * \par
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 or 3 as published by the Free Software Foundation;
 * see the file LICENSE.txt included in the packaging of this file
 * for details.
 *
 * \par
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * \par
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * \see  http://www.gnu.org/copyleft/gpl.html
 ***************************************************************/

#ifdef _MSC_VER
#pragma once
#endif

#ifndef OGDF_IO_POINTS_H
#define OGDF_IO_POINTS_H


#include <ogdf/planarity/PlanRep.h>


namespace ogdf {


/********************************************************************
			representation of an in- or outpoint
********************************************************************/

struct InOutPoint
{
	int  m_dx, m_dy;
	adjEntry m_adj;

	InOutPoint() {
		m_dx = m_dy = 0; m_adj = 0;
	}
	InOutPoint(adjEntry adj) {
		m_adj = adj; m_dx = m_dy = 0;
	}
};


/********************************************************************
			representation of in- and outpoint lists
********************************************************************/

class IOPoints {
public:
	IOPoints() { }
	IOPoints(const Graph &G) : m_depth(G,0), m_height(G,0), m_in(G), m_out(G),
		m_mark(G,false), m_pointOf(G,0)  { }

	~IOPoints () { }


	// length of in- or outpoint list
	int out(node v) const {
		return m_out[v].size();
	}
	int in(node v) const {
		return m_in[v].size();
	}

	// getting a const-reference to in- or outpoint list
	const List<InOutPoint> &inpoints(node v) const {
		return m_in [v];
	}
	List<InOutPoint> &inpoints(node v) {
		return m_in [v];
	}

	const List<InOutPoint> &outpoints(node v) const {
		return m_out [v];
	}
	List<InOutPoint> &outpoints(node v) {
		return m_out [v];
	}

	// getting the in-/outpoint belonging to an adjacency entry
	const InOutPoint *pointOf(adjEntry adj) const {
		return m_pointOf[adj];
	}

	// marking adjacency entries
	bool marked(adjEntry adj) const {
		return m_mark[adj];
	}

	bool marked(node v) {
		return (v->outdeg() == 1 && marked(v->firstAdj()));
	}

	// finding outpoints belonging to non-marked edges
	ListConstIterator<InOutPoint> firstRealOut(node v) const {
		return searchRealForward(m_out[v].begin());
	}

	ListConstIterator<InOutPoint> lastRealOut(node v) const {
		return searchRealBackward(m_out[v].rbegin());
	}

	ListConstIterator<InOutPoint> nextRealOut(ListConstIterator<InOutPoint> it) const {
		return searchRealForward(it.succ());
	}

	ListConstIterator<InOutPoint> prevRealOut(ListConstIterator<InOutPoint> it) const {
		return searchRealBackward(it.pred());
	}

	// building in-/outpoint lists
	void appendInpoint(adjEntry adj) {
		node v = adj->theNode();
		m_pointOf[adj] = &(*m_in[v].pushBack(InOutPoint(adj)));
	}
	void appendOutpoint(adjEntry adj) {
		node v = adj->theNode();
		m_pointOf[adj] = &(*m_out[v].pushBack(InOutPoint(adj)));
	}
	void pushInpoint(adjEntry adj) {
		node v = adj->theNode();
		m_pointOf[adj] = &(*m_in[v].pushFront(InOutPoint(adj)));
	}

	// setting relative coordinates
	void setOutCoord (ListIterator<InOutPoint> it, int dx, int dy) {
		(*it).m_dx = dx;
		(*it).m_dy = dy;
	}
	void setInCoord (ListIterator<InOutPoint> it, int dx, int dy) {
		(*it).m_dx = dx;
		(*it).m_dy = dy;
	}

	void setOutDx(ListIterator<InOutPoint> it, int dx) {
		(*it).m_dx = dx;
	}

	void restoreDeg1Nodes(PlanRep &PG, Stack<PlanRep::Deg1RestoreInfo> &S);

	void changeEdge(node v, adjEntry adj_new) {
		m_out[v].popBack();
		appendInpoint(adj_new);
	}

	// belongs v to a chain (= at most to non-marked incoming edges
	bool isChain(node v) const {
		int i = 0;
		ListConstIterator<InOutPoint> it;
		for(it = m_in[v].begin(); it.valid(); ++it)
			if (!marked((*it).m_adj)) ++i;
		return (i <= 2);
	}

	// width of the left-/right side of an in-/outpoint list
	int outLeft(node v) const {
		return (m_out[v].empty()) ? 0 : (-m_out[v].front().m_dx);
	}

	int outRight(node v) const {
		return (m_out[v].empty()) ? 0 : (m_out[v].back().m_dx);
	}

	int inLeft(node v) const {
		return (m_in[v].empty()) ? 0 : (-m_in[v].front().m_dx);
	}

	int inRight(node v) const {
		return (m_in[v].empty()) ? 0 : (m_in[v].back().m_dx);
	}

	int maxLeft(node v) const {
		return max(inLeft(v), outLeft(v));
	}

	int maxRight(node v) const {
		return max(inRight(v), outRight(v));
	}

	int maxPlusLeft(node v) const {
		return (in(v) >= 3) ?
			max(inLeft(v)+1, outLeft(v)) : maxLeft(v);
	}

	int maxPlusRight(node v) const {
		return (in(v) >= 3) ?
			max(inRight(v)+1, outRight(v)) : maxRight(v);
	}

	InOutPoint middleNeighbor(node z1) const;

	void numDeg1(node v, int &xl, int &xr,
		bool doubleCount) const;

	adjEntry switchBeginIn(node v);
	adjEntry switchEndIn  (node v);

	void switchBeginOut(node v);
	void switchEndOut  (node v);


	NodeArray<int> m_depth, m_height;


private:
	NodeArray<List<InOutPoint> > m_in, m_out;
	AdjEntryArray<bool> m_mark;
	AdjEntryArray<InOutPoint *> m_pointOf;

	ListConstIterator<InOutPoint> searchRealForward (ListConstIterator<InOutPoint> it) const;
	ListConstIterator<InOutPoint> searchRealBackward(ListConstIterator<InOutPoint> it) const;
};



} // end namespace ogdf


#endif
