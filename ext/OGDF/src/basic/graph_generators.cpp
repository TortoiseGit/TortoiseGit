/*
 * $Revision: 2599 $
 *
 * last checkin:
 *   $Author: chimani $
 *   $Date: 2012-07-15 22:39:24 +0200 (So, 15. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of some graph generators
 *
 * \author Carsten Gutwenger, Markus Chimani
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


#include <ogdf/basic/graph_generators.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/CombinatorialEmbedding.h>
#include <ogdf/basic/EdgeArray.h>
#include <ogdf/basic/NodeArray.h>
#include <ogdf/basic/FaceArray.h>
#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/basic/Array2D.h>


namespace ogdf {


inline int __IDX(int a, int b, int n, int max) {
	int t = n - a - 1;
	int s = t * (t+1) / 2;
	int w = max - s;
	int o = b - a - 1;
	return w + o;
}

void randomGraph(Graph &G, int n, int m)
{
	G.clear();

	Array<node> v(n);

	int i;
	for(i = 0; i < n; i++)
		v[i] = G.newNode();

	for(i = 0; i < m; i++) {
		int v1 = randomNumber(0,n-1);
		int v2 = randomNumber(0,n-1);

		G.newEdge(v[v1],v[v2]);
	}
}

bool randomSimpleGraph(Graph &G, int n, int m)
{
	G.clear();

	if(n < 1)
		return false;

	int max = n * (n-1) / 2;
	if(m > max || m < n)
		return false;

	Array<node> v(n);

	int i;
	for(i = 0; i < n; i++)
		v[i] = G.newNode();

	bool remove;
	if(m > max /2) {
		m = max - m;
		remove = true;
	} else {
		remove = false;
	}

	Array<bool> used(max);
	for(i = max; i-->0;)
		used[i] = remove;

	int a,b;
	while(m > 0) {
		a = randomNumber(0,n-1);
		b = randomNumber(0,n-2);
		if(b >= a) b++;
		else {
			int c = a;
			a = b;
			b = c;
		}
		if(used[i = __IDX(a,b,n,max)] == remove) {
			used[i] = !remove;
			m--;
		}
	}

	for(a = 0; a < n; a++)
		for(b = a+1; b < n; b++)
			if(used[__IDX(a,b,n,max)])
				G.newEdge(v[a],v[b]);

	return true;
}


void randomTree(Graph &G, int n, int maxDeg, int maxWidth)
{
	G.clear();

	if (n <= 0) return;
	if (maxDeg   <= 0) maxDeg   = n;
	if (maxWidth <= 0) maxWidth = n;

	int max = 0;
	Array<node> possible(n);
	Array<int> width(0,n,0);
	NodeArray<int> level(G,0);

	level[possible[0] = G.newNode()] = 0;
	--n;

	while(n > 0) {
		int  i = randomNumber(0,max);
		node v = possible[i];

		if (width[level[v]+1] == maxWidth) {
			possible[i] = possible[max--];
			continue;
		}

		if (v->outdeg()+1 == maxDeg)
			possible[i] = possible[max--];

		node w = G.newNode();
		possible[++max] = w;
		G.newEdge(v,w);
		width[level[w] = level[v]+1]++;

		--n;
	}
}


void randomBiconnectedGraph(Graph &G, int n, int m)
{
	if (n < 3) n = 3;
	if (m < n) m = n;

	int kse = n-3; // number of split edge operations
	int kae = m-n; // number of add edge operations

	G.clear();

	Array<edge> edges(m);
	Array<node> nodes(n);

	// we start with a triangle
	nodes[0] = G.newNode();
	nodes[1] = G.newNode();
	nodes[2] = G.newNode();
	edges[0] = G.newEdge(nodes[0],nodes[1]);
	edges[1] = G.newEdge(nodes[1],nodes[2]);
	edges[2] = G.newEdge(nodes[2],nodes[0]);

	int nNodes = 3, nEdges = 3;

	while(kse+kae > 0)
	{
		int p = randomNumber(1,kse+kae);

		if (p <= kse) {
			// split edge operation
			edge e = edges[randomNumber(0,nEdges-1)];
			edge e1 = G.split(e);

			edges[nEdges++] = e1;
			nodes[nNodes++] = e1->source();

			--kse;

		} else {
			// add edge operation
			int i = randomNumber(0,nNodes-1);
			int j = (i + randomNumber(1,nNodes-1)) % nNodes;

			edges[nEdges++] = G.newEdge(nodes[i], nodes[j]);

			--kae;
		}
	}
}


void randomTriconnectedGraph(Graph &G, int n, double p1, double p2)
{
	if(n < 4) n = 4;

	// start with K_4
	completeGraph(G,4);

	// nodes[0],...,nodes[i-1] is array of all nodes
	Array<node> nodes(n);

	node v;
	int i = 0;
	forall_nodes(v,G)
		nodes[i++] = v;

	// Will be used below as array of neighbors of v
	Array<edge> neighbors(n);

	// used to mark neighbors
	//   0 = not marked
	//   1 = marked left
	//   2 = marked right
	//   3 = marked both
	Array<int>  mark(0,n-1,0);

	for(; i < n; ++i)
	{
		// pick a random node
		v = nodes[randomNumber(0,i-1)];

		// create a new node w such that v is split into v and w
		node w = nodes[i] = G.newNode();

		// build array of all neighbors
		int d = v->degree();

		int j = 0;
		adjEntry adj;
		forall_adj(adj,v)
			neighbors[j++] = adj->theEdge();

		// mark two distinct neighbors for left
		for(j = 2; j > 0; ) {
			int r = randomNumber(0,d-1);
			if((mark[r] & 1) == 0) {
				mark[r] |= 1; --j;
			}
		}

		// mark two distinct neighbors for right
		for(j = 2; j > 0; ) {
			int r = randomNumber(0,d-1);
			if((mark[r] & 2) == 0) {
				mark[r] |= 2; --j;
			}
		}

		for(j = 0; j < d; ++j) {
			int m = mark[j];
			mark[j] = 0;

			// decide to with which node each neighbor is connected
			// (possible: v, w, or both)
			double x = randomDouble(0.0,1.0);
			switch(m)
			{
			case 0:
				if(x < p1)
					m = 1;
				else if(x < p1+p2)
					m = 2;
				else
					m = 3;
				break;
			case 1:
			case 2:
				if(x >= p1+p2) m = 3;
				break;
			}

			// move edge or create new one if necessary
			edge e = neighbors[j];
			switch(m)
			{
			case 2:
				if(v == e->source())
					G.moveSource(e,w);
				else
					G.moveTarget(e,w);
				break;
			case 3:
				G.newEdge(w,e->opposite(v));
				break;
			}
		}

		G.newEdge(v,w);
	}
}


void planarTriconnectedGraph(Graph &G, int n, double p1, double p2)
{
	if (n < 4) n = 4;

	// start with K_4
	completeGraph(G,4);

	planarEmbedPlanarGraph(G);

	// nodes[0],...,nodes[i-1] is array of all nodes
	Array<node> nodes(n);

	node v;
	int i = 0;
	forall_nodes(v,G)
		nodes[i++] = v;

	for(; i < n; ++i)
	{
		// pick a random node
		v = nodes[randomNumber(0,i-1)];

		int m = v->degree();
		int a1 = randomNumber(0,m-1);
		int a2 = randomNumber(0,m-2);

		int j;
		adjEntry adj1, adj2;
		for(adj1 = v->firstAdj(), j = 0; j < a1; adj1 = adj1->succ(), ++j) ;
		for(adj2 = adj1->cyclicSucc(), j = 0; j < a2; adj2 = adj2->cyclicSucc(), ++j) ;

		adjEntry adj_b1 = adj2->cyclicPred();
		adjEntry adj_b2 = adj1->cyclicPred();

		nodes[i] = G.splitNode(adj1, adj2);

		if(adj1 == adj_b1)
			G.newEdge(adj_b1, adj2->twin());
		else if(adj2 == adj_b2)
			G.newEdge(adj2, adj_b1->twin(), ogdf::before);
		else {
			double r = randomDouble(0.0,1.0);
			if(r <= p1) {
				int s = randomNumber(0,1);
				if(s == 0)
					G.newEdge(adj_b1, adj2->twin());
				else
					G.newEdge(adj2, adj_b1->twin(), ogdf::before);
			}
		}

		double r = randomDouble(0.0,1.0);
		if(r <= p2) {
			int s = randomNumber(0,1);
			if(s == 0)
				G.newEdge(adj1, adj_b2->twin(), ogdf::before);
			else
				G.newEdge(adj_b2, adj1->twin());
		}
	}
}


void planarTriconnectedGraph(Graph &G, int n, int m)
{
	if (n < 4) n = 4;
	if(n % 2) ++n; // need an even number

	// start with K_4
	completeGraph(G,4);

	planarEmbedPlanarGraph(G);

	// nodes[0],...,nodes[i-1] is array of all nodes
	Array<node> nodes(n);

	node v;
	int i = 0;
	forall_nodes(v,G)
		nodes[i++] = v;

	// create planar triconnected 3-graph
	for(; i < n; )
	{
		// pick a random node
		v = nodes[randomNumber(0,i-1)];

		adjEntry adj2 = v->firstAdj();
		int r = randomNumber(0,2);
		switch(r) {
			case 2: adj2 = adj2->succ(); // fall through to next case
			case 1: adj2 = adj2->succ();
		}
		adjEntry adj1 = adj2->cyclicSucc();

		nodes[i++] = G.splitNode(adj1,adj2);

		r = randomNumber(0,1);
		if(r == 0) {
			adjEntry adj = adj1->twin();
			G.newEdge(adj2,adj);
			nodes[i++] = G.splitNode(adj,adj->cyclicSucc()->cyclicSucc());

		} else {
			adjEntry adj = adj1->cyclicSucc()->twin();
			G.newEdge(adj2,adj,ogdf::before);
			nodes[i++] = G.splitNode(adj->cyclicPred(),adj->cyclicSucc());
		}
	}

	nodes.init();
	Array<edge> edges(m);

	CombinatorialEmbedding E(G);
	Array<face> faces(2*n);

	i = 0;
	face f;
	forall_faces(f,E) {
		if(f->size() >= 4)
			faces[i++] = f;
	}

	while(G.numberOfEdges() < m && i > 0)
	{
		int r = randomNumber(0,i-1);
		f = faces[r];
		faces[r] = faces[--i];

		int p = randomNumber(0,f->size()-1);
		int j = 0;
		adjEntry adj, adj2;
		for(adj = f->firstAdj(); j < p; adj = adj->faceCycleSucc(), ++j) ;

		p = randomNumber(2, f->size()-2);
		for(j = 0, adj2 = adj; j < p; adj2 = adj2->faceCycleSucc(), ++j) ;

		edge e = E.splitFace(adj,adj2);

		f = E.rightFace(e->adjSource());
		if(f->size() >= 4) faces[i++] = f;

		f = E.rightFace(e->adjTarget());
		if(f->size() >= 4) faces[i++] = f;
	}
}

void planarConnectedGraph(Graph &G, int n, int m)
{
	if (n < 1) n = 1;
	if (m < n-1) m = n-1;
	if (m > 3*n-6) m = 3*n-6;

	G.clear();
	Array<node> nodes(n);

	// we start with a triangle
	nodes[0] = G.newNode();

	//build tree
	int i;
	for(i=1; i<n; ++i) {
		node on = nodes[randomNumber(0,i-1)];
		node nn = nodes[i] = G.newNode();
		G.firstNode()->degree();
		if(on->degree() > 1) {
			adjEntry adj = on->firstAdj();
			for(int fwd = randomNumber(0,on->degree()-1); fwd>0; --fwd)
				adj = adj->succ();
			G.newEdge(nn, adj);
		} else {
			G.newEdge(nn, on);
		}
	}

	List<face> bigFaces; // not a triangle

	CombinatorialEmbedding E(G);
	bigFaces.pushBack(E.firstFace());
	for(i = m-n+1; i-->0;) {
		ListIterator<face> fi = bigFaces.chooseIterator();
		face f = *fi;
		bigFaces.del(fi);

		List<adjEntry> fnodes;
		adjEntry adj;
		forall_face_adj(adj, f) {
			fnodes.pushBack(adj);
		}
		fnodes.permute();
		adjEntry adj1,adj2;
		bool okay = false;
		do {
			adj1 = fnodes.popFrontRet();
			node n1 = adj1->theNode();
			forall_listiterators(adjEntry, it, fnodes) {
				adj2 = *it;
				node n2 = adj2->theNode();

				if(n1==n2 || adj1->faceCyclePred() == adj2 || adj2->faceCyclePred() == adj1) {
					continue;
				}
				edge e;
				okay = true;
				forall_adj_edges(e,n1) {
					if(e->opposite(n1) == n2) {
						okay = false;
						break;
					}
				}
				if(okay) break;
			}
		} while(!okay);

		edge ne = E.splitFace(adj1,adj2);

		face f1 = E.rightFace(ne->adjSource());
		face f2 = E.rightFace(ne->adjTarget());

		if (f1->size() > 3) bigFaces.pushBack(f1);
		if (f2->size() > 3) bigFaces.pushBack(f2);
	}
}


void planarBiconnectedGraph(Graph &G, int n, int m, bool multiEdges)
{
	if (n < 3) n = 3;
	if (m < n) m = n;
	if (m > 3*n-6) m = 3*n-6;

	int ke = n-3, kf = m-n;

	G.clear();

	Array<edge> edges(m);
	Array<face> bigFaces(m);
	//random_source S;

	// we start with a triangle
	node v1 = G.newNode(), v2 = G.newNode(), v3 = G.newNode();
	edges[0] = G.newEdge(v1,v2);
	edges[1] = G.newEdge(v2,v3);
	edges[2] = G.newEdge(v3,v1);

	CombinatorialEmbedding E(G);
	FaceArray<int> posBigFaces(E);
	int nBigFaces = 0, nEdges = 3;

	while(ke+kf > 0) {
		int p = randomNumber(1,ke+kf);

		if (nBigFaces == 0 || p <= ke) {
			edge e  = edges[randomNumber(0,nEdges-1)];
			face f  = E.rightFace(e->adjSource());
			face fr = E.rightFace(e->adjTarget());

			edges[nEdges++] = E.split(e);

			if (f->size() == 4) {
				posBigFaces[f] = nBigFaces;
				bigFaces[nBigFaces++] = f;
			}
			if (fr->size() == 4) {
				posBigFaces[fr] = nBigFaces;
				bigFaces[nBigFaces++] = fr;
			}

			ke--;

		} else {
			int pos = randomNumber(0,nBigFaces-1);
			face f = bigFaces[pos];
			int df = f->size();
			int i = randomNumber(0,df-1), j = randomNumber(2,df-2);

			adjEntry adj1;
			for (adj1 = f->firstAdj(); i > 0; adj1 = adj1->faceCycleSucc())
				i--;

			adjEntry adj2;
			for (adj2 = adj1; j > 0; adj2 = adj2->faceCycleSucc())
				j--;

			edge e = E.splitFace(adj1,adj2);
			edges[nEdges++] = e;

			face f1 = E.rightFace(e->adjSource());
			face f2 = E.rightFace(e->adjTarget());

			bigFaces[pos] = f1;
			posBigFaces[f1] = pos;
			if (f2->size() >= 4) {
				posBigFaces[f2] = nBigFaces;
				bigFaces[nBigFaces++] = f2;
			}
			if (f1->size() == 3) {
				bigFaces[pos] = bigFaces[--nBigFaces];
			}

			kf--;
		}
	}

	if (multiEdges == false) {
		SListPure<edge> allEdges;
		EdgeArray<int> minIndex(G), maxIndex(G);

		parallelFreeSortUndirected(G,allEdges,minIndex,maxIndex);

		SListConstIterator<edge> it = allEdges.begin();
		edge ePrev = *it, e;
		for(it = ++it; it.valid(); ++it, ePrev = e) {
			e = *it;
			if (minIndex[ePrev] == minIndex[e] &&
				maxIndex[ePrev] == maxIndex[e])
			{
				G.move(e,
					e->adjTarget()->faceCycleSucc()->twin(), ogdf::before,
					e->adjSource()->faceCycleSucc()->twin(), ogdf::before);
			}
		}

	}
}

void planarCNBGraph(Graph &G, int n, int m,	int b)
{
	G.clear();
	if (b <= 0) b = 1;
	if (n <= 0) n = 1;
	if ((m <= 0) || (m > 3*n-6)) m = 3*n-6;

	node cutv;
	G.newNode();

	for (int nB=1; nB<=b; nB++){
		cutv = G.chooseNode();
		// set number of nodes for the current created block
		int actN = randomNumber(1, n);

		node v1 = G.newNode();

		if (actN <= 1){
			G.newEdge(v1, cutv);
		}
		else
			if (actN == 2){
				node v2 = G.newNode();
				G.newEdge(v1, v2);

				int rnd = randomNumber(1, 2);
				edge newE;
				int rnd2 = randomNumber(1, 2);
				if (rnd == 1){
					newE = G.newEdge(v1, cutv);
				}
				else{
					newE = G.newEdge(v2, cutv);
				}
				if (rnd2 == 1){
					G.contract(newE);
				}
			}
			else{
				// set number of edges for the current created block
				int actM;
				if (m > 3*actN-6)
					actM = randomNumber(1, 3*actN-6);
				else
					actM = randomNumber(1, m);
				if (actM < actN)
					actM = actN;

				int ke = actN-3, kf = actM-actN;

				Array<node> nodes(actN);
				Array<edge> edges(actM);
				Array<face> bigFaces(actM);

				// we start with a triangle
				node v2 = G.newNode(), v3 = G.newNode();
				nodes[0] = v1;
				nodes[1] = v2;
				nodes[2] = v3;
				edges[0] = G.newEdge(v1,v2);
				edges[1] = G.newEdge(v2,v3);
				edges[2] = G.newEdge(v3,v1);

				int actInsertedNodes = 3;

				CombinatorialEmbedding E(G);
				FaceArray<int> posBigFaces(E);
				int nBigFaces = 0, nEdges = 3;

				while(ke+kf > 0) {
					int p = randomNumber(1,ke+kf);

					if (nBigFaces == 0 || p <= ke) {
						int eNr = randomNumber(0,nEdges-1);
						edge e  = edges[eNr];
						face f  = E.rightFace(e->adjSource());
						face fr = E.rightFace(e->adjTarget());

						node u = e->source();
						node v = e->target();

						edges[nEdges++] = E.split(e);

						if (e->source() != v && e->source() != u)
							nodes[actInsertedNodes++] = e->source();
						else
							nodes[actInsertedNodes++] = e->target();

						if (f->size() == 4) {
							posBigFaces[f] = nBigFaces;
							bigFaces[nBigFaces++] = f;
						}
						if (fr->size() == 4) {
							posBigFaces[fr] = nBigFaces;
							bigFaces[nBigFaces++] = fr;
						}

						ke--;
					}
					else {
						int pos = randomNumber(0,nBigFaces-1);
						face f = bigFaces[pos];
						int df = f->size();
						int i = randomNumber(0,df-1), j = randomNumber(2,df-2);

						adjEntry adj1;
						for (adj1 = f->firstAdj(); i > 0; adj1 = adj1->faceCycleSucc())
							i--;

						adjEntry adj2;
						for (adj2 = adj1; j > 0; adj2 = adj2->faceCycleSucc())
							j--;

						edge e = E.splitFace(adj1,adj2);
						edges[nEdges++] = e;

						face f1 = E.rightFace(e->adjSource());
						face f2 = E.rightFace(e->adjTarget());

						bigFaces[pos] = f1;
						posBigFaces[f1] = pos;
						if (f2->size() >= 4) {
							posBigFaces[f2] = nBigFaces;
							bigFaces[nBigFaces++] = f2;
						}
						if (f1->size() == 3) {
							bigFaces[pos] = bigFaces[--nBigFaces];
						}

						kf--;
					}
				}

				// delete multi edges
				SListPure<edge> allEdges;
				EdgeArray<int> minIndex(G), maxIndex(G);

				parallelFreeSortUndirected(G,allEdges,minIndex,maxIndex);

				SListConstIterator<edge> it = allEdges.begin();
				edge ePrev = *it, e;
				for(it = ++it; it.valid(); ++it, ePrev = e) {
					e = *it;
					if (minIndex[ePrev] == minIndex[e] &&
						maxIndex[ePrev] == maxIndex[e])
					{
						G.move(e,
							e->adjTarget()->faceCycleSucc()->twin(), ogdf::before,
							e->adjSource()->faceCycleSucc()->twin(), ogdf::before);
					}
				}

				node cutv2 = nodes[randomNumber(0,actN-1)];

				int rnd = randomNumber(1,2);
				edge newE = G.newEdge(cutv2, cutv);
				if (rnd == 1){
					G.contract(newE);
				}
			}
	}
}


void constructCConnectedCluster(node v,ClusterGraph &C);
void constructCluster(node v,ClusterGraph &C);
void bfs(node v,SList<node> &newCluster,NodeArray<bool> &visited,ClusterGraph &C);

void randomClusterGraph(ClusterGraph &C,Graph &G,int cNum)
{
	int n = G.numberOfNodes();
	//int m = G.numberOfEdges();

	node v;
	int count = 0;
	NodeArray<int> num(G);
	Array<node> numNode(0,n-1,0);
	forall_nodes(v,G)
	{
		num[v] =  count;
		numNode[count] = v;
		count++;
	}

	for (int i = 0; i < cNum; i++)
	{
		int rand = randomNumber(0,n-1);

		node start = numNode[rand];

		constructCluster(start,C);
	}

	OGDF_ASSERT(C.consistencyCheck());

}//randomClusterGraph

void randomClusterPlanarGraph(ClusterGraph &C,Graph &G,int cNum)
{
	int n = G.numberOfNodes();
	//int m = G.numberOfEdges();

	node v;
	int count = 0;
	NodeArray<int> num(G);
	Array<node> numNode(0,n-1,0);
	forall_nodes(v,G)
	{
		num[v] =  count;
		numNode[count] = v;
		count++;
	}

	for (int i = 0; i < cNum; i++)
	{
		int rand = randomNumber(0,n-1);

		node start = numNode[rand];

		constructCConnectedCluster(start,C);
	}

	// By construction, clusters might have just one child.
	// remove these clusters
	SListPure<cluster> store;
	cluster c;
	forall_clusters(c,C)
	{
		if ((c->cCount() + c->nCount()) == 1 )
			store.pushBack(c);
	}
	while (!store.empty())
	{
		c = store.popFrontRet();
		if (c != C.rootCluster())
			C.delCluster(c);
	}
	if ((C.rootCluster()->cCount() == 1) && (C.rootCluster()->nCount() == 0))
	{
		cluster cl = (*C.rootCluster()->cBegin());
		C.delCluster(cl);
	}


	OGDF_ASSERT(C.consistencyCheck());

}

void constructCConnectedCluster(node v,ClusterGraph &C)
{
	SList<node> newCluster;
	newCluster.pushBack(v);
	NodeArray<bool> visited(C.getGraph(),false);
	visited[v] = true;
	bfs(v,newCluster,visited,C);
	if (newCluster.size() > 1)
	{
		cluster cl = C.newCluster(C.clusterOf(v));
//		cout << " Cluster " << cl->index() << endl;
		while (!newCluster.empty())
		{
			node w = newCluster.popFrontRet();
//			cout << " Node in cluster " << w << endl;
			C.reassignNode(w,cl);
		}
	}
}

//construct new (child) cluster by randomly choosing nodes in v's cluster
void constructCluster(node v,ClusterGraph &C)
{
	if (C.clusterOf(v)->nCount() < 2) return;

	SList<node> newCluster;
	newCluster.pushBack(v);

	//store the cluster nodes for random selection
	//we  could just randomly select by running up the list
	//HashArray<int, node> clusterNodes;
	//int i = 0;
	ListConstIterator<node> it = C.clusterOf(v)->nBegin();
	while (it.valid())
	{
		if (!((*it) == v))
		{
			if (randomNumber(0,99) > 65)
				newCluster.pushBack((*it));
		}
		it++;
	}//while

	cluster cl = C.newCluster(C.clusterOf(v));
	while (!newCluster.empty())
	{
		node w = newCluster.popFrontRet();
		C.reassignNode(w,cl);
	}

}//constructcluster

//insert nodes in v's cluster to new cluster with a certain probability
void bfs(node v,SList<node> &newCluster,NodeArray<bool> &visited,ClusterGraph &C)
{

	edge e;
	SListPure<node> bfsL;
	forall_adj_edges (e,v)
	{
		node w = e->opposite(v);
		int probability = randomNumber(0,99);
		if (probability < 70 && !visited[w])
		{
			visited[w] = true;
			if (C.clusterOf(v) == C.clusterOf(w))
			{
				newCluster.pushBack(w);
				bfsL.pushBack(w);
			}
		}
		else
			visited[w] = true;
	}
	while(!bfsL.empty())
		bfs(bfsL.popFrontRet(),newCluster,visited,C);
}


void randomTree(Graph& G, int n) {
	G.clear();
	G.newNode();
	for(int i=1; i<n; i++) {
		node on = G.chooseNode();
		G.newEdge(on, G.newNode());
	}
}

void regularTree(Graph& G, int n, int children) {
	G.clear();
	node* id2node = new node[n];
	id2node[0] = G.newNode();
	for(int i=1; i<n; i++) {
		G.newEdge(id2node[(i-1)/children], id2node[i] = G.newNode());
	}
	delete[] id2node;
}

void createClustersHelper(ClusterGraph& C, const node curr, const node pred, const cluster predC, List<cluster>& internal, List<cluster>& leaves) {
	cluster currC = predC ? C.createEmptyCluster(predC) : C.rootCluster();
	if(curr->degree()==1 && pred!=0) {
		leaves.pushBack(currC);
	} else {
		edge e;
		forall_adj_edges(e,curr) {
			node next = e->opposite(curr);
			if(next == pred) continue;
			createClustersHelper(C,  next,curr,currC,  internal,leaves);
		}
		internal.pushBack(currC);
	}
}

void randomClusterGraph(ClusterGraph& C, const Graph& G, const node root, int moreInLeaves) {
	C.init(G);

	// Build cluster structure (and store which clusters are internal and which are leaves)
	List<cluster> internal;
	List<cluster> leaves;
	createClustersHelper(C,  root,0,0,  internal,leaves);

	// Assign nodes to clusters
	List<node> nodes;
	G.allNodes<List<node> >(nodes);

	// Step 1: Ensure two node per leaf-cluster
	nodes.permute();
	forall_listiterators(cluster, it, leaves) {
		C.reassignNode(nodes.popFrontRet(),*it);
		C.reassignNode(nodes.popFrontRet(),*it);
	}

	// Step 2: Distribute the other nodes
	int n = G.numberOfNodes();
	int numI = internal.size();
	int numL = leaves.size();
	double chanceForInternal = ( numI*n/double(numL*moreInLeaves+numI) ) / double(n-2*numL);
	// a leaf-cluster should have (on average) moreInLeaves-times as many vertices as in internal-cluster.
	// #verticesInInternalCluster = n / (numL*moreInLeaves + numI)
	// #nodesToDistribute = n - 2*numL
	// => chance that a node goes into an internal cluster = numI * #verticesInInternalCluster / (n-2*numL)
	while(!nodes.empty()) {
		cluster cl;
		if(randomDouble(0,1) < chanceForInternal) {
			cl = * internal.get(randomNumber(0,internal.size()-1));
		} else {
			cl = * leaves.get(randomNumber(0,leaves.size()-1));
		}
		C.reassignNode(nodes.popFrontRet(),cl);
	}
}

void completeGraph(Graph &G, int n)
{
	G.clear();

	Array<node> v(n);

	int i,j;
	for(i = n; i-->0;)
		v[i] = G.newNode();

	for(i = n; i-->0;)
		for(j = i; j-->0;)
			G.newEdge(v[i],v[j]);
}

void completeBipartiteGraph(Graph &G, int n, int m)
{
	G.clear();

	Array<node> a(n);
	Array<node> b(m);

	int i,j;
	for(i = n; i-->0;)
		a[i] = G.newNode();
	for(j = m; j-->0;)
		b[j] = G.newNode();

	for(i = n; i-->0;)
		for(j = m; j-->0;)
			G.newEdge(a[i],b[j]);
}

void wheelGraph(Graph &G, int n)
{
	G.clear();
	if(n <= 2) return;

	node center = G.newNode();

	node n0,n1=0,n2;
	for(; n-->0;) {
		G.newEdge(center, n2 = G.newNode());
		if(n1) G.newEdge(n1,n2);
		else n0 = n2;
		n1 = n2;
	}
	G.newEdge(n1,n0);
}

void suspension(Graph &G, int n)
{
	if(n == 0) return;
	OGDF_ASSERT( n>0 );

	List<node> nds;
	G.allNodes(nds);
	for(; n-->0;) {
		node n0 = G.newNode();
		forall_listiterators(node, it, nds)
			G.newEdge(n0,*it);
	}
}

void cubeGraph(Graph &G, int n)
{
	OGDF_ASSERT( n>=0 && n < 8*(int)sizeof(int)-1 ); // one sign bit, one less to be safe
	G.clear();

	int c = 1 << n;
	Array<node> lu(c);
	for(int i=0; i<c; ++i) {
		lu[i] = G.newNode();
		int q = 1;
		while( q <= i ) {
			if(q&i) G.newEdge(lu[i^q],lu[i]);
			q <<= 1;
		}
	}
}

void gridGraph(Graph &G, int n, int m, bool loopN, bool loopM) {
	G.clear();
	Array<node> front(0,n-1,0);
	Array<node> fringe(0,n-1,0);
	node first = 0;
	node last = 0;
	node cur;
	for(int j=m; j-->0;) {
		for(int i=n; i-->0;) {
			cur = G.newNode();
			if(!last) first=cur;
			else G.newEdge(last,cur);
			if(fringe[i]) G.newEdge(fringe[i],cur);
			else front[i] = cur;
			fringe[i] = cur;
			last = cur;
		}
		if(loopN)
			G.newEdge(last, first);
		last = 0;
	}
	if(loopM) {
		for(int i=n; i-->0;) {
			G.newEdge(fringe[i],front[i]);
		}
	}
}

void petersenGraph(Graph &G, int n, int m) {
	G.clear();
	Array<node> inner(0, n-1, 0);
	node first = 0;
	node last = 0;
	for(int i=n; i-->0;) {
		node outn = G.newNode();
		node inn = G.newNode();
		G.newEdge(outn,inn);
		inner[i]=inn;
		if(!last) first=outn;
		else G.newEdge(last,outn);
		last = outn;
	}
	G.newEdge(last, first);
	for(int i=n; i-->0;) {
		G.newEdge(inner[i],inner[(i+m)%n]);
	}
}

void randomDiGraph(Graph &G, int n, double p) {

	OGDF_ASSERT(n>=0 && p<=1 && p>=0);

	for(int i=0; i<n; i++)
		G.newNode();

	List<node> nodeList;
	G.allNodes(nodeList);
	nodeList.permute();
	forall_listiterators(node, it, nodeList) {
		node v = *it;
		node w;
		forall_nodes(w, G) {
			if (v==w)
				continue;
			if (randomDouble(0,1)<p)
				G.newEdge(v,w);
		}
	}
	//remove anti parallel edges
	makeSimple(G);
}

} // end namespace ogdf
