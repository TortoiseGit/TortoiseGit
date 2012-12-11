/*
 * $Revision: 2599 $
 *
 * last checkin:
 *   $Author: chimani $
 *   $Date: 2012-07-15 22:39:24 +0200 (So, 15. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of extended graph algorithms
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


#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/basic/BinaryHeap2.h>
#include <ogdf/cluster/ClusterArray.h>
#include <float.h>


namespace ogdf {

//maybe shift this to basic.h, used in cplanaredgeinserter, toos
class OrigNodePair
{
public:
	node m_src, m_tgt;
};

//---------------------------------------------------------
// Methods for clustered graphs. By S.Leipert and K. Klein
//---------------------------------------------------------

// Recursive call for testing C-connectivity.
bool cConnectTest(ClusterGraph &C,cluster &act,NodeArray<bool> &mark,Graph &G)
{

	ListConstIterator<cluster> it;
	for (it = act->cBegin(); it.valid();)
	{
		ListConstIterator<cluster> succ = it.succ();
		cluster next = (*it);
		if (!cConnectTest(C,next,mark,G))
			return false;
		it = succ;
	}

	ListIterator<node> its;
	for (its = act->nBegin(); its.valid(); its++)
		mark[(*its)] = true;

	node v = (*(act->nBegin()));
	SListPure<node> bfs;
	bfs.pushBack(v);
	mark[v] = false;

	while (!bfs.empty())
	{
		v = bfs.popFrontRet();
		edge e;
		forall_adj_edges(e,v)
		{
			if (mark[e->source()])
			{
				mark[e->source()] = false;
				bfs.pushBack(e->source());
			}
			else if (mark[e->target()])
			{
				mark[e->target()] = false;
				bfs.pushBack(e->target());
			}
		}
	}

	for (its = act->nBegin(); its.valid(); its++)
		if (mark[(*its)])
			return false;

	SListPure<node> collaps;
	for (its = act->nBegin(); its.valid(); its++)
		collaps.pushBack(*its);


	C.collaps(collaps,G);

	if (act != C.rootCluster())
		C.delCluster(act);
	return true;

}


// true <=> C is C-connected
bool isCConnected(const ClusterGraph &C)
{
	if(C.getGraph().empty())
		return true;

	Graph G;
	ClusterGraph Cp(C,G);


	cluster root = Cp.rootCluster();
	SListPure<node>   compNodes;
	NodeArray<bool> mark(G,false);
	return cConnectTest(Cp,root,mark,G);
}


//in ClusterGraph??
//is not yet recursive!!!
node collapseCluster(ClusterGraph& CG, cluster c, Graph& G)
{
	OGDF_ASSERT(c->cCount() == 0)

	ListIterator<node> its;
	SListPure<node> collaps;

	//we should check here if not empty
	node robinson = (*(c->nBegin()));

	for (its = c->nBegin(); its.valid(); its++)
		collaps.pushBack(*its);

	CG.collaps(collaps, G);

	if (c != CG.rootCluster())
		CG.delCluster(c);

	return robinson;
}

//returns a node of cluster c used as endpoint for inserted
//connection edges
//precondition: not empty
node getRepresentationNode(cluster c)
{
	OGDF_ASSERT(c->nCount() + c->cCount() > 0)
	//improvement potential: use specific nodes that optimize connection
	//process
	if (c->nCount() > 0)
		return (*(c->nBegin()));

	return getRepresentationNode((*(c->cBegin())));
}


void recursiveConnect(
	ClusterGraph& CG,
	cluster act,
	NodeArray<cluster>& origCluster,	//on CG, cluster rep.
										//by collapsed node
	ClusterArray<cluster>& oCcluster, //cluster in orig for CG cluster
	NodeArray<node>& origNode,
	Graph& G,
	List<OrigNodePair>& newEdges)
{
	//for non-cc clusters, add edges to make them connected
	//recursively search for connection nodes (simple version:
	//use first node/first node of first child (recursive)

	ListConstIterator<cluster> it;
	for (it = act->cBegin(); it.valid();)
	{
		ListConstIterator<cluster> succ = it.succ();
		cluster next = (*it);
		recursiveConnect(CG, next, origCluster, oCcluster, origNode, G, newEdges);

		it = succ;
	}


	//We construct a copy of the current cluster
	OGDF_ASSERT(act->cCount() == 0)
	Graph cG;
	NodeArray<node> vOrig(cG, 0);
	NodeArray<node> vCopy(CG, 0); //larger than necessary, hashingarray(index)?

	ListIterator<node> its;
	for (its = act->nBegin(); its.valid(); its++)
	{
		node vo = (*its);
		node v = cG.newNode();
		vOrig[v] = vo;
		vCopy[vo] = v;

	}//for

	NodeArray<bool> processed(CG, false);
	for (its = act->nBegin(); its.valid(); its++)
	{
		node vo = (*its);
		processed[vo] = true;
		edge e;
		forall_adj_edges(e, vo)
		{
			//if node in cluster and edge not yet inserted
			if (vCopy[e->opposite(vo)] && !processed[e->opposite(vo)])
				//we don't care about the edge direction
				cG.newEdge(vCopy[vo], vCopy[e->opposite(vo)]);
		}//foralladjedges
	}//for

	//connect the copy (should use improved version of makeconnected later)
	List<edge> added;
	makeConnected(cG, added);

	//now translate connection into clustergraph
	while (!added.empty())
	{
		edge eNew = added.popFrontRet();
		//allow collapse by making cluster connected
		G.newEdge(vOrig[eNew->source()], vOrig[eNew->target()]);

		//maybe some of the nodes represent clusters, we have to find
		//a representative
		node v1 = vOrig[eNew->source()];
		node v2 = vOrig[eNew->target()];

		//save original information
		OrigNodePair np;
		//already collapsed node?
		np.m_src = (origCluster[v1] ? getRepresentationNode(origCluster[v1]) : origNode[v1]);
		np.m_tgt = (origCluster[v2] ? getRepresentationNode(origCluster[v2]) : origNode[v2]);
		newEdges.pushBack(np);

	}//if cluster was not connected

	//collapse cluster in copy and save information on the
	//collapsed cluster in the nodes
	cluster cOrig = oCcluster[act];
	node vNew = collapseCluster(CG, act, G);
	//update info if cluster collapsed
	origCluster[vNew] = cOrig;
}//recursiveConnect


//planarity  checking version

//we should care about the representation nodes, they should be
//good nodes, too

//search for a node without attribute badnode (cluster: node
//with connection over cluster boundary, search for min degree
static void dfsMakeCConnected(node v,
	node source, //the node vMinDeg will be connected to
	NodeArray<bool> &visited,
	const NodeArray<bool> &badNode,
	Graph& fullGraph, //test graph
	NodeArray<node> &fullGraphCopy,
	bool keepsPlanarity,   //does current best keep planarity?
	node &vMinDeg) //current best
{
	visited[v] = true;

	edge e;
	forall_adj_edges(e,v)
	{
		node w = e->opposite(v);
		if (!visited[w])
		{
			//hier grad als erste unterscheidung: kleiner Grad besser
			//dann badnode, dann planaritaet
			bool better = (badNode[fullGraphCopy[vMinDeg]] ||
				!badNode[fullGraphCopy[w]]);
			bool kPlanar = false;

			//***************************************
			//irgendeine Reihenfolge, um nicht jede moegliche
			//Verbindung auf Planaritaet zu testen??
			if (source)
			{
				edge eP = fullGraph.newEdge(
					fullGraphCopy[source],
					fullGraphCopy[vMinDeg]);

				if (isPlanar(fullGraph) == true)
					kPlanar =  true;

				fullGraph.delEdge(eP); //only keep if finally chosen

			}//if source

			//****************************************
			better = ((better || kPlanar) && !keepsPlanarity) || (kPlanar && better);

			if (better)// && (w->degree() < vMinDeg->degree()))
				vMinDeg = w;


			dfsMakeCConnected(w, source, visited, badNode,
				fullGraph, fullGraphCopy, keepsPlanarity, vMinDeg);
		}
	}
}//dfsMakeCConnected


//connect cluster represented by graph G,  observe fullGraph planarity
//in nodepair selection, try to avoid badnodes
void cMakeConnected(
	Graph &G, //cluster subgraph
	Graph &fullGraphCopy, //copy of full graph
	NodeArray<node> &fullGraphNode,	// holds node in fullgraphCopy
									//corresponding to node in G cluster
	NodeArray<bool> &badNode, //some attribute
	List<edge> &added)
{
	added.clear();
	NodeArray<bool> visited(G,false);

	node vMinDeg, pred = 0, v;

	bool keepsPlanarity = false;

	//hier muss irgendwo bewertet werden, ob Kanten die Planaritaet
	//erhalten, aber man kann nicht fuer jeden Knoten einen Test machen
	forall_nodes(v,G) {
		if (!visited[v]) {
			vMinDeg = v;
			dfsMakeCConnected(v, pred, visited, badNode,
				fullGraphCopy, fullGraphNode, keepsPlanarity, vMinDeg);
			if (pred)
			{
				added.pushBack(G.newEdge(pred,vMinDeg));
				//write current status into fullGraphCopy
				fullGraphCopy.newEdge(fullGraphNode[pred], fullGraphNode[vMinDeg]);
			}
			pred = vMinDeg;
		}
	}
}//cMakeConnected


void recursiveCConnect(
	ClusterGraph& CG,
	cluster act,
	NodeArray<cluster>& origCluster,	//on CG, cluster rep.
										//by collapsed node
	ClusterArray<cluster>& oCcluster,	//cluster in orig for CG cluster
	NodeArray<node>& origNode,
	Graph& G,
	Graph& fullCopy,	//copy of graph G be checked for planarity
						//holds corresponding nodes in fullCopy for v of G
	NodeArray<node>& copyNode,
	NodeArray<bool>& badNode, //should not we used for connecting
	List<OrigNodePair>& newEdges)
{
	//for non-cc clusters, add edges to make them connected
	//recursively search for connection nodes (simple version:
	//use first node/first node of first child (recursive)

	ListConstIterator<cluster> it;
	for (it = act->cBegin(); it.valid();)
	{
		ListConstIterator<cluster> succ = it.succ();
		cluster next = (*it);
		recursiveCConnect(CG, next, origCluster, oCcluster, origNode, G, fullCopy,
			copyNode, badNode, newEdges);

		it = succ;
	}

	//******************************************
	//We construct a graph copy of the current cluster subgraph
	OGDF_ASSERT(act->cCount() == 0)
	Graph cG;
	NodeArray<node> vOrig(cG, 0);
	NodeArray<node> vCopy(CG, 0); //larger than necessary, hashingarray(index)?

	NodeArray<node> vFullCopy(cG, 0);//node in planarity tested full copy

	ListIterator<node> its;
	for (its = act->nBegin(); its.valid(); its++)
	{
		node vo = (*its);
		node v = cG.newNode();
		vOrig[v] = vo;
		vCopy[vo] = v;

		vFullCopy[v] = copyNode[vo]; //save the corresponding node in working copy
									 //to check planarity after edge insertion

	}//for

	NodeArray<bool> processed(CG, false);
	for (its = act->nBegin(); its.valid(); its++)
	{
		node vo = (*its);
		processed[vo] = true;
		edge e;
		forall_adj_edges(e, vo)
		{
			//if node in cluster and edge not yet inserted
			if (vCopy[e->opposite(vo)] && !processed[e->opposite(vo)])
				//we don't care about the edge direction
				cG.newEdge(vCopy[vo], vCopy[e->opposite(vo)]);
		}//foralladjedges
	}//for

	//connect the copy (should use improved version of makeconnected later)
	List<edge> added;
	//makeConnected(cG, added);
	cMakeConnected(cG, fullCopy, vFullCopy, badNode, added);

	//now translate connection into clustergraph
	while (!added.empty())
	{
		edge eNew = added.popFrontRet();
		//allow collapse by making cluster connected
		G.newEdge(vOrig[eNew->source()], vOrig[eNew->target()]);

		//maybe some of the nodes represent clusters, we have to find
		//a representative
		node v1 = vOrig[eNew->source()];
		node v2 = vOrig[eNew->target()];

		//save original information
		OrigNodePair np;
		//already collapsed node?
		np.m_src = (origCluster[v1] ? getRepresentationNode(origCluster[v1]) : origNode[v1]);
		np.m_tgt = (origCluster[v2] ? getRepresentationNode(origCluster[v2]) : origNode[v2]);
		newEdges.pushBack(np);

	}//if cluster was not connected

	//collapse cluster in copy and save information on the
	//collapsed cluster in the nodes
	cluster cOrig = oCcluster[act];
	node vNew = collapseCluster(CG, act, G);
	//update info if cluster collapsed
	origCluster[vNew] = cOrig;
}//recursiveCConnect

//*****************************************************************
/*



*/


//*****************************************************************


//second version for advanced connectivity
void cconnect(
	ClusterGraph& CG,
	NodeArray<cluster>& origCluster, //on CG, cluster rep. by collapsed node
	ClusterArray<cluster>& oCcluster, //cluster in orig for CG cluster
	NodeArray<node>& origNode,
	Graph& G,
	List<OrigNodePair>& newEdges)
{
	//We work with a copy of the graph that is checked for planarity
	//for inserted cconnectivity edges
	Graph fullCopy;
	NodeArray<node> fullCopyNode(G);
	node v;
	//check for all nodes if they have an edge adjacent crossing
	//the cluster boundary, I assume this is a bad candidate for
	//cconnection edges
	NodeArray<bool> badNode(fullCopy, false);

	forall_nodes(v, G)
	{
		node w  = fullCopy.newNode();
		fullCopyNode[v] = w;
		edge e2;
		cluster c = CG.clusterOf(v);
		forall_adj_edges(e2, v)
		{
			node u = e2->target();
			//badnode is the case if lca(v,u) is != c(v)
			cluster lca = CG.commonCluster(v, u);
			if (c != lca)
			{
				badNode[w] = true;
				break;
			}
		}

	}//forallnodes

	recursiveCConnect(
		CG,					//check cluster graph copy
		CG.rootCluster(),	//the whole graph
		origCluster,		//original cluster for collapse nodes
		oCcluster,			//cluster in copy
		origNode,			//original nodes
		G,					//original graph
		fullCopy,			//planarity checking copy of original graph G
		fullCopyNode,		//corresponding nodes in fullCopy
		badNode,			//fullCopy node attribute
		newEdges);			//inserted edges

}//cconnect


//make a cluster graph cconnected by adding edges
//simple: just make the cluster subgraph connected
//not simple: check nodes on cluster adjacent edges
//and new edges on planarity
void makeCConnected(ClusterGraph& C, Graph& GG, List<edge>& addedEdges, bool simple)
{
	//work on copy ( is updated )
	Graph G;
	NodeArray<node> copyNode(C, 0);
	ClusterArray<cluster> copyCluster(C, 0);

	ClusterGraph cCopy(C, G, copyCluster, copyNode);

	NodeArray<node> origNode(cCopy, 0);
	node v;

	forall_nodes(v, GG)
		origNode[copyNode[v]] = v;

	//holds information on collapsed clusters (points to original clusters)
	NodeArray<cluster> origCluster(cCopy, 0);
	//holds copy to original cluster info
	ClusterArray<cluster> oCcluster(cCopy, 0);
	cluster c;
	forall_clusters(c, C)
		oCcluster[copyCluster[c]] = c;

	List<OrigNodePair> newEdges; //edges to be inserted
	//recursively check clusters for connectivity and collapse
	//save cluster info on collapsed nodes

	if (!simple)
		cconnect(cCopy, origCluster, oCcluster, origNode, G, newEdges);
	else
		recursiveConnect(cCopy, cCopy.rootCluster(), origCluster, oCcluster,
					 origNode, G, newEdges);

	ListConstIterator<OrigNodePair> it = newEdges.begin();
	while (it.valid())
	{
		OrigNodePair np = (*it);
		edge nedge = GG.newEdge(np.m_src, np.m_tgt);
		//cout<<"Adding edge: "<<np.m_src<<"-"<<np.m_tgt<<"\n"<<flush;
		addedEdges.pushBack(nedge);
		it++;
	}

	//cout << "added " << addedEdges.size() <<"edges\n";
}//makeCConnected




} // end namespace ogdf
