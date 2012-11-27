/*
 * $Revision: 2573 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-10 18:48:33 +0200 (Di, 10. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Reinsertion of deleted edges in embedded subgraph with
 * modeled cluster boundaries.
 *
 * \author Karsten Klein
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


#include <ogdf/cluster/CPlanarEdgeInserter.h>
#include <ogdf/basic/Queue.h>

namespace ogdf {


//Note that edge insertions in cluster (sub)graphs are always performed
//on already embedded graphs with modeled cluster boundaries
void CPlanarEdgeInserter::call(
	ClusterPlanRep& CPR,
	CombinatorialEmbedding& E,
	Graph& G,
	const List<NodePair>& origEdges,
	List<edge>& newEdges)
{
	OGDF_ASSERT(&E.getGraph() == &CPR)

	m_originalGraph = &G;
	FaceArray<node> nodeOfFace(E, 0);
	//NodeArray<face>&, faceOfNode(m_dualGraph, 0);
	EdgeArray<edge> arcRightToLeft(CPR, 0);//arc from srcadj face to tgtadj face
	EdgeArray<edge> arcLeftToRight(CPR, 0);//vice versa
	EdgeArray<edge> arcTwin(m_dualGraph, 0);
	m_arcOrig.init(m_dualGraph, 0);

	constructDualGraph(CPR, E, arcRightToLeft, arcLeftToRight, nodeOfFace, arcTwin);
	//the dual graph has a node for each face of E
	//and two arcs for every edge of CPR

	m_eStatus.init(m_dualGraph, 0); //1 = usable

	const ClusterGraph& CG = CPR.getClusterGraph();

	//every face is completely inside a cluster (at least root)
	//facenodes are associated with clusters
	NodeArray<cluster> clusterOfFaceNode(m_dualGraph, 0);
	deriveFaceCluster(CPR, E, CG, nodeOfFace, clusterOfFaceNode);

	//nodes representing the edge endpoints
	node uDummy = m_dualGraph.newNode();
	node vDummy = m_dualGraph.newNode();

	//for each edge (u,v) to be inserted, we need the path in the
	//cluster hierarchy to orient the dual arcs (set the status)
	ListConstIterator<NodePair> itE = origEdges.begin();
	while (itE.valid())
	{
		//m_eStatus.fill(0); do this manually
		//first, we temporarily insert connections from node dummies
		//to the faces adjacent to start- and endpoint of the edge
		node oSource = (*itE).m_src;
		node oTarget = (*itE).m_tgt;
		node u = CPR.copy(oSource);
		node v = CPR.copy(oTarget);

		List<cluster> cList;

		//we compute the cluster tree path between oS and oT
		CG.commonClusterPath(oSource, oTarget, cList);

		//-----------------------------------------------
		//orient the edges according to cluster structure
		//save which clusters are on path from u to v
		//(do this by setting their edge status)
		Array<bool> onPath(0,CG.clusterIdCount(), false);
		edge eArc;
		EdgeArray<bool> done(m_dualGraph, false);
		forall_edges(eArc, m_dualGraph)
		{
			if (done[eArc]) continue; //twin already processed
			if (arcTwin[eArc] == 0) {done[eArc] = true; continue;} //dummies

			cluster c1 = clusterOfFaceNode[eArc->source()];
			cluster c2 = clusterOfFaceNode[eArc->target()];

			ListIterator<cluster> itC = cList.begin();

			OGDF_ASSERT(itC.valid())
			//run over path and search for c1, c2
			int ind = 1, ind1 = 0, ind2 = 0;
			while (itC.valid())
			{
				cluster cCheck = (*itC);

				if (cCheck == c1)
				{
					ind1 = ind;
				}//if
				if (cCheck == c2)
				{
					ind2 = ind;
				}//if

				itC++;
				ind++;

				//stop search, both clusters found
				if ((ind1 > 0) && (ind2 > 0))
					itC = cList.rbegin().succ();
			}//while
			//set status
			if ((ind1 > 0 ) && (ind2 > 0))
			{
				if (ind1 == ind2) //bidirectional
				{
					m_eStatus[eArc] = 1;
					m_eStatus[arcTwin[eArc]] = 1;
				}//if both
				else
					if (ind1 < ind2)
					{
						m_eStatus[eArc] = 1;
						m_eStatus[arcTwin[eArc]] = 0;
					}
					else
					{
						m_eStatus[eArc] = 0;
						m_eStatus[arcTwin[eArc]] = 1;
					}

			}
			else
			{
				//remove edge
				m_eStatus[eArc] = 0;
				m_eStatus[arcTwin[eArc]] = 0;
			}

			done[arcTwin[eArc]] = true;
			done[eArc] = true;
		}//foralledges

		//----------------------------
		//we compute the shortest path
		SList<adjEntry> crossed;
		findShortestPath(E, u, v, uDummy, vDummy, crossed, nodeOfFace);

		//------------------
		//we insert the edge
		edge newOR = insertEdge(CPR, E, *itE, nodeOfFace, arcRightToLeft, arcLeftToRight,
								arcTwin, clusterOfFaceNode, crossed);
		newEdges.pushBack(newOR);

		//---------------------------------------------------------------
		//we updated the dual graph and are ready to insert the next edge

		itE++;

	}//while edges to be inserted

	//delete artificial endpoint representations
	m_dualGraph.delNode(vDummy);
	m_dualGraph.delNode(uDummy);

}//call


//*****************************************************************************
// protected member functions

void CPlanarEdgeInserter::constructDualGraph(ClusterPlanRep& CPR,
											 CombinatorialEmbedding& E,
											 EdgeArray<edge>& arcRightToLeft,
											 EdgeArray<edge>& arcLeftToRight,
		 									 FaceArray<node>& nodeOfFace,
											 //NodeArray<face>&, faceOfNode,
											 EdgeArray<edge>& arcTwin)
{
	//dual graph gets two arcs for each edge (in both directions)
	//these arcs get their status (usable for path) depending on
	//the edge to be reinserted

	m_dualGraph.clear();
	//faceOfNode.init(m_dualGraph, 0);

	//*********************************
	//construct nodes
	//corresponding to the graphs faces
	face f;
	for (f = E.firstFace(); f; f = f->succ())
	{
		node v = m_dualGraph.newNode();
		nodeOfFace[f] = v;
		//faceOfNode[v] = f;
	}

	//*********************************
	//
	edge e;
	forall_edges(e, CPR)
	{
		edge arc1 = m_dualGraph.newEdge( nodeOfFace[E.rightFace(e->adjTarget())],
			nodeOfFace[E.rightFace(e->adjSource())] );
		arcLeftToRight[e] = arc1;
		edge arc2 = m_dualGraph.newEdge( nodeOfFace[E.rightFace(e->adjSource())],
			nodeOfFace[E.rightFace(e->adjTarget())] );
		arcRightToLeft[e] = arc2;
		arcTwin[arc1] = arc2;
		arcTwin[arc2] = arc1;
		m_arcOrig[arc1] = e->adjSource();//e->adjTarget();
		m_arcOrig[arc2] = e->adjTarget();//e->adjSource();
	}

}//constructDualGraph


//*****************************************************************************
//private functions
void CPlanarEdgeInserter::deriveFaceCluster(ClusterPlanRep& CPR,
											CombinatorialEmbedding& E,
											const ClusterGraph& CG,
											FaceArray<node>& nodeOfFace,
											NodeArray<cluster>& clusterOfFaceNode)
{
	//we need to map indices to clusters
	cluster ci;
	//cluster numbers don't need to be consecutive
	HashArray<int, cluster> ClusterOfIndex;
	forall_clusters(ci, CG)
	{
		ClusterOfIndex[ci->index()] = ci; //numbers are unique
	}//forallclusters

	face f;
	for (f = E.firstFace(); f; f = f->succ())
	{
		//we examine all face nodes
		//nodes v with original define the cluster in which the face lies
		//- it's cluster(original(v))
		//dummy nodes can sit on unbounded many different cluster boundaries
		//either one is the parent of another (=> is the searched face)
		//or all lie in the same parent face
		cluster c1 = 0;
		cluster cResult = 0;
		adjEntry adjE;
		forall_face_adj(adjE, f)
		{
			node v = adjE->theNode();
			if (CPR.original(v))
			{
				cResult = CG.clusterOf(CPR.original(v));
				break;
			}//if original
			else
			{
				//a dummy node on a cluster boundary
				cluster c = ClusterOfIndex[CPR.ClusterID(v)];
				if (!c1) c1 = c;
				else
				{
					if (c != c1)
					{
						//either they lie in the same parent or one is the parent
						//of the other one
						OGDF_ASSERT( (c->parent() == c1->parent()) || (c1 == c->parent()) || (c == c1->parent()) );
						if (c1 == c->parent())
						{
							cResult = c1;
							break;
						}//if
						if (c == c1->parent())
						{
							cResult = c;
							break;
						}//if
						if (c->parent() == c1->parent())
						{
							cResult = c->parent();
							break;
						}//if

					}//if
				}//else c1
			}//else
		}//forall face adjacencies

		OGDF_ASSERT(cResult);
		clusterOfFaceNode[nodeOfFace[f]] = cResult;
	}//for all faces
}//deriveFaceCluster


//---------------------------------------------------------
// finds a shortest path in the dual graph augmented by s and t (represented
// by sDummy and tDummy); returns list of crossed adjacency entries (corresponding
// to used edges in the dual) in crossed.
//
void CPlanarEdgeInserter::findShortestPath(
	const CombinatorialEmbedding &E,
	node s, //edge startpoint
	node t,	//edge endpoint
	node sDummy, //representing s in network
	node tDummy, //representing t in network
	//Graph::EdgeType eType,
	SList<adjEntry> &crossed,
	FaceArray<node>& nodeOfFace)
{
	OGDF_ASSERT(s != t)

	OGDF_ASSERT(sDummy->graphOf() == tDummy->graphOf())

	OGDF_ASSERT(s->graphOf() == t->graphOf())

	NodeArray<edge> spPred(m_dualGraph,0);
	QueuePure<edge> queue;
	int oldIdCount = m_dualGraph.maxEdgeIndex();

	//list of current best path
	SList<adjEntry> bestCrossed;
	SList<adjEntry> currentCrossed;
	//int bestCost = 4*m_dualGraph.numberOfEdges(); //just an upper bound

	adjEntry adjE;
	//insert connections to adjacent faces
	//be careful with selfloops and bridges (later)
	forall_adj(adjE, s)
	{
		edge eNew = m_dualGraph.newEdge(sDummy, nodeOfFace[E.rightFace(adjE)]);
		m_arcOrig[eNew] = adjE;
		m_eStatus[eNew] = 1;
	}//foralladj
	forall_adj(adjE, t)
	{
		edge eNew = m_dualGraph.newEdge(nodeOfFace[E.rightFace(adjE)], tDummy);
		m_arcOrig[eNew] = adjE;
		m_eStatus[eNew] = 1;
	}//foralladj

	// Start with outgoing edges
	adjEntry adj;
	forall_adj(adj, sDummy) {
		// starting edges of bfs-search are all edges leaving s
		//edge eDual = m_dual.newEdge(m_vS, m_nodeOf[E.rightFace(adj)]);
		//m_primalAdj[eDual] = adj;
		queue.append(adj->theEdge());
	}
	OGDF_ASSERT(!queue.empty())
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
			if (v == tDummy)
			{
				// ... then search is done.
				// We should not stop here but calculate the cost and save
				// the path if it is an improvement

				// constructed list of used edges (translated to crossed
				// adjacency entries in PG) from t back to s (including first
				// and last!)

				do {
					edge eDual = spPred[v];
					if (m_arcOrig[eDual] != 0)
						currentCrossed.pushFront(m_arcOrig[eDual]);
					v = eDual->source();
				} while(v != sDummy);

				//break;
				//now check if the current solution is cheaper than bestCrossed
				//min cross is 1 at this point
				bool betterSol = false;
				if (bestCrossed.empty()) betterSol = true;
				if (!betterSol)
				{
					//derive actual cost
					SListIterator<adjEntry> cit = currentCrossed.begin();
					while (cit.valid())
					{
						//here we can check different edge costs

						//only temporary: just fill in
						bestCrossed.pushBack((*cit));
						cit++;
					}//while
				}//if not bestcrossed empty: compare
				//cop current into best
				if (betterSol)
				{
					SListIterator<adjEntry> cit = currentCrossed.begin();
					while (cit.valid())
					{
						bestCrossed.pushBack((*cit));
						cit++;
					}
				}

				break;//only temporary, rebuild path later
			}//if target found

			// append next candidate edges to queue
			// (all edges leaving v)
			edge e;
			forall_adj_edges(e,v) {
				if ((v == e->source()) &&
					(m_eStatus[e] == 1) )
				{
					queue.append(e);
				}
			}
		}
	}

	//set result in list parameter
	SListIterator<adjEntry> cit = bestCrossed.begin();
	while (cit.valid())
	{
		crossed.pushBack((*cit));
		cit++;
	}

	bestCrossed.clear();
	currentCrossed.clear();

	//--------------
	//delete dummies
	//connections and update graph
	List<edge> delMe;
	forall_adj(adjE,sDummy)
	{
		delMe.pushBack(adjE->theEdge());
	}
	while (!delMe.empty())
		m_dualGraph.delEdge(delMe.popFrontRet());

	forall_adj(adjE,tDummy)
	{
		delMe.pushBack(adjE->theEdge());
	}
	while (!delMe.empty())
		m_dualGraph.delEdge(delMe.popFrontRet());
/*
	// remove augmented edges again
	while ((adj = sDummy->firstAdj()) != 0)
		m_dualGraph.delEdge(adj->theEdge());

	while ((adj = tDummy->firstAdj()) != 0)
		m_dualGraph.delEdge(adj->theEdge());


	*/
	m_dualGraph.resetEdgeIdCount(oldIdCount);
}//shortestPath

//---------------------------------------------------------
// inserts edge e according to insertion path crossed.
// updates embeding and dual graph
//
edge CPlanarEdgeInserter::insertEdge(
	ClusterPlanRep &CPR,
	CombinatorialEmbedding &E,
	const NodePair& np,
	FaceArray<node>& nodeOfFace,
	EdgeArray<edge>& arcRightToLeft,
	EdgeArray<edge>& arcLeftToRight,
	EdgeArray<edge>& arcTwin,
	NodeArray<cluster>& clusterOfFaceNode,
	const SList<adjEntry> &crossed)
{
	// remove dual nodes on insertion path

	List<cluster> faceCluster; //clusters of deleted faces

	//first node double, what about last?
	SListConstIterator<adjEntry> it;
	Stack<node> delS;
	it = crossed.begin();
	while(it.valid())
	//for(it = crossed.begin(); it != crossed.rbegin(); ++it)
	{
		//m_dualGraph.delNode(nodeOfFace[E.rightFace(*it)]);
		if (!delS.empty())
		{
			if (!(delS.top() == nodeOfFace[E.rightFace(*it)]))
			{
				delS.push(nodeOfFace[E.rightFace(*it)]);
				faceCluster.pushBack(clusterOfFaceNode[nodeOfFace[E.rightFace(*it)]]);
			}
		}
		else
		{
			delS.push(nodeOfFace[E.rightFace(*it)]);
			faceCluster.pushBack(clusterOfFaceNode[nodeOfFace[E.rightFace(*it)]]);
		}
		it++;
	}//while/for

	while (!delS.empty())
	{
		m_dualGraph.delNode(delS.pop());
	}
	/*
	for(it = crossed.begin(); it.valid(); it++)
	{
	//it != crossed.rbegin(); ++it) {
		//only dummy edge
		if (!( (CPR.copy(np.m_src) == (*it)->theNode())
			 || (CPR.copy(np.m_tgt) == (*it)->theNode())))
			m_dualGraph.delNode(nodeOfFace[E.rightFace(*it)]);
	}*/

	//---------------------
	//update original Graph

	adjEntry orE;
	it = crossed.begin();
	edge e = CPR.original((*it)->theEdge());
	OGDF_ASSERT(e)
	OGDF_ASSERT((np.m_src == e->source()) || (np.m_src == e->target()))
	if (np.m_src == e->source()) orE = e->adjSource();
	else orE = e->adjTarget();
	adjEntry orF;
	it = crossed.rbegin();
	e = CPR.original((*it)->theEdge());
	OGDF_ASSERT(e)
	OGDF_ASSERT((np.m_tgt == e->source()) || (np.m_tgt == e->target()))
	if (np.m_tgt == e->source()) orF = e->adjSource();
	else orF = e->adjTarget();
	//*****************
	edge orEdge = m_originalGraph->newEdge(orE, orF);//(np.m_src, np.m_tgt);
	//**************
	// update primal
	CPR.insertEdgePathEmbedded(orEdge,E,crossed);


	// insert new face nodes into dual
	const List<edge> &path = CPR.chain(orEdge);
	ListConstIterator<edge> itEdge;

	OGDF_ASSERT(faceCluster.size() == path.size())
	ListConstIterator<cluster> itC = faceCluster.begin();

	for(itEdge = path.begin(); itEdge.valid(); ++itEdge)
	{
		adjEntry adj = (*itEdge)->adjSource();
		nodeOfFace[E.leftFace (adj)] = m_dualGraph.newNode();
		nodeOfFace[E.rightFace(adj)] = m_dualGraph.newNode();
		clusterOfFaceNode[nodeOfFace[E.leftFace (adj)]] = (*itC);
		clusterOfFaceNode[nodeOfFace[E.rightFace (adj)]] = (*itC);
		itC++;

	}

	//*****************************
	//update network for both faces

	// insert new edges into dual
	for(itEdge = path.begin(); itEdge.valid(); ++itEdge)
	{
		adjEntry adjSrc = (*itEdge)->adjSource();
		face f = E.rightFace(adjSrc);  // face to the right of adj in loop
		node vRight = nodeOfFace[f];

		adjEntry adj1 = f->firstAdj(), adj = adj1;
		do {
			node vLeft = nodeOfFace[E.leftFace(adj)];

			edge eLR = m_dualGraph.newEdge(vLeft,vRight);
			m_arcOrig[eLR] = adj;

			edge eRL = m_dualGraph.newEdge(vRight,vLeft);
			m_arcOrig[eRL] = adj->twin();

			arcTwin[eLR] = eRL;
			arcTwin[eRL] = eLR;

			//now check if edge can be used
			setArcStatus(eLR, np.m_src, np.m_tgt, CPR.getClusterGraph(),
						 clusterOfFaceNode, arcTwin);

			if (adj == adj->theEdge()->adjSource())
			{
				arcLeftToRight[adj->theEdge()] = eLR;
				arcRightToLeft[adj->theEdge()] = eRL;
				//m_arcOrig[eLR] = e->adjTarget();
				//m_arcOrig[eRL] = e->adjSource();
			}
			else
			{
				arcLeftToRight[adj->theEdge()] = eRL;
				arcRightToLeft[adj->theEdge()] = eLR;
				//m_arcOrig[eRL] = e->adjTarget();
				//m_arcOrig[eLR] = e->adjSource();
			}

		}
		while((adj = adj->faceCycleSucc()) != adj1);

		// the other face adjacent to *itEdge ...
		f = E.rightFace(adjSrc->twin());
		vRight = nodeOfFace[f];

		adj1 = f->firstAdj();
		adj = adj1;
		do {
			node vLeft = nodeOfFace[E.leftFace(adj)];

			edge eLR = m_dualGraph.newEdge(vLeft,vRight);
			m_arcOrig[eLR] = adj;

			edge eRL = m_dualGraph.newEdge(vRight,vLeft);
			m_arcOrig[eRL] = adj->twin();

			arcTwin[eLR] = eRL;
			arcTwin[eRL] = eLR;

			if (adj == adj->theEdge()->adjSource())
			{
				arcLeftToRight[adj->theEdge()] = eLR;
				arcRightToLeft[adj->theEdge()] = eRL;
				//m_arcOrig[eLR] = e->adjTarget();
				//m_arcOrig[eRL] = e->adjSource();
			}
			else
			{
				arcLeftToRight[adj->theEdge()] = eRL;
				arcRightToLeft[adj->theEdge()] = eLR;
				//m_arcOrig[eRL] = e->adjTarget();
				//m_arcOrig[eLR] = e->adjSource();
			}

		}
		while((adj = adj->faceCycleSucc()) != adj1);
	}
	return orEdge;
}//insertEdge


//sets status for new arc and twin
//uses dual arc, original nodes of edge to be inserted, ClusterGraph
void CPlanarEdgeInserter::setArcStatus(
	edge eArc,
	node oSrc,
	node oTgt,
	const ClusterGraph& CG,
	NodeArray<cluster>& clusterOfFaceNode,
	EdgeArray<edge>& arcTwin)
{
	cluster c1 = clusterOfFaceNode[eArc->source()];
	cluster c2 = clusterOfFaceNode[eArc->target()];
			//cout<< "Searching for clusters " << c1 << " and " << c2 << "\n";
	List<cluster> cList;

	//we compute the cluster tree path between oS and oT
	CG.commonClusterPath(oSrc, oTgt, cList);
	ListIterator<cluster> itC = cList.begin();
	OGDF_ASSERT(itC.valid())
	//run over path and search for c1, c2
	int ind = 0, ind1 = 0, ind2 = 0;
	while (itC.valid())
	{
		cluster cCheck = (*itC);
		//cout << "Checking " << cCheck << "\n" << flush;

		if (cCheck == c1)
		{
			ind1 = ind;
				 //cout << "Found c1 " << cCheck << " at number "<< ind << "\n" << flush;
		}//if
		if (cCheck == c2)
		{
			ind2 = ind;
				 //cout << "Found c2 " << cCheck << " at number "<< ind << "\n" << flush;
		}//if

		itC++;
		ind++;

		//stop search, both clusters found
		if ((ind1 > 0) && (ind2 > 0))
			itC = cList.rbegin().succ();
	}//while

	//**********
	//set status
	OGDF_ASSERT(arcTwin[eArc])
	if ((ind1 > 0 ) && (ind2 > 0))
	{
		if (ind1 == ind2) //bidirectional
		{
			m_eStatus[eArc] = 1;
			m_eStatus[arcTwin[eArc]] = 1;
		}//if both
		else
			if (ind1 < ind2)
			{
				m_eStatus[eArc] = 1;
				m_eStatus[arcTwin[eArc]] = 0;
			}
			else
			{
				m_eStatus[eArc] = 0;
				m_eStatus[arcTwin[eArc]] = 1;
			}

	}//if
	else
	{
		//remove edge
		m_eStatus[eArc] = 0;
		m_eStatus[arcTwin[eArc]] = 0;
	}//else
}//setArcStatus


//************************************
//improve the insertion result by heuristics
//TODO
void CPlanarEdgeInserter::postProcess()
{
	switch (m_ppType)
	{
		case ppRemoveReinsert:

			break;
		default:
			break;
	}//switch
}//postprocess

//*****************************************************************************
//file output

void CPlanarEdgeInserter::writeDual(const char *fileName)
{
	Layout drawing(m_dualGraph);
	ofstream os(fileName);
	writeGML(os,drawing);
}


void CPlanarEdgeInserter::writeGML(ostream &os, const Layout &drawing)
{
	const Graph &G = m_dualGraph;

	NodeArray<int> id(m_dualGraph);
	int nextId = 0;

	os.setf(ios::showpoint);
	os.precision(10);

	os << "Creator \"ogdf::CPlanarEdgeInserter::writeGML\"\n";
	os << "graph [\n";
	os << "  directed 1\n";

	node v;
	forall_nodes(v,G) {
		os << "  node [\n";

		os << "    id " << (id[v] = nextId++) << "\n";
		os << "    label \"" << v->index() << "\"\n";

		os << "    graphics [\n";
		os << "      x " << drawing.x(v) << "\n";
		os << "      y " << drawing.y(v) << "\n";
		os << "      w " << 10.0 << "\n";
		os << "      h " << 10.0 << "\n";
		os << "      type \"rectangle\"\n";
		os << "      width 1.0\n";

		os << "      type \"oval\"\n";
		os << "      fill \"#00FF00\"\n";

		os << "    ]\n"; // graphics

		os << "  ]\n"; // node
	}


	edge e;
	forall_edges(e,G)
	{
		os << "  edge [\n";

		os << "    source " << id[e->source()] << "\n";
		os << "    target " << id[e->target()] << "\n";

		os << "    graphics [\n";

		os << "      type \"line\"\n";
		os << "      arrow \"last\"\n";

		if (m_eStatus[e] > 0)
			os << "      fill \"#FF0000\"\n";
		else
			os << "      fill \"#0000FF\"\n";
		os << "      width 3.0\n";

		os << "    ]\n"; // graphics

		os << "  ]\n"; // edge
	}

	os << "]\n"; // graph
}


} // end namespace ogdf

