/*
 * $Revision: 2565 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 17:14:54 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of Sugiyama algorithm (classes Hierarchy,
 * Level, SugiyamaLayout)
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





#include <ogdf/layered/Hierarchy.h>
#include <ogdf/layered/SugiyamaLayout.h>
#include <ogdf/layered/LongestPathRanking.h>
#include <ogdf/layered/BarycenterHeuristic.h>
#include <ogdf/layered/SplitHeuristic.h>
#include <ogdf/layered/FastHierarchyLayout.h>
#include <ogdf/layered/OptimalHierarchyClusterLayout.h>
#include <ogdf/packing/TileToRowsCCPacker.h>
#include <ogdf/basic/simple_graph_alg.h>

#include <algorithm>


namespace ogdf {


//---------------------------------------------------------
// GraphCopyAttributes
// manages access on copy of an attributed graph
//---------------------------------------------------------

void GraphCopyAttributes::transform()
{
	node v;
	forall_nodes(v,*m_pGC)
	{
		node vG = m_pGC->original(v);
		if(vG) {
			m_pAG->x(vG) = m_x[v];
			m_pAG->y(vG) = m_y[v];
		}
	}

	edge e;
	forall_edges(e,*m_pGC)
	{
		edge eG = m_pGC->original(e);
		if(eG == 0 || e != m_pGC->chain(eG).front())
			continue;
		// current implementation does not layout self-loops;
		// they are simply ignored
		//if (e->isSelfLoop()) continue;

		DPolyline &dpl = m_pAG->bends(eG);
		dpl.clear();

		ListConstIterator<edge> itE = m_pGC->chain(eG).begin();
		node v      = (*itE)->source();
		node vAfter = (*itE)->target();

		for (++itE; itE.valid(); ++itE)
		{
			node vBefore = v;
			v      = vAfter;
			vAfter = (*itE)->target();

			// filter real bend points
			if((m_x[v] != m_x[vBefore] || m_x[v] != m_x[vAfter]) &&
				(m_y[v] != m_y[vBefore] || m_y[v] != m_y[vAfter]))
				dpl.pushBack(DPoint(m_x[v],m_y[v]));
		}

		if (m_pGC->isReversed(eG))
			dpl.reverse();
	}
}


//---------------------------------------------------------
// ClusterGraphCopyAttributes
// manages access on copy of an attributed cluster graph
//---------------------------------------------------------

void ClusterGraphCopyAttributes::transform()
{
	node v;
	forall_nodes(v,*m_pH)
	{
		node vG = m_pH->origNode(v);
		if(vG) {
			m_pACG->x(vG) = m_x[v];
			m_pACG->y(vG) = m_y[v];
		}
	}

	edge e;
	forall_edges(e,*m_pH)
	{
		edge eG = m_pH->origEdge(e);
		if(eG == 0 || e != m_pH->chain(eG).front())
			continue;
		// current implementation does not layout self-loops;
		// they are simply ignored
		//if (e->isSelfLoop()) continue;

		DPolyline &dpl = m_pACG->bends(eG);
		dpl.clear();

		ListConstIterator<edge> itE = m_pH->chain(eG).begin();
		node v      = (*itE)->source();
		node vAfter = (*itE)->target();

		for (++itE; itE.valid(); ++itE)
		{
			node vBefore = v;
			v      = vAfter;
			vAfter = (*itE)->target();

			// filter real bend points
			if((m_x[v] != m_x[vBefore] || m_x[v] != m_x[vAfter]) &&
				(m_y[v] != m_y[vBefore] || m_y[v] != m_y[vAfter]))
				dpl.pushBack(DPoint(m_x[v],m_y[v]));
		}

		if (m_pH->isReversed(eG))
			dpl.reverse();
	}
}


//---------------------------------------------------------
// Level
// representation of levels in hierarchies
//---------------------------------------------------------

const Array<node> &Level::adjNodes(node v)
{
	return m_pHierarchy->adjNodes(v);
}


void Level::swap(int i, int j)
{
	m_nodes.swap(i,j);
	m_pHierarchy->m_pos[m_nodes[i]] = i;
	m_pHierarchy->m_pos[m_nodes[j]] = j;
}



class WeightBucket : public BucketFunc<node> {
	const NodeArray<int> *m_pWeight;

public:
	WeightBucket(const NodeArray<int> *pWeight) : m_pWeight(pWeight) { }

	int getBucket(const node &v) { return (*m_pWeight)[v]; }
};


void Level::recalcPos()
{
	NodeArray<int> &pos = m_pHierarchy->m_pos;

	for(int i = 0; i <= high(); ++i)
		pos[m_nodes[i]] = i;

	m_pHierarchy->buildAdjNodes(m_index);
}


void Level::getIsolatedNodes(SListPure<Tuple2<node,int> > &isolated)
{
	for (int i = 0; i <= high(); ++i)
		if (adjNodes(m_nodes[i]).high() < 0)
			isolated.pushBack(Tuple2<node,int>(m_nodes[i],i));
}


void Level::setIsolatedNodes(SListPure<Tuple2<node,int> > &isolated)
{
	const int sizeL = size();
	Array<node> sortedNodes(sizeL);
	isolated.pushBack(Tuple2<node,int>(0,sizeL));
	SListConstIterator<Tuple2<node,int> > itIsolated = isolated.begin();

	int nextPos = (*itIsolated).x2();
	for( int iNodes = 0, iSortedNodes = 0; nextPos <= sizeL; ) {
		if( iSortedNodes == nextPos ) {
			if( iSortedNodes == sizeL )
				break;
			sortedNodes[iSortedNodes++] = (*itIsolated).x1();
			nextPos = (*(++itIsolated)).x2();
		} else {
			node v = m_nodes[iNodes++];
			if( adjNodes(v).size() > 0 )
				sortedNodes[iSortedNodes++] = v;
		}
	}

	for( int i = 0; i < sizeL; ++i)
		m_nodes[i] = sortedNodes[i];
}


void Level::sort(NodeArray<double> &weight)
{
	SListPure<Tuple2<node,int> > isolated;
	getIsolatedNodes(isolated);

	WeightComparer<> cmp(&weight);
	//m_nodes.quicksort(cmp);
	std::stable_sort(&m_nodes[0], &m_nodes[0]+m_nodes.size(), cmp);

	if (!isolated.empty()) setIsolatedNodes(isolated);
	recalcPos();
}


void Level::sortByWeightOnly(NodeArray<double> &weight)
{
	WeightComparer<> cmp(&weight);
	std::stable_sort(&m_nodes[0], &m_nodes[0]+m_nodes.size(), cmp);
	recalcPos();
}


void Level::sort(NodeArray<int> &weight, int minBucket, int maxBucket)
{
	SListPure<Tuple2<node,int> > isolated;
	getIsolatedNodes(isolated);

	WeightBucket bucketFunc(&weight);
	bucketSort(m_nodes,minBucket,maxBucket,bucketFunc);

	if (!isolated.empty()) setIsolatedNodes(isolated);
	recalcPos();
}



//---------------------------------------------------------
// Hierarchy
// representation of proper hierarchies used by Sugiyama
//---------------------------------------------------------

Hierarchy::Hierarchy(const Graph &G, const NodeArray<int> &rank) :
	m_GC(G), m_rank(m_GC)
{
	doInit(rank);
}


void Hierarchy::doInit(const NodeArray<int> &rank)
{
	makeLoopFree(m_GC);

	int maxRank = 0;

	node v;
	forall_nodes(v,m_GC) {
		int r = m_rank[v] = rank[m_GC.original(v)];
		OGDF_ASSERT(r >= 0)
		if (r > maxRank) maxRank = r;
	}

	SListPure<edge> edges;
	m_GC.allEdges(edges);
	SListConstIterator<edge> it;
	for(it = edges.begin(); it.valid(); ++it)
	{
		edge e = *it;

		int rankSrc = m_rank[e->source()], rankTgt = m_rank[e->target()];

		if (rankSrc > rankTgt) {
			m_GC.reverseEdge(e); std::swap(rankSrc,rankTgt);
		}

		if (rankSrc == rankTgt) {
			e = m_GC.split(e);
			m_GC.reverseEdge(e);
			if ((m_rank[e->target()] = rankSrc+1) > maxRank)
				maxRank = rankSrc+1;

		} else {
			for(++rankSrc; rankSrc < rankTgt; ++rankSrc)
				m_rank[(e = m_GC.split(e))->source()] = rankSrc;
		}
	}

	Array<int> length(0,maxRank,0);
	forall_nodes(v,m_GC)
		length[m_rank[v]]++;

	int i;
	for(i = 0; i <= m_pLevel.high(); ++i)
		delete m_pLevel[i];
	m_pLevel.init(0,maxRank);

	for(i = 0; i <= maxRank; ++i)
		m_pLevel[i] = new Level(this,i,length[i]);

	m_pos.init(m_GC);
	m_lowerAdjNodes.init(m_GC);
	m_upperAdjNodes.init(m_GC);
	m_lastOcc.init(m_GC);  // only used by calculateCrossingsPlaneSweep()

	forall_nodes(v,m_GC) {
		int r = m_rank[v], pos = --length[r];
		m_pos[(*m_pLevel[r])[pos] = v] = pos;

		m_lowerAdjNodes[v].init(v->indeg());
		m_upperAdjNodes[v].init(v->outdeg());
	}

	m_nSet.init(m_GC,0);
	buildAdjNodes();
}


void Hierarchy::createEmpty(const Graph &G)
{
	m_GC.createEmpty(G);
	m_rank.init(m_GC);
}


void Hierarchy::initByNodes(const List<node> &nodes,
	EdgeArray<edge> &eCopy,
	const NodeArray<int> &rank)
{
	m_GC.initByNodes(nodes,eCopy);

	doInit(rank);
}


Hierarchy::~Hierarchy()
{
	for(int i = 0; i <= high(); ++i)
		delete m_pLevel[i];
}


void Hierarchy::buildAdjNodes()
{
	for(int i = 0; i <= high(); ++i)
		buildAdjNodes(i);
}


void Hierarchy::buildAdjNodes(int i)
{
	if (i > 0) {
		const Level &lowerLevel = *m_pLevel[i-1];

		for(int j = 0; j <= lowerLevel.high(); ++j)
			m_nSet[lowerLevel[j]] = 0;
	}

	if (i < high()) {
		const Level &upperLevel = *m_pLevel[i+1];

		for(int j = 0; j <= upperLevel.high(); ++j)
			m_nSet[upperLevel[j]] = 0;
	}

	const Level &level = *m_pLevel[i];

	for(int j = 0; j <= level.high(); ++j) {
		node v = level[j];
		edge e;
		forall_adj_edges(e,v) {
			if (e->source() == v) {
				(m_lowerAdjNodes[e->target()])[m_nSet[e->target()]++] = v;
			} else {
				(m_upperAdjNodes[e->source()])[m_nSet[e->source()]++] = v;
			}
		}
	}
}


void Hierarchy::storePos (NodeArray<int> &oldPos)
{
	oldPos = m_pos;
}


void Hierarchy::restorePos (const NodeArray<int> &newPos)
{
	m_pos = newPos;

	node v;
	forall_nodes(v,m_GC) {
		(*m_pLevel[m_rank[v]])[m_pos[v]] = v;
	}

	//check();

	buildAdjNodes();
}


void Hierarchy::permute()
{
	for(int i = 0; i < m_pLevel.high(); ++i) {
		Level &level = *m_pLevel[i];
		level.m_nodes.permute();
		for(int j = 0; j <= level.high(); ++j)
			m_pos[level[j]] = j;
	}

	//check();

	buildAdjNodes();
}


void Hierarchy::separateCCs(int numCC, NodeArray<int> &component)
{
	Array<SListPure<node> > table(numCC);

	for(int i = 0; i < m_pLevel.high(); ++i) {
		Level &level = *m_pLevel[i];
		for(int j = 0; j <= level.high(); ++j) {
			node v = level[j];
			table[component[v]].pushBack(v);
		}
	}

	Array<int> count(0, m_pLevel.high(), 0);
	for(int c = 0; c < numCC; ++c) {
		SListConstIterator<node> it;
		for(it = table[c].begin(); it.valid(); ++it)
			m_pos[*it] = count[m_rank[*it]]++;
	}

	node v;
	forall_nodes(v,m_GC) {
		(*m_pLevel[m_rank[v]])[m_pos[v]] = v;
	}

	//check();

	buildAdjNodes();
}


int Hierarchy::calculateCrossings()
{
	int nCrossings = 0;

	for(int i = 0; i < m_pLevel.high(); ++i) {
		nCrossings += calculateCrossings(i);
	}

	return nCrossings;
}


// calculation of edge crossings between level i and i+1
// implementation by Michael Juenger, Decembre 2000, adapted by Carsten Gutwenger
// implements the algorithm by Barth, Juenger, Mutzel

int Hierarchy::calculateCrossings(int i)
{
	const Level &L = *m_pLevel[i];             // level i
	const int nUpper = m_pLevel[i+1]->size();  // number of nodes on level i+1

	int nc = 0; // number of crossings

	int fa = 1;
	while (fa < nUpper)
		fa *= 2;

	int nTreeNodes = 2*fa - 1; // number of tree nodes
	fa -= 1;         // "first address:" indexincrement in tree

	Array<int> nin(0,nTreeNodes-1,0);

	for(int j = 0; j < L.size(); ++j)
	{
		const Array<node> &adjNodes = m_upperAdjNodes[L[j]];
		for(int k = 0; k < adjNodes.size(); ++k)
		{
			// index of tree node for vertex adjNode[k]
			int index = m_pos[adjNodes[k]] + fa;
			nin[index]++;

			while (index>0) {
				if (index % 2)
					nc += nin[index+1]; // new crossing
				index = (index - 1) / 2;
				nin[index]++;
			}

		}
	}

	return nc;
}



//-----------------------------------------------
// The following code
//   calculateCrossingsPlaneSweep() and
//   calculateCrossingsPlaneSweep(int i)
// is the old implementation of the crossing calculation using a
// plane sweep algorithm. We keep it in the library in order to
// test the new implementation against the old one.

int Hierarchy::calculateCrossingsPlaneSweep()
{
	int nCrossings = 0;

	for(int i = 0; i < m_pLevel.high(); ++i) {
		nCrossings += calculateCrossingsPlaneSweep(i);
	}

	return nCrossings;
}


// The member node array m_lastOcc is only used by this function!

int Hierarchy::calculateCrossingsPlaneSweep(int i)
{
	const Level *pLevel[2];
	pLevel[0] = m_pLevel[i]; pLevel[1] = m_pLevel[i+1];

	if (pLevel[0]->high() <= 0 || pLevel[1] <= 0) return 0;

	int j, k;
	for(j = 0; j <= 1; ++j)
		for(k = 0; k <= pLevel[j]->high(); ++k)
			m_lastOcc[(*pLevel[j])[k]] = 0;

	j = 0;
	int index[2] = { 0, 0 };
	List<node> endNodes[2];
	int nCrossings = 0;

	do {
		int nOccW = 0, nCrossW = 0, nSumCrossW = 0;
		node w = (*pLevel[j])[index[j]];

		if (m_lastOcc[w].valid()) {
			ListIterator<node> it, itSucc;
			for(it = endNodes[j].begin(); it.valid(); it = itSucc) {
				itSucc = it.succ();
				if (*it == w) {
					nOccW++;
					nSumCrossW += nCrossW;
					endNodes[j].del(it);
				} else
					nCrossW++;
				if (m_lastOcc[w] == it) break;
			}
			nCrossings += nOccW * endNodes[1-j].size() + nSumCrossW;
		}

		const Array<node> &adjNodes =
			(j == 0) ? m_upperAdjNodes[w] : m_lowerAdjNodes[w];

		for(k = 0; k <= adjNodes.high(); ++k) {
			node uk = adjNodes[k];
			if (m_pos[uk] > m_pos[w] || (m_pos[uk] == m_pos[w] && j == 0))
				m_lastOcc[uk] = endNodes[1-j].pushBack(uk);
		}

		++index[j];
		if (index[1-j] < pLevel[1-j]->size())
			j = 1-j;

	} while (index[j] < pLevel[j]->size());

	return nCrossings;
}

// end of old implementation
//-----------------------------------------------


int Hierarchy::calculateCrossingsSimDraw(const EdgeArray<unsigned int> *edgeSubGraph)
{
	int nCrossings = 0;

	for(int i = 0; i < m_pLevel.high(); ++i) {
		nCrossings += calculateCrossingsSimDraw(i, edgeSubGraph);
	}

	return nCrossings;
}


// naive calculation of edge crossings between level i and i+1
// for SimDraw-calculation by Michael Schulz

int Hierarchy::calculateCrossingsSimDraw(int i, const EdgeArray<unsigned int> *edgeSubGraph)
{
	const int maxGraphs = 32;

	const Level &L = *m_pLevel[i];             // level i
	const GraphCopy &GC = L.hierarchy();

	int nc = 0; // number of crossings

	for(int j = 0; j < L.size(); ++j)
	{
		node v = L[j];
		edge e;
		forall_adj_edges(e,v) {
			if (e->source() == v){
				int pos_adj_e = pos(e->target());
				for (int k = j+1; k < L.size(); k++) {
					node w = L[k];
					edge f;
					forall_adj_edges(f,w) {
						if (f->source() == w) {
							int pos_adj_f = pos(f->target());
							if(pos_adj_f < pos_adj_e)
							{
								int graphCounter = 0;
								for(int numGraphs = 0; numGraphs < maxGraphs; numGraphs++)
									if((1 << numGraphs) & (*edgeSubGraph)[GC.original(e)] & (*edgeSubGraph)[GC.original(f)])
										graphCounter++;
								nc += graphCounter;
							}
						}
					}
				}
			}
		}
	}

	return nc;
}


void Hierarchy::print(ostream &os)
{
	for(int i = 0; i <= m_pLevel.high(); ++i) {
		os << i << ": ";
		const Level &level = *m_pLevel[i];
		for(int j = 0; j < level.size(); ++j)
			os << level[j] << " ";
		os << endl;
	}

	os << endl;
	node v;
	forall_nodes(v,m_GC) {
		os << v << ": lower: " << (m_lowerAdjNodes[v]) <<
			", upper: " << (m_upperAdjNodes[v]) << endl;
	}

}

int Hierarchy::transposePart(
	const Array<node> &adjV,
	const Array<node> &adjW)
{
	const int vSize = adjV.size();
	int iW = 0, iV = 0, sum = 0;

	for(; iW <= adjW.high(); ++iW) {
		int p = m_pos[adjW[iW]];
		while(iV < vSize && m_pos[adjV[iV]] <= p) ++iV;
		sum += vSize - iV;
	}

	return sum;
}


bool Hierarchy::transpose(node v)
{
	int rankV = m_rank[v], posV = m_pos[v];
	node w = (*m_pLevel[rankV])[posV+1];

	int d = 0;
	d += transposePart(m_upperAdjNodes[v],m_upperAdjNodes[w]);
	d -= transposePart(m_upperAdjNodes[w],m_upperAdjNodes[v]);
	d += transposePart(m_lowerAdjNodes[v],m_lowerAdjNodes[w]);
	d -= transposePart(m_lowerAdjNodes[w],m_lowerAdjNodes[v]);

	if (d > 0) {
		m_pLevel[rankV]->swap(posV,posV+1);
		return true;
	}

	return false;
}


void Hierarchy::check()
{
	int i, j;
	for(i = 0; i <= high(); ++i) {
		Level &L = *m_pLevel[i];
		for(j = 0; j <= L.high(); ++j) {
			if (m_pos[L[j]] != j) {
				cerr << "m_pos[" << L[j] << "] wrong!" << endl;
			}
			if (m_rank[L[j]] != i) {
				cerr << "m_rank[" << L[j] << "] wrong!" << endl;
			}
		}
	}

/*	node v;
	forall_nodes(v,m_GC) {
		const Array<node> &upperNodes = m_upperAdjNodes[v];
		for(i = 0; i <= upperNodes.high(); ++i) {
			if (i > 0 && m_pos[upperNodes[i-1]] > m_pos[upperNodes[i]]) {
				cerr << "upperNodes[" << v << "] not sorted!" << endl;
			}
		}
		const Array<node> &lowerNodes = m_lowerAdjNodes[v];
		for(i = 0; i <= lowerNodes.high(); ++i) {
			if (i > 0 && m_pos[lowerNodes[i-1]] > m_pos[lowerNodes[i]]) {
				cerr << "lowerNodes[" << v << "] not sorted!" << endl;
			}
		}
	}*/
}




//---------------------------------------------------------
// SugiyamaLayout
// Sugiyama drawing algorithm for hierarchical graphs
//---------------------------------------------------------

SugiyamaLayout::SugiyamaLayout()
{
	m_ranking        .set(new LongestPathRanking);
	m_crossMin       .set(new BarycenterHeuristic);
	m_crossMinSimDraw.set(new SplitHeuristic);
	m_layout         .set(new FastHierarchyLayout);
	m_clusterLayout  .set(new OptimalHierarchyClusterLayout);
	m_packer         .set(new TileToRowsCCPacker);

	m_fails = 4;
	m_runs = 15;
	m_transpose = true;
	m_permuteFirst = false;

	m_arrangeCCs = true;
	m_minDistCC = 20;
	m_pageRatio = 1.0;

	m_alignBaseClasses = false;
	m_alignSiblings = false;
	m_subgraphs = 0;

	m_maxLevelSize = -1;
	m_numLevels = -1;
	m_timeReduceCrossings = 0.0;
}




void SugiyamaLayout::call(GraphAttributes &AG)
{
	doCall(AG,false);
}


void SugiyamaLayout::call(GraphAttributes &AG, NodeArray<int> &rank)
{
	doCall(AG,false,rank);
}


void SugiyamaLayout::doCall(GraphAttributes &AG, bool umlCall)
{
	NodeArray<int> rank;
	doCall(AG, umlCall, rank);
}


void SugiyamaLayout::doCall(GraphAttributes &AG, bool umlCall, NodeArray<int> &rank)
{
	const Graph &G = AG.constGraph();
	if (G.numberOfNodes() == 0)
		return;

	// compute connected component of G
	NodeArray<int> component(G);
	m_numCC = connectedComponents(G,component);

	const bool optimizeHorizEdges = (umlCall || rank.valid());
	if(!rank.valid())
	{
		if(umlCall)
		{
			LongestPathRanking ranking;
			ranking.alignBaseClasses(m_alignBaseClasses);
			ranking.alignSiblings(m_alignSiblings);

			ranking.callUML(AG,rank);

		} else {
			m_ranking.get().call(AG.constGraph(),rank);
		}
	}

	if(m_arrangeCCs) {
		// intialize the array of lists of nodes contained in a CC
		Array<List<node> > nodesInCC(m_numCC);

		node v;
		forall_nodes(v,G)
			nodesInCC[component[v]].pushBack(v);

		Hierarchy H;
		H.createEmpty(G);
		const GraphCopy &GC = H;

		EdgeArray<edge> auxCopy(G);
		Array<DPoint> boundingBox(m_numCC);
		Array<DPoint> offset1(m_numCC);
		NodeArray<bool> mark(GC);

		m_numLevels = m_maxLevelSize = 0;

		int totalCrossings = 0;
		for(int i = 0; i < m_numCC; ++i)
		{
			// adjust ranks in cc to start with 0
			int minRank = INT_MAX;
			ListConstIterator<node> it;
			for(it = nodesInCC[i].begin(); it.valid(); ++it)
				if(rank[*it] < minRank)
					minRank = rank[*it];

			if(minRank != 0) {
				for(it = nodesInCC[i].begin(); it.valid(); ++it)
					rank[*it] -= minRank;
			}

			H.initByNodes(nodesInCC[i],auxCopy,rank);

			reduceCrossings(H);
			totalCrossings += m_nCrossings;

			m_layout.get().call(H,AG);

			double minX = DBL_MAX, maxX = -DBL_MAX, minY = DBL_MAX, maxY = -DBL_MAX;

			node vCopy;
			forall_nodes(vCopy,GC)
			{
				mark[vCopy] = false;
				node v = GC.original(vCopy);
				if(v == 0) continue;

				if(AG.x(v)-AG.width (v)/2 < minX) minX = AG.x(v)-AG.width(v) /2;
				if(AG.x(v)+AG.width (v)/2 > maxX) maxX = AG.x(v)+AG.width(v) /2;
				if(AG.y(v)-AG.height(v)/2 < minY) minY = AG.y(v)-AG.height(v)/2;
				if(AG.y(v)+AG.height(v)/2 > maxY) maxY = AG.y(v)+AG.height(v)/2;
			}

			if(optimizeHorizEdges)
			{
				for(int i = 0; i < H.size(); ++i) {
					const Level &L = H[i];
					for(int j = 0; j < L.size(); ++j) {
						node v = L[j];
						if(!GC.isDummy(v)) continue;
						edge e = GC.original(v->firstAdj()->theEdge());
						if(e == 0) continue;
						node src = GC.copy(e->source());
						node tgt = GC.copy(e->target());

						if(H.rank(src) == H.rank(tgt)) {
							int minPos = H.pos(src), maxPos = H.pos(tgt);
							if(minPos > maxPos) std::swap(minPos,maxPos);

							bool straight = true;
							const Level &L_e = H[H.rank(src)];
							for(int p = minPos+1; p < maxPos; ++p) {
								if(!H.isLongEdgeDummy(L_e[p]) && mark[L_e[p]] == false) {
									straight = false; break;
								}
							}
							if(straight) {
								AG.bends(e).clear();
								mark[v] = true;
							}
						}
					}
				}
			}

			edge eCopy;
			forall_edges(eCopy,GC)
			{
				edge e = GC.original(eCopy);
				if(e == 0 || eCopy != GC.chain(e).front()) continue;

				const DPolyline &dpl = AG.bends(e);
				ListConstIterator<DPoint> it;
				for(it = dpl.begin(); it.valid(); ++it)
				{
					if((*it).m_x < minX) minX = (*it).m_x;
					if((*it).m_x > maxX) maxX = (*it).m_x;
					if((*it).m_y < minY) minY = (*it).m_y;
					if((*it).m_y > maxY) maxY = (*it).m_y;
				}
			}

			minX -= m_minDistCC;
			minY -= m_minDistCC;

			boundingBox[i] = DPoint(maxX - minX, maxY - minY);
			offset1    [i] = DPoint(minX,minY);

			m_numLevels = max(m_numLevels, H.size());
			for(int i = 0; i <= H.high(); i++) {
				Level &l = H[i];
				m_maxLevelSize = max(m_maxLevelSize, l.size());
			}
		}

		m_nCrossings = totalCrossings;

		// call packer
		Array<DPoint> offset(m_numCC);
		m_packer.get().call(boundingBox,offset,m_pageRatio);

		// The arrangement is given by offset to the origin of the coordinate
		// system. We still have to shift each node and edge by the offset
		// of its connected component.

		for(int i = 0; i < m_numCC; ++i)
		{
			const List<node> &nodes = nodesInCC[i];

			const double dx = offset[i].m_x - offset1[i].m_x;
			const double dy = offset[i].m_y - offset1[i].m_y;

			// iterate over all nodes in ith CC
			ListConstIterator<node> it;
			for(it = nodes.begin(); it.valid(); ++it)
			{
				node v = *it;

				AG.x(v) += dx;
				AG.y(v) += dy;

				edge e;
				forall_adj_edges(e,v)
				{
					if(e->isSelfLoop() || e->source() != v) continue;

					DPolyline &dpl = AG.bends(e);
					ListIterator<DPoint> it;
					for(it = dpl.begin(); it.valid(); ++it)
					{
						(*it).m_x += dx;
						(*it).m_y += dy;
					}
				}
			}
		}

	} else {
		int minRank = INT_MAX;
		node v;
		forall_nodes(v,G)
			if(rank[v] < minRank)
				minRank = rank[v];

		if(minRank != 0) {
			forall_nodes(v,G)
				rank[v] -= minRank;
		}

		Hierarchy H(G,rank);
		const GraphCopy &GC = H;

		m_compGC.init(GC);
		forall_nodes(v,GC) {
			node vOrig = GC.original(v);
			if(vOrig == 0)
				vOrig = GC.original(v->firstAdj()->theEdge())->source();

			m_compGC[v] = component[vOrig];
		}

		reduceCrossings(H);
		m_compGC.init();

		m_layout.get().call(H,AG);

		if(optimizeHorizEdges)
		{
			NodeArray<bool> mark(GC,false);
			for(int i = 0; i < H.size(); ++i) {
				const Level &L = H[i];
				for(int j = 0; j < L.size(); ++j) {
					node v = L[j];
					if(!GC.isDummy(v)) continue;
					edge e = GC.original(v->firstAdj()->theEdge());
					if(e == 0) continue;
					node src = GC.copy(e->source());
					node tgt = GC.copy(e->target());

					if(H.rank(src) == H.rank(tgt)) {
						int minPos = H.pos(src), maxPos = H.pos(tgt);
						if(minPos > maxPos) std::swap(minPos,maxPos);

						bool straight = true;
						const Level &L_e = H[H.rank(src)];
						for(int p = minPos+1; p < maxPos; ++p) {
							if(!H.isLongEdgeDummy(L_e[p]) && mark[L_e[p]] == false) {
								straight = false; break;
							}
						}
						if(straight) {
							AG.bends(e).clear();
							mark[v] = true;
						}
					}
				}
			}
		}

		m_numLevels = H.size();
		m_maxLevelSize = 0;
		for(int i = 0; i <= H.high(); i++) {
			Level &l = H[i];
			if (l.size() > m_maxLevelSize)
				m_maxLevelSize = l.size();
		}

	}

}


void SugiyamaLayout::callUML(GraphAttributes &AG)
{
	doCall(AG,true);
}


bool SugiyamaLayout::transposeLevel(int i, Hierarchy &H)
{
	bool improved = false;

	if (m_levelChanged[i] || m_levelChanged[i-1] || m_levelChanged[i+1]) {
		Level &L = H[i];

		for (int j = 0; j < L.high(); j++) {
			if (H.transpose(L[j])) improved = true;
		}
	}

	if (improved) H.buildAdjNodes(i);
	return (m_levelChanged[i] = improved);
}


void SugiyamaLayout::doTranspose(Hierarchy &H)
{
	m_levelChanged.fill(true);

	bool improved;
	do {
		improved = false;

		for (int i = 0; i <= H.high(); ++i)
			improved |= transposeLevel(i,H);
	} while (improved);
}


void SugiyamaLayout::doTransposeRev(Hierarchy &H)
{
	m_levelChanged.fill(true);

	bool improved;
	do {
		improved = false;

		for (int i = H.high(); i >= 0 ; --i)
			improved |= transposeLevel(i,H);
	} while (improved);
}


int SugiyamaLayout::traverseTopDown(Hierarchy& H)
{
	H.direction(Hierarchy::downward);

	for (int i = 1; i <= H.high(); ++i) {
		if(!useSubgraphs()) {
			TwoLayerCrossMin &minimizer = m_crossMin.get();
			minimizer(H[i]);
		} else {
			TwoLayerCrossMinSimDraw &minimizer = m_crossMinSimDraw.get();
			minimizer.call(H[i],m_subgraphs);
		}
	}

	if (m_transpose)
		doTranspose(H);
	if(m_arrangeCCs == false)
		H.separateCCs(m_numCC, m_compGC);

	if(!useSubgraphs())
		return H.calculateCrossings();
	else
		return H.calculateCrossingsSimDraw(m_subgraphs);
}


int SugiyamaLayout::traverseBottomUp(Hierarchy& H)
{
	H.direction(Hierarchy::upward);

	for (int i = H.high()-1; i >= 0; i--) {
		if(!useSubgraphs())
		{
			TwoLayerCrossMin &minimizer = m_crossMin.get();
			minimizer(H[i]);
		}
		else
		{
			TwoLayerCrossMinSimDraw &minimizer = m_crossMinSimDraw.get();
			minimizer.call(H[i],m_subgraphs);
		}
	}

	if (m_transpose)
		doTransposeRev(H);
	if(m_arrangeCCs == false)
		H.separateCCs(m_numCC, m_compGC);

	if(!useSubgraphs())
		return H.calculateCrossings();
	else
		return H.calculateCrossingsSimDraw(m_subgraphs);
}


void SugiyamaLayout::reduceCrossings(Hierarchy &H)
{
	__int64 t;
	System::usedRealTime(t);

	TwoLayerCrossMin &minimizer = m_crossMin.get();
	TwoLayerCrossMinSimDraw &sdminimizer = m_crossMinSimDraw.get();

	if(m_permuteFirst)
		H.permute();

	int nCrossingsOld, nCrossingsNew;
	NodeArray<int> bestPos;

	if(!useSubgraphs())
		m_nCrossings = nCrossingsOld = H.calculateCrossings();
	else
		m_nCrossings = nCrossingsOld = H.calculateCrossingsSimDraw(m_subgraphs);
	H.storePos(bestPos);

	if (m_nCrossings == 0) {
		t = System::usedRealTime(t);
		m_timeReduceCrossings = double(t) / 1000;
		return;
	}

	if(!useSubgraphs())
		minimizer.init(H);
	else
		sdminimizer.init(H);

	if (m_transpose) {
		m_levelChanged.init(-1,H.size());
		m_levelChanged[-1] = m_levelChanged[H.size()] = false;
	}

	for (int i = 1; ;i++ ) {
		int nFails = m_fails+1;

		do {
			nCrossingsNew = traverseTopDown(H);
			//cout << nCrossingsNew << endl;

			if (nCrossingsNew < nCrossingsOld) {
				if (nCrossingsNew < m_nCrossings) {
					H.storePos(bestPos);

					if ((m_nCrossings = nCrossingsNew) == 0)
						break;
				}
				nCrossingsOld = nCrossingsNew;
				nFails = m_fails+1;
			} else
				--nFails;

			nCrossingsNew = traverseBottomUp(H);
			//cout << nCrossingsNew << endl;

			if (nCrossingsNew < nCrossingsOld) {
				if (nCrossingsNew < m_nCrossings) {
					H.storePos(bestPos);

					if ((m_nCrossings = nCrossingsNew) == 0)
						break;
				}
				nCrossingsOld = nCrossingsNew;
				nFails = m_fails+1;
			} else
				--nFails;

		} while (nFails > 0);

		if (m_nCrossings == 0 || i >= m_runs)
			break;

		H.permute();

		if(!useSubgraphs())
			nCrossingsOld = H.calculateCrossings();
		else
			nCrossingsOld = H.calculateCrossings();
		if (nCrossingsOld < m_nCrossings) {
			H.storePos(bestPos);

			m_nCrossings = nCrossingsOld;
		}
	}

	H.restorePos(bestPos);

	if(!useSubgraphs())
		minimizer.cleanup();
	else
		sdminimizer.cleanup();
	m_levelChanged.init();

	t = System::usedRealTime(t);
	m_timeReduceCrossings = double(t) / 1000;
}


} // end namespace ogdf
