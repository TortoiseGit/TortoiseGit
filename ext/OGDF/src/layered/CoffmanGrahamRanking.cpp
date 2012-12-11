/*
 * $Revision: 2552 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-05 16:45:20 +0200 (Do, 05. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Definition of coffman graham ranking algorithm for Sugiyama
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

#include <ogdf/layered/CoffmanGrahamRanking.h>
#include <ogdf/basic/ModuleOption.h>
#include <ogdf/layered/DfsAcyclicSubgraph.h>
#include <ogdf/basic/GraphCopy.h>

namespace ogdf {

CoffmanGrahamRanking::CoffmanGrahamRanking() : m_w(3)
{
	m_subgraph.set(new DfsAcyclicSubgraph());
}


void CoffmanGrahamRanking::call (const Graph& G, NodeArray<int>& rank)
{
	rank.init(G);
	GraphCopy gc(G);

	m_subgraph.get().callAndReverse(gc);
	removeTransitiveEdges(gc);

	List<Tuple2<node, int> > ready_nodes;
	NodeArray<int> deg(gc);
	NodeArray<int> pi(gc);
	m_s.init(gc);

	node v;
	List<edge> edges;

	forall_nodes(v,gc) {
		edges.clear();
		gc.inEdges(v, edges);
		deg[v] = edges.size();
		if (deg[v] == 0) {
			ready_nodes.pushBack(Tuple2<node,int>(v,0));
		}
		m_s[v].init(deg[v]);
	}

	int i = 1;
	while(!ready_nodes.empty()) {
		v = ready_nodes.popFrontRet().x1();
		pi[v] = i++;

		adjEntry adj;
		forall_adj(adj,v) {
			if ((adj->theEdge()->source()) == v) {
				node u = adj->twinNode();
				m_s[u].insert(pi[v]);
				if (--deg[u] == 0) {
					insert(u,ready_nodes);
				}
			}
		}
	}


	List<node> ready, waiting;

	forall_nodes(v,gc) {
		edges.clear();
		gc.outEdges(v, edges);
		deg[v] = edges.size();
		if (deg[v] == 0) {
			insert(v,ready,pi);  // ready.append(v);
		}
	}

	int k;
	// 	for all ranks
	for (k = 1; !ready.empty(); k++) {

		for (i = 1; i <= m_w && !ready.empty(); i++) {
			node u = ready.popFrontRet();
			rank[gc.original(u)] = k;

			gc.inEdges<List<edge> >(u, edges);
			for (ListIterator<edge> it = edges.begin(); it.valid() ; ++it){
				if (--deg[(*it)->source()] == 0){
					waiting.pushBack((*it)->source());
				}
			}
		}

		while (!waiting.empty()) {
			insert(waiting.popFrontRet(), ready, pi);
		}
	}

	k--;
	forall_nodes(v,G){
		rank[v] = k - rank[v];
	}

	m_s.init();
}


void CoffmanGrahamRanking::insert (node u, List<Tuple2<node,int> > &ready_nodes)
{
	int j = 0;

	for( ListIterator<Tuple2<node,int> > it = ready_nodes.rbegin(); it.valid(); --it) {
		node v     = (*it).x1();
		int  sigma = (*it).x2();

		if (sigma < j) {
			ready_nodes.insertAfter(Tuple2<node,int>(u,j), it);
			return;
		}

		if (sigma > j) continue;

		const _int_set &x = m_s[u], &y = m_s[v];
		int k = min(x.length(), y.length());

		while (j < k && x[j] == y[j]) {
			j++;
		}

		if (j == k) {
			if (x.length() < y.length()) continue;

			(*it).x2() = k;
			ready_nodes.insertAfter(Tuple2<node,int>(u,sigma), it);
			return;
		}

		if (x[j] < y[j]) continue;

		(*it).x2() = j;
		ready_nodes.insert(Tuple2<node,int>(u,sigma), it);
		return;
	}

	ready_nodes.pushFront(Tuple2<node,int>(u,j));
}


void CoffmanGrahamRanking::insert (node v, List<node> &ready, const NodeArray<int> &pi)
{
	for( ListIterator<node> it = ready.rbegin(); it.valid(); --it) {
		if (pi[v] <= pi[*it]) {
			ready.insertAfter(v, it);
			return;
		}
	}

	ready.pushFront(v);
}


void CoffmanGrahamRanking::dfs(node v)
{
	visited->push(v);
	mark[v] |= 1;

	node w;
	adjEntry adj;
	forall_adj(adj,v) {
		if ((adj->theEdge()->source()) == v) {
			w = adj->twinNode();
			if (mark[w] & 2) {
				mark[w] |= 4;
			}

			if ((mark[w] & 1) == 0) {
				dfs(w);
			}
		}
	}
}

void CoffmanGrahamRanking::removeTransitiveEdges (Graph& G)
{
	node v, w;
	List<edge> vout;

	mark.init(G,0);
	visited = new StackPure<node>();

	forall_nodes(v,G) {
		G.outEdges<List<edge> >(v, vout);
		/* alternative: iterate over all adjELements (only out Edges)
		 *
		 * forall_adj(adj,v) {
		 * if ((adj->theEdge()->source()) == v) ...
		 *
		 * In this solution a List is generated, because we iterate three times
		 * over this subset of adjElements
		 */
		for (ListIterator<edge> it = vout.begin(); it.valid() ; ++it){
			w = (*it)-> target();
			mark[w] = 2;
		}

		// forall out edges
		for (ListIterator<edge> it = vout.begin(); it.valid() ; ++it){
			w = (*it)-> target();

			// if (w != 1)
			if ((mark[w] & 1) == 0) {
				dfs(w);
			}
		}

		// forall out edges
		for (ListIterator<edge> it = vout.begin(); it.valid() ; ++it){
			node u = (*it)->target();
			if (mark[u] & 4) {
				G.delEdge(*it);
			}
		}
		while (!visited->empty())
			mark[visited->pop()] = 0;
	}

	mark.init();
	delete visited;
}

} //namespace ogdf
