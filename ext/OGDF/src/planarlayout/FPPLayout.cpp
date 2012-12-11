/*
 * $Revision: 2599 $
 *
 * last checkin:
 *   $Author: chimani $
 *   $Date: 2012-07-15 22:39:24 +0200 (So, 15. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Definition of the Fraysseix, Pach, Pollack Algorithm (FPPLayout)
 *
 * \author Till Sch&auml;fer
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

#include <ogdf/planarlayout/FPPLayout.h>
#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/basic/GraphCopy.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/exceptions.h>
#include <ogdf/basic/List.h>

namespace ogdf {

FPPLayout::FPPLayout() : PlanarGridLayoutModule() {
}

void FPPLayout::doCall(
	const Graph &G,
	adjEntry adjExternal,
	GridLayout &gridLayout,
	IPoint &boundingBox,
	bool fixEmbedding)
{
	// check for double edges & self loops
	OGDF_ASSERT(isSimple(G));

	// handle special case of graphs with less than 3 nodes
	if (G.numberOfNodes() < 3) {
		node v1, v2;
		switch (G.numberOfNodes()) {
		case 0:
			boundingBox = IPoint(0, 0);
			return;

		case 1:
			v1 = G.firstNode();
			gridLayout.x(v1) = gridLayout.y(v1) = 0;
			boundingBox = IPoint(0, 0);
			return;

		case 2:
			v1 = G.firstNode();
			v2 = G.lastNode();
			gridLayout.x(v1) = gridLayout.y(v1) = gridLayout.y(v2) = 0;
			gridLayout.x(v2) = 1;
			boundingBox = IPoint(1, 0);
			return;
		}
	}

	// make a copy for triangulation
	GraphCopy GC(G);

	// embed
	if (!fixEmbedding) {
		if (planarEmbed(GC) == false) {
			OGDF_THROW_PARAM(PreconditionViolatedException, pvcPlanar);
		}
	}

	triangulate(GC);

	// get edges for outer face (triangle)
	adjEntry e_12;
	if (adjExternal != 0) {
		edge eG  = adjExternal->theEdge();
		edge eGC = GC.copy(eG);
		e_12 = (adjExternal == eG->adjSource()) ? eGC->adjSource() : eGC->adjTarget();
	}
	else {
		e_12 = GC.firstEdge()->adjSource();
	}
	adjEntry e_2n = e_12->faceCycleSucc();

	NodeArray<int>  num(GC);

	NodeArray<adjEntry> e_wp(GC);					// List of predecessors on circle C_k
	NodeArray<adjEntry> e_wq(GC);					// List of successors on circle  C_k

	computeOrder(GC, num , e_wp, e_wq, e_12, e_2n, e_2n->faceCycleSucc());
	computeCoordinates(GC, boundingBox, gridLayout, num, e_wp, e_wq);
}


void FPPLayout::computeOrder(
	const GraphCopy &G,
	NodeArray<int> &num,
	NodeArray<adjEntry> &e_wp,
	NodeArray<adjEntry> &e_wq,
	adjEntry e_12,
	adjEntry e_2n,
	adjEntry e_n1)
{
	NodeArray<int> num_diag(G, 0);							// number of chords
	// link[v] = Iterator in possible, that points to v (if diag[v] = 0 and outer[v] = TRUE)
	NodeArray<ListIterator<node> > link(G, 0);
	// outer[v] = TRUE <=> v is a node of the actual outer face
	NodeArray<bool> outer(G, false);
	// List of all nodes v with outer[v] = TRUE and diag[v] = 0
	List<node> possible;

	// nodes of the outer triangle (v_1,v_2,v_n)
	node v_1 = e_12->theNode();
	node v_2 = e_2n->theNode();
	node v_n = e_n1->theNode();
	node v_k, wp, wq, u;
	adjEntry e, e2;
	int k;

	// initialization: beginn with outer face (v_1,v_2,v_n)
	// v_n is the only possible node
	num[v_1] = 1;
	num[v_2] = 2;

	outer[v_1] = true;
	outer[v_2] = true;
	outer[v_n] = true;

	link[v_n] = possible.pushBack(v_n);

	e_wq[v_1] = e_n1->twin();
	e_wp[v_2] = e_2n;

	e_wq[v_n] = e_2n->twin();
	e_wp[v_n] = e_n1;

	// select next v_k and delete it
	for (k = G.numberOfNodes(); k >= 3; k--) {
		v_k = possible.popFrontRet();	// select arbitrary node from possible as v_k

		num[v_k] = k;

		// predecessor wp and successor wq from vk in C_k (actual outer face)
		wq = (e_wq [v_k])->twinNode();
		wp = (e_wp [v_k])->twinNode();

		// v_k not in C_k-1 anymore
		outer[v_k] = false;

		// shortfall of a chord?
		if (e_wq[wp]->cyclicSucc()->twinNode() == wq) {   // wp, wq is the only successor of vk in G_k
			// wp, wq loose a chord
			if (--num_diag[wp] == 0) {
				link[wp] = possible.pushBack(wp);
			}
			if (--num_diag[wq] == 0) {
				link[wq] = possible.pushBack(wq);
			}
		}

		// update or initialize e_wq, e_wp
		e_wq[wp] = e_wq[wp]->cyclicSucc();

		e_wp[wq] = e_wp[wq]->cyclicPred();
		e = e_wq[wp];
		for (u = e->twinNode(); u != wq; u = e->twinNode()) {
			outer[u] = true;
			e_wp[u] = e->twin();
			e = e_wq[u] = e_wp[u]->cyclicSucc()->cyclicSucc();

			// search for new chords
			for (e2 = e_wp[u]->cyclicPred(); e2 != e_wq[u]; e2 = e2->cyclicPred()) {
				node w = e2->twinNode();
				if (outer[w] == true) {
					++num_diag[u];
					if (w != v_1 && w != v_2)
						if (++num_diag[w] == 1) possible.del(link[w]);
				}
			}

			if (num_diag[u] == 0) {
				link[u] = possible.pushBack(u);
			}
		}
	}
}


void FPPLayout::computeCoordinates(const GraphCopy &G, IPoint &boundingBox, GridLayout &gridLayout, NodeArray<int> &num,
									NodeArray<adjEntry> &e_wp, NodeArray<adjEntry> &e_wq) {
	NodeArray<int> &x = gridLayout.x();
	NodeArray<int> &y = gridLayout.y();

	const int n = G.numberOfNodes();
	NodeArray<int>  x_rel(G);
	NodeArray<node> upper(G);
	NodeArray<node> next(G);
	Array<node, int> v(1, n);
	node w, vk, wp, wq;
	int k, xq, dx;

	forall_nodes(w, G) {
		v[num[w]] = (node) w;
	}

	x_rel[v[1]] = 0;
	x_rel[v[2]] = 0;
	y[G.original(v[1])] = 0;
	y[G.original(v[2])] = 0;

	next[v[1]] = v[2];
	next[v[2]] = 0;

	for (k = 3; k <= n; k++) {
		vk = v[k];
		wp = e_wp [vk]->twinNode();
		wq = e_wq [vk]->twinNode();

		xq = 2;
		w = wp;
		do {
			w = next [w];
			xq += x_rel[w];
		}
		while (w != wq);

		x_rel[vk] = (xq + y[G.original(wq)] - y[G.original(wp)]) / 2;
		y[G.original(vk)] = (xq + y[G.original(wq)] + y[G.original(wp)]) / 2;
		x_rel[wq] = xq - x_rel[vk];

		dx = 1;
		for (w = next[wp]; w != wq; w = next[w]) {
			dx += x_rel[w];
			x[G.original(w)] = dx - x_rel[vk];
			upper[w] = vk;
		}

		next[wp] = vk;
		next[vk] = wq;
	}

	// calculate absolute x-coordinate off the outer face
	x[G.original(v[n])] = x_rel[v[n]];
	x[G.original(v[2])] = x[G.original(v[n])] + x_rel[v[2]];
	x[G.original(v[1])] = 0;

	// calculate absolute x-coordinate for all inner nodes
	for (k = n - 1; k >= 3; k--) {
		x[G.original(v[k])] += x[G.original(upper[v[k]])];
	}

	// compute grid bounding box
	int xr = 0, yt = 0;
	switch (n) {
	case 0:
	case 1:
		break;
	case 2:
		xr = 1;
		break;
	default:
		yt = n - 2;
		xr = 2 * (n - 2);
	}

	boundingBox = IPoint(xr, yt);
}


} //namespace ogdf
