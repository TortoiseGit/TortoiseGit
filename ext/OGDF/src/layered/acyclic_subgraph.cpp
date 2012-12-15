/*
 * $Revision: 2552 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-05 16:45:20 +0200 (Do, 05. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of algorithms for computing an
 * acyclic subgraph (DfsAcyclicSubgraph, GreedyCycleRemovel)
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


#include <ogdf/layered/DfsAcyclicSubgraph.h>
#include <ogdf/layered/GreedyCycleRemoval.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/SList.h>
#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/basic/Queue.h>


namespace ogdf {


//---------------------------------------------------------
// DfsAcyclicSubgraph
// computes a maximal acyclic subgraph by deleting all DFS
// backedges (linear-time)
//---------------------------------------------------------

void DfsAcyclicSubgraph::call (const Graph &G, List<edge> &arcSet)
{
	isAcyclic(G,arcSet);
}


void DfsAcyclicSubgraph::callUML (
	const GraphAttributes &AG,
	List<edge> &arcSet)
{
	const Graph &G = AG.constGraph();

	// identify hierarchies
	NodeArray<int> hierarchy(G,-1);
	int count = 0;
	int treeNum = -1;

	node v;
	forall_nodes(v,G)
	{
		if(hierarchy[v] == -1) {
			int n = dfsFindHierarchies(AG,hierarchy,count,v);
			if(n > 1) treeNum = count;
			++count;
		}
	}

	arcSet.clear();

	// perform DFS on the directed graph formed by generalizations
	NodeArray<int> number(G,0), completion(G);
	int nNumber = 0, nCompletion = 0;

	forall_nodes(v,G) {
		if(number[v] == 0)
			dfsBackedgesHierarchies(AG,v,number,completion,nNumber,nCompletion);
	}

	// collect all backedges within a hierarchy
	// and compute outdeg of each vertex within its hierarchy
	EdgeArray<bool> reversed(G,false);
	NodeArray<int> outdeg(G,0);

	edge e;
	forall_edges(e,G) {
		if(AG.type(e) != Graph::generalization || e->isSelfLoop())
			continue;

		node src = e->source(), tgt = e->target();

		outdeg[src]++;

		if (hierarchy[src] == hierarchy[tgt] &&
			number[src] >= number[tgt] && completion[src] <= completion[tgt])
			reversed[e] = true;
	}

	// topologial numbering of nodes within a hierarchy (for each hierarchy)
	NodeArray<int> numV(G);
	Queue<node> Q;
	int countV = 0;

	forall_nodes(v,G)
		if(outdeg[v] == 0)
			Q.append(v);

	while(!Q.empty()) {
		v = Q.pop();

		numV[v] = countV++;

		forall_adj_edges(e,v) {
			node w = e->source();
			if(w != v) {
				if(--outdeg[w] == 0)
					Q.append(w);
			}
		}
	}

	// "direct" associations
	forall_edges(e,G) {
		if(AG.type(e) == Graph::generalization || e->isSelfLoop())
			continue;

		node src = e->source(), tgt = e->target();

		if(hierarchy[src] == hierarchy[tgt]) {
			if (numV[src] < numV[tgt])
				reversed[e] = true;
		} else {
			if(hierarchy[src] == treeNum || (hierarchy[tgt] != treeNum &&
				hierarchy[src] > hierarchy[tgt]))
				reversed[e] = true;
		}
	}

	// collect reversed edges
	forall_edges(e,G)
		if(reversed[e])
			arcSet.pushBack(e);
}


int DfsAcyclicSubgraph::dfsFindHierarchies(
	const GraphAttributes &AG,
	NodeArray<int> &hierarchy,
	int i,
	node v)
{
	int count = 1;
	hierarchy[v] = i;

	edge e;
	forall_adj_edges(e,v) {
		if(AG.type(e) != Graph::generalization)
			continue;

		node w = e->opposite(v);
		if(hierarchy[w] == -1)
			count += dfsFindHierarchies(AG,hierarchy,i,w);
	}

	return count;
}


void DfsAcyclicSubgraph::dfsBackedgesHierarchies(
	const GraphAttributes &AG,
	node v,
	NodeArray<int> &number,
	NodeArray<int> &completion,
	int &nNumber,
	int &nCompletion)
{
	number[v] = ++nNumber;

	edge e;
	forall_adj_edges(e,v)
	{
		if(AG.type(e) != Graph::generalization)
			continue;

		node w = e->target();

		if (number[w] == 0)
			dfsBackedgesHierarchies(AG,w,number,completion,nNumber,nCompletion);
	}

	completion[v] = ++nCompletion;
}




//---------------------------------------------------------
// GreedyCycleRemoval
// greedy heuristic for computing a maximal acyclic
// subgraph (linear-time)
//---------------------------------------------------------

void GreedyCycleRemoval::dfs (node v, const Graph &G)
{
	m_visited[v] = true;

	int i;
	if (v->outdeg() == 0) i = m_min;
	else if (v->indeg() == 0) i = m_max;
	else i = v->outdeg() - v->indeg();

	m_item[v] = m_B[m_index[v] = i].pushBack(v);
	m_in [v] = v->indeg();
	m_out[v] = v->outdeg();
	m_counter++;

	edge e;
	forall_adj_edges(e,v) {
		node u = e->opposite(v);
		if (!m_visited[u])
			dfs(u,G);
	}
}


void GreedyCycleRemoval::call (const Graph &G, List<edge> &arcSet)
{
	arcSet.clear();

	node u, v, w;
	edge e;

	m_max = m_min = 0;
	forall_nodes(v,G) {
		if (-v->indeg () < m_min) m_min = -v->indeg ();
		if ( v->outdeg() > m_max) m_max =  v->outdeg();
	}

	if (G.numberOfEdges() == 0) return;

	m_visited.init(G,false); m_item.init(G);
	m_in     .init(G);       m_out .init(G);
	m_index  .init(G);       m_B   .init(m_min,m_max);

	SListPure<node> S_l, S_r;
	NodeArray<int> pos(G);

	m_counter = 0;
	forall_nodes(v,G) {
		if (m_visited[v]) continue;
		dfs(v,G);

		int i, max_i = m_max-1, min_i = m_min+1;

		for ( ; m_counter > 0; m_counter--) {
			if (!m_B[m_min].empty()) {
				u = m_B[m_min].front(); m_B[m_min].popFront();
				S_r.pushFront(u);

			} else if (!m_B[m_max].empty()) {
				u = m_B[m_max].front(); m_B[m_max].popFront();
				S_l.pushBack(u);

			} else {
				while (m_B[max_i].empty())
					max_i--;
				while (m_B[min_i].empty())
					min_i++;

				if (abs(max_i) > abs(min_i)) {
					u = m_B[max_i].front(); m_B[max_i].popFront();
					S_l.pushBack(u);
				} else {
					u = m_B[min_i].front(); m_B[min_i].popFront();
					S_r.pushFront(u);
				}
			}

			m_item[u] = ListIterator<node>();

			forall_adj_edges(e,u) {
				if (e->target() == u) {
					w = e->source();
					if (m_item[w].valid()) {
						m_out[w]--; i = m_index[w];
						m_B[i].del(m_item[w]);
						if (m_out[w] == 0) i = m_min;
						else if (m_in[w] == 0) i = m_max;
						else i--;
						m_item[w] = m_B[m_index[w] = i].pushBack(w);

						if (m_index[w] < min_i)
							min_i = m_index[w];
					}
				} else {
					w = e->target();
					if (m_item[w].valid()) {
						m_in[w]--; i = m_index[w];
						m_B[i].del(m_item[w]);
						if (m_out[w] == 0) i = m_min;
						else if (m_in[w] == 0) i = m_max;
						else i++;
						m_item[w] = m_B[m_index[w] = i].pushBack(w);

						if (m_index[w] > max_i)
							max_i = m_index[w];
					}
				}
			}
		}

		SListConstIterator<node> it;
		for(i = 0, it = S_l.begin(); it.valid(); ++it)
			pos[*it] = i++;
		for(it = S_r.begin(); it.valid(); ++it)
			pos[*it] = i++;

		S_l.clear(); S_r.clear();
	}

	forall_edges(e,G)
		if (pos[e->source()] >= pos[e->target()])
			arcSet.pushBack(e);

	m_visited.init(); m_item.init();
	m_in     .init(); m_out .init();
	m_index  .init(); m_B.init();
}


} // end namespace ogdf
