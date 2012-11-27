/*
 * $Revision: 2599 $
 *
 * last checkin:
 *   $Author: chimani $
 *   $Date: 2012-07-15 22:39:24 +0200 (So, 15. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief  Implementation of the Booth-Lueker planarity test.
 *
 * Implements planarity test and planar embedding algorithm.
 *
 * \author Sebastian Leipert
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


#include <ogdf/basic/basic.h>
#include <ogdf/basic/Array.h>
#include <ogdf/basic/NodeArray.h>
#include <ogdf/basic/SList.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/internal/planarity/PlanarPQTree.h>
#include <ogdf/internal/planarity/PlanarLeafKey.h>
#include <ogdf/planarity/BoothLueker.h>
#include <ogdf/internal/planarity/EmbedPQTree.h>


namespace ogdf{

bool BoothLueker::isPlanarDestructive(Graph &G)
{
	bool ret = preparation(G,false);
	m_parallelEdges.init();
	m_isParallel.init();

	return ret;
}

bool BoothLueker::isPlanar(const Graph &G)
{
	Graph Gp(G);
	bool ret = preparation(Gp,false);
	m_parallelEdges.init();
	m_isParallel.init();

	return ret;
}

// Prepares the planarity test and the planar embedding
// Parallel edges:  do not need to be ignored, they can be handled
// by the planarity test.
// Selfloops: need to be ignored.
bool BoothLueker::preparation(Graph &G, bool embed)
{
	if (G.numberOfEdges() < 9 && !embed)
		return true;
	else if (G.numberOfEdges() < 3 && embed)
		return true;

	node v;
	edge e;

	SListPure<node> selfLoops;
	makeLoopFree(G,selfLoops);

	prepareParallelEdges(G);

	int  isolated = 0;
	forall_nodes(v,G)
		if (v->degree() == 0)
			isolated++;

	if (((G.numberOfNodes()-isolated) > 2) &&
		((3*(G.numberOfNodes()-isolated) -6) < (G.numberOfEdges() - m_parallelCount)))
		return false;

	bool planar = true;

	NodeArray<node> tableNodes(G,0);
	EdgeArray<edge> tableEdges(G,0);
	NodeArray<bool> mark(G,0);

	EdgeArray<int> componentID(G);

	// Determine Biconnected Components
	int bcCount = biconnectedComponents(G,componentID);

	// Determine edges per biconnected component
	Array<SList<edge> > blockEdges(0,bcCount-1);
	forall_edges(e,G)
	{
		blockEdges[componentID[e]].pushFront(e);
	}

	// Determine nodes per biconnected component.
	Array<SList<node> > blockNodes(0,bcCount-1);
	int i;
	for (i = 0; i < bcCount; i++)
	{
		SListIterator<edge> it;
		for (it = blockEdges[i].begin(); it.valid(); ++it)
		{
			e = *it;
			if (!mark[e->source()])
			{
				blockNodes[i].pushBack(e->source());
				mark[e->source()] = true;
			}
			if (!mark[e->target()])
			{
				blockNodes[i].pushBack(e->target());
				mark[e->target()] = true;
			}
		}
		SListIterator<node> itn;
		for (itn = blockNodes[i].begin(); itn.valid(); ++itn)
			mark[*itn] = false;
	}

	// Perform Planarity Test for every biconnected component

	if (bcCount == 1)
	{
		if (G.numberOfEdges() >= 2)
		{
			// Compute st-numbering
			NodeArray<int> numbering(G,0);
#ifdef OGDF_DEBUG
			int n =
#endif
				stNumber(G,numbering);
			OGDF_ASSERT_IF(dlConsistencyChecks,testSTnumber(G,numbering,n))

			EdgeArray<edge> backTableEdges(G,0);
			forall_edges(e,G)
				backTableEdges[e] = e;

			if (embed)
				planar = doEmbed(G,numbering,backTableEdges,backTableEdges);
			else
				planar = doTest(G,numbering);
		}
	}
	else
	{
		NodeArray<SListPure<adjEntry> > entireEmbedding(G);
		for (i = 0; i < bcCount; i++)
		{
			Graph C;

			SListIterator<node> itn;
			for (itn = blockNodes[i].begin(); itn.valid(); ++ itn)
			{
				v = *itn;
				node w = C.newNode();
				tableNodes[v] = w;
			}

			NodeArray<node> backTableNodes(C,0);
			if (embed)
			{
				for (itn = blockNodes[i].begin(); itn.valid(); ++ itn)
					backTableNodes[tableNodes[*itn]] = *itn;
			}

			SListIterator<edge> it;
			for (it = blockEdges[i].begin(); it.valid(); ++it)
			{
				e = *it;
				edge f = C.newEdge(tableNodes[e->source()],tableNodes[e->target()]);
				tableEdges[e] = f;
			}

			EdgeArray<edge> backTableEdges(C,0);
			for (it = blockEdges[i].begin(); it.valid(); ++it)
				backTableEdges[tableEdges[*it]] = *it;

			if (C.numberOfEdges() >= 2)
			{
				// Compute st-numbering
				NodeArray<int> numbering(C,0);
#ifdef OGDF_DEBUG
				int n =
#endif
					stNumber(C,numbering);
				OGDF_ASSERT_IF(dlConsistencyChecks,testSTnumber(C,numbering,n))

				if (embed)
					planar = doEmbed(C,numbering,backTableEdges,tableEdges);
				else
					planar = doTest(C,numbering);

				if (!planar)
					break;
			}

			if (embed)
			{
				forall_nodes(v,C)
				{
					node w = backTableNodes[v];
					adjEntry a;
					forall_adj(a,v)
					{
						edge e = backTableEdges[a->theEdge()];
						adjEntry adj = (e->adjSource()->theNode() == w)?
										e->adjSource() : e->adjTarget();
						entireEmbedding[w].pushBack(adj);
					}
				}
			}
		}

		if (planar && embed)
		{
			forall_nodes(v,G)
				G.sort(v,entireEmbedding[v]);
		}

	}

	while (!selfLoops.empty())
	{
		v = selfLoops.popFrontRet();
		G.newEdge(v,v);
	}

	OGDF_ASSERT_IF(dlConsistencyChecks,
		planar == false || embed == false || G.representsCombEmbedding())

	return planar;
}


// Performs a planarity test on a biconnected component
// of G. numbering contains an st-numbering of the component.
bool BoothLueker::doTest(Graph &G,NodeArray<int> &numbering)
{
	bool planar = true;

	NodeArray<SListPure<PlanarLeafKey<IndInfo*>* > > inLeaves(G);
	NodeArray<SListPure<PlanarLeafKey<IndInfo*>* > > outLeaves(G);
	Array<node> table(G.numberOfNodes()+1);

	node v;
	forall_nodes(v,G)
	{
		edge e;
		forall_adj_edges(e,v)
		{
			if (numbering[e->opposite(v)] > numbering[v])
				//sideeffect: loops are ignored
			{
				PlanarLeafKey<IndInfo*>* L = OGDF_NEW PlanarLeafKey<IndInfo*>(e);
				inLeaves[v].pushFront(L);
			}
		}
		table[numbering[v]] = v;
	}

	forall_nodes(v,G)
	{
		SListIterator<PlanarLeafKey<IndInfo*>* > it;
		for (it = inLeaves[v].begin(); it.valid(); ++it)
		{
			PlanarLeafKey<IndInfo*>* L = *it;
			outLeaves[L->userStructKey()->opposite(v)].pushFront(L);
		}
	}

	PlanarPQTree T;

	T.Initialize(inLeaves[table[1]]);
	for (int i = 2; i < G.numberOfNodes(); i++)
	{
		if (T.Reduction(outLeaves[table[i]]))
		{
			T.ReplaceRoot(inLeaves[table[i]]);
			T.emptyAllPertinentNodes();

		}
		else
		{
			planar = false;
			break;
		}
	}
	if (planar)
		T.emptyAllPertinentNodes();


	// Cleanup
	forall_nodes(v,G)
	{
		while (!inLeaves[v].empty())
		{
			PlanarLeafKey<IndInfo*>* L = inLeaves[v].popFrontRet();
			delete L;
		}
	}

	return planar;
}


// Performs a planarity test on a biconnected component
// of G and embedds it planar.
// numbering contains an st-numbering of the component.
bool BoothLueker::doEmbed(
	Graph &G,
	NodeArray<int>  &numbering,
	EdgeArray<edge> &backTableEdges,
	EdgeArray<edge> &forwardTableEdges)
{

	NodeArray<SListPure<PlanarLeafKey<IndInfo*>* > > inLeaves(G);
	NodeArray<SListPure<PlanarLeafKey<IndInfo*>* > > outLeaves(G);
	NodeArray<SListPure<edge> > frontier(G);
	NodeArray<SListPure<node> > opposed(G);
	NodeArray<SListPure<node> > nonOpposed(G);
	Array<node> table(G.numberOfNodes()+1);
	Array<bool> toReverse(1,G.numberOfNodes()+1,false);

	node v;
	forall_nodes(v,G)
	{
		edge e;

		forall_adj_edges(e,v)
		{
			if (numbering[e->opposite(v)] > numbering[v])
			{
				PlanarLeafKey<IndInfo*>* L = OGDF_NEW PlanarLeafKey<IndInfo*>(e);
				inLeaves[v].pushFront(L);
			}
		}
		table[numbering[v]] = v;
	}

	forall_nodes(v,G)
	{
		SListIterator<PlanarLeafKey<IndInfo*>* > it;
		for (it = inLeaves[v].begin(); it.valid(); ++it)
		{
			PlanarLeafKey<IndInfo*>* L = *it;
			outLeaves[L->userStructKey()->opposite(v)].pushFront(L);
		}
	}

	EmbedPQTree T;

	T.Initialize(inLeaves[table[1]]);
	int i;
	for (i = 2; i <= G.numberOfNodes(); i++)
	{
		if (T.Reduction(outLeaves[table[i]]))
		{
			T.ReplaceRoot(inLeaves[table[i]], frontier[table[i]], opposed[table[i]], nonOpposed[table[i]], table[i]);
			T.emptyAllPertinentNodes();
		}
		else
		{
			// Cleanup
			forall_nodes(v,G)
			{
				while (!inLeaves[v].empty())
				{
					PlanarLeafKey<IndInfo*>* L = inLeaves[v].popFrontRet();
					delete L;
				}
			}
			return false;
		}
	}

	// Reverse adjacency lists if necessary
	// This gives an upward embedding
	for (i = G.numberOfNodes(); i >= 2; i--)
	{
		if (toReverse[i])
		{
			while (!nonOpposed[table[i]].empty())
			{
				v = nonOpposed[table[i]].popFrontRet();
				toReverse[numbering[v]] =  true;
			}
			frontier[table[i]].reverse();
		}
		else
		{
			while (!opposed[table[i]].empty())
			{
				v = opposed[table[i]].popFrontRet();
				toReverse[numbering[v]] =  true;
			}
		}
		nonOpposed[table[i]].clear();
		opposed[table[i]].clear();
	}

	// Compute the entire embedding
	NodeArray<SListPure<adjEntry> > entireEmbedding(G);
	forall_nodes(v,G)
	{
		while (!frontier[v].empty())
		{
			edge e = frontier[v].popFrontRet();
			entireEmbedding[v].pushBack(
				(e->adjSource()->theNode() == v)? e->adjSource() : e->adjTarget());
		}
	}

	NodeArray<bool> mark(G,false);
	NodeArray<SListIterator<adjEntry> > adjMarker(G,0);
	forall_nodes(v,G)
		adjMarker[v] = entireEmbedding[v].begin();
	v = table[G.numberOfNodes()];
	entireEmbed(G,entireEmbedding,adjMarker,mark,v);

	NodeArray<SListPure<adjEntry> > newEntireEmbedding(G);
	if (m_parallelCount > 0)
	{
		forall_nodes(v,G)
		{
			//adjEntry a;
			SListIterator<adjEntry> it;
			for(it=entireEmbedding[v].begin();it.valid();it++)
			{
				edge e = (*it)->theEdge(); // edge in biconnected component
				edge trans = backTableEdges[e]; // edge in original graph.
				if (!m_parallelEdges[trans].empty())
				{
					// This original edge is the reference edge
					// of a bundle of parallel edges

					ListIterator<edge> it;
					// If v is source of e, insert the parallel edges
					// in the order stored in the list.
					if (e->adjSource()->theNode() == v)
					{
						adjEntry adj = e->adjSource();
						newEntireEmbedding[v].pushBack(adj);
						for (it = m_parallelEdges[trans].begin(); it.valid(); it++)
						{
							edge parallel = forwardTableEdges[*it];
							adjEntry adj = parallel->adjSource()->theNode() == v ?
								parallel->adjSource() : parallel->adjTarget();
							newEntireEmbedding[v].pushBack(adj);
						}
					}
					else
					// v is target of e, insert the parallel edges
					// in the opposite order stored in the list.
					// This keeps the embedding.
					{
						for (it = m_parallelEdges[trans].rbegin(); it.valid(); it--)
						{
							edge parallel = forwardTableEdges[*it];
							adjEntry adj = parallel->adjSource()->theNode() == v ?
								parallel->adjSource() : parallel->adjTarget();
							newEntireEmbedding[v].pushBack(adj);
						}
						adjEntry adj = e->adjTarget();
						newEntireEmbedding[v].pushBack(adj);
					}
				}
				else if (!m_isParallel[trans])
					// normal non-multi-edge
				{
					adjEntry adj = e->adjSource()->theNode() == v?
									e->adjSource() : e->adjTarget();
					newEntireEmbedding[v].pushBack(adj);
				}
				// else e is a multi-edge but not the reference edge
			}
		}

		forall_nodes(v,G)
			G.sort(v,newEntireEmbedding[v]);
	}
	else
	{
		forall_nodes(v,G)
			G.sort(v,entireEmbedding[v]);
	}


	//cleanup
	forall_nodes(v,G)
	{
		while (!inLeaves[v].empty())
		{
			PlanarLeafKey<IndInfo*>* L = inLeaves[v].popFrontRet();
			delete L;
		}
	}

	return true;
}

// Used by doEmbed. Computes an entire embedding from an
// upward embedding.
void BoothLueker::entireEmbed(
	Graph &G,
	NodeArray<SListPure<adjEntry> > &entireEmbedding,
	NodeArray<SListIterator<adjEntry> > &adjMarker,
	NodeArray<bool> &mark,
	node v)
{
	mark[v] = true;
	SListIterator<adjEntry> it;
	for (it = adjMarker[v]; it.valid(); ++it)
	{
		adjEntry a = *it;
		edge e = a->theEdge();
		adjEntry adj = (e->adjSource()->theNode() == v)?
						e->adjTarget() : e->adjSource();
		node w = adj->theNode();
		entireEmbedding[w].pushFront(adj);
		if (!mark[w])
			entireEmbed(G,entireEmbedding,adjMarker,mark,w);
	}
}



void BoothLueker::prepareParallelEdges(Graph &G)
{
	edge e;

	// Stores for one reference edge all parallel edges.
	m_parallelEdges.init(G);
	// Is true for any multiedge, except for the reference edge.
	m_isParallel.init(G,false);
	getParallelFreeUndirected(G,m_parallelEdges);
	m_parallelCount = 0;
	forall_edges(e,G)
	{
		if (!m_parallelEdges[e].empty())
		{
			ListIterator<edge> it;
			for (it = m_parallelEdges[e].begin(); it.valid(); it++)
			{
				m_isParallel[*it] = true;
				m_parallelCount++;
			}
		}
	}
}


}
