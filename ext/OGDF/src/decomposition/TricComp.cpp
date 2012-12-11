/*
 * $Revision: 2552 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-05 16:45:20 +0200 (Do, 05. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implements Hopcroft/Tarjan algorithm for finding the
 * triconnected components of a biconnected multi-graph
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


#include "TricComp.h"
#include <ogdf/basic/GraphCopy.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/NodeSet.h>

//#define TRIC_COMP_OUTPUT


namespace ogdf {


//----------------------------------------------------------
//                      Destructor
//
//  deletes GraphCopy
//----------------------------------------------------------

TricComp::~TricComp()
{
	delete m_pGC;
}


//----------------------------------------------------------
//                      Constructor
//
//  Divides G into triconnected components.
//----------------------------------------------------------

TricComp::TricComp (const Graph& G) :
	m_ESTACK(G.numberOfEdges())
{
	m_pGC = new GraphCopySimple(G);
	GraphCopySimple &GC = *m_pGC;

	const int n = GC.numberOfNodes();
	const int m = GC.numberOfEdges();

#ifdef TRIC_COMP_OUTPUT
	cout << "Dividing G into triconnected components.\n" << endl;
	cout << "n = " << n << ", m = " << m << endl << endl;
#endif

	m_component = Array<CompStruct>(3*m-6);
	m_numComp = 0;

	// special cases
	OGDF_ASSERT(n >= 2);
	OGDF_ASSERT_IF(dlExtendedChecking, isBiconnected(G));

	if (n <= 2) {
		OGDF_ASSERT(m >= 3);
		CompStruct &C = newComp();
		edge e;
		forall_edges(e,GC)
			C << e;
		C.m_type = bond;
		return;
	}

	m_TYPE.init(GC,unseen);
	splitMultiEdges();

	// initialize arrays
	m_NUMBER.init(GC,0); m_LOWPT1.init(GC);
	m_LOWPT2.init(GC);   m_FATHER.init(GC,0);
	m_ND    .init(GC);   m_DEGREE.init(GC);
	m_TREE_ARC.init(GC,0);
	m_NODEAT = Array<node>(1,n);

	m_numCount = 0;
	m_start = GC.firstNode();
	DFS1(GC,m_start,0);

	edge e;
	forall_edges(e,GC) {
		bool up = (m_NUMBER[e->target()] - m_NUMBER[e->source()] > 0);
		if ((up && m_TYPE[e] == frond) || (!up && m_TYPE[e] == tree))
			GC.reverseEdge(e);
	}

#ifdef TRIC_COMP_OUTPUT
	cout << "\nnode\tNUMBER\tFATHER\tLOWPT1\tLOWPT2\tND" << endl;
	node v;
	forall_nodes(v,GC) {
		cout << GC.original(v) << ":  \t" << m_NUMBER[v] << "   \t";
		if (m_FATHER[v] == 0) cout << "nil \t";
		else cout << GC.original(m_FATHER[v]) << "   \t";
		cout << m_LOWPT1[v] << "   \t" << m_LOWPT2[v] << "   \t" << m_ND[v] << endl;
	}
#endif

	m_A.init(GC);
	m_IN_ADJ.init(GC,0);
	buildAcceptableAdjStruct(GC);

#ifdef TRIC_COMP_OUTPUT
	cout << "\nadjaceny lists:" << endl;
	forall_nodes(v,GC) {
		cout << v << "\t";
		ListConstIterator<edge> itE;
		for(itE = m_A[v].begin(); itE.valid(); ++itE) {
			printOs(*itE);
		}
		cout << endl;
	}
#endif

	DFS2(GC);

#ifdef TRIC_COMP_OUTPUT
	cout << "\nnode\tNEWNUM\tLOWPT1\tLOWPT2\tHIGHPT" << endl;
	forall_nodes(v,GC) {
		cout << GC.original(v) << ":  \t" << m_NEWNUM[v] << "   \t";
		cout << m_LOWPT1[v] << "   \t" << m_LOWPT2[v] << "   \t";
		ListConstIterator<int> itH;
		for(itH = m_HIGHPT[v].begin(); itH.valid(); ++itH)
			cout << *itH << " ";
		cout << endl;
	}

	cout << "\nedges starting a path:" << endl;
	forall_edges(e,GC) {
		if (m_START[e]) {
			printOs(e);
		}
	}
#endif


	m_TSTACK_h = new int[2*m+1];
	m_TSTACK_a = new int[2*m+1];
	m_TSTACK_b = new int[2*m+1];
	m_TSTACK_a[m_top = 0] = -1; // start with EOS

	pathSearch(G,m_start);

	// last split component
	CompStruct &C = newComp();
	while(!m_ESTACK.empty()) {
		C << m_ESTACK.pop();
	}
	C.m_type = (C.m_edges.size() > 4) ? triconnected : polygon;

#ifdef TRIC_COMP_OUTPUT
	printStacks();
#endif

	delete [] m_TSTACK_h;
	delete [] m_TSTACK_a;
	delete [] m_TSTACK_b;

	// free resources
	m_NUMBER.init(); m_LOWPT1.init();
	m_LOWPT2.init(); m_FATHER.init();
	m_ND    .init(); m_TYPE  .init();
	m_A     .init(); m_NEWNUM.init();
	m_HIGHPT.init(); m_START .init();
	m_DEGREE.init(); m_TREE_ARC.init();
	m_IN_ADJ.init(); m_IN_HIGH.init();
	m_NODEAT.init();
	m_ESTACK.clear();

	assembleTriconnectedComponents();

	// Caution: checkComp() assumes that the graph is simple!
	//OGDF_ASSERT(checkComp());

#ifdef TRIC_COMP_OUTPUT
	cout << "\n\nTriconnected components:\n";
	for (int i = 0; i < m_numComp; i++) {
		const List<edge> &L = m_component[i].m_edges;
		if (L.size() == 0) continue;
		cout << "[" << i << "] ";
		switch(m_component[i].m_type) {
		case bond: cout << "bond "; break;
		case polygon: cout << "polygon "; break;
		case triconnected: cout << "triconnected "; break;
		}

		ListConstIterator<edge> itE;
		for(itE = L.begin(); itE.valid(); ++itE)
			printOs(*itE);
		cout << "\n";
	}
#endif
}


//----------------------------------------------------------
//                      Constructor
//
// Tests G for triconnectivity and returns a cut vertex in
// s1 or a separation pair in (s1,s2).
//----------------------------------------------------------

TricComp::TricComp(const Graph &G, bool &isTric, node &s1, node &s2)
{
	m_pGC = new GraphCopySimple(G);
	GraphCopySimple &GC = *m_pGC;

	const int n = GC.numberOfNodes();
	const int m = GC.numberOfEdges();

	s1 = s2 = 0;

	if(n == 0) {
		isTric = true; return;
	}

	makeLoopFree(GC);
	makeParallelFreeUndirected(GC);

	// initialize arrays
	m_TYPE.init(GC,unseen);
	m_NUMBER.init(GC,0); m_LOWPT1.init(GC);
	m_LOWPT2.init(GC);   m_FATHER.init(GC,0);
	m_ND    .init(GC);   m_DEGREE.init(GC);
	m_NODEAT.init(1,n);

	m_TREE_ARC.init(GC,0); // probably not required

	m_numCount = 0;
	m_start = GC.firstNode();
	DFS1(GC,m_start,0,s1);

	// graph not even connected?
	if(m_numCount < n) {
		s1 = 0; isTric = false;
		return;
	}

	// graph no biconnected?
	if(s1 != 0) {
		s1 = GC.original(s1);
		isTric = false; // s1 is a cut vertex
		return;
	}

	edge e;
	forall_edges(e,GC) {
		bool up = (m_NUMBER[e->target()] - m_NUMBER[e->source()] > 0);
		if ((up && m_TYPE[e] == frond) || (!up && m_TYPE[e] == tree))
			GC.reverseEdge(e);
	}

	m_A.init(GC);
	m_IN_ADJ.init(GC,0);
	buildAcceptableAdjStruct(GC);

	DFS2(GC);

	m_TSTACK_h = new int[m];
	m_TSTACK_a = new int[m];
	m_TSTACK_b = new int[m];
	m_TSTACK_a[m_top = 0] = -1; // start with EOS

	isTric = pathSearch(G,m_start,s1,s2);
	if(s1) {
		s1 = GC.original(s1);
		s2 = GC.original(s2);
	}

	delete [] m_TSTACK_h;
	delete [] m_TSTACK_a;
	delete [] m_TSTACK_b;

	// free resources
	m_NUMBER.init(); m_LOWPT1.init();
	m_LOWPT2.init(); m_FATHER.init();
	m_ND    .init(); m_TYPE  .init();
	m_A     .init(); m_NEWNUM.init();
	m_HIGHPT.init(); m_START .init();
	m_DEGREE.init(); m_TREE_ARC.init();
	m_IN_ADJ.init(); m_IN_HIGH.init();
	m_NODEAT.init();
}


//----------------------------------------------------------
//                     splitMultiEdges
//
//  Splits bundles of multi-edges into bonds and creates
//  a new virtual edge in GC.
//----------------------------------------------------------

void TricComp::splitMultiEdges()
{
	GraphCopySimple &GC = *m_pGC;

	SListPure<edge> edges;
	EdgeArray<int> minIndex(GC), maxIndex(GC);
	parallelFreeSortUndirected(GC,edges,minIndex,maxIndex);

	SListIterator<edge> it;
	for (it = edges.begin(); it.valid(); ) {
		edge e = *it;
		int minI = minIndex[e], maxI = maxIndex[e];
		++it;
		if (it.valid() && minI == minIndex[*it] && maxI == maxIndex[*it]) {
			CompStruct &C = newComp(bond);
			C << GC.newEdge(e->source(),e->target()) << e << *it;
			m_TYPE[e] = m_TYPE[*it] = removed;

			for (++it; it.valid() &&
				minI == minIndex[*it] && maxI == maxIndex[*it];	++it)
			{
				C << *it;
				m_TYPE[*it] = removed;
			}
		}
	}
}



//----------------------------------------------------------
//                      checkComp
//
//  Checks if computed triconnected components are correct.
//----------------------------------------------------------

bool TricComp::checkSepPair(edge eVirt)
{
	GraphCopySimple G(*m_pGC);

	G.delNode(G.copy(m_pGC->original(eVirt->source())));
	G.delNode(G.copy(m_pGC->original(eVirt->target())));

	return !isConnected(G);
}

bool TricComp::checkComp()
{
	bool ok = true;

	GraphCopySimple &GC = *m_pGC;
	GraphCopySimple GTest(GC.original());

	if (!isLoopFree(GC)) {
		ok = false;
		cout << "GC contains loops!" << endl;
	}

	int i;
	edge e;
	node v;

	EdgeArray<int> count(GC,0);
	for (i = 0; i < m_numComp; i++) {
		ListIterator<edge> itE;
		for(itE = m_component[i].m_edges.begin(); itE.valid(); ++itE)
			count[*itE]++;
	}

	forall_edges(e,GC) {
		if (GC.original(e) == 0) {
			if (count[e] != 2) {
				ok = false;
				cout << "virtual edge contained " << count[e];
				printOs(e); cout << endl;
			}
			if (checkSepPair(e) == false) {
				ok = false;
				cout << "virtual edge"; printOs(e);
				cout << " does not correspond to a sep. pair." << endl;
			}

		} else {
			if (count[e] != 1) {
				ok = false;
				cout << "real edge contained " << count[e];
				printOs(e); cout << endl;
			}
		}
	}

	NodeSet S(GC);
	NodeArray<node> map(GC);

	for(i = 0; i < m_numComp; i++) {
		CompStruct &C = m_component[i];
		const List<edge> &L = C.m_edges;
		if (L.size() == 0) continue;

		S.clear();

		ListConstIterator<edge> itE;
		for(itE = L.begin(); itE.valid(); ++itE) {
			S.insert((*itE)->source());
			S.insert((*itE)->target());
		}

		const int n = S.size();

		switch(C.m_type) {
		case bond:
			if (n != 2) {
				ok = false;
				cout << "bond [" << i << "] with " << n << " nodes!" << endl;
			}
			break;

		case polygon:
			if (n < 3) {
				ok = false;
				cout << "polygon [" << i << "] with " << n << " nodes!" << endl;
			}

			if (L.size() != n) {
				ok = false;
				cout << "polygon [" << i << "] with " << n << " vertices and " << L.size() << " edges!" << endl;

			} else {
				Graph Gp;
				ListConstIterator<node> itV;
				for(itV = S.nodes().begin(); itV.valid(); ++itV)
					map[*itV] = Gp.newNode();
				for(itE = L.begin(); itE.valid(); ++itE)
					Gp.newEdge(map[(*itE)->source()],map[(*itE)->target()]);

				forall_nodes(v,Gp)
					if (v->degree() != 2) {
						ok = false;
						cout << "polygon [" << i << "] contains node with degree " << v->degree() << endl;
					}
					if (!isConnected(Gp)) {
						ok = false;
						cout << "polygon [" << i << "] not connected." << endl;
					}
			}
			break;

		case triconnected:
			if (n < 4) {
				ok = false;
				cout << "triconnected component [" << i << "] with " << n << " nodes!" << endl;
			}

			{
			Graph Gp;
			ListConstIterator<node> itV;
			for(itV = S.nodes().begin(); itV.valid(); ++itV)
				map[*itV] = Gp.newNode();
			for(itE = L.begin(); itE.valid(); ++itE)
				Gp.newEdge(map[(*itE)->source()],map[(*itE)->target()]);

			if (!isTriconnectedPrimitive(Gp)) {
				ok = false;
				cout << "component [" << i << "] not triconnected!" << endl;
			}
			if (!isSimple(Gp)) {
				ok = false;
				cout << "triconnected component [" << i << "] not simple!" << endl;
			}
			}
			break;

		default:
			ok = false;
			cout << "component [" << i << "] with undefined type!" << endl;
		}
	}

	return ok;
}


//----------------------------------------------------------
//               assembleTriconnectedComponents
//
//  joins bonds and polygons with common virtual edge in
//  order to build the triconnected components.
//----------------------------------------------------------

void TricComp::assembleTriconnectedComponents()
{
	GraphCopySimple &GC = *m_pGC;

	EdgeArray<int>       comp1(GC), comp2(GC);
	EdgeArray<ListIterator<edge> > item1(GC,ListIterator<edge>());
	EdgeArray<ListIterator<edge> > item2(GC);

	bool *visited = new bool[m_numComp];

	int i;
	for(i = 0; i < m_numComp; i++) {
		visited[i] = false;
		List<edge> &L = m_component[i].m_edges;

		ListIterator<edge> it;
		for(it = L.begin(); it.valid(); ++it) {
			edge e = *it;
			if (!item1[e].valid()) {
				comp1[e] = i; item1[e] = it;
			} else {
				comp2[e] = i; item2[e] = it;
			}
		}
	}

	for(i = 0; i < m_numComp; i++) {
		CompStruct &C1 = m_component[i];
		List<edge> &L1 = C1.m_edges;
		visited[i] = true;

		if (L1.size() == 0) continue;

		if (C1.m_type == polygon || C1.m_type == bond) {
			ListIterator<edge> it, itNext;
			for(it = L1.begin(); it.valid(); it = itNext) {
				itNext = it.succ();
				edge e  = *it;

				if (GC.original(e) != 0) continue;

				int j = comp1[e];
				ListIterator<edge> it2;
				if (visited[j]) {
					j = comp2[e];
					if (visited[j]) continue;
					it2 = item2[e];
				} else
					it2 = item1[e];

				CompStruct &C2 = m_component[j];

				if (C2.m_type != C1.m_type) continue;

				visited[j] = true;
				List<edge> &L2 = C2.m_edges;

				L2.del(it2);
				L1.conc(L2);
				if (!itNext.valid())
					itNext = it.succ();
				L1.del(it);

				GC.delEdge(e);
			}
		}
	}

	delete [] visited;
}



//----------------------------------------------------------
//                   The first dfs-search
//
//  computes NUMBER[v], FATHER[v], LOWPT1[v], LOWPT2[v],
//           ND[v], TYPE[e], DEGREE[v]
//----------------------------------------------------------

void TricComp::DFS1 (const Graph& G, node v, node u)
{
	edge e;

	m_NUMBER[v] = ++m_numCount;
	m_FATHER[v] = u;
	m_DEGREE[v] = v->degree();

	m_LOWPT1[v] = m_LOWPT2[v] = m_NUMBER[v];
	m_ND[v] = 1;

	forall_adj_edges (e,v) {

		if (m_TYPE[e] != unseen)
			continue;

		node w = e->opposite(v);

		if (m_NUMBER[w] == 0) {
			m_TYPE[e] = tree;

			m_TREE_ARC[w] = e;

			DFS1(G,w,v);

			if (m_LOWPT1[w] < m_LOWPT1[v]) {
				m_LOWPT2[v] = min(m_LOWPT1[v],m_LOWPT2[w]);
				m_LOWPT1[v] = m_LOWPT1[w];

			} else if (m_LOWPT1[w] == m_LOWPT1[v]) {
				m_LOWPT2[v] = min(m_LOWPT2[v],m_LOWPT2[w]);

			} else {
				m_LOWPT2[v] = min(m_LOWPT2[v],m_LOWPT1[w]);
			}

			m_ND[v] += m_ND[w];

		} else {

			m_TYPE[e] = frond;

			if (m_NUMBER[w] < m_LOWPT1[v]) {
				m_LOWPT2[v] = m_LOWPT1[v];
				m_LOWPT1[v] = m_NUMBER[w];

			} else if (m_NUMBER[w] > m_LOWPT1[v]) {
				m_LOWPT2[v] = min(m_LOWPT2[v],m_NUMBER[w]);
			}
		}
	}
}

void TricComp::DFS1 (const Graph& G, node v, node u, node &s1)
{
	node firstSon = 0;
	edge e;

	m_NUMBER[v] = ++m_numCount;
	m_FATHER[v] = u;
	m_DEGREE[v] = v->degree();

	m_LOWPT1[v] = m_LOWPT2[v] = m_NUMBER[v];
	m_ND[v] = 1;

	forall_adj_edges (e,v) {

		if (m_TYPE[e] != unseen)
			continue;

		node w = e->opposite(v);

		if (m_NUMBER[w] == 0) {
			m_TYPE[e] = tree;
			if(firstSon == 0) firstSon = w;

			m_TREE_ARC[w] = e;

			DFS1(G,w,v,s1);

			// check for cut vertex
			if(m_LOWPT1[w] >= m_NUMBER[v] && (w != firstSon || u != 0))
				s1 = v;

			if (m_LOWPT1[w] < m_LOWPT1[v]) {
				m_LOWPT2[v] = min(m_LOWPT1[v],m_LOWPT2[w]);
				m_LOWPT1[v] = m_LOWPT1[w];

			} else if (m_LOWPT1[w] == m_LOWPT1[v]) {
				m_LOWPT2[v] = min(m_LOWPT2[v],m_LOWPT2[w]);

			} else {
				m_LOWPT2[v] = min(m_LOWPT2[v],m_LOWPT1[w]);
			}

			m_ND[v] += m_ND[w];

		} else {

			m_TYPE[e] = frond;

			if (m_NUMBER[w] < m_LOWPT1[v]) {
				m_LOWPT2[v] = m_LOWPT1[v];
				m_LOWPT1[v] = m_NUMBER[w];

			} else if (m_NUMBER[w] > m_LOWPT1[v]) {
				m_LOWPT2[v] = min(m_LOWPT2[v],m_NUMBER[w]);
			}
		}
	}
}


//----------------------------------------------------------
//           Construction of ordered adjaceny lists
//----------------------------------------------------------

void TricComp::buildAcceptableAdjStruct(const Graph& G)
{
	edge e;
	int max = 3*G.numberOfNodes()+2;
	Array<List<edge> > BUCKET(1,max);

	forall_edges(e,G) {
		edgeType t = m_TYPE[e];
		if (t == removed) continue;

		node w = e->target();
		int phi = (t == frond) ? 3*m_NUMBER[w]+1 : (
			(m_LOWPT2[w] < m_NUMBER[e->source()]) ? 3*m_LOWPT1[w] :
			3*m_LOWPT1[w]+2);
		BUCKET[phi].pushBack(e);
	}

	for (int i = 1; i <= max; i++) {
		ListConstIterator<edge> it;
		for(it = BUCKET[i].begin(); it.valid(); ++it) {
			e = *it;
			m_IN_ADJ[e] = m_A[e->source()].pushBack(e);
		}
	}
}


//----------------------------------------------------------
//                   The second dfs-search
//----------------------------------------------------------

void TricComp::pathFinder(const Graph& G, node v)
{
	m_NEWNUM[v] = m_numCount - m_ND[v] + 1;

	ListConstIterator<edge> it;
	for(it = m_A[v].begin(); it.valid(); ++it) {
		edge e = *it;
		node w = e->opposite(v);

		if (m_newPath) {
			m_newPath = false;
			m_START[e] = true;
		}

		if (m_TYPE[e] == tree) {
			pathFinder(G,w);
			m_numCount--;

		} else {
			m_IN_HIGH[e] = m_HIGHPT[w].pushBack(m_NEWNUM[v]);
			m_newPath = true;
		}
	}
}

void TricComp::DFS2 (const Graph& G)
{
	m_NEWNUM .init(G,0);
	m_HIGHPT .init(G);
	m_IN_HIGH.init(G,ListIterator<int>());
	m_START  .init(G,false);

	m_numCount = G.numberOfNodes();
	m_newPath = true;

	pathFinder(G,m_start);

	node v;
	Array<int> old2new(1,G.numberOfNodes());

	forall_nodes(v,G)
		old2new[m_NUMBER[v]] = m_NEWNUM[v];

	forall_nodes(v,G) {
		m_NODEAT[m_NEWNUM[v]] = v;
		m_LOWPT1[v] = old2new[m_LOWPT1[v]];
		m_LOWPT2[v] = old2new[m_LOWPT2[v]];
	}
}


//----------------------------------------------------------
//                      pathSearch()
//
// recognition of split components
//----------------------------------------------------------

void TricComp::pathSearch (const Graph& G, node v)
{
	node w;
	edge e;
	int y, vnum = m_NEWNUM[v], wnum;
	int a, b;

	List<edge> &Adj = m_A[v];
	int outv = Adj.size();

	ListIterator<edge> it, itNext;
	for(it = Adj.begin(); it.valid(); it=itNext)
	{
		itNext = it.succ();
		e = *it;
		w = e->target(); wnum = m_NEWNUM[w];

		if (m_TYPE[e] == tree) {

			if (m_START[e]) {
				y = 0;
				if (m_TSTACK_a[m_top] > m_LOWPT1[w]) {
					do {
						y = max(y,m_TSTACK_h[m_top]);
						b = m_TSTACK_b[m_top--];
					} while (m_TSTACK_a[m_top] > m_LOWPT1[w]);
					TSTACK_push(y,m_LOWPT1[w],b);
				} else {
					TSTACK_push(wnum+m_ND[w]-1,m_LOWPT1[w],vnum);
				}
				TSTACK_pushEOS();
			}

			pathSearch(G,w);

			m_ESTACK.push(m_TREE_ARC[w]);  // add (v,w) to ESTACK (can differ from e!)

			node x;

			while (vnum != 1 && ((m_TSTACK_a[m_top] == vnum) ||
				(m_DEGREE[w] == 2 && m_NEWNUM[m_A[w].front()->target()] > wnum)))
			{
				a = m_TSTACK_a[m_top];
				b = m_TSTACK_b[m_top];

				edge eVirt;

				if (a == vnum && m_FATHER[m_NODEAT[b]] == m_NODEAT[a]) {
					m_top--;
				}

				else {
					edge e_ab = 0;

					if (m_DEGREE[w] == 2 && m_NEWNUM[m_A[w].front()->target()] > wnum) {
#ifdef TRIC_COMP_OUTPUT
						cout << endl << "\nfound type-2 separation pair " <<
							m_pGC->original(v) << ", " <<
							m_pGC->original(m_A[w].front()->target());
#endif

						edge e1 = m_ESTACK.pop();
						edge e2 = m_ESTACK.pop();
						m_A[w].del(m_IN_ADJ[e2]);

						x = e2->target();

						eVirt = m_pGC->newEdge(v,x);
						m_DEGREE[x]--; m_DEGREE[v]--;

						OGDF_ASSERT(e2->source() == w);
						CompStruct &C = newComp(polygon);
						C << e1 << e2 << eVirt;

						if (!m_ESTACK.empty()) {
							e1 = m_ESTACK.top();
							if (e1->source() == x && e1->target() == v) {
								e_ab = m_ESTACK.pop();
								m_A[x].del(m_IN_ADJ[e_ab]);
								delHigh(e_ab);
							}
						}

					} else {
#ifdef TRIC_COMP_OUTPUT
						cout << "\nfound type-2 separation pair " <<
							m_pGC->original(m_NODEAT[a]) << ", " <<
							m_pGC->original(m_NODEAT[b]);
#endif

						int h = m_TSTACK_h[m_top--];

						CompStruct &C = newComp();
						while(true) {
							edge xy = m_ESTACK.top();
							x = xy->source(); node y = xy->target();
							if (!(a <= m_NEWNUM[x] && m_NEWNUM[x] <= h &&
								a <= m_NEWNUM[y] && m_NEWNUM[y] <= h)) break;

							if ((m_NEWNUM[x] == a && m_NEWNUM[y] == b) ||
								(m_NEWNUM[y] == a && m_NEWNUM[x] == b))
							{
								e_ab = m_ESTACK.pop();
								m_A[e_ab->source()].del(m_IN_ADJ[e_ab]);
								delHigh(e_ab);

							} else {
								edge eh = m_ESTACK.pop();
								if (it != m_IN_ADJ[eh]) {
									m_A[eh->source()].del(m_IN_ADJ[eh]);
									delHigh(eh);
								}
								C << eh;
								m_DEGREE[x]--; m_DEGREE[y]--;
							}
						}

						eVirt = m_pGC->newEdge(m_NODEAT[a],m_NODEAT[b]);
						C.finishTricOrPoly(eVirt);
						x = m_NODEAT[b];
					}

					if (e_ab != 0) {
						CompStruct &C = newComp(bond);
						C << e_ab << eVirt;

						eVirt = m_pGC->newEdge(v,x);
						C << eVirt;

						m_DEGREE[x]--; m_DEGREE[v]--;
					}

					m_ESTACK.push(eVirt);
					*it = eVirt;
					m_IN_ADJ[eVirt] = it;

					m_DEGREE[x]++; m_DEGREE[v]++;
					m_FATHER[x] = v;
					m_TREE_ARC[x] = eVirt;
					m_TYPE[eVirt] = tree;

					w = x; wnum = m_NEWNUM[w];
				}
			}

			if (m_LOWPT2[w] >= vnum && m_LOWPT1[w] < vnum && (m_FATHER[v] != m_start || outv >= 2))
			{
#ifdef TRIC_COMP_OUTPUT
				cout << "\nfound type-1 separation pair " <<
					m_pGC->original(m_NODEAT[m_LOWPT1[w]]) << ", " <<
					m_pGC->original(v);
#endif

				CompStruct &C = newComp();
				edge xy;
				int x;
				while (!m_ESTACK.empty()) {
					xy = m_ESTACK.top();
					x = m_NEWNUM[xy->source()];
					y = m_NEWNUM[xy->target()];

					if (!((wnum <= x && x < wnum+m_ND[w]) || (wnum <= y && y < wnum+m_ND[w])))
						break;

					C << m_ESTACK.pop();
					delHigh(xy);
					m_DEGREE[m_NODEAT[x]]--; m_DEGREE[m_NODEAT[y]]--;
				}

				edge eVirt = m_pGC->newEdge(v,m_NODEAT[m_LOWPT1[w]]);
				C.finishTricOrPoly(eVirt);

				if ((x == vnum && y == m_LOWPT1[w]) || (y == vnum && x == m_LOWPT1[w])) {
					CompStruct &C = newComp(bond);
					edge eh = m_ESTACK.pop();
					if (m_IN_ADJ[eh] != it) {
						m_A[eh->source()].del(m_IN_ADJ[eh]);
					}
					C << eh << eVirt;
					eVirt = m_pGC->newEdge(v,m_NODEAT[m_LOWPT1[w]]);
					C << eVirt;
					m_IN_HIGH[eVirt] = m_IN_HIGH[eh];
					m_DEGREE[v]--;
					m_DEGREE[m_NODEAT[m_LOWPT1[w]]]--;
				}

				if (m_NODEAT[m_LOWPT1[w]] != m_FATHER[v]) {
					m_ESTACK.push(eVirt);
					*it = eVirt;
					m_IN_ADJ[eVirt] = it;
					if (!m_IN_HIGH[eVirt].valid() && high(m_NODEAT[m_LOWPT1[w]]) < vnum)
						m_IN_HIGH[eVirt] = m_HIGHPT[m_NODEAT[m_LOWPT1[w]]].pushFront(vnum);

					m_DEGREE[v]++;
					m_DEGREE[m_NODEAT[m_LOWPT1[w]]]++;

				} else {
					Adj.del(it);

					CompStruct &C = newComp(bond);
					C << eVirt;
					eVirt = m_pGC->newEdge(m_NODEAT[m_LOWPT1[w]],v);
					C << eVirt;

					edge eh = m_TREE_ARC[v];

					C << m_TREE_ARC[v];

					m_TREE_ARC[v] = eVirt;
					m_TYPE[eVirt] = tree;

					m_IN_ADJ[eVirt] = m_IN_ADJ[eh];
					*m_IN_ADJ[eh] = eVirt;
				}
			}

			if (m_START[e]) {
				while (TSTACK_notEOS()) {
					m_top--;
				}
				m_top--;
			}

			while (TSTACK_notEOS() &&
				m_TSTACK_b[m_top] != vnum && high(v) > m_TSTACK_h[m_top]) {
				m_top--;
			}

			outv--;

		} else { // frond arc
			if (m_START[e]) {
				y = 0;
				if (m_TSTACK_a[m_top] > wnum) {
					do {
						y = max(y,m_TSTACK_h[m_top]);
						b = m_TSTACK_b[m_top--];
					} while (m_TSTACK_a[m_top] > wnum);
					TSTACK_push(y,wnum,b);
				} else {
					TSTACK_push(vnum,wnum,vnum);
				}
			}

			m_ESTACK.push(e);  // add (v,w) to ESTACK
		}
	}
}

// simplified path search for triconnectivity test
bool TricComp::pathSearch (const Graph &G, node v, node &s1, node &s2)
{
	node w;
	edge e;
	int y, vnum = m_NEWNUM[v], wnum;
	int a, b;

	List<edge> &Adj = m_A[v];
	int outv = Adj.size();

	ListIterator<edge> it, itNext;
	for(it = Adj.begin(); it.valid(); it=itNext)
	{
		itNext = it.succ();
		e = *it;
		w = e->target(); wnum = m_NEWNUM[w];

		if (m_TYPE[e] == tree) {

			if (m_START[e]) {
				y = 0;
				if (m_TSTACK_a[m_top] > m_LOWPT1[w]) {
					do {
						y = max(y,m_TSTACK_h[m_top]);
						b = m_TSTACK_b[m_top--];
					} while (m_TSTACK_a[m_top] > m_LOWPT1[w]);
					TSTACK_push(y,m_LOWPT1[w],b);
				} else {
					TSTACK_push(wnum+m_ND[w]-1,m_LOWPT1[w],vnum);
				}
				TSTACK_pushEOS();
			}

			if(pathSearch(G,w,s1,s2) == false)
				return false;

			while (vnum != 1 && ((m_TSTACK_a[m_top] == vnum) ||
				(m_DEGREE[w] == 2 && m_NEWNUM[m_A[w].front()->target()] > wnum)))
			{
				a = m_TSTACK_a[m_top];
				b = m_TSTACK_b[m_top];

				if (a == vnum && m_FATHER[m_NODEAT[b]] == m_NODEAT[a]) {
					m_top--;

				} else if (m_DEGREE[w] == 2 && m_NEWNUM[m_A[w].front()->target()] > wnum)
				{
					s1 = v;
					s2 = m_A[w].front()->target();
					return false;

				} else {
					s1 = m_NODEAT[a];
					s2 = m_NODEAT[b];
					return false;
				}
			}

			if (m_LOWPT2[w] >= vnum && m_LOWPT1[w] < vnum && (m_FATHER[v] != m_start || outv >= 2))
			{
				s1 = m_NODEAT[m_LOWPT1[w]];
				s2 = v;
				return false;
			}

			if (m_START[e]) {
				while (TSTACK_notEOS()) {
					m_top--;
				}
				m_top--;
			}

			while (TSTACK_notEOS() &&
				m_TSTACK_b[m_top] != vnum && high(v) > m_TSTACK_h[m_top]) {
				m_top--;
			}

			outv--;

		} else { // frond arc
			if (m_START[e]) {
				y = 0;
				if (m_TSTACK_a[m_top] > wnum) {
					do {
						y = max(y,m_TSTACK_h[m_top]);
						b = m_TSTACK_b[m_top--];
					} while (m_TSTACK_a[m_top] > wnum);
					TSTACK_push(y,wnum,b);
				} else {
					TSTACK_push(vnum,wnum,vnum);
				}
			}
		}
	}

	return true;
}


// triconnectivity test
bool isTriconnected(const Graph &G, node &s1, node &s2)
{
	bool isTric;
	TricComp tric2(G,isTric,s1,s2);

	return isTric;
}


//----------------------------------------------------------
//                       debugging stuff
//----------------------------------------------------------

void TricComp::printOs(edge e)
{
#ifdef TRIC_COMP_OUTPUT
	cout << " (" << m_pGC->original(e->source()) << "," <<
		m_pGC->original(e->target()) << "," << e->index() << ")";
	if (m_pGC->original(e) == 0) cout << "v";
#endif
}

void TricComp::printStacks()
{
#ifdef TRIC_COMP_OUTPUT
	cout << "\n\nTSTACK:" << endl;

	for (int i = m_top; i >= 0; i--)
		cout << "(" << m_TSTACK_h[i] << "," << m_TSTACK_a[i] << "," << m_TSTACK_b[i] << ")\n";

	cout << "\nESTACK\n";
	while(!m_ESTACK.empty()) {
		printOs(m_ESTACK.pop()); cout << endl;
	}
#endif
}


} // end namespace ogdf

