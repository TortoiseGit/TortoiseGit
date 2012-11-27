/*
 * $Revision: 2552 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-05 16:45:20 +0200 (Do, 05. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of the static functions.
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


#include <ogdf/module/HierarchyLayoutModule.h>
#include <ogdf/basic/Queue.h>

namespace ogdf {

void HierarchyLayoutModule::addBends(GraphCopyAttributes &AGC, Hierarchy &H) {

	EdgeArray<int> done(H, -1);
	NodeArray<bool> dirty(H, false);

	for (int i = 0; i <= H.high(); i++) { // all level
		const Level &lvl_cur = H[i];

		/*
			compute the max. height (y coord.) of the bounding boxex of lvl_cur
		*/
		// y coord. of the top
		double y_h = AGC.y(lvl_cur[0]) + AGC.getHeight(lvl_cur[0])/2;
		//y coord. of the bottom
		double y_l = AGC.y(lvl_cur[0]) - AGC.getHeight(lvl_cur[0])/2;

		//iterate over all nodes and find the max. height
		for(int j = 0; j <= lvl_cur.high(); j++) {
			node v = lvl_cur[j];
			double a = AGC.y(v) - AGC.getHeight(v)/2;
			double b = AGC.y(v) + AGC.getHeight(v)/2;
			if (y_h < b )
				y_h = b;
			if (y_l > a)
				y_l = a;
		}

		// list with edges, which muss be bended later
		List<edge> bendMe;

		for(int j = 0; j <= lvl_cur.high(); j++) {
			node v = lvl_cur[j];

			adjEntry adj;

			/*
			compute the edges, which overlap a node in lvl_cur
			*/
			forall_adj(adj, v) {
				edge e = adj->theEdge();

				if (done[e] == i)
					continue; // already bended

				node w = e->target();
				if (w == v)
					w = e->source();

				node nodeLeft = v, nodeRight = w;

				if (AGC.x(v) == AGC.x(w))
					continue; // edge e cannot overlap a node

				if (AGC.x(v) > AGC.x(w))
					swap(nodeLeft, nodeRight);

				DLine line_v2w( DPoint(AGC.x(v), AGC.y(v)), DPoint(AGC.x(w), AGC.y(w)) );

				//iterate over all node of lvl_cur and find out wether line_v2w overlap a node or not
				for(int k = 0; k <= lvl_cur.high(); k++) {
					node u = lvl_cur[k];

					if (u == v)
						continue;

					int ci = 0;
					int cj = 0;

					overlap(AGC, H, e->source(), e->target(), i, ci, cj);

					if (ci > 0)
						bendMe.pushBack(e);

				} // for k

			}// forall_adj
		}

		NodeArray<bool> isNewNode(H.m_GC, false);
		while (!bendMe.empty()) {
			edge splitMe = bendMe.popFrontRet();

			if (done[splitMe] == i)
				continue; //already bended

			// coord of the new bend point
			double bendX, bendY = y_h;
			// represents a incomming edges, i.e. the target of splitMe is on current level i
			bool incomming = false;
			// v is the node of splitMe on current level i
			node v;
			v = splitMe->source();
			node t = splitMe->target();

			if (H.rank(v) != i) {
				v = t;
				incomming = true;
			}

			// compute coord. of the new bend
			if (incomming)
				bendY = y_l;

			// the x coord. of splitMe is smaler than of the x coord. of the source, i.e the segment is pointing upward from left to right
			bool toRight = true;
			if (AGC.x(splitMe->source()) > AGC.x(splitMe->target()))
				toRight = false;

			if (H.isLongEdgeDummy(v))
				// long edge dummy v, just add a new bend point "above" v
				bendX = AGC.x(v);
			else {
				// we have to compute the x coord. of the new bend point and ensure that other bend point do not have the same coord. assigned

				//the "neighbour" of node v, the bend points are placed between w and v if some edges of v are bended
				node w = 0;

				if (toRight && incomming && H.pos(v) != 0)
					w = lvl_cur[H.pos(v) - 1];

				if (toRight && !incomming && H.pos(v) != lvl_cur.high())
					w = lvl_cur[H.pos(v) + 1];

				if (!toRight && incomming && H.pos(v) != lvl_cur.high())
					w = lvl_cur[H.pos(v) + 1];

				if (!toRight && !incomming && H.pos(v) != 0)
					w = lvl_cur[H.pos(v) - 1];

				//number of edges, which crossed the area between v und w
				int num = 0;
				int edgeNum = 1;

				adjEntry adj;
				forall_adj(adj, v) {
					edge eTmp = adj->theEdge();

					if (incomming && eTmp->target() == v) {
						node src = eTmp->source();
						if (isNewNode[src]) {
							src = eTmp->adjSource()->cyclicSucc()->theEdge()->source();
							if (isNewNode[src])
								src = eTmp->adjSource()->cyclicSucc()->theEdge()->adjSource()->cyclicSucc()->theEdge()->source();
						}

						if (AGC.x(src) < AGC.x(v) && toRight) {
							if (AGC.x(src) < AGC.x(splitMe->source()))
								edgeNum++;
						}
						else {
							if (AGC.x(src) > AGC.x(v) && !toRight) {//in-edges from right to left
								if (AGC.x(src) < AGC.x(splitMe->source()))
									edgeNum++;
							}
						}
					}
					else {
						node tgt = eTmp->target();
						if (isNewNode[tgt]) {
							tgt = eTmp->adjTarget()->cyclicSucc()->theEdge()->target();
							if (isNewNode[tgt])
								tgt = eTmp->adjTarget()->cyclicSucc()->theEdge()->adjTarget()->cyclicSucc()->theEdge()->target();
						}

						if (AGC.x(tgt) < AGC.x(v) && !toRight) {//out-edges from left to right
							if (AGC.x(splitMe->target()) > AGC.x(tgt) )
								edgeNum++;
						}
						else {
							if (AGC.x(tgt) > AGC.x(v) && toRight) { //in-edges from right to left
								if (AGC.x(splitMe->target()) > AGC.x(tgt) )
									edgeNum++;
							}
						}
					}

					if (incomming && toRight && eTmp->target() == v && AGC.x(eTmp->source()) > AGC.x(eTmp->target()))
						num++;

					if (incomming && !toRight && eTmp->target() == v && AGC.x(eTmp->source()) < AGC.x(eTmp->target()))
						num++;

					if (!incomming && toRight && eTmp->source() == v && AGC.x(eTmp->source()) < AGC.x(eTmp->target()))
						num++;

					if (!incomming && !toRight && eTmp->source() == v && AGC.x(eTmp->source()) < AGC.x(eTmp->target()))
						num++;
				}

				// default value if w is null
				double delta = 10;
				double a = AGC.x(v) - AGC.getWidth(v)/2;
				double b = a;
				if (w != 0) {
					b = AGC.x(w) + AGC.getWidth(w)/2;
					if (AGC.x(v) < AGC.x(w)) {
						a = AGC.x(v) + AGC.getWidth(v)/2;
						b = AGC.x(w) - AGC.getWidth(w)/2;
					}
					forall_adj(adj, w) {
						edge eTmp = adj->theEdge();
						if (incomming && toRight && eTmp->target() == w && AGC.x(eTmp->source()) < AGC.x(eTmp->target()))
							num++;

						if (incomming && !toRight && eTmp->target() == w && AGC.x(eTmp->source()) > AGC.x(eTmp->target()))
							num++;

						if (!incomming && toRight && eTmp->source() == w && AGC.x(eTmp->source()) < AGC.x(eTmp->target()))
							num++;

						if (!incomming && !toRight && eTmp->source() == w && AGC.x(eTmp->source()) > AGC.x(eTmp->target()))
							num++;
					}

					delta = fabs(a - b)/(num+3);
					if (AGC.x(v) < AGC.x(w))
						bendX = a + edgeNum * delta;
					else
						bendX = b + edgeNum * delta;
				}
				else {
					if (H.pos(v)==0)
						bendX = b + edgeNum * delta;
					else
						bendX = a + edgeNum * delta;
				}
				if (H.pos(v) % 2 != 0)
						bendX = bendX + delta/2;
			}// else x coord

			DLine segment1, segment2;

			//replace v if it is a bend point
			double oldPosX = AGC.x(v);
			double oldPosY = AGC.y(v);
			bool ok = false;

			// replace v or add new bend if v is long edge dummy
			if (H.isLongEdgeDummy(v)) {
				AGC.y(v) = bendY;
				AGC.x(v) = bendX;
				int c_out = 0;
				int c_in = 0;

				edge e_out = v->firstAdj()->theEdge();
				edge e_in = v->lastAdj()->theEdge();

				if (e_out->source() != v)
					swap(e_out, e_in);

				overlap(AGC, H, e_out->source(), e_out->target(), i, c_out, c_out);

				overlap(AGC, H, e_in->source(), e_in->target(), i, c_in, c_in);

				// add new bend point
				if (c_in + c_out == 0)
					ok = true;
			}

			if (ok && !dirty[v]) {

				done[v->firstAdj()] = done[v->lastAdj()] = i;

				edge e1 = v->firstAdj()->theEdge();
				edge e2 = v->lastAdj()->theEdge();

				segment1 = DLine( DPoint(AGC.x(e1->source()), AGC.y(e1->source())),
							DPoint(AGC.x(e1->target()), AGC.y(e1->target())) );

				segment2 = DLine( DPoint(AGC.x(e2->source()), AGC.y(e2->source())),
							DPoint(AGC.x(e2->target()), AGC.y(e2->target())) );

				dirty[v] = true;
			}
			else {
				//retore old position
				AGC.y(v) = oldPosY;
				AGC.x(v) = oldPosX;

				// create a new bend point by splitting splitMe
				node newNode = H.m_GC.split(splitMe)->source();
				isNewNode[newNode] = true;
				AGC.y(newNode) = bendY;
				AGC.x(newNode) = bendX;
				done[newNode->firstAdj()] = done[newNode->lastAdj()] = i;

				edge e1 = newNode->firstAdj()->theEdge();
				edge e2 = newNode->lastAdj()->theEdge();

				/*
					compute edges which crossed the two new segment of the bended edge
				*/
				segment1 = DLine( DPoint(AGC.x(e1->source()), AGC.y(e1->source())),
								DPoint(AGC.x(e1->target()), AGC.y(e1->target())) );

				segment2 =DLine( DPoint(AGC.x(e2->source()), AGC.y(e2->source())),
								DPoint(AGC.x(e2->target()), AGC.y(e2->target())) );
			}

			for(int z = 0; z <= lvl_cur.high(); z++) {
				node uu = lvl_cur[z];

				adjEntry adj;
				forall_adj(adj, uu) {
					edge ee = adj->theEdge();

					DLine line_ee( DPoint(AGC.x(ee->source()), AGC.y(ee->source())), DPoint(AGC.x(ee->target()), AGC.y(ee->target()) ) );

					DPoint dummy;
					if (line_ee.intersection(segment1, dummy) && line_ee.intersection(segment2, dummy))
						bendMe.pushBack(ee);
				}
			}
		} // while

	}//for i (all level)

}




void HierarchyLayoutModule::dynLayerDistance(GraphCopyAttributes &AGC, Hierarchy &H) {
	if (H.high() < 1)
		return;

	// min. angle between horizon and an edge segment
	double minAngle = 0.087266; //=5 degree

	double y_low = AGC.y(H[0][0]);
	double maxH_low = 0; // the height of the node with maximal height on lvl i-1

	const Level &lvl0 = H[0];
	for (int j = 0; j <= lvl0.high(); j++) {
		node v = lvl0[j];
		if (maxH_low < AGC.getHeight(v))
			maxH_low = AGC.getHeight(v);
	}

	for (int i = 1; i <= H.high(); i++) { // all level
		const Level &lvl = H[i];
		const Level &lvl_low = H[i-1];
		double y_cur = AGC.y(lvl[0]); //current y-coord. of the lvl
		double maxH_cur = 0;
		int count = 0; //number of edges, which overlap a node

		for (int j = 0; j <= lvl.high(); j++) {
			node v = lvl[j];

			if (maxH_cur < AGC.getHeight(v))
				maxH_cur = AGC.getHeight(v);

			adjEntry adj;
			int ci = 0;
			int cj = 0;
			forall_adj(adj, v) {
				edge e = adj->theEdge();
				node w = e->source();

				if (w == v)
					continue; // only incoming edges

				if (AGC.x(v) == AGC.x(w))
					continue; // edge e cannot overlap a node

				overlap(AGC, H, e->source(), e->target(), i, ci, cj);

				DLine line_v2w( DPoint(AGC.x(v), AGC.y(v)), DPoint(AGC.x(w), AGC.y(w)) );
				count = count + ci + cj;
			}
		}


		//	node distance contrain; node on lvl should not overlap node of the lvl below
		double diff = (y_cur - maxH_cur/2) - (y_low + maxH_low/2);
		double newY = y_cur;
		if ( diff < 0 ) {
			newY = newY - diff;
		}


		//min. angle constrain
		double delta_x = fabs(AGC.x(lvl[0]) -  AGC.x(lvl_low[lvl_low.high()]));
		double minH = tan(minAngle) * delta_x;
		diff = (newY - maxH_cur/2) - (y_low + maxH_low/2);
		if (diff < 0) {
			newY = newY + fabs(diff - minH);
		}

		// compute the number of long edges between lvl and lvl_low
		double numEdge = 0;
		for (int j = 0; j <= lvl.high(); j++) {
			node v = lvl[j];

			if (v->indeg() == 0)
				continue;

			adjEntry adj;
			forall_adj(adj, v) {
				edge e = adj->theEdge();
				node w = e->source();
				if (w == v)
					continue; // only incoming edges

				DLine line_v2w( DPoint(AGC.x(v), AGC.y(v)), DPoint(AGC.x(w), AGC.y(w)) );
				if (line_v2w.length()>3*(y_cur - y_low))
					numEdge++;
			}
		}

		//increase visibility, if there are a lot of edges overlap nodes
		double factor = 0;
		if (count >= 1 && 3 <= count)
			factor = 0.4;
		if ( count > 3)
			factor = 0.8;

		// factor depends on the number of very long edges
		if (numEdge <= 3 && numEdge>=1)
			factor = 0.5;
		if (numEdge >3 && numEdge<7)
			factor = 1.5;
		if (numEdge > 7)
			factor = 2;

		newY = newY + (y_cur - y_low) * factor;

		//assign new y coord.
		if (newY != y_cur) {
			double b = fabs(newY - y_cur);
			for (int ii = i; ii <= H.high(); ii++) {
				const Level &lvlTmp = H[ii];
				for (int j = 0; j <= lvlTmp.high(); j++) {
					node z = lvlTmp[j];
					AGC.y(z) = AGC.y(z) + b;
				}
			}
		}

		y_low = newY;
	}
}


void HierarchyLayoutModule::overlap(ogdf::GraphCopyAttributes &AGC, ogdf::Hierarchy &H, ogdf::node s, ogdf::node t, int i, int &ci, int &cj) {
	const Level &lvl_cur = H[i];
	DLine line(DPoint(AGC.x(s), AGC.y(s)), DPoint(AGC.x(t), AGC.y(t)));

	//iterate over all node of level lvl_cur
	for(int k = 0; k <= lvl_cur.high(); k++) {
		node u = lvl_cur[k];

		if (u == s || u== t || H.isLongEdgeDummy(u))
			continue;

		double h = AGC.getHeight(u);
		double b = AGC.getWidth(u);

		//bounding box of the node u
		DLine left(DPoint(AGC.x(u)-b/2, AGC.y(u)-h/2), DPoint(AGC.x(u)-b/2, AGC.y(u)+h/2));
		DLine right(DPoint(AGC.x(u)+b/2, AGC.y(u)-h/2), DPoint(AGC.x(u)+b/2, AGC.y(u)+h/2));
		DLine bottom(DPoint(AGC.x(u)-b/2, AGC.y(u)-h/2), DPoint(AGC.x(u)+b/2, AGC.y(u)-h/2));

		DPoint ipoint;
		bool intersecLeft = line.intersection(left, ipoint);
		bool intersecRight = line.intersection(right, ipoint);
		bool intersectBottom = line.intersection(bottom, ipoint);

		if (intersecLeft || intersecRight || intersectBottom)
			ci++;;

	}

	if (i-1 >= 0) {
		const Level &lvl_low = H[i-1];

		//iterate over all node of lvl_low
		for(int k = 0; k <= lvl_low.high(); k++) {
			node u = lvl_low[k];

			if (u == s || u == t || H.isLongEdgeDummy(u))
				continue;

			double h = AGC.getHeight(u);
			double b = AGC.getWidth(u);

			//bounding box of the node u
			DLine left(DPoint(AGC.x(u)-b/2, AGC.y(u)-h/2), DPoint(AGC.x(u)-b/2, AGC.y(u)+h/2));
			DLine right(DPoint(AGC.x(u)+b/2, AGC.y(u)-h/2), DPoint(AGC.x(u)+b/2, AGC.y(u)+h/2));
			DLine bottom(DPoint(AGC.x(u)-b/2, AGC.y(u)-h/2), DPoint(AGC.x(u)+b/2, AGC.y(u)-h/2));

			DPoint ipoint;
			bool intersecLeft = line.intersection(left, ipoint);
			bool intersecRight = line.intersection(right, ipoint);
			bool intersectBottom = line.intersection(bottom, ipoint);

			if (intersecLeft || intersecRight || intersectBottom)
				cj++;
		}
	}
}

}
