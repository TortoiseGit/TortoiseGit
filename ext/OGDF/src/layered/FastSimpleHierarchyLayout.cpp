/*
 * $Revision: 2552 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-05 16:45:20 +0200 (Do, 05. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of the FastSimpleHierarchyLayout
 * (third phase of sugiyama)
 *
 * \author Till Sch&auml;fer
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


#include <ogdf/layered/FastSimpleHierarchyLayout.h>
#include <ogdf/layered/Hierarchy.h>
#include <ogdf/layered/Level.h>
#include <ogdf/basic/exceptions.h>
#include <ogdf/basic/Array.h>
#include <ogdf/basic/List.h>
#include <math.h>
#include <limits>


namespace ogdf {

FastSimpleHierarchyLayout::FastSimpleHierarchyLayout(int minXSep, int ySep)
	: m_minXSep(minXSep), m_ySep(ySep)
{
	m_balanced = true;
}

FastSimpleHierarchyLayout::FastSimpleHierarchyLayout(bool downward, bool leftToRight, int minXSep, int ySep)
	: m_minXSep(minXSep), m_ySep(ySep), m_downward(downward), m_leftToRight(leftToRight)
{
	m_balanced = false;
}

FastSimpleHierarchyLayout::FastSimpleHierarchyLayout(const FastSimpleHierarchyLayout &fshl)
{
	m_minXSep = fshl.m_minXSep;
	m_ySep = fshl.m_ySep;
	m_balanced = fshl.m_balanced;
	m_downward = fshl.m_downward;
	m_leftToRight = fshl.m_leftToRight;
}


FastSimpleHierarchyLayout::~FastSimpleHierarchyLayout() { }


FastSimpleHierarchyLayout &FastSimpleHierarchyLayout::operator=(const FastSimpleHierarchyLayout &fshl)
{
	m_minXSep = fshl.m_minXSep;
	m_ySep = fshl.m_ySep;
	m_balanced = fshl.m_balanced;
	m_downward = fshl.m_downward;
	m_leftToRight = fshl.m_leftToRight;

	return *this;
}


void FastSimpleHierarchyLayout::doCall(const Hierarchy& H, GraphCopyAttributes &AGC)
{
	const GraphCopy& GC = H;
	node v;
	NodeArray<node> align(GC);
	NodeArray<node> root(GC);

	if (m_balanced) {
		// the x positions; x=-1 <=> x is undefined
		NodeArray<int> x[4];
		int width[4];
		int min[4];
		int max[4];
		int minWidthLayout = 0;

		// initializing
		for (int i = 0; i < 4; i++) {
			min[i] = std::numeric_limits<int>::max();
			max[i] = std::numeric_limits<int>::min();
		}

		// calc the layout for down/up and leftToRight/rightToLeft
		for (int downward = 0; downward <= 1; downward++) {
			NodeArray<NodeArray<bool> > type1Conflicts = markType1Conflicts(H, downward == 0);
			for (int leftToRight = 0; leftToRight <= 1; leftToRight++) {
				verticalAlignment(H, root, align, type1Conflicts, downward == 0, leftToRight == 0);
				horizontalCompactation(align, H, root, x[2 * downward + leftToRight], leftToRight == 0, downward == 0);
			}
		}

		/*
		* - calc min/max x coordinate for each layout
		* - calc x-width for each layout
		* - find the layout with the minimal width
		*/
		for (int i = 0; i < 4; i++) {
			forall_nodes(v, GC) {
				if (min[i] > x[i][v]) {
					min[i] = x[i][v];
				}
				if (max[i] < x[i][v]) {
					max[i] = x[i][v];
				}
			}
			width[i] = max[i] - min[i];
			if (width[minWidthLayout] > width[i]) {
				minWidthLayout = i;
			}
		}

		/*
		* shift the layout so that they align with the minimum width layout
		* - leftToRight: align minimum coordinate
		* - rightToLeft: align maximum coordinate
		*/
		int shift[4];
		for (int i = 0; i < 4; i++) {
			if (i % 1 == 0) {
				// for leftToRight layouts
				shift[i] = min[minWidthLayout] - min[i];
			} else {
				// for rightToLeft layouts
				shift[i] = max[minWidthLayout] - max[i];
			}
		}

		/*
		* shift the layouts and use the
		* median average coordinate for each node
		*/
		Array<int> sorting(4);
		forall_nodes(v, GC) {
			for (int i = 0; i < 4; i++) {
				sorting[i] = x[i][v] + shift[i];
			}
			sorting.quicksort();
			AGC.x(v) = ((double)sorting[1] + (double)sorting[2]) / 2.0;
			AGC.y(v) = H.rank(v) * m_ySep;
		}
	} else {
		NodeArray<int> x;
		NodeArray<NodeArray<bool> > type1Conflicts = markType1Conflicts(H, m_downward);
		verticalAlignment(H, root, align, type1Conflicts, m_downward, m_leftToRight);
		horizontalCompactation(align, H, root, x, m_leftToRight, m_downward);
		forall_nodes(v, GC) {
			AGC.x(v) = x[v];
			AGC.y(v) = H.rank(v) * m_ySep;
		}
	}
}

NodeArray<NodeArray<bool> > FastSimpleHierarchyLayout::markType1Conflicts(const Hierarchy &H, const bool downward)
{
	const GraphCopy& GC = H;
	NodeArray<NodeArray<bool> > type1Conflicts(GC);
	node v;

	forall_nodes(v, GC) {
		NodeArray<bool> array(GC, false);
		type1Conflicts[v] = array;
	}

	if (H.size() >= 4) {
		int upper, lower; 	// iteration bounds
		int k0, k1; 		// node position boundaries of closest inner segments
		int l; 				// node position on current level
		Hierarchy::TraversingDir relupward; // upward relativ to the direction from downward

		if (downward) {
			lower = 1;
			upper = H.high() - 2;

			relupward = Hierarchy::downward;
		}
		else {
			lower = H.high() - 1;
			upper = 2;

			relupward = Hierarchy::upward;
		}

		/*
		 * iterate level[2..h-2] in the given direction
		 *
		 * availible levels: 1 to h
		 */
		for (int i = lower; (downward && i <= upper) || (!downward && i >= upper); i = downward ? i + 1 : i - 1) {
			k0 = 0;
			l = 0; 			// index of first node on layer
			Level currentLevel = H[i];
			Level nextLevel = downward ? H[i+1] : H[i-1];

			// for all nodes on next level
			for (int l1 = 0; l1 <= nextLevel.high(); l1++) {
				const node virtualTwin = virtualTwinNode(H, nextLevel[l1], relupward);

				if (l1 == nextLevel.high() || virtualTwin != NULL) {
					k1 = currentLevel.high();

					if (virtualTwin != NULL) {
						k1 = H.pos(virtualTwin);
					}

					for (; l <= l1; l++) {
						Array<node> upperNeighbours = H.adjNodes(nextLevel[l1], relupward);

						for (int i = 0; i < upperNeighbours.size(); i++) {
							node currentNeighbour = upperNeighbours[i];

							/*
							 * XXX: < 0 in first iteration is still ok for indizes starting
							 * with 0 because no index can be smaller than 0
							 */
							if (H.pos(currentNeighbour) < k0 || H.pos(currentNeighbour) > k1) {
								(type1Conflicts[l1])[currentNeighbour] = true;
							}
						}
					}
					k0 = k1;
				}
			}
		}
	}
	return type1Conflicts;
}

void FastSimpleHierarchyLayout::verticalAlignment(const Hierarchy &H, NodeArray<node> &root,
		NodeArray<node> &align, const NodeArray<NodeArray<bool> > &type1Conflicts,
		bool downward, const bool leftToRight)
{
	const GraphCopy& GC = H;
	node v, u;
	int r;
	int median;
	Hierarchy::TraversingDir relupward;		// upward relativ to the direction from downward

	int medianCount;

	relupward = downward ? Hierarchy::downward : Hierarchy::upward;

	// initialize root and align
	forall_nodes(v, GC) {
		root[v] = v;
		align[v] = v;
	}

	// for all Level
	for (int i = downward ? 0 : H.high();
		 (downward && i <= H.high()) || (!downward && i >= 0); i = downward ? i + 1 : i - 1) {
		Level currentLevel = H[i];
		r = leftToRight ? -1 : std::numeric_limits<int>::max();

		// for all nodes on Level i (with direction leftToRight)
		for (int j = leftToRight ? 0 : currentLevel.high();
			 (leftToRight && j <= currentLevel.high()) || (!leftToRight && j >= 0); leftToRight ? j++ : j--) {
			v = currentLevel[j];
			// the fist median
			median = (int)floor((H.adjNodes(v, relupward).size() + 1) / 2.0);

			medianCount = (H.adjNodes(v, relupward).size() % 2 == 1) ? 1 : 2;
			if (H.adjNodes(v, relupward).size() == 0) {
				medianCount = 0;
			}

			// for all median neighbours in direction of H
			for (int count = 0; count < medianCount; count++) {
				u = H.adjNodes(v, relupward)[median + count - 1];

				if (align[v] == v) {
					// if segment (u,v) not marked by type1 conflicts AND ...
					if ((type1Conflicts[v])[u] == false &&
						((leftToRight && r < H.pos(u)) || (!leftToRight && r > H.pos(u)))) {
						align[u] = v;
						root[v] = root[u];
						align[v] = root[v];
						r = H.pos(u);
					}
				}
			}
		}
	}

#ifdef DEBUG_OUTPUT
	forall_nodes(v, GC) {
		cout << "node: " << GC.original(v) << "/" << v << ", root: " << GC.original(root[v]) << "/" << root[v] << ", alignment: " << GC.original(align[v]) << "/" << align[v] << endl;
	}
#endif
}

void FastSimpleHierarchyLayout::horizontalCompactation(const NodeArray<node> &align,
		const Hierarchy &H, const NodeArray<node> root, NodeArray<int> &x, const bool leftToRight, bool downward)
{
#ifdef DEBUG_OUTPUT
	cout << "-------- Horizontal Compactation --------" << endl;
#endif

	const GraphCopy& GC = H;
	node v;
	NodeArray<node> sink(GC);
	NodeArray<int> shift(GC, std::numeric_limits<int>::max());

	x.init(GC, -1);

	forall_nodes(v, GC) {
		sink[v] = v;
	}

	// calculate class relative coordinates for all roots
	for (int i = downward ? 0 : H.high();
		(downward && i <= H.high()) || (!downward && i >= 0); i = downward ? i + 1 : i - 1) {
		Level currentLevel = H[i];

		for (int j = leftToRight ? 0 : currentLevel.high();
			(leftToRight && j <= currentLevel.high()) || (!leftToRight && j >= 0); leftToRight ? j++ : j--) {
			v = currentLevel[j];
			if (root[v] == v) {
				placeBlock(v, sink, shift, x, align, H, root, leftToRight);
			}
		}
	}

#ifdef DEBUG_OUTPUT
	cout << "------- Sinks ----------" << endl;
#endif
	// apply root coordinates for all aligned nodes
	// (place block did this only for the roots)
	forall_nodes(v, GC) {
#ifdef DEBUG_OUTPUT
		if (sink[root[v]] == v) {
			cout << "Topmost Root von Senke!: " << GC.original(v) << endl;
			cout << "-> Shift: " << shift[v] << endl;
			cout << "-> x: " << x[v] << endl;
		}
#endif
		x[v] = x[root[v]];
	}

	// apply shift for each class
	forall_nodes(v, GC) {
		if (shift[sink[root[v]]] < std::numeric_limits<int>::max()) {
			x[v] = x[v] + shift[sink[root[v]]];
		}
	}
}

void FastSimpleHierarchyLayout::placeBlock(node v, NodeArray<node> &sink,
		NodeArray<int> &shift, NodeArray<int> &x, const NodeArray<node> &align,
		const Hierarchy &H, const NodeArray<node> &root, const bool leftToRight)
{
	node w;
	node u;

#ifdef DEBUG_OUTPUT
	const GraphCopy& GC = H;
#endif

	if (x[v] == -1) {
		x[v] = 0;
		w = v;
#ifdef DEBUG_OUTPUT
		cout << "---placeblock: " << GC.original(v) << " ---" << endl;
#endif
		do {
			// if not first node on layer
			if ((leftToRight && H.pos(w) > 0) || (!leftToRight && H.pos(w) < H[H.rank(w)].high())) {
				u = root[pred(w, H, leftToRight)];
				placeBlock(u, sink, shift, x, align, H, root, leftToRight);
				if (sink[v] == v) {
					sink[v] = sink[u];
				}
				if (sink[v] != sink[u]) {
#ifdef DEBUG_OUTPUT
					cout << "old shift " << GC.original(sink[u]) << ": " << shift[sink[u]] << "<>" << x[v] - x[u] - m_minXSep << endl;
#endif
					if (leftToRight) {
						shift[sink[u]] = min<int>(shift[sink[u]], x[v] - x[u] - m_minXSep);
					} else {
						shift[sink[u]] = max<int>(shift[sink[u]], x[v] - x[u] + m_minXSep);
					}
#ifdef DEBUG_OUTPUT
					cout << "-> new shift: " << shift[sink[u]] << endl;
#endif
				}
				else {
					if (leftToRight) {
						x[v] = max<int>(x[v], x[u] + m_minXSep);
					} else {
						x[v] = min<int>(x[v], x[u] - m_minXSep);
					}
				}
#ifdef DEBUG_OUTPUT
				cout << "placing w: " << GC.original(w) << "; predecessor: " << GC.original(pred(w, H, leftToRight)) <<
					"; root(w)=v: " << GC.original(v) << "; root(pred(u)): " << GC.original(u) <<
					"; sink(v): " << GC.original(sink[v]) << "; sink(u): " << GC.original(sink[u]) << endl;
				cout << "x(v): " << x[v] << endl;
			} else {
				cout << "not placing w: " << GC.original(w) << " because at beginning of layer" << endl;
#endif
			}
			w = align[w];
		} while (w != v);
#ifdef DEBUG_OUTPUT
		cout << "---END placeblock: " << GC.original(v) << " ---" << endl;
#endif
	}
}

node FastSimpleHierarchyLayout::virtualTwinNode(const Hierarchy &H, const node v, const Hierarchy::TraversingDir dir) const
{
	if (!H.isLongEdgeDummy(v) || H.adjNodes(v, dir).size() == 0) {
		return NULL;
	}

	if (H.adjNodes(v, dir).size() > 1) {
		// since v is a dummy there sould be only one upper neighbour
		throw AlgorithmFailureException("FastSimpleHierarchyLayout.cpp");
	}

	return *H.adjNodes(v, dir).begin();
}

node FastSimpleHierarchyLayout::pred(const node v, const Hierarchy &H, const bool leftToRight)
{
	int pos = H.pos(v);
	int rank = H.rank(v);

	Level level = H[rank];
	if ((leftToRight && pos != 0) || (!leftToRight && pos != level.high())) {
		return level[leftToRight ? pos - 1 : pos + 1];
	}
	else {
		return NULL;
	}
}

} // end namespace ogdf
