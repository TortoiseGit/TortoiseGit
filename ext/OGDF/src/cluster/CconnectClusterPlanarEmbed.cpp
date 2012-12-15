/*
 * $Revision: 2599 $
 *
 * last checkin:
 *   $Author: chimani $
 *   $Date: 2012-07-15 22:39:24 +0200 (So, 15. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of Cluster Planarity tests and Cluster
 * Planar embedding for C-connected Cluster Graphs
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


//#include <qapplication.h>
#include <ogdf/basic/Graph.h>

#include <ogdf/cluster/CconnectClusterPlanarEmbed.h>

#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/basic/AdjEntryArray.h>

#include <ogdf/internal/planarity/EmbedPQTree.h>


namespace ogdf {

// Constructor
CconnectClusterPlanarEmbed::CconnectClusterPlanarEmbed()
{
	ogdf::strcpy(errorCode,124,"\0");
	m_errorCode = none;
}

// Destructor
CconnectClusterPlanarEmbed::~CconnectClusterPlanarEmbed()
{
}


// Tests if a ClusterGraph is c-planar and embedds it.
bool CconnectClusterPlanarEmbed::embed(ClusterGraph &C,Graph &G)
{

	OGDF_ASSERT(C.consistencyCheck())

	if (G.numberOfNodes() <= 1) return true;

	// Initialize Node and cluster arrays associated with original graph.
	m_instance = &C;
	m_nodeTableOrig2Copy.init(G,0);
	m_clusterTableOrig2Copy.init(C,0);
	m_clusterEmbedding.init(C,0);
	m_clusterSubgraph.init(C,0);
	m_clusterSubgraphHubs.init(C,0);
	m_clusterSubgraphWheelGraph.init(C,0);
	m_clusterClusterGraph.init(C,0);
	m_clusterNodeTableNew2Orig.init(C,0);
	m_clusterOutgoingEdgesAnker.init(C,0);
	m_clusterSuperSink.init(C,0);
	m_clusterPQContainer.init(C);
	m_unsatisfiedCluster.init(C,false);

	// Copy the graph (necessary, since we modify it throughout the planarity test)
	Graph Gcopy;
	ClusterGraph Ccopy(C,Gcopy,m_clusterTableOrig2Copy,m_nodeTableOrig2Copy);

	// Initialize translation tables for nodes and clusters
	m_clusterTableCopy2Orig.init(Ccopy,0);
	cluster c;
	forall_clusters(c,C)
	{
		cluster c1 = m_clusterTableOrig2Copy[c];
		m_clusterTableCopy2Orig[c1] = c;
	}
	m_nodeTableCopy2Orig.init(Gcopy,0);
	node v;
	forall_nodes(v,G)
	{
		node w = m_nodeTableOrig2Copy[v];
		m_nodeTableCopy2Orig[w] = v;
	}
	// Remove empty clusters
	SList<cluster> removeCluster;
	forall_clusters(c,Ccopy)
	{
		if (c->cCount() == 0 && c->nCount() == 0)
			removeCluster.pushBack(c);
	}
	while (!removeCluster.empty())
	{
		c = removeCluster.popFrontRet();
		m_unsatisfiedCluster[m_clusterTableCopy2Orig[c]] = true;
		cluster parent = c->parent();
		Ccopy.delCluster(c);
		if (parent->cCount() == 0 && parent->nCount() == 0)
			removeCluster.pushBack(parent);
	}
	while (Ccopy.rootCluster()->cCount() == 1 && Ccopy.rootCluster()->nCount() == 0)
	{
		c = (*(Ccopy.rootCluster()->cBegin()));
		m_unsatisfiedCluster[m_clusterTableCopy2Orig[c]] = true;
		Ccopy.delCluster(c);
	}

	OGDF_ASSERT(Ccopy.consistencyCheck());

	// Initialize node and cluster arrays associated with copied graph.
	m_clusterPQTree.init(Ccopy,0);
	m_currentHubs.init(Gcopy,false);
	m_wheelGraphNodes.init(Gcopy,0);
	m_outgoingEdgesAnker.init(Gcopy,0);

	// Planarity test
	bool cPlanar = preProcess(Ccopy,Gcopy);

	if (cPlanar)
	{
		OGDF_ASSERT(Gcopy.representsCombEmbedding())
		//OGDF_ASSERT(Ccopy.consistencyCheck());

		recursiveEmbed(Ccopy,Gcopy);
		OGDF_ASSERT(Ccopy.consistencyCheck());

		copyEmbedding(Ccopy,Gcopy,C,G);

		C.adjAvailable(true);

	}
	else
		nonPlanarCleanup(Ccopy,Gcopy);


	// Cleanup
	forall_clusters(c,C)
	{
		if (m_clusterSubgraph[c] != 0 && c != C.rootCluster())
			delete m_clusterSubgraph[c];
	}


	// Deinitialize all node and cluster arrays
	m_parallelEdges.init();
	m_isParallel.init();
	m_clusterPQTree.init();
	m_clusterEmbedding.init();
	m_clusterSubgraph.init();
	m_clusterSubgraphHubs.init();
	m_clusterSubgraphWheelGraph.init();
	m_clusterClusterGraph.init();
	m_clusterNodeTableNew2Orig.init();
	m_clusterOutgoingEdgesAnker.init();
	m_clusterSuperSink.init();
	m_clusterPQContainer.init();

	m_clusterTableOrig2Copy.init();
	m_clusterTableCopy2Orig.init();
	m_nodeTableOrig2Copy.init();
	m_nodeTableCopy2Orig.init();
	m_currentHubs.init();
	m_wheelGraphNodes.init();
	m_outgoingEdgesAnker.init();

	return cPlanar;
}



// Tests if a ClusterGraph is c-planar and embedds it.
// Specifies reason for non planarity
bool CconnectClusterPlanarEmbed::embed(ClusterGraph &C,Graph &G, char (&code)[124])
{

	bool cPlanar = embed(C,G);
	ogdf::strcpy(code,124,errorCode);
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
//				preparation(Graph  &G,cluster &origCluster)
//
//					foreach biconnected Component
//						doTest(Graph &G,NodeArray<int> &numbering,cluster &origCluster)
//



/*******************************************************************************
						copyEmbedding
********************************************************************************/

// Copies the embedding of Ccopy to C

void CconnectClusterPlanarEmbed::copyEmbedding(
	ClusterGraph &Ccopy,
	Graph &Gcopy,
	ClusterGraph &C,
	Graph &G)
{

	node vCopy;
	node v;
	cluster c;
	OGDF_ASSERT(Gcopy.representsCombEmbedding())

	OGDF_ASSERT(Ccopy.representsCombEmbedding())

	AdjEntryArray<adjEntry> adjTableCopy2Orig(Gcopy);
	AdjEntryArray<adjEntry> adjTableOrig2Copy(G);
	AdjEntryArray<bool>     visited(G,false);				 // For parallel edges
	EdgeArray<edge>         edgeTableCopy2Orig(Gcopy,0);     // Translation table for parallel edges
	EdgeArray<bool>         parallelEdge(Gcopy,false);		 // Marks parallel edges in copy Graph
	AdjEntryArray<adjEntry>	parallelEntryPoint(G,0);		 // For storing information on parallel
															 // edges for cluster adjlistst.
	AdjEntryArray<bool>		parallelToBeIgnored(Gcopy,false);// For storing information on parallel
															 // edges for cluster adjlistst.

	// prepare parallel Edges
	prepareParallelEdges(G);
	NodeArray<SListPure<adjEntry> > entireEmbedding(G);

	//process over all copy nodes
	forall_nodes(vCopy,Gcopy)
	{
		//get the original node
		node wOrig = m_nodeTableCopy2Orig[vCopy];

		adjEntry vAdj;

		//process over all adjacent copy edges
		SList<adjEntry> entries;
		Gcopy.adjEntries(vCopy,entries);
		SListIterator<adjEntry> itv;
		for (itv = entries.begin(); itv.valid(); itv++)
		{
			vAdj = *itv;
			node vN = vAdj->twinNode();
			node wN = m_nodeTableCopy2Orig[vN];
			m_nodeTableOrig2Copy[wN] = vN;

			adjEntry wAdj;
			forall_adj(wAdj,wOrig)
			{

				if (edgeTableCopy2Orig[vAdj->theEdge()] != 0 &&
					m_isParallel[edgeTableCopy2Orig[vAdj->theEdge()]])
					// Break if parallel edge (not a reference edge) that has already been assigned.
					break;
				if (wAdj->twinNode() == wN
					&& !visited[wAdj] && !m_isParallel[wAdj->theEdge()])
//					&& !m_isParallel[wAdj->theEdge()])
					// Either a non parallel edge or the reference edge of a set of
					// parallel edges.
				{
					adjTableCopy2Orig[vAdj] = wAdj;
					adjTableOrig2Copy[wAdj] = vAdj;
//					adjTableCopy2Orig[vAdj->twin()] = wAdj->twin();
//					adjTableOrig2Copy[wAdj->twin()] = vAdj->twin();
					edgeTableCopy2Orig[vAdj->theEdge()] = wAdj->theEdge();
					#ifdef OGDF_DEBUG
					if (int(ogdf::debugLevel) >= int(dlHeavyChecks)){
						cout << "Orig " << wAdj << " " << wAdj->index() << "\t twin " << wAdj->twin()->index() << endl;
						cout << "Copy " << vAdj << " " << vAdj->index() << "\t twin " << vAdj->twin()->index() << endl << endl;}
					//qDebug ("Visited: %d->%d %d", wAdj->theNode()->index(),
					//	wAdj->twinNode()->index(),
					//	wAdj->index());
					#endif
					entireEmbedding[wOrig].pushBack(wAdj);	// if no parallel edges exist,
															// this will be our embedding.
//					entireEmbedding[wN].pushFront(wAdj->twin());
					visited[wAdj] = true; // for multi-edges
//					visited[wAdj->twin()] = true; // for multi-edges
					break;
				}
				else if (wAdj->twinNode() == wN  && !visited[wAdj])
					// A parallel edge that is not the reference edge.
					// We need to set the translation table
				{
					adjTableCopy2Orig[vAdj] = wAdj;
					adjTableOrig2Copy[wAdj] = vAdj;
					adjTableCopy2Orig[vAdj->twin()] = wAdj->twin();
					adjTableOrig2Copy[wAdj->twin()] = vAdj->twin();
					edgeTableCopy2Orig[vAdj->theEdge()] = wAdj->theEdge();
					visited[wAdj] = true; // So we do not consider parallel edges twice.
					visited[wAdj->twin()] = true; // So we do not consider parallel edges twice.
				}

			}
		}
	}

	// Locate all parallel edges
	// Sort them within the adjacency lists,
	// such that they appear consecutively.
	NodeArray<SListPure<adjEntry> > newEntireEmbedding(G);
	NodeArray<SListPure<adjEntry> > newEntireEmbeddingCopy(Gcopy);

	if (m_parallelCount > 0)
	{
		forall_nodes(v,G)
		{
			SListIterator<adjEntry> it;
			for(it = entireEmbedding[v].begin();it.valid();it++)
			{
				edge e = (*it)->theEdge();

				if (!m_parallelEdges[e].empty())
				{
					// This edge is the reference edge
					// of a bundle of parallel edges

					ListIterator<edge> it;
					// If v is source of e, insert the parallel edges
					// in the order stored in the list.
					if (e->adjSource()->theNode() == v)
					{
						adjEntry adj = e->adjSource();

						newEntireEmbedding[v].pushBack(adj);
						newEntireEmbeddingCopy[m_nodeTableOrig2Copy[v]].pushBack(adjTableOrig2Copy[adj]);

						parallelEntryPoint[e->adjSource()] = adj;
						parallelToBeIgnored[adjTableOrig2Copy[adj]] = true;

						for (it = m_parallelEdges[e].begin(); it.valid(); it++)
						{
							edge parallel = (*it);
							adjEntry adj = parallel->adjSource()->theNode() == v ?
								parallel->adjSource() : parallel->adjTarget();
							parallelToBeIgnored[adjTableOrig2Copy[adj]] = true;
							#ifdef OGDF_DEBUG
							if (int(ogdf::debugLevel) >= int(dlHeavyChecks)){
								cout << adj << " " << adj->index() << "\t twin " << adj->twin()->index() << endl;}
							#endif
							newEntireEmbedding[v].pushBack(adj);
							newEntireEmbeddingCopy[m_nodeTableOrig2Copy[v]].pushBack(adjTableOrig2Copy[adj]);
						}
					}
					else
					// v is target of e, insert the parallel edges
					// in the opposite order stored in the list.
					// This keeps the embedding.
					{
						bool first = true;
						for (it = m_parallelEdges[e].rbegin(); it.valid(); it--)
						{
							edge parallel = (*it);
							adjEntry adj = parallel->adjSource()->theNode() == v ?
								parallel->adjSource() : parallel->adjTarget();
							parallelToBeIgnored[adjTableOrig2Copy[adj]] = true;

							newEntireEmbedding[v].pushBack(adj);
							newEntireEmbeddingCopy[m_nodeTableOrig2Copy[v]].pushBack(adjTableOrig2Copy[adj]);
							if (first)
							{
//								parallelEntryPoint[adjTableOrig2Copy[adj]] = adj;
								parallelEntryPoint[e->adjTarget()] = adj;
								first = false;
							}
						}
						adjEntry adj = e->adjTarget();

						newEntireEmbedding[v].pushBack(adj);
						newEntireEmbeddingCopy[m_nodeTableOrig2Copy[v]];//.pushBack(adjTableOrig2Copy[adj]);
						newEntireEmbeddingCopy[m_nodeTableOrig2Copy[v]].pushBack(adjTableOrig2Copy[adj]);
						parallelToBeIgnored[adjTableOrig2Copy[adj]] = true;
					}
				}//if parallel edges
				else if (!m_isParallel[e])
					// normal non-multi-edge
				{
					adjEntry adj = e->adjSource()->theNode() == v?
									e->adjSource() : e->adjTarget();

					newEntireEmbedding[v].pushBack(adj);
					newEntireEmbeddingCopy[m_nodeTableOrig2Copy[v]];//pushBack(adjTableOrig2Copy[adj]);
					adjTableOrig2Copy[adj];
					newEntireEmbeddingCopy[m_nodeTableOrig2Copy[v]].pushBack(adjTableOrig2Copy[adj]);
				}
				// else e is a multi-edge but not the reference edge
			}
		}

		forall_nodes(v,G)
			G.sort(v,newEntireEmbedding[v]);
		forall_nodes(v,Gcopy)
			Gcopy.sort(v,newEntireEmbeddingCopy[v]);

	}
	else
	{
		forall_nodes(v,G)
			G.sort(v,entireEmbedding[v]);
		OGDF_ASSERT(G.representsCombEmbedding())
	}

	adjEntry adj;

	OGDF_ASSERT(G.representsCombEmbedding())

	forall_clusters(c,Ccopy)
	{
		SListPure<adjEntry>		embedding;


		ListIterator<adjEntry> it;

		for(it = c->firstAdj();it.valid(); it++)
		{
			adj = *it;
			edge e = adj->theEdge();

			if (!m_parallelEdges[edgeTableCopy2Orig[e]].empty())
			{
				adjEntry padj = parallelEntryPoint[adjTableCopy2Orig[adj]];

				bool lastMultiEdgeFound = false;
				node target = padj->twinNode();

				while (!lastMultiEdgeFound) // Scan the parallel edges of e
											// in the original graph along the embedded
											// adjacency list of its target
				{
					if (padj->twinNode() == target) // is a multi edge
					{
						embedding.pushBack(padj);
						padj = padj->succ();
						if (!padj) break; //only multi edges
					}
					else		// Not a multi Edge
						break;
				}
			}
			else if (!parallelToBeIgnored[adj])
			{
				embedding.pushBack(adjTableCopy2Orig[adj]);
			}
		}

		C.makeAdjEntries(m_clusterTableCopy2Orig[c],embedding.begin());
	}

}

/*******************************************************************************
						nonPlanarCleanup
********************************************************************************/


// Deallocates all memory, if the cluster graph is not cluster planar

void CconnectClusterPlanarEmbed::nonPlanarCleanup(ClusterGraph &Ccopy,Graph &Gcopy)
{

	while (!m_callStack.empty())
	{
		cluster	act	= m_callStack.pop();

		Graph *subGraph	= m_clusterSubgraph[act];

		node superSink = m_clusterPQContainer[act].m_superSink;
		if (superSink)
		{
			edge e;
			forall_edges(e,*subGraph)
			{
				if (e->source() != superSink && e->target() != superSink)
					if ((*m_clusterOutgoingEdgesAnker[act])[e])
						delete (*m_clusterOutgoingEdgesAnker[act])[e];
			}
		}

		if (m_clusterEmbedding[act] != 0)
			delete m_clusterEmbedding[act];
		delete m_clusterSubgraphHubs[act];
		delete m_clusterSubgraphWheelGraph[act];
		delete m_clusterNodeTableNew2Orig[act];
		delete m_clusterOutgoingEdgesAnker[act];

		m_clusterPQContainer[act].Cleanup();
	}


	edge e;
	forall_edges(e,Gcopy)
	{
		if (m_outgoingEdgesAnker[e])
			delete m_outgoingEdgesAnker[e];
	}
}



/*******************************************************************************
						hubControl
********************************************************************************/


// This function is called by recursiveEmbed only. It fixes
// the adjacency lists of the hubs in Gcopy after a cluster has been
// reembedded.

void CconnectClusterPlanarEmbed::hubControl(Graph &G,NodeArray<bool> &hubs)
{
	node hub;
	forall_nodes(hub,G)
	{
		if (hubs[hub]) // hub is a hub
		{
			node firstNode;
			node secNode;

			adjEntry startAdj = hub->firstAdj();
			adjEntry firstAdj = 0;
			adjEntry secAdj = 0;
			while (firstAdj != startAdj)
			{
				if (firstAdj == 0)
					firstAdj = startAdj;
				secAdj = firstAdj->cyclicSucc();
				firstNode = firstAdj->twinNode();
				secNode = secAdj->twinNode();

				adjEntry cyclicPredOfFirst = firstAdj->twin()->cyclicPred();
				while(cyclicPredOfFirst->twinNode()
						!= secNode)
				{
					cyclicPredOfFirst = cyclicPredOfFirst->cyclicPred();
				}
				G.moveAdjBefore(cyclicPredOfFirst,firstAdj->twin());


				adjEntry cyclicSuccOfSec= secAdj->twin()->cyclicSucc();
				while(cyclicSuccOfSec->twinNode()
						!= firstNode)
				{
					cyclicSuccOfSec = cyclicSuccOfSec->cyclicSucc();
				}
				G.moveAdjAfter(cyclicSuccOfSec,secAdj->twin());

				firstAdj = secAdj;

			}

		}
	}
}






/*******************************************************************************
						recursiveEmbed
********************************************************************************/


// Function computes the cluster planar embedding of a cluster graph
// by recursively reinserting the clusters back into Gcopy and embedding
// their corresponding subgraphs within the planar embedding of Gcopy.


void CconnectClusterPlanarEmbed::recursiveEmbed(ClusterGraph &Ccopy,Graph &Gcopy)
{

	node v;
	// Remove root cluster from stack.
	// Induced subgraph of root cluster corresponds to Gcopy
	cluster root = m_callStack.pop();

	OGDF_ASSERT(Gcopy.representsCombEmbedding())

	hubControl(Gcopy,m_currentHubs);

	while (!m_callStack.empty())
	{

		// Cluster act is reinserted into Gcopy.
		cluster							act				= m_callStack.pop();
		if (m_unsatisfiedCluster[act] == true)
			continue;

		// subgraph is the graph that replaces the wheelGraph of act in Gcopy
		Graph							*subGraph		= m_clusterSubgraph[act];
		// embedding contains the (partial) embedding of all biconnected components
		// that do not have outgoing edges of the cluster act.
		NodeArray<SListPure<adjEntry> >	*embedding		= m_clusterEmbedding[act];
		// For every node of subGraph hubs is true if the node is a hub in subGraph
		NodeArray<bool>					*hubs			= m_clusterSubgraphHubs[act];
		// For every node in subGraph wheelGraphNodes stores the corresponding
		// cluster, if the node is a node of a wheel graph
		NodeArray<cluster>				*wheelGraphNodes= m_clusterSubgraphWheelGraph[act];
		EmbedPQTree						*T				= m_clusterPQContainer[act].m_T;
		EdgeArray<Stack<edge>*>			*outgoingAnker  = m_clusterOutgoingEdgesAnker[act];

		// What else do we have:
		//
		// 1. In m_wheelGraphNodes we have for every node of Gcopy that
		//    is a wheel graph node its corresponding cluster.
		//    Must UPDATE this information after we have replaced the current
		//    wheel graph by subGraph.

		// Make sure that:
		//
		// 1. When inserting new Nodes to Gcopy, that correspond to nodes of subGraph
		//    copy the information on the wheel graphs (stored in wheelGraphNodes)
		// 2. When inserting new Nodes to Gcopy, that correspond to nodes of subGraph
		//    copy the information if it is a hub (stored in hubs)


		//----------------------------------------//
		// Translation tables between the subgraph and
		// its corresponding subgraph in Gcopy
		AdjEntryArray<adjEntry> tableAdjEntrySubGraph2Gcopy(*subGraph);
		NodeArray<node> nodeTableGcopy2SubGraph(Gcopy,0);
		NodeArray<node> nodeTableSubGraph2Gcopy(*subGraph,0);


		//----------------------------------------//
		// Identify all wheelgraph nodes in Gcopy that correspond to act.
		// These nodes have to be removed and replaced by subGraph.

		SList<node> replaceNodes;
		forall_nodes(v,Gcopy)
			if (m_wheelGraphNodes[v] == act)
				replaceNodes.pushBack(v);


		//----------------------------------------//
		// Introduce a new cluster in Gcopy
		cluster newCluster = 0;
		if (m_unsatisfiedCluster[act->parent()] == true)
			newCluster = Ccopy.newCluster(Ccopy.rootCluster());
		else
			newCluster = Ccopy.newCluster(m_clusterTableOrig2Copy[act->parent()]);
		m_clusterTableOrig2Copy[act] = newCluster;
		m_clusterTableCopy2Orig[newCluster] = act;


		//----------------------------------------//
		// Insert for every node of subGraph
		// a new node in Gcopy.
		forall_nodes(v,*subGraph)
		{
			if (v != m_clusterSuperSink[act])
			{
				node newNode = Gcopy.newNode();
				Ccopy.reassignNode(newNode,newCluster);
				nodeTableGcopy2SubGraph[newNode] = v;
				nodeTableSubGraph2Gcopy[v] = newNode;

				// Copy information from subGraph nodes to new Gcopy nodes.
				if ((*wheelGraphNodes)[v])
					m_wheelGraphNodes[newNode] = (*wheelGraphNodes)[v];
				if ((*hubs)[v])
					m_currentHubs[newNode] = (*hubs)[v];
				m_nodeTableCopy2Orig[newNode] = (*m_clusterNodeTableNew2Orig[act])[v];
			}
		}


		//----------------------------------------//
		// Insert the edges between the new nodes
		EdgeArray<bool> visited((*subGraph),false);
		forall_nodes(v,*subGraph)
		{
			node newV = nodeTableSubGraph2Gcopy[v];
			edge e;

			if (v != m_clusterSuperSink[act])
			{
				forall_adj_edges (e,v)
				{
					node w = e->opposite(v);

					if (w != m_clusterSuperSink[act] && !visited[e])
					{
						node newW = nodeTableSubGraph2Gcopy[w];
						edge eNew = Gcopy.newEdge(newV,newW);
						if ((e->adjSource()->theNode() == v &&
							 eNew->adjSource()->theNode() == nodeTableSubGraph2Gcopy[v]) ||
							(e->adjTarget()->theNode() == v &&
							 eNew->adjTarget()->theNode() == nodeTableSubGraph2Gcopy[v]))
						{
							tableAdjEntrySubGraph2Gcopy[e->adjSource()] = eNew->adjSource();
							tableAdjEntrySubGraph2Gcopy[e->adjTarget()] = eNew->adjTarget();
						}
						else
						{
							tableAdjEntrySubGraph2Gcopy[e->adjTarget()] = eNew->adjSource();
							tableAdjEntrySubGraph2Gcopy[e->adjSource()] = eNew->adjTarget();
						}

						// Copy the information of outgoing edges
						// to the new edge.
						m_outgoingEdgesAnker[eNew] = (*outgoingAnker)[e];
						visited[e] = true;
					}
				}
			}
		}//forallnodes
		//edge borderEdge = m_clusterPQContainer[act].m_stEdgeLeaf->userStructKey();



		//----------------------------------------//

		edge startEdge = 0; // first outgoing edge of cluster
							// start embedding here
		SListIterator<node> its;
		for (its = replaceNodes.begin(); its.valid(); its++)
		{
			v = (*its);
			// Assert that v is a node of the wheelgraph belonging
			// to cluster child.
			OGDF_ASSERT(m_wheelGraphNodes[v] == act)

			// Traverse all edges adajcent to v to locate an outgoing edge.
			edge e;
			forall_adj_edges(e,v)
			{
				node w = e->opposite(v);
				if (act != m_wheelGraphNodes[w])
				{
					// Outgoing Edge of wheelgraph detected.
					startEdge = e;
					its = replaceNodes.rbegin(); // break outer for loop
					break;
				}
			}
		}


		// Stack outgoing edges according to embedding

		// Assert that there is an outgoing edge of the cluster
		OGDF_ASSERT(startEdge);
		List<edge> outgoingEdges;
		outgoingEdges.pushBack(startEdge);

		adjEntry adj =  startEdge->adjSource()->theNode() == v ?
						startEdge->adjSource() : startEdge->adjTarget();
		edge currentEdge = 0;
		while (currentEdge != startEdge)
		{
			adjEntry newAdj = adj->cyclicSucc();
			newAdj = newAdj->twin();
			currentEdge = newAdj->theEdge();
			if (act != m_wheelGraphNodes[newAdj->theNode()])
			{
				// Outgoing Edge of wheelgraph detected.
				if (currentEdge != startEdge)
					outgoingEdges.pushBack(currentEdge);
				adj = adj->cyclicSucc();
			}
			else
				adj = newAdj;

		}

		//----------------------------------------//
		// Insert the edges between the new nodes and
		// the existing nodes of Gcopy.

		PlanarLeafKey<IndInfo*>* leftKey = 0;
		PlanarLeafKey<IndInfo*>* rightKey = 0;
		edge firstEdge = 0;
		node t = m_clusterPQContainer[act].m_superSink;
		SListPure<PlanarLeafKey<IndInfo*>*> allOutgoing;

		#ifdef OGDF_DEBUG
		EdgeArray<edge> debugTableOutgoingSubGraph2Gcopy(*subGraph,0);
		#endif

		ListIterator<edge> ite;
		for (ite = outgoingEdges.begin(); ite.valid();)
		{
			edge e = (*ite);
			ListIterator<edge> succ = ite.succ();


			// Assert that stack for anker nodes is not empty
			OGDF_ASSERT(!m_outgoingEdgesAnker[e]->empty())

			node nonWheelNode; // The node of Gcopy that does not
								// correspond to cluster act
			if (act != m_wheelGraphNodes[e->source()])
				nonWheelNode = e->source();
			else {
				OGDF_ASSERT(act != m_wheelGraphNodes[e->target()])
				nonWheelNode = e->target();
			}

			edge subGraphEdge = m_outgoingEdgesAnker[e]->pop();
			node subGraphNode = subGraphEdge->opposite(t);

			#ifdef OGDF_DEBUG
			if (int(ogdf::debugLevel) >= int(dlHeavyChecks)){
			debugTableOutgoingSubGraph2Gcopy[subGraphEdge] = e;}
			#endif

			rightKey = (*m_clusterPQContainer[act].m_edge2Key)[subGraphEdge];
			allOutgoing.pushBack(rightKey);
			if (leftKey)
			{
				SListPure<PlanarLeafKey<IndInfo*>*> pair;
				pair.pushBack(leftKey);
				pair.pushBack(rightKey);
#ifdef OGDF_DEBUG
				bool planar =
#endif
					T->Reduction(pair);
				// Assert that the Reduction did not fail
				OGDF_ASSERT(planar)
				T->PQTree<edge,IndInfo*,bool>::emptyAllPertinentNodes();
			}
			else
				firstEdge = subGraphEdge;

			leftKey = rightKey;

			// Assert that the anker node is a node
			// of the subgraph.
			OGDF_ASSERT(subGraphNode->graphOf() == subGraph)

			// Redirect the edge to the new node.
			// This keeps the embedding of Gcopy.
			if (nonWheelNode == e->source())
			{
				Gcopy.moveTarget(e, nodeTableSubGraph2Gcopy[subGraphNode]);

				if (subGraphEdge->source() == subGraphNode)
				{
					tableAdjEntrySubGraph2Gcopy[subGraphEdge->adjSource()] = e->adjTarget();
					tableAdjEntrySubGraph2Gcopy[subGraphEdge->adjTarget()] = e->adjSource();
				}
				else
				{
					tableAdjEntrySubGraph2Gcopy[subGraphEdge->adjSource()] = e->adjSource();
					tableAdjEntrySubGraph2Gcopy[subGraphEdge->adjTarget()] = e->adjTarget();
				}
			}
			else
			{
				Gcopy.moveSource(e,nodeTableSubGraph2Gcopy[subGraphNode]);

				if (subGraphEdge->target() == subGraphNode)
				{
					tableAdjEntrySubGraph2Gcopy[subGraphEdge->adjSource()] = e->adjTarget();
					tableAdjEntrySubGraph2Gcopy[subGraphEdge->adjTarget()] = e->adjSource();
				}
				else
				{
					tableAdjEntrySubGraph2Gcopy[subGraphEdge->adjSource()] = e->adjSource();
					tableAdjEntrySubGraph2Gcopy[subGraphEdge->adjTarget()] = e->adjTarget();
				}
			}

			ite = succ;
		}


		//----------------------------------------//
		// Compute an embedding of the subgraph

		// Mark all leaves as relevant
#ifdef OGDF_DEBUG
		bool planar =
#endif
			T->Reduction(allOutgoing);
		// Assert that the Reduction did not fail
		OGDF_ASSERT(planar)

		// Stores for every node v the keys corresponding to the incoming edges of v
		NodeArray<SListPure<PlanarLeafKey<IndInfo*>* > >* inLeaves
			= m_clusterPQContainer[act].m_inLeaves;

		// Stores for every node v the keys corresponding to the outgoing edges of v
		/*NodeArray<SListPure<PlanarLeafKey<IndInfo*>* > >* outLeaves
			= m_clusterPQContainer[act].m_outLeaves;*/

		// Stores for every node v the sequence of incoming edges of v according
		// to the embedding
		NodeArray<SListPure<edge> >* frontier
			= m_clusterPQContainer[act].m_frontier;

		// Stores for every node v the nodes corresponding to the
		// opposed sink indicators found in the frontier of v.
		NodeArray<SListPure<node> >* opposed
			= m_clusterPQContainer[act].m_opposed;

		// Stores for every node v the nodes corresponding to the
		// opposed sink indicators found in the frontier of v.
		NodeArray<SListPure<node> >* nonOpposed
			= m_clusterPQContainer[act].m_nonOpposed;

		// Stores for every node the st-number
		NodeArray<int>* numbering = m_clusterPQContainer[act].m_numbering;

		// Stores for every st-Number the corresponding node
		Array<node>* tableNumber2Node = m_clusterPQContainer[act].m_tableNumber2Node;

		Array<bool> toReverse(1,(*numbering)[t],false);

		// Get necessary embedding information
		T->ReplaceRoot((*inLeaves)[t], (*frontier)[t], (*opposed)[t], (*nonOpposed)[t],t);


		//---------------------------------------------------------//
		// Compute a regular embedding of the biconnected component.

		// Reverse adjacency lists if necessary
		edge check = (*frontier)[t].front();

		// Check if the order of edges around t has to be reversed.
		if (firstEdge == check)
			toReverse[(*numbering)[t]] = true;

		int i;
		for (i = (*numbering)[t]; i >= 2; i--)
		{
			if (toReverse[i])
			{
				while (!(*nonOpposed)[(*tableNumber2Node)[i]].empty())
				{
					v = (*nonOpposed)[(*tableNumber2Node)[i]].popFrontRet();
					OGDF_ASSERT(!toReverse[(*numbering)[v]])
					toReverse[(*numbering)[v]] =  true;
				}
				(*frontier)[(*tableNumber2Node)[i]].reverse();
			}
			else
			{
				while (!(*opposed)[(*tableNumber2Node)[i]].empty())
				{
					v = (*opposed)[(*tableNumber2Node)[i]].popFrontRet();
					OGDF_ASSERT(!toReverse[(*numbering)[v]])
					toReverse[(*numbering)[v]] =  true;
				}
			}
			(*nonOpposed)[(*tableNumber2Node)[i]].clear();
			(*opposed)[(*tableNumber2Node)[i]].clear();
		}

		#ifdef OGDF_DEBUG
		if (int(ogdf::debugLevel) >= int(dlHeavyChecks)){
			cout << endl << "New Lists after Reversing " << endl;
			for (i = 1; i <= (*numbering)[t]; i++){v = (*tableNumber2Node)[i];
				cout<<"v = "<<v<<" : "<<" ";SListIterator<edge> it;
				for(it=(*frontier)[v].begin();it.valid();it++)cout<<*it<<" ";
				cout << endl;}}
		#endif

		// Compute the upward embedding

		NodeArray<SListPure<adjEntry> > biCompEmbedding(*subGraph);
		for (i = 1; i <= (*numbering)[t]; i++)
		{
			v = (*tableNumber2Node)[i];
			while (!(*frontier)[v].empty())
			{
				edge e = (*frontier)[v].popFrontRet();
				biCompEmbedding[v].pushBack(
					(e->adjSource()->theNode() == v)? e->adjSource() : e->adjTarget());
			}
		}

		//---------------------------------------------//
		// Compute the entire embedding of the subGraph

		NodeArray<bool> mark(*subGraph,false);
		NodeArray<SListIterator<adjEntry> > adjMarker(*subGraph,0);
		for (i = 1; i <= (*numbering)[t]; i++)
		{
			v = (*tableNumber2Node)[i];
			adjMarker[v] = biCompEmbedding[v].begin();
		}
		v = (*tableNumber2Node)[(*numbering)[t]];
		entireEmbed(*subGraph,biCompEmbedding,adjMarker,mark,v);


		//--------------------------------------------------//
		// Sort the adjacency list of the new nodes in Gcopy
		// using the entire embedding of subGraph

		NodeArray<SListPure<adjEntry> >	embeddingGcopy(Gcopy);

		// Copy Embedding of biconnected Componts with no outging edges first

		forall_nodes(v,(*subGraph))
		{
			SListIterator<adjEntry> it;
			for (it = (*embedding)[v].begin(); it.valid(); it++)
				embeddingGcopy[nodeTableSubGraph2Gcopy[v]].pushBack(
					tableAdjEntrySubGraph2Gcopy[*it]);
		}


		// Copy Embedding of the biconnected componts
		// with outging edges. Don't add the outgoing edges

		for (i = 1; i < (*numbering)[t]; i++)
		{
			v = (*tableNumber2Node)[i];
			SListIterator<adjEntry> it;
			while (!biCompEmbedding[v].empty())
			{
				adjEntry adj = biCompEmbedding[v].popFrontRet();
				(*embedding)[v].pushBack(adj);
				embeddingGcopy[nodeTableSubGraph2Gcopy[v]].pushBack(
					tableAdjEntrySubGraph2Gcopy[adj]);
			}
		}


		forall_nodes(v,*subGraph)
			if (v != t)
				Gcopy.sort(nodeTableSubGraph2Gcopy[v], embeddingGcopy[nodeTableSubGraph2Gcopy[v]]);


		//----------------------------------------//
		// Sort the adjacency list of the new cluster nodes in Gcopy
		// using the adjacency list of t

		SListPure<adjEntry> embeddingClusterList;
		while (!biCompEmbedding[t].empty())
		{
			adjEntry adj = biCompEmbedding[t].popFrontRet();
			(*embedding)[t].pushBack(adj);
			// Choose the twin of adj, since adj is associated with t
			// which is the outside of the cluster.
			embeddingClusterList.pushFront(tableAdjEntrySubGraph2Gcopy[adj->twin()]);
		}

		Ccopy.makeAdjEntries(newCluster,embeddingClusterList.begin());




		//----------------------------------------//
		// Delete the wheelGraph nodes from Gcopy
		while (!replaceNodes.empty())
		{
			v = replaceNodes.popFrontRet();
//			Ccopy.unassignNode(v);
			Gcopy.delNode(v);
		}

		OGDF_ASSERT(Gcopy.representsCombEmbedding())


		if (m_clusterEmbedding[act] != 0)
			delete m_clusterEmbedding[act];
		delete m_clusterSubgraphHubs[act];
		delete m_clusterSubgraphWheelGraph[act];
		delete m_clusterNodeTableNew2Orig[act];
		delete m_clusterOutgoingEdgesAnker[act];

		m_clusterPQContainer[act].Cleanup();

		hubControl(Gcopy,m_currentHubs);

	}

	edge e;
	forall_edges(e,Gcopy)
	{
		if (m_outgoingEdgesAnker[e])
			delete m_outgoingEdgesAnker[e];
	}

	delete m_clusterSubgraphHubs[root];
	delete m_clusterSubgraphWheelGraph[root];
	delete m_clusterOutgoingEdgesAnker[root];


	Ccopy.adjAvailable(true);
}





/*******************************************************************************
						preProcess
********************************************************************************/

//Checks if the algorithm is applicable (input is c-connected and planar) and
//then calls the planarity test method

bool CconnectClusterPlanarEmbed::preProcess(ClusterGraph &Ccopy,Graph &Gcopy)
{
	m_errorCode = none;
	if (!isCConnected(Ccopy))
	{
		ogdf::sprintf(errorCode,124,"Graph is not Ccopy-connected \n");
		m_errorCode = nonCConnected;
		return false;
	}

	if (!isPlanar(Ccopy))
	{
		ogdf::sprintf(errorCode,124,"Graph is not planar\n");
		m_errorCode = nonPlanar;
		return false;
	}

	cluster c;

	SListPure<node> selfLoops;
	makeLoopFree(Gcopy,selfLoops);

	c = Ccopy.rootCluster();

	bool cPlanar = planarityTest(Ccopy,c,Gcopy);


	return cPlanar;
}



/*******************************************************************************
						planarityTest
********************************************************************************/


// Recursive call for testing Planarity of a Cluster

bool CconnectClusterPlanarEmbed::planarityTest(
	ClusterGraph &Ccopy,
	cluster &act,
	Graph &Gcopy)
{
	cluster origOfAct = m_clusterTableCopy2Orig[act];


	// Test children first
	ListConstIterator<cluster> it;
	for (it = act->cBegin(); it.valid();)
	{
		ListConstIterator<cluster> succ = it.succ();
		cluster next = (*it);
		if (!planarityTest(Ccopy,next,Gcopy))
			return false;
		it = succ;
	}


	m_callStack.push(origOfAct);

	// Get induced subgraph of cluster act and test it for planarity

	#ifdef OGDF_DEBUG
	if (int(ogdf::debugLevel) >= int(dlHeavyChecks)){
		cout << endl << endl << "Testing cluster " << origOfAct->index()<<endl;}
	#endif

	List<node> subGraphNodes;
	ListIterator<node> its;
	for (its = act->nBegin(); its.valid(); its++)
		subGraphNodes.pushBack(*its);

	Graph			*subGraph =  OGDF_NEW Graph();
	NodeArray<node> nodeTableOrig2New;
	EdgeArray<edge> edgeTableOrig2New;
	inducedSubGraph(Gcopy, subGraphNodes.begin(), (*subGraph), nodeTableOrig2New, edgeTableOrig2New);
	NodeArray<node> nodeTableNew2Orig((*subGraph),0);

	// Necessary only for root cluster.
	EdgeArray<edge> edgeTableNew2Orig(*subGraph,0);

	if (act != Ccopy.rootCluster())
	{
		m_clusterSubgraph[origOfAct]			= subGraph;
		m_clusterNodeTableNew2Orig[origOfAct]	= new NodeArray<node>((*subGraph),0);
		m_clusterSubgraphHubs[origOfAct]		= OGDF_NEW NodeArray<bool>((*subGraph),0);
		m_clusterSubgraphWheelGraph[origOfAct]	= OGDF_NEW NodeArray<cluster>((*subGraph),0);
		m_clusterOutgoingEdgesAnker[origOfAct]  = OGDF_NEW EdgeArray<Stack<edge>*>((*subGraph),0);
		for (its = act->nBegin(); its.valid(); its++)
		{
			node w = (*its);
			(*m_clusterNodeTableNew2Orig[origOfAct])[nodeTableOrig2New[w]]
				= m_nodeTableCopy2Orig[w];
		}
		edge e;
		forall_edges(e,Gcopy)
		{
			if (edgeTableOrig2New[e] && m_outgoingEdgesAnker[e])
				(*m_clusterOutgoingEdgesAnker[origOfAct])[edgeTableOrig2New[e]]
					= m_outgoingEdgesAnker[e];
		}
	}
	else
	{
		m_clusterSubgraph[origOfAct]			= &Gcopy;
		m_clusterSubgraphHubs[origOfAct]		= OGDF_NEW NodeArray<bool>(Gcopy,0);
		m_clusterSubgraphWheelGraph[origOfAct]	= OGDF_NEW NodeArray<cluster>(Gcopy,0);
		m_clusterOutgoingEdgesAnker[origOfAct]  = OGDF_NEW EdgeArray<Stack<edge>*>(Gcopy,0);
		for (its = act->nBegin(); its.valid(); its++)
		{
			node w = (*its);
			node ttt = nodeTableOrig2New[w];
			nodeTableNew2Orig[ttt] = w;
		}
		edge e;
		forall_edges(e,Gcopy)
		{
			edgeTableNew2Orig[edgeTableOrig2New[e]] = e;
			if (m_outgoingEdgesAnker[e])
				(*m_clusterOutgoingEdgesAnker[origOfAct])[e]
					= m_outgoingEdgesAnker[e];
		}
	}



	// Introduce super sink and add edges corresponding
	// to outgoing edges of the cluster

	node superSink = subGraph->newNode();
	EdgeArray<node> outgoingTable((*subGraph),0);

	for (its = act->nBegin(); its.valid(); its++)
	{
		node w = (*its);
		adjEntry adj = w->firstAdj();
		forall_adj(adj,w)
		{
			edge e = adj->theEdge();
			edge cor = 0;
			if (nodeTableOrig2New[e->source()] == 0)
				// edge is connected to a node outside the cluster
			{
				cor = subGraph->newEdge(nodeTableOrig2New[e->target()],superSink);
				outgoingTable[cor] = e->source();
				if (m_outgoingEdgesAnker[e])
					(*m_clusterOutgoingEdgesAnker[origOfAct])[cor]
						= m_outgoingEdgesAnker[e];
			}
			else if (nodeTableOrig2New[e->target()] == 0) // dito
			{
				cor = subGraph->newEdge(nodeTableOrig2New[e->source()],superSink);
				outgoingTable[cor] = e->target();
				if (m_outgoingEdgesAnker[e])
					(*m_clusterOutgoingEdgesAnker[origOfAct])[cor]
						= m_outgoingEdgesAnker[e];			}

			// else edge connects two nodes of the cluster
		}
	}
	if (superSink->degree() == 0) // root cluster is not connected to outside clusters
	{
		subGraph->delNode(superSink);
		superSink = 0;
	}
	else
		m_clusterSuperSink[origOfAct] = superSink;

	#ifdef OGDF_DEBUG
	if (int(ogdf::debugLevel) >= int(dlHeavyChecks)){
		char filename[124];
		ogdf::sprintf(filename,124,"Ccopy%d.gml",origOfAct->index());
		subGraph->writeGML(filename);
	}
	#endif


	bool cPlanar = preparation((*subGraph),origOfAct,superSink);


	if (cPlanar && act != Ccopy.rootCluster())
	{
		// Remove induced subgraph and the cluster act.
		// Replace it by a wheel graph
		while (!subGraphNodes.empty())
		{
			node w = subGraphNodes.popFrontRet();
			if (m_currentHubs[w])
				(*m_clusterSubgraphHubs[origOfAct])[nodeTableOrig2New[w]]
					= true;
			if (m_wheelGraphNodes[w])
				(*m_clusterSubgraphWheelGraph[origOfAct])[nodeTableOrig2New[w]]
					= m_wheelGraphNodes[w];

//			Ccopy.unassignNode(w);
			Gcopy.delNode(w);
		}

		cluster parent = act->parent();

		if (superSink && m_clusterPQContainer[origOfAct].m_T)
			constructWheelGraph(Ccopy,Gcopy,parent,origOfAct,
								m_clusterPQContainer[origOfAct].m_T,
								outgoingTable,superSink);


		m_clusterTableOrig2Copy[origOfAct] = 0;
		Ccopy.delCluster(act);
	}

	else if (cPlanar && act == Ccopy.rootCluster())
	{

		node w ;
		forall_nodes(w,Gcopy)
		{
			if (m_currentHubs[w])
				(*m_clusterSubgraphHubs[origOfAct])[w] = true;
			if (m_wheelGraphNodes[w])
				(*m_clusterSubgraphWheelGraph[origOfAct])[w] = m_wheelGraphNodes[w];
		}

		forall_nodes(w,*subGraph)
			subGraph->sort(w,(*m_clusterEmbedding[origOfAct])[w]);

		forall_nodes(w,(*subGraph))
		{
			node originalOfw = nodeTableNew2Orig[w];

			SListPure<adjEntry> adjList;

			adjEntry a;
			forall_adj(a,w)
			{
				edge e = edgeTableNew2Orig[a->theEdge()];
				adjEntry adj = (e->adjSource()->theNode() == originalOfw)?
								e->adjSource() : e->adjTarget();
				adjList.pushBack(adj);
			}

			Gcopy.sort(originalOfw,adjList);
		}

		// Test if embedding was determined correctly.
		OGDF_ASSERT(subGraph->representsCombEmbedding())

		edgeTableNew2Orig.init();
		outgoingTable.init();
		nodeTableNew2Orig.init();
		delete m_clusterEmbedding[origOfAct];
		m_clusterEmbedding[origOfAct] = 0;
		delete subGraph;

	}

	else if (!cPlanar && act == Ccopy.rootCluster())
	{
		edgeTableNew2Orig.init();
		outgoingTable.init();
		nodeTableNew2Orig.init();
		delete m_clusterEmbedding[origOfAct];
		m_clusterEmbedding[origOfAct] = 0;
		delete subGraph;
	}

	if (!cPlanar)
	{
		ogdf::sprintf(errorCode,124,"Graph is not planar at cluster %d.\n",act->index());
		m_errorCode = nonCPlanar;
	}//if


	return cPlanar;

}




/*******************************************************************************
						preparation
********************************************************************************/


//
// Prepare planarity test for one cluster
//
bool CconnectClusterPlanarEmbed::preparation(Graph &subGraph,
											 cluster &origCluster,
											 node superSink)
{

	node v;
	edge e;
	int  bcIdSuperSink = -1; // ID of biconnected component that contains superSink
							 // Initialization with -1 necessary for assertion
	bool cPlanar = true;


	NodeArray<node> tableNodesSubGraph2BiComp(subGraph,0);
	EdgeArray<edge> tableEdgesSubGraph2BiComp(subGraph,0);
	NodeArray<bool> mark(subGraph,0);

	EdgeArray<int> componentID(subGraph);

	// Generate datastructure for embedding, even if it is left empty.
	// Embedding either contains
	//		Embedding of the root cluster
	// or
	//		Partial Embedding of the biconnected components not having
	//		outgoing edges.

	NodeArray<SListPure<adjEntry> >
		*entireEmbedding = OGDF_NEW NodeArray<SListPure<adjEntry> >(subGraph);
	m_clusterEmbedding[origCluster] = entireEmbedding;

	// Determine Biconnected Components
	int bcCount = biconnectedComponents(subGraph,componentID);

	// Determine edges per biconnected component
	Array<SList<edge> > blockEdges(0,bcCount-1);
	forall_edges(e,subGraph)
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



	// Perform Planarity Test for every biconnected component

	if (bcCount == 1)
	{
		// Compute st-numbering
		NodeArray<int> numbering(subGraph,0);
		int n;
		if (superSink)
			n = stNumber(subGraph,numbering,0,superSink);
		else
			n = stNumber(subGraph,numbering);
		OGDF_ASSERT_IF(dlConsistencyChecks,testSTnumber(subGraph,numbering,n))

		EdgeArray<edge> tableEdgesBiComp2SubGraph(subGraph,0);
		NodeArray<node> tableNodesBiComp2SubGraph(subGraph,0);
		forall_edges(e,subGraph)
			tableEdgesBiComp2SubGraph[e] = e;
		forall_nodes(v,subGraph)
			tableNodesBiComp2SubGraph[v] = v;

		// Initialize the container class for storing all information
		// if it does not belong to the root cluster.
		if (bcIdSuperSink == 0)
			m_clusterPQContainer[origCluster].init(&subGraph);

		cPlanar = doEmbed(
			&subGraph,
			numbering,
			origCluster,
			superSink,
			subGraph,
			tableEdgesBiComp2SubGraph,
			tableEdgesBiComp2SubGraph,
			tableNodesBiComp2SubGraph);

		// Do not save the embedding of the subgraph. It is not complete.
		if (bcIdSuperSink == -1)
		{
			// The root cluster is embedded.
			// Gather the embeddding of the biconnected graph, if it belongs to
			// the root cluster.
			// The embedding of the subgraph is saved, as it is the root cluster graph.
			forall_nodes(v,subGraph)
			{
				adjEntry a;
				forall_adj(a,v)
					(*entireEmbedding)[v].pushBack(a);
			}
		}

	}
	else
	{
		for (int i = 0; i < bcCount; i++)
		{
			Graph *biCompOfSubGraph = OGDF_NEW Graph();

			SListIterator<node> itn;
			for (itn = blockNodes[i].begin(); itn.valid(); ++ itn)
			{
				v = *itn;
				node w = biCompOfSubGraph->newNode();
				tableNodesSubGraph2BiComp[v] = w;
			}

			NodeArray<node> tableNodesBiComp2SubGraph(*biCompOfSubGraph,0);
			for (itn = blockNodes[i].begin(); itn.valid(); ++ itn)
				tableNodesBiComp2SubGraph[tableNodesSubGraph2BiComp[*itn]] = *itn;

			SListIterator<edge> it;
			for (it = blockEdges[i].begin(); it.valid(); ++it)
			{
				e = *it;
				edge f = biCompOfSubGraph->newEdge(
					tableNodesSubGraph2BiComp[e->source()], tableNodesSubGraph2BiComp[e->target()]);
				tableEdgesSubGraph2BiComp[e] = f;
			}

			EdgeArray<edge> tableEdgesBiComp2SubGraph(*biCompOfSubGraph,0);
			for (it = blockEdges[i].begin(); it.valid(); ++it)
				tableEdgesBiComp2SubGraph[tableEdgesSubGraph2BiComp[*it]] = *it;

			NodeArray<int> numbering(*biCompOfSubGraph,0);
			int n;
			if (bcIdSuperSink == i)
			{
				n = stNumber(*biCompOfSubGraph,numbering,0,tableNodesSubGraph2BiComp[superSink]);
				OGDF_ASSERT_IF(dlConsistencyChecks,testSTnumber(*biCompOfSubGraph,numbering,n))

				// Initialize the container class for storing all information
				m_clusterPQContainer[origCluster].init(&subGraph);

				cPlanar = doEmbed(
					biCompOfSubGraph,
					numbering,
					origCluster,
					tableNodesSubGraph2BiComp[superSink],
					subGraph,
					tableEdgesBiComp2SubGraph,
					tableEdgesSubGraph2BiComp,
					tableNodesBiComp2SubGraph);
			}
			else
			{
				n = stNumber(*biCompOfSubGraph,numbering);
				OGDF_ASSERT_IF(dlConsistencyChecks,testSTnumber(*biCompOfSubGraph,numbering,n));
				cPlanar = doEmbed(
					biCompOfSubGraph,
					numbering,
					origCluster,
					0,
					subGraph,
					tableEdgesBiComp2SubGraph,
					tableEdgesSubGraph2BiComp,
					tableNodesBiComp2SubGraph);
			}

			if (!cPlanar)
			{
				numbering.init();
				tableEdgesBiComp2SubGraph.init();
				tableNodesBiComp2SubGraph.init();
				delete biCompOfSubGraph;
				break;
			}

			if (bcIdSuperSink == -1)
			{
				// The root cluster is embedded.
				// Gather the embedding of the biconnected graph, if it belongs to
				// the root cluster.
				// The embedding of the subgraph is saved, as it is the root cluster graph.
				forall_nodes(v,*biCompOfSubGraph)
				{
					node w = tableNodesBiComp2SubGraph[v];
					adjEntry a;
					forall_adj(a,v)
					{
						edge e = tableEdgesBiComp2SubGraph[a->theEdge()];
						adjEntry adj = (e->adjSource()->theNode() == w)?
										e->adjSource() : e->adjTarget();
						(*entireEmbedding)[w].pushBack(adj);
					}
				}
			}
			else if (bcIdSuperSink != i)
			{
				// A non root cluster is embedded.
				// Gather the embeddings of the biconnected components
				// that do not have outgoing edges of the cluster.
				forall_nodes(v,*biCompOfSubGraph)
				{
					node w = tableNodesBiComp2SubGraph[v];
					adjEntry a;
					forall_adj(a,v)
					{
						edge e = tableEdgesBiComp2SubGraph[a->theEdge()];
						adjEntry adj = (e->adjSource()->theNode() == w)?
										e->adjSource() : e->adjTarget();
						(*entireEmbedding)[w].pushBack(adj);
					}
				}

			}
			numbering.init();
			tableEdgesBiComp2SubGraph.init();
			tableNodesBiComp2SubGraph.init();
			delete biCompOfSubGraph;


		}//for bccount

		// m_clusterEmbedding[origCluster] now contains the (partial) embedding
		// of all biconnected components that do not have outgoing edges
		// of the cluster origCluster.
	}

	return cPlanar;

}// 						preparation




/*******************************************************************************
						doEmbed
********************************************************************************/


// Performs a planarity test on a biconnected component
// of subGraph and embedds it planar.
// numbering contains an st-numbering of the component.
bool CconnectClusterPlanarEmbed::doEmbed(Graph *biconComp,
									NodeArray<int>  &numbering,
									cluster &origCluster,
									node superSink,
									Graph &subGraph,
									EdgeArray<edge> &tableEdgesBiComp2SubGraph,
									EdgeArray<edge> &tableEdgesSubGraph2BiComp,
									NodeArray<node> &tableNodesBiComp2SubGraph)
{
	node v;
	bool cPlanar = true;

	// Definition
	// incoming edge of v: an edge e = (v,w) with number(v) < number(w)


	// Stores for every node v the keys corresponding to the incoming edges of v
	NodeArray<SListPure<PlanarLeafKey<IndInfo*>* > > inLeaves(*biconComp);

	// Stores for every node v the keys corresponding to the outgoing edges of v
	NodeArray<SListPure<PlanarLeafKey<IndInfo*>* > > outLeaves(*biconComp);

	// Stores for every node v the sequence of incoming edges of v according
	// to the embedding
	NodeArray<SListPure<edge> > frontier(*biconComp);

	// Stores for every node v the nodes corresponding to the
	// opposed sink indicators found in the frontier of v.
	NodeArray<SListPure<node> > opposed(*biconComp);

	// Stores for every node v the nodes corresponding to the
	// non opposed sink indicators found in the frontier of v.
	NodeArray<SListPure<node> > nonOpposed(*biconComp);

	// Stores for every st-Number the corresponding node
	Array<node> tableNumber2Node(biconComp->numberOfNodes()+1);

	Array<bool> toReverse(1,biconComp->numberOfNodes()+1,false);

	PlanarLeafKey<IndInfo*>* stEdgeLeaf;

	forall_nodes(v,*biconComp)
	{
		edge e;

		forall_adj_edges(e,v)
		{
			if (numbering[e->opposite(v)] > numbering[v])
			{
				PlanarLeafKey<IndInfo*>* L = OGDF_NEW PlanarLeafKey<IndInfo*>(e);
				inLeaves[v].pushFront(L);
				if (numbering[v] == 1 && numbering[e->opposite(v)])
					stEdgeLeaf = L;
			}
		}
		tableNumber2Node[numbering[v]] = v;
	}

	forall_nodes(v,*biconComp)
	{
		SListIterator<PlanarLeafKey<IndInfo*>* > it;
		for (it = inLeaves[v].begin(); it.valid(); ++it)
		{
			PlanarLeafKey<IndInfo*>* L = *it;
			outLeaves[L->userStructKey()->opposite(v)].pushFront(L);
		}
	}

	EmbedPQTree* T = new EmbedPQTree();

	T->Initialize(inLeaves[tableNumber2Node[1]]);
	int i;
	for (i = 2; i < biconComp->numberOfNodes(); i++)
	{
		if (T->Reduction(outLeaves[tableNumber2Node[i]]))
		{
			T->ReplaceRoot(
				inLeaves[tableNumber2Node[i]],
				frontier[tableNumber2Node[i]],
				opposed[tableNumber2Node[i]],
				nonOpposed[tableNumber2Node[i]],
				tableNumber2Node[i]);
			T->emptyAllPertinentNodes();
		}
		else
		{
			cPlanar = false;
			break;
		}
	}

	if (cPlanar && superSink)
	{
		// The tested component contains the outgoing edges
		// of the cluster.

		// Keep the PQTree to construct a Wheelgraph
		// Replace the edge stored in the keys of T
		// by the original edges.
		// Necessary, since the edges currently in T
		// correspond to a graph that mirrors a biconnected
		// component and thus is deallocated

		// For embedding the graph, we need to keep the
		// PQTree as well.

		SListIterator<PlanarLeafKey<IndInfo*>* >  it;
		//int n = biconComp->numberOfNodes();

		// Replace the edge stored in the keys of T
		// by the original edges.


		//--------------------------------------//
		// All information that we keep is dependend on subGraph.
		// Translate the information back from biconComp to subGraph.


		m_clusterPQContainer[origCluster].m_superSink
			= tableNodesBiComp2SubGraph[superSink];

		forall_nodes(v,*biconComp)
		{
			// Replace the edge stored in the every key used for constructing T
			// by the original edges.
			// This implicity replaces the keys at the leaves and at inLeaves.


			node orig = tableNodesBiComp2SubGraph[v];

			// Assert that m_outLeaves is empty
			OGDF_ASSERT((*m_clusterPQContainer[origCluster].m_outLeaves)[orig].empty())
			for (it = outLeaves[v].begin(); it.valid(); ++it)
			{
				PlanarLeafKey<IndInfo*>* key = *it;
				key->m_userStructKey = tableEdgesBiComp2SubGraph[key->m_userStructKey];
				(*m_clusterPQContainer[origCluster].m_edge2Key)[key->m_userStructKey] = key;
				(*m_clusterPQContainer[origCluster].m_outLeaves)[orig].pushBack(key);
			}

			// Assert that m_inLeaves is empty
			OGDF_ASSERT((*m_clusterPQContainer[origCluster].m_inLeaves)[orig].empty())
			for (it = inLeaves[v].begin(); it.valid(); ++it)
			{
				PlanarLeafKey<IndInfo*>* key = *it;
				(*m_clusterPQContainer[origCluster].m_inLeaves)[orig].pushBack(key);
			}

			// Replace the nodes stored in the lists opposed and nonOpposed
			// by the original nodes

			// Assert that m_opposed and m_nonOpposed are empty
			OGDF_ASSERT((*m_clusterPQContainer[origCluster].m_opposed)[orig].empty())
			OGDF_ASSERT((*m_clusterPQContainer[origCluster].m_nonOpposed)[orig].empty())
			SListIterator<node> itn;
			for (itn = nonOpposed[v].begin(); itn.valid(); itn++)
			{
				node w = tableNodesBiComp2SubGraph[(*itn)];
				(*m_clusterPQContainer[origCluster].m_nonOpposed)[orig].pushBack(w);
			}
			for (itn = opposed[v].begin(); itn.valid(); itn++)
			{
				node w = tableNodesBiComp2SubGraph[(*itn)];
				(*m_clusterPQContainer[origCluster].m_opposed)[orig].pushBack(w);
			}

			(*m_clusterPQContainer[origCluster].m_numbering)[orig] = numbering[v];
			(*m_clusterPQContainer[origCluster].m_tableNumber2Node)[numbering[v]] = orig;


			// Replace the edges stored in frontier
			// by the original edges of subgraph.

			OGDF_ASSERT((*m_clusterPQContainer[origCluster].m_frontier)[orig].empty())
			SListIterator<edge> ite;
			for (ite = frontier[v].begin(); ite.valid(); ite++)
			{
				edge e = tableEdgesBiComp2SubGraph[(*ite)];
				(*m_clusterPQContainer[origCluster].m_frontier)[orig].pushBack(e);
			}


		}
		m_clusterPQContainer[origCluster].m_T = T;
		m_clusterPQContainer[origCluster].m_stEdgeLeaf = stEdgeLeaf;
		SListPure<PQBasicKey<edge,IndInfo*,bool>*> leafKeys;
		T->getFront(T->root(),leafKeys);
		SListIterator<PQBasicKey<edge,IndInfo*,bool>* >  itk;
		for (itk = leafKeys.begin(); itk.valid(); itk++)
		{
			if ((*itk)->nodePointer()->status() == PQNodeRoot::INDICATOR)
			{
				node ofInd = (*itk)->nodePointer()->getNodeInfo()->userStructInfo()->getAssociatedNode();
				(*itk)->nodePointer()->getNodeInfo()->userStructInfo()->resetAssociatedNode(tableNodesBiComp2SubGraph[ofInd]);
			}
		}
	}
	else if (cPlanar)
	{
		// The tested component does not contain outgoing edges
		// of the cluster.
		// Compute a regular embedding of the biconnected component.
		int i = biconComp->numberOfNodes();
		if (T->Reduction(outLeaves[tableNumber2Node[i]]))
		{
			T->ReplaceRoot(
				inLeaves[tableNumber2Node[i]],
				frontier[tableNumber2Node[i]],
				opposed[tableNumber2Node[i]],
				nonOpposed[tableNumber2Node[i]],
				tableNumber2Node[i]);
		}
		delete T;
	}

	// Cleanup
	if (!origCluster || !superSink || !cPlanar)
									// Do not cleanup information of component
									// with outgoing edges.
	{
		forall_nodes(v,*biconComp)
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
	}
	if (!cPlanar)
		delete T;


	if (cPlanar && (!origCluster || !superSink))
	{
		// The tested component does not contain outgoing edges
		// of the cluster.
		// Compute a regular embedding of the biconnected component.

		// Reverse adjacency lists if necessary
		// This gives an upward embedding
		for (i = biconComp->numberOfNodes(); i >= 2; i--)
		{
			if (toReverse[i])
			{
				while (!nonOpposed[tableNumber2Node[i]].empty())
				{
					v = nonOpposed[tableNumber2Node[i]].popFrontRet();
					OGDF_ASSERT(!toReverse[numbering[v]])
					toReverse[numbering[v]] =  true;
				}
				frontier[tableNumber2Node[i]].reverse();
			}
			else
			{
				while (!opposed[tableNumber2Node[i]].empty())
				{
					v = opposed[tableNumber2Node[i]].popFrontRet();
					OGDF_ASSERT(!toReverse[numbering[v]])
					toReverse[numbering[v]] =  true;
				}
			}
			nonOpposed[tableNumber2Node[i]].clear();
			opposed[tableNumber2Node[i]].clear();
		}

		// Compute the entire embedding
		NodeArray<SListPure<adjEntry> > entireEmbedding(*biconComp);
		forall_nodes(v,*biconComp)
		{
			while (!frontier[v].empty())
			{
				edge e = frontier[v].popFrontRet();
				entireEmbedding[v].pushBack(
					(e->adjSource()->theNode() == v)? e->adjSource() : e->adjTarget());
			}
		}


		NodeArray<bool> mark(*biconComp,false);
		NodeArray<SListIterator<adjEntry> > adjMarker(*biconComp,0);
		forall_nodes(v,*biconComp)
			adjMarker[v] = entireEmbedding[v].begin();
		v = tableNumber2Node[biconComp->numberOfNodes()];
		entireEmbed(*biconComp,entireEmbedding,adjMarker,mark,v);


		forall_nodes(v,*biconComp)
			biconComp->sort(v,entireEmbedding[v]);

		// Test if embedding was determined correctly.
		OGDF_ASSERT(biconComp->representsCombEmbedding())

	}

	return cPlanar;


}//						doEmbed





/*******************************************************************************
						entireEmbed
********************************************************************************/


// Used by doEmbed. Computes an entire embedding from an
// upward embedding.
void CconnectClusterPlanarEmbed::entireEmbed(
	Graph &biconComp,
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
			entireEmbed(biconComp,entireEmbedding,adjMarker,mark,w);
	}
}






/*******************************************************************************
					prepareParallelEdges
********************************************************************************/


void CconnectClusterPlanarEmbed::prepareParallelEdges(Graph &G)
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




/*******************************************************************************
					constructWheelGraph
********************************************************************************/


void CconnectClusterPlanarEmbed::constructWheelGraph(ClusterGraph &Ccopy,
												Graph &Gcopy,
												cluster &parent,
												cluster &origOfAct,
												EmbedPQTree* T,
												EdgeArray<node> &outgoingTable,
												node superSink)
{

	OGDF_ASSERT(Ccopy.consistencyCheck());
	PQNode<edge,IndInfo*,bool>* root = T->root();
	PQNode<edge,IndInfo*,bool>*  checkNode = 0;

	Queue<PQNode<edge,IndInfo*,bool>*> treeNodes;
	treeNodes.append(root);

	node correspond = Gcopy.newNode(); // Corresponds to the root node.
		 						       // root node is either a leaf or a P-node
	m_nodeTableCopy2Orig[correspond] = 0; // Node does not correspond to a node
										 // in the original graph
	m_wheelGraphNodes[correspond] = origOfAct;
	Ccopy.reassignNode(correspond,parent);

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
				newNode = Gcopy.newNode();
				m_nodeTableCopy2Orig[newNode] = 0;
				m_wheelGraphNodes[newNode] = origOfAct;
				Ccopy.reassignNode(newNode,parent);
				graphNodes.append(newNode);
				Gcopy.newEdge(correspond,newNode);
			}
			else
			{
				// insert Edge to the outside
				PQLeaf<edge,IndInfo*,bool>* leaf =
					(PQLeaf<edge,IndInfo*,bool>*) firstSon;
				edge f = leaf->getKey()->m_userStructKey;
				//node x = outgoingTable[f];
				edge newEdge = Gcopy.newEdge(correspond,outgoingTable[f]);


				if ((*m_clusterOutgoingEdgesAnker[origOfAct])[f])
				{
					m_outgoingEdgesAnker[newEdge]
						= (*m_clusterOutgoingEdgesAnker[origOfAct])[f];
				}
				else
					m_outgoingEdgesAnker[newEdge] = OGDF_NEW Stack<edge>;
				m_outgoingEdgesAnker[newEdge]->push(f);
			}

			nextSon = firstSon->getNextSib(oldSib);
			oldSib = firstSon;
			pre = next;
			while (nextSon && nextSon != firstSon)
			{
				if (nextSon->type() != PQNodeRoot::leaf)
				{
					treeNodes.append(nextSon);
					newNode = Gcopy.newNode();  // new node corresponding to anchor
												// or cutnode
					m_nodeTableCopy2Orig[newNode] = 0;
					m_wheelGraphNodes[newNode] = origOfAct;
					Ccopy.reassignNode(newNode,parent);
					graphNodes.append(newNode);
					Gcopy.newEdge(correspond,newNode);
				}
				else
				{
					// insert Edge to the outside
					PQLeaf<edge,IndInfo*,bool>* leaf =
						(PQLeaf<edge,IndInfo*,bool>*) nextSon;
					edge f = leaf->getKey()->m_userStructKey;
					//node x = outgoingTable[f];
					edge newEdge = Gcopy.newEdge(correspond,outgoingTable[f]);

					if ((*m_clusterOutgoingEdgesAnker[origOfAct])[f])
					{
						m_outgoingEdgesAnker[newEdge]
							= (*m_clusterOutgoingEdgesAnker[origOfAct])[f];
					}
					else
						m_outgoingEdgesAnker[newEdge] = OGDF_NEW Stack<edge>;
					m_outgoingEdgesAnker[newEdge]->push(f);
				}
				holdSib = nextSon->getNextSib(oldSib);
				oldSib = nextSon;
				nextSon = holdSib;
			}

		}
 		else if (checkNode->type() == PQNodeRoot::QNode)
		{

			// correspond is the achor of a hub
			OGDF_ASSERT(T->scanLeftEndmost(checkNode))
			firstSon = T->scanLeftEndmost(checkNode);

			hub = Gcopy.newNode();
			m_nodeTableCopy2Orig[hub] = 0;
			m_currentHubs[hub] = true;
			m_wheelGraphNodes[hub] = origOfAct;
			Ccopy.reassignNode(hub,parent);

			Gcopy.newEdge(hub,correspond); // link achor and hub
			next = Gcopy.newNode();   // for first son
			m_nodeTableCopy2Orig[next] = 0;
			m_wheelGraphNodes[next] = origOfAct;
			Ccopy.reassignNode(next,parent);
			Gcopy.newEdge(hub,next);
			Gcopy.newEdge(correspond,next);

			if (firstSon->type() != PQNodeRoot::leaf)
			{
				treeNodes.append(firstSon);
				newNode = Gcopy.newNode();
				m_nodeTableCopy2Orig[newNode] = 0;
				m_wheelGraphNodes[newNode] = origOfAct;
				Ccopy.reassignNode(newNode,parent);
				graphNodes.append(newNode);
				Gcopy.newEdge(next,newNode);
			}
			else
			{
				// insert Edge to the outside
				PQLeaf<edge,IndInfo*,bool>* leaf =
					(PQLeaf<edge,IndInfo*,bool>*) firstSon;
				edge f = leaf->getKey()->m_userStructKey;
				//node x = outgoingTable[f];
				edge newEdge = Gcopy.newEdge(next,outgoingTable[f]);

				if ((*m_clusterOutgoingEdgesAnker[origOfAct])[f])
				{
					m_outgoingEdgesAnker[newEdge]
						= (*m_clusterOutgoingEdgesAnker[origOfAct])[f];
				}
				else
					m_outgoingEdgesAnker[newEdge] = OGDF_NEW Stack<edge>;
				m_outgoingEdgesAnker[newEdge]->push(f);
			}

			nextSon = T->scanNextSib(firstSon,oldSib);
			oldSib = firstSon;
			pre = next;
			while (nextSon)
			{
				next = Gcopy.newNode();
				m_nodeTableCopy2Orig[next] = 0;
				m_wheelGraphNodes[next] = origOfAct;
				Ccopy.reassignNode(next,parent);
				Gcopy.newEdge(hub,next);
				Gcopy.newEdge(pre,next);
				if (nextSon->type() != PQNodeRoot::leaf)
				{
					treeNodes.append(nextSon);
					newNode = Gcopy.newNode();  // new node corresponding to anchor
												// or cutnode
					m_nodeTableCopy2Orig[newNode] = 0;
					m_wheelGraphNodes[newNode] = origOfAct;
					Ccopy.reassignNode(newNode,parent);
					graphNodes.append(newNode);

					Gcopy.newEdge(next,newNode);
				}
				else
				{
					// insert Edge to the outside
					PQLeaf<edge,IndInfo*,bool>* leaf =
						(PQLeaf<edge,IndInfo*,bool>*) nextSon;
					edge f = leaf->getKey()->m_userStructKey;
					//node x = outgoingTable[f];
					edge newEdge = Gcopy.newEdge(next,outgoingTable[f]);

					if ((*m_clusterOutgoingEdgesAnker[origOfAct])[f])
					{
						m_outgoingEdgesAnker[newEdge]
							= (*m_clusterOutgoingEdgesAnker[origOfAct])[f];
					}
					else
						m_outgoingEdgesAnker[newEdge] = OGDF_NEW Stack<edge>;
					m_outgoingEdgesAnker[newEdge]->push(f);
				}
				holdSib = T->scanNextSib(nextSon,oldSib);
				oldSib = nextSon;
				nextSon = holdSib;
				pre = next;

			}
			Gcopy.newEdge(next,correspond);
		}
	}

	OGDF_ASSERT(Ccopy.consistencyCheck());
}//					constructWheelGraph


} // end namespace ogdf

