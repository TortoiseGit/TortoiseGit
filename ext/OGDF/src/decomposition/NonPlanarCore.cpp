/*
 * $Revision: 2599 $
 *
 * last checkin:
 *   $Author: chimani $
 *   $Date: 2012-07-15 22:39:24 +0200 (So, 15. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implements the class NonPlanarCore.
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


#include <ogdf/planarity/NonPlanarCore.h>
#include <ogdf/decomposition/StaticSPQRTree.h>
#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/basic/Queue.h>
#include <ogdf/basic/CombinatorialEmbedding.h>
#include <ogdf/basic/FaceArray.h>


namespace ogdf {


NonPlanarCore::NonPlanarCore(const Graph &G) : m_pOriginal(&G), m_orig(m_graph),
	m_real(m_graph,0), m_mincut(m_graph), m_cost(m_graph)
{
	if(G.numberOfNodes() <= 4)
		return; // nothing to do; planar graph => empty core

	// Build SPQR-tree of graph
	StaticSPQRTree T(G);

	// mark tree nodes in the core
	NodeArray<bool> mark;
	markCore(T,mark);

	NodeArray<node> map(G,0);
	NodeArray<node> mapAux(G,0);
	const Graph &tree = T.tree();

	node v;
	forall_nodes(v,tree) {
		if(mark[v] == false)
			continue;

		Skeleton &S = T.skeleton(v);
		edge e;
		forall_edges(e,S.getGraph()) {
			node src = S.original(e->source());
			node tgt = S.original(e->target());

			if(map[src] == 0) {
				m_orig[map[src] = m_graph.newNode()] = S.original(e->source());
			}
			if(map[tgt] == 0) {
				m_orig[map[tgt] = m_graph.newNode()] = S.original(e->target());
			}

			if(S.isVirtual(e)) {
				node w = S.twinTreeNode(e);
				if(mark[w] == false) {
					// new virtual edge in core graph
					edge eC = m_graph.newEdge(map[src],map[tgt]);
					traversingPath(S,e,m_mincut[eC],mapAux);
				}

			} else {
				// new real edge in core graph
				edge eC = m_graph.newEdge(map[src],map[tgt]);
				m_real[eC] = S.realEdge(e);
				m_mincut[eC].pushBack(S.realEdge(e));
			}
		}
	}

	edge e;
	forall_edges(e, m_graph) {
		m_cost[e] = m_mincut[e].size();
	}
}


// This function marks all nodes in the SPQR-tree which induce the
// non-planar core.
void NonPlanarCore::markCore(const SPQRTree &T, NodeArray<bool> &mark)
{
	const Graph &tree = T.tree();

	// We mark every tree node that belongs to the core
	mark.init(tree,true);  // start with all nodes and unmark planar leaves
	NodeArray<int> degree(tree);

	Queue<node> Q;

	node v;
	forall_nodes(v,tree) {
		degree[v] = v->degree();
		if(degree[v] <= 1) // also append deg-0 node (T has only one node)
			Q.append(v);
	}

	while(!Q.empty())
	{
		v = Q.pop();

		// if v has a planar skeleton
		if(T.typeOf(v) != SPQRTree::RNode ||
			isPlanar(T.skeleton(v).getGraph()) == true)
		{
			mark[v] = false; // unmark this leaf

			node w = 0;
			adjEntry adj;
			forall_adj(adj,v) {
				node x = adj->twinNode();
				if(mark[x] == true) {
					w = x; break;
				}
			}

			if(w != 0) {
				--degree[w];
				if(degree[w] == 1)
					Q.append(w);
			}
		}
	}
}

struct OGDF_EXPORT QueueEntry
{
	QueueEntry(node p, node v) : m_parent(p), m_current(v) { }

	node m_parent;
	node m_current;
};

void NonPlanarCore::traversingPath(Skeleton &Sv, edge eS, List<edge> &path, NodeArray<node> &mapV)
{
	const SPQRTree &T = Sv.owner();

	//-----------------------------------------------------
	// Build the graph representing the planar st-component
	Graph H;
	EdgeArray<edge> mapE(H,0);
	SListPure<node> nodes;

	Queue<QueueEntry> Q;
	Q.append(QueueEntry(Sv.treeNode(),Sv.twinTreeNode(eS)));

	while(!Q.empty())
	{
		QueueEntry x = Q.pop();
		node parent = x.m_parent;
		node current = x.m_current;

		const Skeleton &S = T.skeleton(current);

		edge e;
		forall_edges(e,S.getGraph()) {
			if(S.isVirtual(e) == true)
				continue;

			node src = S.original(e->source());
			node tgt = S.original(e->target());

			if(mapV[src] == 0) {
				nodes.pushBack(src);
				mapV[src] = H.newNode();
			}
			if(mapV[tgt] == 0) {
				nodes.pushBack(tgt);
				mapV[tgt] = H.newNode();
			}

			mapE[H.newEdge(mapV[src],mapV[tgt])] = S.realEdge(e);
		}

		adjEntry adj;
		forall_adj(adj,current) {
			node w = adj->twinNode();
			if(w != parent)
				Q.append(QueueEntry(current,w));
		}
	}

	// add st-edge
	edge e_st = H.newEdge(mapV[Sv.original(eS->source())],mapV[Sv.original(eS->target())]);

	// Compute planar embedding of H
#ifdef OGDF_DEBUG
	bool ok =
#endif
		planarEmbed(H);
	OGDF_ASSERT(ok)
	CombinatorialEmbedding E(H);

	//---------------------------------
	// Compute corresponding dual graph
	Graph dual;
	FaceArray<node> nodeOf(E);
	EdgeArray<adjEntry> primalAdj(dual);

	// insert a node in the dual graph for each face in E
	face f;
	forall_faces(f,E)
		nodeOf[f] = dual.newNode();


	node s = nodeOf[E.rightFace(e_st->adjSource())];
	node t = nodeOf[E.rightFace(e_st->adjTarget())];

	// Insert an edge into the dual graph for each adjacency entry in E.
	// The edges are directed from the left face to the right face.
	node v;
	forall_nodes(v,H)
	{
		adjEntry adj;
		forall_adj(adj,v)
		{
			// do not insert edges crossing e_st
			if(adj->theEdge() == e_st)
				continue;

			node vLeft  = nodeOf[E.leftFace (adj)];
			node vRight = nodeOf[E.rightFace(adj)];

			primalAdj[dual.newEdge(vLeft,vRight)] = adj;
		}
	}

	//---------------------------
	// Find shortest path in dual
	NodeArray<edge> spPred(dual,0);
	QueuePure<edge> queue;

	edge eDual;
	forall_adj_edges(eDual,s) {
		if(s == eDual->source())
			queue.append(eDual);
	}

	// actual search (using bfs on directed dual)
	for( ; ; )
	{
		// next candidate edge
		edge eCand = queue.pop();
		node v = eCand->target();

		// leads to an unvisited node?
		if (spPred[v] == 0)
		{
			// yes, then we set v's predecessor in search tree
			spPred[v] = eCand;

			// have we reached t ...
			if (v == t)
			{
				// ... then search is done.
				// constructed list of used edges (translated to crossed
				// edges entries in G) from t back to s (including first
				// and last!)

				do {
					edge eDual = spPred[v];
					edge eG = mapE[primalAdj[eDual]->theEdge()];
					OGDF_ASSERT(eG != 0)
					path.pushFront(eG);
					v = eDual->source();
				} while(v != s);

				break;
			}

			// append next candidate edges to queue
			// (all edges leaving v)
			edge e;
			forall_adj_edges(e,v) {
				if (v == e->source())
					queue.append(e);
			}
		}
	}


	//---------
	// Clean-up
	SListConstIterator<node> it;
	for(it = nodes.begin(); it.valid(); ++it)
		mapV[*it] = 0;
}


} // end namespace ogdf
