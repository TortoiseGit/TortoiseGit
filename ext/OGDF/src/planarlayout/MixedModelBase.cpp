/*
 * $Revision: 2599 $
 *
 * last checkin:
 *   $Author: chimani $
 *   $Date: 2012-07-15 22:39:24 +0200 (So, 15. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of Mixed-Model basic functionality.
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


#include "MixedModelBase.h"
#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/basic/simple_graph_alg.h>


namespace ogdf {


/********************************************************************
						hasLeft, hasRight

	Determine if the kth set in the ordered partition has a "real"
	left or right vertex, respectively.
********************************************************************/

bool MixedModelBase::hasLeft (int k) const
{
	const ShellingOrderSet &V = m_mmo[k];
	const List<InOutPoint> &L = m_iops.inpoints(V[1]);

	ListConstIterator<InOutPoint> it = L.begin();
	return (it.valid() && (*it).m_adj->twinNode() == m_mmo.m_left[k]);
}

bool MixedModelBase::hasRight(int k) const
{
	const ShellingOrderSet &V = m_mmo[k];
	const List<InOutPoint> &L = m_iops.inpoints(V[V.len()]);

	ListConstIterator<InOutPoint> it = L.rbegin();
	return (it.valid() && (*it).m_adj->twinNode() == m_mmo.m_right[k]);
}


/********************************************************************
							computeOrder

	Computes the ordered partition (incl. m_left[k], m_right[k]) and
	constructs the in- and outpoint lists.
********************************************************************/


void MixedModelBase::computeOrder(
	AugmentationModule &augmenter,
	EmbedderModule *pEmbedder,
	adjEntry adjExternal,
	ShellingOrderModule &compOrder)
{
	// remove (temporary) deg-1-nodes;
	removeDeg1Nodes();

	// augment PG (temporary) to achieve required connectivity
	List<edge> augmentedEdges;
	augmenter.call(m_PG,augmentedEdges);

	// embed augmented graph (if required)
	if(pEmbedder)
		pEmbedder->call(m_PG,adjExternal);

	// compute ordering of biconnected plane graph
	m_mmo.init(m_PG, compOrder, adjExternal);

	// restore deg1-nodes and mark incident edges
	m_iops.restoreDeg1Nodes(m_PG,m_deg1RestoreStack);

	// compute in- and outpoint lists
	for (int k = 1; k <= m_mmo.length(); ++k)
	{
		const ShellingOrderSet &V = m_mmo[k];
		for (int i = 1; i <= V.len(); ++i)
		{
			node v = V[i];
			node cl = (i == 1)       ? V.left () : V[i-1];
			node cr = (i == V.len()) ? V.right() : V[i+1];

			//edge e, er = 0, el = 0;
			adjEntry adj, adjR = 0, adjL = 0;
			forall_adj(adj,v)
			{
				node t = adj->twinNode();
				if (t == cr) adjR = adj;
				if (t == cl) adjL = adj;
			}
			// one of adjL and adjR is not 0 by definition of the ordering
			if (adjR == 0) adjR = adjL;
			if (adjL == 0) adjL = adjR;

			adj = adjR;
			do {
				if (exists(adj))
					m_iops.pushInpoint(adj);
				adj = adj->cyclicSucc();
			} while (m_iops.marked(adj) || (m_mmo.rank(adj->twinNode()) <= k && adj != adjR));

			for ( ; m_iops.marked(adj) || m_mmo.rank(adj->twinNode()) > k; adj = adj->cyclicSucc())
				if (exists(adj))
					m_iops.appendOutpoint(adj);

			// move In-/Outpoints to deg1-nodes to appropriate places
			adjL = m_iops.switchBeginIn(v);
			adjR = m_iops.switchEndIn  (v);

			// has a left- or right edge ?
			bool has_el = (adjL != 0);
			bool has_er = (adjR != 0);
			if (adjL && adjL == adjR) {
				has_el = (adjL->twinNode() != cr);
				has_er = !has_el;
			}

			// determine left(k) and right(k)
			if (i == 1)       m_mmo.m_left [k] = (has_el) ? adjL->twinNode() : cl;
			if (i == V.len()) m_mmo.m_right[k] = (has_er) ? adjR->twinNode() : cr;

			int xl, xr;
			m_iops.numDeg1(v,xl,xr,has_el || has_er);

			int x = 0;
			if (!has_el) x += xl;
			if (!has_er) x += xr;

			int x_alpha = max(0,min(x, (m_iops.in(v)-m_iops.out(v)+1+2*x-2)/2));
			int x_beta = x - x_alpha;

			if (!has_el)
				for( ; x_beta > 0 && xl > 0; --x_beta, --xl)
					m_iops.switchBeginOut(v);
			if (!has_er)
				for( ; x_beta > 0 && xr > 0; --x_beta, --xr)
					m_iops.switchEndOut(v);
		}
	}

	// remove augmented edges
	ListConstIterator<edge> itE;
	for(itE = augmentedEdges.begin(); itE.valid(); ++itE)
		m_PG.delEdge(*itE);
}


/********************************************************************
						removeDeg1Nodes

	Removes deg-1-nodes and store informations for restoring them.
********************************************************************/

void MixedModelBase::removeDeg1Nodes()
{
	NodeArray<bool> mark(m_PG,false);
	node v;

	// mark all deg-1-nodes we want to remove
	int n = m_PG.numberOfNodes();
	forall_nodes(v,m_PG) {
		if (n <= 3) break;
		if ((mark[v] = (v->degree() == 1)) == true) {
			node w = v->firstAdj()->twinNode();
			if (mark[w]) mark[w] = false; else --n;
		}
	}

	m_PG.removeDeg1Nodes(m_deg1RestoreStack,mark);
}


/********************************************************************
						assignIopCoords

	Computes the relative coordinates of the in- and outpoints,
	incl. height(v), depth(v).
********************************************************************/

void MixedModelBase::assignIopCoords()
{
	for (int k = 1; k <= m_mmo.length(); ++k)
	{
		const ShellingOrderSet &V = m_mmo[k];

		for (int i = 1; i <= V.len(); ++i)
		{
			node v = V[i];

			bool onlyLeft = (m_iops.in(v) == 2 &&
				i >= 2 && m_iops.inpoints(v).front().m_adj->twinNode() == V[i-1] &&
					m_iops.marked(m_iops.inpoints(v).back().m_adj));
			bool onlyRight = (m_iops.in(v) == 2 &&
				i < V.len() && m_iops.inpoints(v).back().m_adj->twinNode() == V[i+1] &&
					m_iops.marked(m_iops.inpoints(v).front().m_adj));

			if (m_iops.out(v) >= 1)
			{
				int outL = 0, outR = 0;

				int outPlus  = m_iops.out(v)/2;
				int outMinus = m_iops.out(v) - 1 - outPlus;
				int deltaL = 0, deltaR = 0;
				outL = outMinus;

				if (m_iops.in(v) == 2) {
					deltaL = (onlyRight) ? 0 : 1;
					deltaR = (onlyLeft)  ? 0 : 1;

				} else if (m_iops.in(v) >= 3) {
					deltaL = deltaR = 1;

				} else if (m_iops.in(v) == 1) {
					node vl = (i == 1) ? m_mmo.m_left[k] : V[i-1];
					if (m_iops.inpoints(v).front().m_adj->twinNode() != vl) {
						outL = outPlus;
						deltaR = 1;

					} else {
						deltaL = 1;
					}
				}

				outR = m_iops.out(v) - 1 - outL;

				List<InOutPoint> &opl = m_iops.outpoints(v);
				int j;
				ListIterator<InOutPoint> it = opl.begin();
				for (j = 0; j < outL; j++, ++it)
					m_iops.setOutCoord(it,-outL+j,deltaL+j);
				m_iops.m_height[v] = max(outL+deltaL,outR+deltaR)-1;
				if (m_iops.m_height[v] == 0 && m_iops.marked((*it).m_adj))
					m_iops.m_height[v] = 1;
				m_iops.setOutCoord(it,0,m_iops.m_height[v]);
				for (j = 1, ++it; j <= outR; j++, ++it)
					m_iops.setOutCoord(it,j,outR+deltaR-j);
			}

			if (m_iops.in(v) <= 3)
			{
				List<InOutPoint> &ipl = m_iops.inpoints(v);
				int in_v = m_iops.in(v);

				if (in_v == 3 || (in_v == 2 && !onlyRight)) {
					if (m_iops.marked(ipl.front().m_adj))
						m_iops.setInCoord(ipl.begin(),-1,0);
				}
				if (in_v == 3 || (in_v == 2 && !onlyLeft)) {
					if (m_iops.marked(ipl.back().m_adj))
						m_iops.setInCoord(ipl.rbegin(),  1,0);
				}
				if (in_v != 0 && (in_v != 2 || onlyLeft || onlyRight)) {
					ListIterator<InOutPoint> it = ipl.begin();
					if (in_v == 3 || (in_v == 2 && onlyLeft))
						++it;
					if (m_iops.marked((*it).m_adj)) {
						m_iops.setInCoord(it,0,-1);
						m_iops.m_depth[v] = 1;
					}
				}

			} else {
				int in_l = (m_iops.in(v)-3) / 2;
				int in_r = m_iops.in(v)-3 - in_l;

				int j;
				List<InOutPoint> &ipl = m_iops.inpoints(v);
				ListIterator<InOutPoint> it = ipl.begin();
				m_iops.setInCoord(it,(in_l == 0 && m_iops.marked((*it).m_adj)) ? -1 : -in_l,0);
				for (j = 1, ++it; j <= in_l; ++j, ++it)
					m_iops.setInCoord(it,j-in_l-1,-j);
				m_iops.setInCoord(it,0,-in_r);
				m_iops.m_depth[v] = in_r;  // inpoint with smallest y-coordinate
				for (j = 1, ++it; j <= in_r; ++j, ++it)
					m_iops.setInCoord(it,j,j-in_r-1);
				m_iops.setInCoord(it,in_r,0);
			}
		}
	}
}


/********************************************************************
							placeNodes

	Implements the placement step. Computes x[v] and y[v].
********************************************************************/

void MixedModelBase::placeNodes()
{
	m_dyl.init(2,m_mmo.length());
	m_dyr.init(2,m_mmo.length());

	m_leftOp .init(2,m_mmo.length());
	m_rightOp.init(2,m_mmo.length());

	m_nextLeft .init(m_PG);
	m_nextRight.init(m_PG);
	m_dxla.init(m_PG,0);
	m_dxra.init(m_PG,0);

	computeXCoords();
	computeYCoords();
}


/********************************************************************
							computeXCoords

	Computes the absolute x-coordinates x[v] of all nodes in the
	ordering, furthermore dyla[k] and dyra[k] (used by compute_y_coordinates)
********************************************************************/

void MixedModelBase::computeXCoords()
{
	NodeArray<int> &x = m_gridLayout.x();

	int  k, i;
	node v;

	// representation of the contour
	NodeArray<node> prev(m_PG), next(m_PG);
	NodeArray<node> father(m_PG, 0);

	// maintaining of free space for shifting
	Array<int> shiftSpace(1,m_mmo.length(), 0);
	NodeArray<int> comp(m_PG,0);

	forall_nodes(v,m_PG) {
		m_nextLeft [v] = m_iops.firstRealOut(v);
		m_nextRight[v] = m_iops.lastRealOut (v);
	}

	// last_right[v] = last vertex of highest set with right vertex v
	NodeArray<node> lastRight(m_PG,0);
	for(k = 2; k <= m_mmo.length(); ++k) {
		const ShellingOrderSet &V = m_mmo[k];
		lastRight[m_mmo.m_right[k]] = V[V.len()];
	}

	NodeArray<int> high(m_PG,0);
	forall_nodes(v,m_PG) {
		InOutPoint op;
		ListConstIterator<InOutPoint> it;
		for(it = m_iops.outpoints(v).begin(); it.valid(); ++it) {
			if (!m_iops.marked((*it).m_adj))
				high[v] = max(m_mmo.rank((*it).m_adj->twinNode()), high[v]);
		}
	}

	// initialization
	const ShellingOrderSet &V1 = m_mmo[1];
	int p = V1.len();

	x[V1[1]] = m_iops.outLeft(V1[1]);
	for (i = 2; i <= p; i++) {
		x[V1[i]] = m_iops.maxRight(V1[i-1]) + m_iops.maxLeft(V1[i]) + 1;
	}

	for (i = 1; i <= p; i++) {
		if (i < p) next[V1[i]] = V1[i+1];
		if (i > 1) prev[V1[i]] = V1[i-1];
	}
	prev [V1[1]] = next [V1[p]] = 0;

	// main loop
	for(k = 2; k <= m_mmo.length(); ++k)
	{
		// consider set Vk
		const ShellingOrderSet &Vk = m_mmo[k];
		p = Vk.len();
		node z1 = Vk[1];
		node cl = m_mmo.m_left[k];
		node cr = m_mmo.m_right[k];

		if (!hasLeft(k)) {
			while(lastRight[cl] && high[cl] < k && !hasRight(m_mmo.rank(lastRight[cl])))
				cl = m_mmo.m_left[k] = lastRight[cl];
		}

		// determine temporarily the x-coordinates of c_l+1,...,c_r relative to cl
		int sum = 0;
		for (v = next[cl]; v != cr; v = next[v]) {
			sum += x[v]; x[v] = sum;
		}
		x[cr] += sum;

		m_leftOp [k] = m_nextRight[cl];
		m_rightOp[k] = m_nextLeft [cr];

		// compute dxl, dxr, dyl, dyr
		int dxl, dxr;
		m_dyl[k] = m_dyr[k] = 0;

		ListConstIterator<InOutPoint> it;
		if ((it = m_nextRight[cl]).valid())  {
			dxl = (*it).m_dx;
			if ((*it).m_adj->twinNode() != z1)
				dxl++;
			else
				m_nextRight[cl] = m_iops.prevRealOut(it);

			if (dxl < 0)
				m_dyl[k] = m_iops.m_height[cl];
			else if ((++it).valid())
				m_dyl[k] = (*it).m_dy;
		} else {
			dxl = (m_iops.out(cl) == 0) ? 0 : -m_iops.outLeft(cl);
		}
		if ((it = m_nextLeft[cr]).valid()) {
			dxr = (*it).m_dx;
			if ((*it).m_adj->twinNode() != Vk[p])
				dxr--;
			else
				m_nextLeft[cr] = m_iops.nextRealOut(it);

			if (dxr > 0)
				m_dyr[k] = m_iops.m_height[cr];
			else if ((it = it.pred()).valid())
				m_dyr[k] = (*it).m_dy;
		} else {
			dxr = (m_iops.out(cr) == 0) ? 0 : m_iops.outRight(cr);
		}

		m_dxla[Vk[1]] = dxl; m_dxra[Vk[p]] = dxr;

		int old_x_cr;

		// vertex case
		if (!m_iops.isChain(z1))
		{
			InOutPoint ip_ct = m_iops.middleNeighbor(z1);
			InOutPoint op_ct = *m_iops.pointOf(ip_ct.m_adj->twin());
			node ct = ip_ct.m_adj->twinNode();

			int delta = dxl + m_iops.maxPlusLeft(z1) + ip_ct.m_dx - (x[ct] + op_ct.m_dx);
			if (delta < 0) delta = 0;
			x[ct] += delta;

			int x_0 = x[ct] + op_ct.m_dx - ip_ct.m_dx;

			int sum = 0;
			for (v = prev[ct]; v != cl; v = prev[v]) {
				x[v] -= sum;
				if (m_nextRight[v].valid() && (*m_nextRight[v]).m_adj->twinNode() == z1) {
					InOutPoint op_v = *m_nextRight[v];
					InOutPoint ip_v = *m_iops.pointOf(op_v.m_adj->twin());
					int diff = x[v] + op_v.m_dx - x_0 - ip_v.m_dx;
					if (diff > 0) {
						sum += diff;
						x[v] -= diff;
					}
				}
			}

			for (v = next[cl]; v != next[ct]; v = next[v])
				x[v] += sum;
			x_0 += sum;

			sum += delta;

			for (v = next[ct]; v != next[cr]; v = next[v]) {
				x[v] += sum;
				if (m_nextRight[v].valid() && (*m_nextRight[v]).m_adj->twinNode() == z1) {
					InOutPoint op_v = *m_nextRight[v];
					InOutPoint ip_v = *m_iops.pointOf(op_v.m_adj->twin());
					int diff = x[v] + op_v.m_dx - x_0 - ip_v.m_dx;
					if (diff < 0) {
						sum -= diff;
						x[v] -= diff;
					}
				}
			}

			x[z1] = x_0;

			old_x_cr = x[cr] - x[z1];
			x[cr] = max(old_x_cr, m_iops.maxPlusRight(z1) - dxr);

			for (v = next[cl]; v != cr; v = next[v]) {
				x[v]      = x[v] - x[z1];
				father[v] = z1;
			}

		// chain case
		} else {
			int sum = x[z1] = m_iops.maxPlusLeft(z1) + dxl;
			for (i = 2; i <= p; i++)
				sum += (x[Vk[i]] = m_iops.maxRight(Vk[i-1]) + m_iops.maxLeft(Vk[i]) + 1);

			old_x_cr = x[cr] - sum;
			int new_x_cr = m_iops.maxPlusRight(Vk[p]) - dxr;
			x[cr] = max(old_x_cr, new_x_cr);
			shiftSpace[k] = max(0, old_x_cr - new_x_cr);

			for (v = next[cl]; v != cr; v = next[v]) {
				x[v]      = x[v] - x[z1];
				father[v] = z1;
			}
		}

		int need = x[cr] - old_x_cr;
		int k_cr = m_mmo.rank(cr);
		if (shiftSpace[k_cr] > 0) {
			int use = min(shiftSpace[k_cr], need);
			shiftSpace[k_cr] -= use;
			comp[cr] += use;
			x[m_mmo.m_right[k_cr]] -= use;
		}

		// update contour after insertion of z1,...,zp
		for (i = 1; i <= p; i++) {
			if (i < p) next[Vk[i]] = Vk[i+1];
			if (i > 1) prev[Vk[i]] = Vk[i-1];
		}

		next [prev [z1]    = cl] = z1;
		prev [next [Vk[p]] = cr] = Vk[p];

	}

	// compute final x-coordinates for nodes on final contour
	int sum = 0;
	for (v = V1[1]; v != 0; v = next[v])
		x [v] = (sum += x[v]);

	// compute final x-coordinates for inner nodes
	for (k = m_mmo.length(); k >= 1; k--) {
		for (i = 1; i <= m_mmo.len(k); i++) {
			v = m_mmo(k,i);
			if (father[v] != 0) {
				x[v] = x[v] + x[father[v]] - comp[father[v]];
			}
		}
	}
}


/********************************************************************
							computeYCoords

	Computes the absolute y-coordinates y[v] of all nodes in the
	ordering.
********************************************************************/

class SetYCoords
{
public:
	SetYCoords(const Graph &G, const IOPoints &iops, const MMOrder &mmo,
		const NodeArray<int> &x, const NodeArray<int> &y) :
		m_G(G), m_iops(iops), m_mmo(mmo), m_x(x), m_y(y) {
	}

	void init(int k);

	void checkYCoord (int xleft, int xright, int ys, bool nodeSep);
	void checkYCoord (int xs, int ys, bool nodeSep ) {
		checkYCoord (xs, xs, ys, nodeSep);
	}

	int getYmax() const {
		return m_ymax;
	}

	// avoid automatic creation of assignment operator
	SetYCoords &operator=(const SetYCoords &);

private:
	void getNextRegion();
	node z(int j) const {
		return (*m_V)[j];
	}

	bool marked(adjEntry adj) const {
		return m_iops.marked(adj);
	}

	const InOutPoint &outpoint(const InOutPoint &ip) {
		return *m_iops.pointOf(ip.m_adj->twin());
	}

	void searchNextInpoint();

	const Graph            &m_G;
	const IOPoints         &m_iops;
	const MMOrder          &m_mmo;
	const NodeArray<int>   &m_x;
	const NodeArray<int>   &m_y;
	const ShellingOrderSet *m_V;

	int m_k;
	node m_cl, m_cr;

	int m_ymax, m_xNext, m_lookAheadX, m_lookAheadNextX;
	int m_i, m_iNext, m_deltaY, m_infinity;
	ListConstIterator<InOutPoint> m_itIp, m_itIpNext, m_itLookAhead;
	bool m_onBase;
};

void SetYCoords::init(int k)
{
	m_k = k; m_V = &m_mmo[k];
	m_ymax = 0;
	m_lookAheadX = 0;

	m_i = 0;
	m_cl = m_mmo.m_left[k]; m_cr = m_mmo.m_right[k];

	m_onBase = true; m_xNext = -1;
	m_infinity = m_x[m_cr] + m_iops.outRight(m_cr) + 1;

	searchNextInpoint();
	m_itIp = m_itIpNext; m_i = m_iNext;

	getNextRegion();
}

void SetYCoords::searchNextInpoint()
{
	m_iNext = m_i; m_itIpNext = m_itIp;

	do {
		if (!m_itIpNext.valid()) {
			if (++m_iNext > m_V->len()) {
				m_itIpNext = ListConstIterator<InOutPoint>();
				return;
			}
			m_itIpNext = m_iops.inpoints(z(m_iNext)).begin();
		} else {
			++m_itIpNext;
		}
	} while (!m_itIpNext.valid() || (*m_itIpNext).m_dy == 0);

	if (m_itIpNext.valid() && m_iops.marked((*m_itIpNext).m_adj)) {
		int ipX = m_x[z(m_iNext)] + (*m_itIpNext).m_dx;

		if (m_lookAheadX <= ipX) {
			for (m_itLookAhead = m_itIpNext;
				(*m_itLookAhead).m_dx < 0 && m_iops.marked((*m_itLookAhead).m_adj);
				++m_itLookAhead) ;

			const InOutPoint &ipLookAhead = *m_itLookAhead;
			m_lookAheadX = m_x[z(m_iNext)] + ipLookAhead.m_dx;
			if(ipLookAhead.m_dx < 0) {
				m_lookAheadNextX = m_x[ipLookAhead.m_adj->twinNode()] + outpoint(ipLookAhead).m_dx;
			} else {
				m_lookAheadNextX = m_lookAheadX;
			}
		}

		if (m_lookAheadNextX <= ipX) {
			m_itIpNext = m_itLookAhead;
		}
	}
}


void SetYCoords::getNextRegion()
{
	int xOld = m_xNext;

	do {
		if (m_onBase) {
			m_deltaY = 0;
			if (!m_itIp.valid()) {
				m_xNext = m_infinity;
			} else {
				const InOutPoint &ip = *m_itIp;
				m_xNext = (marked(ip.m_adj)) ? (m_x[z(m_i)] + ip.m_dx) :
					(m_x[ip.m_adj->twinNode()] + outpoint(ip).m_dx);
			}
			m_onBase = (m_iNext != m_i);

		} else {
			const InOutPoint &ip = *m_itIp;
			m_deltaY = -ip.m_dy;
			searchNextInpoint();
			if (m_itIpNext.valid() && ip.m_dx < 0) {
				const InOutPoint &m_ipNext = *m_itIpNext;
				m_xNext = (marked(m_ipNext.m_adj)) ? (m_x[z(m_i)] + m_ipNext.m_dx) :
					(m_x[m_ipNext.m_adj->twinNode()] + outpoint(m_ipNext).m_dx);
			} else {
				m_xNext = (marked(ip.m_adj)) ? (m_x[z(m_i)] + ip.m_dx + 1) :
					(m_x[ip.m_adj->twinNode()] + outpoint(ip).m_dx + 1);
			}

			m_onBase = (m_iNext != m_i);
			m_i = m_iNext; m_itIp = m_itIpNext;
		}
	} while (m_xNext <= xOld);
}

void SetYCoords::checkYCoord(int xleft, int xright, int ys, bool nodeSep)
{
	while (m_xNext <= xleft)
		getNextRegion();

	int maxDy = m_deltaY;

	while (m_xNext <= xright) {
		getNextRegion();
		if (m_deltaY > maxDy) maxDy = m_deltaY;
	}

	if (nodeSep && maxDy == 0)
		maxDy = 1;

	if (ys + maxDy > m_ymax)
		m_ymax = ys + maxDy;
}

void MixedModelBase::computeYCoords()
{
	NodeArray<int> &x = m_gridLayout.x(), &y = m_gridLayout.y();

	int k, i;

	// representation of the contour
	NodeArray<node> prev(m_PG), next(m_PG);

	// initialization
	SetYCoords setY(m_PG,m_iops,m_mmo,x,y);

	const ShellingOrderSet &V1 = m_mmo[1];
	int p = V1.len();

	for (i = 1; i <= p; ++i) {
		if (i < p) next[V1[i]] = V1[i+1];
		if (i > 1) prev[V1[i]] = V1[i-1];
	}
	prev [V1[1]] = next [V1[p]] = 0;

	// main loop
	for (k = 2; k <= m_mmo.length(); ++k)
	{
		// consider set Vk
		const ShellingOrderSet &Vk = m_mmo[k];
		p = Vk.len();
		node cl = m_mmo.m_left[k];
		node cr = m_mmo.m_right[k];

		setY.init(k);

		for(node v = cl; v != next[cr]; v = next[v])
		{
			ListConstIterator<InOutPoint> itFirst, itLast;

			const List<InOutPoint> &out = m_iops.outpoints(v);

			if (v == cl) {
				if (!(itFirst = m_leftOp[k]).valid())
					itFirst = out.begin();
				else if ((*itFirst).m_adj->twinNode() != Vk[1])
					++itFirst;

				for(itLast = itFirst; itLast.valid() && (m_iops.marked((*itLast).m_adj) ||
					(*itLast).m_adj->twinNode() == Vk[1]); ++itLast) ;

			} else if (v == cr) {
				itLast = m_rightOp[k];
				if (itLast.valid() && (*itLast).m_adj->twinNode() == Vk[p])
					++itLast;
				itFirst = (itLast.valid()) ? itLast.pred() : out.rbegin();

				while(itFirst.valid() && (m_iops.marked((*itFirst).m_adj) ||
					(*itFirst).m_adj->twinNode() == Vk[p]))
					--itFirst;
				itFirst = (itFirst.valid()) ? itFirst.succ() : out.begin();

			} else {
				itFirst = m_nextLeft[v];
				itFirst = (itFirst.valid()) ? itFirst.pred() : out.rbegin();

				while (itFirst.valid() && m_iops.marked((*itFirst).m_adj))
					--itFirst;

				itFirst = (itFirst.valid()) ? itFirst.succ() : out.begin();

				if (m_nextRight[v].valid() && m_nextLeft[v] == m_nextRight[v]) {
					for (itLast = m_nextRight[v].succ(); itLast.valid() && m_iops.marked((*itLast).m_adj);
						++itLast) ;
				} else {
					itLast = m_nextLeft[v];
				}
			}
			if (v != cr && itFirst != itLast && m_mmo.rank(next[v]) > m_mmo.rank(v)) {
				int x_n_v = x[next[v]] - m_iops.outLeft(next[v]);
				ListConstIterator<InOutPoint> it = (itLast.valid()) ? itLast.pred() : out.rbegin();
				for( ; ; ) {
					if(x[v]+(*it).m_dx >= x_n_v)
						itLast = it;
					else break;
					if (it == itFirst) break;
					--it;
				}
			}

			if (v != cl) {
				int xl = x[prev[v]], xr = x[v];
				int r_p_v = m_mmo.rank(prev[v]), r_v = m_mmo.rank(v);

				if (r_p_v >= r_v) {
					if (m_iops.out(prev[v]) > 0)
						xl += 1+m_iops.outRight(prev[v]);
				} else {
					xl += m_dxla[v];
				}
				if (r_p_v <= r_v) {
					if (m_iops.out(v) > 0)
						xr -= 1+m_iops.outLeft(v);
				} else {
					xr += m_dxra[prev[v]];
				}

				if (xl <= xr)
					setY.checkYCoord(xl, xr, 1+max(y[prev[v]],y[v]), false);
			}

			for (ListConstIterator<InOutPoint> it = itFirst; it != itLast; ++it) {
				const InOutPoint &op = *it;

				if (m_iops.marked(op.m_adj))
					setY.checkYCoord(x[v]+op.m_dx, y[v]+op.m_dy+1, false);
				else
					setY.checkYCoord(x[v]+op.m_dx, y[v]+op.m_dy,
						(op.m_dx == 0 && op.m_dy == 0 && x[v] == x[op.m_adj->twinNode()]));
			}
		}

		for (i = 1; i <= p; i++) {
			y[Vk[i]] = setY.getYmax();
		}

		// update contour after insertion of z1,...,zp
		for (i = 1; i <= p; i++) {
			if (i < p) next[Vk[i]] = Vk[i+1];
			if (i > 1) prev[Vk[i]] = Vk[i-1];
		}

		next [prev [Vk[1]] = cl] = Vk[1];
		prev [next [Vk[p]] = cr] = Vk[p];
	}
}


/********************************************************************
							setBends

	Assigns polylines to edges of the original graph and computes the
	x- and y-coordinates of deg-1-nodes not in the ordering.
********************************************************************/

void MixedModelBase::setBends ()
{
	//printMMOrder(cout);
	//printInOutPoints(cout);
	//cout.flush();

	NodeArray<int> &x = m_gridLayout.x(), &y = m_gridLayout.y();
	EdgeArray<IPolyline> &bends = m_gridLayout.bends();

	for(int k = 1; k <= m_mmo.length(); ++k)
	{
		for (int i = 1; i <= m_mmo[k].len(); ++i)
		{
			node v_s = m_mmo(k,i);
			adjEntry adj;
			forall_adj(adj,v_s)
			{
				node v_t = adj->twinNode();
				edge e = adj->theEdge();
				const InOutPoint &p_s = *m_iops.pointOf(adj);
				if (m_iops.marked(adj)) {
					x[v_t] = x[v_s] + p_s.m_dx;
					y[v_t] = y[v_s] + p_s.m_dy;
				}
				else if(e->source() == adj->theNode())
				{
					const InOutPoint &p_t = *m_iops.pointOf(adj->twin());
					IPoint p1 (x[v_s] + p_s.m_dx, y[v_s] + p_s.m_dy);
					IPoint p2 (x[v_t] + p_t.m_dx, y[v_t] + p_t.m_dy);

					bends[e].pushBack(p1);
					if (m_mmo.rank(v_s) < m_mmo.rank(v_t)) {
						bends[e].pushBack(IPoint(p1.m_x,p2.m_y));
					} else {
						bends[e].pushBack(IPoint(p2.m_x,p1.m_y));
					}
					bends[e].pushBack(p2);
				}
			}
		}
	}
}


/********************************************************************
							postprocessing1

	Tries to reduce the number of bends by changing the outpoints of
	nodes with indeg and outdeg 2.
********************************************************************/

void MixedModelBase::postprocessing1()
{
	NodeArray<int> &x = m_gridLayout.x();

	for(int k = 2; k <= m_mmo.length(); ++k) {
		const ShellingOrderSet &V = m_mmo[k];
		node v = V[V.len()];

		if (m_iops.in(v) != 2 || m_iops.out(v) != 2) continue;

		const List<InOutPoint> &in  = m_iops.inpoints (v);
		List<InOutPoint> &out = m_iops.outpoints(v);
		adjEntry adjL = (*in.begin ()).m_adj;
		adjEntry adjR = (*in.rbegin()).m_adj;

		if (!m_iops.marked(adjL) && !m_iops.marked(adjR) &&
			x[adjL->twinNode()] + m_iops.pointOf(adjL->twin())->m_dx < x[v] &&
			x[adjR->twinNode()] + m_iops.pointOf(adjR->twin())->m_dx == x[v]+1 &&
			m_gridLayout.y(adjR->twinNode()) < m_gridLayout.y(v))
		{
			x[v] += 1;
			m_iops.setOutDx(out.begin (),-1);
			m_iops.setOutDx(out.rbegin(), 0);
		}
	}
}


/********************************************************************
							postprocessing2

	Tries to reduce the number of bends by moving degree-2 nodes on
	bend points.
********************************************************************/

void MixedModelBase::firstPoint(int &x, int &y, adjEntry adj)
{
	edge e = adj->theEdge();
	bool sameDirection = (adj->theNode() == e->source());

	if (m_gridLayout.bends(e).empty()) {
		node t = (sameDirection) ? e->target() : e->source();
		x = m_gridLayout.x(t);
		y = m_gridLayout.y(t);
	} else {
		if(sameDirection) {
			x = m_gridLayout.bends(e).front().m_x;
			y = m_gridLayout.bends(e).front().m_y;
		} else {
			x = m_gridLayout.bends(e).back().m_x;
			y = m_gridLayout.bends(e).back().m_y;
		}
	}
}

bool MixedModelBase::isRedundant(int x1, int y1, int x2, int y2, int x3, int y3)
{
	int dzy1 = x3 - x2;
	int dzy2 = y3 - y2;
	int dyx1 = x2 - x1;

	if (dzy1 == 0) return (dyx1 == 0);

	int f = dyx1 * dzy2;

	return (f % dzy1 == 0 && (y2 - y1) == f / dzy1);
}

void MixedModelBase::postprocessing2()
{
	m_gridLayout.compactAllBends();

	node v;
	forall_nodes(v,m_PG)
	{
		if(v->degree() != 2) continue;

		adjEntry adj1 = v->firstAdj();
		edge e1 = adj1->theEdge();
		adjEntry adj2 = v->lastAdj();
		edge e2 = adj2->theEdge();

		IPolyline &bends1 = m_gridLayout.bends(e1);
		IPolyline &bends2 = m_gridLayout.bends(e2);

		if (bends1.empty() && bends2.empty()) continue;

		int x1,y1,x3,y3;
		firstPoint(x1,y1,adj1);
		firstPoint(x3,y3,adj2);

		if (isRedundant(x1,y1,m_gridLayout.x(v),m_gridLayout.y(v),x3,y3)) {
			if (!bends1.empty()) {
				m_gridLayout.x(v) = x1;
				m_gridLayout.y(v) = y1;
				if(adj1->theNode() == e1->source())
					bends1.popFront();
				else
					bends1.popBack();
			} else {
				m_gridLayout.x(v) = x3;
				m_gridLayout.y(v) = y3;
				if(adj2->theNode() == e2->source())
					bends2.popFront();
				else
					bends2.popBack();
			}
		}
	}
}


/********************************************************************
						debugging output
********************************************************************/

void MixedModelBase::printMMOrder(ostream &os)
{
	int k, i;

	os << "left and right:\n\n";
	for (k = 1; k <= m_mmo.length(); ++k)
	{
		const ShellingOrderSet &V = m_mmo[k];

		os << k << ": { ";
		for (i = 1; i <= V.len(); i++)
			os << V[i] << " ";
		os << "};";
		if (k >= 2)
			os << " cl = " << m_mmo.m_left[k] <<
				", cr = " << m_mmo.m_right[k];
		os << endl;

	}
	os.flush();
}

void MixedModelBase::printInOutPoints(ostream &os)
{
	node v;

	os << "\n\nin- and outpoint lists:\n";
	forall_nodes(v,m_PG) {
		const List<InOutPoint> &in  = m_iops.inpoints (v);
		const List<InOutPoint> &out = m_iops.outpoints(v);

		os << "\n" << v << ":\n";
		os << "  outpoints: ";
		ListConstIterator<InOutPoint> it;
		for(it = out.begin(); it.valid(); ++it) {
			print(os,*it);
			os << " ";
		}
		os << "\n  inpoints:  ";
		for(it = in.begin(); it.valid(); ++it) {
			print(os,*it);
			os << " ";
		}
	}
	os << endl;
}

void MixedModelBase::print(ostream &os, const InOutPoint &iop)
{
	if(iop.m_adj)
		os << "[(" << m_PG.original(iop.m_adj->theNode()) << "," <<
			m_PG.original(iop.m_adj->twinNode()) << ")," <<
			iop.m_dx << "," << iop.m_dy << "]";
	else
		os << "[ ]";
}

void MixedModelBase::printNodeCoords(ostream &os)
{
	node v;

	os << "\nx- and y-coordinates:\n\n";
	forall_nodes(v,m_PG)
		os << v << ": (" << m_gridLayout.x(v) << "," << m_gridLayout.y(v) << ")\n";
}

} // end namespace ogdf
