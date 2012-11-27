/*
 * $Revision: 2559 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-06 15:04:28 +0200 (Fr, 06. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of OrderComparer and LayerBasedUPRLayout classes.
 *
 * \author Hoi-Ming Wong
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

#include <ogdf/upward/LayerBasedUPRLayout.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/Queue.h>
#include <ogdf/basic/Stack.h>


namespace ogdf {


OrderComparer::OrderComparer(const UpwardPlanRep &_UPR, Hierarchy &_H) : UPR(_UPR), H(_H)
{
	dfsNum.init(UPR, -1);
	crossed.init(UPR, false);

	//compute dfs number
	node start;
	hasSingleSource(UPR, start);
	NodeArray<bool> visited(UPR, false);
	adjEntry rightAdj = UPR.getAdjEntry(UPR.getEmbedding(), start, UPR.getEmbedding().externalFace());
	int num = 0;
	dfsNum[start] = num++;
	adjEntry run = rightAdj;
	do {
		run = run->cyclicSucc();
		if (!visited[run->theEdge()->target()])
			dfs_LR(run->theEdge(), visited, dfsNum, num);
	} while(run != rightAdj);

}



bool OrderComparer::left(edge e1UPR, edge e2UPR) const
{
	OGDF_ASSERT(e1UPR->source() == e2UPR->source() || e1UPR->target() == e2UPR->target());
	OGDF_ASSERT(e1UPR != e2UPR);

	node v = e1UPR->source();
	if (e2UPR->source() != v)
		v = e1UPR->target();

	adjEntry inLeft = 0, outLeft = 0;
	//compute left in and left out edge of the common node v if exist
	if (v->indeg() != 0) {
		adjEntry run;
		forall_adj(run, v) {
			if (run->cyclicSucc()->theEdge()->source() == v)
				break;
		}
		inLeft = run;
	}
	if (v->outdeg() != 0) {
		adjEntry run;
		forall_adj(run, v) {
			if (run->cyclicPred()->theEdge()->target() == v)
				break;
			if (UPR.getEmbedding().leftFace(run) == UPR.getEmbedding().externalFace())
				break;
		}
		outLeft = run;
	}

	//same source;
	if (v == e2UPR->source()) {
		do {
			if (outLeft->theEdge() == e1UPR)
				return false;
			if (outLeft->theEdge() == e2UPR)
				return true;
			outLeft = outLeft->cyclicSucc();
		} while (true);
	}
	//same target
	else {
		do {
			if (inLeft->theEdge() == e1UPR)
				return false;
			if (inLeft->theEdge() == e2UPR)
				return true;
			inLeft = inLeft->cyclicPred();
		} while (true);
	}
}



bool OrderComparer::left(node v1UPR, List<edge> chain1, node v2UPR , List<edge> chain2) const
{
	//mark the edges an nodes of chain2 list
	NodeArray<bool> visitedNode(UPR, false);
	EdgeArray<bool> visitedEdge(UPR, false);
	forall_listiterators(edge, it, chain2) {
		edge e = *it;
		visitedNode[e->source()] =  visitedNode[e->target()] = true;
		visitedEdge[e] = true;
	}

	// traverse from vUPR2 to the super source using left path p and marks it.
	visitedNode[v2UPR] = true;
	adjEntry run = UPR.leftInEdge(v2UPR);
	while (run != 0) {
		visitedNode[run->theEdge()->source()] = visitedNode[run->theEdge()->target()] = true;
		visitedEdge[run->theEdge()] = true;
		run = UPR.leftInEdge(run->theEdge()->source());
	}

	//is one of the node of chain1 marked?
	ListIterator<edge> it;
	for(it = chain1.rbegin(); it.valid(); --it) {
		node u = (*it)->source();
		if (visitedNode[u]) {
			adjEntry run;
			forall_adj(run, u) {
				if (visitedEdge[run->theEdge()] && run->theEdge()->source() == run->theNode()) // outgoing edges only
					return left(*it, run->theEdge()); //(outEdgeOrder[*it] > outEdgeOrder[run->theEdge()]);
			}
		}
	}

	// traverse from vUPR1 to a node of path p (using left path).
	adjEntry adj_v1 = 0, adj_v2 = 0;
	run = UPR.leftInEdge(v1UPR);
	while (run != 0) {
		if (visitedNode[run->theEdge()->source()]) {
			adj_v1 = run->twin(); //reached a marked node
			break;
		}
		run = UPR.leftInEdge(run->theEdge()->source());
	}

	OGDF_ASSERT(adj_v1 != 0);

	forall_adj(run, adj_v1->theNode()) {
		if (visitedEdge[run->theEdge()] && run->theEdge()->source() == run->theNode()){ // outgoing edges only
			adj_v2 = run;
			break;
		}
	}

	OGDF_ASSERT(adj_v2 != 0);

	return left(adj_v1->theEdge(), adj_v2->theEdge());
}



bool OrderComparer::checkUp(node vUPR, int level) const
{
	const GraphCopy &GC = H;

	//traverse from vUPR (going up)
	NodeArray<bool> inList(UPR, false);
	List<node> l;
	l.pushBack(vUPR);
	inList[vUPR] = true;
	while (!l.empty()) {
		node v = l.popFrontRet();
		node vOrig = UPR.original(v);
		if (vOrig != 0 && H.rank(GC.copy(vOrig)) <= level)
			return true;
		List<edge> outEdges;
		UPR.outEdges(v, outEdges);
		forall_listiterators(edge, it, outEdges) {
			node tgt = (*it)->target();
			if (!inList[tgt]) {
				l.pushBack(tgt);
				inList[tgt] = true;
			}
		}
	}
	return false;
}



bool OrderComparer::left(List<edge> &chain1, List<edge> &chain2, int level) const
{
	//mark the source nodes of the edges of chain1
	NodeArray<bool> markedNodes(UPR, false);
	EdgeArray<bool> markedEdges(UPR, false);
	forall_listiterators(edge, iter, chain1) {
		node v = (*iter)->source();
		markedNodes[v] = true;
		markedEdges[*iter] = true;
	}

	//compute the list of common nodes of chain1 and chain2
	List< Tuple2<node, bool> > commonNodeList; // first: common node; second: true if vH1 (associated with chain1) is on the left hand side
	forall_listiterators(edge, iter, chain2) {
		node v = (*iter)->source();
		if (markedNodes[v]) {
			edge e = *iter;
			bool value = true;
			adjEntry adj = e->adjSource();
			while (true) {
				adj = adj->cyclicSucc();
				if (adj->theEdge()->target() == v) {
					value = false;
					break;
				}
				if (markedEdges[adj->theEdge()]) {
					break;
				}

			}
			Tuple2<node, bool> tulp(v, value);
			commonNodeList.pushFront(tulp);
		}
	}

	//no crossings between the associated edges
	if (commonNodeList.empty()) {
		if (chain1.front()->source() == chain2.front()->source()) {
				return left(chain1.front(), chain2.front());
			}
			else
				return left(chain1.front()->source(), chain1, chain2.front()->source(), chain2);
	}

	// there is a least one crossing
	ListIterator< Tuple2<node, bool> > it = commonNodeList.begin();
	while(it.valid()) {
		Tuple2<node, bool> tulp = *it;
		// is there a node above which level is lower or equal the given level?
		// if so, then return the value
		if (checkUp(tulp.x1(), level)) {
			// there is a node above, which is lower or equal the given level
			return tulp.x2();
		}
		it = it.succ();
	}

	// the both edges are on the "first segment" of the crossing
	Tuple2<node, bool> tulp = *(commonNodeList.rbegin());
	return !tulp.x2();
}



bool OrderComparer::less(node vH1, node vH2) const
{
	if (vH1 == vH2)
		return false;

	/*
	case:vH1 and vH2 are not long-edge dummies.
	*/
	const GraphCopy &GC = H;
	if (!H.isLongEdgeDummy(vH1) && !H.isLongEdgeDummy(vH2)) {
		node v1 = UPR.copy(GC.original(vH1));
		node v2 = UPR.copy(GC.original(vH2));
		if (dfsNum[v1] > dfsNum[v2])
			return true;
		else
			return false;
	}

	/*
	vH1 and vH2 are long-edge-dummies
	*/
	if (H.isLongEdgeDummy(vH1) && H.isLongEdgeDummy(vH2)) {
		List<edge> chain1 = UPR.chain(GC.original(vH1->firstAdj()->theEdge()));
		List<edge> chain2 = UPR.chain(GC.original(vH2->firstAdj()->theEdge()));

		OGDF_ASSERT(!chain1.empty() && !chain2.empty());

		int level = H.rank(vH1);
		return (left(chain1, chain2, level));
	}//end both are long edge dummies

	/*
	only vH1 or vH2 is a long-edge dummy
	*/
	node v;
	List<edge> chain1, chain2;
	if (H.isLongEdgeDummy(vH1)) {
		chain1 = UPR.chain(GC.original(vH1->firstAdj()->theEdge()));
		v = UPR.copy(GC.original(vH2));

		OGDF_ASSERT(!chain1.empty());

		return left(chain1.front()->source(), chain1, v, chain2);
	}
	else {
		chain2 = UPR.chain(GC.original(vH2->firstAdj()->theEdge()));
		v = UPR.copy(GC.original(vH1));

		OGDF_ASSERT(!chain2.empty());

		return left(v, chain1, chain2.front()->source(), chain2);
	}
}



void OrderComparer::dfs_LR(
	edge e,
	NodeArray<bool> &visited,
	NodeArray<int> &dfsNum,
	int &num)
{
	node v = e->target();
	//outEdgeOrder[e] = num++;
	dfsNum[v] = num++;
	if (e->target()->outdeg() > 0) {
		// compute left out edge
		adjEntry run;
		forall_adj(run, v) {
			adjEntry adj_pred = run->cyclicPred();
			if (adj_pred->theEdge()->target() == v && run->theEdge()->source() == v)
				break; // run is the left out-edge
		}

		do {
			if (!visited[run->theEdge()->target()]) {
				dfs_LR(run->theEdge(), visited, dfsNum, num);
			}
			run = run->cyclicSucc();
		} while(run->theEdge()->target() != e->target());
	}
	visited[v] = true;
}



void LayerBasedUPRLayout::doCall(const UpwardPlanRep &UPR, GraphAttributes &AG)
{
	OGDF_ASSERT(UPR.augmented());

	numberOfLevels = 0;
	m_numLevels = 0;
	m_crossings = 0;

	const Graph &G = UPR.original();
	NodeArray<int> rank_G(G);
	computeRanking(UPR, rank_G);
	Hierarchy H(G, rank_G);
	const GraphCopy &GC = H;

	/*
	//debug
	//UPR.outputFaces(UPR.getEmbedding());
	node x;
	forall_nodes(x, G) {
		cout << "vOrig " << x << ";   vUPR " << UPR.copy(x) << endl;
	}
	forall_nodes(x, UPR) {
		cout << "UPR edge order:" << endl;
		adjEntry adj = x->firstAdj();
		cout << "node " << x << endl;
		do {
			cout << " edge : " << adj->theEdge() << endl;
			adj = adj->cyclicSucc();
		} while (adj != x->firstAdj());
	}
	*/

	//adjust order
	OrderComparer oComparer(UPR, H);
	for(int i = 0; i < H.size(); ++i) {
		Level &l = H[i];
		l.sortOrder(oComparer);
	}


	// ********************** postprocessing *******************************************
	node vTmp;
	List<node> sources;
	forall_nodes(vTmp, GC) {
		if (vTmp->indeg() == 0)
			sources.pushBack(vTmp);
	}

	RankComparer comp;
	comp.H = &H;
	sources.quicksort(comp);
	sources.reverse();



	postProcessing_reduceLED(H, sources);
	H.buildAdjNodes();

	/*
	//debug
	cout << endl << endl;
	for(int i = 0; i <= H.high(); i++) {
		Level &lvl = H[i];
		cout << "level : " << lvl.index() << endl;
		cout << "nodes : ";
		for(int j = 0; j <= lvl.high(); j++) {
			cout << lvl[j] << " ";
		}
		cout << endl;
	}
	*/

	postProcessing_sourceReorder(H, sources);
	m_crossings = H.calculateCrossings();

	OGDF_ASSERT(m_crossings <= UPR.numberOfCrossings());
	OGDF_ASSERT(m_layout.valid());

	GraphCopyAttributes AGC(H, AG);
	m_layout.get().call(H,AG);
	// ********************** end postprocessing *******************************************

	numberOfLevels = H.size();
	m_maxLevelSize = 0;
	for(int i = 0; i <= H.high(); i++) {
		Level &l = H[i];
		if (m_maxLevelSize < l.size())
			m_maxLevelSize = l.size();
	}
}


void LayerBasedUPRLayout::computeRanking(const UpwardPlanRep &UPR, NodeArray<int> &rank)
{
	OGDF_ASSERT(UPR.augmented());

	GraphCopy GC = UPR.original();
	edge e;
	forall_edges(e, UPR.original()) {
		if (UPR.isReversed(e)) {
			GC.reverseEdge(GC.copy(e));
		}
	}

	// compute auxiliary edges
	EdgeArray<int> cost(GC,1);
	List< Tuple2<node, node> > auxEdges;
	NodeArray<int> inL(UPR, -1);
	int num = -1;
	node v;
	forall_nodes(v, UPR) {

		if (UPR.isDummy(v) || v->indeg()==0)
			continue;

		num = num +1;
		//compute all "adjacent" non dummy nodes
		List<node> toDo, srcNodes;
		toDo.pushBack(v);
		inL[v] = num;
		while (!toDo.empty()) {
			node u = toDo.popFrontRet();
			List<edge> inEdges;
			UPR.inEdges(u, inEdges);
			forall_listiterators(edge, it, inEdges) {
				node w = (*it)->source();
				if (UPR.isDummy(w)) {
					if (inL[w] != num) {
						toDo.pushBack(w);
						inL[w] = num;
					}
				}
				else {

					OGDF_ASSERT(UPR.original(w) != 0 && UPR.original(v) != 0);

					node wGC = GC.copy(UPR.original(w));
					node vGC = GC.copy(UPR.original(v));
					edge eNew = GC.newEdge(wGC, vGC);
					cost[eNew] = 0;
				}
			}
		}
	}

	makeSimple(GC);

	OGDF_ASSERT(isAcyclic(GC));


	// ****************************debug*******************************
	/*
	GraphAttributes GA(GC, GraphAttributes::nodeGraphics|
						GraphAttributes::edgeGraphics|
						GraphAttributes::nodeColor|
						GraphAttributes::edgeColor|
						GraphAttributes::nodeLabel|
						GraphAttributes::edgeLabel);
	node vTmp;
	// label the nodes with their index
	forall_nodes(vTmp, GC) {
		char str[255];
		node w = GC.original(vTmp);
		sprintf_s(str, 255, "%d", w->index()); 	// convert to string
		GA.labelNode(vTmp) = str;
	}
	GA.writeGML("c:/temp/ranking_graph.gml");
	*/
	//********************************************************************


	NodeArray<int> ranking(GC, 0);
	EdgeArray<int> length(GC,1);

	m_ranking.get().call(GC, length, cost, ranking);

	// adjust ranking
	int minRank = INT_MAX;
	forall_nodes(v,GC) {
		if(ranking[v] < minRank)
			minRank = ranking[v];
	}

	if(minRank != 0) {
		forall_nodes(v, GC)
			ranking[v] -= minRank;
	}

	forall_nodes(v, GC){
		node vOrig = GC.original(v);
		rank[vOrig] = ranking[v];
	}

	/*
	//debug output ranking
	cout << "Ranking GOrig: " << endl;
	forall_nodes(v, UPR.original())
		cout << "node :" << v << " ranking : "<< rank[v] << endl;
	*/
}



void LayerBasedUPRLayout::postProcessing_sourceReorder(Hierarchy &H, List<node> &sources)
{
	//reorder the sources;
	forall_listiterators(node, it, sources) {
		node s = *it;
		Level &l = H[H.rank(s)];

		//compute the desire position (heuristic)
		int wantedPos = 0;
		adjEntry adj;
		if (s->outdeg() == 1) {
			node tgt =  s->firstAdj()->theEdge()->target();
			List<node> nodes;

			forall_adj(adj, tgt) {
				if (adj->theEdge()->target() == tgt)
					nodes.pushBack(adj->theEdge()->source());
			}

			RankComparer comp;
			comp.H = &H;
			nodes.quicksort(comp);

			//postion of the median
			node v = *nodes.get(nodes.size()/2);
			wantedPos = H.pos(v);
		}
		else {
			List<node> nodes;

			forall_adj(adj, s)
				nodes.pushBack(adj->theEdge()->source());

			RankComparer comp;
			comp.H = &H;
			nodes.quicksort(comp);

			//postion of the median
			node v = *nodes.get(nodes.size()/2);
			wantedPos = H.pos(v);
		}

		//move s to front of the array
		int pos = H.pos(s);
		while (pos != 0) {
			l.swap(pos-1, pos);
			pos--;
		}

		// compute the position of s, which cause min. crossing
		int minPos = pos;
		int oldCr = H.calculateCrossings(l.index());;
		while(pos != l.size()-1) {
			l.swap(pos, pos+1);
			int newCr = H.calculateCrossings(l.index());
			if (newCr <= oldCr) {
				if (newCr < oldCr) {
					minPos = H.pos(s);
					oldCr = newCr;
				}
				else {
					if (abs(minPos - wantedPos) > abs(pos+1 - wantedPos)) {
						minPos = H.pos(s);
						oldCr = newCr;
					}

				}
			}
			pos++;
		}

		//move s to minPos
		while (pos != minPos) {
			if (minPos > pos) {
				l.swap(pos, pos+1);
				pos++;
			}
			if (minPos < pos) {
				l.swap(pos, pos-1);
				pos--;
			}
		}
	}
}



void LayerBasedUPRLayout::postProcessing_markUp(Hierarchy &H, node s, NodeArray<bool> &markedNodes)
{
	const GraphCopy &GC = H;
	NodeArray<bool> inQueue(GC, false);
	Queue<node> nodesToDo;
	nodesToDo.append(s);

	while(!nodesToDo.empty()) {
		node w = nodesToDo.pop();
		markedNodes[w] = true;
		List<edge> outEdges;
		GC.outEdges(w, outEdges);
		ListIterator <edge> it;
		for (it = outEdges.begin(); it.valid(); ++it) {
			edge e = *it;
			if (!inQueue[e->target()] && !markedNodes[e->target()]) { // put the next node in queue if it is not already in queue
				nodesToDo.append( e->target() );
				inQueue[e->target()] = true;
			}
		}
	}
}



void LayerBasedUPRLayout::postProcessing_reduceLED(Hierarchy &H, node s)
{
	const GraphCopy &GC = H;
	NodeArray<bool> markedNodes(GC, false);

	// mark all nodes dominated by s, we call the graph induced by the marked node G*
	// note that not necessary all nodes are marked.
	postProcessing_markUp(H, s, markedNodes);


	for (int i = H.rank(s) + 1; i <= H.high(); i++) {
		const Level &lvl = H[i];

		// Compute the start and end index of the marked graph on this level.
		int minIdx = INT_MAX;
		int maxIdx = -1;
		List<node> sList;

		int numEdges = 0;
		int sumInDeg = 0;
		int numMarkedNodes = 0;
		int numDummies = 0;
		for(int j = 0; j <= lvl.high(); j++) {
			node u = lvl[j];

			if (markedNodes[u]) {
				numMarkedNodes++;

				if (H.isLongEdgeDummy(u))
					numDummies++;

				if (H.pos(u)< minIdx)
					minIdx = H.pos(u);
				if (H.pos(u) > maxIdx)
					maxIdx = H.pos(u);

				sumInDeg += u->indeg();
				adjEntry adj;
				forall_adj(adj, u) {
					if (adj->theEdge()->target()==u && markedNodes[adj->theEdge()->source()])
						numEdges++;
				}
			}
		}
		if (numEdges!=sumInDeg || maxIdx-minIdx+1!=numMarkedNodes )
			return;

		if (numDummies!=numMarkedNodes)
			continue;

		//delete long edge dummies
		for (int k = minIdx; k <= maxIdx; k++) {
			node u = lvl[k];

			OGDF_ASSERT(H.isLongEdgeDummy(u));

			edge inEdge = u->firstAdj()->theEdge();
			edge outEdge = u->lastAdj()->theEdge();
			if (inEdge->target() != u)
				swap(inEdge, outEdge);
			H.m_GC.unsplit(inEdge, outEdge);
		}

		/*
		//debug
		cout << endl << endl;
		cout << "vor :					" << endl;
		for(int ii = 0; ii <= H.high(); ii++) {
			Level &lvl = H[ii];
			cout << endl;
			cout << "level : " << lvl.index() << endl;
			cout << "nodes : ";
			for(int jj = 0; jj <= lvl.high(); jj++) {
				cout << lvl[jj] << "/" << H.pos(lvl[jj]) << "  ";
			}
			cout << endl;
		}
		*/

		post_processing_reduce(H, i, s, minIdx, maxIdx, markedNodes);

		/*
		//debug
		cout << endl << endl;
		cout << endl << endl;
		cout << "nach :					" << endl;
		for(int ii = 0; ii <= H.high(); ii++) {
			Level &lvl = H[ii];
			cout << endl;
			cout << "level : " << lvl.index() << endl;
			cout << "nodes : ";
			for(int jj = 0; jj <= lvl.high(); jj++) {
				cout << lvl[jj] << "/" << H.pos(lvl[jj]) << "  ";
			}
			cout << endl;
		}
		*/

	}
}


void LayerBasedUPRLayout::post_processing_reduce(
	Hierarchy &H,
	int &i,
	node s,
	int minIdx,
	int maxIdx,
	NodeArray<bool> &markedNodes)
{
	const Level &lvl = H[i];

	if (maxIdx-minIdx+1 == lvl.size()) {
		post_processing_deleteLvl(H, i);
		i--;
		return;
	}

	// delete the dummies in interval[minIdx,maxIdx] and copy the nodes in lvl i-1 to lvl i for i={0,..,i-1}
	int startLvl = H.rank(s);
	for (int j = i; j > startLvl; j--) {

		int idxl1 = INT_MAX;
		int idxl2 = INT_MAX;
		int idxh1 = -1;
		int idxh2 = -1;
		for (int k = 0; k<=H[j].high(); k++) {
			node u = H[j][k];

			if (markedNodes[u]) {
				if (k<idxl1)
					idxl1 = k;
				if (k>idxh1)
					idxh1 = k;
			}
		}

		for (int k = 0; k<=H[j-1].high(); k++) {
			node u = H[j-1][k];

			if (markedNodes[u]) {
				if (k<idxl2)
					idxl2 = k;
				if (k>idxh2)
					idxh2 = k;
			}
		}

		int jTmp = j;
		post_processing_deleteInterval(H, idxl1, idxh1, j);
		if (jTmp!=j) {
			i--;
			return; //a level was deleted, we are done
		}

		/*
		//debug
		cout << endl << endl;
		cout << "nach delete :					" << endl;
		for(int ii = 0; ii <= H.high(); ii++) {
			Level &lvl = H[ii];
			cout << endl;
			cout << "level : " << lvl.index() << endl;
			cout << "nodes : ";
			for(int jj = 0; jj <= lvl.high(); jj++) {
				cout << lvl[jj] << "/" << H.pos(lvl[jj]) << "  ";
			}
			cout << endl;
		}
		*/

		post_processing_CopyInterval(H, j, idxl2, idxh2, idxl1);

		/*
		//debug
		cout << endl << endl;
		cout << "nach copy :					" << endl;
		for(int ii = 0; ii <= H.high(); ii++) {
			Level &lvl = H[ii];
			cout << endl;
			cout << "level : " << lvl.index() << endl;
			cout << "nodes : ";
			for(int jj = 0; jj <= lvl.high(); jj++) {
				cout << lvl[jj] << "/" << H.pos(lvl[jj]) << "  ";
			}
			cout << endl;
		}
		*/

	}

	int idxl1 = INT_MAX;
	int idxh1 = -1;
	for (int k = 0; k<=H[startLvl].high(); k++) {
			node u = H[startLvl][k];

			if (markedNodes[u]) {
				if (k<idxl1)
					idxl1 = k;
				if (k>idxh1)
					idxh1 = k;
			}
		}
	int tmp = startLvl;
	post_processing_deleteInterval(H, idxl1, idxh1, startLvl);
	if (tmp!=startLvl)
		i--;
}


void LayerBasedUPRLayout::post_processing_CopyInterval(Hierarchy &H, int i, int beginIdx, int endIdx, int pos)
{
	Level &lvl_cur = H[i];
	int intervalSize = endIdx - beginIdx +1;
	int lastIdx = lvl_cur.high();

	OGDF_ASSERT(intervalSize > 0);

	// grow array
	lvl_cur.m_nodes.grow(intervalSize);
	//move all the data block [pos,lvl_cur.high()] to the end of the array
	for (int k = 0; k < (lastIdx - pos + 1) ; k++) {
		//update position
		H.m_pos[lvl_cur[lastIdx - k]] = lvl_cur.high() - k;
		lvl_cur[lvl_cur.high() - k] = lvl_cur[lastIdx - k];
	}

	/*
	//debug
	cout << endl << endl;
	cout << "level after shift block to end of array : " << lvl.index() << endl;
	cout << "nodes : ";
	for(int j = 0; j <= lvl.high(); j++) {
		cout << lvl[j] << " ; pos() " << pos(lvl[j]) << "  ";
	}
	cout << endl;
	*/

	//copy the nodes of nodeList into the array
	Level &lvl_low = H[i-1];
	int idx = pos;
	for (int k = beginIdx; k <= endIdx; k++) {
		node u = lvl_low[k];
		lvl_cur[idx] = u;
		// update member data
		H.m_pos[u] = idx;
		H.m_rank[u] = lvl_cur.index();
		idx++;
	}
}



void LayerBasedUPRLayout::post_processing_deleteInterval(Hierarchy &H, int beginIdx, int endIdx, int &j)
{
	Level &lvl = H[j];

	int i = 0;
	while ((endIdx + i) < lvl.high()) {
		lvl[beginIdx + i] = lvl[endIdx + i +1];
		H.m_pos[lvl[endIdx + i +1]] = beginIdx + i;
		i++;
	}

	int blockSize = endIdx - beginIdx + 1;

	if (lvl.m_nodes.size()==blockSize) {
		int l = lvl.index();
		post_processing_deleteLvl(H, l); //delete the lvl
		j--;
	}
	else
		lvl.m_nodes.grow(-blockSize); // reduce the size of the lvl
}



void LayerBasedUPRLayout::post_processing_deleteLvl(Hierarchy &H, int i)
{
	//move the pointer to end, then delete the lvl
	int curPos = i;
	while (curPos < H.high()) {
		swap(H.m_pLevel[curPos], H.m_pLevel[curPos+1]);
		Level &lvlTmp = H[curPos];
		lvlTmp.m_index = curPos;
		//update rank
		for(int i = 0; i <= lvlTmp.high(); i++) {
			H.m_rank[lvlTmp[i]] = curPos;
		}
		curPos++;
	}
	//delete
	delete H.m_pLevel[H.high()];
	H.m_pLevel.grow(-1);
}




void LayerBasedUPRLayout::UPRLayoutSimple(const UpwardPlanRep &UPR, GraphAttributes &GA)
{
	//clear some data
	edge e;
	forall_edges(e, GA.constGraph()) {
		GA.bends(e).clear();
	}

	// -------------layout the representation-------------------
	GraphAttributes GA_UPR(UPR);
	node v;
	forall_nodes(v, GA.constGraph()) {
		node vUPR = UPR.copy(v);
		GA_UPR.height(vUPR) = GA.height(v);
		GA_UPR.width(vUPR) = GA.width(v);
	}


	//compute the left edge
	adjEntry adj;
	forall_adj(adj, UPR.getSuperSource()) {
		if (UPR.getEmbedding().rightFace(adj) == UPR.getEmbedding().externalFace())
			break;
	}
	adj = adj->cyclicSucc();
	callSimple(GA_UPR, adj);

	//map to AG
	forall_nodes(v, GA.constGraph()) {
		double vX = GA_UPR.x(UPR.copy(v));
		double vY = GA_UPR.y(UPR.copy(v));
		GA.x(v) = vX;
		GA.y(v) = vY;
	}

	// add bends to original edges
	forall_edges(e, GA.constGraph()) {
		List<edge> chain = UPR.chain(e);
		forall_nonconst_listiterators(edge, it, chain) {
			edge eUPR = *it;
			node tgtUPR = eUPR->target();

			//add bend point of eUPR to original edge
			ListIterator<DPoint> iter;
			DPolyline &line = GA_UPR.bends(eUPR);
			for(iter = line.begin(); iter.valid(); iter++) {
				double x2 = (*iter).m_x;
				double y2 = (*iter).m_y;
				DPoint p(x2, y2);
				GA.bends(e).pushBack(p);
			}
			//add target node of a edge segment as bend point
			if (tgtUPR != chain.back()->target()) {
				double pX = GA_UPR.x(tgtUPR);
				double pY = GA_UPR.y(tgtUPR);
				DPoint p(pX, pY);
				GA.bends(e).pushBack(p);
			}
		}

		DPolyline &poly = GA.bends(e);
		DPoint pSrc(GA.x(e->source()), GA.y(e->source()));
		DPoint pTgt(GA.x(e->target()), GA.y(e->target()));
		poly.normalize(pSrc, pTgt);
	}

	//layers and max. layer size
}


void LayerBasedUPRLayout::callSimple(GraphAttributes &GA, adjEntry adj)
{
	m_numLevels = -1;	//not implemented yet!
	m_maxLevelSize = -1; //not implemented yet!

	const Graph &G = GA.constGraph();

	OGDF_ASSERT(adj->graphOf() == &G);

	// We make a copy stGraph of the input graph G

	GraphCopySimple stGraph(G);

	// determine single source s, single sink t and edge (s,t)
	node s, t;
	hasSingleSource(G, s);
	hasSingleSink(G, t);
	s = stGraph.copy(s);
	t = stGraph.copy(t);

	adjEntry adjCopy = stGraph.copy(adj->theEdge())->adjSource();

	/*cout << "stGraph:" << endl;
	node x;
	forall_nodes(x,stGraph) {
		cout << x << ":";
		edge e;
		forall_adj_edges(e,x)
			cout << " " << e;
		cout << endl;
	}*/

	// For the st-graph, we compute a longest path ranking. Since the graph
	// is st-planar, it is also level planar for the computed rank assignment.
	NodeArray<int> stRank(stGraph);
	longestPathRanking(stGraph,stRank);

	edge e;
	forall_edges(e,stGraph)
		OGDF_ASSERT(stRank[e->source()] < stRank[e->target()]);


	// We translate the rank assignment for stGraph to a rank assignment of G
	// a compute a proper hierarchy for G with this ranking. Since G is a
	// subgraph of stGraph, G is level planar with this ranking.
	NodeArray<int> rank(G);

	node vG;
	forall_nodes(vG,G)
		rank[vG] = stRank[stGraph.copy(vG)];


	/*cout << "rank assignment G:" << endl;
	forall_nodes(vG,G) {
		cout << vG << ": " << rank[vG] << endl;
	}*/

	Hierarchy H(G,rank);

	// GC is the underlying graph of the proper hierarchy H.
	const GraphCopy &GC = H;


	// We compute the positions of the nodes of GC on each level. It is
	// important to determine also the positions of the dummy nodes which
	// resulted from splitting edges. The node array st2GC maps the nodes in
	// stGraph to the nodes in GC.
	NodeArray<node> st2GC(stGraph,0);

	// For nodes representing real nodes in G this is simple.
	forall_nodes(vG,G) {
		OGDF_ASSERT(H.rank(GC.copy(vG)) == stRank[stGraph.copy(vG)]);
		st2GC[stGraph.copy(vG)] = GC.copy(vG);
	}

	// For the dummy nodes, we first have to split edges in stGraph.
	// For an edge e=(v,w), we have to split e stRank[w]-stRank[v]-1 times.
	edge eG;
	forall_edges(eG,G) {
		edge eSt = stGraph.copy(eG);
		const List<edge> &pathGC = GC.chain(eG);

		ListConstIterator<edge> it;
		int r = stRank[eSt->source()];
		for(it = pathGC.begin().succ(); it.valid(); ++it) {
			eSt = stGraph.split(eSt);
			node v = eSt->source();
			node vGC = (*it)->source();
			stRank[v] = ++r;
			st2GC[v] = vGC;

			OGDF_ASSERT(stRank[v] == H.rank(vGC));
		}
	}

	node v;
	forall_nodes(v,stGraph) {
		node vGC = st2GC[v];
		OGDF_ASSERT(vGC == 0 || stRank[v] == H.rank(vGC));
	}

	/*cout << "mapping stGraph -> GC -> G:" << endl;
	node v;
	forall_nodes(v,stGraph)
		cout << v << ": " << st2GC[v] << " " << stGraph.original(v) << endl;*/


	// The array nodes contains the sorted nodes of stGraph on each level.
	Array<SListPure<node> > nodes(stRank[s],stRank[t]);

	dfsSortLevels(adjCopy,stRank,nodes);

	/*for(int i = stRank[s]; i <= stRank[t]; ++i) {
		cout << i << ": ";
		SListPure<node> &L = nodes[i];
		SListConstIterator<node> it;
		for(it = L.begin(); it.valid(); ++it) {
			cout << stGraph.original(*it) << " ";
			node vGC = st2GC[*it];
			OGDF_ASSERT(vGC == 0 || H.rank(vGC) == i);
		}
		cout << endl;
	}*/

	// We translate the node lists to node lists of nodes in GC using node
	// array st2GC. Note that there are also nodes in stGraph which have
	// no counterpart in GC (these are face nodes of the face-sink graph
	// introduced by the augmentation). We can simply ignore such nodes.
	int i;
	for (i = 0; i <= H.high(); ++i) {
		Level &level = H[i];

		//cout << i << endl;
		//cout << level << endl;

		int j = 0;
		SListConstIterator<node> itSt;
		for(itSt = nodes[i].begin(); itSt.valid(); ++itSt) {
			node vGC = st2GC[*itSt];
			if(vGC != 0)
				level[j++] = vGC;
		}

		//cout << level << endl;
		//cout << endl;

		level.recalcPos(); // Recalculate some internal data structures in H
	}

	H.check();


	//cout << "crossings: " << H.calculateCrossings() << endl;
	OGDF_ASSERT(H.calculateCrossings() == 0);

	// Finally, we draw the computed hierarchy applying a hierarchy layout
	// module.
	m_layout.get().call(H,GA);
}


// This procedure computes the sorted nodes lists on each level of an st-graph.
// adj1 corresponds to the leftmost outgoing edge of v = adj1->theNode().
// Levels are build from left to right.
void LayerBasedUPRLayout::dfsSortLevels(
	adjEntry adj1,                   // leftmost outgoing edge
	const NodeArray<int> &rank,      // ranking
	Array<SListPure<node> > &nodes)  // sorted nodes on levels
{
	node v = adj1->theNode();

	nodes[rank[v]].pushBack(v);

	// iterate over all outgoing edges from left to right
	adjEntry adj = adj1;
	do {
		node w = adj->theEdge()->target();
		OGDF_ASSERT(v != w);

		// Is adjW the leftmost outgoing edge of w ?
		adjEntry adjW = adj->twin()->cyclicSucc();
		if(adjW->theEdge()->source() == w)
			dfsSortLevels(adjW,rank,nodes);

		adj = adj->cyclicSucc();
	} while (adj != adj1 && adj->theEdge()->source() == v);
}


// for UPRLayoutSimple
void LayerBasedUPRLayout::longestPathRanking(
	const Graph &G,
	NodeArray<int> &rank)
{
	StackPure<node> sources;
	NodeArray<int> indeg(G);

	node v;
	forall_nodes(v,G) {
		indeg[v] = v->indeg();
		rank[v] = 0;
		if(indeg[v] == 0) {
			sources.push(v);
		}
	}

	while(!sources.empty())
	{
		v = sources.pop();

		edge e;
		forall_adj_edges(e,v) {
			node w = e->target();
			if (w == v) continue;

			if(rank[w] < rank[v]+1)
				rank[w] = rank[v]+1;

			if(--indeg[w] == 0) {
				sources.push(w);
			}
		}
	}
}



}// end namespace ogdf
