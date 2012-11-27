/*
 * $Revision: 2599 $
 *
 * last checkin:
 *   $Author: chimani $
 *   $Date: 2012-07-15 22:39:24 +0200 (So, 15. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of cluster planarity tests and cluster
 * planar embedding for c-connected clustered graphs. Based on
 * the algorithm by Cohen, Feng and Eades which uses PQ-trees.
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


#include <ogdf/cluster/CconnectClusterPlanar.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/extended_graph_alg.h>

namespace ogdf {

// Constructor
CconnectClusterPlanar::CconnectClusterPlanar()
{
	ogdf::strcpy(errorCode,124,"\0");
	m_errorCode = none;
}

// Destructor
CconnectClusterPlanar::~CconnectClusterPlanar()
{
}

// Tests if a ClusterGraph is C-planar
// Specifies reason for non planarity
bool CconnectClusterPlanar::call(ClusterGraph &C, char (&code)[124])
{
	bool cPlanar =  call(C);
	strcpy(code,124,errorCode);
	return cPlanar;
}

// Tests if a ClusterGraph is C-planar
bool CconnectClusterPlanar::call(ClusterGraph &C)
{
	Graph G;
	ClusterGraph Cp(C,G);
	OGDF_ASSERT(Cp.consistencyCheck());


	m_clusterPQTree.init(Cp,0);

	bool cPlanar = preProcess(Cp,G);

	m_parallelEdges.init();
	m_isParallel.init();
	m_clusterPQTree.init();

	return cPlanar;
}



// Tests if a ClusterGraph is C-planar
bool CconnectClusterPlanar::call(const ClusterGraph &C)
{
	Graph G;
	ClusterGraph Cp(C,G);
	OGDF_ASSERT(Cp.consistencyCheck());
	OGDF_ASSERT(&G == &(Graph&) Cp);

	m_clusterPQTree.init(Cp,0);


	bool cPlanar = preProcess(Cp,G);

	m_parallelEdges.init();
	m_isParallel.init();
	m_clusterPQTree.init();

	return cPlanar;
}



//
//	CallTree:
//
//  call(ClusterGraph &C)
//
//		preProcess(ClusterGraph &C,Graph &G)
//
//			planarityTest(ClusterGraph &C,cluster &act,Graph &G)
//
//				foreach ChildCluster
//					planarityTest(ClusterGraph &C,cluster &act,Graph &G)
//
//				preparation(Graph  &G,cluster &cl)
//
//					foreach biconnected Component
//						doTest(Graph &G,NodeArray<int> &numbering,cluster &cl)
//






bool CconnectClusterPlanar::preProcess(ClusterGraph &C,Graph &G)
{
	if (!isCConnected(C))
	{
		ogdf::sprintf(errorCode,124,"Graph is not C-connected \n");
		m_errorCode = nonCConnected;
		return false;
	}

	if (!isPlanar(C))
	{
		ogdf::sprintf(errorCode,124,"Graph is not planar\n");
		m_errorCode = nonPlanar;
		return false;
	}

	cluster c;

	SListPure<node> selfLoops;
	makeLoopFree(G,selfLoops);

	c = C.rootCluster();

	bool cPlanar = planarityTest(C,c,G);

	return cPlanar;
}



// Recursive call for testing c-planarity of the clustered graph
// that is induced by cluster act
bool CconnectClusterPlanar::planarityTest(
	ClusterGraph &C,
	cluster &act,
	Graph &G)
{
	// Test children first
	ListConstIterator<cluster> it;
	for (it = act->cBegin(); it.valid();)
	{
		ListConstIterator<cluster> succ = it.succ();
		cluster next = (*it);
		if (!planarityTest(C,next,G))
			return false;
		it = succ;
	}

	// Get induced subgraph of cluster act and test it for planarity

	List<node> subGraphNodes;
	ListIterator<node> its;
	for (its = act->nBegin(); its.valid(); its++)
		subGraphNodes.pushBack(*its);

	Graph subGraph;
	NodeArray<node> table;
	inducedSubGraph(G,subGraphNodes.begin(),subGraph,table);


	// Introduce super sink and add edges corresponding
	// to outgoing edges of the cluster

	node superSink = subGraph.newNode();
	EdgeArray<node> outgoingTable(subGraph,0);



	for (its = act->nBegin(); its.valid(); its++)
	{
		node w = (*its);
		adjEntry adj = w->firstAdj();
		forall_adj(adj,w)
		{
			edge e = adj->theEdge();
			edge cor = 0;
			if (table[e->source()] == 0) // edge is connected to a node outside the cluster
			{
				cor = subGraph.newEdge(table[e->target()],superSink);
				outgoingTable[cor] = e->source();
			}
			else if (table[e->target()] == 0) // dito
			{
				cor = subGraph.newEdge(table[e->source()],superSink);
				outgoingTable[cor] = e->target();
			}

			// else edge connects two nodes of the cluster
		}
	}
	if (superSink->degree() == 0) // root cluster is not connected to outside clusters
	{
		subGraph.delNode(superSink);
		superSink = 0;
	}


	bool cPlanar = preparation(subGraph,act,superSink);


	if (cPlanar && act != C.rootCluster())
	{
		// Remove induced subgraph and the cluster act.
		// Replace it by a wheel graph
		while (!subGraphNodes.empty())
		{
			node w = subGraphNodes.popFrontRet();
//			C.unassignNode(w);
			G.delNode(w);
		}

		cluster parent = act->parent();

		if (superSink && m_clusterPQTree[act])
			constructWheelGraph(C,G,parent,m_clusterPQTree[act],outgoingTable);

		C.delCluster(act);
		if (m_clusterPQTree[act] != 0) // if query necessary for clusters with just one child
		{
			m_clusterPQTree[act]->emptyAllPertinentNodes();
			delete m_clusterPQTree[act];
		}

	}
	else if (!cPlanar)
		{
			ogdf::sprintf(errorCode,124,"Graph is not planar at cluster %d.\n",act->index());
			m_errorCode = nonCPlanar;
		}//if not cplanar

	return cPlanar;

}




void CconnectClusterPlanar::constructWheelGraph(ClusterGraph &C,
												Graph &G,
												cluster &parent,
												PlanarPQTree* T,
												EdgeArray<node> &outgoingTable)
{

	const PQNode<edge,IndInfo*,bool>* root = T->root();
	const PQNode<edge,IndInfo*,bool>*  checkNode = 0;

	Queue<const PQNode<edge,IndInfo*,bool>*> treeNodes;
	treeNodes.append(root);

	node correspond = G.newNode(); // Corresponds to the root node.
		 						   // root node is either a leaf or a P-node
	C.reassignNode(correspond,parent);

	Queue<node> graphNodes;
	graphNodes.append(correspond);

	node hub;
	node next = 0;
	node pre;
	node newNode; // corresponds to anchor of a hub or a cut node



	while (!treeNodes.empty())
	{
		checkNode = treeNodes.pop();
		correspond = graphNodes.pop();

		PQNode<edge,IndInfo*,bool>*  firstSon  = 0;
		PQNode<edge,IndInfo*,bool>*  nextSon   = 0;
		PQNode<edge,IndInfo*,bool>*  oldSib    = 0;
		PQNode<edge,IndInfo*,bool>*  holdSib   = 0;


		if (checkNode->type() == PQNodeRoot::PNode)
		{
			// correspond is a cut node

			OGDF_ASSERT(checkNode->referenceChild())
			firstSon = checkNode->referenceChild();

			if (firstSon->type() != PQNodeRoot::leaf)
			{
				treeNodes.append(firstSon);
				newNode = G.newNode();
				C.reassignNode(newNode,parent);
				graphNodes.append(newNode);
				G.newEdge(correspond,newNode);
			}
			else
			{
				// insert Edge to the outside
				PQLeaf<edge,IndInfo*,bool>* leaf =
					(PQLeaf<edge,IndInfo*,bool>*) firstSon;
				edge f = leaf->getKey()->m_userStructKey;
				//node x = outgoingTable[f];
				G.newEdge(correspond,outgoingTable[f]);
				delete leaf->getKey();
			}

			nextSon = firstSon->getNextSib(oldSib);
			oldSib = firstSon;
			pre = next;
			while (nextSon && nextSon != firstSon)
			{
				if (nextSon->type() != PQNodeRoot::leaf)
				{
					treeNodes.append(nextSon);
					newNode = G.newNode();  // new node corresponding to anchor
											// or cutnode
					C.reassignNode(newNode,parent);
					graphNodes.append(newNode);
					G.newEdge(correspond,newNode);
				}
				else
				{
					// insert Edge to the outside
					PQLeaf<edge,IndInfo*,bool>* leaf =
						(PQLeaf<edge,IndInfo*,bool>*) nextSon;
					edge f = leaf->getKey()->m_userStructKey;
					//node x = outgoingTable[f];
					G.newEdge(correspond,outgoingTable[f]);
					delete leaf->getKey();
				}
				holdSib = nextSon->getNextSib(oldSib);
				oldSib = nextSon;
				nextSon = holdSib;
			}

		}
 		else if (checkNode->type() == PQNodeRoot::QNode)
		{
			// correspond is the anchor of a hub
			OGDF_ASSERT(checkNode->getEndmost(PQNodeRoot::LEFT))
			firstSon = checkNode->getEndmost(PQNodeRoot::LEFT);

			hub = G.newNode();
			C.reassignNode(hub,parent);
			G.newEdge(hub,correspond); // link anchor and hub
			next = G.newNode();   // for first son
			C.reassignNode(next,parent);
			G.newEdge(hub,next);
			G.newEdge(correspond,next);

			if (firstSon->type() != PQNodeRoot::leaf)
			{
				treeNodes.append(firstSon);
				newNode = G.newNode();
				C.reassignNode(newNode,parent);
				graphNodes.append(newNode);
				G.newEdge(next,newNode);
			}
			else
			{
				// insert Edge to the outside
				PQLeaf<edge,IndInfo*,bool>* leaf =
					(PQLeaf<edge,IndInfo*,bool>*) firstSon;
				edge f = leaf->getKey()->m_userStructKey;
				//node x = outgoingTable[f];
				G.newEdge(next,outgoingTable[f]);
				delete leaf->getKey();
			}

			nextSon = firstSon->getNextSib(oldSib);
			oldSib = firstSon;
			pre = next;
			while (nextSon)
			{
				next = G.newNode();
				C.reassignNode(next,parent);
				G.newEdge(hub,next);
				G.newEdge(pre,next);
				if (nextSon->type() != PQNodeRoot::leaf)
				{
					treeNodes.append(nextSon);
					newNode = G.newNode();  // new node corresponding to anchor
											// or cutnode
					C.reassignNode(newNode,parent);
					graphNodes.append(newNode);

					G.newEdge(next,newNode);
				}
				else
				{
					// insert Edge to the outside
					PQLeaf<edge,IndInfo*,bool>* leaf =
						(PQLeaf<edge,IndInfo*,bool>*) nextSon;
					edge f = leaf->getKey()->m_userStructKey;
					G.newEdge(next,outgoingTable[f]);
					delete leaf->getKey();
				}
				holdSib = nextSon->getNextSib(oldSib);
				oldSib = nextSon;
				nextSon = holdSib;
				pre = next;

			}
			G.newEdge(next,correspond);
		}
	}

	OGDF_ASSERT(C.consistencyCheck());
}





//
// Prepare planarity test for one cluster
//
bool CconnectClusterPlanar::preparation(Graph  &G,
										cluster &cl,
										node superSink)
{

	node v;
	edge e;
	int  bcIdSuperSink = -1; // ID of biconnected component that contains superSink
							 // Initialization with -1 necessary for assertion
	bool cPlanar = true;


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
	for (int i = 0; i < bcCount; i++)
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
		if (superSink && mark[superSink])
		{
			OGDF_ASSERT(bcIdSuperSink == -1);
			bcIdSuperSink = i;
		}
		SListIterator<node> itn;
		for (itn = blockNodes[i].begin(); itn.valid(); ++itn)
		{
			v = *itn;
			if (mark[v])
				mark[v] = false;
			else
			{
				OGDF_ASSERT(mark[v]); // v has been placed two times on the list.
			}
		}

	}


	// Perform planarity test for every biconnected component

	if (bcCount == 1)
	{
		// Compute st-numbering
		NodeArray<int> numbering(G,0);
		int n;
		if (superSink)
			n = stNumber(G,numbering,0,superSink);
		else
			n = stNumber(G,numbering);
		OGDF_ASSERT_IF(dlConsistencyChecks,testSTnumber(G,numbering,n))


		EdgeArray<edge> backTableEdges(G,0);
		forall_edges(e,G)
			backTableEdges[e] = e;

		cPlanar = doTest(G,numbering,cl,superSink,backTableEdges);
	}
	else
	{
		for (int i = 0; i < bcCount; i++)
		{
			#ifdef OGDF_DEBUG
			if(int(ogdf::debugLevel)>=int(dlHeavyChecks)){
				cout<<endl<<endl<<"-----------------------------------";
				cout<<endl<<endl<<"Component "<<i<<endl;}
			#endif

			Graph C;

			SListIterator<node> itn;
			for (itn = blockNodes[i].begin(); itn.valid(); ++ itn)
			{
				v = *itn;
				node w = C.newNode();
				tableNodes[v] = w;

				#ifdef OGDF_DEBUG
				if(int(ogdf::debugLevel)>=int(dlHeavyChecks)){
					cout <<"Original: " << v << " New: " << w<< endl;}
				#endif

			}

			NodeArray<node> backTableNodes(C,0);

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

			// Compute st-numbering
			NodeArray<int> numbering(C,0);
			int n;
			if (bcIdSuperSink == i)
			{
				n = stNumber(C,numbering,0,tableNodes[superSink]);
				OGDF_ASSERT_IF(dlConsistencyChecks,testSTnumber(C,numbering,n))
				cPlanar = doTest(C,numbering,cl,tableNodes[superSink],backTableEdges);
			}
			else
			{
				n = stNumber(C,numbering);
				OGDF_ASSERT_IF(dlConsistencyChecks,testSTnumber(C,numbering,n))
				cPlanar = doTest(C,numbering,cl,0,backTableEdges);
			}

			if (!cPlanar)
				break;



		}

	}

	return cPlanar;
}


// Performs a planarity test on a biconnected component
// of G. numbering contains an st-numbering of the component.
bool CconnectClusterPlanar::doTest(
	Graph &G,
	NodeArray<int> &numbering,
	cluster &cl,
	node superSink,
	EdgeArray<edge> &edgeTable)
{
	node v;
	bool cPlanar = true;

	NodeArray<SListPure<PlanarLeafKey<IndInfo*>* > > inLeaves(G);
	NodeArray<SListPure<PlanarLeafKey<IndInfo*>* > > outLeaves(G);
	Array<node> table(G.numberOfNodes()+1);

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

	PlanarPQTree* T = new PlanarPQTree();

	T->Initialize(inLeaves[table[1]]);
	for (int i = 2; i < G.numberOfNodes(); i++)
	{
		if (T->Reduction(outLeaves[table[i]]))
		{
			T->ReplaceRoot(inLeaves[table[i]]);
			T->emptyAllPertinentNodes();

		}
		else
		{
			cPlanar = false;
			break;
		}
	}
	if (cPlanar && cl && superSink)
	{
		// Keep the PQTree to construct a wheelgraph
		// Replace the edge stored in the keys of T
		// by the original edges.
		// Necessary, since the edges currently in T
		// correspond to a graph that mirrors a biconnected
		// component and thus is deallocated

		SListIterator<PlanarLeafKey<IndInfo*>* >  it;
		int n = G.numberOfNodes();

		for (it = outLeaves[table[n]].begin(); it.valid(); ++it)
		{
			PQLeafKey<edge,IndInfo*,bool>* key = (PQLeafKey<edge,IndInfo*,bool>*) *it;
			key->m_userStructKey = edgeTable[key->m_userStructKey];
		}

		m_clusterPQTree[cl] = T;

	}
	else //if (cPlanar)
		delete T;

	// Cleanup
	forall_nodes(v,G)
	{
		if (v != superSink || !cPlanar)
		{
			while (!outLeaves[v].empty())
			{
				PlanarLeafKey<IndInfo*>* L = outLeaves[v].popFrontRet();
				delete L;
			}
		}
	}

	return cPlanar;
}



void CconnectClusterPlanar::prepareParallelEdges(Graph &G)
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




} // end namespace ogdf

