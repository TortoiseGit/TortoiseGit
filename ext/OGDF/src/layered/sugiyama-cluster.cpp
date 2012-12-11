/*
 * $Revision: 2565 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 17:14:54 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of Sugiyama algorithm (extension with
 * clusters)
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


#include <ogdf/layered/SugiyamaLayout.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/layered/OptimalRanking.h>
#include <ogdf/basic/Queue.h>
#include <ogdf/basic/Array2D.h>
#include <ogdf/cluster/ClusterSet.h>


namespace ogdf {


//---------------------------------------------------------
// RCCrossings
//---------------------------------------------------------

ostream& operator<<(ostream &os, const RCCrossings &cr)
{
	os << "(" << cr.m_cnClusters << "," << cr.m_cnEdges << ")";
	return os;
}


//---------------------------------------------------------
// LHTreeNode
// represents a node in a layer hierarchy tree
//---------------------------------------------------------

void LHTreeNode::setPos()
{
	for(int i = 0; i <= m_child.high(); ++i)
		m_child[i]->m_pos = i;
}


void LHTreeNode::removeAuxChildren()
{
	OGDF_ASSERT(isCompound() == true);

	int j = 0;
	int i;
	for(i = 0; i <= m_child.high(); ++i)
		if(m_child[i]->m_type != AuxNode)
			m_child[j++] = m_child[i];
		else
			delete m_child[i];

	int add = j-i;
	if(add != 0)
		m_child.grow(add, 0);
}


ostream &operator<<(ostream &os, const LHTreeNode *n)
{
	if(n->isCompound()) {
		os << "C" << n->originalCluster();

		os << " [";
		for(int i = 0; i < n->numberOfChildren(); ++i)
			os << " " << n->child(i);
		os << " ]";

	} else {
		os << "N" << n->getNode() << " ";
	}

	return os;
}


//---------------------------------------------------------
// AdjacencyComparer
// compares adjacency entries in an LHTreeNode
//---------------------------------------------------------

class AdjacencyComparer
{
	static int compare(const LHTreeNode::Adjacency &x, const LHTreeNode::Adjacency &y)
	{
		if(x.m_u->index() < y.m_u->index())
			return -1;

		else if(x.m_u == y.m_u) {
			if(x.m_v->isCompound()) {
				if(!y.m_v->isCompound()) return -1;
				return (x.m_v->originalCluster()->index() < y.m_v->originalCluster()->index()) ? -1 : +1;

			} else if(y.m_v->isCompound())
				return +1;

			else
				return (x.m_v->getNode()->index() < y.m_v->getNode()->index()) ? -1 : +1;

		} else
			return +1;
	}

	OGDF_AUGMENT_STATICCOMPARER(LHTreeNode::Adjacency)
};


//---------------------------------------------------------
// ENGLayer
// represents layer in an extended nesting graph
//---------------------------------------------------------

ENGLayer::~ENGLayer()
{
	Queue<LHTreeNode*> Q;
	Q.append(m_root);

	while(!Q.empty()) {
		LHTreeNode *p = Q.pop();

		for(int i = 0; i < p->numberOfChildren(); ++i)
			Q.append(p->child(i));

		delete p;
	}
}


void ENGLayer::store() {
	Queue<LHTreeNode*> Q;
	Q.append(m_root);

	while(!Q.empty()) {
		LHTreeNode *p = Q.pop();

		if(p->isCompound()) {
			p->store();

			for(int i = 0; i < p->numberOfChildren(); ++i)
				Q.append(p->child(i));
		}
	}
}


void ENGLayer::restore() {
	Queue<LHTreeNode*> Q;
	Q.append(m_root);

	while(!Q.empty()) {
		LHTreeNode *p = Q.pop();

		if(p->isCompound()) {
			p->restore();

			for(int i = 0; i < p->numberOfChildren(); ++i)
				Q.append(p->child(i));
		}
	}
}


void ENGLayer::permute() {
	Queue<LHTreeNode*> Q;
	Q.append(m_root);

	while(!Q.empty()) {
		LHTreeNode *p = Q.pop();

		if(p->isCompound()) {
			p->permute();

			for(int i = 0; i < p->numberOfChildren(); ++i)
				Q.append(p->child(i));
		}
	}
}


void ENGLayer::removeAuxNodes() {
	Queue<LHTreeNode*> Q;
	Q.append(m_root);

	while(!Q.empty()) {
		LHTreeNode *p = Q.pop();

		if(p->isCompound()) {
			p->removeAuxChildren();

			for(int i = 0; i < p->numberOfChildren(); ++i)
				Q.append(p->child(i));
		}
	}
}


void ENGLayer::simplifyAdjacencies(List<LHTreeNode::Adjacency> &adjs)
{
	AdjacencyComparer cmp;

	if(!adjs.empty()) {
		adjs.quicksort(cmp);

		ListIterator<LHTreeNode::Adjacency> it = adjs.begin();
		ListIterator<LHTreeNode::Adjacency> itNext = it.succ();

		while(itNext.valid()) {
			if((*it).m_u == (*itNext).m_u && (*it).m_v == (*itNext).m_v) {
				(*it).m_weight += (*itNext).m_weight;

				adjs.del(itNext);
				itNext = it.succ();

			} else {
				it = itNext;
				++itNext;
			}
		}
	}
}


void ENGLayer::simplifyAdjacencies()
{
	Queue<LHTreeNode*> Q;
	Q.append(m_root);

	while(!Q.empty()) {
		LHTreeNode *p = Q.pop();

		simplifyAdjacencies(p->m_upperAdj);
		simplifyAdjacencies(p->m_lowerAdj);

		for(int i = 0; i < p->numberOfChildren(); ++i)
			Q.append(p->child(i));
	}
}


//---------------------------------------------------------
// ClusterGraphCopy
//---------------------------------------------------------

ClusterGraphCopy::ClusterGraphCopy()
{
	m_pCG = 0;
	m_pH  = 0;
}

ClusterGraphCopy::ClusterGraphCopy(const ExtendedNestingGraph &H, const ClusterGraph &CG)
: ClusterGraph(H), m_pCG(&CG), m_pH(&H), m_copy(CG,0)
{
	m_original.init(*this,0);
	m_copy    [CG.rootCluster()] = rootCluster();
	m_original[rootCluster()]    = CG.rootCluster();

	createClusterTree(CG.rootCluster());
}


void ClusterGraphCopy::init(const ExtendedNestingGraph &H, const ClusterGraph &CG)
{
	ClusterGraph::init(H);
	m_pCG = &CG;
	m_pH  = &H;
	m_copy    .init(CG,0);
	m_original.init(*this,0);

	m_copy    [CG.rootCluster()] = rootCluster();
	m_original[rootCluster()]    = CG.rootCluster();

	createClusterTree(CG.rootCluster());
}


void ClusterGraphCopy::createClusterTree(cluster cOrig)
{
	cluster c = m_copy[cOrig];

	ListConstIterator<cluster> itC;
	for(itC = cOrig->cBegin(); itC.valid(); ++itC) {
		cluster child = newCluster(c);
		m_copy    [*itC]  = child;
		m_original[child] = *itC;

		createClusterTree(*itC);
	}

	ListConstIterator<node> itV;
	for(itV = cOrig->nBegin(); itV.valid(); ++itV) {
		reassignNode(m_pH->copy(*itV), c);
	}
}


void ClusterGraphCopy::setParent(node v, cluster c)
{
	reassignNode(v, c);
}


//---------------------------------------------------------
// ExtendedNestingGraph
//---------------------------------------------------------

ExtendedNestingGraph::ExtendedNestingGraph(const ClusterGraph &CG) :
	m_copy(CG),
	m_topNode(CG),
	m_bottomNode(CG),
	m_copyEdge(CG),
	m_mark(CG, 0)
{
	const Graph &G = CG;

	m_origNode.init(*this, 0);
	m_type    .init(*this, ntDummy);
	m_origEdge.init(*this, 0);

	// Create nodes
	node v;
	forall_nodes(v,G) {
		node vH = newNode();
		m_copy    [v]  = vH;
		m_origNode[vH] = v;
		m_type[vH] = ntNode;
	}

	m_CGC.init(*this,CG);

	cluster c;
	forall_clusters(c, CG) {
		m_type[m_topNode   [c] = newNode()] = ntClusterTop;
		m_type[m_bottomNode[c] = newNode()] = ntClusterBottom;

		m_CGC.setParent(m_topNode   [c], m_CGC.copy(c));
		m_CGC.setParent(m_bottomNode[c], m_CGC.copy(c));
	}

	// Create edges
	forall_nodes(v,G) {
		node    vH = m_copy[v];
		cluster c  = CG.clusterOf(v);

		newEdge(m_topNode[c], vH);
		newEdge(vH, m_bottomNode[c]);
	}

	forall_clusters(c,CG) {
		if(c != CG.rootCluster()) {
			cluster u = c->parent();

			newEdge(m_topNode[u], m_topNode[c]);
			newEdge(m_bottomNode[c], m_bottomNode[u]);

			newEdge(m_topNode[c], m_bottomNode[c]);
		}
	}

	OGDF_ASSERT(isAcyclic(*this));


	// preparation for improved test for cycles
	m_aeLevel.init(*this, -1);
	int count = 0;
	assignAeLevel(CG.rootCluster(), count);
	m_aeVisited.init(*this, false);


	// Add adjacency edges
	edge e;
	forall_edges(e, G) {
		edge eH = addEdge(m_copy[e->source()], m_copy[e->target()], true);
		m_copyEdge[e].pushBack(eH);
		m_origEdge[eH] = e;
	}

	// Add additional edges between nodes and clusters to reflect adjacency hierarchy also
	// with respect to clusters
	forall_edges(e, G) {
		node u = e->source();
		node v = e->target();

		// e was reversed?
		if(m_copyEdge[e].front()->source() != m_copy[e->source()])
			swap(u,v);

		if(CG.clusterOf(u) != CG.clusterOf(v)) {
			cluster c = lca(u, v);
			cluster cTo, cFrom;

			if(m_secondPathTo == v) {
				cTo   = m_secondPath;
				cFrom = m_mark[c];
			} else {
				cFrom = m_secondPath;
				cTo   = m_mark[c];
			}

			// Transfer adjacency relationship to a relationship between clusters
			// "clusters shall be above each other"
			edge eH = 0;
			if(cFrom != c && cTo != c)
				eH = addEdge(m_bottomNode[cFrom], m_topNode[cTo]);

			// if this is not possible, try to relax it to a relationship between node and cluster
			if(eH == 0) {
				addEdge(m_copy[u], m_topNode[cTo]);
				addEdge(m_bottomNode[cFrom], m_copy[v]);
			}
		}
	}

	OGDF_ASSERT(isAcyclic(*this));

	// cleanup
	m_aeVisited.init();
	m_aeLevel.init();

	// compute ranking and proper hierarchy
	computeRanking();
	createDummyNodes();
	//createVirtualClusters();
	buildLayers();

	// assign positions on top layer
	m_pos.init(*this);
	count = 0;
	assignPos(m_layer[0].root(), count);
}

void ExtendedNestingGraph::computeRanking()
{
	// Compute ranking
	OptimalRanking ranking;
	ranking.separateMultiEdges(false);

	EdgeArray<int> length(*this);
	EdgeArray<int> cost(*this);
	edge e;
	forall_edges(e,*this) {
		NodeType typeSrc = type(e->source());
		NodeType typeTgt = type(e->target());

		if(typeSrc == ntNode && typeTgt == ntNode)
			length[e] = 2; // Node -> Node
		else if (typeSrc != ntNode && typeTgt != ntNode)
			length[e] = 2; // Cluster -> Cluster
		else
			length[e] = 1; // Node <-> Cluster

		cost[e] = (origEdge(e) != 0) ? 2 : 1;
	}

	ranking.call(*this, length, cost, m_rank);

	// adjust ranks of top / bottom node
	cluster c;
	forall_postOrderClusters(c,m_CGC)
	{
		int t = INT_MAX;
		int b = INT_MIN;

		ListConstIterator<node> itV;
		for(itV = c->nBegin(); itV.valid();  ++itV) {
			if(type(*itV) != ntNode)
				continue;

			int r = m_rank[*itV];
			if(r-1 < t)
				t = r-1;
			if(r+1 > b)
				b = r+1;
		}

		ListConstIterator<cluster> itC;
		for(itC = c->cBegin(); itC.valid(); ++itC) {
			int rb = m_rank[bottom(m_CGC.original(*itC))];
			if(rb+2 > b)
				b = rb+2;
			int rt = m_rank[top(m_CGC.original(*itC))];
			if(rt-2 < t)
				t = rt-2;
		}

		cluster cOrig = m_CGC.original(c);
		OGDF_ASSERT(m_rank[top(cOrig)] <= t && b <= m_rank[bottom(cOrig)]);

		if(t < INT_MAX) {
			m_rank[top   (cOrig)] = t;
			m_rank[bottom(cOrig)] = b;
		}
	}

	// Remove all non-adjacency edges
	edge eNext;
	for(e = firstEdge(); e != 0; e = eNext) {
		eNext = e->succ();
		if(m_origEdge[e] == 0) {
			cluster c = originalCluster(e->source());
			// we do not remove edges from top(c)->bottom(c)
			if(e->source() != top(c) || e->target() != bottom(c))
				delEdge(e);
		}
	}

	// Remove nodes for root cluster
	cluster r = getOriginalClusterGraph().rootCluster();
	int high = m_rank[m_bottomNode[r]];
	int low  = m_rank[m_topNode[r]];

	delNode(m_topNode[r]);
	delNode(m_bottomNode[r]);
	m_topNode   [r] = 0;
	m_bottomNode[r] = 0;

	// Reassign ranks
	Array<SListPure<node> > levels(low,high);

	node v;
	forall_nodes(v, *this)
		levels[m_rank[v]].pushBack(v);

	int currentRank = 0;
	for(int i = low+1; i < high; ++i)
	{
		SListPure<node> &L = levels[i];
		if(L.empty())
			continue;

		SListConstIterator<node> it;
		for(it = L.begin(); it.valid(); ++it)
			m_rank[*it] = currentRank;

		++currentRank;
	}

	m_numLayers = currentRank;
}


void ExtendedNestingGraph::createDummyNodes()
{
	const ClusterGraph &CG = getOriginalClusterGraph();
	const Graph        &G  = CG;

	edge e;
	forall_edges(e, G)
	{
		edge eH = m_copyEdge[e].front();
		node uH = eH->source();
		node vH = eH->target();

		int span = m_rank[vH] - m_rank[uH];
		OGDF_ASSERT(span >= 1);
		if(span < 2)
			continue;

		// find cluster cTop containing both u and v
		node u = m_origNode[uH];
		node v = m_origNode[vH];

		cluster cTop = lca(u,v);

		// create split nodes
		for(int i = m_rank[uH]+1; i < m_rank[vH]; ++i) {
			eH = split(eH);
			m_copyEdge[e].pushBack(eH);
			m_origEdge[eH] = e;
			m_rank[eH->source()] = i;
			// assign preliminary cTop to all dummies since this is ok
			// try to aesthetically improve this later
			m_CGC.setParent(eH->source(), m_CGC.copy(cTop));
		}

		// improve cluster assignment
		cluster c_1 = CG.clusterOf(u);
		cluster c_2 = CG.clusterOf(v);
		cluster root = CG.rootCluster();

		if(c_1 == root || c_2 == root || m_rank[m_bottomNode[c_1]] >= m_rank[m_topNode[c_2]]) {
			if(c_2 != root && m_rank[uH] < m_rank[m_topNode[c_2]])
			{
				c_1 = 0;
				while(c_2->parent() != root && m_rank[uH] < m_rank[m_topNode[c_2->parent()]])
					c_2 = c_2->parent();
			}
			else if(c_1 != root && m_rank[vH] > m_rank[m_bottomNode[c_1]])
			{
				c_2 = 0;
				while(c_1->parent() != root && m_rank[vH] > m_rank[m_bottomNode[c_1->parent()]])
					c_1 = c_1->parent();

			} else {
				continue; // leave all dummies in cTop
			}

		} else {
			bool cont;
			do {
				cont = false;

				cluster parent = c_1->parent();
				if(parent != root && m_rank[m_bottomNode[parent]] < m_rank[m_topNode[c_2]]) {
					c_1 = parent;
					cont = true;
				}

				parent = c_2->parent();
				if(parent != root && m_rank[m_bottomNode[c_1]] < m_rank[m_topNode[parent]]) {
					c_2 = parent;
					cont = true;
				}

			} while(cont);
		}

		if(c_1 != 0) {
			ListConstIterator<edge> it = m_copyEdge[e].begin();
			for(cluster c = CG.clusterOf(u); c != c_1->parent(); c = c->parent()) {
				while(m_rank[(*it)->target()] <= m_rank[m_bottomNode[c]]) {
					m_CGC.setParent((*it)->target(), m_CGC.copy(c));
					++it;
				}
			}
		}

		if(c_2 != 0) {
			ListConstIterator<edge> it = m_copyEdge[e].rbegin();
			for(cluster c = CG.clusterOf(v); c != c_2->parent(); c = c->parent()) {
				while(m_rank[(*it)->source()] >= m_rank[m_topNode[c]]) {
					m_CGC.setParent((*it)->source(), m_CGC.copy(c));
					--it;
				}
			}
		}
	}

	// create dummy nodes for edges top(c)->bottom(c)
	cluster c;
	forall_clusters(c,CG)
	{
		if(c == CG.rootCluster())
			continue;

		node vTop = top(c);
		node vBottom = bottom(c);

		forall_adj_edges(e,vTop)
			if(e->target() == vBottom) {
				int span = m_rank[vBottom] - m_rank[vTop];
				OGDF_ASSERT(span >= 1);
				if(span < 2)
					continue;

				// create split nodes
				edge eH = e;
				for(int i = m_rank[vTop]+1; i < m_rank[vBottom]; ++i) {
					eH = split(eH);
					m_rank[eH->source()] = i;
					m_type[eH->source()] = ntClusterTopBottom;
					m_CGC.setParent(eH->source(), m_CGC.copy(c));
				}
				break;
			}
	}
}


void ExtendedNestingGraph::createVirtualClusters()
{
	NodeArray   <node> vCopy(*this);
	ClusterArray<node> cCopy(m_CGC);

	createVirtualClusters(m_CGC.rootCluster(), vCopy, cCopy);

	// for each original edge, put the edge segments that are in the same cluster
	// into a separate cluster
	edge eOrig;
	forall_edges(eOrig, m_CGC.getOriginalClusterGraph().getGraph())
	{
		const List<edge> &L = m_copyEdge[eOrig];
		if(L.size() >= 3) {
			ListConstIterator<edge> it = L.begin().succ();
			node v = (*it)->source();

			cluster c = parent(v);
			SList<node> nextCluster;
			nextCluster.pushBack(v);

			for(++it; it.valid(); ++it) {
				node u = (*it)->source();
				cluster cu = parent(u);

				if(cu != c) {
					if(nextCluster.size() > 1)
						m_CGC.createCluster(nextCluster, c);

					nextCluster.clear();
					c = cu;
				}

				nextCluster.pushBack(u);
			}

			if(nextCluster.size() > 1)
				m_CGC.createCluster(nextCluster, c);
		}
	}
}


void ExtendedNestingGraph::createVirtualClusters(
	cluster c,
	NodeArray<node>    &vCopy,
	ClusterArray<node> &cCopy)
{
	if(c->cCount() >= 1 && c->nCount() >= 1)
	{
		// build auxiliaray graph G
		Graph G;

		ListConstIterator<node> itV;
		for(itV = c->nBegin(); itV.valid(); ++itV) {
			vCopy[*itV] = G.newNode();
		}

		ListConstIterator<cluster> itC;
		for(itC = c->cBegin(); itC.valid(); ++itC) {
			cCopy[*itC] = G.newNode();
		}

		for(itV = c->nBegin(); itV.valid(); ++itV) {
			node v = *itV;
			adjEntry adj;
			forall_adj(adj,v) {
				if(origEdge(adj->theEdge()) == 0)
					continue;

				node w = adj->twinNode();
				cluster cw = parent(w);
				if(cw == c)
					G.newEdge(vCopy[v],vCopy[w]);

				else if(cw->parent() == c) {
					cluster cwOrig = m_CGC.original(cw);
					OGDF_ASSERT(cwOrig != 0);
					if(rank(w) == rank(top(cwOrig)) || rank(w) == rank(bottom(cwOrig)))
						G.newEdge(vCopy[v],cCopy[cw]);
				}
			}
		}

		// find connect components in G
		NodeArray<int> component(G);
		int k = connectedComponents(G, component);

		// create virtual clusters
		if(k > 1) {
			Array<SList<node   > > nodes(k);
			Array<SList<cluster> > clusters(k);

			for(itV = c->nBegin(); itV.valid(); ++itV)
				nodes[component[vCopy[*itV]]].pushBack(*itV);

			for(itC = c->cBegin(); itC.valid(); ++itC)
				clusters[component[cCopy[*itC]]].pushBack(*itC);

			for(int i = 0; i < k; ++i) {
				if(nodes[i].size() + clusters[i].size() > 1) {
					cluster cVirt = m_CGC.createCluster(nodes[i], c);
					SListConstIterator<cluster> it;
					for(it = clusters[i].begin(); it.valid(); ++it)
						m_CGC.moveCluster(*it,cVirt);
				}
			}
		}
	}

	// recurive call
	for(ListConstIterator<cluster> it = c->cBegin(); it.valid(); ++it)
		createVirtualClusters(*it, vCopy, cCopy);
}


void ExtendedNestingGraph::buildLayers()
{
	m_layer.init(m_numLayers);

	Array<List<node> > L(m_numLayers);

	node v;
	forall_nodes(v,*this) {
		L[rank(v)].pushBack(v);
	}

	// compute minimum and maximum level of each cluster
	m_topRank.init(m_CGC,m_numLayers);
	m_bottomRank.init(m_CGC,0);
	cluster c;
	forall_postOrderClusters(c, m_CGC) {
		ListConstIterator<node> itV;
		for(itV = c->nBegin(); itV.valid(); ++itV) {
			int r = rank(*itV);
			if(r > m_bottomRank[c])
				m_bottomRank[c] = r;
			if(r < m_topRank[c])
				m_topRank[c] = r;
		}
		ListConstIterator<cluster> itC;
		for(itC = c->cBegin(); itC.valid(); ++itC) {
			if(m_topRank[*itC] < m_topRank[c])
				m_topRank[c] = m_topRank[*itC];
			if(m_bottomRank[*itC] > m_bottomRank[c])
				m_bottomRank[c] = m_bottomRank[*itC];
		}
	}

	Array<SListPure<cluster> > clusterBegin(m_numLayers);
	Array<SListPure<cluster> > clusterEnd(m_numLayers);

	forall_clusters(c,m_CGC) {
		clusterBegin[m_topRank   [c]].pushBack(c);
		clusterEnd  [m_bottomRank[c]].pushBack(c);
	}


	ClusterSetPure activeClusters(m_CGC);
	activeClusters.insert(m_CGC.rootCluster());

	ClusterArray<LHTreeNode*> clusterToTreeNode(m_CGC, 0);
	ClusterArray<int>         numChildren(m_CGC, 0);
	NodeArray<LHTreeNode*>    treeNode(*this, 0);

	int i;
	for(i = 0; i < m_numLayers; ++i) {
		// identify new clusters on this layer
		ListConstIterator<node> it;
		for(it = L[i].begin(); it.valid(); ++it) {
			++numChildren[parent(*it)];
		}

		SListConstIterator<cluster> itActive;
		for(itActive = clusterBegin[i].begin(); itActive.valid(); ++itActive)
			activeClusters.insert(*itActive);

		// create compound tree nodes
		ListConstIterator<cluster> itC;
		for(itC = activeClusters.clusters().begin(); itC.valid(); ++itC) {
			cluster c = *itC;
			clusterToTreeNode[c] = OGDF_NEW LHTreeNode(c, clusterToTreeNode[c]);
			if(c != m_CGC.rootCluster())
				++numChildren[c->parent()];
		}

		// initialize child arrays
		for(itC = activeClusters.clusters().begin(); itC.valid(); ++itC) {
			clusterToTreeNode[*itC]->initChild(numChildren[*itC]);
		}

		// set parent and children of compound tree nodes
		for(itC = activeClusters.clusters().begin(); itC.valid(); ++itC) {
			if(*itC != m_CGC.rootCluster()) {
				LHTreeNode *cNode = clusterToTreeNode[*itC];
				LHTreeNode *pNode = clusterToTreeNode[(*itC)->parent()];

				cNode->setParent(pNode);
				pNode->setChild(--numChildren[(*itC)->parent()], cNode);
			}
		}

		// set root of layer
		m_layer[i].setRoot(clusterToTreeNode[m_CGC.rootCluster()]);

		// create tree nodes for nodes on this layer
		for(it = L[i].begin(); it.valid(); ++it) {
			LHTreeNode *cNode = clusterToTreeNode[parent(*it)];
			LHTreeNode::Type type = (m_type[*it] == ntClusterTopBottom) ?
				LHTreeNode::AuxNode : LHTreeNode::Node;
			LHTreeNode *vNode =  OGDF_NEW LHTreeNode(cNode, *it, type);
			treeNode[*it] = vNode;
			cNode->setChild(--numChildren[parent(*it)], vNode);
		}

		// clean-up
		for(itC = activeClusters.clusters().begin(); itC.valid(); ++itC) {
			numChildren      [*itC] = 0;
		}

		// identify clusters that are not on next layer
		for(itActive = clusterEnd[i].begin(); itActive.valid(); ++itActive)
			activeClusters.remove(*itActive);
	}

	// identify adjacencies between nodes and tree nodes
	edge e;
	forall_edges(e,*this)
	{
		node u = e->source();
		node v = e->target();
		bool isTopBottomEdge = (origEdge(e) == 0);
		int weight = (isTopBottomEdge) ? 100 : 1;

		if(isTopBottomEdge)
			continue;

		LHTreeNode *nd = treeNode[v];
		LHTreeNode *parent = nd->parent();
		if(isTopBottomEdge) {
			nd = parent;
			parent = parent->parent();
		}

		while(parent != 0) {
			parent->m_upperAdj.pushBack(LHTreeNode::Adjacency(u,nd,weight));

			nd = parent;
			parent = parent->parent();
		}

		nd = treeNode[u];
		parent = nd->parent();
		if(isTopBottomEdge) {
			nd = parent;
			parent = parent->parent();
		}

		while(parent != 0) {
			parent->m_lowerAdj.pushBack(LHTreeNode::Adjacency(v,nd,weight));

			nd = parent;
			parent = parent->parent();
		}
	}

	for(i = 0; i < m_numLayers; ++i)
		m_layer[i].simplifyAdjacencies();


	// identify relevant pairs for crossings between top->bottom edges
	// and foreign edges
	m_markTree.init(m_CGC,0);
	ClusterArray<List<Tuple3<edge,LHTreeNode*,LHTreeNode*> > > edges(m_CGC);
	ClusterSetSimple C(m_CGC);
	for(i = 0; i < m_numLayers-1; ++i)
	{
		ListConstIterator<node> it;
		for(it = L[i].begin(); it.valid(); ++it) {
			node u = *it;

			edge e;
			forall_adj_edges(e,u) {
				if(origEdge(e) == 0)
					continue;
				if(e->source() == u) {
					node v = e->target();

					LHTreeNode *uChild, *vChild;
					cluster c = lca(treeNode[u], treeNode[v], &uChild, &vChild)->originalCluster();

					edges[c].pushBack(
						Tuple3<edge,LHTreeNode*,LHTreeNode*>(e,uChild,vChild));
					C.insert(c);
				}
			}
		}

		for(it = L[i].begin(); it.valid(); ++it) {
			node u = *it;

			edge e;
			forall_adj_edges(e,u) {
				if(e->source() == u && origEdge(e) == 0) {
					LHTreeNode *aNode = treeNode[e->target()];
					cluster ca = aNode->parent()->originalCluster();
					LHTreeNode *aParent = aNode->parent()->parent();

					for(; aParent != 0; aParent = aParent->parent()) {
						ListConstIterator<Tuple3<edge,LHTreeNode*,LHTreeNode*> > itE;
						for(itE = edges[aParent->originalCluster()].begin();
							itE.valid(); ++itE)
						{
							LHTreeNode *aChild, *vChild, *h1, *h2;
							LHTreeNode *cNode = lca(aNode, treeNode[(*itE).x1()->target()],
								&aChild, &vChild);
							if(cNode != aNode->parent() &&
								lca(aNode,treeNode[(*itE).x1()->source()],&h1,&h2)->originalCluster() != ca)
								cNode->m_upperClusterCrossing.pushBack(
									LHTreeNode::ClusterCrossing(e->source(),aChild,
									(*itE).x1()->source(),vChild,
									(*itE).x1()));
						}
					}

					aNode = treeNode[e->source()];
					ca = aNode->parent()->originalCluster();
					aParent = aNode->parent()->parent();

					for(; aParent != 0; aParent = aParent->parent()) {
						ListConstIterator<Tuple3<edge,LHTreeNode*,LHTreeNode*> > itE;
						for(itE = edges[aParent->originalCluster()].begin();
							itE.valid(); ++itE)
						{
							LHTreeNode *aChild, *vChild, *h1, *h2;
							LHTreeNode *cNode = lca(aNode, treeNode[(*itE).x1()->source()],
								&aChild, &vChild);
							if(cNode != aNode->parent() &&
								lca(aNode,treeNode[(*itE).x1()->target()],&h1,&h2)->originalCluster() != ca)
								cNode->m_lowerClusterCrossing.pushBack(
									LHTreeNode::ClusterCrossing(e->target(),aChild,
									(*itE).x1()->target(),vChild,
									(*itE).x1()));
						}
					}
				}
			}
		}

		// get rid of edges in edges[c]
		SListConstIterator<cluster> itC;
		for(itC = C.clusters().begin(); itC.valid(); ++itC)
			edges[*itC].clear();
		C.clear();
	}

	// clean-up
	m_markTree.init();
}


void ExtendedNestingGraph::storeCurrentPos()
{
	for(int i = 0; i < m_numLayers; ++i)
		m_layer[i].store();
}


void ExtendedNestingGraph::restorePos()
{
	int count;
	for(int i = 0; i < m_numLayers; ++i) {
		m_layer[i].restore();

		count = 0;
		assignPos(m_layer[i].root(), count);
	}
}


void ExtendedNestingGraph::permute()
{
	for(int i = 0; i < m_numLayers; ++i)
		m_layer[i].permute();

	int count = 0;
	assignPos(m_layer[0].root(), count);
}


RCCrossings ExtendedNestingGraph::reduceCrossings(int i, bool dirTopDown)
{
	//cout << "Layer " << i << ":\n";
	LHTreeNode *root = m_layer[i].root();

	Stack<LHTreeNode*> S;
	S.push(root);

	RCCrossings numCrossings;
	while(!S.empty()) {
		LHTreeNode *cNode = S.pop();
		numCrossings += reduceCrossings(cNode, dirTopDown);

		for(int j = 0; j < cNode->numberOfChildren(); ++j) {
			if(cNode->child(j)->isCompound())
				S.push(cNode->child(j));
		}
	}

	// set positions
	int count = 0;
	assignPos(root, count);

	return numCrossings;
}


struct RCEdge
{
	RCEdge() { }
	RCEdge(node src, node tgt, RCCrossings cr, RCCrossings crReverse) {
		m_src       = src;
		m_tgt       = tgt;
		m_cr        = cr;
		m_crReverse = crReverse;
	}

	RCCrossings weight() const { return m_crReverse - m_cr; }

	node        m_src;
	node        m_tgt;
	RCCrossings m_cr;
	RCCrossings m_crReverse;
};


class LocationRelationshipComparer
{
public:
	static int compare(const RCEdge &x, const RCEdge &y)
	{
		return RCCrossings::compare(x.weight(), y.weight());
	}
	OGDF_AUGMENT_STATICCOMPARER(RCEdge)
};


bool ExtendedNestingGraph::tryEdge(node u, node v, Graph &G, NodeArray<int> &level)
{
	const int n = G.numberOfNodes();

	if(level[u] == -1) {
		if(level[v] == -1) {
			level[v] = n;
			level[u] = n-1;
		} else
			level[u] = level[v]-1;

	} else if(level[v] == -1)
		level[v] = level[u]+1;

	else if(level[u] >= level[v]) {
		SListPure<node> successors;
		if(reachable(v, u, successors))
			return false;
		else {
			level[v] = level[u] + 1;
			moveDown(v, successors, level);
		}
	}

	G.newEdge(u,v);

	return true;
}


RCCrossings ExtendedNestingGraph::reduceCrossings(LHTreeNode *cNode, bool dirTopDown)
{
	const int n = cNode->numberOfChildren();
	if(n < 2)
		return RCCrossings(); // nothing to do

	cNode->setPos();

	// Build
	// crossings matrix
	Array2D<RCCrossings> cn(0,n-1,0,n-1);

	// crossings between adjacency edges
	Array<List<LHTreeNode::Adjacency> > adj(n);
	ListConstIterator<LHTreeNode::Adjacency> it;
	for(it = (dirTopDown) ? cNode->m_upperAdj.begin() : cNode->m_lowerAdj.begin(); it.valid(); ++it)
		adj[(*it).m_v->pos()].pushBack(*it);

	int j;
	for(j = 0; j < n; ++j) {
		ListConstIterator<LHTreeNode::Adjacency> itJ;
		for(itJ = adj[j].begin(); itJ.valid(); ++itJ) {
			int posJ = m_pos[(*itJ).m_u];

			for(int k = j+1; k < n; ++k) {
				ListConstIterator<LHTreeNode::Adjacency> itK;
				for(itK = adj[k].begin(); itK.valid(); ++itK) {
					int posK   = m_pos[(*itK).m_u];
					int weight = (*itJ).m_weight * (*itK).m_weight;

					if(posJ > posK)
						cn(j,k).incEdges(weight);
					if(posK > posJ)
						cn(k,j).incEdges(weight);
				}
			}
		}
	}

	// crossings between clusters and foreign adjacency edges
	ListConstIterator<LHTreeNode::ClusterCrossing> itCC;
	for(itCC = (dirTopDown) ?
		cNode->m_upperClusterCrossing.begin() : cNode->m_lowerClusterCrossing.begin();
		itCC.valid(); ++itCC)
	{
		/*cout << "crossing: C" << m_CGC.clusterOf((*itCC).m_edgeCluster->source()) <<
			" e=" << (*itCC).m_edgeCluster << "  with edge " << (*itCC).m_edge <<
			" cluster: C" << m_CGC.clusterOf((*itCC).m_edge->source()) <<
			", C" << m_CGC.clusterOf((*itCC).m_edge->target()) << "\n";*/

		int j = (*itCC).m_cNode->pos();
		int k = (*itCC).m_uNode->pos();

		int posJ = m_pos[(*itCC).m_uc];
		int posK = m_pos[(*itCC).m_u];

		OGDF_ASSERT(j != k && posJ != posK);

		if(posJ > posK)
			cn(j,k).incClusters();
		else
			cn(k,j).incClusters();
	}


	Graph G; // crossing reduction graph
	NodeArray<int>  level(G,-1);
	m_aeVisited.init(G,false);
	m_auxDeg.init(G,0);

	// create nodes
	NodeArray<LHTreeNode*> fromG(G);
	Array<node>            toG(n);

	for(j = 0; j < n; ++j)
		fromG[toG[j] = G.newNode()] = cNode->child(j);

	// create edges for l-r constraints
	const LHTreeNode *neighbourParent = (dirTopDown) ? cNode->up() : cNode->down();
	if(neighbourParent != 0) {
		node src = 0;
		for(int i = 0; i < neighbourParent->numberOfChildren(); ++i) {
			const LHTreeNode *vNode =
				(dirTopDown) ?
				neighbourParent->child(i)->down() : neighbourParent->child(i)->up();

			if(vNode != 0) {
				node tgt = toG[vNode->pos()];
				if(src != 0) {
#ifdef OGDF_DEBUG
					bool result =
#endif
						tryEdge(src, tgt, G, level);
					OGDF_ASSERT(result);
				}
				src = tgt;
			}
		}
	}

	// list of location relationships
	List<RCEdge> edges;
	for(j = 0; j < n; ++j)
		for(int k = j+1; k < n; ++k) {
			if(cn(j,k) <= cn(k,j))
				edges.pushBack(RCEdge(toG[j], toG[k], cn(j,k), cn(k,j)));
			else
				edges.pushBack(RCEdge(toG[k], toG[j], cn(k,j), cn(j,k)));
		}

	// sort list according to weights
	LocationRelationshipComparer cmp;
	edges.quicksort(cmp);

	// build acyclic graph
	RCCrossings numCrossings;
	ListConstIterator<RCEdge> itE;
	for(itE = edges.begin(); itE.valid(); ++itE)
	{
		node u = (*itE).m_src;
		node v = (*itE).m_tgt;

		if(tryEdge(u, v, G, level)) {
			numCrossings += (*itE).m_cr;

		} else {
			numCrossings += (*itE).m_crReverse;
		}
	}

	OGDF_ASSERT(isAcyclic(G));

	// sort nodes in G topological
	topologicalNumbering(G,level);

	// sort children of cNode according to topological numbering
	node v;
	forall_nodes(v,G)
		cNode->setChild(level[v], fromG[v]);

	//for(j = 0; j < n; ++j) {
	//	LHTreeNode *vNode = cNode->child(j);
	//}

	return numCrossings;
}

void ExtendedNestingGraph::assignPos(const LHTreeNode *vNode, int &count)
{
	if(vNode->isCompound()) {
		for(int i = 0; i < vNode->numberOfChildren(); ++i)
			assignPos(vNode->child(i), count);

	} else {
		m_pos[vNode->getNode()] = count++;
	}
}


void ExtendedNestingGraph::removeAuxNodes()
{
	for(int i = 0; i < m_numLayers; ++i)
		m_layer[i].removeAuxNodes();
}


void ExtendedNestingGraph::removeTopBottomEdges()
{
	// compute m_vertical
	m_vertical.init(*this);

	//cluster rootOrig = getOriginalClusterGraph().rootCluster();
	edge e;
	forall_edges(e,*this)
	{
		if(origEdge(e) == 0)
			continue;

		bool vert = false;
		node u = e->source();
		node v = e->target();

		// if we do not use virtual clusters, cu and cv are simply the
		// clusters containing u and v (=> no while-loop required)
		cluster cu = parent(u);
		while(isVirtual(cu))
			cu = cu->parent();
		cluster cv = parent(v);
		while(isVirtual(cv))
			cv = cv->parent();

		if(isLongEdgeDummy(u) && isLongEdgeDummy(v)) {
			if(cu != cv) {
				cluster cuOrig = m_CGC.original(cu);
				cluster cvOrig = m_CGC.original(cv);
				cluster cuOrigParent = cuOrig->parent();
				cluster cvOrigParent = cvOrig->parent();

				if((cvOrig == cuOrigParent && rank(u) == rank(bottom(cuOrig))) ||
					(cuOrig == cvOrigParent && rank(v) == rank(top   (cvOrig))) ||
					(cuOrigParent == cvOrigParent &&
					rank(u) == rank(bottom(cuOrig)) &&
					rank(v) == rank(top   (cvOrig))
					))
				{
					vert = true;
				}
			} else
				vert = true;
		}

		m_vertical[e] = vert;
	}

	for(int i = 1; i < m_numLayers; ++i)
	{
		LHTreeNode *root = m_layer[i].root();

		Stack<LHTreeNode*> S;
		S.push(root);

		while(!S.empty()) {
			LHTreeNode *cNode = S.pop();

			cNode->setPos();
			ListConstIterator<LHTreeNode::ClusterCrossing> itCC;
			for(itCC = cNode->m_upperClusterCrossing.begin(); itCC.valid(); ++itCC) {
				int j = (*itCC).m_cNode->pos();
				int k = (*itCC).m_uNode->pos();

				int posJ = m_pos[(*itCC).m_uc];
				int posK = m_pos[(*itCC).m_u];

				OGDF_ASSERT(j != k && posJ != posK);

				// do we have a cluster-edge crossing?
				if((j < k && posJ > posK) || (j > k && posJ < posK))
					m_vertical[(*itCC).m_edge] = false;
			}


			for(int j = 0; j < cNode->numberOfChildren(); ++j) {
				if(cNode->child(j)->isCompound())
					S.push(cNode->child(j));
			}
		}
	}

	// delete nodes in hierarchy tree
	removeAuxNodes();

	// delete nodes in graph
	node v, vNext;
	for(v = firstNode(); v != 0; v = vNext) {
		vNext = v->succ();
		if(type(v) == ntClusterTopBottom)
			delNode(v);
	}
}


cluster ExtendedNestingGraph::lca(node u, node v) const
{
	const ClusterGraph &CG = getOriginalClusterGraph();

	SListConstIterator<cluster> it;
	for(it = m_markedClustersTree.begin(); it.valid(); ++it)
		m_mark[*it] = 0;
	m_markedClustersTree.clear();

	cluster c1 = CG.clusterOf(u);
	cluster pred1 = c1;
	cluster c2 = CG.clusterOf(v);
	cluster pred2 = c2;

	for( ; ; ) {
		if(c1 != 0) {
			if(m_mark[c1] != 0) {
				m_secondPath = pred1;
				m_secondPathTo = u;
				return c1;

			} else {
				m_mark[c1] = pred1;
				pred1 = c1;
				m_markedClustersTree.pushBack(c1);
				c1 = c1->parent();
			}
		}
		if(c2 != 0) {
			if(m_mark[c2] != 0) {
				m_secondPath = pred2;
				m_secondPathTo = v;
				return c2;

			} else {
				m_mark[c2] = pred2;
				pred2 = c2;
				m_markedClustersTree.pushBack(c2);
				c2 = c2->parent();
			}
		}
	}
}


LHTreeNode *ExtendedNestingGraph::lca(
	LHTreeNode *uNode,
	LHTreeNode *vNode,
	LHTreeNode **uChild,
	LHTreeNode **vChild) const
{
	OGDF_ASSERT(uNode->isCompound() == false && vNode->isCompound() == false);

	SListConstIterator<cluster> it;
	for(it = m_markedClusters.begin(); it.valid(); ++it)
		m_markTree[*it] = 0;
	m_markedClusters.clear();

	LHTreeNode *cuNode = uNode->parent();
	LHTreeNode *cvNode = vNode->parent();

	LHTreeNode *uPred = uNode;
	LHTreeNode *vPred = vNode;

	while(cuNode != 0 || cvNode != 0) {
		if(cuNode != 0) {
			if(m_markTree[cuNode->originalCluster()] != 0) {
				*uChild = uPred;
				*vChild = m_markTree[cuNode->originalCluster()];
				return cuNode;

			} else {
				m_markTree[cuNode->originalCluster()] = uPred;
				uPred = cuNode;
				m_markedClusters.pushBack(cuNode->originalCluster());
				cuNode = cuNode->parent();
			}
		}
		if(cvNode != 0) {
			if(m_markTree[cvNode->originalCluster()] != 0) {
				*uChild = m_markTree[cvNode->originalCluster()];
				*vChild = vPred;
				return cvNode;

			} else {
				m_markTree[cvNode->originalCluster()] = vPred;
				vPred = cvNode;
				m_markedClusters.pushBack(cvNode->originalCluster());
				cvNode = cvNode->parent();
			}
		}
	}

	return 0; // error; not found!
}


void ExtendedNestingGraph::assignAeLevel(cluster c, int &count)
{
	m_aeLevel[m_topNode[c]] = count++;

	ListConstIterator<node> itV;
	for(itV = c->nBegin(); itV.valid(); ++itV)
		m_aeLevel[m_copy[*itV]] = count++;

	ListConstIterator<cluster> itC;
	for(itC = c->cBegin(); itC.valid(); ++itC)
		assignAeLevel(*itC, count);

	m_aeLevel[m_bottomNode[c]] = count++;
}


bool ExtendedNestingGraph::reachable(node v, node u, SListPure<node> &successors)
{
	if(u == v)
		return true;

	SListPure<node> Q;
	m_aeVisited[v] = true;
	Q.pushBack(v);

	while(!Q.empty())
	{
		node w = Q.popFrontRet();
		successors.pushBack(w);

		edge e;
		forall_adj_edges(e, w) {
			node t = e->target();

			if(t == u) {
				// we've found u, so we do not need the list of successors
				Q.conc(successors);

				// reset all visited entries
				SListConstIterator<node> it;
				for(it = Q.begin(); it.valid(); ++it)
					m_aeVisited[*it] = false;

				return true;
			}

			if(m_aeVisited[t] == false) {
				m_aeVisited[t] = true;
				Q.pushBack(t);
			}
		}
	}

	// reset all visited entries
	SListConstIterator<node> it;
	for(it = successors.begin(); it.valid(); ++it)
		m_aeVisited[*it] = false;

	return false;
}


void ExtendedNestingGraph::moveDown(node v, const SListPure<node> &successors, NodeArray<int> &level)
{
	SListConstIterator<node> it;
	for(it = successors.begin(); it.valid(); ++it) {
		m_aeVisited[*it] = true;
		m_auxDeg[*it] = 0;
	}

	for(it = successors.begin(); it.valid(); ++it) {
		edge e;
		forall_adj_edges(e,*it) {
			node s = e->source();
			if(s != *it && m_aeVisited[s])
				++m_auxDeg[*it];
		}
	}

	SListPure<node> Q;
	edge e;
	forall_adj_edges(e,v) {
		node t = e->target();
		if(t != v) {
			if( --m_auxDeg[t] == 0 )
				Q.pushBack(t);
		}
	}

	while(!Q.empty())
	{
		node w = Q.popFrontRet();

		int maxLevel = 0;
		edge e;
		forall_adj_edges(e, w) {
			node s = e->source();
			node t = e->target();

			if(s != w)
				maxLevel = max(maxLevel, level[s]);
			if(t != w) {
				if(--m_auxDeg[t] == 0)
					Q.pushBack(t);
			}
		}

		level[w] = maxLevel+1;
	}

	for(it = successors.begin(); it.valid(); ++it) {
		m_aeVisited[*it] = false;
	}
}


edge ExtendedNestingGraph::addEdge(node u, node v, bool addAlways)
{
	if(m_aeLevel[u] < m_aeLevel[v])
		return newEdge(u,v);

	SListPure<node> successors;
	if(reachable(v, u, successors) == false) {
		int d = m_aeLevel[u] - m_aeLevel[v] + 1;
		OGDF_ASSERT(d > 0);

		SListConstIterator<node> it;
		for(it = successors.begin(); it.valid(); ++it)
			m_aeLevel[*it] += d;

		return newEdge(u,v);

	} else if(addAlways)
		return newEdge(v,u);

	return 0;
}


//---------------------------------------------------------
// SugiyamaLayout
// implementations for extension with clusters
//---------------------------------------------------------

void SugiyamaLayout::call(ClusterGraphAttributes &AG)
{
//	ofstream os("C:\\temp\\sugi.txt");
	//freopen("c:\\work\\GDE\\cout.txt", "w", stdout);

	//const Graph &G = AG.constGraph();
	const ClusterGraph &CG = AG.constClusterGraph();
/*	if (G.numberOfNodes() == 0) {
		os << "Empty graph." << endl;
		return;
	}*/

	// 1. Phase: Edge Orientation and Layer Assignment

	// Build extended nesting hierarchy H
	ExtendedNestingGraph H(CG);


	/*os << "Cluster Hierarchy:\n";
	node v;
	forall_nodes(v, G)
		os << "V " << v << ": parent = " << CG.clusterOf(v)->index() << "\n";

	cluster c;
	forall_clusters(c, CG)
		if(c != CG.rootCluster())
			os << "C " << c->index() << ": parent = " << c->parent()->index() << "\n";

	os << "\nExtended Nesting Graph:\n";
	os << "  nodes: " << H.numberOfNodes() << "\n";
	os << "  edges: " << H.numberOfEdges() << "\n\n";

	forall_nodes(v, H) {
		os << v->index() << ": ";
		switch(H.type(v)) {
			case ExtendedNestingGraph::ntNode:
				os << "[V  " << H.origNode(v);
				break;
			case ExtendedNestingGraph::ntClusterTop:
				os << "[CT " << H.originalCluster(v)->index();
				break;
			case ExtendedNestingGraph::ntClusterBottom:
				os << "[CB " << H.originalCluster(v)->index();
				break;
		}
		os << ", rank = " << H.rank(v) << "]  ";

		edge e;
		forall_adj_edges(e, v)
			if(e->target() != v)
				os << e->target() << " ";
		os << "\n";
	}


	os << "\nLong Edges:\n";
	edge e;
	forall_edges(e, G) {
		os << e << ": ";
		ListConstIterator<edge> it;
		for(it = H.chain(e).begin(); it.valid(); ++it)
			os << " " << (*it);
		os << "\n";
	}

	os << "\nLevel:\n";
	int maxLevel = 0;
	forall_nodes(v,H)
		if(H.rank(v) > maxLevel)
			maxLevel = H.rank(v);
*/
	Array<List<node> > level(H.numberOfLayers());
	node v;
	forall_nodes(v,H)
		level[H.rank(v)].pushBack(v);
/*
	for(int i = 0; i <= maxLevel; ++i) {
		os << i << ": ";
		ListConstIterator<node> it;
		for(it = level[i].begin(); it.valid(); ++it) {
			node v = *it;
			switch(H.type(v)) {
				case ExtendedNestingGraph::ntNode:
					os << "(V" << H.origNode(v);
					break;
				case ExtendedNestingGraph::ntClusterTop:
					os << "(CT" << H.originalCluster(v)->index();
					break;
				case ExtendedNestingGraph::ntClusterBottom:
					os << "(CB" << H.originalCluster(v)->index();
					break;
				case ExtendedNestingGraph::ntDummy:
					os << "(D" << v;
					break;
			}

			os << ",C" << H.originalCluster(v) << ")  ";
		}
		os << "\n";
	}


	os << "\nLayer Hierarchy Trees:\n";
	for(int i = 0; i <= maxLevel; ++i) {
		os << i << ": " << H.layerHierarchyTree(i) << "\n";
	}

	os << "\nCompound Nodes Adjacencies:\n";
	for(int i = 0; i <= maxLevel; ++i) {
		os << "Layer " << i << ":\n";

		Queue<const LHTreeNode*> Q;
		Q.append(H.layerHierarchyTree(i));

		while(!Q.empty()) {
			const LHTreeNode *p = Q.pop();
			os << "  C" << p->originalCluster() << ": ";

			ListConstIterator<LHTreeNode::Adjacency> it;
			for(it = p->m_upperAdj.begin(); it.valid(); ++it) {
				node        u      = (*it).m_u;
				LHTreeNode *vNode  = (*it).m_v;
				int         weight = (*it).m_weight;

				os << " (" << u << ",";
				if(vNode->isCompound())
					os << "C" << vNode->originalCluster();
				else
					os << "N" << vNode->getNode();
				os << "," << weight << ") ";
			}
			os << "\n";

			for(int i = 0; i < p->numberOfChildren(); ++i)
				if(p->child(i)->isCompound())
					Q.append(p->child(i));
		}
	}*/

	// 2. Phase: Crossing Reduction
	reduceCrossings(H);
/*
	os << "\nLayers:\n";
	for(int i = 0; i < H.numberOfLayers(); ++i) {
		os << i << ": ";
		const List<node> &L = level[i];
		Array<node> order(0,L.size()-1,0);
		ListConstIterator<node> it;
		for(it = L.begin(); it.valid(); ++it)
			order[H.pos(*it)] = *it;

		for(int j = 0; j < L.size(); ++j)
			os << " " << order[j];
		os << "\n";
	}

	os << "\nnumber of crossings: " << m_nCrossingsCluster << "\n";
*/
	// 3. Phase: Coordinate Assignment
	H.removeTopBottomEdges();
	m_clusterLayout.get().callCluster(H, AG);
}


RCCrossings SugiyamaLayout::traverseTopDown(ExtendedNestingGraph &H)
{
	RCCrossings numCrossings;

	for(int i = 1; i < H.numberOfLayers(); ++i)
		numCrossings += H.reduceCrossings(i,true);

	return numCrossings;
}


RCCrossings SugiyamaLayout::traverseBottomUp(ExtendedNestingGraph &H)
{
	RCCrossings numCrossings;

	for(int i = H.numberOfLayers()-2; i >= 0; --i)
		numCrossings += H.reduceCrossings(i,false);

	return numCrossings;
}


void SugiyamaLayout::reduceCrossings(ExtendedNestingGraph &H)
{
	RCCrossings nCrossingsOld, nCrossingsNew;
	m_nCrossingsCluster = nCrossingsOld.setInfinity();

	for(int i = 1; ; ++i)
	{
		int nFails = m_fails+1;
		int counter = 0;

		do {
			counter++;
			// top-down traversal
			nCrossingsNew = traverseTopDown(H);

			if(nCrossingsNew < nCrossingsOld) {
				if(nCrossingsNew < m_nCrossingsCluster) {
					H.storeCurrentPos();

					if((m_nCrossingsCluster = nCrossingsNew).isZero())
						break;
				}
				nCrossingsOld = nCrossingsNew;
				nFails = m_fails+1;
			} else
				--nFails;

			// bottom-up traversal
			nCrossingsNew = traverseBottomUp(H);

			if(nCrossingsNew < nCrossingsOld) {
				if(nCrossingsNew < m_nCrossingsCluster) {
					H.storeCurrentPos();

					if((m_nCrossingsCluster = nCrossingsNew).isZero())
						break;
				}
				nCrossingsOld = nCrossingsNew;
				nFails = m_fails+1;
			} else
				--nFails;

		} while(nFails > 0);

		if(m_nCrossingsCluster.isZero() || i >= m_runs)
			break;

		H.permute();
		nCrossingsOld.setInfinity();
	}

	H.restorePos();
	m_nCrossings = m_nCrossingsCluster.m_cnEdges;
}


} // end namespace ogdf
