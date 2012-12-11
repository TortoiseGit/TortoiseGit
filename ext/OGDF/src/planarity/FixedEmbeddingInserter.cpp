/*
 * $Revision: 2559 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-06 15:04:28 +0200 (Fr, 06. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief implementation of FixedEmbeddingInserter class
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


#include <ogdf/planarity/FixedEmbeddingInserter.h>
#include <ogdf/basic/FaceArray.h>
#include <ogdf/basic/Queue.h>
#include <ogdf/basic/FaceSet.h>


namespace ogdf {


//---------------------------------------------------------
// constructor
// sets default values for options
//
FixedEmbeddingInserter::FixedEmbeddingInserter()
{
	m_rrOption = rrNone;
	m_percentMostCrossed = 25;
	m_keepEmbedding = false;
	m_runsPostprocessing = 0;
}


//---------------------------------------------------------
// FEICrossingsBucket
// bucket function for sorting edges by decreasing number
// of crossings
class FEICrossingsBucket : public BucketFunc<edge>
{
	const PlanRep *m_pPG;

public:
	FEICrossingsBucket(const PlanRep *pPG) :
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
Module::ReturnType FixedEmbeddingInserter::doCall(
	PlanRep &PG,
	const List<edge> &origEdges,
	bool forbidCrossingGens,
	const EdgeArray<int>  *costOrig,
	const EdgeArray<bool> *forbiddenEdgeOrig,
	const EdgeArray<unsigned int> *edgeSubGraph)
{

	double T;
	usedTime(T);

	ReturnType retValue = retFeasible;
	m_runsPostprocessing = 0;

	if(!m_keepEmbedding) PG.embed();
	OGDF_ASSERT(PG.representsCombEmbedding() == true);

	if (origEdges.size() == 0)
		return retOptimal;  // nothing to do

	// initialization
	CombinatorialEmbedding E(PG);  // embedding of PG

	m_dual.clear();
	m_primalAdj.init(m_dual);
	m_nodeOf.init(E);

	// construct dual graph
	m_primalIsGen.init(m_dual,false);

	OGDF_ASSERT(forbidCrossingGens == false || forbiddenEdgeOrig == 0);

	if(forbidCrossingGens)
		constructDualForbidCrossingGens((const PlanRepUML&)PG,E);
	else
		constructDual(PG,E,forbiddenEdgeOrig);

	// m_delFaces and m_newFaces are used by removeEdge()
	// if we can't allocate memory for them, we throw an exception
	if (removeReinsert() != rrNone) {
		m_delFaces = new FaceSetSimple(E);
		if (m_delFaces == 0)
			OGDF_THROW(InsufficientMemoryException);

		m_newFaces = new FaceSetPure(E);
		if (m_newFaces == 0) {
			delete m_delFaces;
			OGDF_THROW(InsufficientMemoryException);
		}

	// no postprocessing -> no removeEdge()
	} else {
		m_delFaces = 0;
		m_newFaces = 0;
	}

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

		int eSubGraph = 0;  // edgeSubGraph-data of eOrig
		if(edgeSubGraph!=0) eSubGraph = (*edgeSubGraph)[eOrig];

		SList<adjEntry> crossed;
		if(costOrig != 0) {
			findShortestPath(PG, E, *costOrig,
				PG.copy(eOrig->source()),PG.copy(eOrig->target()),
				forbidCrossingGens ? ((const PlanRepUML&)PG).typeOrig(eOrig) : Graph::association,
				crossed, edgeSubGraph, eSubGraph);
		} else {
			findShortestPath(E,
				PG.copy(eOrig->source()),PG.copy(eOrig->target()),
				forbidCrossingGens ? ((const PlanRepUML&)PG).typeOrig(eOrig) : Graph::association,
				crossed);
		}

		insertEdge(PG,E,eOrig,crossed,forbidCrossingGens,forbiddenEdgeOrig);

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
						pathLength = costCrossed(eOrigRR,PG,*costOrig,edgeSubGraph);
					else
						pathLength = PG.chain(eOrigRR).size() - 1;
					if (pathLength == 0) continue; // cannot improve

					removeEdge(PG,E,eOrigRR,forbidCrossingGens,forbiddenEdgeOrig);

					// try to find a better insertion path
					SList<adjEntry> crossed;
					if(costOrig != 0) {
						int eSubGraph = 0;  // edgeSubGraph-data of eOrig
						if(edgeSubGraph!=0) eSubGraph = (*edgeSubGraph)[eOrigRR];

						findShortestPath(PG, E, *costOrig,
							PG.copy(eOrigRR->source()),PG.copy(eOrigRR->target()),
							forbidCrossingGens ? ((const PlanRepUML&)PG).typeOrig(eOrigRR) : Graph::association,
							crossed, edgeSubGraph, eSubGraph);
					} else {
						findShortestPath(E,
							PG.copy(eOrigRR->source()),PG.copy(eOrigRR->target()),
							forbidCrossingGens ? ((const PlanRepUML&)PG).typeOrig(eOrigRR) : Graph::association,
							crossed);
					}

					// re-insert edge (insertion path cannot be longer)
					insertEdge(PG,E,eOrigRR,crossed,forbidCrossingGens,forbiddenEdgeOrig);

					int newPathLength = (costOrig != 0) ? costCrossed(eOrigRR,PG,*costOrig,edgeSubGraph) : (PG.chain(eOrigRR).size() - 1);
					OGDF_ASSERT(newPathLength <= pathLength);

					if(newPathLength < pathLength)
						improved = true;
				}
			} while (improved);
		}
	}

	const Graph &G = PG.original();
	if(removeReinsert() != rrIncremental && removeReinsert() != rrIncInserted) {
		// postprocessing (remove-reinsert heuristc)
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
				FEICrossingsBucket bucket(&PG);
				rrEdges.bucketSort(bucket);

				const int num = int(0.01 * percentMostCrossed() * G.numberOfEdges());
				itStop = rrEdges.get(num);
			}

			SListConstIterator<edge> it;
			for(it = rrEdges.begin(); it != itStop; ++it)
			{
				edge eOrig = *it;

				// remove only if crossings on edge;
				// in especially: forbidden edges are never handled by postprocessing
				//   since there are no crossings on such edges
				int pathLength;
				if(costOrig != 0)
					pathLength = costCrossed(eOrig,PG,*costOrig,edgeSubGraph);
				else
					pathLength = PG.chain(eOrig).size() - 1;
				if (pathLength == 0) continue; // cannot improve

				removeEdge(PG,E,eOrig,forbidCrossingGens,forbiddenEdgeOrig);

				// try to find a better insertion path
				SList<adjEntry> crossed;
				if(costOrig != 0) {
					int eSubGraph = 0;  // edgeSubGraph-data of eOrig
					if(edgeSubGraph!=0) eSubGraph = (*edgeSubGraph)[eOrig];

					findShortestPath(PG, E, *costOrig,
						PG.copy(eOrig->source()),PG.copy(eOrig->target()),
						forbidCrossingGens ? ((const PlanRepUML&)PG).typeOrig(eOrig) : Graph::association,
						crossed, edgeSubGraph, eSubGraph);
				} else {
					findShortestPath(E,
						PG.copy(eOrig->source()),PG.copy(eOrig->target()),
						forbidCrossingGens ? ((const PlanRepUML&)PG).typeOrig(eOrig) : Graph::association,
						crossed);
				}

				// re-insert edge (insertion path cannot be longer)
				insertEdge(PG,E,eOrig,crossed,forbidCrossingGens,forbiddenEdgeOrig);

				int newPathLength = (costOrig != 0) ? costCrossed(eOrig,PG,*costOrig,edgeSubGraph) : (PG.chain(eOrig).size() - 1);
				OGDF_ASSERT(newPathLength <= pathLength);

				if(newPathLength < pathLength)
					improved = true;
			}
		} while(improved); // iterate as long as we improve
	}

	// verify computed planarization
	OGDF_ASSERT(PG.representsCombEmbedding());
	OGDF_ASSERT(forbidCrossingGens == false || checkCrossingGens((const PlanRepUML&)PG) == true);

	// free resources
	delete m_newFaces;
	delete m_delFaces;

	m_primalIsGen.init();
	m_nodeOf.init();
	m_primalAdj.init();
	m_dual.clear();

	return retValue;
}

edge FixedEmbeddingInserter::crossedEdge(adjEntry adj) const
{
	edge e = adj->theEdge();

	adj = adj->cyclicSucc();
	while(adj->theEdge() == e)
		adj = adj->cyclicSucc();

	return adj->theEdge();
}


int FixedEmbeddingInserter::costCrossed(edge eOrig,
	const PlanRep &PG,
	const EdgeArray<int> &costOrig,
	const EdgeArray<unsigned int> *subgraphs) const
{
	int c = 0;

	const List<edge> &L = PG.chain(eOrig);

	ListConstIterator<edge> it = L.begin();
	for(++it; it.valid(); ++it) {
		edge e = PG.original(crossedEdge((*it)->adjSource()));
		if(subgraphs!=0) {
			int subgraphCounter = 0;
			for(int i=0; i<32; i++)
				if((((*subgraphs)[e] & (1<<i))!=0) && (((*subgraphs)[eOrig] & (1<<i))!=0))
					subgraphCounter++;
			c += subgraphCounter*costOrig[e];
		} else {
			c += costOrig[e];
		}
	}

	return c;
}


//---------------------------------------------------------
// construct dual graph, marks dual edges corresponding to generalization
// in m_primalIsGen
// assumes that m_pDual, m_primalAdj and m_nodeOf are already constructed
//
void FixedEmbeddingInserter::constructDualForbidCrossingGens(
	const PlanRepUML &PG,
	const CombinatorialEmbedding &E)
{
	// insert a node in the dual graph for each face in E
	face f;
	forall_faces(f,E)
		m_nodeOf[f] = m_dual.newNode();


	// Insert an edge into the dual graph for each adjacency entry in E.
	// The edges are directed from the left face to the right face.
	node v;
	forall_nodes(v,PG)
	{
		adjEntry adj;
		forall_adj(adj,v)
		{
			node vLeft  = m_nodeOf[E.leftFace (adj)];
			node vRight = m_nodeOf[E.rightFace(adj)];

			edge e = m_dual.newEdge(vLeft,vRight);
			m_primalAdj[e] = adj;

			// mark dual edges corresponding to generalizations
			if (PG.typeOf(adj->theEdge()) == Graph::generalization)
				m_primalIsGen[e] = true;
		}
	}

	// Augment the dual graph by two new vertices. These are used temporarily
	// when searching for a shortest path in the dual graph.
	m_vS = m_dual.newNode();
	m_vT = m_dual.newNode();
}



//---------------------------------------------------------
// construct dual graph
// assumes that m_pDual, m_primalAdj and m_nodeOf are already constructed
//
void FixedEmbeddingInserter::constructDual(
	const GraphCopy &GC,
	const CombinatorialEmbedding &E,
	const EdgeArray<bool> *forbiddenEdgeOrig)
{
	// insert a node in the dual graph for each face in E
	face f;
	forall_faces(f,E)
		m_nodeOf[f] = m_dual.newNode();


	// Insert an edge into the dual graph for each adjacency entry in E.
	// The edges are directed from the left face to the right face.
	node v;
	forall_nodes(v,GC)
	{
		adjEntry adj;
		forall_adj(adj,v)
		{
			// Do not insert edges into dual if crossing the original edge
			// is forbidden
			if(forbiddenEdgeOrig && (*forbiddenEdgeOrig)[GC.original(adj->theEdge())] == true)
				continue;

			node vLeft  = m_nodeOf[E.leftFace (adj)];
			node vRight = m_nodeOf[E.rightFace(adj)];

			m_primalAdj[m_dual.newEdge(vLeft,vRight)] = adj;
		}
	}

	// Augment the dual graph by two new vertices. These are used temporarily
	// when searching for a shortest path in the dual graph.
	m_vS = m_dual.newNode();
	m_vT = m_dual.newNode();
}



//---------------------------------------------------------
// finds a shortest path in the dual graph augmented by s and t (represented
// by m_vS and m_vT); returns list of crossed adjacency entries (corresponding
// to used edges in the dual) in crossed.
//
void FixedEmbeddingInserter::findShortestPath(
	const CombinatorialEmbedding &E,
	node s,
	node t,
	Graph::EdgeType eType,
	SList<adjEntry> &crossed)
{
	OGDF_ASSERT(s != t);

	NodeArray<edge> spPred(m_dual,0);
	QueuePure<edge> queue;
	int oldIdCount = m_dual.maxEdgeIndex();

	// augment dual by edges from s to all adjacent faces of s ...
	adjEntry adj;
	forall_adj(adj,s) {
		// starting edges of bfs-search are all edges leaving s
		edge eDual = m_dual.newEdge(m_vS, m_nodeOf[E.rightFace(adj)]);
		m_primalAdj[eDual] = adj;
		queue.append(eDual);
	}

	// ... and from all adjacent faces of t to t
	forall_adj(adj,t) {
		edge eDual = m_dual.newEdge(m_nodeOf[E.rightFace(adj)], m_vT);
		m_primalAdj[eDual] = adj;
	}

	// actual search (using bfs on directed dual)
	for( ; ;)
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
			if (v == m_vT)
			{
				// ... then search is done.
				// constructed list of used edges (translated to crossed
				// adjacency entries in PG) from t back to s (including first
				// and last!)

				do {
					edge eDual = spPred[v];
					crossed.pushFront(m_primalAdj[eDual]);
					v = eDual->source();
				} while(v != m_vS);

				break;
			}

			// append next candidate edges to queue
			// (all edges leaving v)
			edge e;
			forall_adj_edges(e,v) {
				if (v == e->source() &&
					(eType != Graph::generalization || m_primalIsGen[e] == false))
				{
					queue.append(e);
				}
			}
		}
	}


	// remove augmented edges again
	while ((adj = m_vS->firstAdj()) != 0)
		m_dual.delEdge(adj->theEdge());

	while ((adj = m_vT->firstAdj()) != 0)
		m_dual.delEdge(adj->theEdge());

	m_dual.resetEdgeIdCount(oldIdCount);
}



inline int getCost(const PlanRep &PG, const EdgeArray<int> &cost, edge e, const EdgeArray<unsigned int> *edgeSubGraph, int stSubGraph)
{
	edge eOrig = PG.original(e);
	if(edgeSubGraph==0)
		return (eOrig == 0) ? 0 : cost[eOrig];

	int edgeCost = 0;
	if(eOrig!=0) {
		for(int i=0; i<32; i++) {
			if((((*edgeSubGraph)[eOrig] & (1 << i)) != 0) && ((stSubGraph & (1 << i)) != 0))
				edgeCost++;
		}
		edgeCost *= cost[eOrig];
		edgeCost *= 10000;
		if(edgeCost == 0)
			edgeCost = 1;
	}
	return edgeCost;
}


//---------------------------------------------------------
// finds a weighted shortest path in the dual graph augmented by s and t
// (represented by m_vS and m_vT) using edges weights given by costOrig;
// returns list of crossed adjacency entries (corresponding
// to used edges in the dual) in crossed.
//
// running time: O(|dual| + L + C),
//   where L ist the weighted length of the insertion path and C the
//   maximum cost of an edge
//
void FixedEmbeddingInserter::findShortestPath(
	const PlanRep &PG,
	const CombinatorialEmbedding &E,
	const EdgeArray<int> &costOrig,
	node s,
	node t,
	Graph::EdgeType eType,
	SList<adjEntry> &crossed,
	const EdgeArray<unsigned int> *edgeSubGraph,
	int eSubGraph)
{

	OGDF_ASSERT(s != t);

	EdgeArray<int> costDual(m_dual, 0);
	int maxCost = 0;
	edge eDual;
	forall_edges(eDual, m_dual) {
		int c = getCost(PG,costOrig, m_primalAdj[eDual]->theEdge(), edgeSubGraph, eSubGraph);
		costDual[eDual] = c;
		if (c > maxCost)
			maxCost = c;
	}

	++maxCost;
	Array<SListPure<edge> > nodesAtDist(maxCost);

	NodeArray<edge> spPred(m_dual,0);

	int oldIdCount = m_dual.maxEdgeIndex();

	// augment dual by edges from s to all adjacent faces of s ...
	adjEntry adj;
	forall_adj(adj,s) {
		// starting edges of bfs-search are all edges leaving s
		edge eDual = m_dual.newEdge(m_vS, m_nodeOf[E.rightFace(adj)]);
		m_primalAdj[eDual] = adj;
		nodesAtDist[0].pushBack(eDual);
	}

	// ... and from all adjacent faces of t to t
	forall_adj(adj,t) {
		edge eDual = m_dual.newEdge(m_nodeOf[E.rightFace(adj)], m_vT);
		m_primalAdj[eDual] = adj;
	}

	// actual search (using extended bfs on directed dual)
	int currentDist = 0;

	for( ; ; )
	{
		// next candidate edge
		while(nodesAtDist[currentDist % maxCost].empty())
			++currentDist;

		edge eCand = nodesAtDist[currentDist % maxCost].popFrontRet();
		node v = eCand->target();

		// leads to an unvisited node?
		if (spPred[v] == 0)
		{
			// yes, then we set v's predecessor in search tree
			spPred[v] = eCand;

			// have we reached t ...
			if (v == m_vT)
			{
				// ... then search is done.
				// constructed list of used edges (translated to crossed
				// adjacency entries in PG) from t back to s (including first
				// and last!)

				do {
					edge eDual = spPred[v];
					crossed.pushFront(m_primalAdj[eDual]);
					v = eDual->source();
				} while(v != m_vS);

				break;
			}

			// append next candidate edges to queue
			// (all edges leaving v)
			edge e;
			forall_adj_edges(e,v) {
				if (v == e->source() &&
					(eType != Graph::generalization || m_primalIsGen[e] == false))
				{
					int listPos = (currentDist + costDual[e]) % maxCost;
					nodesAtDist[listPos].pushBack(e);
				}
			}
		}
	}


	// remove augmented edges again
	while ((adj = m_vS->firstAdj()) != 0)
		m_dual.delEdge(adj->theEdge());

	while ((adj = m_vT->firstAdj()) != 0)
		m_dual.delEdge(adj->theEdge());

	m_dual.resetEdgeIdCount(oldIdCount);
}



//---------------------------------------------------------
// inserts edge e according to insertion path crossed.
// updates embeding and dual graph
//
void FixedEmbeddingInserter::insertEdge(
	PlanRep &PG,
	CombinatorialEmbedding &E,
	edge eOrig,
	const SList<adjEntry> &crossed,
	bool forbidCrossingGens,
	const EdgeArray<bool> *forbiddenEdgeOrig)
{
	// remove dual nodes on insertion path
	SListConstIterator<adjEntry> it;
	for(it = crossed.begin(); it != crossed.rbegin(); ++it) {
		m_dual.delNode(m_nodeOf[E.rightFace(*it)]);
	}

	// update primal
	PG.insertEdgePathEmbedded(eOrig,E,crossed);


	// insert new face nodes into dual
	const List<edge> &path = PG.chain(eOrig);
	ListConstIterator<edge> itEdge;
	for(itEdge = path.begin(); itEdge.valid(); ++itEdge)
	{
		adjEntry adj = (*itEdge)->adjSource();
		m_nodeOf[E.leftFace (adj)] = m_dual.newNode();
		m_nodeOf[E.rightFace(adj)] = m_dual.newNode();
	}

	// insert new edges into dual
	for(itEdge = path.begin(); itEdge.valid(); ++itEdge)
	{
		adjEntry adjSrc = (*itEdge)->adjSource();
		face f = E.rightFace(adjSrc);  // face to the right of adj in loop
		node vRight = m_nodeOf[f];

		adjEntry adj1 = f->firstAdj(), adj = adj1;
		do {
			if(forbiddenEdgeOrig && (*forbiddenEdgeOrig)[PG.original(adj->theEdge())] == true)
				continue;

			node vLeft = m_nodeOf[E.leftFace(adj)];

			edge eLR = m_dual.newEdge(vLeft,vRight);
			m_primalAdj[eLR] = adj;

			edge eRL = m_dual.newEdge(vRight,vLeft);
			m_primalAdj[eRL] = adj->twin();

			if(forbidCrossingGens &&
				((const PlanRepUML&)PG).typeOf(adj->theEdge()) == Graph::generalization)
			{
				m_primalIsGen[eLR] = m_primalIsGen[eRL] = true;
			}
		}
		while((adj = adj->faceCycleSucc()) != adj1);

		// the other face adjacent to *itEdge ...
		f = E.rightFace(adjSrc->twin());
		vRight = m_nodeOf[f];

		adj1 = f->firstAdj();
		adj = adj1;
		do {
			if(forbiddenEdgeOrig && (*forbiddenEdgeOrig)[PG.original(adj->theEdge())] == true)
				continue;

			node vLeft = m_nodeOf[E.leftFace(adj)];

			edge eLR = m_dual.newEdge(vLeft,vRight);
			m_primalAdj[eLR] = adj;

			edge eRL = m_dual.newEdge(vRight,vLeft);
			m_primalAdj[eRL] = adj->twin();

			if(forbidCrossingGens &&
				((const PlanRepUML&)PG).typeOf(adj->theEdge()) == Graph::generalization)
			{
				m_primalIsGen[eLR] = m_primalIsGen[eRL] = true;
			}
		}
		while((adj = adj->faceCycleSucc()) != adj1);
	}
}


//---------------------------------------------------------
// removes edge eOrig; updates embedding and dual graph
//
void FixedEmbeddingInserter::removeEdge(
	PlanRep &PG,
	CombinatorialEmbedding &E,
	edge eOrig,
	bool forbidCrossingGens,
	const EdgeArray<bool> *forbiddenEdgeOrig)
{
	const List<edge> &path = PG.chain(eOrig);
	ListConstIterator<edge> itEdge;
	for(itEdge = path.begin(); itEdge.valid(); ++itEdge)
	{
		adjEntry adj = (*itEdge)->adjSource();
		m_delFaces->insert(E.leftFace  (adj));
		m_delFaces->insert(E.rightFace (adj));
	}

	// delete all corresponding nodes in dual
	SListConstIterator<face> itsF;
	for(itsF = m_delFaces->faces().begin(); itsF.valid(); ++itsF)
		m_dual.delNode(m_nodeOf[*itsF]);

	m_delFaces->clear();

	// remove edge path from PG
	PG.removeEdgePathEmbedded(E,eOrig,*m_newFaces);

	// update dual
	// insert new nodes
	ListConstIterator<face> itF;
	for(itF = m_newFaces->faces().begin(); itF.valid(); ++itF) {
		m_nodeOf[*itF] = m_dual.newNode();
	}

	// insert new edges into dual
	for(itF = m_newFaces->faces().begin(); itF.valid(); ++itF)
	{
		face f = *itF;  // face to the right of adj in loop
		node vRight = m_nodeOf[f];

		adjEntry adj1 = f->firstAdj(), adj = adj1;
		do {
			if(forbiddenEdgeOrig && (*forbiddenEdgeOrig)[PG.original(adj->theEdge())] == true)
				continue;

			node vLeft = m_nodeOf[E.leftFace(adj)];

			edge eLR = m_dual.newEdge(vLeft,vRight);
			m_primalAdj[eLR] = adj;

			edge eRL = m_dual.newEdge(vRight,vLeft);
			m_primalAdj[eRL] = adj->twin();

			if(forbidCrossingGens &&
				((const PlanRepUML&)PG).typeOf(adj->theEdge()) == Graph::generalization)
			{
				m_primalIsGen[eLR] = m_primalIsGen[eRL] = true;
			}
		}
		while((adj = adj->faceCycleSucc()) != adj1);
	}

	m_newFaces->clear();

}


} // end namespace ogdf
