/*
 * $Revision: 2552 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-05 16:45:20 +0200 (Do, 05. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of node ranking algorithms
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


#include <ogdf/layered/LongestPathRanking.h>
#include <ogdf/layered/DfsAcyclicSubgraph.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/basic/GraphCopy.h>
#include <ogdf/basic/Queue.h>


namespace ogdf {


//---------------------------------------------------------
// LongestPathRanking
// linear-time node ranking for hierarchical graphs
//---------------------------------------------------------

LongestPathRanking::LongestPathRanking()
{
	m_subgraph.set(new DfsAcyclicSubgraph);
	m_sepDeg0 = true;
	m_separateMultiEdges = true;
	m_optimizeEdgeLength = true;
	m_alignBaseClasses = false;
	m_alignSiblings = false;
}


void LongestPathRanking::call(const Graph &G, const EdgeArray<int> &length, NodeArray<int> &rank)
{
	List<edge> R;

	m_subgraph.get().call(G,R);

	EdgeArray<bool> reversed(G,false);
	for (ListConstIterator<edge> it = R.begin(); it.valid(); ++it)
		reversed[*it] = true;
	R.clear();

	doCall(G, rank, reversed, length);
}


void LongestPathRanking::call (const Graph& G, NodeArray<int> &rank)
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

	doCall(G, rank, reversed, length);
}


void LongestPathRanking::callUML(const GraphAttributes &AG, NodeArray<int> &rank)
{
	const Graph &G = AG.constGraph();

	// find base classes
	List<node> baseClasses;
	node v;
	forall_nodes(v,G) {
		bool isBase = false;
		edge e;
		forall_adj_edges(e,v) {
			if(AG.type(e) != Graph::generalization)
				continue;

			if(e->target() == v) {
				// incoming edge
				isBase = true; // possible base of a hierarchy
			}
			if(e->source() == v) {
				// outgoing edge
				isBase = false;
				break;
			}
		}
		if(isBase)
			baseClasses.pushBack(v);
	}

	// insert a super sink
	GraphCopySimple GC(G);
	makeLoopFree(GC);
	GraphAttributes AGC(GC,GraphAttributes::edgeType);

	node superSink = GC.newNode();

	ListConstIterator<node> it;
	for(it = baseClasses.begin(); it.valid(); ++it) {
		edge ec = GC.newEdge(GC.copy(*it), superSink);
		AGC.type(ec) = Graph::generalization;
	}

	edge e;
	forall_edges(e,G)
		AGC.type(GC.copy(e)) = AG.type(e);

	// compute length of edges
	EdgeArray<int> length(GC,1);

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
					length[GC.copy(e)] = 2;
				else {
					prevSrc = minIndex[e];
					prevTgt = maxIndex[e];
				}
			}
		}
	}

	// compute spanning tree
	// marked edges belong to tree
	NodeArray<int> outdeg(GC,0);
	forall_nodes(v,GC) {
		forall_adj_edges(e,v)
			if(!e->isSelfLoop() && e->source() == v &&
				AGC.type(e) == Graph::generalization /*&& reversed[e] == true*/)
				++outdeg[v];
	}

	Queue<node> Q;
	Q.append(superSink);
	EdgeArray<bool> marked(GC,false);
	while(!Q.empty()) {
		v = Q.pop();
		forall_adj_edges(e,v) {
			node u = e->source();
			if(u == v || AGC.type(e) != Graph::generalization/* || reversed[e] == false*/)
				continue;

			--outdeg[u];
			if(outdeg[u] == 0) {
				marked[e] = true;
				Q.append(u);
			}
		}
	}

	outdeg.init();

	// build super graph on which we will compute the ranking
	// we join nodes that have to be placed on the same level to super nodes
	NodeArray<node> superNode(G,0);
	NodeArray<SListPure<node> > joinedNodes(GC);

	// initially, there is a single node in GC for every node in G
	forall_nodes(v,G) {
		node vc = GC.copy(v);
		superNode[v] = vc;
		joinedNodes[vc].pushBack(v);
	}

	if(m_alignBaseClasses && baseClasses.size() >= 2) {
		ListConstIterator<node> it = baseClasses.begin();
		node v1 = superNode[*it++];
		for(; it.valid(); ++it)
			join(GC,superNode,joinedNodes,v1,superNode[*it]);
	}

	// not needed anymore
	GC.delNode(superSink);
	baseClasses.clear();

	if(m_alignSiblings) {
		NodeArray<SListPure<node> > toJoin(GC);

		forall_nodes(v,GC) {
			node v1 = 0;
			forall_adj_edges(e,v) {
				if(marked[e] == false || e->source() == v)
					continue;

				node u = e->source();
				if(v1 == 0)
					v1 = u;
				else
					toJoin[v1].pushBack(u);
			}
		}

		forall_nodes(v,GC) {
			SListConstIterator<node> it;
			for(it = toJoin[v].begin(); it.valid(); ++it)
				join(GC,superNode,joinedNodes,v,*it);
		}
	}

	marked.init();
	joinedNodes.init();

	// don't want self-loops
	makeLoopFree(GC);

	// determine reversed edges
	DfsAcyclicSubgraph sub;
	List<edge> R;
	sub.callUML(AGC,R);

	EdgeArray<bool> reversed(GC,true);
	for (ListConstIterator<edge> itE = R.begin(); itE.valid(); ++itE)
		reversed[*itE] = false;
	R.clear();

	// compute ranking of GC
	NodeArray<int> rankGC;
	doCall(GC, rankGC, reversed, length);

	// transfer to ranking of G
	rank.init(G);
	forall_nodes(v,G)
		rank[v] = rankGC[superNode[v]];
}


void LongestPathRanking::join(
	GraphCopySimple &GC,
	NodeArray<node> &superNode,
	NodeArray<SListPure<node> > &joinedNodes,
	node v, node w)
{
	OGDF_ASSERT(v != w);

	SListConstIterator<node> it;
	for(it = joinedNodes[w].begin(); it.valid(); ++it)
		superNode[*it] = v;

	joinedNodes[v].conc(joinedNodes[w]);

	SListPure<edge> edges;
	GC.adjEdges(w,edges);
	SListConstIterator<edge> itE;
	for(itE = edges.begin(); itE.valid(); ++itE) {
		edge e = *itE;
		if(e->source() == w)
			GC.moveSource(e, v);
		else
			GC.moveTarget(e, v);
	}

	GC.delNode(w);
}


void LongestPathRanking::doCall(
	const Graph& G,
	NodeArray<int> &rank,
	EdgeArray<bool> &reversed,
	const EdgeArray<int> &length)
{
	rank.init(G,0);

	m_isSource.init(G,true);
	m_adjacent.init(G);

	edge e;
	forall_edges(e,G) {
		if (e->isSelfLoop()) continue;

		if (!reversed[e]) {
			m_adjacent[e->source()].pushBack(Tuple2<node,int>(e->target(),length[e]));
			m_isSource[e->target()] = false;
		} else {
			m_adjacent[e->target()].pushBack(Tuple2<node,int>(e->source(),length[e]));
			m_isSource[e->source()] = false;
		}
	}

	m_ingoing.init(G,0);

	if(m_optimizeEdgeLength) {
		m_finished.init(G,false);

		int min = 0, max = 0;
		m_maxN = G.numberOfNodes();

		node v;
		forall_nodes(v,G)
			if (m_isSource[v]) {
				dfs(v);
				getTmpRank(v,rank);
				dfsAdd(v,rank);

				if (rank[v] < min) min = rank[v];
			}

		forall_nodes(v,G) {
			if ((rank[v] -= min) > max) max = rank[v];
		}

		if (max > 0 && separateDeg0Layer()) {
			max++;
			forall_nodes(v,G)
				if (v->degree() == 0) rank[v] = max;
		}

		m_finished.init();

	} else {
		SListPure<node> sources;
		SListConstIterator<Tuple2<node,int> > it;

		node v;
		forall_nodes(v,G) {
			if(m_isSource[v])
				sources.pushBack(v);
			for(it = m_adjacent[v].begin(); it.valid(); ++it)
				++m_ingoing[(*it).x1()];
		}

		while(!sources.empty()) {
			v = sources.popFrontRet();

			for(it = m_adjacent[v].begin(); it.valid(); ++it) {
				node u = (*it).x1();
				int r = rank[v]+(*it).x2();
				if(r > rank[u])
					rank[u] = r;

				if (--m_ingoing[u] == 0)
					sources.pushBack(u);
			}
		}
	}

	m_isSource.init();
	m_adjacent.init();
	m_ingoing .init();
}


void LongestPathRanking::dfs(node v)
{
	m_ingoing[v]++;
	if (m_ingoing[v] == 1 && !m_finished[v]) {
		SListConstIterator<Tuple2<node,int> > it;
		for(it = m_adjacent[v].begin(); it.valid(); ++it)
			dfs((*it).x1());
	}
}


void LongestPathRanking::getTmpRank(node v, NodeArray<int> &rank)
{
	List<node> N;

	m_offset = m_maxN;
	N.pushBack(v);
	rank[v] = 0;

	while (!N.empty()) {
		node w = N.front(); N.popFront();

		SListConstIterator<Tuple2<node,int> > it;
		for(it = m_adjacent[w].begin(); it.valid(); ++it) {
			node u = (*it).x1();

			int r = max(rank[u],rank[w]+(*it).x2());

			m_ingoing[u]--;
			if (m_finished[u])
				m_offset = min(m_offset, rank[u] - rank[w]-(*it).x2());

			else {
				if (m_ingoing[u] == 0) {
					N.pushBack(u);
				}
				rank[u] = r;
			}
		}
	}
	if (m_offset == m_maxN) m_offset = 0;
}


void LongestPathRanking::dfsAdd(node v, NodeArray<int> &rank)
{
	if (!m_finished[v]) {
		m_finished[v] = true;
		rank[v] += m_offset;

		SListConstIterator<Tuple2<node,int> > it;
		for(it = m_adjacent[v].begin(); it.valid(); ++it)
			dfsAdd((*it).x1(),rank);
	}
}



} // end namespace ogdf
