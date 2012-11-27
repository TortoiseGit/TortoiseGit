/*
 * $Revision: 2565 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 17:14:54 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implements class PlanarDrawLayout
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


#include <ogdf/planarlayout/PlanarDrawLayout.h>
#include <ogdf/basic/GraphCopy.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/augmentation/PlanarAugmentation.h>
#include <ogdf/augmentation/PlanarAugmentationFix.h>
#include <ogdf/module/ShellingOrderModule.h>
#include <ogdf/basic/BoundedStack.h>
#include <ogdf/planarlayout/BiconnectedShellingOrder.h>
#include <ogdf/planarity/SimpleEmbedder.h>

namespace ogdf {


PlanarDrawLayout::PlanarDrawLayout()
{
	m_sizeOptimization = true;
	m_sideOptimization = false;
	m_baseRatio        = 0.33;

	m_augmenter.set(new PlanarAugmentation);
	m_computeOrder.set(new BiconnectedShellingOrder);
	m_embedder.set(new SimpleEmbedder);
}


void PlanarDrawLayout::doCall(
	const Graph &G,
	adjEntry adjExternal,
	GridLayout &gridLayout,
	IPoint &boundingBox,
	bool fixEmbedding)
{
	// require to have a planar graph without multi-edges and self-loops;
	// planarity is checked below
	OGDF_ASSERT(isSimple(G) && isLoopFree(G));

	// handle special case of graphs with less than 3 nodes
	if(G.numberOfNodes() < 3)
	{
		node v1, v2;
		switch(G.numberOfNodes())
		{
		case 0:
			boundingBox = IPoint(0,0);
			return;

		case 1:
			v1 = G.firstNode();
			gridLayout.x(v1) = gridLayout.y(v1) = 0;
			boundingBox = IPoint(0,0);
			return;

		case 2:
			v1 = G.firstNode();
			v2 = G.lastNode ();
			gridLayout.x(v1) = gridLayout.y(v1) = gridLayout.y(v2) = 0;
			gridLayout.x(v2) = 1;
			boundingBox = IPoint(1,0);
			return;
		}
	}

	// we make a copy of G since we use planar biconnected augmentation
	GraphCopySimple GC(G);

	if(fixEmbedding) {
		PlanarAugmentationFix augmenter;
		augmenter.call(GC);

	} else {
		// augment graph planar biconnected
		m_augmenter.get().call(GC);

		// embed augmented graph
		m_embedder.get().call(GC,adjExternal);
	}

	// compute shelling order
	m_computeOrder.get().baseRatio(m_baseRatio);

	ShellingOrder order;
	m_computeOrder.get().call(GC,order,adjExternal);

	// compute grid coordinates for GC
	NodeArray<int> x(GC), y(GC);
	computeCoordinates(GC,order,x,y);

	boundingBox.m_x = x[order(1,order.len(1))];
	boundingBox.m_y = 0;
	node v;
	forall_nodes(v,GC)
		if(y[v] > boundingBox.m_y) boundingBox.m_y = y[v];

	// copy coordinates from GC to G
	forall_nodes(v,G) {
		node vCopy = GC.copy(v);
		gridLayout.x(v) = x[vCopy];
		gridLayout.y(v) = y[vCopy];
	}
}

void PlanarDrawLayout::computeCoordinates(const Graph &G,
	ShellingOrder &order,
	NodeArray<int> &x,
	NodeArray<int> &y)
{
	// let c_1,...,c_q be the the current contour, then
	// next[c_i] = c_i+1, prev[c_i] = c_i-1
	NodeArray<node>	next(G), prev(G);

	// upper[v] = w means x-coord. of v is relative to w
	// (abs. x-coord. of v = x[v] + abs. x-coord of w)
	NodeArray<node>	upper(G,0);

	// maximal rank of a neighbour
	NodeArray<int> maxNeighbour(G,0);
	// internal nodes (nodes not on contour)
	BoundedStack<node> internals(G.numberOfNodes());

	node v;
	forall_nodes(v,G)
	{
		adjEntry adj;
		forall_adj(adj,v) {
			int r = order.rank(adj->twinNode());
			if (r > maxNeighbour[v])
				maxNeighbour[v] = r;
		}
	}

	// initialize contour with base
	const ShellingOrderSet &V1 = order[1];
	node v1 = V1[1];
	node v2 = V1[V1.len()];
	node rightSide = v2;

	int i;
	for (i = 1; i <= V1.len(); ++i)
	{
		y[V1[i]] = 0;
		x[V1[i]] = (i == 1) ? 0 : 1;
		if (i < V1.len())
			next[V1[i]] = V1[i+1];
		if (i > 1)
			prev[V1[i]] = V1[i-1];
	}
	prev[v1] = next[v2] = 0;

	// process shelling order from bottom to top
	for (int k = 2; k <= order.length(); k++)
	{
		// Referenz auf aktuelle Menge Vk (als Abk?rzung)
		const ShellingOrderSet &Vk = order[k]; // Vk = { z_1,...,z_l }
		int l = Vk.len();

		node z1 = Vk[1];
		node cl = Vk.left();  // left vertex
		node cr = Vk.right(); // right vertex

		bool isOuter;
		if (m_sideOptimization && cr == rightSide && maxNeighbour[cr] <= k)
		{
			isOuter   = true;
			rightSide = Vk[l];
		} else
			isOuter = false;

		// compute relative x-distance from c_i to cl for i = l+1, ..., r
		int sum = 0;
		for (v = next[cl]; v != cr; v = next[v]) {
			sum += x[v];
			x[v] = sum;
		}
		x[cr] += sum;

		int eps = (maxNeighbour [cl] <= k && k > 2) ? 0 : 1;

		int x_cr, y_z;
		if (m_sizeOptimization)
		{
			int yMax;
			if (isOuter)
			{
				yMax = max(y[cl]+1-eps,
					y[cr] + ((x[cr] == 1 && eps == 1) ? 1 : 0));
				for (v = next[cl]; v != cr; v = next[v]) {
					if (x[v] < x[cr]) {
						int y1 = (y[cr]-y[v])*(eps-x[cr])/(x[cr]-x[v])+y[cr];
						if (y1 >= yMax)
							yMax = 1+y1;
					}
				}
				for (v = cr; v != cl; v = prev[v]) {
					if (y[prev[v]] > y[v] && maxNeighbour[v] >= k) {
						if (yMax <= y[v] + x[v] - eps) {
							eps  = 1;
							yMax = y[v] + x[v];
						}
						break;
					}
				}
				x_cr = max(x[cr]-eps-l+1, (y[cr] == yMax) ? 1 : 0);
				y_z  = yMax;

			} else {
				// yMax = max { y[c_i] | l <= i <= r }
				yMax = y[cl] - eps;
				for (v = cr; v != cl; v = prev[v]) {
					if (y[v] > yMax)
						yMax = y[v];
				}
				int offset = max (yMax-x[cr]+l+eps-y[cr],
					(y[prev[cr]] > y[cr]) ? 1 : 0);
				y_z  = y[cr] + x[cr] + offset - l + 1 - eps;
				x_cr = y_z - y[cr];
			}

		} else {
			y_z  = y[cr] + x[cr] + 1 - eps;
			x_cr = y_z - y[cr];
		}

		node alpha = cl;
		for (v = next[cl];
			maxNeighbour[v] <= k-1 && order.rank(v) <= order.rank(prev[v]);
			v = next[v])
		{
			if (order.rank (v) < order.rank (alpha))
				alpha = v;
			if (v == cr)
				break;
		}

		node beta = prev[cr];
		for (v = prev[cr];
			maxNeighbour[v] <= k-1 && order.rank(v) <= order.rank(next[v]);
			v = prev[v])
		{
			if (order.rank (v) <= order.rank (beta))
				beta = v;
			if (v == cl)
				break;
		}

		for (i = 1; i <= l; ++i) {
			x[Vk[i]] = 1;
			y[Vk[i]] = y_z;
		}
		x[z1] = eps;

		for (v = alpha; v != cl; v = prev [v]) {
			upper[v] = cl;
			internals.push (v);
		}
		for (v = next [beta]; v != cr; v = next [v]) {
			upper[v]  = cr;
			x [v]    -= x[cr];
			internals.push (v);
		}
		for (v = beta; v != alpha; v = prev[v]) {
			upper[v]  = z1;
			x [v]    -= x[z1];
			internals.push (v);
		}

		x[cr] = x_cr;

		// update contour after insertion of z_1,...,z_l
		for (i = 1; i <= l; i++) {
			if (i < l)
				next[Vk[i]] = Vk[i+1];
			if (i > 1)
				prev[Vk[i]] = Vk[i-1];
		}
		next [cl]    = z1;
		next [Vk[l]] = cr;
		prev [cr]    = Vk[l];
		prev [z1]    = cl;
	}

	// compute final x-coordinates for the nodes on the (final) contour
	int sum = 0;
	for (v = v1; v != 0; v = next[v])
		x [v] = (sum += x[v]);

	// compute final x-coordinates for the internal nodes
	while (!internals.empty()) {
		v     = internals.pop();
		x[v] += x[upper[v]];
	}
}


} // end namespace ogdf
