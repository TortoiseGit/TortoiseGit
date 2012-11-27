/*
 * $Revision: 2573 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-10 18:48:33 +0200 (Di, 10. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implements the class ClusterGraph, providing
 * extra functionality for clustered graphs.
 * A clustered graph C=(G,T) consists of an undirected graph G
 * and a rooted tree T in which the leaves of T correspond
 * to the vertices of G=(V,E).
 *
 * \author Sebastian Leipert, Karsten Klein
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


#include <ogdf/cluster/ClusterGraph.h>
#include <ogdf/cluster/ClusterArray.h>
#include <ogdf/cluster/ClusterGraphObserver.h>

#include <limits.h>
#include <ctype.h>
#include <string.h>

#include <ogdf/basic/String.h>
#include <ogdf/basic/AdjEntryArray.h>

#include <ogdf/fileformats/GmlParser.h>


namespace ogdf {

#define MIN_CLUSTER_TABLE_SIZE (1 << 4)

//---------------------------------------------------------
//node search in cluster hierarchy
//---------------------------------------------------------
void ClusterElement::getClusterInducedNodes(List<node> &clusterNodes) {

	ListConstIterator<node> nit;
	for (nit=m_entries.begin(); nit.valid(); ++nit) {
		clusterNodes.pushBack(*nit);
	}
	ListConstIterator<cluster> cit;
	for (cit=m_children.begin(); cit.valid(); ++cit) {
		(*cit)->getClusterInducedNodes(clusterNodes);
	}
}

void ClusterElement::getClusterNodes(List<node> &clusterNodes) {

	clusterNodes.clear();
	getClusterInducedNodes(clusterNodes);
}

void ClusterElement::getClusterInducedNodes(NodeArray<bool> &clusterNode, int& num) {

	ListConstIterator<node> nit;
	for (nit=m_entries.begin(); nit.valid(); ++nit) {
		clusterNode[*nit] = true;
		num++;
	}
	ListConstIterator<cluster> cit;
	for (cit=m_children.begin(); cit.valid(); ++cit) {
		(*cit)->getClusterInducedNodes(clusterNode, num);
	}
}

int ClusterElement::getClusterNodes(NodeArray<bool> &clusterNode)
{
	int num = 0;
	getClusterInducedNodes(clusterNode, num);

	return num;
}


//---------------------------------------------------------
// Construction
//---------------------------------------------------------

ClusterGraph::ClusterGraph()
{
	m_clusterIdCount = 0;
	m_postOrderStart = 0;
	m_rootCluster    = 0;

	m_allowEmptyClusters = true;
	m_updateDepth   = false;
	m_depthUpToDate = false;

	m_nClusters = 0;
	m_lcaNumber = 0;
	m_clusterArrayTableSize = MIN_CLUSTER_TABLE_SIZE;
	m_adjAvailable = false;
	m_lcaNumber = 0;
	m_lcaSearch = 0;
	m_vAncestor = 0;
	m_wAncestor = 0;
	//m_clusterDepth.init(*this, 0);
}



// Construction of a new cluster graph. All nodes
// are children of the root cluster
ClusterGraph::ClusterGraph(const Graph &G) : GraphObserver(&G), m_pGraph(&G)
{
	m_clusterIdCount = 0;
	m_postOrderStart = 0;
	m_rootCluster    = 0;

	m_allowEmptyClusters = true;
	m_updateDepth   = false;
	m_depthUpToDate = false;

	m_nClusters = 0;
	m_lcaNumber = 0;
	m_clusterArrayTableSize = G.nextPower2(MIN_CLUSTER_TABLE_SIZE, G.nodeArrayTableSize());
	//m_clusterDepth.init(*this, 0);
	initGraph(G);
}


ClusterGraph::ClusterGraph(const ClusterGraph &C) :
	GraphObserver(&(C.getGraph())),
	m_lcaSearch(0),
	m_vAncestor(0),
	m_wAncestor(0)
{
	m_clusterIdCount = 0;
	m_postOrderStart = 0;
	m_rootCluster    = 0;

	m_allowEmptyClusters = true;
	m_updateDepth   = false;
	m_depthUpToDate = false;

	m_nClusters = 0;
	m_lcaNumber = 0;

	m_clusterArrayTableSize = C.m_clusterArrayTableSize;
	shallowCopy(C);
}


ClusterGraph::ClusterGraph(
	const ClusterGraph &C,
	Graph &G,
	ClusterArray<cluster> &originalClusterTable,
	NodeArray<node> &originalNodeTable)
:
	GraphObserver(&G),
	m_lcaSearch(0),
	m_vAncestor(0),
	m_wAncestor(0)
{
	m_clusterIdCount = 0;
	m_postOrderStart = 0;
	m_rootCluster    = 0;

	m_allowEmptyClusters = true;
	m_updateDepth   = false;
	m_depthUpToDate = false;

	m_nClusters = 0;
	m_lcaNumber = 0;

	m_clusterArrayTableSize = C.m_clusterArrayTableSize;
	deepCopy(C,G,originalClusterTable,originalNodeTable);
}


ClusterGraph::ClusterGraph(
	const ClusterGraph &C,
	Graph &G,
	ClusterArray<cluster> &originalClusterTable,
	NodeArray<node> &originalNodeTable,
	EdgeArray<edge> &edgeCopy)
:
	GraphObserver(&G),
	m_lcaSearch(0),
	m_vAncestor(0),
	m_wAncestor(0)
{
	m_clusterIdCount = 0;
	m_postOrderStart = 0;
	m_rootCluster    = 0;

	m_allowEmptyClusters = true;
	m_updateDepth   = false;
	m_depthUpToDate = false;

	m_nClusters = 0;
	m_lcaNumber = 0;

	m_clusterArrayTableSize = C.m_clusterArrayTableSize;
	deepCopy(C, G, originalClusterTable, originalNodeTable, edgeCopy);
}


ClusterGraph::ClusterGraph(const ClusterGraph &C,Graph &G) :
	GraphObserver(&G),
	m_lcaSearch(0),
	m_vAncestor(0),
	m_wAncestor(0)
{
	m_clusterIdCount = 0;
	m_postOrderStart = 0;
	m_rootCluster    = 0;

	m_allowEmptyClusters = true;
	m_updateDepth   = false;
	m_depthUpToDate = false;

	m_nClusters = 0;
	m_lcaNumber = 0;

	m_clusterArrayTableSize = C.m_clusterArrayTableSize;
	deepCopy(C,G);
}


ClusterGraph::~ClusterGraph()
{
	for(ListIterator<ClusterArrayBase*> it = m_regClusterArrays.begin();
		it.valid(); ++it)
	{
		(*it)->disconnect();
	}

	clear();
}


// Construction of a new cluster graph. All nodes
// are children of the root cluster
void ClusterGraph::init(const Graph &G)
{
	clear();
	m_clusterIdCount = 0;
	m_postOrderStart = 0;
	m_pGraph = &G;

	m_nClusters = 0;
	m_lcaNumber = 0;
	m_clusterArrayTableSize = G.nextPower2(MIN_CLUSTER_TABLE_SIZE, G.nodeArrayTableSize());
	//m_clusterDepth.init(*this, 0);
	initGraph(G);
}



//---------------------------------------------------------
// =
//---------------------------------------------------------

ClusterGraph &ClusterGraph::operator=(const ClusterGraph &C)
{
	clear(); shallowCopy(C);
	m_clusterArrayTableSize = C.m_clusterArrayTableSize;
	reinitArrays();

	OGDF_ASSERT_IF(dlConsistencyChecks, consistencyCheck());
	return *this;
}


//---------------------------------------------------------
// copy,initGraph
//---------------------------------------------------------

// Copy Function
void ClusterGraph::shallowCopy(const ClusterGraph &C)
{
	const Graph &G = C;
	m_pGraph = &G;

	m_nClusters = 0;

	//m_clusterDepth.init(*this, 0);

	initGraph(G);

	m_updateDepth = C.m_updateDepth;
	m_depthUpToDate = C.m_depthUpToDate;

	// Construct cluster tree
	ClusterArray<cluster> originalClusterTable(C);
	cluster c = 0;
	forall_clusters(c,C)
	{
		if (c == C.m_rootCluster)
		{
			originalClusterTable[c] = m_rootCluster;
			//does not really need to be assigned HERE in for
			m_rootCluster->depth() = 1;
			OGDF_ASSERT(C.rootCluster()->depth() == 1)
			continue;
		}
		originalClusterTable[c] = newCluster();
		originalClusterTable[c]->depth() = c->depth();
	}
	forall_clusters(c,C)
	{

		if (c == C.m_rootCluster)
			continue;
		originalClusterTable[c]->m_parent = originalClusterTable[c->m_parent];
		originalClusterTable[c->m_parent]->m_children.pushBack(originalClusterTable[c]);
		originalClusterTable[c]->m_it = originalClusterTable[c->m_parent]->m_children.rbegin();
	}

	node v;
	forall_nodes(v,G)
		reassignNode(v,originalClusterTable[C.clusterOf(v)]);

	copyLCA(C);
}



// Initialize the graph
void ClusterGraph::initGraph(const Graph &G)
{

	reregister(&G); //will in some constructors cause double registration

	m_lcaNumber = 0;
	m_lcaSearch = 0;
	m_vAncestor = 0;
	m_wAncestor = 0;

	//if (G.empty())
	//	return;

	m_adjAvailable = false;

	//assign already existing nodes, new nodes are assigned
	//over nodeadded
	List<node> allNodes;
	G.allNodes(allNodes);

	//clusterIdcount may be zero in case of constructor call,
	//but can be != zero if readgraphwin is used
	//root cluster should always get id 0
	#ifdef OGDF_DEBUG
	m_rootCluster = OGDF_NEW ClusterElement(this, 0);//m_clusterIdCount++);
	#else
	m_rootCluster = OGDF_NEW ClusterElement(0);//m_clusterIdCount++);
	#endif

	OGDF_ASSERT(m_nClusters == 0)

	m_clusterIdCount++;
	m_rootCluster->depth() = 1;
	m_rootCluster->init(allNodes);
	m_nodeMap.init(G,m_rootCluster);
	m_itMap.init(G,0);
	ListIterator<node> it;
	for (it = m_rootCluster->m_entries.begin(); it.valid(); it++)
		m_itMap[*it] = it;

	m_nClusters++;
	m_clusters.pushBack(m_rootCluster);
}


void ClusterGraph::reinitGraph(const Graph &G)
	{
		m_pGraph = &G;

		OGDF_ASSERT_IF(dlConsistencyChecks, G.consistencyCheck());

		m_clusterArrayTableSize = G.nextPower2(MIN_CLUSTER_TABLE_SIZE, G.nodeArrayTableSize());

		if (numberOfClusters() != 0)
		{
			clear();
		}//if
		initGraph(G); //already constructs root cluster, reassign
	}


void ClusterGraph::reinitArrays()
{
	ListIterator<ClusterArrayBase*> itCluster = m_regClusterArrays.begin();
	for(; itCluster.valid(); ++itCluster)
		(*itCluster)->reinit(m_clusterArrayTableSize);
}



// Copy Function
void ClusterGraph::deepCopy(const ClusterGraph &C,Graph &G)
{

	const Graph &cG = C;	// original graph

	ClusterArray<cluster> originalClusterTable(C);
	NodeArray<node> originalNodeTable(cG);
	EdgeArray<edge> edgeCopy(cG);

	deepCopy(C,G,originalClusterTable, originalNodeTable, edgeCopy);
}

void ClusterGraph::deepCopy(const ClusterGraph &C,Graph &G,
							ClusterArray<cluster> &originalClusterTable,
							NodeArray<node> &originalNodeTable)
{

	const Graph &cG = C;	// original graph

	EdgeArray<edge> edgeCopy(cG);

	deepCopy(C,G,originalClusterTable, originalNodeTable, edgeCopy);
}

void ClusterGraph::deepCopy(const ClusterGraph &C,Graph &G,
							ClusterArray<cluster> &originalClusterTable,
							NodeArray<node> &originalNodeTable,
							EdgeArray<edge> &edgeCopy)
{
	G.clear();

	const Graph &cG = C;	// original graph

	m_pGraph = &G;

	m_nClusters = 0;

	initGraph(G); //arrays have already to be initialized for newnode

	m_updateDepth = C.m_updateDepth;
	m_depthUpToDate = C.m_depthUpToDate;

	NodeArray<node> orig(G);
	node v;
	edge e;

	forall_nodes(v,cG)
	{
		node w = G.newNode();
		orig[w] = v;
		originalNodeTable[v] = w;
	}
	forall_edges(e,cG)
	{
		edge eNew = G.newEdge(originalNodeTable[e->adjSource()->theNode()],
					originalNodeTable[e->adjTarget()->theNode()]);
		edgeCopy[e] = eNew;
	}//foralledges

	//m_clusterDepth.init(*this, 0);


	// Construct cluster tree
	cluster c = 0;
	forall_clusters(c,C)
	{
		if (c == C.m_rootCluster)
		{
			originalClusterTable[c] = m_rootCluster;
			//does not really need to be assigned HERE in for
			m_rootCluster->depth() = 1;
			OGDF_ASSERT(c->depth() == 1)
			continue;
		}
		originalClusterTable[c] = newCluster();
		originalClusterTable[c]->depth() = c->depth();
	}
	forall_clusters(c,C)
	{

		if (c == C.m_rootCluster)
			continue;
		originalClusterTable[c]->m_parent = originalClusterTable[c->m_parent];
		originalClusterTable[c->m_parent]->m_children.pushBack(originalClusterTable[c]);
		originalClusterTable[c]->m_it = originalClusterTable[c->m_parent]->m_children.rbegin();
	}

	forall_nodes(v,G)
		reassignNode(v,originalClusterTable[C.clusterOf(orig[v])]);

	//ClusterArray<cluster>* ca = ;
	copyLCA(C, &originalClusterTable);
}

//*********************************************************
//cluster search

//We search for the lowest common cluster of a set of nodes.
//We first compute the common path of two nodes, then update path if root
//path from other nodes hits it .
//We always stop if we encounter root cluster.
cluster ClusterGraph::commonCluster(SList<node>& nodes)
{
	//worst case running time #nodes x clustertreeheight-1
	//always <= complete tree run
	//we could even use pathcompression...
	//at any time, we stop if root is encountered as lowest
	//common cluster of a node subset


	if (nodes.empty()) return 0;

	//For simplicity, we use cluster arrays
	ClusterArray<int> commonPathHit(*this, 0); //count for clusters path hits
	int runs = 0; //number of nodes already considered
	cluster pathCluster;
	SListIterator<node> sIt = nodes.begin();
	node v1 = (*sIt);
	if (nodes.size() == 1) return clusterOf(v1);
	sIt++;
	node v2 = (*sIt);

	cluster lowestCommon = commonCluster(v1, v2);
	commonPathHit[lowestCommon] = 2;
	pathCluster = lowestCommon;
	while (pathCluster->parent())
	{
		pathCluster = pathCluster->parent();
		commonPathHit[pathCluster] = 2;
	}
	runs = 2;
	//we save direct lca access, it also lies on a runs hit path from root
	while ((runs < nodes.size()) && (lowestCommon != m_rootCluster))
	{
		sIt++;
		node v = (*sIt);
		pathCluster = clusterOf(v);
		while (commonPathHit[pathCluster] == 0)
		{
			if (pathCluster->parent()) pathCluster = pathCluster->parent();
			else return m_rootCluster; //can never happen
		}//while
		//assign new (maybe same) lowest common
		if (commonPathHit[pathCluster] == runs) lowestCommon = pathCluster;
		commonPathHit[pathCluster] = commonPathHit[pathCluster]+1;
		if (pathCluster == m_rootCluster) return m_rootCluster;
		//update hits in path to root
		while (pathCluster->parent())
		{
			pathCluster = pathCluster->parent();
			commonPathHit[pathCluster] = commonPathHit[pathCluster]+1;
		}

		runs++;
	}
	return lowestCommon;


}//commoncluster

//lowest common cluster of v,w
cluster ClusterGraph::commonCluster(node v, node w) const
{
	cluster c1, c2;
	return commonClusterLastAncestors(v, w, c1, c2);
}//commonCluster

//lowest common cluster of v,w and its ancestors
cluster ClusterGraph::commonClusterLastAncestors(node v,
												 node w,
												 cluster& c1,
												 cluster& c2) const
{
	List<cluster> e;
	return commonClusterAncestorsPath(v, w, c1, c2, e);
}//commonClusterLastAncestors
//lowest common cluster and path between v and w containing it
//note that eL is directed from v to w
cluster ClusterGraph::commonClusterPath(node v,
										node w,
										List<cluster>& eL) const
{
	cluster c1, c2;
	return commonClusterAncestorsPath(v, w, c1, c2, eL);
}//commonClusterLastAncestors

//note that eL is directed from v to w
cluster ClusterGraph::commonClusterAncestorsPath(node v,
												 node w,
												 cluster& c1,
												 cluster& c2,
												 List<cluster>& eL) const
{
	OGDF_ASSERT(v->graphOf() == m_pGraph)
	OGDF_ASSERT(w->graphOf() == m_pGraph)

	cluster cv = clusterOf(v);
	cluster cw = clusterOf(w);


	//clusters from v and w to common
	List<cluster> vList;
	List<cluster> wList;

	//CASE1 no search necessary
	//if both nodes are in the same cluster, we return this cluster
	//and have to check if c1 == c2 to have a (v,w) representation edge
	if (cv == cw)
	{
		c1 = c2 = cv;
		eL.pushBack(c1);
		return cv;
	}

	if (m_lcaNumber == INT_MAX - 1) m_lcaNumber = 0;
	else m_lcaNumber++;
	if (!m_lcaSearch)
	{
		m_lcaSearch = OGDF_NEW ClusterArray<int>(*this, -1);
		m_vAncestor = OGDF_NEW ClusterArray<cluster>(*this, 0);
		m_wAncestor = OGDF_NEW ClusterArray<cluster>(*this, 0);
	}

	//CASE2: one of the nodes hangs at root: save root as ancestor
	//any other case: save cluster of node as ancestor, too, to check this
	//case:: common = xCluster != yCluster
	//(*m_vAncestor)[rootCluster()] = rootCluster();
	//(*m_wAncestor)[rootCluster()] = rootCluster();
	(*m_vAncestor)[cv] = 0;
	(*m_wAncestor)[cw] = 0;

	//we rely on the fact all nodes are in the rootcluster or
	//that parent is initialized to zero to terminate

	//we start with different clusters due to CASE1
	//save the ancestor information
	(*m_lcaSearch)[cw] = m_lcaNumber; //not really necessary, we won't return
	(*m_lcaSearch)[cv] = m_lcaNumber;
	vList.pushBack(cv);
	wList.pushBack(cw);

	//we break and return if we find a common node
	//before we reach the rootcluster
	do
	{
		if (cv->parent()) //root reached?
		{
			(*m_vAncestor)[cv->parent()] = cv;
			cv = cv->parent();
			//was cv visited on path from w
			if ((*m_lcaSearch)[cv] == m_lcaNumber)
			{
				c1 = (*m_vAncestor)[cv];
				c2 = (*m_wAncestor)[cv];
				//setup list
				ListIterator<cluster> itC = vList.begin();

				while (itC.valid())
				{
					eL.pushBack(*itC);
					itC++;
				}
				itC = wList.rbegin();
				while (itC.valid() && ((*itC) != cv))
					itC--;
				while (itC.valid())
				{
					eL.pushBack(*itC);
					itC--;
				}

				return cv;
			}
			vList.pushBack(cv);
			(*m_lcaSearch)[cv] = m_lcaNumber;
		}//if not root reached on cvpath

		if (cw->parent())
		{
			(*m_wAncestor)[cw->parent()] = cw;
			cw = cw->parent();
			//was cw visited on path from v
			if ((*m_lcaSearch)[cw] == m_lcaNumber)
			{
				c1 = (*m_vAncestor)[cw];
				c2 = (*m_wAncestor)[cw];
				//setup list
				ListIterator<cluster> itC = vList.begin();
				while (itC.valid() && ((*itC) != cw))
				{
					eL.pushBack(*itC);
					itC++;
				}

				eL.pushBack(cw);

				itC = wList.rbegin();
				while (itC.valid())
				{
					eL.pushBack(*itC);
					itC--;
				}

				return cw;
			}
			wList.pushBack(cw);
			(*m_lcaSearch)[cw] = m_lcaNumber;

		}//if not root reached on cwpath
	}	while ( cv->parent() || cw->parent() );

	//v,w should be at least together in the rootcluster
	c1 = (*m_vAncestor)[rootCluster()];
	c2 = (*m_wAncestor)[rootCluster()];
	return rootCluster();

}//commonclusterlastAncestors

void ClusterGraph::copyLCA(
	const ClusterGraph &C,
	ClusterArray<cluster>* clusterCopy)
{
	if (m_lcaSearch)
	{
		delete m_lcaSearch;
		delete m_vAncestor;
		delete m_wAncestor;
	}//if
	if (C.m_lcaSearch)
	{
		//otherwise, initialization won't work
		m_clusterArrayTableSize = C.m_clusterArrayTableSize;

		m_lcaSearch = OGDF_NEW ClusterArray<int>(*this, -1);//(*C.m_lcaSearch);

		m_vAncestor = OGDF_NEW ClusterArray<cluster>(*this, 0); //*C.m_vAncestor);
		//m_vAncestor->init(*this, 0),
		m_wAncestor = OGDF_NEW ClusterArray<cluster>(*this, 0);//*C.m_wAncestor);
		//setting of clusters is not necessary!
		//(*m_v/wAncestor)[(*clusterCopy)[c]]= (*(C.m_v/wAncestor))[c];
	}//if
}//copylca

//---------------------------------------------------------
// check the graph for empty clusters
//---------------------------------------------------------
//we never set rootcluster to be one of the empty clusters!!
void ClusterGraph::emptyClusters(SList<cluster>& emptyCluster,
								 SList<cluster>* checkCluster)
{
	emptyCluster.clear();
	cluster cc;
	//for all nodes = #nodes
	if (checkCluster)
	{
		SListIterator<cluster> it = checkCluster->begin();

		while (it.valid())
		{
			if ((*it)->cCount() + (*it)->nCount() == 0)
				if ((*it) != rootCluster()) //we dont add rootcluster
					emptyCluster.pushBack((*it));
			it++;
		}//while
	}//if checkcluster given
	else
	{
		forall_clusters(cc, *this)
		{
			if (cc->cCount() + cc->nCount() == 0)
				if (cc != rootCluster()) //we dont add rootcluster
					emptyCluster.pushBack(cc);
		}//forallclusters
	}//else checkcluster
	//other clusters can get empty, too, if we delete these
	ClusterArray<int> delCount(*this, 0);
	SList<cluster> emptyParent;
	SListIterator<cluster> itC = emptyCluster.begin();
	while (itC.valid())
	{
		//count deleted children
		cluster runc = (*itC)->parent();
		if (runc)  //is always the case as long as root was not inserted to list
		{
			delCount[runc]++;
			while ((runc->nCount() == 0) && (runc->cCount() == delCount[runc]))
			{
				if (runc == rootCluster()) break;
				emptyParent.pushBack(runc);
				runc = runc->parent();
				delCount[runc]++;
			}//while parent emptied
		}//if not runc = root->parent

		itC++;
	}//while empty leaves

	emptyCluster.conc(emptyParent);
	//for reinsertion, start at emptycluster's back

}//emptyClusters

//---------------------------------------------------------
// newCluster, delCluster, createCluster
//---------------------------------------------------------

// Inserts a new cluster prescribing its parent
cluster ClusterGraph::newCluster(cluster parent, int id)
{
	OGDF_ASSERT(parent);
	cluster c;
	if (id > 0)
		c = newCluster(id);
	else
		c = newCluster();
	parent->m_children.pushBack(c);
	c->m_it = parent->m_children.rbegin();
	c->m_parent = parent;
	c->depth() = parent->depth() + 1;

	return c;
}

//Insert a new cluster with given ID, precondition: id not used
//has to be updated in the same way as newcluster()
cluster ClusterGraph::newCluster(int id)
{
	m_nClusters++;
	m_adjAvailable = false;
	m_postOrderStart = 0;
	if (id >= m_clusterIdCount) m_clusterIdCount = id+1;
	if (m_clusterIdCount >= m_clusterArrayTableSize)
	{
		m_clusterArrayTableSize =
			m_pGraph->nextPower2(m_clusterArrayTableSize, id);
		for(ListIterator<ClusterArrayBase*> it = m_regClusterArrays.begin();
			it.valid(); ++it)
		{
			(*it)->enlargeTable(m_clusterArrayTableSize);
		}
	}
	#ifdef OGDF_DEBUG
	cluster c = OGDF_NEW ClusterElement(this,id);
	#else
	cluster c = OGDF_NEW ClusterElement(id);
	#endif
	m_clusters.pushBack(c);
	// notify observers
	for(ListIterator<ClusterGraphObserver*> it = m_regObservers.begin();
			it.valid(); ++it) (*it)->clusterAdded(c);
	return c;
}

// Inserts a new cluster
//has to be updated in the same way as newcluster(id)
cluster ClusterGraph::newCluster()
{
	m_nClusters++;
	m_adjAvailable = false;
	m_postOrderStart = 0;
	if (m_clusterIdCount == m_clusterArrayTableSize)
	{
		m_clusterArrayTableSize <<= 1;
		for(ListIterator<ClusterArrayBase*> it = m_regClusterArrays.begin();
			it.valid(); ++it)
		{
			(*it)->enlargeTable(m_clusterArrayTableSize);
		}
	}
	#ifdef OGDF_DEBUG
	cluster c = OGDF_NEW ClusterElement(this,m_clusterIdCount++);
	#else
	cluster c = OGDF_NEW ClusterElement(m_clusterIdCount++);
	#endif
	m_clusters.pushBack(c);
	// notify observers
	for(ListIterator<ClusterGraphObserver*> it = m_regObservers.begin();
			it.valid(); ++it) (*it)->clusterAdded(c);
	return c;
}

cluster ClusterGraph::createEmptyCluster(const cluster parent, int clusterId)
{
	//if no id given, use next free id
	if (clusterId < 0) clusterId = m_clusterIdCount;
	//create the new cluster
	cluster cnew;
	if (parent)
		cnew = newCluster(parent, clusterId);
	else
		cnew = newCluster(m_rootCluster, clusterId);
	return cnew;
}//createemptycluster

cluster ClusterGraph::createCluster(SList<node>& nodes, const cluster parent)
{
	cluster c;
	if (m_allowEmptyClusters)
	{
		c = doCreateCluster(nodes, parent);
		return c;
	}
	else
	{
		SList<cluster> emptyCluster;

		c = doCreateCluster(nodes, emptyCluster, parent);

		SListIterator<cluster> sIt = emptyCluster.begin();
		while (sIt.valid())
		{
			delCluster((*sIt));

			//root cluster can never be empty, as we deleted a node
			sIt++;
		}//While
	}
	return c;
}

cluster ClusterGraph::doCreateCluster(SList<node>& nodes,
		const cluster parent,
		int clusterId)
{

	if (nodes.empty()) return 0;

	//if no id given, use next free id
	if (clusterId < 0) clusterId = m_clusterIdCount;
	//create the new cluster
	cluster cnew;
	if (parent)
		cnew = newCluster(parent, clusterId);
	else
		cnew = newCluster(m_rootCluster, clusterId);

	//insert nodes in new cluster
	SListIterator<node> it = nodes.begin();
	while (it.valid())
	{
		reassignNode((*it), cnew);
		it++;
	}//while

	return cnew;
}//createcluster

cluster ClusterGraph::doCreateCluster(SList<node>& nodes,
		SList<cluster>& emptyCluster,
		const cluster parent,
		int clusterId)
{
	// Even if m_allowEmptyClusters is set we check if a cluster
	// looses all of its nodes and has
	// no more entries and childs. This can be used for special cluster
	// object handling or for deletion if m_allowEmptyClusters is not set
	// if it is not the new parent, it can be deleted
	// running time max(#cluster, length(nodelist))
	// TODO: Parameter, der dies auslaesst, da hohe Laufzeit
	// hier macht das nur Sinn, wenn es schneller ist als forallclusters,
	// sonst koennte man es ja auch aussen testen, aber bisher ist es nicht
	// schneller implementiert
	// Vorgehen: hash auf cluster index, falls nicht gesetzt, in liste einfuegen
	// und als checkcluster an emptycluster uebergeben

	if (nodes.empty()) return 0;

	//if no id given, use next free id
	if (clusterId < 0) clusterId = m_clusterIdCount;
	//create the new cluster
	cluster cnew;
	if (parent)
		cnew = newCluster(parent, clusterId);
	else
		cnew = newCluster(m_rootCluster, clusterId);

	//insert nodes in new cluster
	SListIterator<node> it = nodes.begin();
	while (it.valid())
	{
		reassignNode((*it), cnew);
		it++;
	}//while

	//should be: only for changed clusters (see comment above)
	//it is important to save the cluster in an order
	//that allows deletion as well as reinsertion
	emptyClusters(emptyCluster);
	//for reinsertion, start at emptycluster's back

	return cnew;
}//createcluster

// Deletes cluster c
// All subclusters become children of parent cluster
// Precondition: c is not the root cluster
// updating of cluster depth information pumps running time
// up to worst case O(#C)
void ClusterGraph::delCluster(cluster c)
{
	OGDF_ASSERT(c != 0 && c->graphOf() == this && c != m_rootCluster)

	// notify observers
	for(ListIterator<ClusterGraphObserver*> it = m_regObservers.begin();
			it.valid(); ++it) (*it)->clusterDeleted(c);

	--m_nClusters;
	m_adjAvailable = false;

	c->m_parent->m_children.del(c->m_it);
	c->m_it = 0;

	while (!c->m_children.empty())
	{
		cluster trace = c->m_children.popFrontRet();
		trace->m_parent =  c->m_parent;
		trace->m_parent->m_children.pushBack(trace);
		trace->m_it = trace->m_parent->m_children.rbegin();

		//only recompute depth if option set and it makes sense
		if (m_updateDepth && m_depthUpToDate)
		{
			//update depth for all children in subtree
			OGDF_ASSERT(trace->depth() == trace->parent()->depth()+2)
			pullUpSubTree(trace);
			//could just set depth-1 here
			//trace->depth() = trace->parent()->depth()+1;

		}///if depth update
		else m_depthUpToDate = false;
	}
	while (!c->m_entries.empty())
	{
		node v = c->m_entries.popFrontRet();
		m_nodeMap[v] = 0;
		reassignNode(v,c->m_parent);
	}

	m_clusters.del(c);
}

//pulls up depth of subtree located at c by one
//precondition: depth is consistent
//we dont ask for depthuptodate since the caller needs
//to know for himself if he wants the tree to be pulled
//for any special purpose
void ClusterGraph::pullUpSubTree(cluster c)
{
	c->depth() = c->depth() - 1;
	ListConstIterator<cluster> it = c->getChildren().begin();
	while (it.valid())
	{
		pullUpSubTree(*it);
		it++;
	}

}

//---------------------------------------------------------
// clear, clearClusterTree
//---------------------------------------------------------

void ClusterGraph::clear()
{
	//split condition
	if (m_lcaSearch)
	{
		delete m_lcaSearch;
		delete m_vAncestor;
		delete m_wAncestor;
	}
	if (m_nClusters != 0)
	{
		clearClusterTree(m_rootCluster);
		m_clusters.del(m_rootCluster);
	}
	//no clusters, so we can restart at 0
	m_clusterIdCount = 0;
	m_nClusters = 0;
}


// Removes the Clustering of a Tree and frees the allocated memory
void ClusterGraph::clearClusterTree(cluster c)
{
	cluster trace = 0;
	cluster parent = c->parent();
	m_postOrderStart = 0;
	m_adjAvailable = false;

	List<cluster>  children = c->getChildren();
	List<node>     attached;

	while (!children.empty())
	{
		trace = children.popFrontRet();
		clearClusterTree(trace,attached);
	}

	if (parent != 0)
	{
		ListIterator<node> it;
		for (it = attached.begin();it.valid();it++)
		{
			m_nodeMap[(*it)] = parent;
			parent->m_entries.pushBack((*it));
			m_itMap[(*it)] = parent->m_entries.rbegin();
		}
		m_clusters.del(c);
	}
	else if (c == m_rootCluster)
	{
		ListIterator<node> it;
		for (it = attached.begin();it.valid();it++)
		{
			m_nodeMap[(*it)] = m_rootCluster;
			m_rootCluster->m_entries.pushBack((*it));
			m_itMap[(*it)] = m_rootCluster->m_entries.rbegin();
		}
		m_rootCluster->m_children.clear();
	}
}

void ClusterGraph::clearClusterTree(cluster c,List<node> &attached)
{
	cluster trace;
	List<cluster>  children = c->getChildren();
	attached.conc(c->m_entries);
	m_adjAvailable = false;

	while (!children.empty())
	{
		trace = children.popFrontRet();
		clearClusterTree(trace,attached);
	}
	m_clusters.del(c);
}

//don't delete root cluster
void ClusterGraph::semiClear()
{
	//split condition
	if (m_lcaSearch)
	{
		delete m_lcaSearch;
		delete m_vAncestor;
		delete m_wAncestor;
	}
	if (m_nClusters != 0)
	{
		//clear the cluster structure under root cluster
		clearClusterTree(m_rootCluster);
		//now delete all rootcluster entries
		while (!m_rootCluster->m_entries.empty())
		{
			node v = m_rootCluster->m_entries.popFrontRet();
			m_nodeMap[v] = 0;
		}
	}
	//no child clusters, so we can restart at 1
	m_clusterIdCount = 1;
	m_nClusters = 1;
}

//reassign cluster depth for clusters in subtree rooted at c
void ClusterGraph::computeSubTreeDepth(cluster c) const
{
	if (c == rootCluster()) m_depthUpToDate = true;
	if (!(c->parent())) c->depth() = 1;
	else c->depth() = c->parent()->depth() + 1;
	ListConstIterator<cluster> it = c->getChildren().begin();
	while (it.valid())
	{
		computeSubTreeDepth(*it);
		it++;
	}

}

//move cluster from old parent to an other
void ClusterGraph::moveCluster(cluster c, cluster newParent)
{
	if (c == rootCluster()) return;
	if ((c == 0) || (newParent == 0)) return; //no cheap tricks
	if (c->parent() == newParent) return;     //no work to do

	cluster oldParent = c->parent();
	//we dont move root
	OGDF_ASSERT(oldParent)

	//check if we move to a descendant
	cluster crun = newParent->parent();
	bool descendant = false;
	while (crun)
	{
		if (crun == c)
		{
			descendant = true;
			break;
		}
		crun = crun->parent();
	}//while running upwards

	//do not allow to move empty clusters to descendants
	if (descendant && (c->nCount() == 0))
		return;

	// save postorder for old parent
	bool newOrder = false;
	if (!m_postOrderStart)
	{
		newOrder = true;
	}

	//temporarily only recompute postorder for all clusters

	oldParent->m_children.del(c->m_it);
	newParent->m_children.pushBack(c);
	c->m_it = newParent->m_children.rbegin();
	c->m_parent = newParent;

	//update the cluster depth information in the subtree
	//If moved to descendant, recompute
	//depth for parent (including all brother trees)
	if (descendant)
	{
		//how do we move:
		//only entries with c? => may be empty
		//we currently dont allow this, because it makes
		//no sense, you could just delete the cluster or move
		//the children
		//move all children to oldparent

		while (!c->m_children.empty())
		{
			cluster child = c->m_children.popFrontRet();
			child->m_parent =  oldParent;
			child->m_parent->m_children.pushBack(child);
			child->m_it = child->m_parent->m_children.rbegin();
			//child++;
		}

		//recompute depth only if option set AND it makes sense at that point
		if (m_updateDepth && m_depthUpToDate)
			computeSubTreeDepth(oldParent);
		else m_depthUpToDate = false;
	}//moved to descendant
	else
	{
		if (m_updateDepth && m_depthUpToDate)
			computeSubTreeDepth(c);
		else m_depthUpToDate = false;
	}

	// update postorder for new parent
	// we only recompute postorder for all clusters
	// because of special cases like move to descendant...
	if (newOrder) postOrder();
	else postOrder();

	m_adjAvailable = false;

	//checkPostOrder();
}//move cluster


//*****************
//postorder updates

//leftmostcluster in subtree rooted at c, has postorderpred for subtree
cluster ClusterGraph::leftMostCluster(cluster c) const
{
	cluster result = c;
	if (!c) return 0;
	while (!result->m_children.empty())
	{
		result = result->m_children.front();
	}
	return result;
}//leftMostCluster

//searches for predecessor of SUBTREE at c
cluster ClusterGraph::postOrderPredecessor(cluster c) const
{
	//all clusters on a path from root to leftmost cluster in tree
	//have no predecessor for their subtree
	cluster run = c;
	ListConstIterator<cluster> it;
	do
	{
		//predecessor of clustertree is	0
		if (run == m_rootCluster) return 0;
		it = run->m_it;
		//a child to the left is the immediate predecessor,
		//otherwise we go one level up
		if (it == (run->m_parent)->m_children.begin())
			run = run->parent();
		else return (*(it.pred()));

	} while (run);

	return 0;
}//postorderpredecessor

//***************
//node assignment
//Assigns a node to a new cluster
void ClusterGraph::assignNode(node v, cluster c)
{
	m_adjAvailable = false;
	m_postOrderStart = 0;
	m_nodeMap[v] = c;
	c->m_entries.pushBack(v);
	m_itMap[v] = c->m_entries.rbegin();
}


//Reassigns a node to a new cluster
void ClusterGraph::reassignNode(node v, cluster c)
{
	OGDF_ASSERT(v->graphOf() == m_pGraph);
	OGDF_ASSERT(c->graphOf() == this);

	unassignNode(v);
	m_nodeMap[v] = c;
	c->m_entries.pushBack(v);
	m_itMap[v] = c->m_entries.rbegin();
}


//Unassigns a node of cluster
//Note: Nodes can already be unassigned by the nodeDeleted function.
void ClusterGraph::unassignNode(node v)
{
	m_adjAvailable = false;
	m_postOrderStart = 0;

	removeNodeAssignment(v);
}


//---------------------------------------------------------
// Sort clusters in post order
//---------------------------------------------------------

// Start function for post order
void ClusterGraph::postOrder() const
{
	SListPure<cluster> L;
	postOrder(m_rootCluster,L);
	cluster c = 0;
	cluster prev = L.popFrontRet();
	prev->m_pPrev = 0;
	m_postOrderStart = prev;
	while (!L.empty())
	{
		c = L.popFrontRet();
		prev->m_pNext = c;
		c->m_pPrev = prev;
		prev = c;
	}
	if (c != 0)
		c->m_pNext = 0;
	else
		m_postOrderStart->m_pNext = 0;
#ifdef OGDF_DEBUG
	forall_clusters(c, *this)
	{
		cluster cp = leftMostCluster(c);
		OGDF_ASSERT(cp->pPred() == postOrderPredecessor(c))
	}
#endif
}

void ClusterGraph::checkPostOrder() const
{
	SListPure<cluster> L;
	postOrder(m_rootCluster,L);
	cluster c = 0;
	cluster prev = L.popFrontRet();
	OGDF_ASSERT(prev->m_pPrev == 0);

	while (!L.empty())
	{
		c = L.popFrontRet();
		OGDF_ASSERT(prev->m_pNext == c)
		OGDF_ASSERT(c->m_pPrev == prev)
		prev = c;
	}
	if (c != 0)
	{
		OGDF_ASSERT(c->m_pNext == 0)
	}
	else
	{
		OGDF_ASSERT(m_postOrderStart->m_pNext == 0);
	}
}
// Recursive function for post order
void ClusterGraph::postOrder(cluster c,SListPure<cluster> &L) const
{
	ListIterator<cluster> it;
	for (it = c->m_children.begin(); it.valid(); it++)
		postOrder((*it),L);
	L.pushBack(c);
}




//---------------------------------------------------------
// Methods for debugging
//---------------------------------------------------------


// checks the consistency of the data structure
// (for debugging only)
bool ClusterGraph::consistencyCheck()
{

	ClusterArray<bool> visitedClusters((*this),false);
	NodeArray<bool> visitedNodes((*m_pGraph),false);


	cluster c = 0;
	forall_postOrderClusters(c,(*this))
	{
		visitedClusters[c] = true;
		ListIterator<node> itn;
		for (itn = c->m_entries.begin(); itn.valid(); itn++)
		{
			node v = *itn;
			if (m_nodeMap[v] != c)
				return false;
			visitedNodes[v] = true;
		}
	}
	forall_clusters(c,(*this))
		if (!visitedClusters[c])
			return false;
	node v;
	forall_nodes(v,(*m_pGraph))
		if (!visitedNodes[v])
			return false;
	return true;
}



bool ClusterGraph::representsCombEmbedding()
{

	if (!m_adjAvailable)
		return false;

	if (!consistencyCheck())
		return false;


	cluster c = 0;
	forall_postOrderClusters(c,(*this))
	{

		#ifdef OGDF_DEBUG
		if (int(ogdf::debugLevel) >= int(dlHeavyChecks)){
			cout << "__________________________________________________________________"
				 << endl << endl
				 << "Testing cluster " << c << endl
				 << "Check on AdjList of c" << endl;
			adjEntry adjDD;
			forall_cluster_adj(adjDD,c)
				cout << adjDD << ";  ";
			cout << endl;
		}
		#endif

		if (c != m_rootCluster)
		{

			ListIterator<adjEntry> it;
			it = c->firstAdj();
			adjEntry start = *it;

			#ifdef OGDF_DEBUG
			if (int(ogdf::debugLevel) >= int(dlHeavyChecks)){
				cout << "firstAdj " << start << endl; }
			#endif

			while (it.valid())
			{
				AdjEntryArray<bool> visitedAdjEntries((*m_pGraph),false);

				ListIterator<adjEntry> succ = it.succ();
				adjEntry adj = *it;
				adjEntry succAdj;

				if (succ.valid())
					succAdj = *succ;
				else
					succAdj = start;  // reached the last outgoing edge

				#ifdef OGDF_DEBUG
				if (int(ogdf::debugLevel) >= int(dlHeavyChecks)){
					cout << "Check next " << endl;
					cout << "current in adj list of" << adj << endl;
					cout << "succ in adj list of c " << succAdj << endl;
					cout << "cyclic succ in outer face " << adj->cyclicSucc() << endl;
				}
				#endif



				if (adj->cyclicSucc() != succAdj)
					// run along the outer face of the cluster
					// until you find the next outgoing edge
				{
					adjEntry next = adj->cyclicSucc();
					adjEntry twin = next->twin();

					#ifdef OGDF_DEBUG
					if (int(ogdf::debugLevel) >= int(dlHeavyChecks)){
						cout << "Running along the outer face ... " << endl;
						cout << "next adj " << next << endl;
						cout << "twin adj " << twin << endl;
					}
					#endif

					if (visitedAdjEntries[twin])
						return false;
					visitedAdjEntries[twin] = true;
					while ( next != succAdj)
					{
						next = twin->cyclicSucc();
						twin = next->twin();
						#ifdef OGDF_DEBUG
						if (int(ogdf::debugLevel) >= int(dlHeavyChecks)){
							cout << "Running along the outer face ... " << endl;
							cout << "next adj " << next << endl;
							cout << "twin adj " << twin << endl;
						}
						#endif
						if (visitedAdjEntries[twin])
							return false;
						visitedAdjEntries[twin] = true;
					}

				}
				// else
				// next edge is also outgoing

				it = succ;
			}
		}
	}


	return true;

}




// registers a cluster array
ListIterator<ClusterArrayBase*> ClusterGraph::registerArray(
	ClusterArrayBase *pClusterArray) const
{
	return m_regClusterArrays.pushBack(pClusterArray);
}

// unregisters a cluster array
void ClusterGraph::unregisterArray(ListIterator<ClusterArrayBase*> it) const
{
	m_regClusterArrays.del(it);
}

//! Registers a ClusterGraphObserver.
ListIterator<ClusterGraphObserver*> ClusterGraph::registerObserver(ClusterGraphObserver *pObserver) const
{
	return m_regObservers.pushBack(pObserver);
}

//! Unregisters a ClusterGraphObserver.
void ClusterGraph::unregisterObserver(ListIterator<ClusterGraphObserver*> it) const
{
	m_regObservers.del(it);
}
//---------------------------------------------------------
// Methods for printing
//---------------------------------------------------------


// writes graph in GML format to file fileName
void ClusterGraph::writeGML(const char *fileName)
{
	ofstream os(fileName);
	writeGML(os);
}

// writes graph in GML format to output stream os
void ClusterGraph::writeGML(ostream &os)
{
	NodeArray<int>    nId(*m_pGraph);
	ClusterArray<int> cId(*this);
	int nextId = 0;

	os << "Creator \"ogdf::ClusterGraph::writeGML\"\n";
	os << "graph [\n";
	os << "  directed 1\n";

	node v;
	forall_nodes(v,*m_pGraph) {
		os << "  node [\n";
		os << "    id " << (nId[v] = nextId++) << "\n";
		os << "  ]\n"; // node
	}

	edge e;
	forall_edges(e,*m_pGraph) {
		os << "  edge [\n";
		os << "    source " << nId[e->source()] << "\n";
		os << "    target " << nId[e->target()] << "\n";
		os << "  ]\n"; // edge
	}

	String scip = "  ";
	nextId = 0;
	writeCluster(os,nId,cId,nextId,m_rootCluster,scip);

	os << "]\n"; // graph
}


// recursively write the cluster structure in GML
void ClusterGraph::writeCluster(ostream &os,
								NodeArray<int> &nId,
								ClusterArray<int> & cId,
								int &nextId,
								cluster c,
								String scip)
{
	String newScip = scip;
	newScip+="  ";
	os << scip << "cluster [\n";
	os << scip << "  id " << (cId[c] = nextId++) << "\n";
	ListIterator<cluster> it;
	for (it = c->m_children.begin(); it.valid(); it++)
		writeCluster(os,nId,cId,nextId,*it,newScip);
	ListIterator<node> itn;
	for (itn = c->m_entries.begin(); itn.valid(); itn++)
		os << scip << "  node " << nId[*itn] << "\n";
	os << scip << "]\n"; // cluster
}


// recursively write the cluster structure in GraphWin GML
void ClusterGraph::writeGraphWinCluster(ostream &os,
								NodeArray<int> &nId,
								NodeArray<String> &nStr,
								ClusterArray<int> & cId,
								ClusterArray<String> & cStr,
								int &nextId,
								cluster c,
								String scip)
{
	String newScip = scip;
	newScip+="  ";
	if (c == m_rootCluster)
		os << scip << "rootcluster [\n";
	else
	{
		os << scip << "cluster [\n";
//		os << scip << "  id " << (cId[c] = nextId++) << "\n";
		os << scip << "  id " << c->index() << "\n";
		char newLabel[124];
//		sprintf(newLabel,"C%d",cId[c]);
		ogdf::sprintf(newLabel,124,"C%d",c->index());
		cStr[c] = newLabel;
		os << scip << "  label \"" << cStr[c] << "\"\n";

	}
	ListIterator<cluster> it;
	for (it = c->m_children.begin(); it.valid(); it++)
		writeGraphWinCluster(os,nId,nStr,cId,cStr,nextId,*it,newScip);
	ListIterator<node> itn;
	for (itn = c->m_entries.begin(); itn.valid(); itn++)
		os << scip << "  vertex \"v" << nId[*itn] << "\"\n";
	os << scip << "]\n"; // cluster
}


//++++++++++++++++++++++++++++++++++++++++++++
//reading graph,  cluster structure
bool ClusterGraph::readClusterGML(const char* fileName,
								Graph& G)
{
	ifstream is(fileName);
	if (!is)
		return false; // couldn't open file

	return readClusterGML(is, G);
}

bool ClusterGraph::readClusterGML(istream& is,
								Graph& G)
{
	bool result;
	GmlParser gml(is);
	if (gml.error())
		return false;

	result = gml.read(G);

	if (!result) return false;

	return gml.readCluster(G, *this);
}


// read Cluster Graph from OGML file
//bool ClusterGraph::readClusterGraphOGML(const char* fileName,
//												 ClusterGraph& CG,
//												 Graph& G)
//{
//	ifstream is(fileName);
//	// not able to open file
//	if (!is) return false;
//
//	OgmlParser *op = new OgmlParser();
//	// build graph
//	// method read contains the validation
//	if (!op->read(fileName, G, CG, *this)){
//		delete(op);
//		cerr << "ERROR occured while reading. Aborting." << endl << flush;
//		return false;
//	}
//
//	delete(op);
//	return true;
//};


} // end namespace ogdf

//****************************************************************
ostream &operator<<(ostream &os, ogdf::cluster c)
{
	if (c) os << c->index(); else os << "nil";
	return os;
}
