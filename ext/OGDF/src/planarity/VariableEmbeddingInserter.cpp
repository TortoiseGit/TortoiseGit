/*
 * $Revision: 2599 $
 *
 * last checkin:
 *   $Author: chimani $
 *   $Date: 2012-07-15 22:39:24 +0200 (So, 15. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief implements class VariableEmbeddingInserter
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


#include <ogdf/planarity/VariableEmbeddingInserter.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/decomposition/StaticPlanarSPQRTree.h>
#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/basic/CombinatorialEmbedding.h>


namespace ogdf {

int VariableEmbeddingInserter::m_bigM = 10000;

//---------------------------------------------------------
// constructor
// sets default values for options
//
VariableEmbeddingInserter::VariableEmbeddingInserter()
{
	m_rrOption = rrNone;
	m_percentMostCrossed = 25;
}


//---------------------------------------------------------
// VEICrossingsBucket
// bucket function for sorting edges by decreasing number
// of crossings
class VEICrossingsBucket : public BucketFunc<edge>
{
	const PlanRep *m_pPG;

public:
	VEICrossingsBucket(const PlanRep *pPG) :
		m_pPG(pPG) { }

	int getBucket(const edge &e) {
		return -m_pPG->chain(e).size();
	}
};



//---------------------------------------------------------
// actual call (called by all variations of call)
//   crossing of generalizations is forbidden if forbidCrossingGens = true
//   edge costs are obeyed if costOrig != 0
//
Module::ReturnType VariableEmbeddingInserter::doCall(
	PlanRep &PG,
	const List<edge> &origEdges,
	bool forbidCrossingGens,
	const EdgeArray<int> *costOrig,
	const EdgeArray<bool> *forbiddenEdgeOrig,
	const EdgeArray<unsigned int> *edgeSubgraph)
{
	double T;
	usedTime(T);

	ReturnType retValue = retFeasible;
	m_runsPostprocessing = 0;

	if (origEdges.size() == 0)
		return retOptimal;  // nothing to do

	OGDF_ASSERT(forbidCrossingGens == false || forbiddenEdgeOrig == 0);

	m_pPG                = &PG;
	m_forbidCrossingGens = forbidCrossingGens;
	m_costOrig           = costOrig;
	m_forbiddenEdgeOrig  = forbiddenEdgeOrig;
	m_edgeSubgraph       = edgeSubgraph;

	SListPure<edge> currentOrigEdges;
	if(removeReinsert() == rrIncremental) {
		edge e;
		forall_edges(e,PG)
			currentOrigEdges.pushBack(PG.original(e));
	}

	// insertion of edges
	ListConstIterator<edge> it;
	for(it = origEdges.begin(); it.valid(); ++it)
	{
		edge eOrig = *it;
		m_typeOfCurrentEdge = m_forbidCrossingGens ? ((PlanRepUML&)PG).typeOrig(eOrig) : Graph::association;

		SList<adjEntry> eip;
		m_st = eOrig; //save original edge for simdraw cost calculation in dfsvertex
		insert(PG.copy(eOrig->source()),PG.copy(eOrig->target()),eip);

		PG.insertEdgePath(eOrig,eip);

		if(removeReinsert() == rrIncremental || removeReinsert() == rrIncInserted) {
			currentOrigEdges.pushBack(eOrig);

			bool improved;
			do {
				++m_runsPostprocessing;
				improved = false;

				SListConstIterator<edge> itRR;
				for(itRR = currentOrigEdges.begin(); itRR.valid(); ++itRR)
				{
					edge eOrigRR = *itRR;

					int pathLength;
					if(costOrig != 0)
						pathLength = costCrossed(eOrigRR);
					else
						pathLength = PG.chain(eOrigRR).size() - 1;
					if (pathLength == 0) continue; // cannot improve

					PG.removeEdgePath(eOrigRR);

					m_typeOfCurrentEdge = m_forbidCrossingGens ? ((PlanRepUML&)PG).typeOrig(eOrigRR) : Graph::association;

					SList<adjEntry> eip;
					m_st = eOrigRR;
					insert(PG.copy(eOrigRR->source()),PG.copy(eOrigRR->target()),eip);
					PG.insertEdgePath(eOrigRR,eip);

					int newPathLength = (costOrig != 0) ? costCrossed(eOrigRR) : (PG.chain(eOrigRR).size() - 1);
					OGDF_ASSERT(newPathLength <= pathLength);

					if(newPathLength < pathLength)
						improved = true;
				}
			} while (improved);
		}
	}


	if(removeReinsert() != rrIncremental && removeReinsert() != rrIncInserted) {
		// postprocessing (remove-reinsert heuristc)
		const Graph &G = PG.original();
		SListPure<edge> rrEdges;

		switch(removeReinsert())
		{
		case rrAll:
		case rrMostCrossed: {
				const List<node> &origInCC = PG.nodesInCC();
				ListConstIterator<node> itV;

				for(itV = origInCC.begin(); itV.valid(); ++itV) {
					node vG = *itV;
					adjEntry adj;
					forall_adj(adj,vG) {
						if ((adj->index() & 1) == 0) continue;
						edge eG = adj->theEdge();
						rrEdges.pushBack(eG);
					}
				}
			}
			break;

		case rrInserted:
			for(ListConstIterator<edge> it = origEdges.begin(); it.valid(); ++it)
				rrEdges.pushBack(*it);
			break;

		case rrNone:
		case rrIncremental:
		case rrIncInserted:
			break;
		}



		// marks the end of the interval of rrEdges over which we iterate
		// initially set to invalid iterator which means all edges
		SListConstIterator<edge> itStop;

		bool improved;
		do {
			// abort postprocessing if time limit reached
			if (m_timeLimit >= 0 && m_timeLimit <= usedTime(T)) {
				retValue = retTimeoutFeasible;
				break;
			}

			++m_runsPostprocessing;
			improved = false;

			if(removeReinsert() == rrMostCrossed)
			{
				VEICrossingsBucket bucket(&PG);
				rrEdges.bucketSort(bucket);

				const int num = int(0.01 * percentMostCrossed() * G.numberOfEdges());
				itStop = rrEdges.get(num);
			}

			SListConstIterator<edge> it;
			for(it = rrEdges.begin(); it != itStop; ++it)
			{
				edge eOrig = *it;

				int pathLength;
				if(costOrig != 0)
					pathLength = costCrossed(eOrig);
				else
					pathLength = PG.chain(eOrig).size() - 1;
				if (pathLength == 0) continue; // cannot improve

				PG.removeEdgePath(eOrig);

				m_typeOfCurrentEdge = m_forbidCrossingGens ? ((PlanRepUML&)PG).typeOrig(eOrig) : Graph::association;

				SList<adjEntry> eip;
				m_st = eOrig;
				insert(PG.copy(eOrig->source()),PG.copy(eOrig->target()),eip);
				PG.insertEdgePath(eOrig,eip);

				// we cannot find a shortest path that is longer than before!
				int newPathLength = (costOrig != 0) ? costCrossed(eOrig) : (PG.chain(eOrig).size() - 1);
				OGDF_ASSERT(newPathLength <= pathLength);

				if(newPathLength < pathLength)
					improved = true;
			}
		} while (improved);
	}

#ifdef OGDF_DEBUG
	bool isPlanar =
#endif
		planarEmbed(PG);

	OGDF_ASSERT(isPlanar);

	PG.removePseudoCrossings();
	OGDF_ASSERT(PG.representsCombEmbedding());
	OGDF_ASSERT(forbidCrossingGens == false || checkCrossingGens((const PlanRepUML&)PG) == true);

	return retValue;
}


Module::ReturnType VariableEmbeddingInserter::doCallPostprocessing(
	PlanRep &PG,
	const List<edge> &origEdges,
	bool forbidCrossingGens,
	const EdgeArray<int> *costOrig,
	const EdgeArray<bool> *forbiddenEdgeOrig,
	const EdgeArray<unsigned int> *edgeSubgraph)
{
	double T;
	usedTime(T);
	ReturnType retValue = retFeasible;
	m_runsPostprocessing = 0;

	if (origEdges.size() == 0)
		return retOptimal;  // nothing to do

	if(removeReinsert() == rrIncremental || removeReinsert() == rrIncInserted)
		return retFeasible;

	OGDF_ASSERT(forbidCrossingGens == false || forbiddenEdgeOrig == 0);

	m_pPG                = &PG;
	m_forbidCrossingGens = forbidCrossingGens;
	m_costOrig           = costOrig;
	m_forbiddenEdgeOrig  = forbiddenEdgeOrig;
	m_edgeSubgraph       = edgeSubgraph;

	SListPure<edge> currentOrigEdges;

	// postprocessing (remove-reinsert heuristc)
	const Graph &G = PG.original();
	SListPure<edge> rrEdges;

	switch(removeReinsert())
	{
	case rrAll:
	case rrMostCrossed: {
			const List<node> &origInCC = PG.nodesInCC();
			ListConstIterator<node> itV;

			for(itV = origInCC.begin(); itV.valid(); ++itV) {
				node vG = *itV;
				adjEntry adj;
				forall_adj(adj,vG) {
					if ((adj->index() & 1) == 0) continue;
					edge eG = adj->theEdge();
					rrEdges.pushBack(eG);
				}
			}
		}
		break;

	case rrInserted:
		for(ListConstIterator<edge> it = origEdges.begin(); it.valid(); ++it)
			rrEdges.pushBack(*it);
		break;

	case rrNone:
	case rrIncremental:
	case rrIncInserted:
		break;
	}



	// marks the end of the interval of rrEdges over which we iterate
	// initially set to invalid iterator which means all edges
	SListConstIterator<edge> itStop;

	bool improved;
	do {
		// abort postprocessing if time limit reached
		if (m_timeLimit >= 0 && m_timeLimit <= usedTime(T)) {
			retValue = retTimeoutFeasible;
			break;
		}

		++m_runsPostprocessing;
		improved = false;

		if(removeReinsert() == rrMostCrossed)
		{
			VEICrossingsBucket bucket(&PG);
			rrEdges.bucketSort(bucket);

			const int num = int(0.01 * percentMostCrossed() * G.numberOfEdges());
			itStop = rrEdges.get(num);
		}

		SListConstIterator<edge> it;
		for(it = rrEdges.begin(); it != itStop; ++it)
		{
			edge eOrig = *it;

			int pathLength;
			if(costOrig != 0)
				pathLength = costCrossed(eOrig);
			else
				pathLength = PG.chain(eOrig).size() - 1;
			if (pathLength == 0) continue; // cannot improve

			PG.removeEdgePath(eOrig);

			m_typeOfCurrentEdge = m_forbidCrossingGens ? ((PlanRepUML&)PG).typeOrig(eOrig) : Graph::association;

			SList<adjEntry> eip;
			m_st = eOrig;
			insert(PG.copy(eOrig->source()),PG.copy(eOrig->target()),eip);
			PG.insertEdgePath(eOrig,eip);

			// we cannot find a shortest path that is longer than before!
			int newPathLength = (costOrig != 0) ? costCrossed(eOrig) : (PG.chain(eOrig).size() - 1);
			OGDF_ASSERT(newPathLength <= pathLength);

			if(newPathLength < pathLength)
				improved = true;
		}
	} while (improved);

#ifdef OGDF_DEBUG
	bool isPlanar =
#endif
		planarEmbed(PG);

	OGDF_ASSERT(isPlanar);

	PG.removePseudoCrossings();
	OGDF_ASSERT(PG.representsCombEmbedding());
	OGDF_ASSERT(forbidCrossingGens == false || checkCrossingGens((const PlanRepUML&)PG) == true);

	return retValue;
}


edge VariableEmbeddingInserter::crossedEdge(adjEntry adj) const
{
	edge e = adj->theEdge();

	adj = adj->cyclicSucc();
	while(adj->theEdge() == e)
		adj = adj->cyclicSucc();

	return adj->theEdge();
}


int VariableEmbeddingInserter::costCrossed(edge eOrig) const
{
	int c = 0;

	const List<edge> &L = m_pPG->chain(eOrig);

	ListConstIterator<edge> it = L.begin();
	if(m_edgeSubgraph != 0) {
		for(++it; it.valid(); ++it) {
			int counter = 0;
			edge e = m_pPG->original(crossedEdge((*it)->adjSource()));
			for(int i=0; i<32; i++)
				if((*m_edgeSubgraph)[eOrig] & (*m_edgeSubgraph)[e] & (1<<i))
					counter++;
			c += counter*(*m_costOrig)[e];
		}
		c *= m_bigM;
		if(c == 0)
			c = 1;
	} else {
		for(++it; it.valid(); ++it) {
			c += (*m_costOrig)[m_pPG->original(crossedEdge((*it)->adjSource()))];
		}
	}

	return c;
}


//-------------------------------------------------------------------
// find optimal edge insertion path from s to t in connected
// graph G
//-------------------------------------------------------------------

void VariableEmbeddingInserter::insert(node s,
	node t,
	SList<adjEntry> &eip)
{
	PlanRep &PG = *m_pPG;
	eip.clear();

	m_s = s; m_t = t;
	m_pEip = &eip;

	// compute biconnected components of PG
	EdgeArray<int> compnum(PG);
	int c = biconnectedComponents(PG,compnum);

	m_compV.init(PG);
	m_nodeB.init(c);

	// edgeB[i] = list of edges in component i
	m_edgeB.init(c);
	edge e;
	forall_edges(e,PG)
		m_edgeB[compnum[e]].pushBack(e);

	// construct arrays compV and nodeB such that
	// m_compV[v] = list of components containing v
	// m_nodeB[i] = list of vertices in component i
	NodeArray<bool> mark(PG,false);

	int i;
	for(i = 0; i < c; ++i) {
		SListConstIterator<edge> itEdge;
		for(itEdge = m_edgeB[i].begin(); itEdge.valid(); ++itEdge)
		{
			edge e = *itEdge;

			if (!mark[e->source()]) {
				mark[e->source()] = true;
				m_nodeB[i].pushBack(e->source());
			}
			if (!mark[e->target()]) {
				mark[e->target()] = true;
				m_nodeB[i].pushBack(e->target());
			}
		}

		SListConstIterator<node> itNode;
		for(itNode = m_nodeB[i].begin(); itNode.valid(); ++itNode)
		{
			node v = *itNode;
			m_compV[v].pushBack(i);
			mark[v] = false;
		}
	}
	mark.init();

	// find path from s to t in BC-tree
	// call of blockInsert() is done in dfs_vertex() when we have found the
	// path and we return from the recursion.
	// if no path is found, s and t are in different connected components
	// and thus an empty edge insertion path is correct!
	m_GtoBC.init(PG,0);
	dfsVertex(s,-1);

	// deallocate resources used by insert()
	m_GtoBC.init();
	m_edgeB.init();
	m_nodeB.init();
	m_compV.init();
}


class BiconnectedComponent : public Graph
{
public:
	// constructor
	BiconnectedComponent()
	{
		m_BCtoG .init(*this);
		m_cost  .init(*this,1);
		m_typeOf.init(*this, Graph::association);
	}

	void cost(edge e, int c) {
		m_cost[e] = c;
	}

	int cost(edge e) const {
		return m_cost[e];
	}

	void typeOf(edge e, EdgeType et) {
		m_typeOf[e] = et;
	}

	EdgeType typeOf(edge e) const {
		return m_typeOf[e];
	}

	AdjEntryArray<adjEntry> m_BCtoG;

private:
	EdgeArray<int>      m_cost;
	EdgeArray<EdgeType> m_typeOf;
};



//-------------------------------------------------------------------
// recursive path search from s to t in BC-tree (vertex case)
//-------------------------------------------------------------------

bool VariableEmbeddingInserter::dfsVertex(node v, int parent)
{
	// forall biconnected components containing v (except predecessor parent)
	SListConstIterator<int> itI;
	for(itI = m_compV[v].begin(); itI.valid(); ++itI)
	{
		int i = *itI;

		if (i == parent) continue;

		node repT; // representative of t in B(i)
		if (dfsComp(i,v,repT) == true) { // path found?
			// build graph BC of biconnected component B(i)
			SList<node> nodesG;
			BiconnectedComponent BC;

			SListConstIterator<edge> itE;
			for(itE = m_edgeB[i].begin(); itE.valid(); ++itE)
			{
				edge e = *itE;

				if (m_GtoBC[e->source()] == 0) {
					m_GtoBC[e->source()] = BC.newNode();
					nodesG.pushBack(e->source());
				}
				if (m_GtoBC[e->target()] == 0) {
					m_GtoBC[e->target()] = BC.newNode();
					nodesG.pushBack(e->target());
				}

				edge eBC = BC.newEdge(m_GtoBC[e->source()],m_GtoBC[e->target()]);
				BC.m_BCtoG[eBC->adjSource()] = e->adjSource();
				BC.m_BCtoG[eBC->adjTarget()] = e->adjTarget();

				BC.typeOf(eBC, m_forbidCrossingGens ? ((PlanRepUML*)m_pPG)->typeOf(e) : Graph::association);
				edge eOrig = m_pPG->original(e);
				if(m_costOrig != 0) {
					if(m_edgeSubgraph != 0) {
						int counter = 0;
						for(int i = 0; i<32; i++)
							if((*m_edgeSubgraph)[m_st] & (*m_edgeSubgraph)[eOrig] & (1<<i))
								counter++;
						counter *= m_bigM;
						int cost = counter * (*m_costOrig)[eOrig];
						if(cost == 0)
							cost = 1;
						BC.cost(eBC, cost);
					} else
						BC.cost(eBC, (eOrig == 0) ? 0 : (*m_costOrig)[eOrig]);
				}
			}

			// less than 3 nodes requires no crossings (cannot build SPQR-tree
			// for a graph with less than 3 nodes!)
			if (nodesG.size() >= 3) {
				List<adjEntry> L;
				blockInsert(BC,m_GtoBC[v],m_GtoBC[repT],L); // call biconnected case

				// transform crossed edges to edges in G
				ListConstIterator<adjEntry> it;
				for(it = L.rbegin(); it.valid(); --it) {
					m_pEip->pushFront(BC.m_BCtoG[*it]);
				}
			}

			// set entries of GtoBC back to nil (GtoBC allocated only once
			// in insert()!)
			SListConstIterator<node> itV;
			for(itV = nodesG.begin(); itV.valid(); ++itV)
				m_GtoBC[*itV] = 0;

			return true; // path found
		}
	}

	return false; // path not found
}



//-------------------------------------------------------------------
// recursive path search from s to t in BC-tree (component case)
//-------------------------------------------------------------------

bool VariableEmbeddingInserter::dfsComp(int i, node parent, node &repT)
{
	// forall nodes in biconected component B(i) (except predecessor parent)
	SListConstIterator<node> it;
	for(it = m_nodeB[i].begin(); it.valid(); ++it)
	{
		repT = *it;

		if (repT == parent) continue;
		if (repT == m_t) { // t found?
			return true;
		}
		if (dfsVertex(repT,i) == true) {
			return true; // path found
		}
	}

	return false; // path not found
}



//-------------------------------------------------------------------
// ExpandedGraph represents the (partially) expanded graph with
// its augmented dual
//-------------------------------------------------------------------

class ExpandedGraph
{
	const StaticSPQRTree       &m_T;
	const BiconnectedComponent &m_BC;

	NodeArray<node> m_GtoExp;
	List<node>      m_nodesG;
	Graph           m_exp;   // expanded graph
	ConstCombinatorialEmbedding m_E;
	AdjEntryArray<adjEntry> m_expToG;
	edge            m_eS, m_eT; // (virtual) edges in exp representing s and t (if any)

	Graph           m_dual;  // augmented dual graph of exp
	EdgeArray<adjEntry> m_primalEdge;
	EdgeArray<bool>     m_primalIsGen; // true iff corresponding primal edge is a generalization

	node m_vS, m_vT; // augmented nodes in dual representing s and t

public:
	ExpandedGraph(const BiconnectedComponent &BC, const StaticSPQRTree &T);

	void expand(node v, edge eIn, edge eOut);

	void constructDual(node s, node t, GraphCopy &GC, const EdgeArray<bool> *forbiddenEdgeOrig);
	void constructDualForbidCrossingGens(node s, node t);

	void findShortestPath(Graph::EdgeType eType, List<adjEntry> &L);
	void findWeightedShortestPath(Graph::EdgeType eType,
		List<adjEntry> &L);

	int costDual(edge eDual) const {
		adjEntry adjExp = m_primalEdge[eDual];
		return (adjExp == 0) ? 0 : m_BC.cost(m_expToG[adjExp]->theEdge());
	}

	// avoid automatic creation of assignment operator
	ExpandedGraph &operator=(const ExpandedGraph &);

private:
	edge insertEdge(node vG, node wG, edge eG);
	void expandSkeleton(node v, edge e1, edge e2);
};


ExpandedGraph::ExpandedGraph(const BiconnectedComponent &BC,const StaticSPQRTree &T)
	: m_T(T), m_BC(BC),
	m_GtoExp(T.originalGraph(),0), m_expToG(m_exp,0),
	m_primalEdge(m_dual,0), m_primalIsGen(m_dual,false)
{
}


//-------------------------------------------------------------------
// build expanded graph (by expanding skeleton(v), edges eIn and eOut
// are the adjacent tree edges on the path from v1 to v2
//-------------------------------------------------------------------

void ExpandedGraph::expand(node v, edge eIn, edge eOut)
{
	m_exp.clear();
	while (!m_nodesG.empty())
		m_GtoExp[m_nodesG.popBackRet()] = 0;

	const Skeleton &S = m_T.skeleton(v);

	if (eIn != 0) {
		edge eInS = (v != eIn->source()) ? m_T.skeletonEdgeTgt(eIn) :
			m_T.skeletonEdgeSrc(eIn);
		node x = S.original(eInS->source()), y = S.original(eInS->target());
		m_eS = insertEdge(x,y,0);
	}
	if (eOut != 0) {
		edge eOutS = (v != eOut->source()) ? m_T.skeletonEdgeTgt(eOut) :
			m_T.skeletonEdgeSrc(eOut);
		node x = S.original(eOutS->source()), y = S.original(eOutS->target());
		m_eT = insertEdge(x,y,0);
	}

	expandSkeleton(v, eIn, eOut);

	planarEmbed(m_exp);
	m_E.init(m_exp);
}


//-------------------------------------------------------------------
// expand one skeleton (recursive construction)
//-------------------------------------------------------------------

void ExpandedGraph::expandSkeleton(node v, edge e1, edge e2)
{
	const StaticSkeleton &S = *dynamic_cast<StaticSkeleton*>(&m_T.skeleton(v));
	const Graph          &M = S.getGraph();

	edge e;
	forall_edges(e,M)
	{
		edge eG = S.realEdge(e);
		if (eG != 0) {
			insertEdge(eG->source(),eG->target(),eG);

		} else {
			edge eT = S.treeEdge(e);

			// do not expand virtual edges corresponding to tree edges e1 or e2
			if (eT != e1 && eT != e2) {
				expandSkeleton((v == eT->source()) ? eT->target() : eT->source(),
					eT,0);
			}
		}
	}
}


//-------------------------------------------------------------------
// insert edge in exp (from a node corresponding to vG in G to a node
// corresponding to wG)
//-------------------------------------------------------------------

edge ExpandedGraph::insertEdge(node vG, node wG, edge eG)
{
	node &rVG = m_GtoExp[vG];
	node &rWG = m_GtoExp[wG];

	if (rVG == 0) {
		rVG = m_exp.newNode();
		m_nodesG.pushBack(vG);
	}
	if (rWG == 0) {
		rWG = m_exp.newNode();
		m_nodesG.pushBack(wG);
	}

	edge e1 = m_exp.newEdge(rVG,rWG);

	if(eG != 0) {
		m_expToG[e1->adjSource()] = eG->adjSource();
		m_expToG[e1->adjTarget()] = eG->adjTarget();
	} else {
		m_expToG[e1->adjSource()] = 0;
		m_expToG[e1->adjTarget()] = 0;
	}

	return e1;
}


//-------------------------------------------------------------------
// construct augmented dual of exp
//-------------------------------------------------------------------

void ExpandedGraph::constructDual(node s, node t,
	GraphCopy &GC, const EdgeArray<bool> *forbiddenEdgeOrig)
{
	m_dual.clear();

	FaceArray<node> faceNode(m_E);

	// constructs nodes (for faces in exp)
	face f;
	forall_faces(f,m_E) {
		faceNode[f] = m_dual.newNode();
	}

	// construct dual edges (for primal edges in exp)
	node v;
	forall_nodes(v,m_exp)
	{
		adjEntry adj;
		forall_adj(adj,v)
		{
			// cannot cross edges that does not correspond to real edges
			adjEntry adjG = m_expToG[adj];
			if(adjG == 0)
				continue;

			// Do not insert edges into dual if crossing the original edge
			// is forbidden
			if(forbiddenEdgeOrig &&
				(*forbiddenEdgeOrig)[GC.original(m_BC.m_BCtoG[m_expToG[adj]]->theEdge())] == true)
				continue;

			node vLeft  = faceNode[m_E.leftFace (adj)];
			node vRight = faceNode[m_E.rightFace(adj)];

			m_primalEdge[m_dual.newEdge(vLeft,vRight)] = adj;
		}
	}

	// augment dual by m_vS and m_vT
	m_vS = m_dual.newNode();
	if (m_GtoExp[s] != 0)
	{
		adjEntry adj;
		forall_adj(adj,m_GtoExp[s])
			m_dual.newEdge(m_vS,faceNode[m_E.rightFace(adj)]);
	}
	else
	{
		m_dual.newEdge(m_vS,faceNode[m_E.rightFace(m_eS->adjSource())]);
		m_dual.newEdge(m_vS,faceNode[m_E.rightFace(m_eS->adjTarget())]);
	}

	m_vT = m_dual.newNode();
	if (m_GtoExp[t] != 0)
	{
		adjEntry adj;
		forall_adj(adj,m_GtoExp[t])
			m_dual.newEdge(faceNode[m_E.rightFace(adj)], m_vT);
	}
	else
	{
		m_dual.newEdge(faceNode[m_E.rightFace(m_eT->adjSource())], m_vT);
		m_dual.newEdge(faceNode[m_E.rightFace(m_eT->adjTarget())], m_vT);
	}
}


void ExpandedGraph::constructDualForbidCrossingGens(node s, node t)
{
	m_dual.clear();

	FaceArray<node> faceNode(m_E);

	// constructs nodes (for faces in exp)
	face f;
	forall_faces(f,m_E) {
		faceNode[f] = m_dual.newNode();
	}

	edge eDual;
	// construct dual edges (for primal edges in exp)
	node v;
	forall_nodes(v,m_exp)
	{
		adjEntry adj;
		forall_adj(adj,v)
		{
			// cannot cross edges that does not correspond to real edges
			adjEntry adjG = m_expToG[adj];
			if(adjG == 0)
				continue;

			node vLeft  = faceNode[m_E.leftFace (adj)];
			node vRight = faceNode[m_E.rightFace(adj)];

			edge e = m_dual.newEdge(vLeft,vRight);
			m_primalEdge[e] = adj;

			// mark dual edges corresponding to generalizations
			if (adjG && m_BC.typeOf(adjG->theEdge()) == Graph::generalization)
				m_primalIsGen[e] = true;

			OGDF_ASSERT(m_primalEdge[e] == 0 || m_expToG[m_primalEdge[e]] != 0);
		}
	}

	// augment dual by m_vS and m_vT
	m_vS = m_dual.newNode();
	if (m_GtoExp[s] != 0)
	{
		adjEntry adj;
		forall_adj(adj,m_GtoExp[s]) {
			eDual = m_dual.newEdge(m_vS,faceNode[m_E.rightFace(adj)]);
			OGDF_ASSERT(m_primalEdge[eDual] == 0 || m_expToG[m_primalEdge[eDual]] != 0);
		}
	}
	else
	{
		eDual = m_dual.newEdge(m_vS,faceNode[m_E.rightFace(m_eS->adjSource())]);
		OGDF_ASSERT(m_primalEdge[eDual] == 0 || m_expToG[m_primalEdge[eDual]] != 0);

		eDual = m_dual.newEdge(m_vS,faceNode[m_E.rightFace(m_eS->adjTarget())]);
		OGDF_ASSERT(m_primalEdge[eDual] == 0 || m_expToG[m_primalEdge[eDual]] != 0);
	}

	m_vT = m_dual.newNode();
	if (m_GtoExp[t] != 0)
	{
		adjEntry adj;
		forall_adj(adj,m_GtoExp[t]) {
			eDual = m_dual.newEdge(faceNode[m_E.rightFace(adj)], m_vT);
			OGDF_ASSERT(m_primalEdge[eDual] == 0 || m_expToG[m_primalEdge[eDual]] != 0);
		}
	}
	else
	{
		eDual = m_dual.newEdge(faceNode[m_E.rightFace(m_eT->adjSource())], m_vT);
		OGDF_ASSERT(m_primalEdge[eDual] == 0 || m_expToG[m_primalEdge[eDual]] != 0);

		eDual = m_dual.newEdge(faceNode[m_E.rightFace(m_eT->adjTarget())], m_vT);
		OGDF_ASSERT(m_primalEdge[eDual] == 0 || m_expToG[m_primalEdge[eDual]] != 0);
	}
}


//-------------------------------------------------------------------
// find shortest path in dual from m_vS to m_vT; output this path
// in L by omitting first and last edge, and translating edges to G
//-------------------------------------------------------------------

void ExpandedGraph::findShortestPath(Graph::EdgeType eType, List<adjEntry> &L)
{
	NodeArray<edge> spPred(m_dual,0); // predecessor in shortest path tree
	List<edge> queue; // candidate edges

	// start with all edges leaving from m_vS
	edge e;
	forall_adj_edges(e,m_vS)
		queue.pushBack(e);

	for( ; ; ) {
		edge eCand = queue.popFrontRet(); // next candidate from front of queue
		node v = eCand->target();

		// hit an unvisited node ?
		if (spPred[v] == 0) {
			spPred[v] = eCand;

			// if it is m_vT, we have found the shortest path
			if (v == m_vT) {
				// build path from shortest path tree
				while(v != m_vS) {
					adjEntry adjExp = m_primalEdge[spPred[v]];
					if (adjExp != 0) // == nil for first and last edge
						L.pushFront(m_expToG[adjExp]);
					v = spPred[v]->source();
				}
				return;
			}

			// append next candidates to end of queue
			forall_adj_edges(e,v) {
				if(v == e->source() &&
					(eType != Graph::generalization || m_primalIsGen[e] == false))
				{
					queue.pushBack(e);
				}
			}
		}
	}
}


//-------------------------------------------------------------------
// find weighted shortest path in dual from m_vS to m_vT; output this path
// in L by omitting first and last edge, and translating edges to G
//-------------------------------------------------------------------

void ExpandedGraph::findWeightedShortestPath(Graph::EdgeType eType,
	List<adjEntry> &L)
{
	int maxCost = 0;
	edge eDual;
	forall_edges(eDual,m_dual) {
		int c = costDual(eDual);
		if (c > maxCost) maxCost = c;
	}

	++maxCost;
	Array<SListPure<edge> > nodesAtDist(maxCost);

	NodeArray<edge> spPred(m_dual,0); // predecessor in shortest path tree
	//List<edge> queue; // candidate edges

	// start with all edges leaving from m_vS
	edge e;
	forall_adj_edges(e,m_vS)
		nodesAtDist[0].pushBack(e);

	// actual search (using extended bfs on directed dual)
	int currentDist = 0;
	for( ; ; ) {
		// next candidate edge
		while(nodesAtDist[currentDist % maxCost].empty())
			++currentDist;

		edge eCand = nodesAtDist[currentDist % maxCost].popFrontRet();
		node v = eCand->target();

		// leads to an unvisited node ?
		if (spPred[v] == 0) {
			// yes, then we set v's predecessor in search tree
			spPred[v] = eCand;

			// have we reached t ...
			if (v == m_vT) {
				// ... then search is done.
				// construct list of used edges (translated to crossed
				// adjacency entries in G)
				while(v != m_vS) {
					adjEntry adjExp = m_primalEdge[spPred[v]];
					if (adjExp != 0) // == nil for first and last edge
						L.pushFront(m_expToG[adjExp]);
					v = spPred[v]->source();
				}
				return;
			}

			// append next candidates to end of queue
			// (all edges leaving v)
			forall_adj_edges(e,v) {
				if(v == e->source() &&
					(eType != Graph::generalization || m_primalIsGen[e] == false))
				{
					int listPos = (currentDist + costDual(e)) % maxCost;
					nodesAtDist[listPos].pushBack(e);
				}
			}
		}
	}
}


//-------------------------------------------------------------------
// find optimal edge insertion path from s to t for biconnected
// graph G (OptimalBlockInserter)
//-------------------------------------------------------------------

void VariableEmbeddingInserter::blockInsert(const BiconnectedComponent &BC,
	node s,
	node t,
	List<adjEntry> &L)
{
	L.clear();

	// construct SPQR-tree
	StaticPlanarSPQRTree T(BC);
	const Graph &tree = T.tree();


	// find allocation nodes of s and t and representatives in skeletons
	NodeArray<node> containsS(tree,0);
	NodeArray<node> containsT(tree,0);

	node v, w;
	forall_nodes(v,tree) {
		const Skeleton &S = T.skeleton(v);
		const Graph &M = S.getGraph();

		forall_nodes(w,M) {
			if (S.original(w) == s)
				containsS[m_v1 = v] = w;
			if (S.original(w) == t)
				containsT[m_v2 = v] = w;
		}
	}

	// find path in tree from an allocation node m_v1 of s to an
	// allocation m_v2 of t
	List<edge> path;
	pathSearch(m_v1,0,path);

	// remove unnecessary allocation nodes of s from start of path
	while(!path.empty() && containsS[w = path.front()->opposite(m_v1)] != 0)
	{
		m_v1 = w;
		path.popFront();
	}

	// remove unnecessary allocation nodes of t from end of path
	while(!path.empty() && containsT[w = path.back()->opposite(m_v2)] != 0)
	{
		m_v2 = w;
		path.popBack();
	}

	// call build_subpath for every R-node building the list L of crossed edges
	ExpandedGraph Exp(BC,T);

	if (T.typeOf(m_v1) == SPQRTree::RNode)
		buildSubpath(m_v1, 0, (path.empty()) ? 0 : path.front(), L, Exp, s, t);

	v = m_v1;
	ListConstIterator<edge> it;
	for(it = path.begin(); it.valid(); ++it)
	{
		edge e = *it;
		v = e->opposite(v);

		if (T.typeOf(v) == SPQRTree::RNode)
			buildSubpath(v, e,
				(it.succ().valid() == false) ? 0 : *(it.succ()), L, Exp, s, t);
	}
}


//-------------------------------------------------------------------
// recursive search for path from v1 to v2 in tree
//-------------------------------------------------------------------

bool VariableEmbeddingInserter::pathSearch(node v, edge parent, List<edge> &path)
{
	if (v == m_v2)
		return true;

	edge e;
	forall_adj_edges(e,v) {
		if (e == parent) continue;
		if (pathSearch(e->opposite(v),e,path) == true) {
			path.pushFront(e);
			return true;
		}
	}

	return false;
}


//-------------------------------------------------------------------
// find the shortest path from represent. of s to represent. of t in
// the dual of the (partially) expanded skeleton of v
//-------------------------------------------------------------------

void VariableEmbeddingInserter::buildSubpath(
	node v,
	edge eIn,
	edge eOut,
	List<adjEntry> &L,
	ExpandedGraph &Exp,
	node s,
	node t)
{
	// build expanded graph Exp
	Exp.expand(v,eIn,eOut);

	// construct augmented dual of expanded graph
	if(m_forbidCrossingGens)
		Exp.constructDualForbidCrossingGens(s,t);
	else
		Exp.constructDual(s,t,*m_pPG,m_forbiddenEdgeOrig);

	// find shortest path in augmented dual
	List<adjEntry> subpath;
	if(m_costOrig != 0)
		Exp.findWeightedShortestPath(m_typeOfCurrentEdge,subpath);
	else
		Exp.findShortestPath(m_typeOfCurrentEdge,subpath);

	L.conc(subpath);
}

} // end namespace ogdf

