/*
 * $Revision: 2552 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-05 16:45:20 +0200 (Do, 05. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of heuristics for two-layer crossing
 * minimization (BarycenterHeuristic, MedianHeuristic)
 *
 * \author Carsten Gutwenger, Till Sch&auml;fer
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


#include <ogdf/layered/BarycenterHeuristic.h>
#include <ogdf/layered/MedianHeuristic.h>
#include <ogdf/layered/GreedyInsertHeuristic.h>
#include <ogdf/layered/GreedySwitchHeuristic.h>
#include <ogdf/layered/SiftingHeuristic.h>


namespace ogdf {


//---------------------------------------------------------
// BarycenterHeuristic
// implements the barycenter heuristic for 2-layer
// crossing minimization
//---------------------------------------------------------

void BarycenterHeuristic::call (Level &L)
{
	const Hierarchy& H = L.hierarchy();

	for (int i = 0; i <= L.high(); ++i) {
		node v = L[i];
		long sumpos = 0L;

		const Array<node> &adjNodes = L.adjNodes(v);
		for(int j = 0; j <= adjNodes.high(); ++j)
			sumpos += H.pos(adjNodes[j]);

		m_weight[v] = (adjNodes.high() < 0) ? 0.0 :
			double(sumpos) / double(adjNodes.size());
	}

	L.sort(m_weight);
}


//---------------------------------------------------------
// MedianHeuristic
// implements the median heuristic for 2-layer
// crossing minimization
//---------------------------------------------------------

void MedianHeuristic::call (Level &L)
{
	const Hierarchy& H = L.hierarchy();

	for (int i = 0; i <= L.high(); ++i) {
		node v = L[i];

		const Array<node> &adjNodes = L.adjNodes(v);
		const int high = adjNodes.high();

		if (high < 0) m_weight[v] = 0;
		else if (high & 1)
			m_weight[v] = H.pos(adjNodes[high/2]) + H.pos(adjNodes[1+high/2]);
		else
			m_weight[v] = 2*H.pos(adjNodes[high/2]);
	}

	L.sort(m_weight,0,2*H.adjLevel(L.index()).high());
}


//---------------------------------------------------------
// GreedySwitchHeuristic
// implements the greedy switch heuristic for 2-layer
// crossing minimization
//---------------------------------------------------------

void GreedySwitchHeuristic::init (const Hierarchy& H)
{
	m_crossingMatrix = new CrossingsMatrix(H);
}

void GreedySwitchHeuristic::cleanup()
{
	delete m_crossingMatrix;
}

void GreedySwitchHeuristic::call (Level &L)
{
	m_crossingMatrix->init(L);
	int index;
	bool nolocalmin;

	do {
		nolocalmin = false;

		for (index = 0; index < L.size() - 1; index++)
			if ((*m_crossingMatrix)(index,index+1) > (*m_crossingMatrix)(index+1,index)) {

				nolocalmin = true;

				L.swap(index,index+1);
				m_crossingMatrix->swap(index,index+1);
			}
	} while (nolocalmin);
}


//---------------------------------------------------------
// GreedyInsertHeuristic
// implements the greedy insert heuristic for 2-layer
// crossing minimization
//---------------------------------------------------------

void GreedyInsertHeuristic::init (const Hierarchy& H)
{
	m_weight.init(H);
	m_crossingMatrix = new CrossingsMatrix(H);
}

void GreedyInsertHeuristic::cleanup()
{
	m_weight.init();
	delete m_crossingMatrix;
}

void GreedyInsertHeuristic::call(Level &L)
{
	m_crossingMatrix->init(L);
	int index, i;

	// initialisation & priorisation
	for (i = 0; i < L.size(); i++) {
		double prio = 0;
		for (index = 0; index < L.size(); index++)
			prio += (*m_crossingMatrix)(i,index);

		// stable quicksort: no need for unique prio
		m_weight[L[i]] = prio;
	}

	L.sort(m_weight);
}


//---------------------------------------------------------
// SiftingHeuristic
// implements the sifting heuristic for 2-layer
// crossing minimization
//---------------------------------------------------------

void SiftingHeuristic::init (const Hierarchy& H)
{
	m_crossingMatrix = new CrossingsMatrix(H);
}

void SiftingHeuristic::cleanup()
{
	delete m_crossingMatrix;
}

void SiftingHeuristic::call(Level &L)
{
	List<node> vertices;
	int i;

	const int n = L.size();

	m_crossingMatrix->init(L); // initialize crossing matrix

	if (m_strategy == left_to_right || m_strategy == random) {
		for (i = 0; i < n; i++) {
			vertices.pushBack(L[i]);
		}

		if (m_strategy == random) {
			vertices.permute();
		}

	} else { // m_strategy == desc_degree
		int max_deg = 0;

		for (i = 0; i < n; i++) {
			int deg = L.adjNodes(L[i]).size();
			if (deg > max_deg) max_deg = deg;
		}

		Array<List<node>, int> bucket(0, max_deg);
		for (i = 0; i < n; i++) {
			bucket[L.adjNodes(L[i]).size()].pushBack(L[i]);
		}

		for (i = max_deg; i >= 0; i--) {
			while(!bucket[i].empty()) {
				vertices.pushBack(bucket[i].popFrontRet());
			}
		}
	}

	for(i = 0; i< vertices.size(); i++) {
		int dev = 0;

		// sifting left
		for(; i > 0; --i) {
			dev = dev - (*m_crossingMatrix)(i-1,i) + (*m_crossingMatrix)(i,i-1);
			L.swap(i-1,i);
			m_crossingMatrix->swap(i-1,i);
		}

		// sifting right and searching optimal position
		int opt = dev, opt_pos = 0;
		for (; i < n-1; ++i) {
			dev = dev - (*m_crossingMatrix)(i,i+1) + (*m_crossingMatrix)(i+1,i);
			L.swap(i,i+1);
			m_crossingMatrix->swap(i,i+1);
			if (dev <= opt) {
				opt = dev; opt_pos = i+1;
			}
		}

		// set optimal position
		for (; i > opt_pos; --i) {
			L.swap(i-1,i);
			m_crossingMatrix->swap(i-1,i);
		}
	}
}

} // namespace ogdf
