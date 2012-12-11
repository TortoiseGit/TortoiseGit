/*
 * $Revision: 2552 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-05 16:45:20 +0200 (Do, 05. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of optimal node ranking algorithm
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


#include <ogdf/layered/OptimalRanking.h>
#include <ogdf/layered/DfsAcyclicSubgraph.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/graphalg/MinCostFlowReinelt.h>
#include <ogdf/basic/GraphCopy.h>


namespace ogdf {


//---------------------------------------------------------
// OptimalRanking
// optimal node ranking for hierarchical graphs using min-cost flow
//---------------------------------------------------------

OptimalRanking::OptimalRanking()
{
	m_subgraph.set(new DfsAcyclicSubgraph);
	m_separateMultiEdges = true;
}


void OptimalRanking::call(const Graph &G, const EdgeArray<int> &length, NodeArray<int> &rank)
{
	EdgeArray<int> cost(G,1);
	call(G, length, cost, rank);
}


void OptimalRanking::call(
	const Graph &G,
	const EdgeArray<int> &length,
	const EdgeArray<int> &cost,
	NodeArray<int> &rank)
{
	List<edge> R;

	m_subgraph.get().call(G,R);

	EdgeArray<bool> reversed(G,false);
	for (ListConstIterator<edge> it = R.begin(); it.valid(); ++it)
		reversed[*it] = true;
	R.clear();

	doCall(G, rank, reversed, length, cost);
}


void OptimalRanking::call (const Graph& G, NodeArray<int> &rank)
{
	List<edge> R;

	m_subgraph.get().call(G,R);

	EdgeArray<bool> reversed(G,false);
	for (ListConstIterator<edge> it = R.begin(); it.valid(); ++it)
		reversed[*it] = true;
	R.clear();

	EdgeArray<int> length(G,1);

	if(m_separateMultiEdges) {
		SListPure<edge> edges;
		EdgeArray<int> minIndex(G), maxIndex(G);
		parallelFreeSortUndirected(G, edges, minIndex, maxIndex);

		SListConstIterator<edge> it = edges.begin();
		if(it.valid())
		{
			int prevSrc = minIndex[*it];
			int prevTgt = maxIndex[*it];

			for(it = it.succ(); it.valid(); ++it) {
				edge e = *it;
				if (minIndex[e] == prevSrc && maxIndex[e] == prevTgt)
					length[e] = 2;
				else {
					prevSrc = minIndex[e];
					prevTgt = maxIndex[e];
				}
			}
		}
	}

	EdgeArray<int> cost(G,1);
	doCall(G, rank, reversed, length, cost);
}


void OptimalRanking::doCall(
	const Graph& G,
	NodeArray<int> &rank,
	EdgeArray<bool> &reversed,
	const EdgeArray<int> &length,
	const EdgeArray<int> &costOrig)
{
	MinCostFlowReinelt mcf;

	// construct min-cost flow problem
	GraphCopy GC;
	GC.createEmpty(G);

	// compute connected component of G
	NodeArray<int> component(G);
	int numCC = connectedComponents(G,component);

	// intialize the array of lists of nodes contained in a CC
	Array<List<node> > nodesInCC(numCC);

	node v;
	forall_nodes(v,G)
		nodesInCC[component[v]].pushBack(v);

	EdgeArray<edge> auxCopy(G);
	rank.init(G);

	for(int i = 0; i < numCC; ++i)
	{
		GC.initByNodes(nodesInCC[i], auxCopy);
		makeLoopFree(GC);

		edge e;
		forall_edges(e,GC)
			if(reversed[GC.original(e)])
				GC.reverseEdge(e);

		// special cases:
		if(GC.numberOfNodes() == 1) {
			rank[GC.original(GC.firstNode())] = 0;
			continue;
		} else if(GC.numberOfEdges() == 1) {
			e = GC.original(GC.firstEdge());
			rank[e->source()] = 0;
			rank[e->target()] = length[e];
			continue;
		}

		EdgeArray<int> lowerBound(GC,0);
		EdgeArray<int> upperBound(GC,mcf.infinity());
		EdgeArray<int> cost(GC);
		NodeArray<int> supply(GC);

		forall_edges(e,GC)
			cost[e] = -length[GC.original(e)];

		node v;
		forall_nodes(v,GC) {
			int s = 0;
			forall_adj_edges(e,v) {
				if(v == e->source())
					s += costOrig[GC.original(e)];
				else
					s -= costOrig[GC.original(e)];
			}
			supply[v] = s;
		}

		OGDF_ASSERT(isAcyclic(GC) == true);

		// find min-cost flow
		EdgeArray<int> flow(GC);
		NodeArray<int> dual(GC);
#ifdef OGDF_DEBUG
		bool feasible =
#endif
			mcf.call(GC, lowerBound, upperBound, cost, supply, flow, dual);
		OGDF_ASSERT(feasible);

		forall_nodes(v,GC)
			rank[GC.original(v)] = dual[v];
	}
}


} // end namespace ogdf
