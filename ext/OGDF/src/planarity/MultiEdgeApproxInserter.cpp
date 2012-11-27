/*
 * $Revision: 2599 $
 *
 * last checkin:
 *   $Author: chimani $
 *   $Date: 2012-07-15 22:39:24 +0200 (So, 15. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief implements class MultiEdgeApproxInserter
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


#include <ogdf/planarity/MultiEdgeApproxInserter.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/SList.h>
#include <ogdf/basic/Queue.h>
#include <ogdf/planarity/FixedEmbeddingInserter.h>
#include <ogdf/planarity/VariableEmbeddingInserter.h>
#include <ogdf/basic/extended_graph_alg.h>


namespace ogdf {


MultiEdgeApproxInserter::PathDir MultiEdgeApproxInserter::s_oppDir[3] = { pdRight, pdLeft, pdNone };


//---------------------------------------------------------
// class EmbeddingPreference
// encodes an embedding preference
//---------------------------------------------------------

class MultiEdgeApproxInserter::EmbeddingPreference
{
public:
	enum Type { epNone, epRNode, epPNode };

	// constructs an embedding preference of type none (= irrelevant)
	EmbeddingPreference() : m_type(epNone) { }

	// constructs an embedding preference for an R-node
	EmbeddingPreference(bool mirror) : m_type(epRNode), m_mirror(mirror) { }

	EmbeddingPreference(adjEntry a1, adjEntry a2) : m_type(epPNode), m_adj1(a1), m_adj2(a2) { }
	// constructs an embedding preference for a P-node

	Type type() const { return m_type; }
	bool isNull() const { return m_type == epNone; }

	bool mirror() const { return m_mirror; }
	adjEntry adj1() const { return m_adj1; }
	adjEntry adj2() const { return m_adj2; }

	// flip embedding preference
	void flip() {
		m_mirror = !m_mirror;
		swap(m_adj1,m_adj2);
	}

	static const EmbeddingPreference s_none;

	ostream &print(ostream &os) const {
		switch(type()) {
		case MultiEdgeApproxInserter::EmbeddingPreference::epNone:
			os << "none";
			break;
		case MultiEdgeApproxInserter::EmbeddingPreference::epPNode:
			os << "PNode: " << adj1()->index() << "->" << adj2()->index();
			break;
		case MultiEdgeApproxInserter::EmbeddingPreference::epRNode:
			os << "RNode: " << mirror();
			break;
		}
		return os;
	}

private:
	Type m_type;
	bool m_mirror;  // skeleton of an R-node must be mirrored (yes/no)

	adjEntry m_adj1, m_adj2;
	// these two adj entries of first node in skeleton of a P-node must be in clockwise order adj1 -> adj2
};

const MultiEdgeApproxInserter::EmbeddingPreference MultiEdgeApproxInserter::EmbeddingPreference::s_none;



//---------------------------------------------------------
// class Block
// maintains a block in the graph
//---------------------------------------------------------

class MultiEdgeApproxInserter::Block : public Graph
{
public:
	struct SPQRPath {
		SPQRPath() : m_start(0) { }
		node       m_start;  // first node in SPQR-tree (its skeleton contains v1)
		List<edge> m_edges;  // actual path (empty if v1 and v2 are in skeleton of m_start)
		List<EmbeddingPreference> m_prefs;  // embeding preferences along the path
	};

	struct PathElement {
		PathElement() : m_node(0), m_pref(&EmbeddingPreference::s_none) { }

		node m_node;
		const EmbeddingPreference *m_pref;
	};


	// constructor
	Block() :  m_spqr(0), m_embB(0), m_dualB(0), m_faceNodeB(0), m_primalAdjB(0), m_vS(0), m_vT(0)
	{
		m_BCtoG.init(*this);
		m_cost .init(*this,1);
	}

	// destructoe
	~Block() {
		delete m_primalAdjB;
		delete m_faceNodeB;
		delete m_dualB;
		delete m_embB;
		delete m_spqr;
	}

	// returns true iff block is just a bridge (in this case, there is no SPQR-tree!)
	bool isBridge() const { return numberOfEdges() < 3; }

	int cost(edge e) const { return m_cost[e]; }

	// initialize SPQR-tree; compute allocation nodes
	void initSPQR(int m);

	// returns SPQR-tree
	const StaticPlanarSPQRTree &spqr() const { return *m_spqr; }
	StaticPlanarSPQRTree &spqr() { return *m_spqr; }

	// compute traversing costs in skeleton of n; omit skeleton edges e1, e2
	void computeTraversingCosts(node n, edge e1, edge e2);

	int findShortestPath(node n, edge eRef);

	// compute costs of a subpath through skeleton(n) while connecting s with t
	int costsSubpath(node n, edge eIn, edge eOut, node s, node t, PathDir &dirFrom, PathDir &dirTo);

	void pathToArray(int i, Array<PathElement> &path);

	bool embPrefAgree(node n, const EmbeddingPreference &p_pick, const EmbeddingPreference &p_e);

	bool switchingPair(node n, node m,
		const EmbeddingPreference &p_pick_n, const EmbeddingPreference &p_n,
		const EmbeddingPreference &p_pick_m, const EmbeddingPreference &p_m);

	int findBestFaces(node s, node t, adjEntry &adj_s, adjEntry &adj_t);
	adjEntry findBestFace(node s, node t, int &len);

	AdjEntryArray<adjEntry>     m_BCtoG;      // maps adjacency entries in block to original graph
	EdgeArray<int>              m_cost;       // costs of an edge (as given for edges in original graph)
	NodeArray<SListPure<node> > m_allocNodes; // list of allocation nodes
	Array<SPQRPath>             m_pathSPQR;   // insertion path in SPQR-tree

private:
	struct RNodeInfo {
		RNodeInfo() : m_emb(0), m_dual(0), m_faceNode(0), m_primalAdj(0) { }
		~RNodeInfo() {
			delete m_primalAdj;
			delete m_faceNode;
			delete m_dual;
			delete m_emb;
		}

		ConstCombinatorialEmbedding *m_emb;       // combinatorial embedding of skeleton graph
		Graph                       *m_dual;      // dual graph
		FaceArray<node>             *m_faceNode;  // mapping dual node -> face
		AdjEntryArray<adjEntry>     *m_primalAdj; // mapping dual adjEntry -> primal adjEntry
	};

	int recTC(node n, edge eRef);
	void constructDual(node n);

public:
	void constructDualBlock();
private:

	StaticPlanarSPQRTree *m_spqr;
	NodeArray<EdgeArray<int> > m_tc;   // traversing costs

	NodeArray<RNodeInfo> m_info;  // additional data for R-node skeletons

	ConstCombinatorialEmbedding *m_embB;
	Graph                       *m_dualB;
	FaceArray<node>             *m_faceNodeB;
	AdjEntryArray<adjEntry>     *m_primalAdjB;
	node                         m_vS, m_vT;
};


void MultiEdgeApproxInserter::Block::pathToArray(int i, Array<PathElement> &path)
{
	SPQRPath &sp = m_pathSPQR[i];

	if(sp.m_start == 0) {
		path.init();
		return;
	}

	path.init(1+sp.m_edges.size());

	ListConstIterator<edge> itE = sp.m_edges.begin();
	ListConstIterator<EmbeddingPreference> itP = sp.m_prefs.begin();

	node n = sp.m_start;

	path[0].m_node = n;
	if(m_spqr->typeOf(n) != SPQRTree::SNode)
		path[0].m_pref = &(*itP++);

	int j;
	for(j = 1; itE.valid(); ++j)
	{
		n = (*itE++)->opposite(n);
		path[j].m_node = n;

		if(m_spqr->typeOf(n) != SPQRTree::SNode)
			path[j].m_pref = &(*itP++);
	}

	OGDF_ASSERT(j == path.size());
}


bool MultiEdgeApproxInserter::Block::embPrefAgree(node n, const EmbeddingPreference &p_pick, const EmbeddingPreference &p_e)
{
	switch(m_spqr->typeOf(n)) {
	case SPQRTree::RNode:
		return p_pick.mirror() == p_e.mirror();  // check if mirroring is the same

	case SPQRTree::PNode:
		if(!p_e.isNull()) {
			// check if adj entries in (embedded) P-Node are in the right order
			return p_e.adj1()->cyclicSucc() == p_e.adj2();
		}

	default:
		return true;  // any other case "agrees"
	}
}


bool MultiEdgeApproxInserter::Block::switchingPair(
	node n, node m,
	const EmbeddingPreference &p_pick_n, const EmbeddingPreference &p_n,
	const EmbeddingPreference &p_pick_m, const EmbeddingPreference &p_m)
{
	EmbeddingPreference p_n_f = p_n;
	EmbeddingPreference p_m_f = p_m;

	p_n_f.flip();
	p_m_f.flip();

	return
		(embPrefAgree(n, p_pick_n, p_n) && embPrefAgree(m, p_pick_m, p_m_f)) ||
		(embPrefAgree(n, p_pick_n, p_n_f) && embPrefAgree(m, p_pick_m, p_m));
}


//---------------------------------------------------------
// create SPQR-tree and compute allocation nodes
//---------------------------------------------------------

void MultiEdgeApproxInserter::Block::initSPQR(int m)
{
	if(m_spqr == 0) {
		m_spqr = new StaticPlanarSPQRTree(*this,true);
		m_pathSPQR.init(m);

		const Graph &tree = m_spqr->tree();
		m_tc.init(tree);
		m_info.init(tree);

		// compute allocation nodes
		m_allocNodes.init(*this);

		node n;
		forall_nodes(n,tree)
		{
			const Skeleton &S = m_spqr->skeleton(n);
			const Graph &M = S.getGraph();

			EdgeArray<int> &tcS = m_tc[n];
			tcS.init(M,-1);

			node x;
			forall_nodes(x,M)
				m_allocNodes[S.original(x)].pushBack(n);

			edge e;
			forall_edges(e,M) {
				edge eOrig = S.realEdge(e);
				if(eOrig != 0) tcS[e] = 1;
			}
		}
	}
}


//---------------------------------------------------------
// construct dual graph of skeleton of n
//---------------------------------------------------------

void MultiEdgeApproxInserter::Block::constructDual(node n)
{
	const StaticSkeleton &S = *dynamic_cast<StaticSkeleton*>(&m_spqr->skeleton(n));
	const Graph &M = S.getGraph();

	OGDF_ASSERT(m_info[n].m_dual == 0);

	ConstCombinatorialEmbedding *emb = m_info[n].m_emb = new ConstCombinatorialEmbedding(M);
	Graph *dual = m_info[n].m_dual = new Graph;
	FaceArray<node> *faceNode = m_info[n].m_faceNode = new FaceArray<node>(*emb);
	AdjEntryArray<adjEntry> *primalAdj = m_info[n].m_primalAdj = new AdjEntryArray<adjEntry>(*dual);

	// constructs nodes (for faces in emb)
	face f;
	forall_faces(f,*emb) {
		(*faceNode)[f] = dual->newNode();
	}

	// construct dual edges (for primal edges in M)
	node v;
	forall_nodes(v,M)
	{
		adjEntry adj;
		forall_adj(adj,v)
		{
			if(adj->index() & 1) {
				node vLeft  = (*faceNode)[emb->leftFace (adj)];
				node vRight = (*faceNode)[emb->rightFace(adj)];

				edge eDual = dual->newEdge(vLeft,vRight);
				(*primalAdj)[eDual->adjSource()] = adj;
				(*primalAdj)[eDual->adjTarget()] = adj->twin();
			}
		}
	}
}


void MultiEdgeApproxInserter::Block::constructDualBlock()
{
	m_embB = new ConstCombinatorialEmbedding(*this);
	m_dualB = new Graph;
	m_faceNodeB = new FaceArray<node>(*m_embB);
	m_primalAdjB = new AdjEntryArray<adjEntry>(*m_dualB);

	// constructs nodes (for faces in m_embB)
	face f;
	forall_faces(f,*m_embB) {
		(*m_faceNodeB)[f] = m_dualB->newNode();
	}

	// construct dual edges (for primal edges in block)
	node v;
	forall_nodes(v,*this)
	{
		adjEntry adj;
		forall_adj(adj,v)
		{
			if(adj->index() & 1) {
				node vLeft  = (*m_faceNodeB)[m_embB->leftFace (adj)];
				node vRight = (*m_faceNodeB)[m_embB->rightFace(adj)];

				edge eDual = m_dualB->newEdge(vLeft,vRight);
				(*m_primalAdjB)[eDual->adjSource()] = adj;
				(*m_primalAdjB)[eDual->adjTarget()] = adj->twin();
			}
		}
	}

	m_vS = m_dualB->newNode();
	m_vT = m_dualB->newNode();
}


adjEntry MultiEdgeApproxInserter::Block::findBestFace(node s, node t, int &len)
{
	if(isBridge()) {
		len = 0;
		return s->firstAdj();
	}

	adjEntry adj_s, adj_t;
	len = findBestFaces(s,t,adj_s,adj_t);
	return adj_s;
}


int MultiEdgeApproxInserter::Block::findBestFaces(
	node s, node t, adjEntry &adj_s, adjEntry &adj_t)
{
	if(m_dualB == 0)
		constructDualBlock();

	NodeArray<adjEntry> spPred(*m_dualB, 0);
	QueuePure<adjEntry> queue;
	int oldIdCount = m_dualB->maxEdgeIndex();

	// augment dual by edges from s to all adjacent faces of s ...
	adjEntry adj;
	forall_adj(adj,s) {
		// starting edges of bfs-search are all edges leaving s
		edge eDual = m_dualB->newEdge(m_vS, (*m_faceNodeB)[m_embB->rightFace(adj)]);
		(*m_primalAdjB)[eDual->adjSource()] = adj;
		(*m_primalAdjB)[eDual->adjTarget()] = 0;
		queue.append(eDual->adjSource());
	}

	// ... and from all adjacent faces of t to t
	forall_adj(adj,t) {
		edge eDual = m_dualB->newEdge((*m_faceNodeB)[m_embB->rightFace(adj)], m_vT);
		(*m_primalAdjB)[eDual->adjSource()] = adj;
		(*m_primalAdjB)[eDual->adjTarget()] = 0;
	}

	// actual search (using bfs on directed dual)
	int len = -2;
	for( ; ;)
	{
		// next candidate edge
		adjEntry adjCand = queue.pop();
		node v = adjCand->twinNode();

		// leads to an unvisited node?
		if (spPred[v] == 0)
		{
			// yes, then we set v's predecessor in search tree
			spPred[v] = adjCand;

			// have we reached t ...
			if (v == m_vT)
			{
				// ... then search is done.
				adj_t = (*m_primalAdjB)[adjCand];

				adjEntry adj;
				do {
					adj = spPred[v];
					++len;
					v = adj->theNode();
				} while(v != m_vS);
				adj_s = (*m_primalAdjB)[adj];

				break;
			}

			// append next candidate edges to queue
			// (all edges leaving v)
			adjEntry adj;
			forall_adj(adj,v) {
				if(adj->twinNode() != m_vS)
					queue.append(adj);
			}
		}
	}

	// remove augmented edges again
	while ((adj = m_vS->firstAdj()) != 0)
		m_dualB->delEdge(adj->theEdge());

	while ((adj = m_vT->firstAdj()) != 0)
		m_dualB->delEdge(adj->theEdge());

	m_dualB->resetEdgeIdCount(oldIdCount);

	return len;
}


//---------------------------------------------------------
// compute traversing costs in skelton
//---------------------------------------------------------

int MultiEdgeApproxInserter::Block::recTC(node n, edge eRef)
{
	const StaticSkeleton &S = *dynamic_cast<StaticSkeleton*>(&m_spqr->skeleton(n));
	const Graph &M = S.getGraph();
	EdgeArray<int> &tcS = m_tc[n];

	edge e;
	forall_edges(e,M) {
		if(tcS[e] == -1 && e != eRef) {
			edge eT = S.treeEdge(e);

			node nC;
			edge eRefC;
			if(n == eT->source()) {
				nC = eT->target(); eRefC = m_spqr->skeletonEdgeTgt(eT);
			} else {
				nC = eT->source(); eRefC = m_spqr->skeletonEdgeSrc(eT);
			}

			tcS[e] = recTC(nC,eRefC);
		}
	}

	int c = 1;
	switch(m_spqr->typeOf(n))
	{
	case SPQRTree::SNode:
		c = INT_MAX;
		forall_edges(e,M)
			if(e != eRef) c = min(c, tcS[e]);
		break;

	case SPQRTree::PNode:
		c = 0;
		forall_edges(e,M)
			if(e != eRef) c += tcS[e];
		break;

	case SPQRTree::RNode:
		if(m_info[n].m_dual == 0)
			constructDual(n);

		c = findShortestPath(n, eRef);
		break;
	}

	return c;
}

void MultiEdgeApproxInserter::Block::computeTraversingCosts(node n, edge e1, edge e2)
{
	const StaticSkeleton &S = *dynamic_cast<StaticSkeleton*>(&m_spqr->skeleton(n));
	const Graph &M = S.getGraph();
	EdgeArray<int> &tcS = m_tc[n];

	edge e;
	forall_edges(e,M) {
		if(tcS[e] == -1 && e != e1 && e != e2) {
			edge eT = S.treeEdge(e);

			node nC;
			edge eRef;
			if(n == eT->source()) {
				nC = eT->target(); eRef = m_spqr->skeletonEdgeTgt(eT);
			} else {
				nC = eT->source(); eRef = m_spqr->skeletonEdgeSrc(eT);
			}

			tcS[e] = recTC(nC,eRef);
		}
	}
}


int  MultiEdgeApproxInserter::Block::findShortestPath(node n, edge eRef)
{
	const StaticSkeleton &S = *dynamic_cast<StaticSkeleton*>(&m_spqr->skeleton(n));
	const Graph &M = S.getGraph();
	const EdgeArray<int> &tcS = m_tc[n];

	const Graph &dual = *m_info[n].m_dual;
	const ConstCombinatorialEmbedding &emb = *m_info[n].m_emb;
	const FaceArray<node> faceNode = *m_info[n].m_faceNode;
	const AdjEntryArray<adjEntry> primalAdj = *m_info[n].m_primalAdj;

	int maxTC = 0;
	edge e;
	forall_edges(e,M)
		maxTC = max(maxTC, tcS[e]);

	++maxTC;
	Array<SListPure<adjEntry> > nodesAtDist(maxTC);

	NodeArray<adjEntry> spPred(dual,0); // predecessor in shortest path tree

	node vS = faceNode[emb.rightFace(eRef->adjSource())];
	node vT = faceNode[emb.rightFace(eRef->adjTarget())];

	// start with all edges leaving from vS
	adjEntry adj;
	forall_adj(adj,vS) {
		edge eOrig = primalAdj[adj]->theEdge();
		if(eOrig != eRef)
			nodesAtDist[tcS[eOrig]].pushBack(adj);
	}

	int currentDist = 0;
	for( ; ; ) {
		// next candidate edge
		while(nodesAtDist[currentDist % maxTC].empty())
			++currentDist;

		adjEntry adjCand = nodesAtDist[currentDist % maxTC].popFrontRet();
		node v = adjCand->twinNode();

		// leads to an unvisited node ?
		if (spPred[v] == 0) {
			// yes, then we set v's predecessor in search tree
			spPred[v] = adjCand;

			// have we reached t ...
			if (v == vT) {
				// ... then search is done.
				return currentDist;
			}

			// append next candidates to end of queue
			forall_adj(adj,v) {
				int listPos = (currentDist + tcS[primalAdj[adj]->theEdge()]) % maxTC;
				nodesAtDist[listPos].pushBack(adj);
			}
		}
	}
}


int MultiEdgeApproxInserter::Block::costsSubpath(node n, edge eIn, edge eOut, node s, node t, PathDir &dirFrom, PathDir &dirTo)
{
	if(m_info[n].m_dual == 0)
		constructDual(n);

	const StaticSkeleton &S = *dynamic_cast<StaticSkeleton*>(&m_spqr->skeleton(n));
	const Graph &M = S.getGraph();
	const EdgeArray<int> &tcS = m_tc[n];

	const Graph &dual = *m_info[n].m_dual;
	const ConstCombinatorialEmbedding &emb = *m_info[n].m_emb;
	const FaceArray<node> faceNode = *m_info[n].m_faceNode;
	const AdjEntryArray<adjEntry> primalAdj = *m_info[n].m_primalAdj;

	node v1 = 0, v2 = 0;
	if(eIn == 0 || eOut == 0) {
		node v;
		forall_nodes(v,M) {
			node vOrig = S.original(v);
			if(vOrig == s) v1 = v;
			if(vOrig == t) v2 = v;
		}
	}

	edge e1 = (eIn  == 0) ? 0 : ((n != eIn ->source()) ? m_spqr->skeletonEdgeTgt(eIn)  : m_spqr->skeletonEdgeSrc(eIn));
	edge e2 = (eOut == 0) ? 0 : ((n != eOut->source()) ? m_spqr->skeletonEdgeTgt(eOut) : m_spqr->skeletonEdgeSrc(eOut));

	computeTraversingCosts(n, e1, e2);

	int maxTC = 0;
	edge e;
	forall_edges(e,M)
		maxTC = max(maxTC, tcS[e]);

	++maxTC;
	Array<SListPure<Tuple2<node,node> > > nodesAtDist(maxTC);

	// start vertices
	if(e1 != 0) {
		nodesAtDist[0].pushBack( Tuple2<node,node>(faceNode[emb.rightFace(e1->adjSource())], 0) );
		nodesAtDist[0].pushBack( Tuple2<node,node>(faceNode[emb.rightFace(e1->adjTarget())], 0) );
	} else {
		adjEntry adj;
		forall_adj(adj,v1)
			nodesAtDist[0].pushBack( Tuple2<node,node>(faceNode[emb.rightFace(adj)], 0) );
	}

	// stop vertices
	NodeArray<bool> stopVertex(dual,false);

	if(e2 != 0) {
		stopVertex[faceNode[emb.rightFace(e2->adjSource())]] = true;
		stopVertex[faceNode[emb.rightFace(e2->adjTarget())]] = true;
	} else {
		adjEntry adj;
		forall_adj(adj,v2)
			stopVertex[faceNode[emb.rightFace(adj)]] = true;
	}

	// actual shortest path search
	NodeArray<bool> visited(dual,false);
	NodeArray<node> spPred(dual,0);

	int currentDist = 0;
	for( ; ; ) {
		// next candidate
		while(nodesAtDist[currentDist % maxTC].empty())
			++currentDist;

		Tuple2<node,node> pair = nodesAtDist[currentDist % maxTC].popFrontRet();
		node v = pair.x1();

		// leads to an unvisited node ?
		if (visited[v] == false) {
			visited[v] = true;
			spPred[v] = pair.x2();

			// have we reached t ...
			if (stopVertex[v]) {
				// ... then search is done.

				// find start node w
				node w = v;
				while(spPred[w] != 0)
					w = spPred[w];

				// in which direction to we start
				if(e1 == 0)
					dirFrom = pdNone;  // doesn't matter
				else if(w == faceNode[emb.rightFace(e1->adjSource())])
					dirFrom = pdRight; // right face of eIn
				else
					dirFrom = pdLeft;  // left face of eIn

				// from which direction to we enter
				if(e2 == 0)
					dirTo = pdNone;  // doesn't matter
				else if(v == faceNode[emb.rightFace(e2->adjSource())])
					dirTo = pdLeft; // right face of eOut (leaving to the right)
				else
					dirTo = pdRight; // left face of eOut (leaving to the left)

				return currentDist;
			}

			// append next candidates to end of queue
			adjEntry adj;
			forall_adj(adj,v) {
				edge eM = primalAdj[adj]->theEdge();
				if(eM != e1 && eM != e2) {
					int listPos = (currentDist + tcS[eM]) % maxTC;
					nodesAtDist[listPos].pushBack(
						Tuple2<node,node>(adj->twinNode(), v) );
				}
			}
		}
	}
}


//---------------------------------------------------------
// constructor
// sets default values for options
//---------------------------------------------------------

MultiEdgeApproxInserter::MultiEdgeApproxInserter()
{
	m_rrOptionFix = rrNone;
	m_rrOptionVar = rrNone;
	m_percentMostCrossedFix = 25;
	m_percentMostCrossedVar = 25;
	m_statistics = false;
}


//---------------------------------------------------------
// construct graph of block i
// store mapping of original nodes to copies in blocks
//---------------------------------------------------------

MultiEdgeApproxInserter::Block *MultiEdgeApproxInserter::constructBlock(int i)
{
	Block *b = new Block;
	SList<node> nodesG;

	SListConstIterator<edge> itE;
	for(itE = m_edgesB[i].begin(); itE.valid(); ++itE)
	{
		edge e = *itE;

		if (m_GtoBC[e->source()] == 0) {
			m_GtoBC[e->source()] = b->newNode();
			nodesG.pushBack(e->source());
		}
		if (m_GtoBC[e->target()] == 0) {
			m_GtoBC[e->target()] = b->newNode();
			nodesG.pushBack(e->target());
		}

		edge eBC = b->newEdge(m_GtoBC[e->source()],m_GtoBC[e->target()]);
		b->m_BCtoG[eBC->adjSource()] = e->adjSource();
		b->m_BCtoG[eBC->adjTarget()] = e->adjTarget();

		edge eOrig = m_pPG->original(e);
		if(m_costOrig != 0)
			b->m_cost[eBC] = (eOrig == 0) ? 0 : (*m_costOrig)[eOrig];
	}

	// store mapping orginal nodes -> copy in blocks and reset entries of GtoBC
	SListConstIterator<node> itV;
	for(itV = nodesG.begin(); itV.valid(); ++itV) {
		node v = *itV;
		m_copyInBlocks[v].pushBack(VertexBlock(m_GtoBC[v],i));
		m_GtoBC[v] = 0;
	}

	planarEmbed(*b);

	return b;
}


//---------------------------------------------------------
// returns copy of node vOrig in block b
//---------------------------------------------------------

node MultiEdgeApproxInserter::copy(node vOrig, int b)
{
	SListConstIterator<VertexBlock> it;
	for(it = m_copyInBlocks[vOrig].begin(); it.valid(); ++it) {
		if((*it).m_block == b)
			return (*it).m_vertex;
	}

	return 0;
}


//---------------------------------------------------------
// DFS-traversal for computing insertion path in SPQR-tree
//---------------------------------------------------------

bool MultiEdgeApproxInserter::dfsPathSPQR(node v, node v2, edge eParent, List<edge> &path)
{
	if(v == v2)
		return true;

	edge e;
	forall_adj_edges(e,v) {
		if(e == eParent) continue;

		if(dfsPathSPQR(e->opposite(v),v2,e,path) == true) {
			path.pushFront(e);
			return true;
		}
	}

	return false;
}


//---------------------------------------------------------
// compute insertion path of edge edge k in SPQR-tree of block b
// from node vOrig to node wOrig
//---------------------------------------------------------

int MultiEdgeApproxInserter::computePathSPQR(int b, node vOrig, node wOrig, int k)
{
	Block &B = *m_block[b];

	node v = copy(vOrig,b);
	node w = copy(wOrig,b);
	OGDF_ASSERT(v != 0 && w != 0);

	B.initSPQR(m_edge.size());

	const SListPure<node> &vAllocNodes = B.m_allocNodes[v];
	const SListPure<node> &wAllocNodes = B.m_allocNodes[w];
	List<edge> &path = B.m_pathSPQR[k].m_edges;

	node v1 = vAllocNodes.front();
	node v2 = wAllocNodes.front();

	dfsPathSPQR( v1, v2, 0, path );

	node x;
	while(!path.empty() && vAllocNodes.search(x = path.front()->opposite(v1)) >= 0) {
		v1 = x;
		path.popFront();
	}

	B.m_pathSPQR[k].m_start = v1;

	while(!path.empty() && wAllocNodes.search(x = path.back()->opposite(v2)) >= 0) {
		v2 = x;
		path.popBack();
	}

	// compute insertion costs
	const StaticPlanarSPQRTree &spqr = B.spqr();
	List<EmbeddingPreference> &prefs = B.m_pathSPQR[k].m_prefs;

	PathDir dirFrom, dirTo;
	PathDir curDir = pdRight;
	int c = 0;

	switch(spqr.typeOf(v1)) {
	case SPQRTree::RNode:
		c += B.costsSubpath(v1, 0, (path.empty()) ? 0 : path.front(), v, w, dirFrom, dirTo);
		prefs.pushBack(EmbeddingPreference(false));
		curDir = dirTo;
		break;

	case SPQRTree::PNode:
		// in this case, the insertion path is a single P-node and we have no preference
		prefs.pushBack(EmbeddingPreference());
		break;

	case SPQRTree::SNode:
		break; // nothing to do
	}

	node n = v1;
	ListConstIterator<edge> it;
	for(it = path.begin(); it.valid(); ++it)
	{
		edge e = *it;
		n = e->opposite(n);

		switch(spqr.typeOf(n)) {
		case SPQRTree::RNode:
			c += B.costsSubpath(n, e,
				(it.succ().valid() == false) ? 0 : *(it.succ()), v, w,
				dirFrom, dirTo);

			// do we need to mirror embedding of R-node?
			if(dirFrom == curDir) {
				curDir = dirTo;
				prefs.pushBack(EmbeddingPreference(false));  // no
			} else {
				curDir = s_oppDir[dirTo];
				prefs.pushBack(EmbeddingPreference(true));   // yes
			}
			break;

		case SPQRTree::PNode:
			{
			edge eIn = e;
			edge eOut = *(it.succ());

			edge e1 = (n != eIn ->source()) ? spqr.skeletonEdgeTgt(eIn)  : spqr.skeletonEdgeSrc(eIn);
			edge e2 = (n != eOut->source()) ? spqr.skeletonEdgeTgt(eOut) : spqr.skeletonEdgeSrc(eOut);

			const Graph &M = spqr.skeleton(n).getGraph();
			node v1 = M.firstNode();

			adjEntry a1 = (e1->source() == v1) ? e1->adjSource() : e1->adjTarget();
			adjEntry a2 = (e2->source() == v1) ? e2->adjSource() : e2->adjTarget();

			bool srcFits = (e1->source() == v1);
			if( (curDir == pdLeft && srcFits) || (curDir == pdRight && !srcFits) )
				swap(a1,a2);
			prefs.pushBack(EmbeddingPreference(a1,a2));

			if(e1->source() != e2->source())
				curDir = s_oppDir[curDir];
			}
			break;

		case SPQRTree::SNode:
			if(it.succ().valid()) {
				edge eIn = e;
				edge eOut = *(it.succ());

				edge e1 = (n != eIn ->source()) ? spqr.skeletonEdgeTgt(eIn)  : spqr.skeletonEdgeSrc(eIn);
				edge e2 = (n != eOut->source()) ? spqr.skeletonEdgeTgt(eOut) : spqr.skeletonEdgeSrc(eOut);

				adjEntry at = e1->adjSource();
				while(at->theEdge() != e2)
					at = at->twin()->cyclicSucc();

				if(at == e2->adjSource())
					curDir = s_oppDir[curDir];
			}
			break; // nothing to do
		}
	}

	return c;
}


//---------------------------------------------------------
// DFS-traversal for computing insertion path in BC-tree
// (case vertex v)
//---------------------------------------------------------

bool MultiEdgeApproxInserter::dfsPathVertex(node v, int parent, int k, node t)
{
	if(v == t) return true;

	SListConstIterator<int> it;
	for(it = m_compV[v].begin(); it.valid(); ++it) {
		if(*it == parent) continue;
		if(dfsPathBlock(*it,v,k,t)) return true;
	}
	return false;
}


//---------------------------------------------------------
// DFS-traversal for computing insertion path in BC-tree
// (case block b)
//---------------------------------------------------------

bool MultiEdgeApproxInserter::dfsPathBlock(int b, node parent, int k, node t)
{
	SListConstIterator<node> it;
	for(it = m_verticesB[b].begin(); it.valid(); ++it)
	{
		node c = *it;
		if(c == parent) continue;
		if(dfsPathVertex(c,b,k,t)) {
			m_pathBCs[k].pushFront(VertexBlock(parent,b));

			if(!m_block[b]->isBridge())
			{
				// find path from parent to c in block b and compute embedding preferences
				m_insertionCosts[k] += computePathSPQR(b, parent, c, k);
			}

			return true;
		}
	}
	return false;
}


//---------------------------------------------------------
// compute insertion path of edge edge k in BC-tree
//---------------------------------------------------------

void MultiEdgeApproxInserter::computePathBC(int k)
{
	node s = m_pPG->copy(m_edge[k]->source());
	node t = m_pPG->copy(m_edge[k]->target());

	bool found = dfsPathVertex(s, -1, k, t);
	if(!found) cout << "Could not find path in BC-tree!" << endl;
}


//---------------------------------------------------------
// Embeds block b according to combined embedding
// preference s of the k insertion paths
//---------------------------------------------------------

void MultiEdgeApproxInserter::embedBlock(int b, int m)
{
	// Algorithm 3.7.

	Block &B = *m_block[b];
	if(B.isBridge())
		return;
	StaticPlanarSPQRTree &spqr = B.spqr();

	// A.  ---------------------------------------
	NodeArray<EmbeddingPreference> pi_pick(spqr.tree());
	NodeArray<bool> visited(spqr.tree(), false);  // mark nodes with embedding preferences
	NodeArray<bool> markFlipped(spqr.tree(), false);  // mark nodes on current path to get flipped
	Array<Block::PathElement> p;

	// B.  ---------------------------------------
	for(int i = 0; i < m; ++i)
	{
		// (a)
		B.pathToArray(i, p);
		const int len = p.size();
		if(len == 0) continue;  // this path does not go through B

		int j = 0;
		do {
			node n = p[j].m_node;

			// (b)  ---------------------------------------
			bool newlySet = false;
			if(pi_pick[n].isNull()) {
				newlySet = true;
				pi_pick[n] = *p[j].m_pref;

				if(spqr.typeOf(n) == SPQRTree::PNode) {
					adjEntry a1 = pi_pick[n].adj1();
					adjEntry a2 = pi_pick[n].adj2();
					adjEntry adj = a1->cyclicSucc();

					if(adj != a2)
						spqr.swap(n,adj,a2);

					OGDF_ASSERT(a1->cyclicSucc() == a2);
				}
			}

			// (c)  ---------------------------------------

			// determine non-S-node predecessor of n
			int j_mu = -1;
			if(j > 0) {
				node x = p[j-1].m_node;
				if(spqr.typeOf(x) != SPQRTree::SNode)
					j_mu = j-1;
				else if(j > 1)
					j_mu = j-2;
			}

			if(j_mu >= 0)
			{
				node mu = p[j_mu].m_node;

				// do we have a switching pair (mu,n)?
				if(B.switchingPair(mu, n, pi_pick[mu], *p[j_mu].m_pref, pi_pick[n], *p[j].m_pref))
					markFlipped[mu] = true;  // flip embedding at mu
			}

			// (d)  ---------------------------------------
			if(!newlySet) {
				// skip nodes in same embedding partition
				while(j+1 < len && visited[p[j+1].m_node])
					++j;
			}

			// next node
			if(++j >= len)
				break; // end of insertion path

			// skip S-node (can only be one)
			if(spqr.typeOf(p[j].m_node) == SPQRTree::SNode)
				++j;

		} while(j < len);

		// flip embedding preferences
		bool flipping = false;
		for(int j = len-1; j >= 0; --j)
		{
			node n = p[j].m_node;
			if(markFlipped[n]) {
				flipping = !flipping;
				markFlipped[n] = false;
			}

			if(flipping) {
				pi_pick[n].flip();
				if(pi_pick[n].type() == EmbeddingPreference::epPNode)
					spqr.reverse(n);

				if(visited[n]) {
					node n_succ = (j+1 < len) ? p[j+1].m_node : 0;
					node n_pred = (j-1 >=  0) ? p[j-1].m_node : 0;

					adjEntry adj;
					forall_adj(adj,n) {
						node x = adj->twinNode();
						if(x != n_succ && x != n_pred && visited[x])
							recFlipPref(adj->twin(), pi_pick, visited, spqr);
					}
				}
			}

			visited[n] = true;
		}
	}

	// C.  ---------------------------------------
	node n;
	forall_nodes(n,spqr.tree())
	{
		const EmbeddingPreference &p = pi_pick[n];
		if(p.isNull())
			continue;

		switch(spqr.typeOf(n)) {
		case SPQRTree::SNode:
			break;  // nothing to do

		case SPQRTree::PNode:
			break;  // already embedded as desired

		case SPQRTree::RNode:
			if(p.mirror())
				spqr.reverse(n);
			break;
		}
	}

	spqr.embed(B);
}


void MultiEdgeApproxInserter::recFlipPref(
	adjEntry adjP,
	NodeArray<EmbeddingPreference> &pi_pick,
	const NodeArray<bool> &visited,
	StaticPlanarSPQRTree &spqr)
{
	node n = adjP->theNode();

	EmbeddingPreference &pref = pi_pick[n];
	pref.flip();
	if(pref.type() == EmbeddingPreference::epPNode)
		spqr.reverse(n);

	adjEntry adj;
	forall_adj(adj,n) {
		if(adj != adjP && visited[adj->twinNode()])
			recFlipPref(adj->twin(), pi_pick, visited, spqr);
	}
}


//---------------------------------------------------------
// actual call method
// currently not supported:
//   frobidCrossingGens, forbiddenEdgeOrig, edgeSubGraph
//---------------------------------------------------------

const char *strType[] = { "S", "P", "R" };
//#define MEAI_OUTPUT

struct CutvertexPreference {
	CutvertexPreference(node v1, int b1, node v2, int b2)
		: m_v1(v1), m_v2(v2), m_b1(b1), m_b2(b2), m_len1(-1), m_len2(-1) { }

	node m_v1, m_v2;
	int  m_b1, m_b2;
	int m_len1, m_len2;
};

void appendToList(SListPure<adjEntry> &adjList, adjEntry adj1,
	const AdjEntryArray<adjEntry> &BCtoG,
	AdjEntryArray<SListIterator<adjEntry> > &pos)
{
	adjEntry adj = adj1;
	do {
		adj = adj->cyclicSucc();
		adjEntry adjG = BCtoG[adj];
		pos[adjG] = adjList.pushBack(adjG);
	} while(adj != adj1);
}

void insertAfterList(SListPure<adjEntry> &adjList, SListIterator<adjEntry> itBefore, adjEntry adj1,
	const AdjEntryArray<adjEntry> &BCtoG,
	AdjEntryArray<SListIterator<adjEntry> > &pos)
{
	adjEntry adj = adj1;
	do {
		adj = adj->cyclicSucc();
		adjEntry adjG = BCtoG[adj];
		itBefore = pos[adjG] = adjList.insertAfter(adjG, itBefore);
	} while(adj != adj1);
}

Module::ReturnType MultiEdgeApproxInserter::doCall(
	PlanRep &PG,
	const List<edge> &origEdges,
	bool forbidCrossingGens,
	const EdgeArray<int> *costOrig,
	const EdgeArray<bool> *forbiddenEdgeOrig,
	const EdgeArray<unsigned int> *edgeSubGraph)
{
	m_pPG = &PG;
	m_costOrig = costOrig;
	const int m = origEdges.size();

	m_edge.init(m);
	m_pathBCs.init(m);
	m_insertionCosts.init(0,m-1,0);

	int i = 0;
	for(ListConstIterator<edge> it = origEdges.begin(); it.valid(); ++it)
		m_edge[i++] = *it;

	//
	// PHASE 1: Fix a good embedding
	//

	// compute biconnected components of PG
	EdgeArray<int> compnum(PG);
	int c = biconnectedComponents(PG,compnum);

	m_compV.init(PG);
	m_verticesB.init(c);

	// edgesB[i] = list of edges in component i
	m_edgesB.init(c);
	edge e;
	forall_edges(e,PG)
		m_edgesB[compnum[e]].pushBack(e);

	// construct arrays compV and nodeB such that
	// m_compV[v] = list of components containing v
	// m_verticesB[i] = list of vertices in component i
	NodeArray<bool> mark(PG,false);

	for(i = 0; i < c; ++i) {
		SListConstIterator<edge> itEdge;
		for(itEdge = m_edgesB[i].begin(); itEdge.valid(); ++itEdge)
		{
			edge e = *itEdge;

			if (!mark[e->source()]) {
				mark[e->source()] = true;
				m_verticesB[i].pushBack(e->source());
			}
			if (!mark[e->target()]) {
				mark[e->target()] = true;
				m_verticesB[i].pushBack(e->target());
			}
		}

		SListConstIterator<node> itNode;
		for(itNode = m_verticesB[i].begin(); itNode.valid(); ++itNode)
		{
			node v = *itNode;
			m_compV[v].pushBack(i);
			mark[v] = false;
		}
	}
	mark.init();
	m_GtoBC.init(PG,0);
	m_copyInBlocks.init(PG);

	m_block.init(c);
	for(int i = 0; i < c; ++i)
		m_block[i] = constructBlock(i);

	m_sumInsertionCosts = 0;
	for(i = 0; i < m; ++i) {
		computePathBC(i);
		m_sumInsertionCosts += m_insertionCosts[i];

#ifdef MEAI_OUTPUT
		cout << "(" << PG.copy(m_edge[i]->source()) << "," << PG.copy(m_edge[i]->target()) << ")  c = " << m_insertionCosts[i] << ":\n";
		ListConstIterator<VertexBlock> it;
		for(it = m_pathBCs[i].begin(); it.valid(); ++it) {
			int b = (*it).m_block;
			const StaticSPQRTree &spqr = m_block[b]->spqr();

			cout << "   [ " << "V_" << (*it).m_vertex << ", B_" << b << " ]\n";
			cout << "      ";
			if(m_block[b]->isBridge()) {
				cout << "BRIDGE";
			} else {
				node x = m_block[b]->m_pathSPQR[i].m_start;
				SPQRTree::NodeType t = spqr.typeOf(x);
				cout << strType[t] << "_" << x;
				ListConstIterator<EmbeddingPreference> itP = m_block[b]->m_pathSPQR[i].m_prefs.begin();
				if(t == SPQRTree::RNode) {
					if((*itP).type() == EmbeddingPreference::epNone)
						cout << " (NONE)";
					else if((*itP).mirror())
						cout << " (MIRROR)";
					else
						cout << " (KEEP)";
					++itP;
				} else if(t == SPQRTree::PNode) {
					if((*itP).type() == EmbeddingPreference::epNone)
						cout << " (NONE)";
					else
						cout << "(ADJ:" << (*itP).adj1()->index() << ";" << (*itP).adj2()->index() << ")";
					++itP;
				}

				ListConstIterator<edge> itE = m_block[b]->m_pathSPQR[i].m_edges.begin();
				for(; itE.valid(); ++itE)
				{
					node y = (*itE)->opposite(x);
					SPQRTree::NodeType t = spqr.typeOf(y);
					cout << " -> " << strType[t] << "_" << y;

					if(t == SPQRTree::RNode) {
						if((*itP).type() == EmbeddingPreference::epNone)
							cout << " (NONE)";
						else if((*itP).mirror())
							cout << " (MIRROR)";
						else
							cout << " (KEEP)";
						++itP;
					} else if(t == SPQRTree::PNode) {
						if((*itP).type() == EmbeddingPreference::epNone)
							cout << " (NONE)";
						else
							cout << "(ADJ:" << (*itP).adj1()->index() << ";" << (*itP).adj2()->index() << ")";
						++itP;
					}

					x = y;
				}
			}
			cout << endl;
		}
#endif
	}

	// embed blocks
	for(int i = 0; i < c; ++i)
		embedBlock(i,m);


	// find embedding preferences at cutvertices
	NodeArray<SList<CutvertexPreference> > cvPref(PG);

	for(i = 0; i < m; ++i)
	{
		const List<VertexBlock> &L =  m_pathBCs[i];
		if(L.size() < 2)
			continue;

		ListConstIterator<VertexBlock> it = L.begin();
		node last_v = (*it).m_vertex;
		int  last_b = (*it).m_block;
		++it;
		do {
			node v2 = (it.succ().valid()) ? (*it.succ()).m_vertex : PG.copy(m_edge[i]->target());
			cvPref[(*it).m_vertex].pushBack(CutvertexPreference(last_v,last_b,v2,(*it).m_block));

			last_v = (*it).m_vertex;
			last_b = (*it).m_block;
			++it;
		} while(it.valid());
	}


	// embedding graph
	Array<bool> blockSet(0,c-1,false);
	AdjEntryArray<SListIterator<adjEntry> > pos(PG,0);

	node v;
	forall_nodes(v,PG)
	{
		SListPure<adjEntry> adjList;
		SList<VertexBlock> &copies = m_copyInBlocks[v];

		// not a cut vertex?
		if(copies.size() == 1) {
			OGDF_ASSERT(cvPref[v].empty());
			const Block &B = *m_block[copies.front().m_block];
			node vB = copies.front().m_vertex;

			adjEntry adj;
			forall_adj(adj,vB)
				adjList.pushBack(B.m_BCtoG[adj]);

		} else {
			SList<CutvertexPreference> &prefs = cvPref[v];

			SListIterator<CutvertexPreference> it;
			if(!prefs.empty()) {
				// always realize first cutvertex preference
				it = prefs.begin();

				int b1 = (*it).m_b1;
				int b2 = (*it).m_b2;
				blockSet[b1] = blockSet[b2] = true;

				adjEntry adj1 = m_block[b1]->findBestFace(copy(v,b1),copy((*it).m_v1,b1), (*it).m_len1);
				appendToList(adjList, adj1, m_block[b1]->m_BCtoG, pos);

				adjEntry adj2 = m_block[b2]->findBestFace(copy(v,b2),copy((*it).m_v2,b2), (*it).m_len2);
				appendToList(adjList, adj2, m_block[b2]->m_BCtoG, pos);

				for(++it; it.valid(); ++it) {
					b1 = (*it).m_b1;
					b2 = (*it).m_b2;

					if(!blockSet[b1] && !blockSet[b2]) {
						// none of the two blocks set yet
						adjEntry adj1 = m_block[b1]->findBestFace(copy(v,b1),copy((*it).m_v1,b1), (*it).m_len1);
						appendToList(adjList, adj1, m_block[b1]->m_BCtoG, pos);

						adjEntry adj2 = m_block[b2]->findBestFace(copy(v,b2),copy((*it).m_v2,b2), (*it).m_len2);
						appendToList(adjList, adj2, m_block[b2]->m_BCtoG, pos);

						blockSet[b1] = blockSet[b2] = true;

					} else if(!blockSet[b1]) {
						// b2 is set, but b1 is not yet set
						adjEntry adj1 = m_block[b1]->findBestFace(copy(v,b1),copy((*it).m_v1,b1), (*it).m_len1);
						adjEntry adj2 = m_block[b2]->findBestFace(copy(v,b2),copy((*it).m_v2,b2), (*it).m_len2);

						insertAfterList(adjList, pos[m_block[b2]->m_BCtoG[adj2]], adj1, m_block[b1]->m_BCtoG, pos);
						blockSet[b1] = true;

					} else if(!blockSet[b2]) {
						// b1 is set, but b2 is not yet set
						adjEntry adj1 = m_block[b1]->findBestFace(copy(v,b1),copy((*it).m_v1,b1), (*it).m_len1);
						adjEntry adj2 = m_block[b2]->findBestFace(copy(v,b2),copy((*it).m_v2,b2), (*it).m_len2);

						insertAfterList(adjList, pos[m_block[b1]->m_BCtoG[adj1]], adj2, m_block[b2]->m_BCtoG, pos);
						blockSet[b2] = true;
					}
				}
			}

			// embed remaining blocks (if any)
			SListConstIterator<VertexBlock> itVB;
			for(itVB = copies.begin(); itVB.valid(); ++itVB) {
				int b = (*itVB).m_block;
				if(blockSet[b])
					continue;
				appendToList(adjList, (*itVB).m_vertex->firstAdj(), m_block[b]->m_BCtoG, pos);
			}

			// cleanup
			for(it = prefs.begin(); it.valid(); ++it) {
				blockSet[(*it).m_b1] = false;
				blockSet[(*it).m_b2] = false;
			}

			OGDF_ASSERT(adjList.size() == v->degree());
		}

		PG.sort(v, adjList);
	}

	OGDF_ASSERT(PG.representsCombEmbedding());

	// arbitrary embedding for testing
	//planarEmbed(PG);

	// generate further statistic information
	if(m_statistics) {
		constructDual(PG);

		//cout << "\ncutvertex preferences:\n" << endl;
		//forall_nodes(v,PG)
		//{
		//	SListConstIterator<CutvertexPreference> it = cvPref[v].begin();
		//	if(!it.valid()) continue;

		//	cout << v << ":\n";
		//	for(; it.valid(); ++it) {
		//		const CutvertexPreference &p = *it;

		//		int sp1 = findShortestPath(p.m_v1, v);
		//		int sp2 = findShortestPath(v, p.m_v2);
		//		cout << "   ( v" << p.m_v1 << ", b" << p.m_b1 << "; v" << p.m_v2 << ", b" << p.m_b2 << " )  ";
		//		cout << "[ " << p.m_len1 << " / " << sp1 << " ; " << p.m_len2 << " / " << sp2 << " ]" << endl;
		//	}
		//}

		m_sumFEInsertionCosts = 0;
		for(i = 0; i < m; ++i) {
			node s = PG.copy(m_edge[i]->source());
			node t = PG.copy(m_edge[i]->target());
			int len = findShortestPath(s,t);
			m_sumFEInsertionCosts += len;
		}
	} else
		m_sumFEInsertionCosts = -1;


	// release no longer needed resources

	cleanup();


	//
	// PHASE 2: Perform edge insertion with fixed emebdding
	//

	FixedEmbeddingInserter fei;
	fei.keepEmbedding(true);
	fei.removeReinsert(m_rrOptionFix);
	fei.percentMostCrossed(m_percentMostCrossedFix);

	fei.call( PG, origEdges );

	if(m_rrOptionVar != rrNone && m_rrOptionVar != rrIncremental) {
		VariableEmbeddingInserter vei;
		vei.removeReinsert(m_rrOptionVar);
		vei.percentMostCrossed(m_percentMostCrossedFix);
		vei.callPostprocessing( PG, origEdges );
	}


	return retFeasible;
}


//---------------------------------------------------------
// cleanup: delete blocks, reset all auxiliary arrays
//---------------------------------------------------------

void MultiEdgeApproxInserter::cleanup()
{
	int c = m_block.size();
	for(int  i = 0; i < c; ++i)
		delete m_block[i];
	m_block.init();

	m_GtoBC.init();
	m_edgesB.init();
	m_verticesB.init();
	m_compV.init();

	m_edge.init();
	m_pathBCs.init();
	m_insertionCosts.init();
	m_copyInBlocks.init();

	m_primalAdj.init();
	m_faceNode.init();
	m_E.init();
	m_dual.clear();
}


//---------------------------------------------------------
// just for testing and additional statistics
//---------------------------------------------------------

void MultiEdgeApproxInserter::constructDual(const PlanRep &PG)
{
	m_E.init(PG);
	m_faceNode.init(m_E);
	m_primalAdj.init(m_dual);

	// constructs nodes (for faces in m_embB)
	face f;
	forall_faces(f,m_E) {
		m_faceNode[f] = m_dual.newNode();
	}

	// construct dual edges (for primal edges in block)
	node v;
	forall_nodes(v,PG)
	{
		adjEntry adj;
		forall_adj(adj,v)
		{
			if(adj->index() & 1) {
				node vLeft  = m_faceNode[m_E.leftFace (adj)];
				node vRight = m_faceNode[m_E.rightFace(adj)];

				edge eDual = m_dual.newEdge(vLeft,vRight);
				m_primalAdj[eDual->adjSource()] = adj;
				m_primalAdj[eDual->adjTarget()] = adj->twin();
			}
		}
	}

	m_vS = m_dual.newNode();
	m_vT = m_dual.newNode();
}


int MultiEdgeApproxInserter::findShortestPath(node s, node t)
{
	NodeArray<adjEntry> spPred(m_dual, 0);
	QueuePure<adjEntry> queue;
	int oldIdCount = m_dual.maxEdgeIndex();

	// augment dual by edges from s to all adjacent faces of s ...
	adjEntry adj;
	forall_adj(adj,s) {
		// starting edges of bfs-search are all edges leaving s
		edge eDual = m_dual.newEdge(m_vS, m_faceNode[m_E.rightFace(adj)]);
		m_primalAdj[eDual->adjSource()] = adj;
		m_primalAdj[eDual->adjTarget()] = 0;
		queue.append(eDual->adjSource());
	}

	// ... and from all adjacent faces of t to t
	forall_adj(adj,t) {
		edge eDual = m_dual.newEdge(m_faceNode[m_E.rightFace(adj)], m_vT);
		m_primalAdj[eDual->adjSource()] = adj;
		m_primalAdj[eDual->adjTarget()] = 0;
	}

	// actual search (using bfs on directed dual)
	int len = -2;
	for( ; ;)
	{
		// next candidate edge
		adjEntry adjCand = queue.pop();
		node v = adjCand->twinNode();

		// leads to an unvisited node?
		if (spPred[v] == 0)
		{
			// yes, then we set v's predecessor in search tree
			spPred[v] = adjCand;

			// have we reached t ...
			if (v == m_vT)
			{
				// ... then search is done.

				adjEntry adj;
				do {
					adj = spPred[v];
					++len;
					v = adj->theNode();
				} while(v != m_vS);

				break;
			}

			// append next candidate edges to queue
			// (all edges leaving v)
			adjEntry adj;
			forall_adj(adj,v) {
				if(adj->twinNode() != m_vS)
					queue.append(adj);
			}
		}
	}

	// remove augmented edges again
	while ((adj = m_vS->firstAdj()) != 0)
		m_dual.delEdge(adj->theEdge());

	while ((adj = m_vT->firstAdj()) != 0)
		m_dual.delEdge(adj->theEdge());

	m_dual.resetEdgeIdCount(oldIdCount);

	return len;
}


} // end namespace ogdf
